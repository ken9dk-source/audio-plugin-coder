#pragma once
//==============================================================================
// MidBass — dedicated trance mid-bass synthesizer. Phase 0: full APVTS parameter
// surface + plugin scaffold; the voice engine arrives in Phases 1-6 (ROADMAP.md).
//==============================================================================
#include <juce_audio_processors/juce_audio_processors.h>
#include "ParameterIDs.hpp"
#include "Params.h"

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

private:
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (MidBassAudioProcessor)
};
