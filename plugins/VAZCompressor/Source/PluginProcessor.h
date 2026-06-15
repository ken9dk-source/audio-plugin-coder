#pragma once
#include <juce_audio_processors/juce_audio_processors.h>
#include <juce_gui_extra/juce_gui_extra.h>
#include <atomic>

//==============================================================================
// VAZCompressor — standalone clone of VAZ 2010's stereo compressor (Core.dll
// ~0x521-0x522). Stereo-linked peak detector → attack/release one-pole → dB
// gain computer (Threshold / Ratio / Makeup) → gain applied to both channels.
// Exposes a Gain-Reduction meter + clip flag to the UI. Stereo in/out.
//==============================================================================
class VAZCompressorAudioProcessor : public juce::AudioProcessor
{
public:
    VAZCompressorAudioProcessor();
    ~VAZCompressorAudioProcessor() override;

    void prepareToPlay (double sampleRate, int samplesPerBlock) override;
    void releaseResources() override {}
    bool isBusesLayoutSupported (const BusesLayout&) const override;
    void processBlock (juce::AudioBuffer<float>&, juce::MidiBuffer&) override;

    juce::AudioProcessorEditor* createEditor() override;
    bool hasEditor() const override { return true; }

    const juce::String getName() const override { return "VAZCompressor"; }
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

    // ── UI meter feed (read by the editor's getMeter native function) ──
    float gainReductionDb() const noexcept { return grMeter.load(); }   // current GR (dB, >= 0)
    bool  takeClip() noexcept { return clip.exchange (false); }         // read-and-clear clip flag

private:
    double curSR    = 44100.0;
    float  envGr    = 0.0f;   // gain-reduction envelope, dB (stereo-linked)

    std::atomic<float> grMeter { 0.0f };
    std::atomic<bool>  clip    { false };

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (VAZCompressorAudioProcessor)
};
