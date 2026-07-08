#pragma once
//==============================================================================
// MidBass — dedicated trance mid-bass synthesizer.
// Phase 3: first audible path — mono voice (stack) → hybrid filter → amp VCA,
// mono note stack with legato fallback to the previously held note.
//==============================================================================
#include <juce_audio_processors/juce_audio_processors.h>
#include "ParameterIDs.hpp"
#include "Params.h"
#include "MbVoice.h"
#include "MbTone.h"
#include "MbFx.h"
#include "MbMacros.h"
#include "MbPresets.h"

class MidBassAudioProcessor : public juce::AudioProcessor
{
public:
    MidBassAudioProcessor();
    ~MidBassAudioProcessor() override = default;

    void prepareToPlay (double sampleRate, int samplesPerBlock) override;
    void releaseResources() override {}
    bool isBusesLayoutSupported (const BusesLayout& layouts) const override;
    void processBlock (juce::AudioBuffer<float>&, juce::MidiBuffer&) override;

    juce::AudioProcessorEditor* createEditor() override;
    bool hasEditor() const override;

    const juce::String getName() const override { return "MidBass"; }
    bool acceptsMidi() const override           { return true; }
    bool producesMidi() const override          { return false; }
    bool isMidiEffect() const override          { return false; }
    double getTailLengthSeconds() const override { return 0.0; }

    // Factory presets as JUCE programs (Phase 7): hosts and the Phase 8 quick
    // buttons share this one entry point.
    int getNumPrograms() override                              { return mb::kNumFactoryPresets; }
    int getCurrentProgram() override                           { return curProgram; }
    void setCurrentProgram (int index) override;
    const juce::String getProgramName (int index) override
    {
        return juce::isPositiveAndBelow (index, mb::kNumFactoryPresets)
             ? juce::String (mb::kFactoryPresets[index].name) : juce::String();
    }
    void changeProgramName (int, const juce::String&) override {}

    void getStateInformation (juce::MemoryBlock& destData) override;
    void setStateInformation (const void* data, int sizeInBytes) override;

    juce::AudioProcessorValueTreeState apvts;
    juce::MidiKeyboardState keyboardState;   // fed from processBlock, read by the GUI keyboard strip

    // Analyzer capture (Phase 8b): single-writer lock-free ring. The audio
    // thread does ONE float store per sample + ONE relaxed atomic publish per
    // block — no locks, no allocations (condition c). The editor's timer pulls
    // and does windowing/FFT entirely on the message thread.
    static constexpr int kVizSize = 2048;    // power of two
    float vizRing[kVizSize] = {};
    std::atomic<int> vizWritePos { 0 };
    bool vizTapEnabled = true;               // test-only A/B for the FIFO cost measurement

    // ---- test hooks ----
    int    currentNoteForTest() const  { return voice.curNote; }
    bool   voiceActiveForTest() const  { return voice.isActive(); }
    double lfo1HzForTest() const       { return voice.lfo1.curHz; }
    const mb::MbVoice& voiceForTest() const         { return voice; }
    const mb::MbSaturator& satForTest() const       { return sat; }
    const mb::MbTransient& transForTest() const     { return trans; }
    mb::MbFxChain& fxChainForTest()                 { return fx; }

private:
    void handleMidiEvent (const juce::MidiMessage& m);
    void updateVoiceParams (int blockSamples);

    mb::MbVoice voice;
    mb::MbSaturator sat;                     // post-filter, 2x OS, 47-sample latency (reported)
    mb::MbToneEq eq;
    mb::MbTransient trans;
    mb::MbFxChain fx;                        // Chorus→Phaser→Flanger→Delay→Reverb→Comp
    juce::Array<int> heldNotes;              // mono note stack (last-note priority)
    double curBpm = 120.0;                   // last seen host tempo
    float masterGain = 1.0f;                 // OUTPUT param, applied POST-chain
    int curProgram = 0;
    float macroSmooth[mb::Macro::Count] = {};  // ~45 ms macro slew (zipper standard)
    float macroSatDrive = 0, macroSatMix = 0, macroTransAtk = 0, macroChoMix = 0;
    float macroEq[3] = {};

    std::atomic<float>* pRaw (const char* id) { return apvts.getRawParameterValue (id); }

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (MidBassAudioProcessor)
};
