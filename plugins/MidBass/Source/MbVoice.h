#pragma once
//==============================================================================
// MidBass mono voice — Phase 3. First complete signal path:
//   stack of OscEngines (1x/2x/4x, micro-detuned, 1/sqrt(n) summed)
//   → stereo MbFilter pair (filter env via the shared cutoff law, keytrack)
//   → amp env VCA.
//
// Envelopes: TranceAcid's verified exponential ADSR (ta::AdsrEnv). Retrigger is
// click-free BY CONSTRUCTION: noteOn() re-enters Attack from the CURRENT output
// (no reset to zero), oscillator phases free-run, and the filter state is never
// reset on a note event — the "retrigger click" that normally comes from a
// high-resonance filter being state-reset simply has no code path here.
//
// Glide: VAZClone port — constant-rate in cents (glideMs = time per octave),
// with the legato-only option: when glideLegatoOnly, a detached (non-overlapping)
// note jumps instantly and only overlapping notes glide.
//
// Voice stack: N full OscEngine units sharing envelopes and ONE stereo filter
// pair (units sum pre-filter). Per-unit filters were considered and rejected:
// at mid-bass the audible gain is marginal and the CPU cost (8 Type-R instances
// at 4x stereo) works against the < 5 % CPU product target.
// SONIC CONSEQUENCE (character decision, 2026-07-07 review): because the units
// sum BEFORE the filter, the stack's micro-detune beats intermodulate through
// the filter's feedback saturation instead of layering in parallel — i.e. the
// beat amplitude pumps the drive slightly, the way a detuned stack behaves in
// an analog mono synth. Per-unit filters would sound cleaner/wider and less
// glued; that is exactly what this instrument does not want.
//==============================================================================
#include "MbOscillators.h"
#include "MbFilter.h"
#include "Envelope.h"    // ta::AdsrEnv — include dir ../TranceAcid/Source (verified port)
#include <juce_audio_basics/juce_audio_basics.h>

namespace mb
{
struct MbVoice
{
    static constexpr int kMaxStack = 4;

    struct Params
    {
        OscEngine::Config oscCfg;                 // shared by all stack units
        int   stack = 0;                          // 0 = 1x, 1 = 2x, 2 = 4x
        int   voiceMode = 0;                      // 0 = Retrig, 1 = Legato
        float glideMs = 0.0f;                     // time per octave
        bool  glideLegatoOnly = true;
        int   fltMode = MbFilter::LP24;
        float cutoffHz = 700.0f, reso = 0.2f, keytrack = 0.0f, envAmt = 0.4f;
        float drivePre = 0.0f, drivePost = 0.0f;
        float fA = 0.1f, fD = 120.0f, fS = 0.0f,   fR = 50.0f;   // ms/%
        float aA = 0.5f, aD = 300.0f, aS = 100.0f, aR = 10.0f;
        float outGain = 1.0f;
        int   bendRange = 2;
    };

    // Micro-detune cents per stack unit (symmetric, classic analog-stack spread).
    static constexpr double kStackCents[3][kMaxStack] = {
        { 0.0,  0.0,  0.0, 0.0 },
        { -4.0, 4.0,  0.0, 0.0 },
        { -7.0, -2.5, 2.5, 7.0 } };

    OscEngine   eng[kMaxStack];
    MbFilter    fltL, fltR;
    ta::AdsrEnv ampEnv, fltEnv;
    Params      p;
    double      sr = 44100.0;
    int         units = 1;
    double      stackMul[kMaxStack] = { 1.0, 1.0, 1.0, 1.0 };
    float       stackNorm = 1.0f;
    int         curNote = -1;
    double      targetHz = 0.0, glidedHz = 0.0, bendMul = 1.0;
    double      glideStepCents = 0.0;             // per sample

    void prepare (double sampleRate, uint32_t seed = 0x9E3779B9u)
    {
        sr = sampleRate > 0.0 ? sampleRate : 44100.0;
        for (int u = 0; u < kMaxStack; ++u)
        {
            eng[u].prepare (sr, seed + (uint32_t) u * 0x9E3779B9u);
            eng[u].seedPhases (seed * 2654435761u + (uint32_t) u * 40503u);  // deterministic renders
        }
        fltL.prepare (sr); fltR.prepare (sr);
        ampEnv.prepare (sr); fltEnv.prepare (sr);
        curNote = -1; targetHz = glidedHz = 0.0; bendMul = 1.0;
    }

    void setParams (const Params& np)
    {
        p = np;
        units = 1 << std::clamp (p.stack, 0, 2);
        stackNorm = 1.0f / std::sqrt ((float) units);
        for (int u = 0; u < units; ++u)
        {
            stackMul[u] = std::pow (2.0, kStackCents[std::clamp (p.stack, 0, 2)][u] / 1200.0);
            eng[u].setConfig (p.oscCfg);
        }
        fltL.setMode (p.fltMode);            fltR.setMode (p.fltMode);
        fltL.setParams (p.reso, p.drivePre, p.drivePost);
        fltR.setParams (p.reso, p.drivePre, p.drivePost);
        ampEnv.setADSR (p.aA, p.aD, p.aS, p.aR);
        fltEnv.setADSR (p.fA, p.fD, p.fS, p.fR);
        glideStepCents = (p.glideMs >= 1.0f) ? 1200.0 / ((double) p.glideMs * 0.001 * sr) : 0.0;
    }

    void setPitchBend (float wheel01)             // -1..+1
    {
        bendMul = std::pow (2.0, (double) wheel01 * (double) p.bendRange / 12.0);
    }

    // overlapping = another note was already held (legato transition).
    void noteOn (int midiNote, float /*velocity*/, bool overlapping)
    {
        curNote  = midiNote;
        targetHz = juce::MidiMessage::getMidiNoteInHertz (midiNote);
        if (glidedHz <= 0.0)                                   glidedHz = targetHz;  // first note ever
        else if (! overlapping && p.glideLegatoOnly)           glidedHz = targetHz;  // detached → jump
        // else: keep current pitch and glide toward the target

        const bool retrig = (p.voiceMode == 0) || ! overlapping;
        if (retrig) { ampEnv.noteOn(); fltEnv.noteOn(); }
        else if (! ampEnv.isActive()) { ampEnv.noteOn(); fltEnv.noteOn(); }          // safety: silent voice always starts
    }

    void noteOff() { ampEnv.noteOff(); fltEnv.noteOff(); }
    bool isActive() const { return ampEnv.isActive(); }

    void process (float& L, float& R)
    {
        // Constant-rate glide (cents per sample; VAZClone port).
        if (glidedHz != targetHz)
        {
            if (glideStepCents <= 0.0) glidedHz = targetHz;
            else
            {
                const double toTarget = 1200.0 * std::log2 (targetHz / glidedHz);
                if (std::abs (toTarget) <= glideStepCents) glidedHz = targetHz;
                else glidedHz *= std::pow (2.0, (toTarget > 0.0 ? glideStepCents : -glideStepCents) / 1200.0);
            }
        }

        const double hz = glidedHz * bendMul;
        float oL = 0.0f, oR = 0.0f;
        for (int u = 0; u < units; ++u)
        {
            float l = 0.0f, r = 0.0f;
            eng[u].setPitch (hz * stackMul[u]);
            eng[u].process (l, r);
            oL += l; oR += r;
        }
        oL *= stackNorm; oR *= stackNorm;

        const float  fe = fltEnv.process();
        const double fc = MbFilter::modulatedCutoff ((double) p.cutoffHz, curNote < 0 ? 60 : curNote,
                                                     p.keytrack, p.envAmt, fe);
        const float ae = ampEnv.process() * p.outGain;
        L = fltL.process (oL, fc) * ae;
        R = fltR.process (oR, fc) * ae;
    }
};
} // namespace mb
