#pragma once
#include <juce_audio_processors/juce_audio_processors.h>
#include <juce_dsp/juce_dsp.h>
#include <atomic>
#include <memory>
#include "ParameterIDs.h"
#include "Theory/Generator.h"
#include "Theory/MelodyFit.h"
#include "Midi/MidiExport.h"
#include "Layers/Bassline.h"
#include "Layers/Arpeggiator.h"
#include "Layers/CounterMelody.h"
#include "Synth/PadSynth.h"
#include "Synth/SimpleSynth.h"
#include "Synth/Pump.h"

//==============================================================================
// TranceChords — vocal-trance chord-progression generator (instrument).
// Generates a Progression on the message thread, auditions it with a built-in
// pad, emits it as MIDI to the host (PPQ-synced, looped), and exports a temp
// .mid for OS drag-and-drop. Native editor.
//==============================================================================
namespace tc
{
    struct ScheduledNote { double beat; bool on; int note; int vel; };

    // immutable snapshot handed lock-free to the audio thread. One event list per
    // performance layer (all sorted by beat); chords[] is kept for click-audition.
    struct Schedule
    {
        std::vector<ScheduledNote> chordEvents;
        std::vector<ScheduledNote> bassEvents;
        std::vector<ScheduledNote> arpEvents;
        std::vector<ScheduledNote> counterEvents;
        std::vector<VoicedChord>   chords;
        double totalBeats = 0.0;
    };
}

class TranceChordsAudioProcessor : public juce::AudioProcessor
{
public:
    TranceChordsAudioProcessor();
    ~TranceChordsAudioProcessor() override = default;

    void prepareToPlay (double sampleRate, int samplesPerBlock) override;
    void releaseResources() override {}
    bool isBusesLayoutSupported (const BusesLayout&) const override;
    void processBlock (juce::AudioBuffer<float>&, juce::MidiBuffer&) override;

    juce::AudioProcessorEditor* createEditor() override;
    bool hasEditor() const override { return true; }
    const juce::String getName() const override { return "TranceChords"; }
    bool acceptsMidi() const override { return true; }
    bool producesMidi() const override { return true; }
    bool isMidiEffect() const override { return false; }
    double getTailLengthSeconds() const override { return 4.0; }
    int getNumPrograms() override { return 1; }
    int getCurrentProgram() override { return 0; }
    void setCurrentProgram (int) override {}
    const juce::String getProgramName (int) override { return {}; }
    void changeProgramName (int, const juce::String&) override {}
    void getStateInformation (juce::MemoryBlock&) override;
    void setStateInformation (const void*, int) override;

    juce::AudioProcessorValueTreeState apvts;
    static juce::AudioProcessorValueTreeState::ParameterLayout createParameterLayout();

    //== UI-facing API (message thread) ==
    void generate();                          // re-roll the progression from current params
    void revoice();                           // keep chords, rebuild voicing/schedule only
    void applyPreset (int index);
    static constexpr int kNumPresets = 16;
    void auditionChord (int index);           // one-shot a single chord through the pad
    void setPreviewPlaying (bool shouldPlay);

    // history (undo/redo over generated progressions)
    void undo();
    void redo();
    bool canUndo() const { return historyPos > 0; }
    bool canRedo() const { return historyPos >= 0 && historyPos < (int) history.size() - 1; }

    // favorite progressions (persisted in plugin state)
    void saveFavorite();
    void recallFavorite (int index);
    int  numFavorites() const { return (int) favorites.size(); }
    juce::String favoriteLabel (int index) const;
    bool isPreviewPlaying() const { return previewPlaying.load(); }
    void toggleLock (int index);

    // snapshot of the current progression for drawing (message thread only)
    const tc::Progression& progression() const { return currentProg; }
    int  progressionVersion() const { return progVersion.load(); }
    double currentBpm() const { return bpmForUi.load(); }
    int  playheadChordIndex() const;          // which chord is sounding (for the UI playhead), -1 if none
    double playheadBeat() const { return playBeatForUi.load(); }   // continuous playhead beat, -1 if stopped
    double scheduleTotalBeats() const;        // length of the current progression in beats
    std::vector<int> chordNotesAt (int index) const;               // voiced MIDI notes of a chord
    int  activeMelodyNote() const;            // melody MIDI note under the playhead, -1 if none

    // write a layer of the current progression to a temp .mid (0=chords,1=bass,2=arp,3=counter)
    juce::File exportTempMidi (int layer);

    // melody-aware: load a melody (e.g. from a dropped .mid) and re-fit chords
    void setMelody (const tc::NoteLayer& m);
    void clearMelody();
    int  melodyNoteCount() const { return melodyCount.load(); }

private:
    tc::GenParams collectGenParams() const;
    tc::VoicingConfig collectVoicing() const;
    tc::BassParams collectBassParams() const;
    tc::ArpParams  collectArpParams() const;
    tc::CounterParams collectCounterParams() const;
    std::shared_ptr<const tc::Schedule> buildSchedule (const tc::Progression&) const;
    tc::Progression buildSong (const tc::GenParams& base, uint32_t seed) const;
    double hostOrDefaultBpm() const;
    static void emitRange (const std::vector<tc::ScheduledNote>& events, double totalBeats,
                           double beatStart, double blockBeats, int numSamples,
                           juce::MidiBuffer& padMidi, juce::MidiBuffer* hostOut);

    double sr = 44100.0;

    tc::PadSynth padSynth;
    tc::SimpleSynth bassSynth { 4 };
    tc::SimpleSynth pluckSynth { 12 };
    tc::SimpleSynth counterSynth { 8 };
    juce::dsp::Chorus<float> chorus;
    juce::Reverb reverb;
    juce::AudioBuffer<float> scratchPad, scratchPluck, scratchBass, scratchCounter;  // pre-allocated mix buses

    // generated progression (message thread) + lock-free schedule for audio thread
    tc::Progression currentProg;
    tc::NoteLayer    melody;                 // loaded melody for melody-aware fitting (message thread)
    std::atomic<int> melodyCount { 0 };

    std::vector<tc::Progression> history;    // undo/redo stack
    int historyPos = -1;
    std::vector<tc::Progression> favorites;
    void pushHistory();
    static constexpr int kMaxHistory = 60;
    std::atomic<std::shared_ptr<const tc::Schedule>> schedule;  // lock-free swap (C++20)
    std::atomic<int> progVersion { 0 };
    uint32_t seedCounter = 0x1F2E3D;

    // transport / playback state (audio thread)
    std::atomic<bool> previewPlaying { false };
    std::atomic<bool> previewResetPending { false };
    double previewBeat = 0.0;
    double pumpPhase = 0.0;   // 0..1 within a beat, for the sidechain pump
    std::atomic<int>  auditionReq { -1 };
    std::vector<int>  auditionNotes;
    int auditionSamplesLeft = 0;
    std::atomic<int>  curChordForUi { -1 };
    std::atomic<double> bpmForUi { 124.0 };
    std::atomic<double> playBeatForUi { -1.0 };

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (TranceChordsAudioProcessor)
};
