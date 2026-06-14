#pragma once
#include <juce_audio_processors/juce_audio_processors.h>
#include <juce_gui_extra/juce_gui_extra.h>
#include <juce_dsp/juce_dsp.h>

//==============================================================================
// VAZEqualizer — standalone clone of VAZ 2010's 4-band parametric EQ (Core.dll
// 0x517 region). 4 cascaded RBJ biquads per channel: Low and High bands morph
// shelf↔filter via their Q knob (Low: low-shelf→high-pass, High: high-shelf→
// low-pass); the two Mid bands are always peaking. Stereo in/out.
//==============================================================================
class VAZEqualizerAudioProcessor : public juce::AudioProcessor
{
public:
    VAZEqualizerAudioProcessor();
    ~VAZEqualizerAudioProcessor() override;

    void prepareToPlay (double sampleRate, int samplesPerBlock) override;
    void releaseResources() override {}
    bool isBusesLayoutSupported (const BusesLayout&) const override;
    void processBlock (juce::AudioBuffer<float>&, juce::MidiBuffer&) override;

    juce::AudioProcessorEditor* createEditor() override;
    bool hasEditor() const override { return true; }

    const juce::String getName() const override { return "VAZEqualizer"; }
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
    enum Band { Low = 0, LoMid, HiMid, High, NumBands };

    using Filter = juce::dsp::IIR::Filter<float>;
    using Coefs  = juce::dsp::IIR::Coefficients<float>;

    // Build the RBJ coefficients for one band from its normalised gain/freq/q.
    Coefs::Ptr makeBand (int band, float gainN, float freqN, float qN) const;

    double curSR = 44100.0;
    Filter bands[2][NumBands];   // [channel][band]

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (VAZEqualizerAudioProcessor)
};
