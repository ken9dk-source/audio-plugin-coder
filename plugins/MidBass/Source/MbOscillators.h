#pragma once
//==============================================================================
// MidBass oscillator section — Phase 1.
// Reuses the verified VAZClone wavetable oscillator (OscBlock, mip-mapped,
// anti-aliased) via the include path, wrapped in:
//   * 3 oscillators (Saw / Pulse+PWM / Triangle) with oct/semi/fine/level
//   * hard sync + light FM + ring mod (osc2 = master/modulator, osc1 = carrier)
//   * sub oscillator (Sine/Tri/Square, -1/-2 oct) with TRUE bypass (approval
//     condition #6: zero processing when subOn == false)
//   * 1-8 voice unison (VazOsc math: symmetric detune, equal-power spread,
//     1/sqrt(n) normalisation) with an explicit MONO-UNISON rule (condition #7):
//     detune stays active, stereo spread is forced to ZERO, the summed signal is
//     written identically to both channels.
//   * per-oscillator analog drift, bounded to kMaxDriftCents (VAZ-style slop).
//==============================================================================
#include "Synth.h"   // VAZClone OscBlock + WaveTables (include dir: ../VAZClone/Source)
#include <cstdint>
#include <cmath>
#include <algorithm>

namespace mb
{
namespace OscWave { enum { Saw = 0, Pulse = 1, Triangle = 2 }; }
namespace SubWave { enum { Sine = 0, Triangle = 1, Square = 2 }; }

// FM is spec'd "light/subtle": at fm = 1 the peak frequency deviation of osc1 is
// kMaxFmDepth * f1 (12 % ≈ under 2 semitones) — asserted by the Phase 1 test.
inline constexpr float  kMaxFmDepth    = 0.12f;
// Analog drift never exceeds this many cents at amount = 1 (per oscillator).
inline constexpr double kMaxDriftCents = 9.0;

//==============================================================================
// Slow random pitch drift: pick a new bounded target every ~0.75-1.5 s, slew
// toward it with a ~0.4 Hz one-pole. Output in cents. amount == 0 is bit-exact
// zero (and resets state) so an undrifted patch is untouched.
struct DriftGen
{
    double   sr = 44100.0, cur = 0.0, target = 0.0, slew = 0.0;
    int      countdown = 0;
    uint32_t rng = 1;

    void prepare (double sampleRate, uint32_t seed)
    {
        sr = sampleRate > 0.0 ? sampleRate : 44100.0;
        rng = seed != 0 ? seed : 1;
        cur = target = 0.0; countdown = 0;
        slew = 1.0 - std::exp (-2.0 * M_PI * 0.4 / sr);
    }

    double frand()   // xorshift32 → [-1, 1)
    {
        rng ^= rng << 13; rng ^= rng >> 17; rng ^= rng << 5;
        return (double) (int32_t) rng / 2147483648.0;
    }

    double nextCents (float amount)
    {
        if (amount <= 0.0f) { cur = target = 0.0; countdown = 0; return 0.0; }
        if (--countdown <= 0)
        {
            target    = frand() * kMaxDriftCents;                       // |target| < kMaxDriftCents
            countdown = (int) ((0.75 + 0.375 * (frand() + 1.0)) * sr);  // 0.75..1.5 s
        }
        cur += (target - cur) * slew;                                   // |cur| stays inside the target bound
        return cur * (double) amount;
    }
};

//==============================================================================
struct OscEngine
{
    static constexpr int kMaxUnison = 8;

    struct OscCfg
    {
        int   wave = OscWave::Saw;
        int   oct = 0, semi = 0;
        float fineCents = 0.0f, pw = 0.5f, level = 0.0f;
    };

    struct Config
    {
        OscCfg osc[3];
        bool   sync = false;                 // hard sync: osc1 resets on each osc2 cycle
        float  fm = 0.0f, ring = 0.0f;       // 0..1
        float  drift = 0.0f;                 // 0..1 → cents via DriftGen
        bool   subOn = false;
        int    subWave = SubWave::Sine, subOctDown = 1;   // 1 or 2 octaves below
        float  subLevel = 0.5f;
        int    uniVoices = 1;
        float  uniDetune = 0.2f, uniSpread = 0.0f;        // 0..1
        bool   uniMono = true;
    };

    // ---- state ----
    struct Copy { OscBlock o1, o2, o3; };
    Copy     copies[kMaxUnison];
    OscBlock sub;
    DriftGen drift[3];                       // one per oscillator (independent VCO slop)
    Config   cfg;
    double   pitchHz = 110.0;
    float    pwMod = 0.0f;      // per-sample PWM offset (mod matrix / LFO), ±0.45 max

    // derived per setConfig
    double pitchMul[3]  = { 1.0, 1.0, 1.0 };
    int    vazWave[3]   = { 0, 0, 0 };
    double vazShape[3]  = { 0.0, 0.0, 0.0 };
    int    subVazWave   = 3;
    double subVazShape  = 0.0;
    double subDiv       = 2.0;
    int    n            = 1;
    double detuneMul[kMaxUnison] = { 1, 1, 1, 1, 1, 1, 1, 1 };
    float  gL[kMaxUnison] = {}, gR[kMaxUnison] = {};
    float  norm = 1.0f;

    void prepare (double sr, uint32_t seed = 0x9E3779B9u)
    {
        for (auto& c : copies) { c.o1.prepare (sr); c.o2.prepare (sr); c.o3.prepare (sr); }
        sub.prepare (sr);
        for (int i = 0; i < 3; ++i) drift[i].prepare (sr, seed + (uint32_t) i * 0x85EBCA6Bu);
    }

    // Deterministic start phases (tests + reproducible renders). prepare() leaves
    // OscBlock with system-random phases; alias measurements interfere differently
    // per phase set, so tests sweep FIXED seeds instead of trusting one lucky run.
    void seedPhases (uint32_t seed)
    {
        auto next01 = [&seed]()
        {
            seed ^= seed << 13; seed ^= seed >> 17; seed ^= seed << 5;
            return (double) seed / 4294967296.0;
        };
        for (auto& c : copies)
            for (OscBlock* o : { &c.o1, &c.o2, &c.o3 })
                for (double& p : o->phase) p = next01();
        for (double& p : sub.phase) p = next01();
    }

    void setPitch (double hz) { pitchHz = hz > 0.0 ? hz : 1.0; }

    static void mapWave (int wave, float pw, int& vw, double& vs)
    {
        switch (wave)
        {
            // OscBlock pulse: width = 0.05 + 0.9·waveshape (square at waveshape 0.5),
            // so invert that to make the user's PW the ACTUAL pulse width.
            case OscWave::Pulse:    vw = 1; vs = std::clamp (((double) pw - 0.05) / 0.9, 0.0, 1.0); break;
            case OscWave::Triangle: vw = 0; vs = 1.0; break;
            default:                vw = 0; vs = 0.0; break;   // Saw
        }
    }

    void setConfig (const Config& c)
    {
        cfg = c;
        for (int i = 0; i < 3; ++i)
        {
            pitchMul[i] = std::pow (2.0, (double) c.osc[i].oct
                                       + (double) c.osc[i].semi / 12.0
                                       + (double) c.osc[i].fineCents / 1200.0);
            mapWave (c.osc[i].wave, c.osc[i].pw, vazWave[i], vazShape[i]);
        }

        switch (c.subWave)     // OscBlock modes: 3 = sine (no sample loaded), 0/1 = saw-tri / pulse
        {
            case SubWave::Triangle: subVazWave = 0; subVazShape = 1.0; break;
            case SubWave::Square:   subVazWave = 1; subVazShape = 0.5; break;   // waveshape 0.5 = true square
            default:                subVazWave = 3; subVazShape = 0.0; break;
        }
        subDiv = (c.subOctDown >= 2) ? 4.0 : 2.0;

        // ---- unison table (VazOsc math). MONO-UNISON RULE: detune stays active,
        // spread is forced to zero, output is summed and duplicated to L and R. ----
        n = std::clamp (c.uniVoices, 1, kMaxUnison);
        const double maxCents = (double) c.uniDetune * 50.0;
        const float  width    = c.uniMono ? 0.0f : std::clamp (c.uniSpread, 0.0f, 1.0f);
        norm = 1.0f / std::sqrt ((float) n);
        for (int i = 0; i < n; ++i)
        {
            const double t = (n == 1) ? 0.0 : ((double) i / (n - 1) * 2.0 - 1.0);
            detuneMul[i] = std::pow (2.0, (t * maxCents) / 1200.0);
            const float pan = (float) t * width;
            const float ang = (pan * 0.5f + 0.5f) * (float) (M_PI * 0.5);
            gL[i] = std::cos (ang) * norm;
            gR[i] = std::sin (ang) * norm;
        }
    }

    // For the true-bypass test: the sub phase must not advance while subOn == false.
    double subPhaseForTest() const { return sub.phase[0]; }

    // Ring/FM modulator tap: reads the oscillator's CURRENT phase from the wavetable
    // one mip level higher (mipFor(2·hz) = half the harmonics). The product of two
    // half-band signals stays under Nyquist, so ring mod doesn't alias the way a
    // plain v1·v2 of full-band saws does (measured −36 dB → −89 dB; FM −38 → −51 dB).
    //
    // INTENTIONAL CHARACTER DECISION (approved 2026-07-07): halving the harmonic
    // content slightly darkens the FM/ring timbre vs feeding them the full-band
    // signals. That trade is deliberate — the alias floor is the priority for a
    // mid-bass instrument — so do NOT "fix" this back to v1/v2 without re-running
    // the *_alias_floor tests in Tests/test_oscillators.cpp.
    static float ringTap (const WaveTables& wt, double phase, double hz, double sr, int vw, double vs)
    {
        const int mip = wt.mipFor (hz * 2.0, sr);
        if (vw == 1)                                     // pulse = difference of saws
        {
            const double pw  = 0.05 + 0.9 * vs;
            double ph2 = phase - pw; if (ph2 < 0.0) ph2 += 1.0;
            return 0.5f * (WaveTables::read (wt.saw[mip], phase) - WaveTables::read (wt.saw[mip], ph2));
        }
        if (vs >= 0.5)                                   // triangle (saw→tri morph upper half)
            return WaveTables::read (wt.tri[mip], phase);
        return WaveTables::read (wt.saw[mip], phase);    // saw / default
    }

    void process (float& L, float& R)
    {
        // Per-osc drift: ratio ≈ 1 + cents·ln2/1200, linearised (error < 0.001 cent
        // over our ±9 cent bound — avoids a pow() per oscillator per sample).
        constexpr double kCentsToRatio = 5.7762265046662105e-4;   // ln(2)/1200
        const double dm0 = 1.0 + drift[0].nextCents (cfg.drift) * kCentsToRatio;
        const double dm1 = 1.0 + drift[1].nextCents (cfg.drift) * kCentsToRatio;
        const double dm2 = 1.0 + drift[2].nextCents (cfg.drift) * kCentsToRatio;

        const double f1 = pitchHz * pitchMul[0] * dm0;
        const double f2 = pitchHz * pitchMul[1] * dm1;
        const double f3 = pitchHz * pitchMul[2] * dm2;

        const float l1 = cfg.osc[0].level, l2 = cfg.osc[1].level, l3 = cfg.osc[2].level;
        const float fmAmt   = cfg.fm * kMaxFmDepth;
        const float ringAmt = cfg.ring;

        // PWM modulation: recompute the pulse waveshape per sample only when active.
        double vs[3] = { vazShape[0], vazShape[1], vazShape[2] };
        if (pwMod != 0.0f)
            for (int i = 0; i < 3; ++i)
                if (cfg.osc[i].wave == OscWave::Pulse)
                    vs[i] = std::clamp (((double) cfg.osc[i].pw + (double) pwMod - 0.05) / 0.9, 0.0, 1.0);

        float outL = 0.0f, outR = 0.0f, monoSum = 0.0f;
        for (int i = 0; i < n; ++i)
        {
            Copy& cp = copies[i];
            const double fd = detuneMul[i];

            // osc2 first: it is the sync master and the FM/ring modulator.
            const double ph2 = cp.o2.phase[0];                       // this sample's phase (next() advances it)
            const float v2 = (float) cp.o2.next (f2 * fd, vazWave[1], vs[1]);
            if (cfg.sync && cp.o2.mainWrapped)
                cp.o1.hardReset();

            // FM uses the band-limited modulator tap (like ring): a full-band saw
            // modulator at high pitch folds high-order sidebands over Nyquist.
            double f1eff = f1 * fd;
            if (fmAmt > 0.0f)
            {
                const float v2m = ringTap (waveTables(), ph2, f2 * fd, cp.o2.sampleRate, vazWave[1], vs[1]);
                f1eff *= 1.0 + (double) (fmAmt * v2m);
            }
            const double ph1 = cp.o1.phase[0];                       // after any sync reset, before advance
            const float v1 = (float) cp.o1.next (f1eff, vazWave[0], vs[0]);
            const float v3 = (float) cp.o3.next (f3 * fd, vazWave[2], vs[2]);

            float m = v1 * l1 + v2 * l2 + v3 * l3;
            if (ringAmt > 0.0f)
            {
                const auto& wt = waveTables();
                const double sr = cp.o1.sampleRate;
                m += ringAmt * ringTap (wt, ph1, f1eff,   sr, vazWave[0], vs[0])
                             * ringTap (wt, ph2, f2 * fd, sr, vazWave[1], vs[1]);
            }
            if (cfg.uniMono) monoSum += m;
            else             { outL += m * gL[i]; outR += m * gR[i]; }
        }

        if (cfg.uniMono)
        {
            // Same per-copy energy as the stereo path's centre pan: cos(45°) * norm.
            const float g = 0.70710678f * norm;
            outL = outR = monoSum * g;
        }

        if (cfg.subOn)     // TRUE bypass: sub does not run at all when off
        {
            const float s = (float) sub.next (pitchHz / subDiv, subVazWave, subVazShape) * cfg.subLevel;
            outL += s; outR += s;
        }

        L = outL; R = outR;
    }
};
} // namespace mb
