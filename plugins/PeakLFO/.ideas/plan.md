# Implementation Plan — PeakLFO

## Complexity Score: 3 / 5

## Implementation Strategy: **Phased** (score ≥ 3)

The engine is already written & validated, so phasing is about integration, output routing, and GUI.

### Phase 4.A — Core integration (headless-testable)
- [ ] Plugin CMake target (VST3, Visage; effect that reports as modulation/utility).
- [ ] Vendor `tools/fpc/FPCLfo.h` → `Source/FPCEngine.h` (single source of truth for DSP).
- [ ] `PluginProcessor`: APVTS with the 10 params + `cv_out` toggle.
- [ ] `prepareToPlay` → `FPCEngine::prepare(SR)`; param listeners → engine setters.
- [ ] `processBlock`: peak-extract (max|L|,|R|) → `pushAudioPeak`; per-sample `tick()`;
      audio passthrough; store last tick → mod-param outputs.
- [ ] Read-only output params (`out_peaklfo`, `out_peak`, `out_lfo`) exposed to host.

### Phase 4.B — Output transport & sync
- [ ] `cv_out` path: write chosen source as audio-rate CV to output bus.
- [ ] Parameter smoothing (base/amount) to avoid zipper noise on host param moves.
- [ ] Transport sync: `AudioPlayHead` PPQ → `FPCEngine::syncPhase` (retrig on transport start).

### Phase 4.C — Visage GUI (matches DFM layout)
- [ ] `Source/VisageControls.h`: custom `visage::Frame` vector wheel (draw ring/cap/pointer).
- [ ] PEAK panel (Base, Amount, Tension, Decay) + LFO panel (Base, Amount, Tension, Speed,
      Shape select, Phase) + output meter/scope visualiser.
- [ ] Bind controls ↔ APVTS; shape selector (Sine/Triangle/Square/Random).

### Phase 4.D — Polish & test
- [ ] Headless render harness: verify shapes, frequency vs speed, decay behavior.
- [ ] `pluginval` (strictness 10) clean.
- [ ] Edge cases: SR changes, extreme params, denormals, mono input.

## Dependencies
**Required JUCE modules:** juce_audio_basics, juce_audio_processors, juce_dsp, juce_core.
**GUI:** Visage (pure C++), juce_gui_basics (host window), juce_gui_extra.
**Vendored:** `Source/FPCEngine.h` (from `tools/fpc/FPCLfo.h`).

## Risk Assessment
**High Risk**
- **Absolute reference calibration** (exact wheel→raw scaling, host wavetables, tick cadence). Mitigated:
  perceptual-100 is the agreed v1 target; bit-exact deferred to a runtime-capture pass (spec §7).
- **Modulation output in arbitrary DAWs** — no universal "controller" bus; mod-params + CV is the
  portable compromise; behavior varies per host.

**Medium Risk**
- Custom Visage vector-wheel widget (hand-drawn; APC rules forbid assuming `visage::Knob`).
- Per-sample control cost — keep `tick()` branch-light; precompute curve coefficients on param change (engine already does this).

**Low Risk**
- The DSP math/shapes — already implemented and validated in `tools/fpc/`.
- Audio passthrough / APVTS plumbing — standard.

## Notes
- Per APC protocol: **no C++/CMake written in this phase** — that begins in `/impl`.
- Engine determinism already proven (compiles + 4 shapes + freq curve validated).
