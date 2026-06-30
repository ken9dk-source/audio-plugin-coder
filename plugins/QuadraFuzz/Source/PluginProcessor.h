#pragma once
#include <juce_audio_processors/juce_audio_processors.h>
#include <juce_dsp/juce_dsp.h>
#include "ButterworthCrossover.h"
#include <array>
#include <atomic>
#include <vector>

//==============================================================================
// QuadraFuzz — reverse-engineered 1:1 from the original QuadraFuzz.dll
// (Houpert Digital Audio, uniqueID 'QFSX').
//
// VERIFIED parameter set (8 params, exact names/order/ranges from the DLL):
//   0-3  Band1..Band4   -12..+12 dB   (the 4 fuzz-drive knobs)
//   4    In             -20..+20 dB
//   5    Out            -20..+20 dB
//   6    Shape          choice 0..4   (5 waveshapers)
//   7    Preset         0..1
//
// The big EQ window is a SEPARATE filterbank editor, INDEPENDENT of the knobs:
//   - 4 diamond handles  = per-band level   (internal, not automatable)
//   - triangle handles   = crossover freqs  (internal, not automatable)
// These are stored as plain atomics, edited by the editor, saved in state —
// they are NOT linked to the Band1..Band4 knob parameters.
//==============================================================================
class QuadraFuzzAudioProcessor : public juce::AudioProcessor
{
public:
    QuadraFuzzAudioProcessor();
    ~QuadraFuzzAudioProcessor() override = default;

    void prepareToPlay (double sampleRate, int samplesPerBlock) override;
    void releaseResources() override {}
    bool isBusesLayoutSupported (const BusesLayout&) const override;
    void processBlock (juce::AudioBuffer<float>&, juce::MidiBuffer&) override;

    juce::AudioProcessorEditor* createEditor() override;
    bool hasEditor() const override { return true; }
    const juce::String getName() const override { return "QuadraFuzz"; }
    bool acceptsMidi() const override { return false; }
    bool producesMidi() const override { return false; }
    bool isMidiEffect() const override { return false; }
    double getTailLengthSeconds() const override { return 0.0; }
    int getNumPrograms() override { return 1; }
    int getCurrentProgram() override { return 0; }
    void setCurrentProgram (int) override {}
    const juce::String getProgramName (int) override { return {}; }
    void changeProgramName (int, const juce::String&) override {}
    void getStateInformation (juce::MemoryBlock& dest) override;
    void setStateInformation (const void* data, int size) override;

    juce::AudioProcessorValueTreeState apvts;
    static juce::AudioProcessorValueTreeState::ParameterLayout createParameterLayout();

    //==========================================================================
    // PRESET BANK — same model as the original QuadraFuzz: a runtime bank of up
    // to 64 presets saved to disk, with Create/Delete. On first run the bank is
    // seeded from the factory snapshot (recovered from the original's chunk);
    // after that the on-disk bank is authoritative and fully user-editable.
    struct QFPreset
    {
        juce::String name;
        float band[4]  { 0.f, 0.f, 0.f, 0.f };   // Band1..Band4 knob drive (dB)
        float in   = 0.f;                         // In  (dB)
        float out  = 0.f;                         // Out (dB)
        int   shape = 0;                          // waveshaper 0..4
        float level[4] { 0.f, 0.f, 0.f, 0.f };    // EQ band LEVELS / diamonds (dB)
        float cross[3] { 127.f, 677.f, 3338.f };  // crossover frequencies (Hz)
        float edgeLo = 25.f;                       // band-0 low edge  (f4, Hz)
        float edgeHi = 16000.f;                    // band-3 high edge (f8, Hz)
    };

    static constexpr int MAX_PRESETS = 64;        // original's limit (NrOfPreset 1..64)

    int          getNumPresets()    const { return (int) presetBank.size(); }
    int          getCurrentPreset() const { return currentPreset; }
    juce::String getPresetName (int idx) const;
    void         applyPreset   (int presetIndex);
    int          createPreset  (const juce::String& name);  // capture current state; -1 if full
    void         deletePreset  (int presetIndex);
    void         loadPresetBank();
    void         savePresetBank() const;

    //==========================================================================
    // INTERNAL filterbank state (the EQ window) — NOT VST parameters.
    // Edited by the editor, read by the audio thread. Independent of the knobs.
    static constexpr int NUM_BANDS  = 4;
    static constexpr int NUM_CROSS  = 3;   // 3 internal crossovers split 4 bands

    // Default to the neutral "Default" (flat EQ); the bank is loaded in the constructor.
    std::atomic<float> bandLevelDb[NUM_BANDS] { {0.f}, {0.f}, {0.f}, {0.f} };
    std::atomic<float> crossHz[NUM_CROSS]     { {136.2408f}, {742.4620f}, {4046.1445f} };

    // Outer band edges (the 2 outer draggable triangles): low edge of band 0 and
    // high edge of band 3. band0 = bandpass [edgeLo .. cross0], band3 = bandpass
    // [cross2 .. edgeHi]. The clean capture showed band 0 rolling off ~25 Hz, so
    // these are real filters, not just markers.
    std::atomic<float> edgeLoHz { 25.f };
    std::atomic<float> edgeHiHz { 22050.f };

    // OVER lamp — 1:1 with the original (QuadraFuzz.dll FUN_1000a320): set to 1.0
    // whenever any band's waveshaper INPUT exceeds +/-1.0 (x87 FCOM vs double 1.0).
    // It LATCHES; the editor reads-and-clears it each GUI tick (like FUN_1000a110).
    std::atomic<float> overFlag { 0.0f };
    bool readAndClearOver() noexcept { return overFlag.exchange (0.0f) > 0.0f; }

    // Default crossovers = the real DLL's measured power-on crossover frequencies
    // (clean per-band noise capture; LR4 fit). Old guess was {125,800,5000}.
    static constexpr float DEFAULT_CROSS[NUM_CROSS] = { 136.2408f, 742.4620f, 4046.1445f };
    static constexpr float DEFAULT_EDGE_LO = 25.f;
    static constexpr float DEFAULT_EDGE_HI = 22050.f;

private:
    // Preset bank (loaded from disk in the constructor; seeded on first run).
    std::vector<QFPreset> presetBank;
    int                   currentPreset = 0;
    void       captureCurrent (QFPreset& p) const;  // snapshot the live params + EQ
    juce::File getBankFile() const;                 // %APPDATA%/QuadraFuzz/QuadraFuzz.presets

    // Independent per-band 4th-order BUTTERWORTH BANDPASS — the DLL's real
    // realization (FUN_1000b4b0 case 3: LP->BP polynomial transform of a 4th-order
    // Butterworth LP prototype, NOT a HP.LP cascade). Each band b is one true BP
    // filter designed from (loEdge, hiEdge); edges band0=[edgeLo,cross0] ..
    // band3=[cross2,edgeHi]. This is phase-identical to the original (validated
    // bit-exact vs scipy.butter(4,band); HP.LP only matched magnitude, not phase).
    QFX::BW_BP bandBP[NUM_BANDS];

    juce::AudioBuffer<float> dryBuf;
    juce::AudioBuffer<float> band0Buf, above0Buf;
    juce::AudioBuffer<float> band1Buf, above1Buf;
    juce::AudioBuffer<float> band2Buf, band3Buf;

    double sr = 44100.0;
    // 5 band edges: { edgeLo, cross0, cross1, cross2, edgeHi }. Each BP band needs
    // both of its edges, so any change re-designs the affected bands together.
    float  lastEdge[NUM_BANDS + 1] = { -1.f, -1.f, -1.f, -1.f, -1.f };

    void  updateFilters (float lo, float c0, float c1, float c2, float hi);
    void  runCrossover (const juce::AudioBuffer<float>& src);
    static float waveshape (float x, int shape) noexcept;
    void  applyBandFuzz (juce::AudioBuffer<float>& buf, float driveDb, float levelDb, int shape);

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (QuadraFuzzAudioProcessor)
};
