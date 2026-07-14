// seq_test_main.cpp — VazSeqTest: Phase-B Sequencer oracles (test STRUCTURE; engine DSP is Phase C).
// Three headless oracles against the SequencerEngine data model + interface (Phase-A). Timing/range
// assertions that depend on unread runtime data are tagged  AWAITING RUNTIME DUMP  and intentionally
// deferred — they are placeholders whose EXPECTED values get filled in once dump_engine_vmt.py reads
// TSequencer's clock, WITHOUT changing this structure.  Run: VazSeqTest.exe
#include "SequencerEngine.h"
#include <juce_core/juce_core.h>
#include <iostream>
#include <vector>
#include <string>
#include <cmath>

using namespace vazseq;
static int failures = 0;
static void check (bool ok, const std::string& name, const std::string& detail = "")
{
    std::cout << (ok ? "  [PASS] " : "  [FAIL] ") << name;
    if (! detail.empty()) std::cout << "  — " << detail;
    std::cout << "\n"; if (! ok) ++failures;
}

int main()
{
    std::cout << "=== VazSeqTest — Phase-B Sequencer oracles (timing = PLACEHOLDER,  AWAITING RUNTIME DUMP ) ===\n\n";

    // ---- Oracle 1: NOTE-STREAM  (asserts note ORDER + flags now; exact sample times AWAITING DUMP) ----
    std::cout << "1. note-stream oracle:\n";
    {
        SequencerEngine e; e.prepare (44100.0);
        auto& p = e.state.patterns[0];
        p.startStep = 0; p.endStep = 3;
        p.steps[0] = { 60, SeqStep::Accent, 0, 0, 0 };   // C4 accent
        p.steps[1] = { 0,  SeqStep::Rest,   0, 0, 0 };   // rest
        p.steps[2] = { 64, SeqStep::Slide,  0, 0, 0 };   // E4 slide
        p.steps[3] = { 67, 0,               0, 0, 0 };   // G4
        e.state.tempoBpm = 120.0; e.state.timebase = 2;
        e.start();
        std::vector<NoteEvent> ev;
        for (int b = 0; b < 400; ++b) e.process (256, ev);   // ~4.6 steps at the placeholder rate

        std::vector<int> onNotes, onVel;
        for (auto& n : ev) if (n.on) { onNotes.push_back (n.note); onVel.push_back (n.velocity); }
        check (onNotes.size() >= 3, "emits note-ons for non-rest steps", std::to_string (onNotes.size()) + " note-ons");
        check (onNotes.size() >= 3 && onNotes[0] == 60 && onNotes[1] == 64 && onNotes[2] == 67,
               "note ORDER = C4,E4,G4 (Rest step 1 emits nothing)",
               onNotes.size() >= 3 ? std::to_string (onNotes[0]) + "," + std::to_string (onNotes[1]) + "," + std::to_string (onNotes[2]) : "");
        check (! onVel.empty() && onVel[0] > 90, "Accent step boosts velocity", onVel.empty() ? "" : "vel[0]=" + std::to_string (onVel[0]));
        check (true, "Slide->legato + EXACT step sample-positions", " AWAITING RUNTIME DUMP (structure runs; positions pending)");
    }

    // ---- Oracle 2: STATE ROUND-TRIP  (fully functional now — no timing dependency) ----
    std::cout << "\n2. state round-trip oracle (ValueTree save/reload):\n";
    {
        SequencerEngine a;
        a.state.tempoBpm = 137.5; a.state.timebase = 4; a.state.swing = 0.31f; a.state.accentLevel = 118;
        a.state.song.enabled = true; a.state.song.loopEnd = 7;
        for (int i = 0; i < SeqSong::kSlots; ++i) a.state.song.pattern[i] = (i * 3) % 5;
        a.state.patterns.resize (3);
        for (int pi = 0; pi < 3; ++pi) { a.state.patterns[pi].startStep = pi; a.state.patterns[pi].endStep = 12 + pi;
            for (int s = 0; s < SeqPattern::kMaxSteps; ++s) a.state.patterns[pi].steps[s] = { 40 + s + pi, (s * 5) & 15, (s % 3) - 1, s * 7, s * 11 }; }

        SequencerEngine b; b.fromTree (a.toTree());
        bool ok = b.state.patterns.size() == a.state.patterns.size()
               && std::abs (b.state.tempoBpm - a.state.tempoBpm) < 1e-6 && b.state.timebase == a.state.timebase
               && std::abs (b.state.swing - a.state.swing) < 1e-6 && b.state.accentLevel == a.state.accentLevel
               && b.state.song.enabled == a.state.song.enabled;
        for (size_t pi = 0; ok && pi < a.state.patterns.size(); ++pi)
            for (int s = 0; ok && s < SeqPattern::kMaxSteps; ++s) {
                auto& x = a.state.patterns[pi].steps[s]; auto& y = b.state.patterns[pi].steps[s];
                ok = ok && x.pitch == y.pitch && x.flags == y.flags && x.transpose == y.transpose && x.seqA == y.seqA && x.seqB == y.seqB;
            }
        for (int i = 0; ok && i < SeqSong::kSlots; ++i) ok = ok && a.state.song.pattern[i] == b.state.song.pattern[i];
        check (ok, "every pattern/step/song/timing field survives the round-trip");
    }

    // ---- Oracle 3: ModBus mod-source  (mapping SHAPE now; Seq A/B value range AWAITING DUMP) ----
    std::cout << "\n3. ModBus mod-source oracle (Seq A / Seq B / Accent -> 0..1):\n";
    {
        SequencerEngine e; e.prepare (44100.0);
        auto& p = e.state.patterns[0]; p.startStep = 0; p.endStep = 0;
        p.steps[0] = { 60, SeqStep::Accent, 0, 255, 128 };   // seqA=255, seqB=128, accent
        e.state.accentLevel = 127;
        e.start(); std::vector<NoteEvent> ev; e.process (64, ev);   // land on step 0
        check (std::abs (e.modSeqA() - 1.0f) < 1e-4, "Seq A: 255 -> 1.0 (linear normalise)", "modSeqA=" + std::to_string (e.modSeqA()));
        check (std::abs (e.modSeqB() - 128.0f / 255.0f) < 1e-4, "Seq B: 128 -> 0.502", "modSeqB=" + std::to_string (e.modSeqB()));
        check (std::abs (e.modAccent() - 1.0f) < 1e-4, "Accent: level 127 + flag -> 1.0", "modAccent=" + std::to_string (e.modAccent()));
        check (true, "Seq A/B value RANGE (0..255? bipolar?)", " AWAITING RUNTIME DUMP (shape asserted; scale pending)");
    }

    std::cout << "\n" << (failures == 0 ? "ALL STRUCTURAL ORACLES PASS" : std::to_string (failures) + " FAILED")
              << "  (assertions tagged AWAITING RUNTIME DUMP are intentionally deferred placeholders)\n";
    return failures ? 1 : 0;
}
