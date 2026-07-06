# TranceChords — Parameter Spec (APVTS)

All IDs live in `Source/ParameterIDs.h`. Ranges/defaults are authoritative here and mirrored in `createParameterLayout()`.

## Generation controls (drive the chord engine)
| ID | Type | Range / Choices | Default | Notes |
|----|------|-----------------|---------|-------|
| `key` | choice | C, C#, D, D#, E, F, F#, G, G#, A, A#, B | C | Tonic pitch-class |
| `mode` | choice | Ionian, Dorian, Phrygian, Lydian, Mixolydian, Aeolian, Locrian, Harmonic Minor, Melodic Minor | Aeolian | Scale/mode |
| `section` | choice | Verse, Pre-Chorus, Chorus | Verse | Tension curve template |
| `mood` | choice | Dreamy, Romantic, Euphoric | Dreamy | Chord-colour bias |
| `length` | choice | 4, 8, 16 (bars) | 8 | Progression length |
| `density` | choice | 1/bar, 2/bar (½-bar) | 1/bar | Chords per bar |
| `energy` | float | 0..100 % | 40 | Dominant tension / resolution strength |
| `complexity` | float | 0..100 % | 45 | Triads → 7ths → 9ths → 11/13 |
| `voice_leading` | float | 0..100 % | 70 | 0 = free/leaps, 100 = strict common-tone |
| `variation` | float | 0..100 % | 35 | 0 = deterministic (curated template), 100 = free rule engine + high temperature |
| `humanize` | float | 0..100 % | 20 | Timing/velocity jitter + occasional inversion |
| `octave` | int | -2..+2 | 0 | Output octave shift |

## Chord-type toggles
| ID | Type | Default | Notes |
|----|------|---------|-------|
| `allow_sus` | bool | true | Permit sus2/sus4 (and 7sus4) substitutions/passing |
| `allow_borrowed` | bool | true | Modal interchange (♭VII, ♭VI, iv; V in minor) |
| `forbid_triads` | bool | false | Force every chord to ≥ 7th |
| `no_pop` | bool | true | Reject literal I–V–vi–IV and its rotations |
| `scale_lock` | bool | true | Coerce all tones into the selected scale (vocal compatibility) |
| `modulation` | bool | false | Allow +1/+2 semitone lift in the final segment (esp. Chorus) |

## Preview pad (built-in audition synth)
| ID | Type | Range | Default | Notes |
|----|------|-------|---------|-------|
| `prev_enable` | bool | — | true | Internal pad on/off |
| `prev_attack` | float | 1..2000 ms | 12 | Pad amp attack |
| `prev_release` | float | 20..4000 ms | 600 | Pad amp release |
| `prev_cutoff` | float | 200..16000 Hz (log) | 3500 | Pad LP cutoff |
| `prev_detune` | float | 0..100 % | 30 | Supersaw detune spread |
| `prev_reverb` | float | 0..100 % | 35 | Master reverb mix |
| `prev_chorus` | float | 0..100 % | 40 | Master chorus mix |
| `output` | float | -24..+6 dB | -3 | Master output trim |

## Transport / output (not all are APVTS — some are processor state)
- **Preview Play** (UI toggle, non-automated): loops the current progression through the pad at host BPM (or 124 BPM fallback).
- **Host MIDI out:** when the host transport is playing, the progression is emitted as MIDI to the plugin's MIDI output, PPQ-aligned and looped to `length`.
- **BPM:** taken from host `AudioPlayHead`; manual fallback when standalone (124).

## Non-parameter state (saved in `getStateInformation`)
- Current generated `Progression` (chord roots/types/labels/positions) so a session reload restores exactly what was shown.
- Per-chord **lock** flags (locked chords survive re-generation).
- Last random **seed**.
