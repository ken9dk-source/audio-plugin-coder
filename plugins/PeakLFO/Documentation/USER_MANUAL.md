# PeakLFO — User Manual

**Version 1.1.0** · tempo-syncable volume LFO (self-modulating)

PeakLFO is a precision volume LFO (tremolo): it applies a shaped, tempo-lockable LFO
directly to the level of whatever runs through it. Its signature is the **Tension** control,
which warps the waveform into asymmetric, peaked curves.

## Output model
```
out = Base + shape(phase, tension) * Volume
```
The result is the gain applied to the audio (a self-modulating insert).

## Controls
| Control | What it does |
|---------|--------------|
| **BASE**    | Output floor/offset — the level when the LFO is at its low point. |
| **VOLUME**  | How far the LFO swings from Base. **Bipolar**: turn negative to **invert** the wave. The knob has a logarithmic taper (fine control near the centre). |
| **TENSION** | Warps the LFO shape (the signature the reference curve). Centre = unwarped; more tension = more peaked/asymmetric. |
| **SPEED**   | LFO rate. See **MODE**. |
| **MODE**    | **SYNC** (default) = tempo-locked steps (1/2, 1, 2, 3, 4, 8, 16, 32, 64, 128; 4 steps = 1 beat). **FREE** = free-running rate in Hz (faithful to the original the reference, which is not tempo-synced). |
| **PHASE**   | Cycle start offset, full **0–100 %**. |
| **SHAPE**   | Sine, Triangle, Square, **Saw**, Random. |

In **SYNC** mode the LFO follows your DAW's transport (position + BPM); when stopped it
free-runs at the host tempo. **FREE** mode ignores tempo and runs at the Hz you dial in.

## Installation
- **Windows:** copy `PeakLFO.vst3` to `C:\Program Files\Common Files\VST3\`.
- **macOS:** copy `PeakLFO.vst3` to `/Library/Audio/Plug-Ins/VST3/` and/or the AU
  `PeakLFO.component` to `/Library/Audio/Plug-Ins/Components/` (built via CI).
- A **Standalone** app is included for quick testing.

## Credits
Built with Audio Plugin Coder (APC). © 2026
