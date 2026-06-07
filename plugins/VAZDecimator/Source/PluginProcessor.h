#pragma once
#include <juce_audio_processors/juce_audio_processors.h>
#include <juce_gui_extra/juce_gui_extra.h>

//==============================================================================
// VAZDecimator — standalone clone of VAZ 2010's Decimator. Two controls (Core.dll
// FUN_005149d8): Sample Rate (sample-and-hold downsampling → aliasing) + Bit Depth
// (quantisation; VAZ's heaviest step = 1/128). Stereo in/out.
//==============================================================================
class VAZDecimatorAudioProcessor : public juce::AudioProcessor
{
public:
    VAZDecimatorAudioProcessor();
    ~VAZDecimatorAudioProcessor() override;

    void prepareToPlay (double sampleRate, int samplesPerBlock) override;
    void releaseResources() override {}
    bool isBusesLayoutSupported (const BusesLayout&) const override;
    void processBlock (juce::AudioBuffer<float>&, juce::MidiBuffer&) override;

    juce::AudioProcessorEditor* createEditor() override;
    bool hasEditor() const override { return true; }

    const juce::String getName() const override { return "VAZDecimator"; }
    bool acceptsMidi() const override { return false; }
    bool producesMidi() const override { return false; }
    bool isMidiEffect() const override { return false; }
    double getTailLengthSeconds() const override { return 0.0; }

    int getNumPrograms() override { return 1; }
    int getCurrentProgram() override { return 0; }
    void setCurrentProgram (int) override {}
    const juce::String getProgramName (int) override { return {}; }
    void changeProgramName (int, const juce::String&) override {}

    void getStateInformation (juce::MemoryBlock&) override;
    void setStateInformation (const void*, int) override;

    juce::AudioProcessorValueTreeState apvts;
    static juce::AudioProcessorValueTreeState::ParameterLayout createParameterLayout();

private:
    double curSR = 44100.0;
    float  held[2]  = { 0.0f, 0.0f };   // per-channel sample-and-hold value
    double accum[2] = { 1.0, 1.0 };     // per-channel S&H phase (start at 1 → latch first sample)

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (VAZDecimatorAudioProcessor)
};
