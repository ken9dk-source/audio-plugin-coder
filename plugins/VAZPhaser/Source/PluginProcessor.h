#pragma once
#include <juce_audio_processors/juce_audio_processors.h>
#include <juce_gui_extra/juce_gui_extra.h>
#include <cmath>
#include "VazPhaserEngine.h"

//==============================================================================
// VAZPhaser — VAZ's real Phaser (TFXPhaser, render FUN_005218d8 @0x5218d8): an N-stage (=(p+1)·2) first-order
// all-pass cascade swept by a triangle LFO, with feedback. Ported as fixed-point (Q30) in VazPhaserEngine; the
// all-pass coefs come from VAZ's EXACT 512-entry runtime-dumped LUT (replaces the old tan()-bilinear). Render
// bit-exact vs the decompile (VazOracle fx_phaser_render); coef LUT VERIFIED (fx_phaser_coef_lut).
//==============================================================================
class VAZPhaserAudioProcessor : public juce::AudioProcessor
{
public:
    VAZPhaserAudioProcessor();
    ~VAZPhaserAudioProcessor() override;

    void prepareToPlay (double sampleRate, int samplesPerBlock) override;
    void releaseResources() override {}
    bool isBusesLayoutSupported (const BusesLayout&) const override;
    void processBlock (juce::AudioBuffer<float>&, juce::MidiBuffer&) override;

    juce::AudioProcessorEditor* createEditor() override;
    bool hasEditor() const override { return true; }
    const juce::String getName() const override { return "VAZPhaser"; }
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
    VazPhaserEngine engine;
    double sr = 44100.0;
    double lfoPhase = 0.0;   // continuous LFO phase (drives the engine's 32-bit inc via a running accumulator)
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (VAZPhaserAudioProcessor)
};
