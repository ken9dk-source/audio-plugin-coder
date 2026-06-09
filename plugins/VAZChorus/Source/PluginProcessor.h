#pragma once
#include <juce_audio_processors/juce_audio_processors.h>
#include <juce_gui_extra/juce_gui_extra.h>
#include <vector>
#include <cmath>
#include <algorithm>

//==============================================================================
// VAZChorus — VAZ's real Chorus, reverse-engineered 2026-06-09 from Core.dll
// **TFXChorus** (class @0x5184D4, per-sample DSP @0x518AD8): a **dual-LFO ENSEMBLE chorus**.
// One delay line is read by TWO LFOs, each tapping THREE voices at 0° / ±120°
// (`esi ± 0x55555554` @0x518BCD), linear-interpolated and summed → a 6-tap ensemble.
// Waveform per the real's mode switch [+0x264]/[+0x270]:  0 sine · 1 triangle · 2 trapezoid.
// (VAZ's inner loop is fixed-point; we use float — topology-exact, not bit-exact.)
//==============================================================================
struct ChorusChannel
{
    std::vector<float> buf;          // circular delay line (power-of-2)
    int  mask = 0;                   // size − 1
    int  wpos = 0;                   // write index
    void prepare (double sr) noexcept
    {
        int n = 1; const int need = (int) (0.050 * sr) + 4;   // ≤ 50 ms delay headroom
        while (n < need) n <<= 1;
        buf.assign ((size_t) n, 0.0f); mask = n - 1; wpos = 0;
    }
    void reset() noexcept { std::fill (buf.begin(), buf.end(), 0.0f); wpos = 0; }

    inline double readInterp (double delaySamples) const noexcept
    {
        double rp = (double) wpos - delaySamples;             // read 'delaySamples' behind the write head
        const double sz = (double) (mask + 1);
        while (rp < 0.0) rp += sz;
        const int    i0   = (int) rp;
        const double frac = rp - (double) i0;
        const double s0   = (double) buf[(size_t) (i0 & mask)];
        const double s1   = (double) buf[(size_t) ((i0 + 1) & mask)];
        return s0 + frac * (s1 - s0);                         // linear interpolation (real @0x518B41-47)
    }

    // unipolar LFO shape 0..1 (real: |phase|-based, so the delay always sweeps up from the base).
    static inline double waveshape (double p, int waveform) noexcept
    {
        p -= std::floor (p);                                  // wrap to [0,1)
        if (waveform == 0) return 0.5 - 0.5 * std::cos (2.0 * juce::MathConstants<double>::pi * p);   // sine
        const double tri = (p < 0.5) ? 2.0 * p : 2.0 - 2.0 * p;                                       // triangle
        if (waveform == 1) return tri;
        return juce::jlimit (0.0, 1.0, (tri - 0.2) * 1.6);     // trapezoid (clamped triangle, real mode 1)
    }

    // 2 LFOs (ph1, ph2) × 3 taps at 0°/±120° → 6-tap modulated-delay ensemble, summed.
    inline double process (double in, double baseSamp, double depthSamp,
                           double ph1, double ph2, int waveform, double mix, double gain) noexcept
    {
        buf[(size_t) wpos] = (float) in;                      // write input into the delay line
        const double phases[2] = { ph1, ph2 };
        const double third = 1.0 / 3.0;
        double sum = 0.0;
        for (int v = 0; v < 2; ++v)
            for (int k = 0; k < 3; ++k)                        // taps at 0°, +120°, +240°
            {
                const double m = waveshape (phases[v] + (double) k * third, waveform);
                sum += readInterp (baseSamp + m * depthSamp);
            }
        const double ensemble = sum * 0.40824829;             // sum / √6 → unity-level incoherent 6-voice mix
        wpos = (wpos + 1) & mask;
        return (in * (1.0 - mix) + ensemble * mix) * gain;    // dry/wet + output gain
    }
};

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
    ChorusChannel chL, chR;
    double sr = 44100.0;
    double lfo1Phase = 0.0, lfo2Phase = 0.0;   // two LFOs (slightly different rates → ensemble beating)
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (VAZChorusAudioProcessor)
};
