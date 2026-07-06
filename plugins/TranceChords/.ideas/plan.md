# TranceChords — Implementation Plan

**Complexity: 8/10** (no exotic DSP, but a real music-theory engine + native data-rich UI + MIDI drag-out + transport sync).

## Build order (dependency-first)
- **A. Theory core** (header-only, no JUCE audio): `Scales.h` → `Chord.h` → `Voicing.h` → `ProgressionTemplates.h` → `Generator.h`.
- **B. MIDI:** `Midi/MidiExport.h` (Progression → juce::MidiFile + temp file; chord → absolute notes).
- **C. Preview synth:** `Synth/PadVoice.h`, `Synth/PadSynth.h`.
- **D. Processor:** APVTS layout, generate(), state I/O (incl. progression + locks + seed), processBlock (pad render + master chorus/reverb + host MIDI out + transport).
- **E. Editor:** native UI + LookAndFeel + chord cards + drag strip (DragAndDropContainer).
- **F. Harness:** `render_main.cpp` — generate across keys/modes (incl. Phrygian/Dorian), export MIDI, render WAV, PASS/FAIL.
- **G. CMake + build + fix + run harness.**

## Verification gates
1. `TranceChords_Render` builds and prints PASS: progressions are in-scale (scale-lock), MIDI file has expected note count, pad WAV peak > 0.01.
2. VST3 + Standalone build clean (MSVC, exit 0).
3. Manual: load in DAW, Generate, audition, drag MIDI onto a track. (user)

## Risks / mitigations
- **MIDI drag-out from a plugin window** — use native `DragAndDropContainer` + `performExternalDragDropOfFiles` (chosen native UI removes WebView z-order risk).
- **Musical quality** — anchor on the 24 curated PDF progressions; rule engine constrained by function-weights + scale-lock; harness prints labels for eyeball QA.
- **Transport/PPQ alignment** — derive bar length from host BPM + time-sig; loop progression to `length`; guard against host without playhead (standalone → internal Play at 124 BPM).
- **RT safety** — generation only on message thread; audio thread reads a stored progression; no `processBlock` allocation.

## Deferred (v2)
Markov/ML backend; text-prompt mood; per-voice stem export (pad/stab); arpeggiator; MPE; chord substitution drag-reordering; user template editor.
