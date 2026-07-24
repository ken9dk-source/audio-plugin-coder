#pragma once

#include <juce_audio_processors/juce_audio_processors.h>
#include <juce_dsp/juce_dsp.h>
#include <atomic>
#include "FPCEngine.h"

//==============================================================================
// PeakLFO — tempo-synced volume LFO (tremolo).
// LFO shape/tension = reverse-engineered FL Fruity Peak Controller (FPCEngine.h).
// Base = overall volume, Depth = how much the LFO swings, Speed = tempo steps,
// Phase = locked to 0/25/50/75 %, Shape = Sine/Triangle/Square/Random.
//==============================================================================
class PeakLFOAudioProcessor : public juce::AudioProcessor
{
public:
    PeakLFOAudioProcessor();
    ~PeakLFOAudioProcessor() override;

    void prepareToPlay (double sampleRate, int samplesPerBlock) override;
    void releaseResources() override {}
    bool isBusesLayoutSupported (const BusesLayout& layouts) const override;
    void processBlock (juce::AudioBuffer<float>&, juce::MidiBuffer&) override;
    using AudioProcessor::processBlock;

    juce::AudioProcessorEditor* createEditor() override;
    bool hasEditor() const override { return true; }

    const juce::String getName() const override { return "PeakLFO"; }
    bool acceptsMidi() const override { return false; }
    bool producesMidi() const override { return false; }
    bool isMidiEffect() const override { return false; }
    double getTailLengthSeconds() const override { return 0.0; }

    int getNumPrograms() override { return 1; }
    int getCurrentProgram() override { return 0; }
    void setCurrentProgram (int) override {}
    const juce::String getProgramName (int) override { return {}; }
    void changeProgramName (int, const juce::String&) override {}

    void getStateInformation (juce::MemoryBlock& destData) override;
    void setStateInformation (const void* data, int sizeInBytes) override;

    juce::AudioProcessorValueTreeState parameters;

    // GUI meter snapshot (0..1)
    std::atomic<float> meterLfo  { 0.5f };   // current LFO shape value
    std::atomic<float> meterGain { 1.0f };   // current applied volume

    // Speed step table (LFO period length in FL "steps"; 4 steps = 1 beat)
    static constexpr int   kNumSpeedSteps = 10;
    static constexpr float kSpeedSteps[kNumSpeedSteps] = { 0.5f, 1, 2, 3, 4, 8, 16, 32, 64, 128 };
    static const juce::StringArray& speedStepNames();
    static const juce::StringArray& phaseNames();

private:
    juce::AudioProcessorValueTreeState::ParameterLayout createParameterLayout();

    FPCEngine engine;
    double currentSampleRate = 44100.0;
    double freeRunPhase = 0.0;                 // used when transport is not playing

    juce::SmoothedValue<float> smBase, smDepth;

    // tension curve calibration (exact FL shape; scale documented in spec §7)
    static constexpr int TENSION_FULL = 64;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (PeakLFOAudioProcessor)
};
