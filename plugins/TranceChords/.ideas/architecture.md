# TranceChords — Architecture

Native-JUCE **instrument** (IS_SYNTH) that generates trance chord progressions, auditions them with a built-in pad, emits MIDI to the host, and supports **OS drag-and-drop of the progression as a .mid file**.

## File map
```
Source/
  ParameterIDs.h                  APVTS IDs (namespace, inline constexpr)
  Theory/
    Scales.h                      Mode enum + semitone tables; diatonic chord-by-stacked-thirds
    Chord.h                       Chord struct (rootPc, intervals, label, bass, beats) + ChordType factory + note names
    Voicing.h                     voice-leading: octave assignment, common-tone retention, spread, inversions
    ProgressionTemplates.h        24 curated PDF progressions (semitone-from-tonic + ChordType), tagged section/mood
    Generator.h                   GenParams -> Progression. Rule engine (weighted diatonic walk) + template engine, blended by Variation
  Midi/
    MidiExport.h                  Progression -> juce::MidiFile; temp-file writer for drag-out; chord -> absolute MIDI notes
  Synth/
    PadVoice.h                    juce::SynthesiserVoice: 3x PolyBLEP saw (detune) + sub sine -> SVF LP -> ADSR
    PadSynth.h                    juce::Synthesiser wrapper; owns voices + the PadSound
  PluginProcessor.{h,cpp}         APVTS, Generator, PadSynth, master chorus+reverb, MIDI in/out, transport, state, drag file
  PluginEditor.{h,cpp}            Native UI; editor is a juce::DragAndDropContainer; custom LookAndFeel
  render_main.cpp                 offline harness: generate (several keys/modes incl. Phrygian/Dorian), export MIDI, render WAV, PASS/FAIL
```

## Data flow
1. **Generate** (UI button / preset): editor calls `processor.generate()`.
2. Processor builds `GenParams` from APVTS, calls `Generator::generate(params, seed, lockedChords)` → `Progression` (vector of `Chord`, each with `startBeat`/`lengthBeats`, `label`, absolute pitch classes + chosen `ChordType`).
3. Processor stores the `Progression` under a lock, bumps `progressionVersion` (atomic) so the editor repaints the chord cards, and marks the drag `.mid` file dirty.
4. **Audition:** `Voicing` turns each `Chord` into absolute MIDI notes (root in low octave + spread). Preview-Play loops them through `PadSynth`; clicking a card plays one chord.
5. **Host MIDI out:** in `processBlock`, when the host is playing, emit progression note-on/off into the `MidiBuffer` aligned to PPQ (looped to `length`). Also feed those notes to `PadSynth` so the internal pad tracks the host.
6. **Drag-out:** editor's drag strip → `MidiExport::writeTempFile(progression, bpm)` → `performExternalDragDropOfFiles({file}, false)`.

## Generation engine (Generator.h)
- **Diatonic foundation:** from `key`+`mode`, compute the 7 scale pitch-classes and the diatonic chord on each degree by stacking scale thirds (auto-derives maj/min/dim/aug + 7th type; handles harmonic-minor's bIII+/vii°).
- **Function weights by section:**
  - *Verse* — low tension: favour i/I, vi, IV; stepwise; 4-chord loop feel; weak/absent V.
  - *Pre-Chorus* — building: favour ii→V, sus, ascending root; **ends on dominant** (V/Vsus) to set up the chorus.
  - *Chorus* — euphoric: favour I, V, vi, IV with extensions; strong cadences; optional modulation on the last segment.
- **Mood → chord-quality bias:** Dreamy (maj7/maj9/add9/sus2, Lydian colour), Romantic (min7/maj7/add9, occasional m(maj7)), Euphoric (big major, dom on V, sus→resolution, maj7#11 on IV).
- **Complexity:** 0 triads → 7ths → 9ths/add9 → 11/13 on tonic/subdominant.
- **Toggles:** allow_sus, allow_borrowed (♭VII/♭VI/iv; V in minor), forbid_triads, no_pop (reject I–V–vi–IV rotations), scale_lock (coerce tones into scale).
- **Variation slider:** 0 → pick the curated PDF template matching {section,mood} (transposed to key), apply only complexity/voicing; mid → template with substitutions; high → pure rule engine with higher selection temperature. Seed-driven (deterministic per seed); **Generate** advances the seed.
- **Voicing/voice_leading:** after roots+types are chosen, assign octaves to minimise total voice movement (strict) or allow leaps/inversions (free); always root-low + spread, cap sounding voices at 5.
- **Humanize:** applied at emit/export — small timing & velocity jitter, occasional inversion swap.

Generation runs on the **message thread** (button press), never the audio thread → free to use `std::vector`, `juce::String`, `std::mt19937`.

## Real-time safety
- Audio thread only **reads** the stored progression (double-buffered / lock-light) and renders the pad.
- No allocation in `processBlock`; the drag `.mid` is written on the message thread.

## MIDI drag-out (native, reliable)
- `PluginEditor : juce::DragAndDropContainer`.
- A dedicated native "DRAG MIDI → DAW" strip; `mouseDrag` → write temp `.mid` (if dirty) → `performExternalDragDropOfFiles`. No WebView, so no z-order/HWND issues.

## Build/flags
- `IS_SYNTH TRUE`, `NEEDS_MIDI_INPUT TRUE`, `NEEDS_MIDI_OUTPUT TRUE`. `producesMidi()=true`, `acceptsMidi()=true`, `isMidiEffect()=false`.
- VST3 + Standalone (Win), +AU (mac). No WebView flags. Plus a `TranceChords_Render` console harness.
