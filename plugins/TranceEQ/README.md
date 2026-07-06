# TranceEQ

A genre-aware parametric EQ for **Trance Leads, Plucks & Midbasses**, built from the design
document *"Design af EQ-Plugin til Trance Leads, Plucks og Midbasses"*. WebView UI, JUCE 8, VST3 + Standalone.

## What it does
Pick **what you're working on** — `Lead`, `Pluck`, or `Midbass` — and the EQ reconfigures its 6 bands to a
template tuned for that sound (frequencies, Q and gains taken straight from the reference table in the PDF).
Then shape it by dragging the curve.

## Features
- **3 work modes** (Lead / Pluck / Midbass) — the mode tabs load a tailored band template.
- **6 parametric bands** — High-Pass, Low/High Shelf, Peak, Notch, Low-Pass (RBJ Audio-EQ-Cookbook biquads).
- **Live spectrum analyser** (FFT, peak-hold) drawn behind an **interactive EQ curve** — drag a node for
  freq/gain, wheel for Q, ctrl-click to reset.
- **Key-tracking** ("følgende EQ") — chosen peak bands follow the played note. Source = **Audio** (built-in
  YIN pitch detector) or **MIDI** (note input). Amount + reference pitch are adjustable.
- **Neuroinclusive UI** — calm, muted midnight-blue/teal palette and a **Focus Mode** that hides the detail
  and shows just a few macros (Low Cut · Body/Punch · Presence/Attack · Air · Output).

## Build
```powershell
powershell -ExecutionPolicy Bypass -File .\scripts\build-and-install.ps1 -PluginName TranceEQ
```
Artefacts:
- VST3 — `build\plugins\TranceEQ\TranceEQ_artefacts\Release\VST3\TranceEQ.vst3`
- Standalone — `build\plugins\TranceEQ\TranceEQ_artefacts\Release\Standalone\TranceEQ.exe`

> Installing the VST3 into `C:\Program Files\Common Files\VST3` needs an **elevated** shell
> (run the build script, or `scripts\install-vst3.ps1`, as Administrator).

## Layout
| File | Role |
|------|------|
| `Source/Biquad.h` | RBJ cookbook biquad (mirrored in the UI so the curve == the audio) |
| `Source/PitchDetector.h` | YIN pitch detector for audio key-tracking |
| `Source/PluginProcessor.*` | 42 params, 6 bands, mode templates, key-track, FFT spectrum |
| `Source/PluginEditor.*` | WebView relays/attachments + 30 Hz spectrum push |
| `Source/ui/public/index.html` | Self-contained UI (spectrum + curve + controls) |

## Not yet implemented (PDF "nice-to-have")
Linear-phase mode, oversampling, sidechain pitch input, automatic EQ-match, ARA.
