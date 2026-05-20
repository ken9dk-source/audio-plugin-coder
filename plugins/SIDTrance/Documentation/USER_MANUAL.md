# SID Trance Machine — User Manual
**Version 1.0.0 | C-MOS Audio**

---

## Overview

SID Trance Machine (C-MOS) is a hybrid synthesizer that fuses the raw digital character of the MOS Technology 6581/8580 SID chip with the polyphonic power demanded by modern trance music production.

Use it for: uplifting trance leads, emotional plucks, resonant SID basses, and evolving pads.

---

## Installation

### Windows
Copy `SID Trance Machine.vst3` to: `C:\Program Files\Common Files\VST3\`

### macOS
Copy `SID Trance Machine.vst3` to: `/Library/Audio/Plug-Ins/VST3/`
Copy `SID Trance Machine.component` to: `/Library/Audio/Plug-Ins/Components/`

---

## Signal Flow

```
MIDI In → Arp/Sequencer → 16-Voice PolyEngine
  Each voice: OSC1 + OSC2 + OSC3 → Filter → Amp Envelope
  Modulation: LFO1 + LFO2 + Mod Matrix
→ Unison Spread (SID WIDTH)
→ ANALOG GLOW Saturation
→ Chorus → Delay → Reverb
→ Limiter → Stereo Out
```

---

## Oscillators (OSC 1 / 2 / 3)

Each oscillator has independent settings:

| Control | Description |
|---|---|
| **Waveform** | SAW, TRI, PULSE, NOISE, SAW+TRI, RING MOD |
| **SEMI** | Pitch offset in semitones (-24 to +24) |
| **FINE** | Pitch offset in cents (-100 to +100) |
| **PW** | Pulse width — only affects PULSE waveform |
| **A/D/S/R** | Per-oscillator amplitude envelope |
| **VOLUME** | Oscillator output level |

**Tip:** Set OSC2 to TRI with -7 cents fine tuning for the classic SID trance pluck thickness.

---

## Filter

The filter is modeled after the MOS 6581 SID chip's non-linear ladder filter with modern enhancements.

| Control | Description |
|---|---|
| **CUTOFF** | Filter cutoff frequency (20 Hz – 18 kHz) |
| **RESONANCE** | Filter resonance — pushes toward self-oscillation at high values |
| **Type** | Low Pass, High Pass, Band Pass, Notch |
| **Slope** | 12 dB/oct or 24 dB/oct rolloff |
| **KEY TRACK** | Cutoff follows MIDI note pitch |
| **VELOCITY** | Cutoff opens with note velocity |

---

## Amplifier

Global amplitude envelope applied after the filter:

| Control | Description |
|---|---|
| **ATTACK** | Time from note-on to full level |
| **DECAY** | Time from peak to sustain level |
| **SUSTAIN** | Level held while note is held |
| **RELEASE** | Time from note-off to silence |
| **VOLUME** | Amplifier output level |

---

## LFO 1 & LFO 2

Two independent low-frequency oscillators for modulation:

| Control | Description |
|---|---|
| **Shape** | Sine, Triangle, Saw, Rev.Saw, Square, Sample & Hold |
| **RATE** | LFO speed in Hz (free) or beat divisions (sync) |
| **SYNC** | Lock LFO rate to host DAW tempo |
| **RETRIG** | Restart LFO phase on each new note |

---

## Mod Matrix

4 modulation slots. Each slot connects a **Source** to a **Destination** with a bipolar **Amount**.

**Sources:** LFO1, LFO2, Velocity, Mod Wheel, Amp Env, MIDI Key

**Destinations:** Filter Cutoff, OSC1 PW, OSC2 PW, OSC3 PW, Amp Level, Filter Res, OSC1 Fine, OSC2 Fine

**Tip:** LFO1 → Filter Cutoff at 37.5% is the classic trance filter sweep.

---

## Effects

All three effects are in series: Chorus → Delay → Reverb.

### Chorus
BBD-style stereo chorus adds width and movement.
- **RATE**: LFO speed for delay modulation
- **DEPTH**: Modulation depth (more = wider)
- **MIX**: Dry/wet blend

### Delay
Ping-pong delay with tempo sync.
- **TIME**: Note division (1/16, 1/8, 1/4, 3/8, 1/2, 3/4, 1/1)
- **FEEDBACK**: Echo repetitions (keep below 70% for clean repeats)
- **MIX**: Dry/wet blend

### Reverb
Schroeder reverb (Freeverb algorithm).
- **SIZE**: Room size (larger = longer decay)
- **DAMPING**: High-frequency roll-off in the reverb tail
- **MIX**: Dry/wet blend

---

## Master

| Control | Description |
|---|---|
| **OUTPUT** | Master output level fader |
| **LIMITER** | Brickwall limiter to prevent clipping |
| **MONO/POLY** | Monophonic or polyphonic voice mode |
| **VOICE COUNT** | Maximum simultaneous voices (1–16) |

---

## Arp / Sequencer

The 16-step arpeggiator patterns held notes across a rhythmic sequence.

| Control | Description |
|---|---|
| **MODE** | Up, Down, UpDown, Random, As-Played |
| **OCTAVE** | Arpeggiate across 1–4 octaves |
| **GATE** | Note length as fraction of step length |
| **SWING** | Timing swing for groove |
| **TEMPO** | Internal BPM (overridden by DAW when SYNC is on) |
| **SYNC** | Lock to host DAW tempo |
| **Step 1–16** | Click to enable/disable each step in the pattern |

---

## Unique Features

### TRANCE DRIFT
Adds subtle random pitch micro-instability per voice — mimics the SID chip's inherent tuning imprecision. Keep below 0.3 for natural warmth; turn up for unstable lo-fi character.

### DIGITAL AGE
Bit-reduction and sample-rate crushing. At 0, the sound is clean. At maximum, you get authentic 4-bit SID grit.

### SID WIDTH
Stereo unison spread. Controls the stereo width of the voice output via a Haas-effect spread.

### ANALOG GLOW
Soft saturation modeled on the SID chip's output stage combined with Juno chorus warmth. Adds harmonic richness without harshness.

---

## SID Mode

| Mode | Description |
|---|---|
| **Classic SID** | Aliased waveforms, no anti-aliasing — raw C64 character |
| **Modern SID** | PolyBLEP anti-aliasing — clean but still gritty |
| **Trance SID** | Modern + all special features enabled — production-ready |

---

## Preset: TRANCE_LEAD_01

The default patch matches the reference design:
- OSC1: SAW, OSC2: TRI (-7 cents), OSC3: NOISE
- Filter: LP 24dB, Cutoff 2.48 kHz, Resonance 35%
- LFO1: Sine → Filter Cutoff (+37.5%)
- LFO2: Triangle → OSC1 PW (+25%)
- Arp: Up, 1 Oct, 16 steps, 140 BPM
- FX: Chorus + Delay (1/4) + Reverb all active

---

## Credits
Built with **Audio Plugin Coder (APC)** using JUCE 8 and Visage.
© 2026 C-MOS Audio
