#pragma once
#include <juce_audio_basics/juce_audio_basics.h>
#include <cmath>
#include "VAZTypeA.h"   // bit-exact VAZ Type-A Lowpass (integer recurrence + dumped coef tables)
#include "VAZTypeR.h"   // bit-exact VAZ Type-R (cubic 4-pole integrator cascade, self-oscillating)
#include "VAZTypeK.h"   // bit-exact VAZ Type-K (Sallen-Key, distorted self-oscillating resonance)
#include "VAZTypeD.h"   // bit-exact VAZ Type-D (state-variable / Chamberlin SVF)
#include "VAZTypeC.h"   // bit-exact VAZ Type-C (2P/4P resonant cascade + Separation, reuses R tables)
#include "VAZTypeB.h"   // bit-exact VAZ Type-A/B taps (A-HP/BP + B-LP/BP/HP, reuses A biquad/tables)

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

    // 4-point cubic (Catmull-Rom) interpolation — VAZ uses cubic wavetable interpolation
    // (Ghidra: 4-sample Hermite read), not linear. SIZE is a power of two so we mask-wrap.
    static float read (const float* tbl, double phase) noexcept   // phase in [0,1)
    {
        const int   m = SIZE - 1;
        double x = phase * SIZE;
        int    i = (int) x;
        float  f = (float) (x - (double) i);
        float y0 = tbl[(i - 1) & m];
        float y1 = tbl[i & m];
        float y2 = tbl[(i + 1) & m];
        float y3 = tbl[(i + 2) & m];
        return y1 + 0.5f * f * ((y2 - y0)
                   + f * ((2.0f * y0 - 5.0f * y1 + 4.0f * y2 - y3)
                   + f * (3.0f * (y1 - y2) + y3 - y0)));
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

    float read (double pos) const noexcept                  // linear interpolation
    {
        const int n = (int) data.size();
        if (n == 0) return 0.0f;
        int i0 = (int) pos;
        if (i0 < 0) i0 = 0; else if (i0 >= n) i0 = n - 1;
        int i1 = i0 + 1; if (i1 >= n) i1 = i0;
        const float f = (float) (pos - (double) i0);
        return data[(size_t) i0] + (data[(size_t) i1] - data[(size_t) i0]) * f;
    }
};

struct OscBlock
{
    static constexpr int N = 4;
    double phase[N] = { 0.0, 0.0, 0.0, 0.0 };
    double sampleRate = 44100.0;
    const SampleData* sample = nullptr;   // Sample mode (mode 3) data — stable ptr from the processor
    double samplePhase = 0.0;             // playback position in sample frames
    bool   mainWrapped = false;                 // did the main phase wrap this sample? (for hard sync)

    void hardReset() noexcept { phase[0] = 0.0; }   // slave reset on master cycle (OSC2 Sync)

    // Random initial phases once at prepare → free-running, decorrelated (no click).
    void prepare (double sr) noexcept
    {
        sampleRate = sr;
        waveTables();                                          // force table build off the audio thread
        for (int i = 0; i < N; ++i) phase[i] = juce::Random::getSystemRandom().nextDouble();
    }

    static double adv (double& p, double inc) noexcept
    { double cur = p; p += inc; if (p >= 1.0) p -= 1.0; return cur; }

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
            case 1: {  // Pulse = saw(t) − saw(t−pw); pw set by waveshape (band-limited)
                double ph  = adv (phase[0], inc);
                double pw  = 0.5 - 0.49 * waveshape;            // 0.5 square → narrow pulse
                // HARNESS FINDING (2026-06-09): the REAL pulse is SQUARE at waveshape 0.5 (shape byte 128), i.e.
                // pw ≈ waveshape — NOT square at ws=0 like this map. BUT naively setting pw=waveshape (→0.5) makes
                // saw(t)−saw(t−0.5) render flat/wrong here (a saw-pair pulse bug near pw=0.5). Reverted; needs a
                // different pulse impl (e.g. centred PW or a polyBLEP pulse) before re-fixing the PW position.
                double ph2 = ph - pw; if (ph2 < 0.0) ph2 += 1.0;
                float a = WaveTables::read (wt.saw[mip], ph);
                float b = WaveTables::read (wt.saw[mip], ph2);
                return ((double) a - (double) b) * 0.5;
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
    bool   trigOn = false;
    juce::Random rng;

    void setRate (double hz, double s) noexcept { sr = (s > 0.0 ? s : 44100.0); inc = hz / sr; }
    void setTrig (bool t) noexcept { trigOn = t; }
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
            fade += 1.0 / ((0.001 + shape * shape * 4.0) * sr); if (fade > 1.0) fade = 1.0; v *= fade;
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
    enum { Idle = 0, Attack, Decay, Sustain, Release };
    int    stage = Idle;
    double sr = 44100.0, level = 0.0;
    double aCoef = 1.0, dCoef = 1.0, sLvl = 1.0, rCoef = 1.0;
    static constexpr double atkTarget = 1.2;   // exp-attack overshoot target → reaches 1.0 concavely (real VAZ is RC, not linear)
    bool   mReset = false, mCycle = false, mCurve = false;

    void setSampleRate (double s) noexcept { sr = s > 0.0 ? s : 44100.0; }
    void setADSR (float a, float d, float s, float r) noexcept   // a = attack time-to-1.0 (s); harness-RE'd: real attack is exponential
    {
        aCoef = 1.0 - std::exp (-1.79 / std::max (1.0, (double) a * sr));   // ln(1.2/0.2)=1.79 → reaches 1.0 in ~a sec
        dCoef = 1.0 - std::exp (-1.0 / std::max (1.0, (double) d * sr * 0.35));
        sLvl  = (double) s;
        rCoef = 1.0 - std::exp (-1.0 / std::max (1.0, (double) r * sr * 0.35));
    }
    void setModes (bool reset, bool cycle, bool curve) noexcept { mReset = reset; mCycle = cycle; mCurve = curve; }
    void noteOn()  noexcept { if (mReset) level = 0.0; stage = Attack; }
    void noteOff() noexcept { stage = Release; }
    void reset()   noexcept { stage = Idle; level = 0.0; }
    bool isActive() const noexcept { return stage != Idle; }

    float getNextSample() noexcept
    {
        switch (stage)
        {
            case Attack:  level += (atkTarget - level) * aCoef; if (level >= 1.0) { level = 1.0; stage = Decay; } break;   // exponential (concave) attack
            case Decay:   level += (sLvl - level) * dCoef; if (std::abs (level - sLvl) < 0.0008) { level = sLvl; stage = Sustain; } break;
            case Sustain: if (mCycle) stage = Attack; break;   // Cycle → loop (LFO). VAZ loops smoothly + syncs the
            // osc on each cycle (FUN_004dfbf8); the old `if(mReset) level=0` jumped the level to 0 → a per-cycle
            // discontinuity that bit-crushed at audio rate. Loop from the current level instead (smooth tremolo).
            case Release: level += (0.0 - level) * rCoef; if (level < 0.0004) { level = 0.0; stage = Idle; } break;
            default: break;
        }
        // Curve → exact VAZ exponential. Dumped curve-LUT 0x5445e0 (runtime ReadProcessMemory) is a clean
        // 60 dB exp: curve(x)=1024^(x-1), k=ln(1024)=6.931472 (verified to 5 dp vs the live table). Anchored
        // here as (e^{kx}-1)/(e^k-1) so curve(0)=0 (no idle floor) — was a poor level² approximation before.
        return mCurve ? (float) ((std::exp (6.931472 * level) - 1.0) / 1023.0) : (float) level;
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
    VAZTypeD  typeD;                                      // engine D (bit-exact state-variable)
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
            case 10: engine=3; tap=0; break;                                       // D LP
            case 11: engine=3; tap=2; break;                                       // D BP
            case 12: engine=3; tap=1; break;                                       // D HP
            case 13: engine=3; tap=3; hpLP=true; modRoute=2; break;                // D HP+LP (Separation)
            case 15: engine=4; tap=0; break;                                       // K LP
            case 16: engine=4; tap=3; hpLP=true; usesHP=true; break;               // K HP+LP
            case 17: engine=5; poles=2; usesHP=true; modRoute=0; break;            // R 2P+HP RM
            case 18: engine=5; poles=2; usesHP=true; modRoute=1; break;            // R 2P+HP HM
            case 19: engine=5; poles=4; usesHP=true; modRoute=0; break;            // R 4P+HP RM (default)
            case 20: engine=5; poles=4; usesHP=true; modRoute=1; break;            // R 4P+HP HM
            case 21: engine=6; break;                                              // Comb
            default: engine=5; poles=4; break;
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
            case 3: {                                                    // D state-variable
                if (!hpLP) {                                             // D LP/HP/BP → BIT-EXACT typeD (all 3 taps; D-HP validated 2.9 dB)
                    out = typeD.process (tap, x, fc, reso);
                } else {                                                 // D HP+LP (Separation, 2-section cascade) → float Chamberlin (no factory patch)
                    const double fcd=juce::jlimit (20.0,sr/6.0,fc);
                    const double f=2.0*std::sin (M_PI*fcd/sr), q=juce::jlimit (0.08,1.0,1.0-0.9*reso);
                    d_lp+=f*d_bp; const double hp=x-d_lp-q*d_bp; d_bp+=f*hp; d_bp=soft (d_bp);
                    if (hpLP) {                                          // HP→LP series, gap = Separation (aux)
                        const double f2=2.0*std::sin (M_PI*juce::jlimit (20.0,sr/6.0,fc*(0.2+1.4*aux))/sr);
                        d2_lp+=f2*d2_bp; const double hp2=hp-d2_lp-q*d2_bp; d2_bp+=f2*hp2; out=d2_lp;
                    } else out=(tap==2?d_bp:d_lp);
                }
            } break;
            case 4:                                                      // K = BIT-EXACT Sallen-Key (+ own post-HP)
                out=typeK.process (x, fc, reso, hpHz); break;
            case 6: {                                                    // Comb delay-feedback
                const int len=(int) juce::jlimit (8.0,4094.0, sr/juce::jlimit (40.0,2000.0,fc));
                int rd=combIdx-len; if (rd<0) rd+=4096;
                const double y=comb[rd];
                combLP+=(0.05+0.95*aux)*(y-combLP);                  // aux = Damping (1=bright … 0=dark): LP in feedback
                comb[combIdx]=x+juce::jlimit (0.0,0.97,reso*0.97)*cube (combLP);
                combIdx=(combIdx+1)&4095; out=y;
            } break;
            default: out=typeR.process (poles, x, fc, reso, hpHz); break;  // R: BIT-EXACT cubic cascade (+ own post-HP)
        }
        // engines R, K and C do their own exact integer closing stage; others use the float post-HP
        if (usesHP && engine!=5 && engine!=4 && engine!=2) { const double rc=1.0/(2.0*M_PI*juce::jlimit (20.0,sr*0.45,hpHz)), dt=1.0/sr, a=rc/(rc+dt);
                      const double y=a*(hpY+out-hpX); hpX=out; hpY=y; out=y; }
        return out;
    }
};
