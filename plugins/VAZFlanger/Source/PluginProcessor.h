#pragma once
#include <juce_audio_processors/juce_audio_processors.h>
#include <juce_gui_extra/juce_gui_extra.h>
#include <vector>
#include <cmath>
#include <algorithm>

//==============================================================================
// VAZFlanger — VAZ's REAL flanger, reverse-engineered 2026-06-09 from Core.dll
// **TFXFlanger** (class @0x51FCD4, block proc FUN_00520418 @0x520418).  It is a
// textbook **delay-line flanger**, NOT a biquad comb (an earlier RE mistakenly
// decoded TFXEqualizer @0x51E0C0 — that 4-section/5-mode biquad is the EQ, now
// built as plugins/VAZEqualizer; re-confirmed via vaz_fx_all.c 2026-06-15):
//   • triangle LFO  (abs of a 32-bit phase accumulator, NOT sine)
//   • fractional delay with LINEAR interpolation  (buf[i]+frac·(buf[i+1]-buf[i]))
//   • feedback comb:  buf[w] = in·inGain + feedback·delayed
//   • Feedback Phase = polarity negation (FUN_005207d0: +0x264 = ±value), NOT a mode change
//   • Delay Time → base delay (FUN_0052076c @0x52076c): samples = (sr·25/256000)·(value+1),
//     value 0..255 → 0.098 .. 25 ms  (continuous: delay_ms = (value+1)/10.24)
//   • dry/wet mix + per-channel LFO phase offset (L/R Phase) + host-BPM Sync.
// (VAZ's inner loop FUN_004c3ad0 is fixed-point + outside the FX dump → rate/depth/feedback
//  *magnitude* scaling is not extractable; we match topology + the confirmed delay range.)
//==============================================================================
struct FlangerChannel
{
    std::vector<float> buf;          // circular delay line (power-of-2)
    int  mask = 0;                   // size − 1
    int  wpos = 0;                   // write index
    int  length() const noexcept { return mask + 1; }
    void prepare (double sr) noexcept
    {
        int n = 1; const int need = (int) (0.090 * sr) + 4;   // 90 ms headroom (base 25 ms + sweep)
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
