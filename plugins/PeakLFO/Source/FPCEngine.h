// FPCLfo.h — 1:1 reconstruction of the Fruity Peak Controller LFO + peak follower.
// Derived from Fruity Peak Controller_x64.dll (see FPC_LFO_SPEC.md for provenance).
// Single-file, dependency-free. Output range matches FL: 0 .. 2^30, plus a 0..1 helper.
#pragma once
#include <algorithm>
#include <cstdint>
#include <cmath>
#include <cstdlib>
#include <vector>

#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif

class FPCEngine
{
public:
    static constexpr int    kTableSize = 16384;      // phase>>18 → 14-bit index
    static constexpr double kFullScale = 1073741824.0; // 0x40000000 (2^30)

    enum Shape { Sine = 0, Triangle = 1, Square = 2, Saw = 3, Random = 4, kNumShapes = 5 };

    // ---- output model (matches FPC): out = Base + shape * Volume ----
    // Volume knob: bipolar, exponential taper (FPC 6^ family); sign-flips the wave.
    static constexpr double kVolTaperBase = 6.0;
    static float volumeTaper (float knobBipolar)   // -1..1 -> -1..1 tapered
    {
        const float sgn = (knobBipolar < 0.0f) ? -1.0f : 1.0f;
        const float a   = std::abs (knobBipolar);
        const double t  = (std::pow (kVolTaperBase, (double) a) - 1.0) / (kVolTaperBase - 1.0);
        return sgn * (float) t;
    }
    static float outputGain (float base, float shape01, float volTapered)
    {
        const float g = base + shape01 * volTapered;
        return g < 0.0f ? 0.0f : (g > 1.0f ? 1.0f : g);
    }

    void prepare(double sampleRate)
    {
        sr = (int)sampleRate;
        buildTables();
        setShape(Sine);
        recalcSpeed();
        recalcDecay();
        recalcAmountTension(/*peak*/ true);
        recalcAmountTension(/*peak*/ false);
    }

    // ---- raw parameters (match FL param indices; ints as FL stores them) ----
    void setPeakBase (int v){ pPeakBase = v; }
    void setPeakAmount(int v){ pPeakAmt = v; recalcAmountTension(true); }
    void setPeakTension(int v){ pPeakTens = v; recalcAmountTension(true); }
    void setPeakDecay(int v){ pPeakDecay = v; recalcDecay(); }
    void setLfoBase  (int v){ pLfoBase = v; }
    void setLfoAmount(int v){ pLfoAmt = v; recalcAmountTension(false); }
    void setLfoTension(int v){ pLfoTens = v; recalcAmountTension(false); }
    void setLfoSpeed (int v){ pLfoSpeed = v; recalcSpeed(); }
    void setShape    (int s){ pShape = s; table = tables[(s>=0&&s<kNumShapes)?s:0].data(); recalcSpeed(); }
    void setLfoPhase (int v){ pLfoPhase = v; }

    // ---- audio: call per input buffer (instant attack) ----
    void pushAudioPeak(float peakAbsL, float peakAbsR)
    {
        if (peakAbsL > peakLevel) peakLevel = peakAbsL;
        if (peakAbsR > peakLevel) peakLevel = peakAbsR;
    }

    struct Out { int32_t combined, peak, lfo; };

    // ---- control tick: returns the three controller values (0..2^30) ----
    Out tick()
    {
        // ---- peak half ----
        float ps = shapeTension(peakLevel, peakTensMag, peakTensSign);
        int64_t peakOut = llround((double)pPeakBase * 16384.0 + (double)(ps * peakAmount));

        // ---- lfo half ----
        uint32_t idx = phase >> 18;
        float s = (table[idx] + 1.0f) * 0.5f;
        float ls = shapeTension(s, lfoTensMag, lfoTensSign);
        int64_t lfoOut = llround((double)pLfoBase * 16384.0 + (double)(ls * lfoAmount));
        phase += inc;

        int64_t comb = peakOut + lfoOut;
        Out o { (int32_t)clampFS(comb), (int32_t)clampFS(peakOut), (int32_t)clampFS(lfoOut) };

        // linear peak decay
        peakLevel -= decayCoef;
        if (peakLevel <= 0.0f) peakLevel = 0.0f;
        return o;
    }

    // optional: reset LFO phase from song position (PPQ) like FUN_007062a0
    void syncPhase(double ppqPos)
    {
        phase = (uint32_t)((int64_t)llround(ppqPos) * (int64_t)inc) + (uint32_t)(pLfoPhase * 0x10000);
    }

    static double toUnipolar(int32_t v){ return (double)v * 9.313225746154785e-10; } // *2^-30

    // --- debug/inspection accessors (safe to keep; not part of DSP path) ---
    float    dbgTable(int shape,int i) const { return tables[std::clamp(shape,0,kNumShapes-1)][i & (kTableSize-1)]; }
    uint32_t dbgInc() const { return inc; }
    double   dbgFreeRunHz() const { return inc ? (double)inc / 4294967296.0 * (double)sr : 0.0; }

    // Phase-driven LFO evaluation (for tempo-synced use): phase01 in [0,1) →
    // unipolar [0,1] sample of the current shape with the exact FL tension curve applied.
    float evalLfoUnipolar(double phase01) const
    {
        if (table == nullptr) return 0.5f;
        phase01 -= std::floor(phase01);
        uint32_t idx = (uint32_t)(phase01 * (double)kTableSize) & (uint32_t)(kTableSize - 1);
        float s = (table[idx] + 1.0f) * 0.5f;
        return shapeTension(s, lfoTensMag, lfoTensSign);
    }

private:
    // ---------- tension warp ----------
    // NORMALISED FL tension curve: maps s in [0,1] -> [0,1], endpoints fixed, so the warp
    // reshapes the LFO WITHOUT changing its amplitude (FL achieves this by dividing amount
    // by T; we bake the /T into the warp instead). sign flips the curvature; T=0 -> identity.
    //   positive: (T - ((T+1)^(1-s) - 1)) / T      (convex / peaked)
    //   negative: ((T+1)^s - 1) / T                (concave)
    static float shapeTension(float s, float T, int sign)
    {
        if (sign == 0 || T <= 0.0f) return s;
        const double Td = (double) T;
        if (sign > 0) return (float)((Td - (std::pow(Td + 1.0, 1.0 - (double) s) - 1.0)) / Td);
        return          (float)((std::pow(Td + 1.0, (double) s) - 1.0) / Td);
    }

    // Tension magnitude mapping (raw tension int -> T). Coefficients are TUNABLE — set to the
    // FL-decompiled literals; Phase 3 confirms/locks them and reports residual.
    static constexpr double kTensBase = 1001.0;   // curve base
    static constexpr double kTensDiv  = 128.0;    // raw divisor
    static constexpr double kTensAmt  = 0.1;      // magnitude scale
    static float tensionMag(int rawTens)
    {
        return (float)((std::pow(kTensBase, std::abs((double) rawTens) / kTensDiv) - 1.0) * kTensAmt);
    }

    void recalcSpeed()
    {
        double x = (double)pLfoSpeed / 65536.0;
        double r = std::pow(1001.0, x);
        double period = std::floor(((r - 1.0) * (double)(sr >> 2) / 48.0 * 24576.0 / 1000.0) + 0.5);
        int64_t p = (int64_t)period;
        if (p < 4) p = 3;
        inc = (uint32_t)(0x100000000ULL / (uint64_t)p);
    }

    void recalcDecay()
    {
        double c = (std::pow(10001.0, (double)pPeakDecay / 128.0) - 1.0) * 0.0192 / (double)sr;
        decayCoef = (float)c;
    }

    void recalcAmountTension(bool peak)
    {
        int amtRaw  = peak ? pPeakAmt  : pLfoAmt;
        int tensRaw = peak ? pPeakTens : pLfoTens;
        const double amtScale = peak ? 429496736.0 : 214748368.0;

        double amt = std::pow(6.0, std::abs((double)amtRaw) / 256.0) - 1.0;
        if (amtRaw < 0) amt = -amt;
        double amount = amt * amtScale;

        int sign = (tensRaw == 0) ? 0 : (tensRaw < 0 ? -1 : +1);
        float Tmag = 0.0f;
        if (sign != 0)
        {
            Tmag = tensionMag(tensRaw);                 // tunable T-mapping (see tensionMag)
            if (Tmag > 0.0f) amount /= (double) Tmag;   // legacy peak-path amount coupling (unused by LFO path)
        }
        if (peak){ peakAmount=(float)amount; peakTensMag=Tmag; peakTensSign=sign; }
        else     { lfoAmount =(float)amount; lfoTensMag =Tmag; lfoTensSign =sign; }
    }

    void buildTables()
    {
        for (auto& t : tables) t.resize(kTableSize);
        for (int i=0;i<kTableSize;++i)
        {
            double ph = (double)i / kTableSize;                 // 0..1
            tables[Sine][i]     = (float)std::sin(2.0*M_PI*ph);
            tables[Triangle][i] = (float)(ph<0.25? 4*ph : ph<0.75? 2-4*ph : 4*ph-4); // -1..1
            tables[Square][i]   = ph < 0.5f ? 1.0f : -1.0f;
            tables[Saw][i]      = (float)(2.0*ph - 1.0);        // rising ramp -1..1
        }
        // Random = fixed sample&hold noise table (regenerate once, deterministic)
        uint32_t rng = 0x12345678;
        for (int i=0;i<kTableSize;++i){ rng = rng*1664525u+1013904223u; tables[Random][i] = (float)((int)(rng>>1)/1073741823.0 - 1.0); }
    }

    static int64_t clampFS(int64_t v){ return v<0?0:(v>1073741824?1073741824:v); }

    int sr = 44100;
    // params
    int pPeakBase=0,pPeakAmt=0,pPeakTens=0,pPeakDecay=0;
    int pLfoBase=0,pLfoAmt=0,pLfoTens=0,pLfoSpeed=0,pShape=0,pLfoPhase=0;
    // derived
    float peakAmount=0,lfoAmount=0, peakTensMag=0,lfoTensMag=0; int peakTensSign=0,lfoTensSign=0;
    float decayCoef=0, peakLevel=0;
    uint32_t phase=0, inc=0;
    std::vector<float> tables[kNumShapes];
    const float* table=nullptr;
};
