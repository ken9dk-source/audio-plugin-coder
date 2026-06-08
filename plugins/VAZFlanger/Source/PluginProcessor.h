#pragma once
#include <juce_audio_processors/juce_audio_processors.h>
#include <juce_gui_extra/juce_gui_extra.h>
#include <vector>
#include <cmath>

//==============================================================================
// VAZFlanger — single modulated fractional delay line + feedback (±polarity),
// swept by an LFO. (VAZ Flanger: Delay Time / Feedback / Rate / Depth / L-R Phase / Mix / Gain.)
// Max delay line ≈115 ms (VAZ: 5080 samples @44.1 kHz, Core.dll 0x51f094). Topology-accurate
// clone of the VAZ Flanger (~0x51e9xx; per-sample loop is fixed-point Q-format → not bit-exact).
//==============================================================================
struct FlangerChannel
{
    std::vector<float> buf;
    int    n  = 0;
    int    wp = 0;
    double fb = 0.0;

    void prepare (int maxSamples) noexcept
    {
        n = juce::jmax (8, maxSamples);
        buf.assign ((size_t) n, 0.0f);
        wp = 0; fb = 0.0;
    }
    void reset() noexcept { std::fill (buf.begin(), buf.end(), 0.0f); wp = 0; fb = 0.0; }

    // delaySamp = fractional delay (samples); feedback signed by fbSign; dry/wet mix; output gain.
    inline double process (double in, double delaySamp, double feedback, double fbSign,
                           double mix, double gain) noexcept
    {
        if (n < 4) return in * gain;
        delaySamp = juce::jlimit (1.0, (double) (n - 2), delaySamp);
        double rp = (double) wp - delaySamp; if (rp < 0.0) rp += (double) n;
        int i0 = (int) rp; double frac = rp - (double) i0;
        int i1 = i0 + 1; if (i1 >= n) i1 -= n;
        const double delayed = (double) buf[(size_t) i0]
                             + ((double) buf[(size_t) i1] - (double) buf[(size_t) i0]) * frac;
        double x = in + fbSign * feedback * delayed;
        x = juce::jlimit (-4.0, 4.0, x);          // safety (the VAZ fixed-point HW saturates naturally)
        buf[(size_t) wp] = (float) x;
        if (++wp >= n) wp = 0;
        fb = delayed;
        return (in * (1.0 - mix) + delayed * mix) * gain;
    }
};

class VAZFlangerAudioProcessor : public juce::AudioProcessor
{
public:
    VAZFlangerAudioProcessor();
    ~VAZFlangerAudioProcessor() override;

    void prepareToPlay (double sampleRate, int samplesPerBlock) override;
    void releaseResources() override {}
    bool isBusesLayoutSupported (const BusesLayout&) const override;
    void processBlock (juce::AudioBuffer<float>&, juce::MidiBuffer&) override;

    juce::AudioProcessorEditor* createEditor() override;
    bool hasEditor() const override { return true; }
    const juce::String getName() const override { return "VAZFlanger"; }
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
    FlangerChannel chL, chR;
    double sr = 44100.0;
    double lfoPhase = 0.0;
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (VAZFlangerAudioProcessor)
};
