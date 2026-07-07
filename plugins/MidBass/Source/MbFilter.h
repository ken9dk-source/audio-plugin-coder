#pragma once
//==============================================================================
// MidBass hybrid filter — Phase 2.
//
// LP24 / LP12  →  VAZ Type-R (bit-exact integer port from VAZClone): Roland-style
//                 integrator cascade with cubic saturation IN the feedback path,
//                 self-oscillating. This is the hero character of the instrument.
// HP12 / BP12  →  ZDF/TPT SVF core derived from TranceAcid's verified SvfFilter,
//                 with the drive structure matched to Type-R (approval condition):
//                 an ALWAYS-ON tanh saturator on the feedback integrator whose
//                 strength tracks pre-drive + resonance, so HP/BP saturate and
//                 bound self-oscillation like the Type-R modes instead of being
//                 sterile. INTENTIONAL DIFFERENCE (documented): the SVF clips with
//                 a smooth tanh, Type-R with VAZ's integer cubic — at extreme
//                 drive Type-R bites slightly harder; character, not a bug.
//
// Shared, engine-independent structure (approval conditions):
//  * ONE cutoff law — modulatedCutoff() — computes the Hz value fed to whichever
//    engine is active, so keytrack/env scaling is identical BY CONSTRUCTION.
//  * Drive PRE and Drive POST stages sit outside both engines (soft() self-
//    limiting clip, no makeup: more drive = denser, TranceKick lesson).
//  * Mode switching is click-free: 256-sample equal-power crossfade; the incoming
//    engine starts from reset state, the outgoing one keeps running until the
//    fade ends. HP12↔BP12 share one SVF state and crossfade only the output taps.
//==============================================================================
#include "VAZTypeR.h"    // via include path ../VAZClone/Source (bit-exact, inline tables)
#include <cmath>
#include <algorithm>

namespace mb
{
//==============================================================================
// Self-limiting soft clip WITHOUT makeup (VAZClone MultiFilter::soft): unity for
// small signals, knee to ±1 at ±1.5. Drive raises input gain → denser output.
inline float softClip (float x)
{
    x = std::clamp (x, -1.5f, 1.5f);
    return x - x * x * x * (1.0f / 6.75f);
}

struct DriveStage
{
    float g = 1.0f;
    bool  on = false;
    void set (float amount01, float maxExtraGain)
    {
        on = amount01 > 1.0e-4f;
        g  = 1.0f + std::clamp (amount01, 0.0f, 1.0f) * maxExtraGain;
    }
    inline float process (float x) const { return on ? softClip (x * g) : x; }
};

//==============================================================================
// ZDF SVF core (Cytomic TPT form), derived from TranceAcid ta::SvfFilter. Gain
// staging externalised to the shared DriveStages; feedback tanh always active.
struct MbSvfCore
{
    double sr = 44100.0;
    float  ic1 = 0.0f, ic2 = 0.0f;
    float  g = 0.0f, k = 2.0f, a1 = 0.0f, a2 = 0.0f, a3 = 0.0f;
    float  satS = 1.0f;

    void prepare (double sampleRate) { sr = sampleRate > 0.0 ? sampleRate : 44100.0; reset(); }
    void reset()                     { ic1 = ic2 = 0.0f; }

    // Feedback-saturation strength tracks pre-drive and resonance so the HP/BP
    // path "hears" drive the way Type-R's cubic feedback does.
    void setSat (float drivePre01, float reso01) { satS = 1.0f + 0.6f * drivePre01 + 0.4f * reso01; }

    void set (double fcHz, float reso01)
    {
        const double fc = std::clamp (fcHz, 20.0, sr * 0.49);
        g  = (float) std::tan (3.14159265358979323846 * fc / sr);
        k  = 2.0f * (1.0f - std::clamp (reso01, 0.0f, 1.0f) * 0.985f);
        a1 = 1.0f / (1.0f + g * (g + k));
        a2 = g * a1;
        a3 = g * a2;
    }

    inline void step (float v0, float& lp, float& bp, float& hp)
    {
        const float v3 = v0 - ic2;
        const float v1 = a1 * ic1 + a2 * v3;
        const float v2 = ic2 + a2 * ic1 + a3 * v3;
        ic1 = 2.0f * v1 - ic1;
        ic2 = 2.0f * v2 - ic2;
        ic1 = std::tanh (ic1 * satS) / satS;      // always-on (Type-R parity), bounds self-osc
        lp = v2; bp = v1; hp = v0 - k * v1 - v2;
    }
};

//==============================================================================
struct MbFilter
{
    enum Mode { LP24 = 0, LP12 = 1, HP12 = 2, BP12 = 3 };

    static constexpr double kEnvOctaves = 5.0;    // ±100 % env amount sweeps ±5 octaves
    static constexpr int    kFadeLen    = 256;    // ~6 ms @ 44.1k mode crossfade
    static constexpr double kPostHpHz   = 10.0;   // Type-R closing HP: DC removal only

    // Measured calibration (impulse-response sweep, 44.1 kHz, 2026-07-07): the
    // bit-exact Type-R realizes its corner ~1.45x ABOVE the commanded Hz — the
    // offset is constant across 250-2000 Hz (log2 spread < 0.02 oct) — and its
    // passband sits ~5.3 dB (4-pole) / ~2.8 dB (2-pole) below unity at working
    // resonance. These constants align realized corner and level with the
    // unity-gain, exact-frequency SVF path, so the cutoff knob is honest and a
    // mode switch is level/frequency-continuous. The reso-dependent passband
    // dip that remains is VAZ character and is deliberately kept.
    static constexpr double kTypeRCutoffCal = 1.0 / 1.45;
    static constexpr float  kR4Gain = 1.78f;      // +5.0 dB
    static constexpr float  kR2Gain = 1.38f;      // +2.8 dB

    double     sr = 44100.0;
    VAZTypeR   r4, r2;                            // separate instances: LP24 / LP12
    MbSvfCore  svf;                               // shared by HP12 + BP12 (tap select)
    DriveStage pre, post;
    float      reso = 0.2f;
    int        mode = LP24, prevMode = LP24;
    int        fadePos = 0;
    bool       fading = false;

    static bool usesSvf (int m) { return m == HP12 || m == BP12; }

    void prepare (double sampleRate)
    {
        sr = sampleRate > 0.0 ? sampleRate : 44100.0;
        r4.prepare (sr); r2.prepare (sr); svf.prepare (sr);
        mode = prevMode = LP24;
        fading = false; fadePos = 0;
    }

    void reset() { r4.reset(); r2.reset(); svf.reset(); fading = false; }

    void setMode (int m)
    {
        if (m == mode) return;
        prevMode = mode; mode = m;
        fadePos = 0; fading = true;
        // Incoming engine starts from clean state; EXCEPT when both modes share
        // the SVF — then the state is continuous and only the output tap fades.
        if (m == LP24) r4.reset();
        if (m == LP12) r2.reset();
        if (usesSvf (m) && ! usesSvf (prevMode)) svf.reset();
    }

    void finishFades() { fading = false; }

    void setParams (float reso01, float drivePre01, float drivePost01)
    {
        reso = std::clamp (reso01, 0.0f, 1.0f);
        pre.set  (drivePre01,  9.0f);             // up to 10x into the filter
        post.set (drivePost01, 4.0f);             // up to 5x output density
        svf.setSat (drivePre01, reso);
    }

    // THE one cutoff law (approval condition): every engine receives Hz from here,
    // so keytrack and filter-env scaling are engine-independent by construction.
    // note 60 (C4) is the keytrack pivot. Returns unclamped Hz; process() clamps.
    static double modulatedCutoff (double baseHz, int midiNote, float keytrack01,
                                   float envAmount, float envValue)
    {
        const double kt  = (double) keytrack01 * (double) (midiNote - 60) / 12.0;
        const double env = (double) envAmount * (double) envValue * kEnvOctaves;
        return baseHz * std::pow (2.0, kt + env);
    }

    inline float engineOut (int m, float x, double fc, float svfBp, float svfHp)
    {
        switch (m)
        {
            case LP24: return kR4Gain * (float) r4.process (4, (double) x, fc * kTypeRCutoffCal, (double) reso, kPostHpHz);
            case LP12: return kR2Gain * (float) r2.process (2, (double) x, fc * kTypeRCutoffCal, (double) reso, kPostHpHz);
            case HP12: return svfHp;
            default:   return svfBp;
        }
    }

    float process (float in, double fcHz)
    {
        const double fc = std::clamp (fcHz, 20.0, sr * 0.45);
        const float  x  = pre.process (in);

        // Run the SVF at most once per sample even when both fade slots use it.
        float svfLp = 0.0f, svfBp = 0.0f, svfHp = 0.0f;
        if (usesSvf (mode) || (fading && usesSvf (prevMode)))
        {
            svf.set (fc, reso);
            svf.step (x, svfLp, svfBp, svfHp);
        }

        float out = engineOut (mode, x, fc, svfBp, svfHp);
        if (fading)
        {
            const float t    = (float) (fadePos + 1) / (float) kFadeLen;
            const float gNew = std::sin (1.57079632679f * t);
            const float gOld = std::cos (1.57079632679f * t);
            out = out * gNew + engineOut (prevMode, x, fc, svfBp, svfHp) * gOld;
            if (++fadePos >= kFadeLen) fading = false;
        }
        return post.process (out);
    }
};
} // namespace mb
