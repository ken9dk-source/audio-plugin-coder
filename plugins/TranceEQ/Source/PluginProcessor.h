#pragma once
#include <juce_audio_processors/juce_audio_processors.h>
#include <juce_dsp/juce_dsp.h>
#include <array>
#include <vector>
#include <atomic>
#include <memory>
#include "Biquad.h"
#include "PitchDetector.h"
#include "ParameterIDs.hpp"

//==============================================================================
// TranceEQ — genre-aware parametric EQ for Trance Leads / Plucks / Midbasses.
// 6 RBJ-cookbook biquad bands, mode templates, audio+MIDI key-tracking, and an
// FFT spectrum analyser whose magnitudes are pushed to the WebView.
//==============================================================================
struct BandPreset { bool on; teq::FilterType type; float freq, gain, q; bool track; };

class TranceEQAudioProcessor : public juce::AudioProcessor
{
public:
    TranceEQAudioProcessor();
    ~TranceEQAudioProcessor() override;

    void prepareToPlay (double sampleRate, int samplesPerBlock) override;
    void releaseResources() override {}
    bool isBusesLayoutSupported (const BusesLayout&) const override;
    void processBlock (juce::AudioBuffer<float>&, juce::MidiBuffer&) override;

    juce::AudioProcessorEditor* createEditor() override;
    bool hasEditor() const override { return true; }
    const juce::String getName() const override { return "TranceEQ"; }
    bool acceptsMidi() const override { return true; }
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

    // Load a mode's band template into the parameters (called from the UI on mode click).
    void applyModePreset (int modeIndex);
    static const std::array<BandPreset, ParameterIDs::kNumBands>& presetFor (int modeIndex);

    // --- shared with the editor for the analyser ---
    static constexpr int kNumViz = 96;
    std::atomic<float> vizBins[kNumViz];          // spectrum magnitudes (dB), 20 Hz..20 kHz log
    std::atomic<float> currentF0 { 0.0f };        // detected fundamental (Hz), 0 = unvoiced

private:
    void cacheParamPointers();

    double sr = 44100.0;

    teq::Biquad bands[2][ParameterIDs::kNumBands];
    teq::PitchDetector pitch;
    double ktRatioSmoothed = 1.0;
    bool   bandActivePrev[ParameterIDs::kNumBands] {};   // reset biquad state on off→on

    juce::LinearSmoothedValue<float> outGain;            // de-zippered output trim

    // oversampling (Off / 2x / 4x) — runs the biquads at a higher rate to cut HF cramping
    std::unique_ptr<juce::dsp::Oversampling<float>> os2x, os4x;
    int currentOS  = -1;
    int osMaxBlock = 0;

    // latency-compensated, click-free bypass: crossfade the wet path against a dry copy that is
    // delayed by the reported latency, so a bypassed signal is transparent AND stays time-aligned.
    juce::LinearSmoothedValue<float> bypassMix;          // 0 = wet (active), 1 = dry (bypassed)
    int reportedLatency = 0, maxLatency = 0, dryRingSize = 0, dryW = 0;
    std::vector<float> dryRing[2];                       // per-channel dry delay line
    juce::AudioBuffer<float> dryScratch;                 // this block's delayed dry signal

    // cached APVTS raw-value pointers (audio thread)
    std::atomic<float>* pMode = nullptr;  std::atomic<float>* pBypass = nullptr; std::atomic<float>* pOutput = nullptr;
    std::atomic<float>* pKtOn = nullptr;  std::atomic<float>* pKtSrc = nullptr;
    std::atomic<float>* pKtAmt = nullptr; std::atomic<float>* pKtRef = nullptr; std::atomic<float>* pOS = nullptr;
    struct BandPtrs { std::atomic<float>* on=nullptr,*type=nullptr,*freq=nullptr,*gain=nullptr,*q=nullptr,*track=nullptr; };
    BandPtrs bp[ParameterIDs::kNumBands];

    // MIDI key-track
    std::vector<int> heldNotes;
    std::vector<float> monoBuf;   // pre-EQ mono mix fed to the pitch detector

    // FFT spectrum
    static constexpr int kFftOrder = 11;
    static constexpr int kFftSize  = 1 << kFftOrder;   // 2048
    juce::dsp::FFT fft { kFftOrder };
    std::array<float, (size_t) kFftSize>     fifo {};
    std::array<float, (size_t) kFftSize * 2> fftData {};
    std::array<float, (size_t) kFftSize>     hann {};
    int fifoIndex = 0;

    void pushSampleToFifo (float s) noexcept;
    void computeSpectrum() noexcept;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (TranceEQAudioProcessor)
};
