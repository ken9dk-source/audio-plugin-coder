#pragma once
#include <juce_audio_processors/juce_audio_processors.h>
#include <juce_gui_extra/juce_gui_extra.h>
#include <vector>
#include <cmath>
#include <algorithm>

//==============================================================================
// VAZFlanger — VAZ's REAL flanger, reverse-engineered 2026-06-09 from Core.dll
// **TFXFlanger** (class @0x51FCD4, per-sample DSP @0x5204F4).  It is a textbook
// **delay-line flanger**, NOT a biquad comb (an earlier RE mistakenly decoded
// TFXEqualizer @0x51E0C0 — same coef shape, different module):
//   • triangle LFO  (abs of a 32-bit phase accumulator @0x520531-34, NOT sine)
//   • fractional delay with LINEAR interpolation  (buf[i]+frac·(buf[i+1]-buf[i]) @0x520579)
//   • feedback comb:  buf[w] = in·inGain + feedback·delayed                     (@0x5205A9)
//   • dry/wet mix:    out = in + mix·(delayed − in)                            (@0x5205BD)
//   • per-channel LFO phase offset (L/R Phase) + host-BPM Sync (@0x520493).
// (VAZ's inner loop is fixed-point Q23; we use float — topology-exact, not bit-exact.)
//==============================================================================
struct FlangerChannel
{
    std::vector<float> buf;          // circular delay line (power-of-2)
    int  mask = 0;                   // size − 1
    int  wpos = 0;                   // write index
    void prepare (double sr) noexcept
    {
        int n = 1; const int need = (int) (0.060 * sr) + 4;   // ≤ 60 ms delay headroom
        while (n < need) n <<= 1;
        buf.assign ((size_t) n, 0.0f); mask = n - 1; wpos = 0;
    }
    void reset() noexcept { std::fill (buf.begin(), buf.end(), 0.0f); wpos = 0; }

    // delaySamples = base + LFO·depth (fractional);  feedback (signed);  mix 0..1;  inGain.
    inline double process (double in, double delaySamples, double feedback, double mix, double inGain) noexcept
    {
        double rp = (double) wpos - delaySamples;             // read 'delaySamples' behind the write head
        const double sz = (double) (mask + 1);
        while (rp < 0.0) rp += sz;
        const int    i0   = (int) rp;
        const double frac = rp - (double) i0;
        const double s0   = (double) buf[(size_t) (i0 & mask)];
        const double s1   = (double) buf[(size_t) ((i0 + 1) & mask)];
        const double delayed = s0 + frac * (s1 - s0);         // linear interpolation (real @0x520579-7F)
        double w = in * inGain + feedback * delayed;          // feedback comb (real @0x5205A9)
        w = juce::jlimit (-4.0, 4.0, w);                      // safety (VAZ fixed-point HW saturates)
        buf[(size_t) wpos] = (float) w;
        wpos = (wpos + 1) & mask;
        return in + mix * (delayed - in);                     // dry/wet (real @0x5205BD) = (1−mix)·in + mix·delayed
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
