# PeakLFO — Parameter Spec

> **v2 pivot (user request):** scope reduced to the **LFO section only, acting as a
> tempo-synced volume LFO (tremolo)** — no peak follower, no CV/linking. Base = overall
> volume, Depth = LFO swing amount, Speed = tempo steps, Phase = 0/25/50/75 %.
> The peak-follower spec below (v1) is retained for reference/future use.

## v2 — active parameters
| ID | Name | Type | Range / choices | Default |
| :--- | :--- | :--- | :--- | :--- |
| `lfo_base`    | Volume  | Float  | 0..1 (overall output volume) | 0.75 |
| `lfo_depth`   | Depth   | Float  | 0..1 (how much the LFO swings) | 0.5 |
| `lfo_tension` | Tension | Float  | -1..1 (shape skew) | 0.0 |
| `lfo_speed`   | Speed   | Choice | 1/2,1,2,3,4,8,16,32,64,128 steps (4 steps = 1 beat) | 16 steps |
| `lfo_shape`   | Shape   | Choice | Sine / Triangle / Square / Random | Sine |
| `lfo_phase`   | Phase   | Choice | 0% / 25% / 50% / 75% | 0% |

Tremolo law: `gain = volume · (1 − depth + depth · shape01)`; LFO phase locked to host
transport (`ppq / (steps/4)`), free-runs at host BPM when stopped.

---

## v1 (reference) — full Peak Controller model

Derived 1:1 from the reverse-engineered Fruity Peak Controller (`tools/fpc/FPC_LFO_SPEC.md`).
FL stores 10 params at `obj+0x120 + idx*4`. User-facing ranges below; the internal raw→formula
scaling (exponential curve inputs) is applied in `/impl` per the spec's curve constants.

## PEAK panel (envelope follower)
| ID | Name | Type | Range | Default | Unit | DFM control | Notes |
| :--- | :--- | :--- | :--- | :--- | :--- | :--- | :--- |
| `peak_base`    | Peak Base    | Float | -1.0 … 1.0 | 0.0 | — | `PeakBaseLvlWheel` | DC offset added to peak output (`base·16384`) |
| `peak_amount`  | Peak Amount  | Float | -1.0 … 1.0 | 0.5 | — | `PeakLvlWheel`     | depth; `(6^(|a|/256)-1)·scale` |
| `peak_tension` | Peak Tension | Float | -1.0 … 1.0 | 0.0 | — | `PeakLogWheel`     | 0 = linear; curve `T=(1001^(|t|/128)-1)·0.1` |
| `peak_decay`   | Peak Decay   | Float | 0.0 … 1.0  | 0.52| — | `FResOfsWheel`     | linear release; `(10001^(d/128)-1)·0.0192/SR` |

## LFO panel
| ID | Name | Type | Range | Default | Unit | DFM control | Notes |
| :--- | :--- | :--- | :--- | :--- | :--- | :--- | :--- |
| `lfo_base`    | LFO Base    | Float | -1.0 … 1.0 | 0.0 | — | `LFOBaseLvlWheel` | DC offset of LFO output |
| `lfo_amount`  | LFO Amount  | Float | -1.0 … 1.0 | 0.5 | — | `LFOLvlWheel`     | depth (same amount curve) |
| `lfo_tension` | LFO Tension | Float | -1.0 … 1.0 | 0.0 | — | `LFOLogWheel`     | shape skew (same tension curve) |
| `lfo_speed`   | LFO Speed   | Float | 0.0 … 1.0  | 0.5 | — | `LFOSpeedWheel`   | `f≈7.8125/(1001^speed-1)` Hz; 32-bit phase acc |
| `lfo_shape`   | LFO Shape   | Enum  | {Sine,Triangle,Square,Random} | Sine | — | `LFOShapeSelect` | 16384-pt wavetable, `phase>>18` |
| `lfo_phase`   | LFO Phase   | Float | 0.0 … 1.0  | 0.5 | turns | `LFOPhaseWheel`   | phase offset / song-sync anchor |

## Outputs (3 modulation sources)
| ID | Name | Notes |
| :--- | :--- | :--- |
| `out_peaklfo` | Peak+LFO | `clamp(peakOut + lfoOut, 0, 2³⁰)` |
| `out_peak`    | Peak     | peak source only |
| `out_lfo`     | LFO      | lfo source only |

All outputs full-scale `0x40000000` (2³⁰), exposed to host normalized 0..1 (`·2⁻³⁰`).
Host-output transport strategy decided in `/plan` (modulation params + optional CV out).

## Calibration notes (bit-exact upgrade path)
- User ranges above are the perceptual model. Exact FL wheel→raw mapping, host wavetable
  values (esp. Random), and control-tick cadence require a runtime FL capture (spec §7).
- Engine already implemented + validated: `tools/fpc/FPCLfo.h`.
