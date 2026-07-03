#pragma once
#include <juce_audio_processors/juce_audio_processors.h>
#include <juce_gui_extra/juce_gui_extra.h>
#include "VazChorusEngine.h"

//==============================================================================
// VAZChorus — VAZ's real Chorus (TFXChorus, render FUN_00518ad8 @0x518ad8): a dual-LFO ensemble on ONE
// shared mono-summed delay line — 2 LFOs each drive 3 phase-shifted (0°/±120°) modulations that ADD into
// 3 COMBINED delay taps (not 6 independent), with tap-difference stereo. Ported as fixed-point in
// VazChorusEngine (render bit-exact vs the decompile; VazOracle fx_chorus_render). Waveform 0 sine · 1
// trapezoid · 2 triangle (VAZ mode order). Base delay = round((delay+1)/5.12·sr/1000) (FUN_00518fbc).
//==============================================================================
class VAZChorusAudioProcessor : public juce::AudioProcessor
{
public:
    VAZChorusAudioProcessor();
    ~VAZChorusAudioProcessor() override;

    void prepareToPlay (double sampleRate, int samplesPerBlock) override;
    void releaseResources() override {}
    bool isBusesLayoutSupported (const BusesLayout&) const override;
    void processBlock (juce::AudioBuffer<float>&, juce::MidiBuffer&) override;

    juce::AudioProcessorEditor* createEditor() override;
    bool hasEditor() const override { return true; }
    const juce::String getName() const override { return "VAZChorus"; }
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
    VazChorusEngine engine;
    double sr = 44100.0;
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (VAZChorusAudioProcessor)
};
