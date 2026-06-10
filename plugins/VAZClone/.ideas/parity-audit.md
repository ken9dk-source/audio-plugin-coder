# VAZClone ↔ VAZ 2010 — Functional/Mathematical Parity Audit

**Reference:** `Vaz2010Core.dll` (Borland C++ Builder, fixed-point) — ground truth.
**Target:** VAZClone C++ source (JUCE/float). Compared **source ↔ RE'd reference** (disassembling
the JUCE binary and matching it function-by-function to a Borland binary is infeasible — different
compiler/runtime/code layout — so the clean source is the better target ground-truth).

## ⚠️ The bit-exact reality (read first)
The target is **by design a float, topology-accurate, character-tuned approximation** — the source
says so throughout ("not bit-exact", "VAZ uses fixed-point", "aliasing ≈ VAZ's target", "Measured
from VAZ", "tuned to character"). It uses **mathematically-generated wavetables + tuned coefficients**,
NOT VAZ's fixed-point engine + ROM LUTs. So **true 1:1 identity is not reachable without re-implementing
VAZ's fixed-point DSP and extracting all its lookup tables** — effectively rewriting the synth from the
binary. The achievable goal is **perceptual identity**: RE + match the *audible* parameters (envelope
curves, filter coefficients, wave tables, time/scaling maps) subsystem by subsystem. This document is
the deviation queue toward that.

Status legend: **[M]** measured/RE-verified · **[A]** acknowledged approximation (source comment) ·
**[R]** needs reference RE · **[G]** functional gap (missing feature).

---

## Installment 1 — DSP core (osc / env / LFO / filter / modulation / voice)

### A. Oscillators  (Synth.h)
| # | Ref | Target | Difference | Impact | Minimal fix | St |
|---|---|---|---|---|---|---|
| A1 | VAZ wavetable ROM LUTs (32-bit phase, top-bits index, cubic interp, 256/512) | `WaveTables::generate` additive sin-sum (Synth.h:32-57) + `OnePoleLP` 14 kHz "tilt" (SynthVoice.h:110) | Harmonic amplitudes/phases are mathematically ideal, not VAZ's actual tables; the HF tilt is a hand-tuned guess ("refine w/ clean render") | **High** — raw timbre | Extract VAZ's wave LUTs from Core.dll, use directly | R |
| A2 | VAZ pulse/PWM wave + its PW range/curve | Pulse = `saw(t)−saw(t−pw)`, `pw=0.5−0.49·ws` (Synth.h:153-159) | PW range (0.01..0.5) + curve are guessed | **Med** | RE VAZ pulse + PW map | R |
| A3 | Multi-Saw measured **0/+24/+48/+72.5 c** (one-sided, mean +36 c) | 4 saws **symmetric ±36.25·ws** (Synth.h:161-174) | Target re-centres to ±36 → whole multisaw ≈ **36 c flatter** than VAZ + symmetric vs one-sided distribution | **Med** (pitch + beating) | Offset taps to one-sided 0/+24/+48/+72.5·ws | M/A |
| A4 | VAZ note-on phase behaviour | Oscillators **free-running**, no phase reset on note-on (SynthVoice.h:125) | If VAZ resets osc phase on note-on, attack transient + osc correlation differ | **Low-Med** (transient) | Verify VAZ; add optional phase reset | R |

### B. Envelopes  (Synth.h VAZEnv, SynthVoice.h)
| # | Ref | Target | Difference | Impact | Minimal fix | St |
|---|---|---|---|---|---|---|
| B1 | VAZ ADSR attack curve | Attack is **linear** ramp `level += aInc` (Synth.h:296) | VAZ attack may be exp/curved | **High** (envelope feel) | RE VAZ attack shape | R |
| B2 | VAZ decay/release time-constants | one-pole with **`·0.35` fudge** factor (Synth.h:282,284) | The 0.35 is a tuning guess → decay/release reach target faster/slower than VAZ | **High** (timing) | RE VAZ decay/release rate | R |
| B3 | VAZ "Curve" = exp/lin toggle (spec L105) | Curve = **`level²`** always when on (Synth.h:302) | level² is a crude exp; VAZ's exact curve unknown; also applies to *output*, not the rate | **High** | RE VAZ curve | R |
| B4 | VAZ ADSR time range/map | `t=0.001+x²·3.0` (1ms..3s, amp) / `0.0002+x²·4.0` (filt) (SynthVoice.h:268,272) | Min/max + quadratic curve are guesses | **High** (timing) | RE VAZ time map (per .v2p byte→sec) | R |
| B5 | VAZ stage-end thresholds | decay→sus `<0.0008`, rel→idle `<0.0004` (Synth.h:297,299) | Arbitrary cutoffs shift stage timing slightly | Low | match to VAZ epsilon | A |

### C. LFOs  (Synth.h ModLFO)
| # | Ref | Target | Difference | Impact | Minimal fix | St |
|---|---|---|---|---|---|---|
| C1 | VAZ 8 LFO waveforms + shape/morph | Reconstructed from manual descriptions (Synth.h:228-263) | Exact morph curves, fade-in times (`shape²·4`), S&H lag (`shape·0.2`) are guesses | **Med** (mod character) | RE VAZ LFO table/shapes | R |
| C2 | VAZ LFO rate→Hz map + sync | (in PluginProcessor.cpp — installment 2) | TBD | **Med** | RE rate map | R |

### D. Filter  (Synth.h VAZLadder/VAZMultiFilter)
| # | Ref | Target | Difference | Impact | Minimal fix | St |
|---|---|---|---|---|---|---|
| D1 | cutoff coef table @0x4D4720: `fc=e^(10.24·param)` | `std::exp(10.24·coNorm)` (SynthVoice.h:249) | **RE-verified within ~4 %** | High (but ✅ small residual) | tighten last 4 % if needed | M |
| D2 | one-pole pole coef @0x4D4720 | `g=1−((2−cosω)−√(...))` (Synth.h:341) | **RE-exact** | — | — | M |
| D3 | VAZ resonance/self-osc per engine | `fb=reso·4.2` (R, Synth.h:342), `reso·4.5` (K), `k=2−1.98·reso` (A) etc. | All **tuned constants**, not RE'd | **High** (the signature) | RE each engine's reso map + sat | R |
| D4 | VAZ 6 engines A/B/C/D/K exact topology+coef | TPT/Chamberlin/SK reconstructions (Synth.h:446-499) | Topologies RE'd, but coef/sat/separation maps tuned | **High** | RE per-engine coefficients | A/R |
| D5 | VAZ filter saturation shape | `cubic` 1.5(x−x³/3), `soft` x−x³/6.75 (Synth.h:327,396) | Soft-clip curves are guesses | Med | RE VAZ saturation LUT | R |

### E. Modulation matrix  (SynthVoice.h)
| # | Ref | Target | Difference | Impact | Minimal fix | St |
|---|---|---|---|---|---|---|
| E1 | **2 FM slots per osc** (spec L42/135) | only **1** (`o1FmAmt/o2FmAmt`, SynthVoice.h:216-217) | 2nd FM input per osc missing entirely (GUI fm1b/fm2b dead) | **High** | add o1_fm2/o2_fm2 src+amt+inv + DSP | G |
| E2 | all mod slots **bipolar (+/−)** (spec L136/194) | missing `lfo2_rm_amt_inv`, amp-env invert; some bound | **Med** | add the missing invert params | G |
| E3 | 22 mod **sources** | ModBus.value: many `default:0` — Seq A/B, Ext In, Accent, Ctrl A, Osc1 Pitch, etc. not implemented (SynthVoice.h:19-38) | Selecting those sources → no modulation | **Med-High** | implement remaining sources | G |
| E4 | VAZ FM type/depth | exponential `f·2^(mv·amt)`, ±1 oct (SynthVoice.h:216) | VAZ FM may be linear/through-zero + different depth scaling | **High** (FM timbre) | RE VAZ FM math | R |
| E5 | mod application order/smoothing | per-sample mod bus ✓; tremolo `1+amt·mv` clamped 0..2 (SynthVoice.h:252) | depth scaling guessed | Med | RE amp-mod depth | A |

### F. Pitch / voice / timing  (SynthVoice.h)
| # | Ref | Target | Difference | Impact | Minimal fix | St |
|---|---|---|---|---|---|---|
| F1 | VAZ coarse/fine continuous or stepped? | **quantised** coarse ±12 st-steps, fine ±100 c-steps (`round`, SynthVoice.h:169) | If VAZ is continuous, target stair-steps detune | **Med** | verify; drop round() if continuous | R |
| F2 | VAZ portamento time + curve | glide `ws²·0.6` (0..0.6 s), lin-cents default + exp opt (SynthVoice.h:173-181) | time range + curve tuned | Med | RE glide map | A |
| F3 | VAZ velocity curve | `level=max(0.05, vel)` linear + 0.05 floor (SynthVoice.h:120) | VAZ vel→amp curve unknown | Med | RE vel curve | R |
| F4 | VAZ output gain staging | `·0.6` + `ampLevel·0.6` (SynthVoice.h:253) | overall level-match constant | Low (level) | calibrate to VAZ RMS | A |
| F5 | VAZ key-track range/curve | `(note−24)/72` C1..C7 linear (SynthVoice.h:121) | range/curve guessed | Low-Med | RE keytrack | R |
| F6 | VAZ voice-alloc / stealing | JUCE `findFreeVoice` + note-steal (SynthVoice.h:325) | steal order (oldest/quietest) may differ | Low | match VAZ steal rule | R |

---

## Work queue (reference RE needed, by audibility)
1. **Envelope** curves + time map (B1-B4) — highest day-to-day audibility.
2. **Filter** per-engine reso/saturation maps (D3-D5).
3. **FM** math + 2nd slot (E1, E4).
4. **Oscillator** wave LUTs (A1) + pulse/multisaw (A2-A3).
5. **LFO** shapes + rate map (C1-C2).
6. Remaining mod sources (E3), pitch quantise (F1), velocity (F3).

## Installment 2 (TODO) — PluginProcessor.cpp
param scaling maps, LFO rate/sync, .v2p preset decode (currently 45/115 params; gaps:
mix_src, o1_level, e2_dest, env modes, lfo_wave), block/timing, MIDI handling, mod-amp/lag math.
