#pragma once
#include <juce_audio_processors/juce_audio_processors.h>
#include <juce_gui_extra/juce_gui_extra.h>

//==============================================================================
// VAZReverb — standalone clone of VAZ 2010's Reverb. VAZ's reverb is the classic
// Freeverb comb+allpass network (comb tunings 1116/1188/… confirmed in Vaz2010Core.dll),
// which is exactly what juce::Reverb implements — so this is a faithful match.
//==============================================================================
class VAZReverbAudioProcessor : public juce::AudioProcessor
{
public:
    VAZReverbAudioProcessor();
    ~VAZReverbAudioProcessor() override;

    void prepareToPlay (double sampleRate, int samplesPerBlock) override;
    void releaseResources() override {}
    bool isBusesLayoutSupported (const BusesLayout&) const override;
    void processBlock (juce::AudioBuffer<float>&, juce::MidiBuffer&) override;

    juce::AudioProcessorEditor* createEditor() override;
    bool hasEditor() const override { return true; }

    const juce::String getName() const override { return "VAZReverb"; }
    bool acceptsMidi() const override { return false; }
    bool producesMidi() const override { return false; }
    bool isMidiEffect() const override { return false; }
    double getTailLengthSeconds() const override { return 4.0; }

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
    juce::Reverb reverb;
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (VAZReverbAudioProcessor)
};
