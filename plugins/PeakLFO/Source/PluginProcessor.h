#pragma once

#include <juce_audio_processors/juce_audio_processors.h>
#include <juce_dsp/juce_dsp.h>
#include <atomic>
#include "FPCEngine.h"

//==============================================================================
// PeakLFO — FL Fruity Peak Controller LFO (LFO-only), self-modulating volume insert.
// Output model (matches FPC):  out = Base + shape(phase, tension) * Volume
//   Base   = output floor/offset (0..1)
//   Volume = bipolar, log-tapered, sign-flips the wave on negative
//   shape  = tension-warped LFO (FPCEngine, decompiled curve)
// Speed: tempo-synced steps (default, trance) OR free-run Hz (faithful FPC).
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

    std::atomic<float> meterLfo  { 0.5f };
    std::atomic<float> meterGain { 1.0f };

    // Speed step table (LFO period in FL "steps"; 4 steps = 1 beat)
    static constexpr int   kNumSpeedSteps = 10;
    static constexpr float kSpeedSteps[kNumSpeedSteps] = { 0.5f, 1, 2, 3, 4, 8, 16, 32, 64, 128 };
    static const juce::StringArray& speedStepNames();
    static const juce::StringArray& shapeNames();

    // Knob->raw span for tension: |raw| = TENSION_FULL * |knob|. Set to the decompiled
    // divisor (128) so knob |1| spans the FPC tension formula's natural range (|raw|/128 = |knob|).
    static constexpr int TENSION_FULL = 128;

private:
    juce::AudioProcessorValueTreeState::ParameterLayout createParameterLayout();

    FPCEngine engine;
    double currentSampleRate = 44100.0;
    double freeRunPhase = 0.0;
    juce::SmoothedValue<float> smBase, smVol;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (PeakLFOAudioProcessor)
};
