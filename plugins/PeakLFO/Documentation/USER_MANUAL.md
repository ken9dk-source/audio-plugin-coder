# PeakLFO — User Manual

**Version 1.1.0** · FL Fruity Peak Controller LFO (self-modulating volume)

PeakLFO reproduces the **LFO section** of FL Studio's Fruity Peak Controller and applies it
directly to the volume of whatever runs through it. Its shape, tension warp and volume taper
are a faithful reconstruction of the original.

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
| **TENSION** | Warps the LFO shape (the signature FPC curve). Centre = unwarped; more tension = more peaked/asymmetric. |
| **SPEED**   | LFO rate. See **MODE**. |
| **MODE**    | **SYNC** (default) = tempo-locked steps (1/2, 1, 2, 3, 4, 8, 16, 32, 64, 128; 4 steps = 1 beat). **FREE** = free-running rate in Hz (faithful to the original FPC, which is not tempo-synced). |
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
Built with Audio Plugin Coder (APC). DSP reverse-engineered from FL's Fruity Peak Controller
(LFO section). © 2026
