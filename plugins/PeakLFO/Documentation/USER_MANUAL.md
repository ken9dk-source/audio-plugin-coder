# PeakLFO — User Manual

**Version 1.0.0** · Tempo-synced volume LFO (tremolo)

PeakLFO is a modulation effect that rhythmically shapes the volume of whatever runs
through it, using a tempo-locked LFO. Its LFO shape and tension curve are a faithful
reconstruction of FL Studio's Fruity Peak Controller.

## Controls
| Control | What it does |
|---------|--------------|
| **VOLUME** | Overall output level — the level the LFO swings around. |
| **DEPTH**  | How much the LFO swings the volume up and down (0 = no effect). |
| **TENSION**| Skews the LFO shape (concave ↔ convex). Centre = neutral. |
| **SPEED**  | LFO period, locked to host tempo: 1/2, 1, 2, 3, 4, 8, 16, 32, 64, 128 steps (4 steps = 1 beat). |
| **PHASE**  | Cycle start offset, locked to 0 / 25 / 50 / 75 %. |
| **SHAPE**  | LFO waveform: Sine, Triangle, Square, Random. |

The LFO is **locked to your DAW's transport** — it follows the song position and BPM,
so the tremolo stays in time. When the transport is stopped it free-runs at the host tempo.

## Installation
- **Windows:** copy `PeakLFO.vst3` to `C:\Program Files\Common Files\VST3\`.
- **macOS:** copy `PeakLFO.vst3` to `/Library/Audio/Plug-Ins/VST3/` and/or
  `PeakLFO.component` to `/Library/Audio/Plug-Ins/Components/` (AU).
- A **Standalone** app is also included for quick testing.

## Credits
Built with Audio Plugin Coder (APC). DSP reverse-engineered from FL's Fruity Peak Controller.
© 2026
