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
#include "MbMod.h"       // LFOs + sync table + 6-slot matrix (Phase 4)
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

        // ---- modulation (Phase 4) ----
        struct LfoP { int wave = LfoWave::Sine; bool sync = false; float rateHz = 2.0f;
                      int rateDiv = 3; float amount = 0.0f; bool retrig = true; int dest = 0; };
        LfoP        lfo[2];
        ModMatrix6  matrix;
        double      bpm = 120.0;
    };

    // Modulation scaling (one place, documented):
    //  * matrix Cutoff: dst sum 1.0 = kEnvOctaves (5) octaves, total clamped ±10 oct,
    //    then Hz-clamped [20, 0.45·sr] in the filter (clamp, never wrap — cond. c)
    //  * matrix Pitch: dst sum 1.0 = ±12 semitones, total clamped ±24 st
    //  * matrix PWM:   dst sum clamped ±1 → pulse-width offset ±0.45 (pw clamps 5-95 %)
    //  * matrix Amp:   VCA gain = clamp(1 + sum, 0, 2)
    //  * matrix Reso:  clamp(base + sum, 0, 1)
    //  * matrix Drive: CONTROL-RATE with an ENFORCED max update interval of
    //    kDriveModInterval (128) samples (~2.9 ms @44.1k) — re-derived inside
    //    process() so the granularity does NOT depend on the host buffer size
    //    (2026-07-07 review note 2) → pre-drive = clamp(base + sum, 0, 1)
    //  * dedicated LFO routes (lfoN_dest × amount): Cutoff ±3 oct, Pitch ±12 st,
    //    PWM ±0.45, Volume ±1 — added into the same dst sums before clamping.
    static constexpr double kModPitchSemis   = 12.0;
    static constexpr double kModPitchClamp   = 24.0;
    static constexpr double kModCutoffClamp  = 10.0;   // octaves
    static constexpr double kLfoCutoffOct    = 3.0;

    // Micro-detune cents per stack unit (symmetric, classic analog-stack spread).
    static constexpr double kStackCents[3][kMaxStack] = {
        { 0.0,  0.0,  0.0, 0.0 },
        { -4.0, 4.0,  0.0, 0.0 },
        { -7.0, -2.5, 2.5, 7.0 } };

    OscEngine   eng[kMaxStack];
    MbFilter    fltL, fltR;
    ta::AdsrEnv ampEnv, fltEnv;
    MbLfo       lfo1, lfo2;
    Params      p;
    double      sr = 44100.0;
    int         units = 1;
    double      stackMul[kMaxStack] = { 1.0, 1.0, 1.0, 1.0 };
    float       stackNorm = 1.0f;
    int         curNote = -1;
    double      targetHz = 0.0, glidedHz = 0.0, bendMul = 1.0;
    double      glideStepCents = 0.0;             // per sample

    // mod sources (velocity latched at noteOn; wheel/aftertouch fed by the processor)
    float       velocity = 1.0f, modWheel = 0.0f, aftertouch = 0.0f;
    float       lastLfo1 = 0.0f, lastLfo2 = 0.0f;
    double      lastFc = 0.0, lastHz = 0.0;       // test/analysis hooks (post-clamp)

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
        lfo1.prepare (sr, seed ^ 0x1F01u); lfo2.prepare (sr, seed ^ 0x2F02u);
        curNote = -1; targetHz = glidedHz = 0.0; bendMul = 1.0;
        velocity = 1.0f; modWheel = aftertouch = 0.0f; lastLfo1 = lastLfo2 = 0.0f;
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

        // LFO rates: free Hz, or the sync table against the host BPM (re-evaluated
        // every setParams call = every block, so BPM changes track mid-note).
        MbLfo* lfos[2] = { &lfo1, &lfo2 };
        for (int i = 0; i < 2; ++i)
        {
            lfos[i]->setWave (p.lfo[i].wave);
            lfos[i]->setRateHz (p.lfo[i].sync ? syncHz (p.bpm, p.lfo[i].rateDiv)
                                              : (double) p.lfo[i].rateHz);
        }

        updateDriveMod();
        driveModCountdown = 0;                   // param change → re-derive immediately

        ampEnv.setADSR (p.aA, p.aD, p.aS, p.aR);
        fltEnv.setADSR (p.fA, p.fD, p.fS, p.fR);
        glideStepCents = (p.glideMs >= 1.0f) ? 1200.0 / ((double) p.glideMs * 0.001 * sr) : 0.0;
    }

    void setPitchBend (float wheel01)             // -1..+1
    {
        bendMul = std::pow (2.0, (double) wheel01 * (double) p.bendRange / 12.0);
    }

    // Control-rate Drive destination, re-derived at least every kDriveModInterval
    // samples inside process() (host-buffer-independent granularity).
    static constexpr int kDriveModInterval = 128;
    int driveModCountdown = 0;

    void updateDriveMod()
    {
        float src[ModSrc::Count] = { 0.0f, velocity, modWheel, aftertouch, fltEnv.output, lastLfo1, lastLfo2 };
        float dst[ModDst::Count] = {};
        p.matrix.apply (src, dst);
        const float dPre = std::clamp (p.drivePre + dst[ModDst::Drive], 0.0f, 1.0f);
        fltL.setParams (p.reso, dPre, p.drivePost);
        fltR.setParams (p.reso, dPre, p.drivePost);
    }

    // overlapping = another note was already held (legato transition).
    void noteOn (int midiNote, float vel, bool overlapping)
    {
        curNote  = midiNote;
        velocity = std::clamp (vel, 0.0f, 1.0f);
        targetHz = juce::MidiMessage::getMidiNoteInHertz (midiNote);
        if (glidedHz <= 0.0)                                   glidedHz = targetHz;  // first note ever
        else if (! overlapping && p.glideLegatoOnly)           glidedHz = targetHz;  // detached → jump
        // else: keep current pitch and glide toward the target

        const bool retrig = (p.voiceMode == 0) || ! overlapping || ! ampEnv.isActive();
        if (retrig)
        {
            ampEnv.noteOn(); fltEnv.noteOn();
            // LFO note-on retrigger follows the envelope-retrigger condition
            // (legato transitions keep the LFO phase running).
            if (p.lfo[0].retrig) lfo1.retrigger();
            if (p.lfo[1].retrig) lfo2.retrigger();
        }
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

        // ---- modulation sources ----
        lastLfo1 = lfo1.process();
        lastLfo2 = lfo2.process();
        if (--driveModCountdown <= 0)
        {
            driveModCountdown = kDriveModInterval;
            updateDriveMod();
        }
        const float fe = fltEnv.process();
        float src[ModSrc::Count] = { 0.0f, velocity, modWheel, aftertouch, fe, lastLfo1, lastLfo2 };
        float dst[ModDst::Count] = {};
        p.matrix.apply (src, dst);

        // dedicated LFO routes add into the same destination sums (see scaling doc)
        const float lv[2] = { lastLfo1, lastLfo2 };
        for (int i = 0; i < 2; ++i)
        {
            const float a = p.lfo[i].amount * lv[i];
            switch (p.lfo[i].dest)
            {
                case 0:  dst[ModDst::Cutoff] += a * (float) (kLfoCutoffOct / MbFilter::kEnvOctaves); break;
                case 1:  dst[ModDst::Pitch]  += a; break;
                case 2:  dst[ModDst::PWM]    += a; break;
                default: dst[ModDst::Amp]    += a; break;      // Volume (tremolo)
            }
        }

        // ---- apply with CLAMPS (never wrap) ----
        const double pitchSemis = std::clamp ((double) dst[ModDst::Pitch] * kModPitchSemis,
                                              -kModPitchClamp, kModPitchClamp);
        const double hz = glidedHz * bendMul * std::pow (2.0, pitchSemis / 12.0);
        lastHz = hz;

        const float pwOff = std::clamp (dst[ModDst::PWM], -1.0f, 1.0f) * 0.45f;
        const float resoNow = std::clamp (p.reso + dst[ModDst::Reso], 0.0f, 1.0f);
        fltL.reso = resoNow; fltR.reso = resoNow;

        float oL = 0.0f, oR = 0.0f;
        for (int u = 0; u < units; ++u)
        {
            float l = 0.0f, r = 0.0f;
            eng[u].pwMod = pwOff;
            eng[u].setPitch (hz * stackMul[u]);
            eng[u].process (l, r);
            oL += l; oR += r;
        }
        oL *= stackNorm; oR *= stackNorm;

        const double modOct = std::clamp ((double) dst[ModDst::Cutoff] * MbFilter::kEnvOctaves,
                                          -kModCutoffClamp, kModCutoffClamp);
        double fc = MbFilter::modulatedCutoff ((double) p.cutoffHz, curNote < 0 ? 60 : curNote,
                                               p.keytrack, p.envAmt, fe)
                    * std::pow (2.0, modOct);
        fc = std::clamp (fc, 20.0, sr * 0.45);                 // same pin the filter applies
        lastFc = fc;

        const float ampMul = std::clamp (1.0f + dst[ModDst::Amp], 0.0f, 2.0f);
        const float ae = ampEnv.process() * p.outGain * ampMul;
        L = fltL.process (oL, fc) * ae;
        R = fltR.process (oR, fc) * ae;
    }
};
} // namespace mb
