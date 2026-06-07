#pragma once
#include <juce_audio_processors/juce_audio_processors.h>
#include <juce_gui_extra/juce_gui_extra.h>
#include <cmath>

//==============================================================================
// VAZPhaser — N-stage first-order allpass cascade swept by an LFO, with feedback.
// (VAZ Phaser: Stages / Frequency / Feedback / Rate / Depth / L-R Phase / Mix / Gain.)
//==============================================================================
struct APStage
{
    double z = 0.0;
    inline double process (double x, double a) noexcept { double y = -a * x + z; z = x + a * y; return y; }
    void reset() noexcept { z = 0.0; }
};

struct PhaserChannel
{
    APStage ap[12];
    double  fb = 0.0;
    void reset() noexcept { for (auto& s : ap) s.reset(); fb = 0.0; }
    inline double process (double in, int stages, double a, double feedback, double fbSign, double mix, double gain) noexcept
    {
        double x = in + fb * feedback * fbSign;
        for (int i = 0; i < stages; ++i) x = ap[i].process (x, a);
        fb = x;
        return (in * (1.0 - mix) + x * mix) * gain;
    }
};

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
    PhaserChannel chL, chR;
    double sr = 44100.0;
    double lfoPhase = 0.0;
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (VAZPhaserAudioProcessor)
};
