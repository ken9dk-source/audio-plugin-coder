#pragma once
#include <juce_audio_basics/juce_audio_basics.h>
#include <cmath>
#include "VAZTypeA.h"   // bit-exact VAZ Type-A Lowpass (integer recurrence + dumped coef tables)
#include "VAZTypeR.h"   // bit-exact VAZ Type-R (cubic 4-pole integrator cascade, self-oscillating)
#include "VAZTypeK.h"   // bit-exact VAZ Type-K (Sallen-Key, distorted self-oscillating resonance)
#include "VAZTypeD.h"   // (mislabelled: this 0x6d67 SVF is actually VAZ's K — now used for K .v2p 15/16)
#include "VAZTypeDreal.h" // the REAL VAZ Type-D (2-stage cubic resonant SVF, 0x6d45/55/65/66) for D .v2p 10-13
#include "VAZTypeC.h"   // bit-exact VAZ Type-C (2P/4P resonant cascade + Separation, reuses R tables)
#include "VAZTypeB.h"   // bit-exact VAZ Type-A/B taps (A-HP/BP + B-LP/BP/HP, reuses A biquad/tables)
#include "VAZPulseOsc.h" // bit-exact VAZ band-limited (BLEP) pulse oscillator (difference-of-ramps + dumped step tables)
#include "VAZEnvTables.h" // exact envelope rate/curve tables dumped from Vaz2010Core.dll (one-pole ADSR coefs)

// ============================================================================
// VAZClone synthesis engine — Phase A: band-limited oscillators + amp envelope.
// Oscillator targets from reference renders (VAZ_OSC_Analysis.md):
//   saw = 1/n + gentle analog HF tilt; square = odd-only 50%; detune = saw ensemble.
// PolyBLEP gives band-limited saw/square (aliasing ≈ VAZ's −42 dB target).
// ============================================================================

#ifndef M_PI
 #define M_PI 3.14159265358979323846
#endif

// ============================================================================
// Band-limited mip-mapped wavetables — VAZ uses wavetable oscillators (Ghidra:
// 32-bit phase, top bits index the table, low 24 bits = interpolation fraction;
// sizes 256/512). Each mip level holds fewer harmonics for higher pitches → no
// aliasing. Generated once on the message thread (prepare), read on the audio thread.
// ============================================================================
struct WaveTables
{
    static constexpr int SIZE = 512;
    static constexpr int MIPS = 11;
    float saw [MIPS][SIZE + 1];
    float tri [MIPS][SIZE + 1];
    float sine[SIZE + 1];

    WaveTables() noexcept { generate(); }

    void generate() noexcept
    {
        for (int i = 0; i <= SIZE; ++i)
            sine[i] = (float) std::sin (2.0 * M_PI * (double) (i % SIZE) / SIZE);

        for (int m = 0; m < MIPS; ++m)
        {
            int maxH = (SIZE / 2) >> m;                       // harmonics halve each mip level
            if (maxH < 1) maxH = 1;
            for (int i = 0; i < SIZE; ++i)
            {
                double ph = 2.0 * M_PI * (double) i / SIZE;
                double s = 0.0, t = 0.0;
                for (int h = 1; h <= maxH; ++h)
                {
                    s += std::sin (h * ph) / (double) h;                                  // sawtooth
                    if (h & 1)                                                            // triangle: odd harmonics
                        t += (((h - 1) / 2) & 1 ? -1.0 : 1.0) * std::sin (h * ph) / (double) (h * h);
                }
                saw[m][i] = (float) (s * (2.0 / M_PI));
                tri[m][i] = (float) (t * (8.0 / (M_PI * M_PI)));
            }
            saw[m][SIZE] = saw[m][0];                          // wrap point for interpolation
            tri[m][SIZE] = tri[m][0];
        }
    }

    int mipFor (double freq, double sr) const noexcept
    {
        double maxH = (freq > 0.0) ? (0.5 * sr / freq) : 1.0e9;
        int m = 0;
        while (m < MIPS - 1 && (double) ((SIZE / 2) >> m) > maxH) ++m;
        return m;
    }

    // 2-point LINEAR interpolation — matches VAZ's wavetable oscillator exactly
    // (FUN_004dbddc:171-173: tbl[i] + (tbl[i+1]-tbl[i])·frac, 256-entry table, 24-bit fraction).
    // NB the earlier comment claimed VAZ uses 4-point cubic/Hermite here — that is FALSE per the
    // binary: VAZ uses linear on the wavetable osc and cubic only on the SAMPLE osc. (Audit M3-1.)
    static float read (const float* tbl, double phase) noexcept   // phase in [0,1)
    {
        const int   m = SIZE - 1;
        double x = phase * SIZE;
        int    i = (int) x;
        float  f = (float) (x - (double) i);
        return tbl[i & m] + (tbl[(i + 1) & m] - tbl[i & m]) * f;
    }
};

inline const WaveTables& waveTables() noexcept { static WaveTables wt; return wt; }

// One wavetable-oscillator block per OSC slot (Multi-Saw = 4 detuned phases).
// ── Loaded sample for the "Sample" oscillator mode (multisample playback, VAZ Sample Loader) ──
struct SampleData
{
    std::vector<float> data;          // mono sample frames
    double sourceSR = 44100.0;        // the sample's own sample-rate
    double rootHz   = 261.625565;     // pitch of the root note (WAV MidiUnityNote, default C4 = 60)
    int    loopStart = 0, loopEnd = 0;
    bool   hasLoop  = false;          // else loop the whole sample
    juce::String name;

    bool loaded() const noexcept { return ! data.empty(); }
    int  length() const noexcept { return (int) data.size(); }

    float read (double pos) const noexcept    // 4-point cubic — VAZ's SAMPLE osc is cubic (vaz_big.c:979-991), audit M3-1
    {
        const int n = (int) data.size();
        if (n == 0) return 0.0f;
        int i1 = (int) pos;
        if (i1 < 0) i1 = 0; else if (i1 >= n) i1 = n - 1;
        const float f = (float) (pos - (double) i1);
        auto at = [&] (int i) noexcept { i = i < 0 ? 0 : (i >= n ? n - 1 : i); return data[(size_t) i]; };
        const float y0 = at (i1 - 1), y1 = at (i1), y2 = at (i1 + 1), y3 = at (i1 + 2);
        return y1 + 0.5f * f * ((y2 - y0)
                   + f * ((2.0f * y0 - 5.0f * y1 + 4.0f * y2 - y3)
                   + f * (3.0f * (y1 - y2) + y3 - y0)));
    }
};

struct OscBlock
{
    static constexpr int N = 4;
    double phase[N] = { 0.0, 0.0, 0.0, 0.0 };
    uint32_t phaseU = 0;                   // 32-bit phase accumulator for the BIT-EXACT BLEP pulse (VAZPulseOsc)
    double sampleRate = 44100.0;
    const SampleData* sample = nullptr;   // Sample mode (mode 3) data — stable ptr from the processor
    double samplePhase = 0.0;             // playback position in sample frames
    bool   mainWrapped = false;                 // did the main phase wrap this sample? (for hard sync)

    void hardReset() noexcept { phase[0] = 0.0; phaseU = 0; }   // slave reset on master cycle (OSC2 Sync)

    // Random initial phases once at prepare → free-running, decorrelated (no click).
    void prepare (double sr) noexcept
    {
        sampleRate = sr;
        waveTables();                                          // force table build off the audio thread
        for (int i = 0; i < N; ++i) phase[i] = juce::Random::getSystemRandom().nextDouble();
        phaseU = (uint32_t) (phase[0] * 4294967296.0);         // seed the pulse accumulator (decorrelated like phase[0])
    }

    static double adv (double& p, double inc) noexcept
    { double cur = p; p += inc; if (p >= 1.0) p -= 1.0; return cur; }

    // VAZ pulse width: WaveShape byte b = round(shape·255) → param[0xa4] = b<<16 → the 2nd pulse edge sits at
    // b<<24 against a 32-bit phase, i.e. duty = b/256 — LINEAR, exact square at b=128 (shape 0.5). Traced from
    // the WaveShape setter FUN_004de930 @0x4de930 + the band-limited pulse branch vaz_big.c:213/222. (Replaces the
    // earlier asserted 0.05+0.9·shape compression. VAZ's band-limiting is BLEP 006dd2c0/006de2c0 — the clone's
    // difference-of-saws matches this DUTY map but not per-sample; a full BLEP-pulse port is tracked separately.)
    static double pulseWidth (double shape) noexcept
    { return (double) (int) std::lround (juce::jlimit (0.0, 1.0, shape) * 255.0) / 256.0; }

    // VAZ 5 waveform modes (waveshape = the Waveshape slider 0..1):
    //   0 Sawtooth (saw→triangle morph)  1 Pulse (variable pulsewidth)
    //   2 Multi-Saw (4 detuned saws)     3 Sample (default sine)  4 Ext/Sync (→saw)
    double next (double hz, int wave, double waveshape) noexcept
    {
        const auto& wt = waveTables();
        const double inc = (sampleRate > 0.0) ? hz / sampleRate : 0.0;
        const int    mip = wt.mipFor (hz, sampleRate);
        mainWrapped = (phase[0] + inc) >= 1.0;   // master-cycle flag (read by the synced slave)

        switch (wave)
        {
            case 0: {  // Sawtooth → triangle morph
                double ph = adv (phase[0], inc);
                float s = WaveTables::read (wt.saw[mip], ph);
                float t = WaveTables::read (wt.tri[mip], ph);
                return (double) s * (1.0 - waveshape) + (double) t * waveshape;
            }
            case 1: {  // Pulse — BIT-EXACT VAZ band-limited (BLEP) difference-of-ramps (vaz_big.c:206-241, tables dd2c0/de2c0).
                // pw = WaveShape<<16 (the b/256 duty map + freq-dependent edge clamp live INSIDE the recurrence, iVar9).
                const uint32_t incU = (uint32_t) std::llround (juce::jlimit (0.0, sampleRate * 0.5, hz) / sampleRate * 4294967296.0);
                const uint32_t prev = phaseU; phaseU += incU;
                mainWrapped = (phaseU < prev);                 // uint32 wrap = master cycle (for OSC2 hard-sync)
                const int     b  = (int) std::lround (juce::jlimit (0.0, 1.0, waveshape) * 255.0);
                const int32_t pw = (int32_t) ((uint32_t) b << 16);
                return VAZPulseOsc::render (phaseU, incU, pw);
            }
            case 2: {  // Multi-Saw: 4 detuned saws, evenly spaced (detune from waveshape)
                // Measured from VAZ (FFT of Multi-Saw Max): 4 saws at 0/+24/+48/+72.5 cents,
                // 24c spacing, ±36c total around the note. Linear from 0 at waveshape 0.
                const double maxCents = 36.25 * waveshape;
                double sum = 0.0;
                for (int i = 0; i < N; ++i)
                {
                    double d    = ((double) i / (N - 1)) * 2.0 - 1.0;   // −1..+1 (even spacing)
                    double f    = hz * std::pow (2.0, d * maxCents / 1200.0);
                    double finc = (sampleRate > 0.0) ? f / sampleRate : 0.0;
                    int    em   = wt.mipFor (f, sampleRate);
                    sum += WaveTables::read (wt.saw[em], adv (phase[i], finc));
                }
                return sum * 0.25;
            }
            case 3:    // Sample (loaded multisample) — falls back to the default Sine wave if none loaded
                if (sample != nullptr && sample->loaded())
                {
                    const double rootHz = sample->rootHz > 1.0 ? sample->rootHz : 261.625565;
                    const double rate   = (hz / rootHz) * (sample->sourceSR / sampleRate);   // pitch relative to the root note
                    const float  out    = sample->read (samplePhase);
                    samplePhase += rate;
                    const double lend = sample->hasLoop ? (double) sample->loopEnd   : (double) sample->length();
                    const double lst  = sample->hasLoop ? (double) sample->loopStart : 0.0;
                    const double llen = lend - lst;
                    if (llen > 1.0) { while (samplePhase >= lend) samplePhase -= llen; }   // loop
                    else if (samplePhase >= (double) sample->length()) samplePhase = 0.0;
                    return (double) out;
                }
                return WaveTables::read (wt.sine, adv (phase[0], inc));
            default:   // Ext/Sync → saw
                return WaveTables::read (wt.saw[mip], adv (phase[0], inc));
        }
    }
};

// One-pole low-pass — the gentle "analog" HF tilt that softens the raw BLEP saw
// to match VAZ's measured rolloff (−3 dB@H16, −6 dB@H24).
struct OnePoleLP
{
    double z = 0.0, a = 1.0;
    void setCutoff (double hz, double sr) noexcept
    {
        double x = std::exp (-2.0 * M_PI * hz / sr);
        a = 1.0 - x; b = x;
    }
    double process (double in) noexcept { z = a * in + b * z; return z; }
    double b = 0.0;
};

// Global modulation LFO — VAZ's 8 waveforms (Saw/Tri morph, +Delay fade-in variants,
// Pulse morph, S&H +Lag/+Delay) + Trig (reset on note). 'shape' = the per-wave variation slider.
struct ModLFO
{
    double phase = 0.0, inc = 0.0, sr = 44100.0;
    float  shCur = 0.0f, shTgt = 0.0f;       // sample & hold current/target
    double fade = 1.0;                        // +Delay fade-in envelope (0..1)
    double delaySec = 0.0; bool delaySet = false;  // LFO2: dedicated Delay (+0xd8) drives the fade time
    bool   trigOn = false;
    juce::Random rng;

    void setRate (double hz, double s) noexcept { sr = (s > 0.0 ? s : 44100.0); inc = hz / sr; }
    void setTrig (bool t) noexcept { trigOn = t; }
    void setDelay (double sec) noexcept { delaySec = sec; delaySet = true; }   // LFO2 Delay knob (seconds)
    void trigger() noexcept { if (trigOn) phase = 0.0; fade = 0.0; }   // note-on: reset cycle (if Trig) + restart fade

    static double tri (double p) noexcept { return 1.0 - 4.0 * std::abs (p - 0.5); }

    // 8 VAZ waveforms (0-7); shape 0..1 = variation (morph / fade-time / lag).
    double next (int wave, double shape) noexcept
    {
        double v;
        switch (wave)
        {
            case 0: {                                            // Saw/Tri: falling saw → tri → rising ramp
                const double fs = 1.0 - 2.0 * phase, rs = 2.0 * phase - 1.0, t = tri (phase);
                v = shape < 0.5 ? fs + (t - fs) * (shape * 2.0)
                                : t  + (rs - t) * ((shape - 0.5) * 2.0);
            } break;
            case 1:  v = tri (phase); break;                     // Tri + Delay
            case 2:  v = 1.0 - 2.0 * phase; break;               // Saw + Delay (falling)
            case 3:  v = std::sin (2.0 * M_PI * phase); break;   // Sine + Delay
            case 4:  { const double w = 0.05 + 0.9 * shape; v = phase < w ? 1.0 : -1.0; } break; // Pulse morph
            case 5:  v = phase < 0.5 ? 1.0 : -1.0; break;        // Square + Delay
            default: v = shCur; break;                           // 6 = S&H+Lag, 7 = S&H+Delay
        }
        phase += inc;
        if (phase >= 1.0) { phase -= 1.0; shTgt = rng.nextFloat() * 2.0f - 1.0f; if (wave != 6) shCur = shTgt; }
        if (wave == 6) {                                         // S&H + Lag: slide toward target
            const double c = 1.0 - std::exp (-2.0 * M_PI / ((0.001 + shape * 0.2) * sr));
            shCur += (float) ((shTgt - shCur) * c);
        }
        if (wave == 1 || wave == 2 || wave == 3 || wave == 5 || wave == 7) {   // +Delay fade-in
            // LFO2 fades over its dedicated Delay param; LFO1 (no Delay field) falls back to WaveShape.
            const double dsec = delaySet ? delaySec : (0.001 + shape * shape * 4.0);
            fade += 1.0 / (juce::jmax (0.0002, dsec) * sr); if (fade > 1.0) fade = 1.0; v *= fade;
        }
        return v;
    }

    // LFO3 simple path: 0 = triangle, 1 = sine.
    double nextSimple (int triOrSine) noexcept
    {
        const double v = triOrSine == 1 ? std::sin (2.0 * M_PI * phase) : tri (phase);
        phase += inc; if (phase >= 1.0) phase -= 1.0;
        return v;
    }
};

// ============================================================================
// ADSR envelope with VAZ modes: Reset (zero before attack), Cycle (loop at sustain
// = envelope-as-LFO), Curve (exponential output shape). Drop-in for juce::ADSR usage.
// ============================================================================
struct VAZEnv
{
    // BIT-EXACT VAZ envelope: a one-pole ADSR whose per-sample rate coefs come straight from VAZ's
    // runtime rate tables (VAZEnvTables.h, dumped from Vaz2010Core.dll). Level is VAZ's Q30 integer
    // (1.0 = 0x40000000); each stage is L += rate·(target − L) >> 32 exactly as the voice render does
    // (vaz_big.c @0x4dbddc lines 384-443). Replaces the old empirical RC fit (aCoef/x⁴-time, ±10-20%).
    enum { Idle = 0, Attack, Decay, Sustain, Release, PreAttack };   // PreAttack = VAZ stage-0 ramp-down (Reset mode)
    int     stage = Idle;
    int64_t L = 0;                                            // Q30 level (0 .. 0x3fffffff)
    int32_t atkRate = 0, decRate = 0, relRate = 0, susTarget = 0;
    bool    mReset = false, mCycle = false;
    double  srRatio = 1.0;   // 44100/sr — the rate tables were built at ~44.1k; scale the one-pole alpha for other SRs
    static constexpr int64_t ONE = 0x40000000, ATKTGT = 0x44000000, CAP = 0x3fffffff, OVS = 0x400000;

    void setSampleRate (double s) noexcept { srRatio = 44100.0 / (s > 0.0 ? s : 44100.0); }

    // ADSR from the normalised params (0..1). loadV2P stores the .v2p value ÷425 (A/D/R) / ÷255 (sustain),
    // so n·425 recovers the VAZ table index (= the .v2p value for ver≥107). curve = Curve mode (decay/
    // release switch to the attack rate table + sustain uses the curve LUT).
    void setADSR (float aN, float dN, float sN, float rN, bool curve) noexcept
    {
        auto rate = [sr = srRatio] (float n, int base) {             // alpha · 44100/sr → SR-independent wall-clock time
            const int i = juce::jlimit (0, 719, base + (int) std::lround (n * 425.0f));
            return (int32_t) std::llround ((double) VAZEnvT::kRate[i] * sr);
        };
        const int s = juce::jlimit (0, 255, (int) std::lround (sN * 255.0f));
        atkRate   = rate (aN, 12);                                    // DAT_006db818[idx]
        decRate   = rate (dN, curve ? 12 : 0);                        // curve → attack table, else DAT_006db7e8
        relRate   = rate (rN, curve ? 12 : 0);
        susTarget = curve ? VAZEnvT::kSusCurve[s] : (int32_t) (s * 0x404040);
    }
    void setModes (bool reset, bool cycle, bool /*curve→setADSR*/) noexcept { mReset = reset; mCycle = cycle; }
    void noteOn()  noexcept { stage = mReset ? PreAttack : Attack; }   // Reset → VAZ stage-0 ramp to 0 first (no hard-zero click); else re-attack from current L
    void noteOff() noexcept { stage = Release; }
    void reset()   noexcept { stage = Idle; L = 0; }
    bool isActive() const noexcept { return stage != Idle; }

    float getNextSample() noexcept
    {
        switch (stage)
        {
            case Attack:                                              // exp approach to 1.0625, caps at 1.0 (concave)
                L += ((int64_t) atkRate * (ATKTGT - L)) >> 32;
                if (L > CAP) { L = CAP; stage = Decay; }
                break;
            case Decay:                                               // exp approach to sustain (slight under-overshoot)
                L += ((int64_t) decRate * ((susTarget - OVS) - L)) >> 32;
                if (L <= susTarget) { L = susTarget; stage = mCycle ? Attack : Sustain; }   // Cycle → loop (LFO-env)
                break;
            case Sustain:
                if (mCycle) stage = Attack;
                break;
            case Release:                                             // exp approach to 0 (target −OVS, floors at 0)
                L -= ((int64_t) relRate * (OVS + L)) >> 32;
                if (L < 1) { L = 0; stage = Idle; }
                break;
            case PreAttack:                                          // VAZ stage-0 (vaz_big.c:413-419): linear ramp to 0, then Attack
                L -= (int64_t) std::llround ((double) VAZEnvT::kStage0Dec * srRatio);
                if (L < 1) { L = 0; stage = Attack; }
                break;
            default: break;
        }
        return (float) ((double) L / (double) ONE);                  // Q30 → 0..1 for the VCA / mod bus
    }
};

// ============================================================================
// VAZ-style filter: 4-pole ladder ("integrator cascade" = VAZ default Type R) with
// CUBIC-saturated resonance (the "distorted resonance" found in the Ghidra decompile).
// Cutoff is applied per-sample with NO smoothing → the filter envelope snaps instantly
// (juce::dsp::LadderFilter forces a 50 ms cutoff smoother, which caused the attack "fade").
// ============================================================================
struct VAZLadder
{
    double sr = 44100.0;
    double s[4] = { 0.0, 0.0, 0.0, 0.0 };
    double cutoffHz = 1000.0, reso = 0.0, drive = 1.0;
    int    mode = 0;                       // 0 = LP24, 1 = LP12, 2 = HP, 3 = BP

    void prepare (double sampleRate) noexcept { sr = sampleRate; reset(); }
    void reset() noexcept { s[0] = s[1] = s[2] = s[3] = 0.0; }
    void setCutoffHz  (double hz) noexcept { cutoffHz = hz; }
    void setResonance (double r)  noexcept { reso = r; }
    void setDrive     (double d)  noexcept { drive = d; }
    void setMode      (int m)     noexcept { mode = m; }

    // Cubic soft-clip normalised to ±1 (VAZ distorted-resonance shape).
    static double cubic (double x) noexcept
    {
        if (x >  1.0) return  1.0;
        if (x < -1.0) return -1.0;
        return 1.5 * (x - x * x * x * (1.0 / 3.0));
    }

    double process (double in) noexcept
    {
        // VAZ-exact one-pole coefficient (Ghidra @0x4D4720): pole a = (2−cosω) − √((2−cosω)²−1),
        // g = 1−a. Cutoff clamped to 0.49·fs (VAZ's max). More accurate than 1−exp(−ω) up high.
        const double fc = juce::jlimit (20.0, sr * 0.49, cutoffHz);
        const double cw = std::cos (2.0 * M_PI * fc / sr);
        const double xx = 2.0 - cw;
        const double g  = 1.0 - (xx - std::sqrt (xx * xx - 1.0));
        const double fb = reso * 4.2;                               // → self-oscillation near top

        const double x = in * drive - fb * cubic (s[3]);           // saturated resonance feedback
        s[0] += g * (x    - s[0]);                                  // 4 cascaded one-pole LPs
        s[1] += g * (s[0] - s[1]);
        s[2] += g * (s[1] - s[2]);
        s[3] += g * (s[2] - s[3]);

        switch (mode)
        {
            case 1:  return s[1];                                   // 2-pole LP
            case 2:  return x - s[3];                               // high-pass (approx)
            case 3:  return s[1] - s[3];                            // band-pass (approx)
            default: return s[3];                                   // 4-pole LP (default Type R)
        }
    }
};

// ── VAZ multimode filter — 6 engines + Comb, topologies reverse-engineered from
//    Vaz2010Core.dll (FUN_004dbddc). The 22-entry dropdown index selects engine+tap:
//      A = clean 2-pole TPT state-variable (LP/HP/BP, no saturation)        [tables 005d/0059/0055]
//      B = TPT SVF, Bandwidth-shaped Q
//      C = cascaded saturated SVF (2P/4P) + cubic                            [tables 0069/0065/0061]
//      D = Chamberlin SVF + soft-clip (LP/HP/BP/HP+LP)                       [tables 006d67/77]
//      K = 4x one-pole cascade, cubic, 2x oversampled + pre-HP              [tables 006d87/97]
//      R = resonant ladder + cubic feedback (VAZLadder), 2P/4P              [tables 006da7]
//      Comb = delay-feedback engine (the >=0x5e delay path)
struct VAZMultiFilter
{
    double sr = 44100.0;
    int    engine = 5, tap = 0, poles = 4;   // engine 0=A 1=B 2=C 3=D 4=K 5=R 6=Comb; tap 0=LP 1=HP 2=BP 3=HP+LP
    bool   usesHP = false, hpLP = false;
    int    modRoute = 0;        // slot-3 modulation target: 0=Resonance 1=Highpass 2=Separation
    double cutoffHz = 1000.0, reso = 0.0, aux = 0.5, hpHz = 20.0, drive = 1.0;

    VAZLadder ladderR;                                   // engine R (legacy float ladder — fallback)
    VAZTypeA  typeA;                                      // engine A (bit-exact LP)
    VAZTypeR  typeR;                                      // engine R (bit-exact cubic cascade)
    VAZTypeK  typeK;                                      // engine K (bit-exact Sallen-Key)
    VAZTypeD  typeD;                                      // (0x6d67 SVF = VAZ's K; used by K .v2p 15/16 via engine 3 tap 2)
    VAZTypeDreal typeDreal;                               // the REAL VAZ Type-D (2-stage cubic SVF) — engine 7, D .v2p 10-13
    VAZTypeC  typeC;                                      // engine C (bit-exact cascade + Separation)
    VAZTypeB  typeB;                                      // engines A-HP/BP + B (bit-exact A/B biquad taps)
    double a_ic1=0, a_ic2=0, c1_1=0, c1_2=0, c2_1=0, c2_2=0;
    double d_lp=0, d_bp=0, d2_lp=0, d2_bp=0;
    double k_s[4]={0,0,0,0}, k_hpX=0, k_hpY=0;
    double comb[4096]={0}; int combIdx=0; double combLP=0;
    double hpX=0, hpY=0;

    void prepare (double s) noexcept { sr = s; ladderR.prepare (s); typeA.prepare (s); typeR.prepare (s); typeK.prepare (s); typeD.prepare (s); typeC.prepare (s); typeB.prepare (s); reset(); }
    void reset() noexcept
    {
        a_ic1=a_ic2=c1_1=c1_2=c2_1=c2_2=0; d_lp=d_bp=d2_lp=d2_bp=0;
        k_s[0]=k_s[1]=k_s[2]=k_s[3]=0; k_hpX=k_hpY=hpX=hpY=0; combIdx=0; combLP=0;
        for (int i=0;i<4096;++i) comb[i]=0.0; ladderR.reset(); typeA.reset(); typeR.reset(); typeK.reset(); typeD.reset();
    }

    static double cube (double x) noexcept
    { if (x>1.0) return 1.0; if (x<-1.0) return -1.0; return 1.5*(x - x*x*x*(1.0/3.0)); }

    // Gentle soft-clip WITHOUT makeup gain — near-unity for small signals, soft knee to ±1 at ±1.5.
    static double soft (double x) noexcept
    { x = juce::jlimit (-1.5, 1.5, x); return x - x*x*x*(1.0/6.75); }

    void setMode (int idx) noexcept
    {
        engine=5; tap=0; poles=4; usesHP=false; hpLP=false; modRoute=0;
        switch (idx)
        {
            case 0:  engine=0; tap=0; break;                                       // A LP
            case 4:  engine=0; tap=1; break;                                       // A HP
            case 5:  engine=0; tap=2; break;                                       // A BP
            case 1:  engine=1; tap=0; break;                                       // B LP
            case 6:  engine=1; tap=1; break;                                       // B HP
            case 7:  engine=1; tap=2; break;                                       // B BP
            case 2:  engine=2; poles=2; usesHP=true; modRoute=0; break;            // C 2P+HP RM
            case 14: engine=2; poles=2; usesHP=true; modRoute=1; break;            // C 2P+HP HM
            case 3:  engine=2; poles=4; usesHP=true; modRoute=0; break;            // C 4P+HP RM
            case 8:  engine=2; poles=4; usesHP=true; modRoute=2; break;            // C 4P+HP SM
            case 9:  engine=2; poles=4; usesHP=true; modRoute=1; break;            // C 4P+HP HM
            // D (.v2p 10-13): the REAL VAZ D (2-stage cubic SVF @0x4ddaa8, 0x6d45/55/65/66) = VAZTypeDreal (engine 7).
            // taps 0=LP/1=BP/2=HP (mode&3). Was wrongly on engine 3 (=VAZTypeD = VAZ's K).
            case 10: engine=7; tap=0; break;                                       // D LP  → VAZTypeDreal LP
            case 11: engine=7; tap=1; break;                                       // D BP  → VAZTypeDreal BP
            case 12: engine=7; tap=2; break;                                       // D HP  → VAZTypeDreal HP
            case 13: engine=7; hpLP=true; break;                                   // D HP+LP Separation → VAZTypeDreal 2-section (cut±Sep), aux=Separation
            // K (.v2p 15/16): PROVEN (oracle filter_k, BIT-EXACT) to be VAZ's 0x6d67 SVF (handler 0x4ddcfe) = exactly the
            // engine the clone calls VAZTypeD, bp tap (= resonant 2-pole LP), no post-HP. Re-routed engine 4 → engine 3.
            case 15: engine=3; tap=2; break;                                       // K LP  → VAZTypeD bp tap (BIT-EXACT)
            case 16: engine=3; tap=2; hpLP=true; break;                            // K HP+LP (mode 0x44) → VAZTypeD::processHPLP: HP pre-section (cut=aux, reso=hpHz) → main K
            // R (.v2p 17-20): PROVEN to be VAZ's Sallen-Key 0x6d87 handler (0x4ddf44) — the exact recurrence VAZTypeK
            // implements — NOT the 0x69 cubic. Re-routed cubic→Sallen-Key (filter_route_map: these 4 now match VAZ).
            // VAZTypeR (cubic) turned out to be a duplicate of the C engine (0x4dd82b), never VAZ's R → deprecated below.
            case 17: engine=4; poles=2; usesHP=true; modRoute=0; break;            // R 2P+HP RM
            case 18: engine=4; poles=2; usesHP=true; modRoute=1; break;            // R 2P+HP HM
            case 19: engine=4; poles=4; usesHP=true; modRoute=0; break;            // R 4P+HP RM (default)
            case 20: engine=4; poles=4; usesHP=true; modRoute=1; break;            // R 4P+HP HM
            case 21: engine=6; break;                                              // Comb
            default: engine=4; poles=4; break;                                     // default filter = R 4P → Sallen-Key
        }
        ladderR.setMode (poles==2 ? 1 : 0);
    }

    void setParams (double cutHz, double r, double auxv, double hpCutHz, double dr) noexcept
    {
        cutoffHz=cutHz; reso=r; aux=auxv; hpHz=hpCutHz; drive=dr;
        ladderR.setCutoffHz (cutHz); ladderR.setResonance (r); ladderR.setDrive (dr);
    }

    // One TPT state-variable sample (Cytomic form). Outputs lp/bp/hp simultaneously.
    inline void svf (double in, double g, double k, double& ic1, double& ic2,
                     double& lp, double& bp, double& hp) noexcept
    {
        const double a1=1.0/(1.0+g*(g+k)), a2=g*a1, a3=g*a2;
        const double v3=in-ic2, v1=a1*ic1+a2*v3, v2=ic2+a2*ic1+a3*v3;
        ic1=2.0*v1-ic1; ic2=2.0*v2-ic2; lp=v2; bp=v1; hp=in-k*v1-v2;
    }

    double process (double in) noexcept
    {
        const double fc=juce::jlimit (20.0, sr*0.49, cutoffHz);
        const double x=in;          // clean engines stay unity; 'drive' only feeds the R ladder (ladderR)
        double out=0.0;
        switch (engine)
        {
            case 0: case 1: {                                            // A / B
                if (engine==0 && tap==0) {                               // A Lowpass → BIT-EXACT integer engine
                    out = typeA.process (x, fc, reso);                   // (the exact VAZ recurrence + dumped coefs)
                } else {                                                 // A HP/BP + all of B → BIT-EXACT A/B taps
                    const int vazTap     = (tap==0) ? 0 : (tap==1) ? 2 : 1;        // clone LP/HP/BP → VAZ LP/HP/BP (0/2/1)
                    const int mixReso255 = (int) std::lround (juce::jlimit (0.0,1.0,reso) * 255.0);
                    const int rowReso255 = (engine==1)                            // B: resonance row = 0xff − Bandwidth
                                         ? 255 - (int) std::lround (juce::jlimit (0.0,1.0,aux) * 255.0)
                                         : mixReso255;                            // A HP/BP: row = Resonance
                    out = typeB.process (vazTap, x, fc, mixReso255, rowReso255);
                }
            } break;
            case 2: {                                                    // C = BIT-EXACT cascade (reuses R biquad) + Separation
                // closing one-pole coef is indexed by +0x274 (hp_cutoff) — recover its 0..1 value from hpHz.
                const double hpNorm = std::log (juce::jlimit (20.0, sr*0.45, hpHz) / 20.0) / std::log (100.0);
                out = typeC.process (poles, x, fc, reso, aux, hpNorm);   // poles 2/4; aux = Separation
            } break;
            case 3: {                                                    // K SVF (VAZ's K = 0x6d67 handler); .v2p 15/16
                if (!hpLP) {                                             // K LP → BIT-EXACT typeD tap 2 (filter_k)
                    out = typeD.process (tap, x, fc, reso);
                } else {                                                 // K HP+LP (.v2p 16, mode 0x44) → BIT-EXACT: HP pre-section → main K
                    const double hpNorm = std::log (juce::jlimit (20.0, sr*0.45, hpHz) / 20.0) / std::log (100.0);
                    out = typeD.processHPLP (x, fc, reso, aux, hpNorm); // HP cut = aux (param 0x270), HP reso = hpHz byte (param 0x274)
                }
            } break;
            case 4: {                                                    // R (VAZ's real R) = BIT-EXACT Sallen-Key, mode-select 2P|4P + linear post-HP
                const double hpNorm = std::log (juce::jlimit (20.0, sr*0.45, hpHz) / 20.0) / std::log (100.0);
                out = typeK.process (poles == 4, x, fc, reso, hpNorm);   // poles 2/4 = .v2p 17-18 / 19-20
            } break;
            case 7:                                                      // D (real) = 2-stage cubic SVF; hpLP → 2-section Separation
                out = hpLP ? typeDreal.processHPLP (x, fc, reso, aux) : typeDreal.process (tap, x, fc, reso); break;
            case 6: {                                                    // Comb delay-feedback
                const int len=(int) juce::jlimit (8.0,4094.0, sr/juce::jlimit (40.0,2000.0,fc));
                int rd=combIdx-len; if (rd<0) rd+=4096;
                const double y=comb[rd];
                combLP+=(0.05+0.95*aux)*(y-combLP);                  // aux = Damping (1=bright … 0=dark): LP in feedback
                comb[combIdx]=x+juce::jlimit (0.0,0.97,reso*0.97)*cube (combLP);
                combIdx=(combIdx+1)&4095; out=y;
            } break;
            // ⚠ DEPRECATED: VAZTypeR (0x69 cubic) was proven to be a DUPLICATE of the C engine (0x4dd82b), NOT VAZ's R
            // (which is the Sallen-Key 0x6d87 @0x4ddf44 = VAZTypeK). No mode routes here anymore (engine 5 is never set);
            // kept only as a defensive fallback. VAZ's real R now runs through case 4 (VAZTypeK).
            default: { const double hpNorm = std::log (juce::jlimit (20.0, sr*0.45, hpHz) / 20.0) / std::log (100.0);
                       out = typeK.process (poles == 4, x, fc, reso, hpNorm); } break;
        }
        // engines K and C do their own exact integer closing stage; others use the float post-HP
        if (usesHP && engine!=5 && engine!=4 && engine!=2) { const double rc=1.0/(2.0*M_PI*juce::jlimit (20.0,sr*0.45,hpHz)), dt=1.0/sr, a=rc/(rc+dt);
                      const double y=a*(hpY+out-hpX); hpX=out; hpY=y; out=y; }
        return out;
    }
};
