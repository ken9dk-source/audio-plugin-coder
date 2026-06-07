#pragma once
#include <juce_audio_basics/juce_audio_basics.h>
#include "Synth.h"

// Per-sample modulation-source bus, computed once per block by the processor and read
// by both the voices (FM) and the filter. Index matches modSrcs[]/MODSOURCES[].
struct ModBus
{
    const float* lfo1 = nullptr;
    const float* lfo2 = nullptr;
    const float* lfo3 = nullptr;
    const float* env2 = nullptr;
    const float* ma1  = nullptr;     // Mod Amplifier 1 output
    const float* ma2  = nullptr;     // Mod Amplifier 2 output
    const float* lag   = nullptr;    // Lag Processor output
    const float* noise = nullptr;    // white-noise mod source
    float modWheel = 0.0f, velocity = 0.0f, keyTrack = 0.0f, aftertouch = 0.0f, ctrlB = 0.0f;

    inline float value (int src, int i) const noexcept
    {
        switch (src)
        {
            case 1:  return lfo1 ? lfo1[i] : 0.0f;
            case 2:  return lfo2 ? lfo2[i] : 0.0f;
            case 3:  return lfo3 ? lfo3[i] : 0.0f;
            case 5:  return env2 ? env2[i] : 0.0f;
            case 6:  return ma1  ? ma1[i]  : 0.0f;     // Mod Amplifier 1
            case 7:  return ma2  ? ma2[i]  : 0.0f;     // Mod Amplifier 2
            case 8:  return lag  ? lag[i]  : 0.0f;     // Lag Processor
            case 10: return keyTrack;
            case 12: return noise ? noise[i] : 0.0f;   // Noise
            case 17: return velocity;
            case 18: return aftertouch;                // MIDI Pressure (channel aftertouch)
            case 19: return modWheel;
            case 20: return ctrlB;                     // MIDI Control B (CC11 / Expression)
            default: return 0.0f;     // None + not-yet-implemented sources
        }
    }
};

// Parameter snapshot the processor refreshes each block (read-only in the audio render).
struct VoiceParams
{
    int   o1Wave = 0, o2Wave = 0, o1Octave = 2, o2Octave = 2;   // 0 = Sawtooth, octave 2 = 8'
    float o1Coarse = 0.5f, o1Fine = 0.5f, o2Coarse = 0.5f, o2Fine = 0.5f; // 0.5 = center (no transpose)
    float o1Shape = 0.0f, o2Shape = 0.0f;          // Waveshape per oscillator
    float detuneCents = 0.0f;                      // per-voice detune amount (Poly/Unison Detune)
    double duoHighHz = 0.0;                        // Duo mode: Osc2 plays this (highest held) note
    float o1Level = 1.0f, o2Detune = 0.0f, o2Level = 0.0f, noise = 0.0f;
    float atk = 0.0f, dec = 0.3f, sus = 1.0f, rel = 0.2f;
    int   mix1Src = 0, mix2Src = 0, mix3Src = 0;   // Mixer channel sources (default Osc1/Osc2/Noise)
    bool  mix1Post = false, mix2Post = false, mix3Post = false;   // Post = bypass the filter
    bool  link = false;                            // Osc1 FM input 1 → also modulate Osc2
    int   o1FmSrc = 0, o2FmSrc = 0;                // Frequency Modulation source (mod-bus index)
    float o1FmAmt = 0.0f, o2FmAmt = 0.0f;          // FM depth
    int   o1WsSrc = 0, o2WsSrc = 0;                // Waveshape Modulation source
    float o1WsAmt = 0.0f, o2WsAmt = 0.0f;          // Waveshape mod depth
    float portamento = 0.0f;                       // glide time 0..1
    bool  portaExp = false;                        // exponential glide (off = linear constant-rate)
    bool  portaAuto = true;                        // Portamento Auto: glide only on overlapping (legato) notes
    float bendRange = 1.0f / 23.0f;                // pitch-bend range (0..1 → 1..24 st)
    bool  e1Reset = false, e1Cycle = false, e1Curve = false;   // Env1 modes
    // Per-voice filter (osc → filter → amp)
    int   filterMode = 19;
    float baseCut = 1.0f, baseRes = 0.0f, fltAux = 0.5f, hpNorm = 0.0f, fltDrive = 1.0f, nyq = 19845.0f;
    int   cutSrc1 = 0, cutSrc2 = 0, resSrc = 0, ampSrc = 0, panSrc = 0;
    float cutAmt1 = 0.0f, cutAmt2 = 0.0f, resAmt = 0.0f, ampAmt = 0.0f, panAmt = 0.0f, ampLevel = 0.8f;
    float e2Atk = 0.0f, e2Dec = 0.3f, e2Sus = 1.0f, e2Rel = 0.2f;
    bool  e2Reset = false, e2Cycle = false, e2Curve = false;
    bool  e1Multi = false, e2Multi = false;        // Multi-trigger (re-attack env on a legato mono note)
    const SampleData* osc1Sample = nullptr;        // Sample-osc data (OSC1 / OSC2), owned by the processor
    const SampleData* osc2Sample = nullptr;
    int   e2ModSrc = 0, e2Dest = 4;                // Env2 self-mod: source + dest (4=None)
    float e2ModAmt = 0.0f;
    const ModBus* modBus = nullptr;                // shared per-sample mod-source bus
};

struct VAZSound : juce::SynthesiserSound
{
    bool appliesToNote (int) override { return true; }
    bool appliesToChannel (int) override { return true; }
};

// Synthesiser that does NOT stop voices already playing the same note on noteOn (needed for
// Unison: N voices on one note — the default juce::Synthesiser::noteOn stops any ringing voice on
// that note, collapsing unison to one). ALSO implements MONO legato: when monoVoiceIdx >= 0 every
// note drives that single dedicated voice; channel-2 note-ons are legato transitions (glide + no
// env retrigger unless Multi), routed through VAZVoice::legatoTo which bypasses startVoice's
// hard-reset. noteOn is defined out-of-line (below VAZVoice) so it can see VAZVoice::legatoTo.
class VAZVoice;
struct VAZSynth : juce::Synthesiser
{
    int monoVoiceIdx = -1;   // >=0 → MONO mode (dedicated legato voice index); -1 → Poly/Unison
    void noteOn (int midiChannel, int midiNoteNumber, float velocity) override;
};

class VAZVoice : public juce::SynthesiserVoice
{
public:
    // voiceDetune is a fixed −1..+1 position for this voice; scaled by p.detuneCents
    // (per-voice analog drift in Poly, chorus spread in Unison).
    VAZVoice (const VoiceParams& params, double detunePos) : p (params), voiceDetune (detunePos) {}

    bool canPlaySound (juce::SynthesiserSound* s) override { return dynamic_cast<VAZSound*> (s) != nullptr; }

    void setCurrentPlaybackSampleRate (double sr) override
    {
        juce::SynthesiserVoice::setCurrentPlaybackSampleRate (sr);
        osc1.prepare (sr); osc2.prepare (sr); osc3.prepare (sr > 0.0 ? sr : 44100.0);
        tilt.setCutoff (14000.0, sr > 0.0 ? sr : 44100.0); // subtle analog softening (refine w/ clean render)
        amp.setSampleRate (sr > 0.0 ? sr : 44100.0);
        filt.prepare (sr > 0.0 ? sr : 44100.0);
        env2.setSampleRate (sr > 0.0 ? sr : 44100.0);
    }

    void startNote (int midiNote, float velocity, juce::SynthesiserSound*, int) override
    {
        noteHz = juce::MidiMessage::getMidiNoteInHertz (midiNote);
        if (glidedHz <= 0.0 || p.portaAuto) glidedHz = noteHz;  // first note OR Portamento Auto → no glide on a fresh (non-overlapping) note; legato notes still glide
        level  = juce::jmax (0.05f, velocity);
        voiceKeyTrack = juce::jlimit (0.0, 1.0, (midiNote - 24) / 72.0);   // C1..C7 (per-voice key-track)
        voiceVel = velocity;
        osc1.sample = p.osc1Sample; osc2.sample = p.osc2Sample;   // Sample-osc data (stable ptr)
        osc1.samplePhase = 0.0; osc2.samplePhase = 0.0;           // retrigger sample from start
        // Oscillators are FREE-RUNNING (no phase reset on note-on) — this avoids the
        // correlated-start transient/click on Multi-Saw that VAZ doesn't have.
        updateAmp(); updateFilterEnv();
        amp.noteOn(); env2.noteOn();
    }

    void stopNote (float, bool allowTailOff) override
    {
        if (allowTailOff) { amp.noteOff(); env2.noteOff(); }
        else { amp.reset(); env2.reset(); clearCurrentNote(); }
    }

    // Legato note change (MONO channel-2 path): glide to the new note WITHOUT a hard reset.
    // Each envelope re-attacks ONLY if its Multi-trigger is on (manual: "Multi sets the envelope
    // to return to the attack phase if a second note is pressed in monophonic mode").
    void legatoTo (int midiNote, float velocity) noexcept
    {
        noteHz = juce::MidiMessage::getMidiNoteInHertz (midiNote);   // glide target — legato ALWAYS glides (notes overlap)
        level  = juce::jmax (0.05f, velocity);
        voiceKeyTrack = juce::jlimit (0.0, 1.0, (midiNote - 24) / 72.0);
        voiceVel = velocity;
        if (p.e1Multi) amp.noteOn();      // Multi-trigger Env1 → re-attack amplifier envelope
        if (p.e2Multi) env2.noteOn();     // Multi-trigger Env2 → re-attack filter envelope
        updateAmp(); updateFilterEnv();
    }

    void pitchWheelMoved (int v) override { wheelRaw = v; }   // pitchBend recomputed per block from the Bend-range param
    void controllerMoved (int, int) override {}

    void renderNextBlock (juce::AudioBuffer<float>& out, int startSample, int numSamples) override
    {
        if (! amp.isActive()) return;
        updateAmp();
        float e2mod = 0.0f;                                    // Env2 self-modulation (A/D/S/R per Dest)
        if (std::abs (p.e2ModAmt) > 0.0001f && p.e2Dest < 4)
        {
            const int s = p.e2ModSrc;
            const float sv = (s==10) ? (float) voiceKeyTrack : (s==17) ? voiceVel
                           : (p.modBus ? p.modBus->value (s, startSample) : 0.0f);
            e2mod = p.e2ModAmt * sv;
        }
        updateFilterEnv (e2mod); filt.setMode (p.filterMode);

        // octave + coarse (±12 semitone STEPS) + fine (±100 cent STEPS) — VAZ quantises both.
        const double o1Semi = std::round ((p.o1Coarse - 0.5) * 24.0) + std::round ((p.o1Fine - 0.5) * 200.0) / 100.0;
        const double o2Semi = std::round ((p.o2Coarse - 0.5) * 24.0) + std::round ((p.o2Fine - 0.5) * 200.0) / 100.0;

        // Portamento: glide the playing frequency toward the target note (per block).
        const double glideTime = (double) p.portamento * (double) p.portamento * 0.6;   // 0..0.6 s
        if (glideTime < 0.0005) glidedHz = noteHz;
        else if (p.portaExp) glidedHz += (noteHz - glidedHz) * (1.0 - std::exp (-(double) numSamples / (glideTime * getSampleRate())));  // Exp: RC curve (rate ∝ distance)
        else {  // linear constant-rate in cents (VAZ default) → larger intervals take proportionally longer
            const double toTargetCents = 1200.0 * std::log2 (noteHz / glidedHz);
            const double stepCents = (1200.0 / glideTime) * ((double) numSamples / getSampleRate());
            if (std::abs (toTargetCents) <= stepCents) glidedHz = noteHz;
            else glidedHz *= std::pow (2.0, (toTargetCents > 0.0 ? stepCents : -stepCents) / 1200.0);
        }

        pitchBend = std::pow (2.0, ((double) wheelRaw - 8192.0) / 8192.0 * (1.0 + 23.0 * (double) p.bendRange) / 12.0); // ±BendRange semitones
        const double vd = pitchBend * std::pow (2.0, (voiceDetune * (double) p.detuneCents) / 1200.0); // per-voice detune × pitch-bend
        const double f1base = glidedHz * std::pow (2.0, (double) (p.o1Octave - 2) + o1Semi / 12.0) * vd;
        const double o2note = (p.duoHighHz > 0.0) ? p.duoHighHz : glidedHz;       // Duo: Osc2 plays the highest held note
        const double f2base = o2note * std::pow (2.0, (double) (p.o2Octave - 2) + o2Semi / 12.0)
                                 * std::pow (2.0, (p.o2Detune * 50.0) / 1200.0) * vd;

        const float o1g = p.o1Level, o2g = p.o2Level, ng = p.noise;
        const int   w1 = p.o1Wave,   w2 = p.o2Wave;
        const bool  fm1 = (std::abs (p.o1FmAmt) > 0.0001f), fm2 = (std::abs (p.o2FmAmt) > 0.0001f);
        const bool  ws1 = (std::abs (p.o1WsAmt) > 0.0001f), ws2 = (std::abs (p.o2WsAmt) > 0.0001f);
        const double hpHzBase = 20.0 * std::pow (100.0, (double) p.hpNorm);   // +HP stage 20Hz..2kHz

        for (int n = 0; n < numSamples; ++n)
        {
            const int   idx  = startSample + n;
            const float env1  = amp.getNextSample();         // Env1 = the amplifier; also a per-voice mod source
            const float env2v = env2.getNextSample();        // Env2 = this voice's filter envelope
            auto mv = [&] (int src, int i) -> float           // per-voice mod values (Env1/Env2/KeyTrack/Velocity)
            {
                switch (src) {
                    case 4:  return env1;
                    case 5:  return env2v;
                    case 10: return (float) voiceKeyTrack;
                    case 9:  return lastO1;          // Oscillator 1 (audio-rate, previous sample)
                    case 11: return lastO2;          // Oscillator 2
                    case 17: return voiceVel;
                    case 21: return (float) voiceDetune;   // Voice Number ≈ this voice's spread position
                    default: return p.modBus ? p.modBus->value (src, i) : 0.0f;
                }
            };

            double f1 = f1base, f2 = f2base;
            if (fm1) f1 *= std::pow (2.0, (double) mv (p.o1FmSrc, idx) * (double) p.o1FmAmt);   // ± up to 1 oct
            if (fm2) f2 *= std::pow (2.0, (double) mv (p.o2FmSrc, idx) * (double) p.o2FmAmt);
            if (p.link && fm1) f2 *= std::pow (2.0, (double) mv (p.o1FmSrc, idx) * (double) p.o1FmAmt);  // Link: Osc1 FM → Osc2
            double sh1 = p.o1Shape, sh2 = p.o2Shape;
            if (ws1) sh1 = juce::jlimit (0.0, 1.0, sh1 + (double) mv (p.o1WsSrc, idx) * (double) p.o1WsAmt);
            if (ws2) sh2 = juce::jlimit (0.0, 1.0, sh2 + (double) mv (p.o2WsSrc, idx) * (double) p.o2WsAmt);

            const double o1 = osc1.next (f1, w1, sh1);
            if (w2 == 4 && osc1.mainWrapped) osc2.hardReset();        // OSC2 hard Sync to OSC1
            const double o2 = osc2.next (f2, w2, sh2);                // always advance (needed for Ring Mod)
            lastO1 = (float) o1; lastO2 = (float) o2;                 // expose to the mod matrix (Osc1/Osc2 sources)
            const double rm = o1 * o2;                                // Ring Modulator = Osc1 × Osc2
            const double nz = rng.nextDouble() * 2.0 - 1.0;           // white noise
            // 3 mixer channels: source × level. Post channels bypass the filter (mixed in after).
            const double o3 = (p.mix3Src==1) ? osc3.next (glidedHz * vd, 0, 0.0) : 0.0;            // Osc3 = saw, tracks keyboard
            const double c1 = (p.mix1Src==0 ? o1 : p.mix1Src==1 ? rm : p.mix1Src==2 ? nz : 0.0) * o1g;
            const double c2 = (p.mix2Src==0 ? o2 : p.mix2Src==1 ? rm : p.mix2Src==2 ? nz : 0.0) * o2g;
            const double c3 = (p.mix3Src==0 ? nz : p.mix3Src==1 ? o3 : p.mix3Src==2 ? rm : 0.0) * ng;
            double s             = (p.mix1Post?0.0:c1) + (p.mix2Post?0.0:c2) + (p.mix3Post?0.0:c3);  // → filter
            const double postSum = (p.mix1Post?c1:0.0) + (p.mix2Post?c2:0.0) + (p.mix3Post?c3:0.0);  // → bypass filter

            s = tilt.process (s);

            // ── Per-voice FILTER (correct VAZ order: osc → filter → amp) ──
            float coNorm = p.baseCut + p.cutAmt1 * mv (p.cutSrc1, idx) + p.cutAmt2 * mv (p.cutSrc2, idx);
            coNorm = juce::jlimit (0.0f, 1.0f, coNorm);
            const float m3 = p.resAmt * mv (p.resSrc, idx);
            float res = p.baseRes, auxV = p.fltAux; double hpHzI = hpHzBase;
            if      (filt.modRoute == 1) hpHzI = 20.0 * std::pow (100.0, (double) juce::jlimit (0.0f, 1.0f, p.hpNorm + m3));
            else if (filt.modRoute == 2) auxV  = juce::jlimit (0.0f, 1.0f, p.fltAux + m3);
            else                          res  = juce::jlimit (0.0f, 1.0f, p.baseRes + m3);
            const double cutHz = juce::jlimit (20.0, (double) p.nyq, 20.0 * std::pow (900.0, (double) coNorm));
            filt.setParams (cutHz, (double) res, (double) auxV, hpHzI, (double) p.fltDrive);
            double fs = filt.process (s) + postSum;          // Post channels mixed in after the filter
            if (std::abs (p.ampAmt) > 0.0001f) fs *= juce::jlimit (0.0, 2.0, 1.0 + (double) p.ampAmt * (double) mv (p.ampSrc, idx)); // tremolo (±)
            const float vv = (float) fs * env1 * level * p.ampLevel * 0.6f;   // ampLevel = Amplitude-Mod slot-1 depth
            // ── Pan Modulation (per voice) — write stereo ──
            const double pan = (std::abs (p.panAmt) > 0.0001f) ? juce::jlimit (-1.0, 1.0, (double) p.panAmt * (double) mv (p.panSrc, idx)) : 0.0;
            const float lG = pan <= 0.0 ? 1.0f : (float) (1.0 - pan);
            const float rG = pan >= 0.0 ? 1.0f : (float) (1.0 + pan);
            if (out.getNumChannels() >= 2) { out.addSample (0, idx, vv * lG); out.addSample (1, idx, vv * rG); }
            else                            out.addSample (0, idx, vv);

            if (! amp.isActive()) { clearCurrentNote(); break; }
        }
    }

private:
    void updateAmp() noexcept
    {
        auto t = [] (float x) { return 0.001f + x * x * 3.0f; }; // 1ms .. 3s (quadratic)
        amp.setADSR (t (p.atk), t (p.dec), p.sus, t (p.rel));
        amp.setModes (p.e1Reset, p.e1Cycle, p.e1Curve);
    }
    static float ftime (float x) noexcept { return 0.0002f + x * x * 4.0f; }   // filter-env time map
    void updateFilterEnv (float mod = 0.0f) noexcept
    {
        float a = p.e2Atk, d = p.e2Dec, s = p.e2Sus, r = p.e2Rel;
        switch (p.e2Dest) {                                    // 0=Attack 1=Decay 2=Sustain 3=Release 4=None
            case 0: a = juce::jlimit (0.0f, 1.0f, a + mod); break;
            case 1: d = juce::jlimit (0.0f, 1.0f, d + mod); break;
            case 2: s = juce::jlimit (0.0f, 1.0f, s + mod); break;
            case 3: r = juce::jlimit (0.0f, 1.0f, r + mod); break;
            default: break;
        }
        env2.setADSR (ftime (a), ftime (d), s, ftime (r));
        env2.setModes (p.e2Reset, p.e2Cycle, p.e2Curve);
    }

    const VoiceParams& p;
    OscBlock  osc1, osc2, osc3;
    OnePoleLP tilt;
    VAZEnv    amp;
    VAZMultiFilter filt;          // per-voice filter
    VAZEnv    env2;               // per-voice filter envelope
    double    voiceKeyTrack = 0.0;
    float     voiceVel = 1.0f;
    float     lastO1 = 0.0f, lastO2 = 0.0f;
    juce::Random rng;
    double noteHz = 440.0;
    double glidedHz = 0.0;        // portamento current frequency
    double pitchBend = 1.0;       // pitch-wheel frequency multiplier
    int    wheelRaw = 8192;       // last raw pitch-wheel value (center = 8192)
    double voiceDetune = 0.0;     // −1..+1 fixed position for this voice
    float  level = 1.0f;
};

// VAZSynth::noteOn — defined here (after VAZVoice is complete) so the MONO path can call legatoTo.
inline void VAZSynth::noteOn (int midiChannel, int midiNoteNumber, float velocity)
{
    const juce::ScopedLock sl (lock);
    if (monoVoiceIdx >= 0 && monoVoiceIdx < voices.size())          // ── MONO: one dedicated legato voice ──
    {
        auto* mv = static_cast<VAZVoice*> (voices[monoVoiceIdx]);
        for (auto* sound : sounds)
            if (sound->appliesToNote (midiNoteNumber) && sound->appliesToChannel (midiChannel))
            {
                if (midiChannel == 2 && mv->isVoiceActive())        // ch2 = legato (a note is already sounding)
                    mv->legatoTo (midiNoteNumber, velocity);        // glide + conditional env retrigger (no hard reset)
                else
                    startVoice (mv, sound, midiChannel, midiNoteNumber, velocity);  // fresh note (hard-retrigger is fine)
                return;
            }
    }
    else                                                            // ── POLY / UNISON: just don't stop same-note voices ──
        for (auto* sound : sounds)
            if (sound->appliesToNote (midiNoteNumber) && sound->appliesToChannel (midiChannel))
                startVoice (findFreeVoice (sound, midiChannel, midiNoteNumber, isNoteStealingEnabled()),
                            sound, midiChannel, midiNoteNumber, velocity);
}
