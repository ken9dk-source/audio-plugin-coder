# TranceChords — Creative Brief

**Concept:** An advanced **chord-progression generator** for vocal trance. The producer picks a key, scale/mode, song-section and mood, hits **Generate**, and instantly gets a musical, trance-idiomatic chord progression that they can audition with a built-in pad and **drag out as a MIDI clip** onto any DAW track.

**Source:** `Avanceret akkordgenerator for vokal trance.pdf` (deep genre/theory research + 24 worked example progressions).

## What makes it "trance" (from the spec)
- **Emotional scales:** natural minor (Aeolian), major (Ionian), **Lydian** (#4), **harmonic minor** (augmented 2nd drama). Plus the user-requested **Dorian** and **Phrygian**, and the remaining modes for completeness.
- **Extended/colour chords:** sus2/sus4, add9, maj7/maj9, min7/min9, 6, dom7, m(maj7), 11/13, maj7#11; modal interchange (♭VII, ♭VI, iv).
- **Section curves:** Verse = low tension, simple repeating loops; Pre-chorus = building tension (sus, ascending bass, ends on dominant); Chorus = euphoric peak (big major chords, extensions, optional key-change/modulation).
- **Voice-leading:** keep common tones, move other voices by step, use inversions; spread voicing (root low, pad mid, top high); ≤ 4–5 sounding voices to avoid mud.
- **Moods:** Dreamy (maj7/maj9/add9/sus, Lydian), Romantic (min7/maj7/add9, m(maj7)), Euphoric (powerful major, sus→resolution, #11, modulation).

## Headline features (v1)
1. Mode-aware generative engine (rule engine + 24 curated PDF templates, blended by a Variation slider).
2. Built-in **preview pad** (lush trance pad: detuned saws → LP filter → ADSR → chorus + reverb) so chords are auditionable instantly.
3. **MIDI drag-and-drop** of the generated progression to the DAW (native OS drag), plus real-time MIDI output to the host.
4. Per-section templates + mood/energy/complexity/voice-leading/density/variation/humanize controls; chord-type toggles incl. "No-Pop"; scale-lock; optional chorus modulation.
5. Chord cards showing labels (e.g. `Am7`, `Cmaj9`, `Gsus4`) + Roman numerals; click to audition, lock chords you like and re-generate the rest.

## Non-goals (v1)
- No ML/transformer backend (the PDF flags it as impractical offline) — a deterministic, seed-driven rule engine + curated templates instead.
- No audio input analysis; vocal-compatibility is served by **scale-lock** (keeps all generated tones in-key).
- Preview synth is intentionally simple/CPU-light — the product is the MIDI, not the sound design.

**Family:** matches TranceEQ / TranceKick / TranceAcid (APC monorepo, JUCE 8).
