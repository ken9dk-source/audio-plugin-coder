#pragma once
#include <juce_audio_basics/juce_audio_basics.h>
#include <atomic>
#include <cmath>

//==============================================================================
// SimpleSynth — a small, reusable subtractive voice (1-2 PolyBLEP saws + sub
// sine -> TPT low-pass with its own filter envelope -> tanh drive -> amp ADSR).
// Configured for BASS (deep, punchy) and PLUCK (bright, fast-decay) so the
// bassline and arp/counter layers get their own timbre instead of the pad.
//==============================================================================
namespace tc
{
    struct SynthConfig
    {
        std::atomic<float> ampA { 2.0f }, ampD { 150.0f }, ampS { 0.4f }, ampR { 120.0f }; // ms / 0..1 / ms
        std::atomic<float> cutoff { 1200.0f }, filtEnv { 2500.0f };                          // Hz base + env amount
        std::atomic<float> filtD { 180.0f }, filtS { 0.2f };                                 // filter env (A=1ms, R=ampR)
        std::atomic<float> reso { 0.3f };
        std::atomic<float> drive { 1.5f };
        std::atomic<float> subLevel { 0.5f }, detune { 8.0f };                               // 0..1 / cents
        std::atomic<int>   numSaws { 2 };
        std::atomic<float> level { 0.85f };
    };

    struct SimpleSound : public juce::SynthesiserSound
    {
        bool appliesToNote (int)    override { return true; }
        bool appliesToChannel (int) override { return true; }
    };

    class SimpleVoice : public juce::SynthesiserVoice
    {
    public:
        explicit SimpleVoice (const SynthConfig& c) : cfg (c) {}

        bool canPlaySound (juce::SynthesiserSound* s) override { return dynamic_cast<SimpleSound*> (s) != nullptr; }

        void startNote (int midiNote, float velocity, juce::SynthesiserSound*, int) override
        {
            freq = (float) juce::MidiMessage::getMidiNoteInHertz (midiNote);
            vel  = 0.3f + 0.7f * velocity;
            phase[0] = phase[1] = 0.0f;
            subPhase = 0.0f; ic1 = ic2 = 0.0f;

            const double sr = getSampleRate();
            amp.setSampleRate (sr); filt.setSampleRate (sr);
            juce::ADSR::Parameters ap { juce::jmax (0.001f, cfg.ampA.load() * 0.001f),
                                        juce::jmax (0.001f, cfg.ampD.load() * 0.001f),
                                        juce::jlimit (0.0f, 1.0f, cfg.ampS.load()),
                                        juce::jmax (0.005f, cfg.ampR.load() * 0.001f) };
            amp.setParameters (ap);
            juce::ADSR::Parameters fp { 0.001f, juce::jmax (0.001f, cfg.filtD.load() * 0.001f),
                                        juce::jlimit (0.0f, 1.0f, cfg.filtS.load()),
                                        juce::jmax (0.005f, cfg.ampR.load() * 0.001f) };
            filt.setParameters (fp);
            amp.noteOn(); filt.noteOn();
        }

        void stopNote (float, bool allowTailOff) override
        {
            if (allowTailOff) { amp.noteOff(); filt.noteOff(); }
            else { amp.reset(); clearCurrentNote(); }
        }

        void pitchWheelMoved (int) override {}
        void controllerMoved (int, int) override {}

        void renderNextBlock (juce::AudioBuffer<float>& out, int startSample, int numSamples) override
        {
            if (! amp.isActive()) return;
            const double sr = getSampleRate();

            const int   saws    = juce::jlimit (1, 2, cfg.numSaws.load());
            const float det      = std::pow (2.0f, cfg.detune.load() / 1200.0f);
            const float inc0     = freq / (float) sr;
            const float inc1     = (freq * det) / (float) sr;
            const float subInc   = (freq * 0.5f) / (float) sr;
            const float subLvl   = cfg.subLevel.load();
            const float drive    = juce::jmax (1.0f, cfg.drive.load());
            const float lvl       = cfg.level.load() * vel;
            const float baseCut  = cfg.cutoff.load();
            const float envCut   = cfg.filtEnv.load();
            const float q         = 0.5f + juce::jlimit (0.0f, 1.0f, cfg.reso.load()) * 6.0f;
            const float k         = 1.0f / q;

            float* L = out.getWritePointer (0, startSample);
            float* R = out.getNumChannels() > 1 ? out.getWritePointer (1, startSample) : nullptr;

            for (int i = 0; i < numSamples; ++i)
            {
                float s = sawBlep (phase[0], inc0);
                if (saws > 1) s += sawBlep (phase[1], inc1);
                s *= (saws > 1 ? 0.5f : 1.0f);
                const float sub = std::sin (juce::MathConstants<float>::twoPi * subPhase);
                float x = 0.85f * s + subLvl * sub;

                phase[0] += inc0; if (phase[0] >= 1.0f) phase[0] -= 1.0f;
                phase[1] += inc1; if (phase[1] >= 1.0f) phase[1] -= 1.0f;
                subPhase += subInc; if (subPhase >= 1.0f) subPhase -= 1.0f;

                // filter env -> cutoff
                const float fe  = filt.getNextSample();
                const float fc  = juce::jlimit (40.0f, (float) (sr * 0.45), baseCut + envCut * fe);
                const float g   = std::tan (juce::MathConstants<float>::pi * fc / (float) sr);
                const float a1  = 1.0f / (1.0f + g * (g + k));
                const float a2  = g * a1, a3 = g * a2;
                const float v3  = x - ic2;
                const float v1  = a1 * ic1 + a2 * v3;
                const float v2  = ic2 + a2 * ic1 + a3 * v3;
                ic1 = 2.0f * v1 - ic1; ic2 = 2.0f * v2 - ic2;
                float y = std::tanh (v2 * drive) / std::tanh (drive);

                y *= amp.getNextSample() * lvl;
                L[i] += y;
                if (R) R[i] += y;
            }

            if (! amp.isActive()) clearCurrentNote();
        }

    private:
        static float sawBlep (float t, float dt) noexcept
        {
            float v = 2.0f * t - 1.0f;
            if (t < dt)             { float x = t / dt;           v -= (x + x - x * x - 1.0f); }
            else if (t > 1.0f - dt) { float x = (t - 1.0f) / dt;  v -= (x * x + x + x + 1.0f); }
            return v;
        }

        const SynthConfig& cfg;
        juce::ADSR amp, filt;
        float freq = 110.0f, vel = 0.8f;
        float phase[2] { 0.0f, 0.0f };
        float subPhase = 0.0f, ic1 = 0.0f, ic2 = 0.0f;
    };

    class SimpleSynth
    {
    public:
        explicit SimpleSynth (int numVoices = 8)
        {
            for (int i = 0; i < numVoices; ++i) synth.addVoice (new SimpleVoice (cfg));
            synth.addSound (new SimpleSound());
            synth.setNoteStealingEnabled (true);
        }
        void prepare (double sr) { synth.setCurrentPlaybackSampleRate (sr); }
        void render (juce::AudioBuffer<float>& b, juce::MidiBuffer& m, int s, int n) { synth.renderNextBlock (b, m, s, n); }

        SynthConfig cfg;     // declared before synth so it outlives the voices
    private:
        juce::Synthesiser synth;
    };
}
