#pragma once
//==============================================================================
// MidBass FX chain — Phase 6. Fixed order, each bypassable:
//   Chorus → Phaser → Flanger → Delay (tempo-synced) → Reverb → Compressor.
//
// Chorus/Phaser/Delay/Reverb are the BIT-EXACT fixed-point VAZ engines
// (VazChorusEngine / VazPhaserEngine / VazDelayEngine / VazReverbEngine),
// untouched, via include path; parameter mappings mirror the shipping VAZ*
// wrapper plugins. Flanger and Compressor are ADAPTED ports (their DSP loops
// lifted verbatim from plugins/VAZFlanger and plugins/VAZCompressor — parity
// gates in test_fx.cpp are bit-exact against reference copies of those loops).
//
// LATENCY (condition c): none of these effects has lookahead or FIR delay —
// every dry path is instantaneous — so the plugin's total latency stays the
// saturator's 63 samples, accounted in ONE place (MbSaturator::kLatency),
// regardless of bypass states. Asserted in tests.
//
// BYPASS (condition b): off = the effect's process is not called → bit-exact.
//
// DELAY RETIME (condition d): the VAZDelay-proven strategy — the target tap
// glides through a ~20 ms one-pole and is re-quantised per sample, so a BPM or
// division change RE-TIMES with a brief tape-style pitch glide on the repeats
// (no clicks, no buffer garbage, feedback history preserved). Artifact-gated
// per the Phase 3 isolation lesson.
//
// TAILS/DENORMALS (condition e): the integer engines decay to EXACT zero by
// construction (integer truncation); the float feedback paths (flanger, comp
// envelope) carry explicit flushes (Phase 5 lesson).
//==============================================================================
#include "VazChorusEngine.h"    // ../../VAZChorus/Source
#include "VazPhaserEngine.h"    // ../../VAZPhaser/Source
#include "VazDelayEngine.h"     // ../../VAZDelay/Source
#include "VazReverbEngine.h"    // ../../VAZReverb/Source
#include <vector>
#include <cmath>
#include <algorithm>

namespace mb
{
inline constexpr double kQ23 = 8388608.0;      // VAZ fixed-point full scale

//==============================================================================
// BASS-SAFE split (condition f): complementary one-pole pair — lp + hp == input
// EXACTLY, so routing only the hp band through a modulated effect leaves the
// low end bit-perfect and mono-proof while the effect works above the corner.
// out = lp + effect(hp): the effect's internal dry/wet applies to hp only.
struct BassSplit
{
    static constexpr double kCornerHz = 250.0;
    float lpL = 0.0f, lpR = 0.0f, k = 0.03f;
    void prepare (double sr)
    {
        k = 1.0f - (float) std::exp (-2.0 * 3.14159265358979323846 * kCornerHz / (sr > 0.0 ? sr : 44100.0));
        lpL = lpR = 0.0f;
    }
    void reset() { lpL = lpR = 0.0f; }
    inline void split (float& L, float& R, float& lowL, float& lowR)
    {
        lpL += (L - lpL) * k;
        lpR += (R - lpR) * k;
        if (std::abs (lpL) < 1.0e-20f) lpL = 0.0f;           // denormal flush
        if (std::abs (lpR) < 1.0e-20f) lpR = 0.0f;
        lowL = lpL; lowR = lpR;
        L -= lpL; R -= lpR;                                  // hp band (complementary)
    }
};

//==============================================================================
struct MbChorus
{
    VazChorusEngine e;
    BassSplit bass;
    bool bassSafe = true;                       // parity tests disable (engine untouched either way)
    double sr = 44100.0;

    void prepare (double sampleRate)
    {
        sr = sampleRate > 0.0 ? sampleRate : 44100.0;
        e.clearBuffers();
        e.buildSineLut();
        bass.prepare (sr);
    }
    void reset() { e.clearBuffers(); bass.reset(); }

    // Mapping mirrors plugins/VAZChorus PluginProcessor (delay fixed at a classic
    // ~9 ms ensemble base; triangle LFO mode = VAZ default).
    void setParams (float rateHz, float depth01, float mix01)
    {
        const int srI = (int) std::llround (sr);
        e.base  = ((srI * 50) / 256000) * (46 + 1);                     // ~9 ms @44.1k
        e.mode1 = e.mode2 = 2;                                          // triangle
        e.inc1  = (uint32_t) (int64_t) ((double) rateHz / sr * 4294967296.0);
        e.inc2  = (uint32_t) (int64_t) ((double) rateHz * 1.13 / sr * 4294967296.0);  // dual-LFO beating
        e.level = 0x8000;
        const int32_t d = (int32_t) std::llround ((double) depth01 * 0.04 * sr);
        e.depth = e.level2 = d;
        e.lrPhase = (int32_t) std::llround (0.5 * 1073741824.0);
        e.gain    = std::clamp ((int) std::lround (mix01 * 255.0f), 0, 255);
    }

    inline void processSample (float& L, float& R)
    {
        float lowL = 0.0f, lowR = 0.0f;
        if (bassSafe) bass.split (L, R, lowL, lowR);
        int32_t li = (int32_t) std::llround ((double) L * kQ23);
        int32_t ri = (int32_t) std::llround ((double) R * kQ23);
        e.processFrame (li, ri);
        L = lowL + (float) ((double) li / kQ23);
        R = lowR + (float) ((double) ri / kQ23);
    }
};

//==============================================================================
struct MbPhaser
{
    VazPhaserEngine e;
    BassSplit bass;
    bool bassSafe = true;
    double sr = 44100.0;

    void prepare (double sampleRate)
    {
        sr = sampleRate > 0.0 ? sampleRate : 44100.0;
        e.clearBuffers();
        e.setSampleRate (sr);
        bass.prepare (sr);
    }
    void reset() { e.clearBuffers(); bass.reset(); }

    // Mapping mirrors plugins/VAZPhaser: 4-stage default, centre fixed mid-sweep.
    void setParams (float rateHz, float depth01, float fb01, float mix01)
    {
        const uint32_t inc = (uint32_t) (int64_t) ((double) rateHz / sr * 4294967296.0);
        e.setParams (1,                                                  // stages param → N=4
                     std::clamp ((int) std::lround (fb01 * 100.0f), 0, 100),
                     false,
                     std::clamp ((int) std::lround (depth01 * 255.0f), 0, 255),
                     128,                                                // centre (mid)
                     std::clamp ((int) std::lround (0.5 * 255.0), 0, 255),
                     std::clamp ((int) std::lround (mix01 * 255.0f), 0, 255),
                     inc);
    }

    inline void processSample (float& L, float& R)
    {
        float lowL = 0.0f, lowR = 0.0f;
        if (bassSafe) bass.split (L, R, lowL, lowR);
        int32_t li = (int32_t) std::llround ((double) L * kQ23);
        int32_t ri = (int32_t) std::llround ((double) R * kQ23);
        e.processFrame (li, ri);
        L = lowL + (float) ((double) li / kQ23);
        R = lowR + (float) ((double) ri / kQ23);
    }
};

//==============================================================================
// ADAPTED from plugins/VAZFlanger (FlangerChannel + its triangle-LFO render
// loop, lifted verbatim — parity gate: bit-exact vs a reference copy with
// bassSafe disabled). MidBass addition: the same BassSplit as chorus/phaser
// (condition f) — the flanger only ever sees content above the corner.
struct MbFlanger
{
    struct Channel
    {
        std::vector<float> buf;
        int  mask = 0, wpos = 0;
        void prepare (double sr)
        {
            int n = 1; const int need = (int) (0.090 * sr) + 4;
            while (n < need) n <<= 1;
            buf.assign ((size_t) n, 0.0f); mask = n - 1; wpos = 0;
        }
        void reset() { std::fill (buf.begin(), buf.end(), 0.0f); wpos = 0; }

        inline double process (double in, double delaySamples, double feedback, double mix)
        {
            double rp = (double) wpos - delaySamples;
            const double sz = (double) (mask + 1);
            while (rp < 0.0) rp += sz;
            const int    i0   = (int) rp;
            const double frac = rp - (double) i0;
            const double s0   = (double) buf[(size_t) (i0 & mask)];
            const double s1   = (double) buf[(size_t) ((i0 + 1) & mask)];
            const double delayed = s0 + frac * (s1 - s0);
            double w = in + feedback * delayed;                          // feedback comb (VAZ @0x5205A9)
            w = std::clamp (w, -4.0, 4.0);
            if (std::abs (w) < 1.0e-20) w = 0.0;                         // denormal flush (Phase 5 lesson)
            buf[(size_t) wpos] = (float) w;
            wpos = (wpos + 1) & mask;
            return in + mix * (delayed - in);                            // dry/wet (VAZ @0x5205BD)
        }
    };

    Channel   chL, chR;
    BassSplit bass;
    bool      bassSafe = true;
    double    sr = 44100.0, lfoPhase = 0.0, lfoInc = 0.0;
    double    baseSamples = 0.0, depthSamples = 0.0, feedback = 0.0, mix = 0.0;

    void prepare (double sampleRate)
    {
        sr = sampleRate > 0.0 ? sampleRate : 44100.0;
        chL.prepare (sr); chR.prepare (sr);
        bass.prepare (sr);
        lfoPhase = 0.0;
    }
    void reset() { chL.reset(); chR.reset(); bass.reset(); lfoPhase = 0.0; }

    void setParams (float rateHz, float depth01, float fb01, float mix01)
    {
        lfoInc       = (double) rateHz / sr;
        baseSamples  = 0.004 * sr;                                       // 4 ms centre (VAZ range 0.1-25 ms)
        depthSamples = (double) depth01 * 0.003 * sr;                    // ±3 ms sweep
        feedback     = (double) fb01 * 0.9;
        mix          = (double) mix01;
    }

    inline void processSample (float& L, float& R)
    {
        float lowL = 0.0f, lowR = 0.0f;
        if (bassSafe) bass.split (L, R, lowL, lowR);
        const double tri  = 2.0 * std::abs (2.0 * lfoPhase - 1.0) - 1.0;  // triangle LFO (VAZ: abs of phase)
        const double triR = 2.0 * std::abs (2.0 * (lfoPhase >= 0.75 ? lfoPhase - 0.75 : lfoPhase + 0.25) - 1.0) - 1.0;
        lfoPhase += lfoInc; if (lfoPhase >= 1.0) lfoPhase -= 1.0;
        const double dL = std::max (1.0, baseSamples + tri  * depthSamples);
        const double dR = std::max (1.0, baseSamples + triR * depthSamples);
        L = lowL + (float) chL.process ((double) L, dL, feedback, mix);
        R = lowR + (float) chR.process ((double) R, dR, feedback, mix);
    }
};

//==============================================================================
struct MbDelayFx
{
    VazDelayEngine e;
    double sr = 44100.0, curDel = 1000.0, tgtDel = 1000.0, smooth = 0.0;

    // Division indices match Params.h fx_dly_div: { 1/4, 1/8, 1/8., 1/8T, 1/16, 1/16., 3/16 }
    static double divBeats (int idx)
    {
        switch (idx)
        {
            case 0:  return 1.0;
            case 1:  return 0.5;
            case 2:  return 0.75;
            case 3:  return 1.0 / 3.0;
            case 4:  return 0.25;
            case 5:  return 0.375;
            default: return 0.75;      // 3/16 (== 1/8. musically; kept as the classic trance label)
        }
    }

    void prepare (double sampleRate)
    {
        sr = sampleRate > 0.0 ? sampleRate : 44100.0;
        e.prepare (sr);
        smooth = 1.0 - std::exp (-1.0 / (0.02 * sr));   // VAZDelay-proven ~20 ms retime glide
        curDel = tgtDel = 0.25 * sr;
    }
    void reset() { e.reset(); }

    void setParams (double bpm, int divIdx, float fb01, float damp01, float mix01, bool pingpong)
    {
        tgtDel = std::clamp (divBeats (divIdx) * (60.0 / std::max (bpm, 1.0)) * sr,
                             1.0, (double) (e.mask - 2));
        e.mode = pingpong ? 1 : 0;
        e.fbL = e.fbR = std::clamp ((int) std::lround (fb01 * 255.0f), 0, 255);
        // damp mapping mirrors plugins/VAZDelay (fc = 150·66^t, one-pole Q28)
        const double fc = 150.0 * std::pow (66.0, (double) (1.0f - damp01));
        const double k  = std::exp (-2.0 * 3.14159265358979323846 * fc / sr);
        e.dampL = e.dampR = (int32_t) std::clamp ((long long) std::llround (k * (double) 0x10000000),
                                                  (long long) 0, (long long) 0x0FFFFFFF);
        e.dryL = e.dryR = (int32_t) 0x40000000;                          // unity dry
        e.wetL = e.wetR = (int32_t) std::llround ((double) mix01 * (double) 0x40000000);
    }

    // CLEANUP (dcBlock flag, parity tests disable): the integer engine's
    // truncating feedback carries (1) a small DC bias and (2) ±1-LSB limit
    // cycles that never reach zero (measured ~-95/-149 dBFS) — real VAZ
    // behaviour. A 5 Hz DC blocker kills (1); a silence gate zeroes the OUTPUT
    // for (2) once the input has been silent > 1 s AND the tail has decayed
    // below -120 dBFS — the engine keeps running, so state stays consistent.
    bool  dcBlock = true;
    float dcxL = 0, dcyL = 0, dcxR = 0, dcyR = 0, dcK = 0.9993f;
    int   quiet = 0;

    inline void processSample (float& L, float& R)
    {
        const bool inSilent = std::abs (L) + std::abs (R) < 1.0e-9f;
        quiet = inSilent ? quiet + 1 : 0;

        curDel += (tgtDel - curDel) * smooth;                            // tape-style retime
        e.delayL = e.delayR = std::clamp ((int) std::lround (curDel), 1, e.mask - 2);
        int32_t li = (int32_t) std::llround ((double) L * kQ23);
        int32_t ri = (int32_t) std::llround ((double) R * kQ23);
        e.processFrame (li, ri);
        L = (float) ((double) li / kQ23);
        R = (float) ((double) ri / kQ23);
        if (dcBlock)
        {
            float y = L - dcxL + dcK * dcyL; if (std::abs (y) < 1.0e-20f) y = 0.0f;
            dcxL = L; dcyL = y; L = y;
            y = R - dcxR + dcK * dcyR; if (std::abs (y) < 1.0e-20f) y = 0.0f;
            dcxR = R; dcyR = y; R = y;
            if (quiet > (int) sr && std::abs (L) < 1.0e-6f && std::abs (R) < 1.0e-6f)
                L = R = 0.0f;
        }
    }
};

//==============================================================================
struct MbReverbFx
{
    VazReverbEngine e;
    double sr = 44100.0;

    void prepare (double sampleRate)
    {
        sr = sampleRate > 0.0 ? sampleRate : 44100.0;
        e.clearBuffers();
        e.setParams (sr, 128, 128, 128);
    }
    void reset() { e.clearBuffers(); }

    void setParams (float size01, float damp01, float mix01)
    {
        e.setParams (sr,
                     std::clamp ((int) std::lround (size01 * 255.0f), 0, 255),
                     std::clamp ((int) std::lround ((1.0f - damp01) * 255.0f), 0, 255),   // param = darkness
                     std::clamp ((int) std::lround (mix01 * 255.0f), 0, 255));
    }

    bool  dcBlock = true;                       // same truncation-DC + limit-cycle story as the delay
    float dcxL = 0, dcyL = 0, dcxR = 0, dcyR = 0, dcK = 0.9993f;
    int   quiet = 0;

    inline void processSample (float& L, float& R)
    {
        const bool inSilent = std::abs (L) + std::abs (R) < 1.0e-9f;
        quiet = inSilent ? quiet + 1 : 0;

        int32_t li = (int32_t) std::llround ((double) L * kQ23);
        int32_t ri = (int32_t) std::llround ((double) R * kQ23);
        e.processFrame (li, ri);
        L = (float) ((double) li / kQ23);
        R = (float) ((double) ri / kQ23);
        if (dcBlock)
        {
            float y = L - dcxL + dcK * dcyL; if (std::abs (y) < 1.0e-20f) y = 0.0f;
            dcxL = L; dcyL = y; L = y;
            y = R - dcxR + dcK * dcyR; if (std::abs (y) < 1.0e-20f) y = 0.0f;
            dcxR = R; dcyR = y; R = y;
            if (quiet > (int) sr && std::abs (L) < 1.0e-6f && std::abs (R) < 1.0e-6f)
                L = R = 0.0f;
        }
    }
};

//==============================================================================
// ADAPTED from plugins/VAZCompressor (stereo-linked peak detector, one-pole
// smoothing of the GAIN-REDUCTION in dB — inherently zipper-free, Phase 4 slew
// standard). Hard knee (documented). Parity: bit-exact vs a reference copy.
struct MbComp
{
    double sr = 44100.0;
    float  envGr = 0.0f;
    float  thrDb = -12.0f, slope = 0.5f, atkCoef = 0.0f, relCoef = 0.0f, makeup = 1.0f;

    void prepare (double sampleRate) { sr = sampleRate > 0.0 ? sampleRate : 44100.0; envGr = 0.0f; }
    void reset() { envGr = 0.0f; }

    void setParams (float thresholdDb, float ratio, float atkMs, float relMs, float makeupDb)
    {
        thrDb   = thresholdDb;
        slope   = 1.0f - 1.0f / std::max (ratio, 1.0f);
        atkCoef = std::exp (-1.0f / (std::max (atkMs, 0.01f) * 0.001f * (float) sr));
        relCoef = std::exp (-1.0f / (std::max (relMs, 1.0f) * 0.001f * (float) sr));
        makeup  = std::pow (10.0f, makeupDb / 20.0f);
    }

    inline void processSample (float& L, float& R)
    {
        const float detect  = std::max (std::abs (L), std::abs (R));
        const float levelDb = 20.0f * std::log10 (std::max (detect, 1.0e-7f));
        const float over    = levelDb - thrDb;
        const float target  = over > 0.0f ? over * slope : 0.0f;
        const float coef    = (target > envGr) ? atkCoef : relCoef;
        envGr = coef * envGr + (1.0f - coef) * target;
        if (envGr < 1.0e-6f) envGr = 0.0f;                               // flush
        const float g = makeup * std::pow (10.0f, -envGr / 20.0f);
        L *= g; R *= g;
    }
};

//==============================================================================
struct MbFxChain
{
    MbChorus   chorus;
    MbPhaser   phaser;
    MbFlanger  flanger;
    MbDelayFx  delay;
    MbReverbFx reverb;
    MbComp     comp;
    bool onChorus = false, onPhaser = false, onFlanger = false,
         onDelay = false, onReverb = false, onComp = false;

    void prepare (double sr)
    {
        chorus.prepare (sr); phaser.prepare (sr); flanger.prepare (sr);
        delay.prepare (sr);  reverb.prepare (sr); comp.prepare (sr);
    }
    void reset()
    {
        chorus.reset(); phaser.reset(); flanger.reset();
        delay.reset(); reverb.reset(); comp.reset();
    }

    inline void processSample (float& L, float& R)
    {
        if (onChorus)  chorus.processSample (L, R);
        if (onPhaser)  phaser.processSample (L, R);
        if (onFlanger) flanger.processSample (L, R);
        if (onDelay)   delay.processSample (L, R);
        if (onReverb)  reverb.processSample (L, R);
        if (onComp)    comp.processSample (L, R);
    }
};
} // namespace mb
