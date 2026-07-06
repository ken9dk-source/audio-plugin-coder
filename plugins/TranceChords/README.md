# TranceChords

**An advanced chord-progression generator for vocal trance.** Pick a key, scale/mode,
song-section and mood, hit **GENERATE**, audition the result with the built-in pad, and
**drag the progression out as a MIDI clip** onto any track in your DAW.

Built from `Avanceret akkordgenerator for vokal trance.pdf` — a deep genre/theory study with
24 worked example progressions, all of which are baked in as curated templates.

JUCE 8 · native C++ UI · instrument (VST3 / Standalone, +AU on macOS) · MIDI in & out.

---

## Highlights
- **Every mode**, including the requested **Dorian** and **Phrygian**: Ionian, Dorian, Phrygian,
  Lydian, Mixolydian, Aeolian, Locrian, **Harmonic Minor**, **Melodic Minor**.
- **Section-aware** generation: *Verse* (low-tension loops), *Pre-Chorus* (builds and ends on the
  dominant), *Chorus* (euphoric, optional key-lift / modulation).
- **Mood** bias: *Dreamy* (maj7/maj9/add9/sus, Lydian colour), *Romantic* (min7/maj7/add9, m(maj7)),
  *Euphoric* (big major, sus→resolution).
- **Hybrid engine**: 24 curated PDF progressions (low *Variation*) ⇄ a free rule engine
  (high *Variation*) that walks section-weighted functional harmony with scale-accurate,
  voice-led chords.
- **Built-in preview pad** (detuned saws + sub → low-pass → ADSR → chorus + reverb) so chords are
  auditionable instantly — click a chord card to hear it, or hit PLAY to loop.
- **MIDI drag-and-drop** to the DAW, plus real-time MIDI output synced to the host transport.
- **Scale-Lock** keeps every generated tone in-key (vocal compatibility); **Lock** individual
  chords you like and re-generate the rest.

## Controls
| Group | Controls |
|-------|----------|
| Header | Key, Mode, Section, Mood, Length (4/8/16 bars), Density (1 or 2 per bar), host BPM |
| Engine | Energy, Complexity (triads→7ths→9ths→11/13), Voice Leading (free↔strict), Variation (template↔rules), Humanize, Octave |
| Toggles | Allow Sus · Allow Borrowed · No Triads · No Pop · Scale Lock · Modulation · Preview Snd |
| Pad / FX | Attack, Release, Cutoff, Detune, Chorus, Reverb, Output |
| Actions | GENERATE · PLAY/STOP · Presets · the **DRAG MIDI → DAW** strip |

Changing Key/Mode/Section/Mood/Length/Density or a toggle re-rolls automatically; the knobs apply on
the next **GENERATE** (Voice-Leading & Octave re-voice instantly without changing the chords).

## Using it
1. Add **TranceChords** as an **instrument**. It generates an initial progression on load.
2. Set Key/Mode/Section/Mood and tweak the engine knobs; press **GENERATE** for new variations.
3. **Click a chord card** to audition it; **PLAY** loops the whole progression; press transport in
   your DAW to hear it follow the host and send MIDI out.
4. Click the small ○ on a card to **lock** it, then GENERATE to keep it and re-roll the rest.
5. **Drag the "DRAG MIDI → DAW" strip** onto a MIDI/instrument track to drop the progression as a
   `.mid` clip.

## Build
```powershell
# Windows (from repo root)
powershell -ExecutionPolicy Bypass -File .\scripts\build-and-install.ps1 -PluginName TranceChords
```
```bash
# macOS
bash scripts/build-and-install.sh TranceChords
```
Installing the VST3 to `C:\Program Files\Common Files\VST3` needs an **elevated** shell; otherwise
run the Standalone from
`build\plugins\TranceChords\TranceChords_artefacts\Release\Standalone\TranceChords.exe`.

## Validate the engine (offline harness)
`TranceChords_Render` generates progressions across modes (incl. Phrygian/Dorian/harmonic minor),
verifies Scale-Lock keeps every tone in-key, exports a MIDI file, and renders the pad to
`trancechords_test.wav`. It prints `RESULT: PASS/FAIL`.

## Layout
```
Source/Theory/   Scales, Chord, Voicing, ProgressionTemplates, Generator   (header-only engine)
Source/Midi/     MidiExport                                                (.mid build + drag-out)
Source/Synth/    PadVoice, PadSynth                                        (preview pad)
Source/          PluginProcessor, PluginEditor, render_main
```

## Deferred (v2 ideas)
Markov/ML backend · text-prompt mood · per-voice stem export (pad/stab) · arpeggiator ·
chord substitution by drag-reorder · user-editable templates.
