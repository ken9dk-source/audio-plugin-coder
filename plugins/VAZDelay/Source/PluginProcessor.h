#pragma once
#include <juce_audio_processors/juce_audio_processors.h>
#include <juce_gui_extra/juce_gui_extra.h>
#include <vector>
#include <cmath>
#include "VazDelayEngine.h"

//==============================================================================
// VAZDelay — VAZ's real Delay (TFXDelay, render FUN_0051bba8 @0x51bba8): a stereo circular delay with a one-pole
// damping LP in each feedback path, per-channel dry/wet, and 3 routing modes (0 Stereo · 1 Ping-Pong = cross
// feedback · ≥2 serial Double). Ported as fixed-point in VazDelayEngine (integer taps, no interpolation — matches
// VAZ); render bit-exact vs the decompile (VazOracle fx_delay_render). Link + tempo Sync; delay-time smoothing.
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
    VazDelayEngine engine;
    double sr = 44100.0;
    double curDelL = 4410.0, curDelR = 4410.0;   // smoothed delay length (samples) → quantised to the engine's int taps

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (VAZDelayAudioProcessor)
};
