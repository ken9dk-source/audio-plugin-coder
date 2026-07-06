#pragma once
#include <juce_audio_basics/juce_audio_basics.h>
#include "PadVoice.h"

//==============================================================================
// PadSynth — thin juce::Synthesiser wrapper that owns the pad voices + sound and
// exposes parameter setters. Master chorus/reverb are added by the processor.
//==============================================================================
namespace tc
{
    class PadSynth
    {
    public:
        PadSynth()
        {
            for (int i = 0; i < kNumVoices; ++i)
                synth.addVoice (new PadVoice (params));
            synth.addSound (new PadSound());
            synth.setNoteStealingEnabled (true);
        }

        void prepare (double sampleRate)
        {
            synth.setCurrentPlaybackSampleRate (sampleRate);
        }

        void setParams (float attackMs, float releaseMs, float cutoffHz, float detune)
        {
            params.attackMs  = attackMs;
            params.releaseMs = releaseMs;
            params.cutoffHz  = cutoffHz;
            params.detune    = detune;
        }

        void render (juce::AudioBuffer<float>& buffer, juce::MidiBuffer& midi, int startSample, int numSamples)
        {
            synth.renderNextBlock (buffer, midi, startSample, numSamples);
        }

        void allNotesOff() { synth.allNotesOff (0, false); }

        static constexpr int kNumVoices = 16;

    private:
        PadParams params;            // declared before synth so it outlives the voices
        juce::Synthesiser synth;
    };
}
