# DSP Architecture Specification — PeakLFO

## Core Components
The DSP core is already implemented and validated (`tools/fpc/FPCLfo.h` → will be vendored as
`Source/FPCEngine.h`). Architecture wraps it for a generic VST3 host.

1. **Audio tap / peak extractor** — per block, compute `max(|L|)`, `max(|R|)` (matches the reference's
   IPP max-abs `FUN_005efbc0`); feed `FPCEngine::pushAudioPeak`.
2. **FPCEngine** (the ported engine) — peak follower (instant attack, linear decay) + LFO
   (uint32 phase acc, 16384 wavetable, 4 shapes) + tension/amount/base shaping + output combine.
3. **Per-sample control tick** — `FPCEngine::tick()` runs **once per audio sample**. This is
   dictated by the RE'd math: decay coef is `/SR` (per-sample subtraction) and the LFO period
   is expressed in samples with `inc = 2³²/period_samples`. Produces 3 values/sample.
4. **Output transport** — exposes the 3 sources three ways:
   - **Modulation parameters** (read-only, automatable APVTS params) — host mod-mapping. Updated
     once per block with the block's last tick value.
   - **CV audio output** (optional, gated by a `cv_out` toggle) — write control signal to the
     output bus at audio rate (for CV/modular chains).
   - **MIDI CC out** (deferred to a later pass).
5. **Audio passthrough** — like the original, PeakLFO does **not** color audio; input is copied
   to output unchanged unless `cv_out` replaces the bus with CV.
6. **Parameter binding** — APVTS (10 params) → `FPCEngine` setters; smoothed where needed.
7. **Transport sync** (optional) — `FPCEngine::syncPhase(ppqPos)` from `AudioPlayHead` for
   tempo-anchored LFO phase (mirrors the reference `FUN_007062a0`).

## Processing Chain
```
                 ┌──────────────────────────── per sample ───────────────────────────┐
Audio In ──► PeakExtract(max|L|,|R|) ──► FPCEngine.pushAudioPeak (per block)          │
                                          FPCEngine.tick() ──► {peaklfo, peak, lfo} ───┤
                                                                    │                  │
                                        ┌───────────────────────────┼──────────────┐   │
                                   ModParams(RO, per block)   CV out (opt)    (MIDI opt)│
Audio In ─────────────────────────────────────────────────────────────────────► Audio Out (passthrough)
```

## Parameter Mapping
| Parameter | Component | Function | User range |
|-----------|-----------|----------|------------|
| peak_base    | FPCEngine.setPeakBase   | peak output DC offset (`base·16384`)          | -1..1 |
| peak_amount  | FPCEngine.setPeakAmount | peak depth (`(6^(|a|/256)-1)·scale`)          | -1..1 |
| peak_tension | FPCEngine.setPeakTension| peak curve `T=(1001^(|t|/128)-1)·0.1`         | -1..1 |
| peak_decay   | FPCEngine.setPeakDecay  | linear release `(10001^(d/128)-1)·0.0192/SR`  | 0..1  |
| lfo_base     | FPCEngine.setLfoBase    | LFO output DC offset                           | -1..1 |
| lfo_amount   | FPCEngine.setLfoAmount  | LFO depth                                      | -1..1 |
| lfo_tension  | FPCEngine.setLfoTension | LFO shape skew                                 | -1..1 |
| lfo_speed    | FPCEngine.setLfoSpeed   | `f≈7.8125/(1001^speed-1)` Hz                   | 0..1  |
| lfo_shape    | FPCEngine.setShape      | Sine/Triangle/Square/Random wavetable          | enum  |
| lfo_phase    | FPCEngine.setLfoPhase / syncPhase | phase offset / song anchor           | 0..1  |
| cv_out       | transport               | route CV to audio bus                          | bool  |

## Complexity Assessment
**Score: 3 / 5 (Advanced)**

**Rationale:** The DSP algorithm itself is fully solved and validated (lowers risk), but the
plugin still requires: (a) non-trivial output routing — modulation params + optional audio-rate
CV in a host with no native "controller" bus; (b) per-sample control processing with correct
smoothing; (c) a custom Visage GUI (two panels, vector wheels, live meter/scope) which per APC
rules means hand-drawn `visage::Frame` widgets; (d) transport/tempo sync. Not a trivial single
knob, not a research-grade synth — solidly Level 3.
