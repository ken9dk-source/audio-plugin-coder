#pragma once
//==============================================================================
// MidBass — dedicated trance mid-bass synthesizer.
// Phase 3: first audible path — mono voice (stack) → hybrid filter → amp VCA,
// mono note stack with legato fallback to the previously held note.
//==============================================================================
#include <juce_audio_processors/juce_audio_processors.h>
#include "ParameterIDs.hpp"
#include "Params.h"
#include "MbVoice.h"

class MidBassAudioProcessor : public juce::AudioProcessor
{
public:
    MidBassAudioProcessor();
    ~MidBassAudioProcessor() override = default;

    void prepareToPlay (double sampleRate, int samplesPerBlock) override;
    void releaseResources() override {}
    bool isBusesLayoutSupported (const BusesLayout& layouts) const override;
    void processBlock (juce::AudioBuffer<float>&, juce::MidiBuffer&) override;

    juce::AudioProcessorEditor* createEditor() override;
    bool hasEditor() const override;

    const juce::String getName() const override { return "MidBass"; }
    bool acceptsMidi() const override           { return true; }
    bool producesMidi() const override          { return false; }
    bool isMidiEffect() const override          { return false; }
    double getTailLengthSeconds() const override { return 0.0; }

    int getNumPrograms() override                              { return 1; }
    int getCurrentProgram() override                           { return 0; }
    void setCurrentProgram (int) override                      {}
    const juce::String getProgramName (int) override           { return {}; }
    void changeProgramName (int, const juce::String&) override {}

    void getStateInformation (juce::MemoryBlock& destData) override;
    void setStateInformation (const void* data, int sizeInBytes) override;

    juce::AudioProcessorValueTreeState apvts;
    juce::MidiKeyboardState keyboardState;   // fed from processBlock, read by the GUI keyboard strip

    // ---- test hooks ----
    int    currentNoteForTest() const  { return voice.curNote; }
    bool   voiceActiveForTest() const  { return voice.isActive(); }
    double lfo1HzForTest() const       { return voice.lfo1.curHz; }

private:
    void handleMidiEvent (const juce::MidiMessage& m);
    void updateVoiceParams();

    mb::MbVoice voice;
    juce::Array<int> heldNotes;              // mono note stack (last-note priority)
    double curBpm = 120.0;                   // last seen host tempo

    std::atomic<float>* pRaw (const char* id) { return apvts.getRawParameterValue (id); }

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (MidBassAudioProcessor)
};
