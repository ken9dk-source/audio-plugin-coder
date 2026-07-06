#pragma once
#include <juce_audio_basics/juce_audio_basics.h>
#include <cmath>

//==============================================================================
// PadVoice — a light, lush trance pad for auditioning chords (the product is the
// MIDI, not the sound). 3 detuned PolyBLEP saws + sub sine -> TPT low-pass ->
// ADSR. Master chorus/reverb are applied by the processor, not here.
//==============================================================================
namespace tc
{
    struct PadParams
    {
        std::atomic<float> attackMs   { 12.0f };
        std::atomic<float> releaseMs  { 600.0f };
        std::atomic<float> cutoffHz   { 3500.0f };
        std::atomic<float> detune     { 0.30f };   // 0..1
    };

    struct PadSound : public juce::SynthesiserSound
    {
        bool appliesToNote (int)    override { return true; }
        bool appliesToChannel (int) override { return true; }
    };

    class PadVoice : public juce::SynthesiserVoice
    {
    public:
        explicit PadVoice (const PadParams& p) : params (p) {}

        bool canPlaySound (juce::SynthesiserSound* s) override { return dynamic_cast<PadSound*> (s) != nullptr; }

        void startNote (int midiNote, float velocity, juce::SynthesiserSound*, int) override
        {
            freq = (float) juce::MidiMessage::getMidiNoteInHertz (midiNote);
            level = 0.20f + 0.55f * velocity;
            for (auto& ph : phase) ph = juce::Random::getSystemRandom().nextFloat();
            subPhase = 0.0f;
            ic1 = ic2 = 0.0f;

            juce::ADSR::Parameters ap;
            ap.attack  = juce::jmax (0.001f, params.attackMs.load()  * 0.001f);
            ap.decay   = 0.12f;
            ap.sustain = 0.85f;
            ap.release = juce::jmax (0.005f, params.releaseMs.load() * 0.001f);
            adsr.setSampleRate (getSampleRate());
            adsr.setParameters (ap);
            adsr.noteOn();
        }

        void stopNote (float, bool allowTailOff) override
        {
            if (allowTailOff) adsr.noteOff();
            else { adsr.reset(); clearCurrentNote(); }
        }

        void pitchWheelMoved (int) override {}
        void controllerMoved (int, int) override {}

        void renderNextBlock (juce::AudioBuffer<float>& out, int startSample, int numSamples) override
        {
            if (! adsr.isActive()) return;

            const double sr = getSampleRate();
            // refresh ADSR release in case the user changed it mid-note
            juce::ADSR::Parameters ap = adsr.getParameters();
            ap.attack  = juce::jmax (0.001f, params.attackMs.load()  * 0.001f);
            ap.release = juce::jmax (0.005f, params.releaseMs.load() * 0.001f);
            adsr.setParameters (ap);

            // TPT SVF coefficients (gentle 12 dB LP)
            const float fc = juce::jlimit (60.0f, (float) (sr * 0.45), params.cutoffHz.load());
            const float g  = std::tan (juce::MathConstants<float>::pi * fc / (float) sr);
            const float k  = 1.0f / 0.9f;            // ~Q 0.9
            const float a1 = 1.0f / (1.0f + g * (g + k));
            const float a2 = g * a1;
            const float a3 = g * a2;

            const float det = juce::jlimit (0.0f, 1.0f, params.detune.load());
            const float spreadCents = det * 22.0f;
            const float dRatioUp   = std::pow (2.0f, ( spreadCents) / 1200.0f);
            const float dRatioDn   = std::pow (2.0f, (-spreadCents) / 1200.0f);
            const float fr[3]      = { freq * dRatioDn, freq, freq * dRatioUp };
            const float inc[3]     = { fr[0] / (float) sr, fr[1] / (float) sr, fr[2] / (float) sr };
            const float subInc     = (freq * 0.5f) / (float) sr;

            const int chans = out.getNumChannels();
            float* L = out.getWritePointer (0, startSample);
            float* R = chans > 1 ? out.getWritePointer (1, startSample) : nullptr;

            for (int i = 0; i < numSamples; ++i)
            {
                float s0 = sawBlep (phase[0], inc[0]);
                float s1 = sawBlep (phase[1], inc[1]);
                float s2 = sawBlep (phase[2], inc[2]);
                float sub = std::sin (juce::MathConstants<float>::twoPi * subPhase);
                for (int o = 0; o < 3; ++o) { phase[(size_t) o] += inc[o]; if (phase[(size_t) o] >= 1.0f) phase[(size_t) o] -= 1.0f; }
                subPhase += subInc; if (subPhase >= 1.0f) subPhase -= 1.0f;

                float mono = 0.30f * (s0 + s1 + s2) + 0.28f * sub;

                // TPT low-pass
                const float v3 = mono - ic2;
                const float v1 = a1 * ic1 + a2 * v3;
                const float v2 = ic2 + a2 * ic1 + a3 * v3;
                ic1 = 2.0f * v1 - ic1;
                ic2 = 2.0f * v2 - ic2;
                float lp = v2;

                const float env = adsr.getNextSample();
                const float g0 = lp * env * level;

                // gentle stereo spread between the detuned saws
                const float gl = 0.30f * (s0 * 0.7f + s1 + s2 * 0.4f) + 0.28f * sub;
                const float gr = 0.30f * (s0 * 0.4f + s1 + s2 * 0.7f) + 0.28f * sub;
                juce::ignoreUnused (gl, gr);

                L[i] += g0 * 0.52f;   // headroom for 5-voice chords + wet FX
                if (R) R[i] += g0 * 0.52f;
            }

            if (! adsr.isActive()) clearCurrentNote();
        }

    private:
        static float sawBlep (float t, float dt) noexcept
        {
            float v = 2.0f * t - 1.0f;
            // PolyBLEP correction at the discontinuity
            if (t < dt)            { float x = t / dt;            v -= (x + x - x * x - 1.0f); }
            else if (t > 1.0f - dt){ float x = (t - 1.0f) / dt;  v -= (x * x + x + x + 1.0f); }
            return v;
        }

        const PadParams& params;
        juce::ADSR adsr;
        float freq = 220.0f, level = 0.6f;
        float phase[3] { 0.0f, 0.0f, 0.0f };
        float subPhase = 0.0f;
        float ic1 = 0.0f, ic2 = 0.0f; // SVF state
    };
}
