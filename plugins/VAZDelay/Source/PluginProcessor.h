#pragma once
#include <juce_audio_processors/juce_audio_processors.h>
#include <juce_gui_extra/juce_gui_extra.h>
#include <vector>
#include <cmath>

//==============================================================================
// VAZDelay — standalone clone of VAZ 2010's Delay. Two L/R delay lines, 3 routing
// modes (Stereo / Ping-Pong / Double), per-channel Feedback, Tone (1-pole LP in
// the feedback path), Wet/Dry; Link + tempo Sync. Cross-fades delay-time changes.
//==============================================================================
class VAZDelayAudioProcessor : public juce::AudioProcessor
{
public:
    VAZDelayAudioProcessor();
    ~VAZDelayAudioProcessor() override;

    void prepareToPlay (double sampleRate, int samplesPerBlock) override;
    void releaseResources() override {}
    bool isBusesLayoutSupported (const BusesLayout&) const override;
    void processBlock (juce::AudioBuffer<float>&, juce::MidiBuffer&) override;

    juce::AudioProcessorEditor* createEditor() override;
    bool hasEditor() const override { return true; }

    const juce::String getName() const override { return "VAZDelay"; }
    bool acceptsMidi() const override { return false; }
    bool producesMidi() const override { return false; }
    bool isMidiEffect() const override { return false; }
    double getTailLengthSeconds() const override { return 6.0; }

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
    float readInterp (const std::vector<float>& buf, double readPos) const noexcept;

    double sr = 44100.0;
    std::vector<float> bufL, bufR;     // delay lines (sized for 6 s)
    int    bufLen = 1;
    int    writeL = 0, writeR = 0;
    double curDelL = 100.0, curDelR = 100.0;   // smoothed delay length (samples)
    double toneZL = 0.0, toneZR = 0.0;         // 1-pole LP feedback-tone state

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (VAZDelayAudioProcessor)
};
