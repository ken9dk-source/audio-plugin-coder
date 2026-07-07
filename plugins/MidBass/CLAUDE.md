# MidBass — agent notes

Focused trance mid-bass synth (2004–2010 uplifting/hard trance). NOT a
general-purpose synth: every control must serve that one sound. Low CPU, one
fixed panel, no hidden menus.

## Ground rules
- **Reuse-first.** Verified DSP already exists in this repo — port, don't rewrite:
  see `.ideas/vaz-reuse-audit.md` for the module-by-module map
  (VAZClone OscBlock/Type-R, TranceAcid ADSR/LFO/SVF/ModMatrix/VazOsc,
  TranceEQ Biquad/FFT, TranceKick saturate/punch, VazChorus/Phaser/Delay/Reverb engines).
- **Phased development** per `ROADMAP.md`. One phase = one commit. Tests green
  before commit. Write the failing test BEFORE the DSP. Do not start the next
  phase without approval.
- **Verify-before-edit:** read existing code before modifying it.

## Build / test
- Build+install (from repo root, never manual cmake):
  `powershell -ExecutionPolicy Bypass -File .\scripts\build-and-install.ps1 -PluginName MidBass`
- Tests: configure with `-DAPC_BUILD_TESTS=ON`, target `MidBass_Tests`
  (Catch2 v3, headless: `MIDBASS_HEADLESS=1`, no editor/GUI compiled).
- pluginval: `scripts/pluginval-integration.ps1`.

## Architecture
- Native JUCE GUI (custom LookAndFeel + MidiKeyboardComponent) — NOT WebView.
  `status.json: ui_framework = "native"` (like TranceChords).
- All parameters exist from Phase 0: IDs in `Source/ParameterIDs.hpp`, layout in
  `Source/Params.h` (single source of truth for ranges/defaults). Never add a
  param elsewhere.
- VAZClone headers come in via include path
  (`target_include_directories(... ../VAZClone/Source)`), header-only — same
  pattern TranceAcid uses. MidBass `Source/` is first so local headers win.
- Filter: VAZ Type-R (bit-exact integer) = LP24/LP12; ZDF SVF = HP12/BP12.
- Macros/sweet-spots are data tables (`MacroMap`, `SweetSpots`) — tune there only.

## Signal flow
OSC1/2/3 (+sync/FM/ring) + SUB → unison/stack mix → drive PRE → FILTER (fenv,
keytrack, LFO/matrix) → drive POST → SATURATION (5 types, 2x OS, parallel mix) →
EQ (LS/peak/HS) → TRANSIENT shaper → amp env × VCA → FX chain
(Chorus→Phaser→Flanger→Delay→Reverb→Comp, each bypassable) → output.
