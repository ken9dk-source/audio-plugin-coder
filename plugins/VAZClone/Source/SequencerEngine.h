#pragma once
#include <vector>
#include <juce_data_structures/juce_data_structures.h>

// VAZClone Step Sequencer — DATA MODEL + engine INTERFACE (Phase-B test scaffold; NOT the final engine).
// Data model transcribed from Phase-A RE (vaz_big.c:3600-3800 + DFM) — see sequencer-mixer-status-map.md §6.
// The step-tick TIMING is PLACEHOLDER, isolated in SeqTimingPLACEHOLDER and tagged  AWAITING RUNTIME DUMP .
// The interface + the Phase-B oracles are FINAL; only those 3 timing functions get replaced once
// dump_engine_vmt.py reads TSequencer's clock. Phase C wires note-emit → juce::Synthesiser + the ModBus.

namespace vazseq {

struct SeqStep
{
    int pitch     = 60;   // note 0..127     (.v2p "Pitch N M", step +0x34)
    int flags     = 0;    // step +0x35 bitfield (see Flag)
    int transpose = 0;    // per-step transpose (DFM edTransStep)
    int seqA      = 0;    // SeqControl1 = ModBus lane A (step +0x54)   [ AWAITING DUMP: assume range 0..255 ]
    int seqB      = 0;    // SeqControl2 = ModBus lane B (step +0x55)
    enum Flag { Double = 1, Rest = 2, Slide = 4, Accent = 8 };
    bool isRest()   const { return (flags & Rest)   != 0; }
    bool isSlide()  const { return (flags & Slide)  != 0; }
    bool isAccent() const { return (flags & Accent) != 0; }
    bool isDouble() const { return (flags & Double) != 0; }
};

struct SeqPattern
{
    static constexpr int kMaxSteps = 16;      // Phase-A (0x237-0x34)/0x22 ~ 16   [ AWAITING DUMP: exact 15/16 ]
    SeqStep steps[kMaxSteps];
    int startStep = 0;                        // cbStartStep (loop start)
    int endStep   = kMaxSteps - 1;            // cbEndStep   (loop end)
    int finalStep = kMaxSteps - 1;            // udFinalStep (pattern length)
};

struct SeqSong
{
    static constexpr int kSlots = 34;         // edSongStep1..34
    int  pattern[kSlots] = {};
    int  loopStart = 0, loopEnd = 0;          // "Loop Start Step"
    bool enabled = false;                     // song mode vs single pattern
};

struct SeqState
{
    std::vector<SeqPattern> patterns { SeqPattern{} };
    SeqSong song;
    double tempoBpm    = 120.0;  // msTempo (or host BPM when synced)
    int    timebase    = 2;      // msTimebase index (note division)    [ AWAITING DUMP: division table ]
    float  swing       = 0.0f;   // sbSwing 0..1                        [ AWAITING DUMP: swing curve ]
    float  gateTime    = 0.5f;   // sbGateTime (note-length fraction)
    bool   freeRun     = false;  // btFreeRunning (un-synced)           [ AWAITING DUMP: free-rate law ]
    float  freeRate    = 0.5f;   // free-run rate knob 0..1
    int    accentLevel = 100;    // sbAccentLevel 0..127 (accent velocity boost)
    int    curPattern  = 0;      // active pattern (single-pattern mode)
};

// =============================================================================================
//  AWAITING RUNTIME DUMP  — PLACEHOLDER timing. Real values live in TSequencer's clock/advance
//  callback (get them via dump_engine_vmt.py on the running standalone while a pattern plays).
//  ONLY these functions change later; the SequencerEngine interface + the oracles do NOT.
// =============================================================================================
namespace SeqTimingPLACEHOLDER
{
    inline double stepSeconds (const SeqState& s)
    {
        // Timebase index → beats-per-step. PLACEHOLDER table; real division set TBD.
        static const double divBeats[] = { 4.0, 2.0, 1.0, 0.5, 0.25, 1.0/3.0, 2.0/3.0 }; //  AWAITING DUMP
        const int i = juce::jlimit (0, (int) (sizeof (divBeats) / sizeof (double)) - 1, s.timebase);
        if (s.freeRun) return 1.0 / juce::jmax (0.01, (double) s.freeRate * 20.0);        //  AWAITING DUMP (free law)
        return divBeats[i] * 60.0 / juce::jmax (1.0, s.tempoBpm);
    }
    inline double swingDelaySeconds (const SeqState& s, int stepIndex)
    {
        // odd steps delayed. PLACEHOLDER linear up to 2/3 step; real curve TBD.
        return (stepIndex & 1) ? (double) s.swing * (2.0 / 3.0) * stepSeconds (s) : 0.0; //  AWAITING DUMP (swing)
    }
}

struct NoteEvent { int sampleOffset; int note; int velocity; bool on; };

class SequencerEngine
{
public:
    SeqState state;

    void prepare (double sampleRate) { sr = sampleRate; reset(); }
    void reset ()   { phase = 0.0; step = pat().endStep; playing = false; heldNote = -1; firstTick = false; }
    void start ()   { playing = true; firstTick = true; step = pat().endStep; phase = 0.0; }  // fire start-step first
    void stop ()    { playing = false; }

    // Advance `numSamples`, pushing NoteEvents into `out`. PLACEHOLDER timing ( AWAITING DUMP ).
    void process (int numSamples, std::vector<NoteEvent>& out)
    {
        if (! playing) { if (heldNote >= 0) { out.push_back ({ 0, heldNote, 0, false }); heldNote = -1; } return; }
        const double stepSamp = juce::jmax (1.0, SeqTimingPLACEHOLDER::stepSeconds (state) * sr);
        for (int i = 0; i < numSamples; ++i)
        {
            if (firstTick) { firstTick = false; advanceStep (i, out); continue; }   // emit start-step at t=0
            phase += 1.0;
            if (phase >= stepSamp) { phase -= stepSamp; advanceStep (i, out); }
        }
    }

    // ModBus mod-source values for the CURRENT step, normalised 0..1 (Phase C fills the 3 ModBus cases).
    float modSeqA   () const { return cur().seqA / 255.0f; }   //  AWAITING DUMP: confirm 0..255 range
    float modSeqB   () const { return cur().seqB / 255.0f; }
    float modAccent () const { return cur().isAccent() ? state.accentLevel / 127.0f : 0.0f; }
    int   currentStep () const { return step; }

    // ---- State (de)serialisation for the round-trip oracle + .v2p / getStateInformation ----
    juce::ValueTree toTree () const
    {
        juce::ValueTree t ("SEQ");
        t.setProperty ("tempo", state.tempoBpm, nullptr);   t.setProperty ("timebase", state.timebase, nullptr);
        t.setProperty ("swing", state.swing, nullptr);      t.setProperty ("gate", state.gateTime, nullptr);
        t.setProperty ("freeRun", state.freeRun, nullptr);  t.setProperty ("freeRate", state.freeRate, nullptr);
        t.setProperty ("accent", state.accentLevel, nullptr); t.setProperty ("curPat", state.curPattern, nullptr);
        juce::ValueTree song ("SONG");
        song.setProperty ("loopStart", state.song.loopStart, nullptr); song.setProperty ("loopEnd", state.song.loopEnd, nullptr);
        song.setProperty ("enabled", state.song.enabled, nullptr);
        for (int i = 0; i < SeqSong::kSlots; ++i) song.setProperty ("s" + juce::String (i), state.song.pattern[i], nullptr);
        t.addChild (song, -1, nullptr);
        for (auto& p : state.patterns)
        {
            juce::ValueTree pt ("PAT");
            pt.setProperty ("start", p.startStep, nullptr); pt.setProperty ("end", p.endStep, nullptr); pt.setProperty ("final", p.finalStep, nullptr);
            for (int i = 0; i < SeqPattern::kMaxSteps; ++i)
            {
                juce::ValueTree st ("ST");
                st.setProperty ("p", p.steps[i].pitch, nullptr);     st.setProperty ("f", p.steps[i].flags, nullptr);
                st.setProperty ("t", p.steps[i].transpose, nullptr); st.setProperty ("a", p.steps[i].seqA, nullptr);
                st.setProperty ("b", p.steps[i].seqB, nullptr);
                pt.addChild (st, -1, nullptr);
            }
            t.addChild (pt, -1, nullptr);
        }
        return t;
    }
    void fromTree (const juce::ValueTree& t)
    {
        if (! t.hasType ("SEQ")) return;
        state = SeqState{};
        state.tempoBpm = t.getProperty ("tempo", 120.0);  state.timebase = t.getProperty ("timebase", 2);
        state.swing = t.getProperty ("swing", 0.0f);      state.gateTime = t.getProperty ("gate", 0.5f);
        state.freeRun = t.getProperty ("freeRun", false); state.freeRate = t.getProperty ("freeRate", 0.5f);
        state.accentLevel = t.getProperty ("accent", 100); state.curPattern = t.getProperty ("curPat", 0);
        if (auto song = t.getChildWithName ("SONG"); song.isValid())
        {
            state.song.loopStart = song.getProperty ("loopStart", 0); state.song.loopEnd = song.getProperty ("loopEnd", 0);
            state.song.enabled = song.getProperty ("enabled", false);
            for (int i = 0; i < SeqSong::kSlots; ++i) state.song.pattern[i] = song.getProperty ("s" + juce::String (i), 0);
        }
        state.patterns.clear();
        for (int pi = 0; pi < t.getNumChildren(); ++pi)
        {
            auto pt = t.getChild (pi); if (! pt.hasType ("PAT")) continue;
            SeqPattern p; p.startStep = pt.getProperty ("start", 0); p.endStep = pt.getProperty ("end", 15); p.finalStep = pt.getProperty ("final", 15);
            for (int i = 0; i < SeqPattern::kMaxSteps && i < pt.getNumChildren(); ++i)
            {
                auto st = pt.getChild (i);
                p.steps[i].pitch = st.getProperty ("p", 60); p.steps[i].flags = st.getProperty ("f", 0);
                p.steps[i].transpose = st.getProperty ("t", 0); p.steps[i].seqA = st.getProperty ("a", 0); p.steps[i].seqB = st.getProperty ("b", 0);
            }
            state.patterns.push_back (p);
        }
        if (state.patterns.empty()) state.patterns.push_back (SeqPattern{});
    }

private:
    double sr = 44100.0, phase = 0.0;
    int    step = 0; bool playing = false; int heldNote = -1; bool firstTick = false;

    SeqPattern&       pat ()       { return state.patterns[(size_t) juce::jlimit (0, (int) state.patterns.size() - 1, state.curPattern)]; }
    const SeqPattern& pat () const { return state.patterns[(size_t) juce::jlimit (0, (int) state.patterns.size() - 1, state.curPattern)]; }
    const SeqStep&    cur () const { return pat().steps[(size_t) juce::jlimit (0, SeqPattern::kMaxSteps - 1, step)]; }

    void advanceStep (int sampleOffset, std::vector<NoteEvent>& out)
    {
        const int prev = heldNote;
        step = (step >= pat().endStep) ? pat().startStep : step + 1;   // loop start..end
        const SeqStep& s = cur();
        if (s.isRest()) { if (prev >= 0 && ! s.isSlide()) { out.push_back ({ sampleOffset, prev, 0, false }); heldNote = -1; } return; }
        const int note = juce::jlimit (0, 127, s.pitch + s.transpose);
        const int vel  = s.isAccent() ? juce::jlimit (1, 127, 90 + state.accentLevel / 2) : 90;   //  AWAITING DUMP: accent curve
        if (prev >= 0 && ! s.isSlide()) out.push_back ({ sampleOffset, prev, 0, false });          // non-legato: release prev
        out.push_back ({ sampleOffset, note, vel, true });
        heldNote = note;
    }
};

} // namespace vazseq
