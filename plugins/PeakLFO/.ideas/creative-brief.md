# PeakLFO — Creative Brief

## Hook
A bit-faithful reconstruction of FL Studio's **Fruity Peak Controller** — a dual-source
modulation generator that turns audio dynamics and a precision LFO into clean, host-usable
control signals. Reverse-engineered to the formula, not just the vibe.

## Description
PeakLFO is a **modulation/utility effect** (not an audio-coloring effect). It listens to the
incoming audio, extracts its peak envelope, and simultaneously runs a phase-accurate LFO.
Both are shaped (base offset, amount, exponential tension curve) and combined into three
outputs — **Peak+LFO**, **Peak**, and **LFO** — exactly as the original does.

The DSP is a validated 1:1 port (see `tools/fpc/FPC_LFO_SPEC.md` + `FPCLfo.h`):
- **Peak follower:** instant attack `peak = max(peak, |L|, |R|)`, linear decay
  `peak -= (10001^(decay/128)-1)·0.0192/SR` per control tick.
- **LFO:** uint32 phase accumulator, `inc = 2³²/period`, `period` from `1001^(speed/65536)`;
  16384-point wavetable indexed by `phase>>18`; shapes **Sine / Triangle / Square / Random**.
- **Tension curve** (both sources): `(T+1)^s−1` / `T−((T+1)^(1−s)−1)`, `T=(1001^(|t|/128)−1)·0.1`.
- **Output:** full-scale 2³⁰, normalized 0..1.

## Character / Vibe
Two-panel "meter box" aesthetic straight from the original's embedded form: a **PEAK** panel
and an **LFO** panel of round vector wheels, with a live meter/scope visualiser. Clean,
technical, functional — a modulation tool, not a toy.

## Key design question for /plan (host-output strategy)
The original emits FL-internal controllers. In a generic VST3 host there is no equivalent
"internal controller" bus, so /plan must choose how PeakLFO exposes its output:
1. **Modulation parameters** — expose Peak+LFO / Peak / LFO as read-only automatable params
   the host can map (works in most DAWs).
2. **CV / audio-rate output** — write the control signal to the output audio bus (for CV chains).
3. **MIDI CC output** — emit the values as MIDI CC.
Recommended default: **(1) + (2)** — modulation params for mapping, plus an optional CV output.

## Scope
- v1: full 10-parameter model, all 4 shapes, three outputs, Visage GUI matching the DFM layout.
- Deferred to bit-exact pass (needs runtime FL capture): exact host wavetables, exact param
  raw-value scaling, exact control-tick cadence. See spec §7.
