#pragma once

#include <juce_audio_processors/juce_audio_processors.h>
#include <juce_dsp/juce_dsp.h>
#include <juce_audio_formats/juce_audio_formats.h>
#include <vector>
#include <utility>
#include <functional>
#include "SynthVoice.h"

//==============================================================================
// VAZClone — faithful VAZ 2010 virtual-analog clone.
// Phase 4.0: project shell (APVTS + WebView GUI). Voice DSP (oscillators,
// ladder filter, envelopes) lands in DSP phases A-G — see .ideas/plan.md.
//==============================================================================
class VAZCloneAudioProcessor : public juce::AudioProcessor
{
public:
    VAZCloneAudioProcessor();
    ~VAZCloneAudioProcessor() override;

    //==============================================================================
    void prepareToPlay (double sampleRate, int samplesPerBlock) override;
    void releaseResources() override;
    bool isBusesLayoutSupported (const BusesLayout& layouts) const override;
    void processBlock (juce::AudioBuffer<float>&, juce::MidiBuffer&) override;

    //==============================================================================
    juce::AudioProcessorEditor* createEditor() override;
    bool hasEditor() const override { return true; }

    //==============================================================================
    const juce::String getName() const override { return "VAZClone"; }
    bool acceptsMidi() const override { return true; }
    bool producesMidi() const override { return false; }
    bool isMidiEffect() const override { return false; }
    double getTailLengthSeconds() const override { return 0.0; }

    //==============================================================================
    int getNumPrograms() override { return 1; }
    int getCurrentProgram() override { return 0; }
    void setCurrentProgram (int) override {}
    const juce::String getProgramName (int) override { return {}; }
    void changeProgramName (int, const juce::String&) override {}

    //==============================================================================
    void getStateInformation (juce::MemoryBlock& destData) override;
    void setStateInformation (const void* data, int sizeInBytes) override;

    //==============================================================================
    // Public state
    juce::AudioProcessorValueTreeState apvts;

    // .v2p patch load/save — called from the WebView menu (Load Patch / Save Patch)
    void loadPatchDialog();
    void savePatchDialog();
    void loadSampleDialog (int osc, std::function<void (juce::String)> onDone);   // Sample-osc loader (OSC 0/1)
    void resetSample (int osc);                                                   // back to default Sine
    bool loadV2P (const juce::MemoryBlock& mb);
    juce::MemoryBlock buildV2P();
    juce::MemoryBlock lastPatchBytes;                 // template for Save (last loaded patch bytes)
    std::unique_ptr<juce::FileChooser> patchChooser;  // kept alive during async dialog
    std::unique_ptr<juce::FileChooser> sampleChooser; // kept alive during async sample dialog
    juce::AudioFormatManager sampleFormatMgr;         // WAV/AIFF/FLAC reading for the sample osc
    SampleData osc1SampleData, osc2SampleData;        // loaded samples (Sample waveform), audio-thread read
    static juce::AudioProcessorValueTreeState::ParameterLayout createParameterLayout();

private:
    void refreshVoiceParams();

    double currentSampleRate = 44100.0;
    bool   envPatchTried = false;   // one-shot guard for the VAZCLONE_LOAD_PATCH A/B hook

    VoiceParams voiceParams;          // declared before synth → outlives the voices that ref it
    VAZSynth synth;
    static constexpr int kNumVoices = 32;   // matches VAZ's 32-voice pool (Unison up to 32)

    // VAZ multimode filter: 6 engines (A/B/C/D/K/R) + Comb, topologies reverse-engineered
    // from Core.dll. Dispatched by the 22-entry mode dropdown. No cutoff smoothing → instant env.
    VAZMultiFilter ladder;
    juce::SmoothedValue<float> smoothCutoff, smoothRes;

    // Type R/C/K have a 1-pole highpass AFTER the cascade (the "Highpass Cutoff" slider).
    float hpX1[2] { 0.0f, 0.0f }, hpY1[2] { 0.0f, 0.0f };

    // Global 2× oversampling (VAZ "Oversample x2"): the whole voice render runs at 2× SR then downsamples.
    juce::dsp::Oversampling<float> oversampler { 2, 1, juce::dsp::Oversampling<float>::filterHalfBandPolyphaseIIR };
    double     baseSampleRate = 44100.0;     // the host SR (oversampled rate = base × factor)
    bool       osActive = false;             // current oversample state (re-prepare the synth SR when it flips)

    // Phase E: filter modulation — Env2 -> cutoff (sweep) + LFO1 -> cutoff (movement).
    ModLFO     modLfo, modLfo2, modLfo3;   // LFO1/2/3 (mod sources)
    VAZEnv     filterEnv;                    // Env2 (with Reset/Cycle/Curve modes)
    double     lagState = 0.0;              // Lag Processor slew state
    int        activeNotes = 0;

    // Modulation-source bus state (sources usable in the filter mod matrix).
    float modWheel     = 0.0f;   // MIDI Control A (CC1)
    float randomVal    = 0.0f;   // re-rolled per note-on
    float keyTrack     = 0.0f;   // last note pitch, 0..1 (Oscillator 1 Pitch source)
    float lastVelocity = 0.0f;   // MIDI Velocity source
    float aftertouch   = 0.0f;   // MIDI Pressure source
    float midiCtrlB    = 0.0f;   // MIDI Control B (CC11 / Expression)
    juce::Random modRng;

    // Per-block mod-source bus buffers (filled before voices render; read by voices+filter).
    std::vector<float> lfo1Buf, lfo2Buf, lfo3Buf, env2Buf, ma1Buf, ma2Buf, lagBuf, noiseModBuf;
    ModBus modBus;

    // Mono-mode last-note-priority state.
    int monoNote = -1;
    int monoStartNote = -1;   // note the dedicated MONO voice was started with (legato keeps it → matches the note-off)
    std::vector<std::pair<int, int>> heldNotes;   // (note, velocity) stack
    // Arpeggiator
    std::vector<int> arpHeld;                     // notes currently arpeggiated
    int    arpStep = 0, arpSounding = -1;
    double arpClock = 0.0;
    int    arpVel = 100;
    void   processArp (const juce::MidiBuffer& in, juce::MidiBuffer& out, int numSamples,
                       double bpm, bool hold, int mode, int rate, int octs);

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (VAZCloneAudioProcessor)
};
