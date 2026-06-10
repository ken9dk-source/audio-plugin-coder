# VAZ voice-render — Ghidra map (Core.dll, the per-sample synth DSP)

Found by digging from the filter process (no RTTI anchor for the synth engine). The whole
per-sample voice loop lives around **0x4DD2xx–0x4DE3xx**. `esi` = the VOICE object (per-sample
mod array at `[esi+idx*4]`), `ebx` = the params/patch object. Stages in order:

## 1. Mod generators  (~0x4DD2D0)
Segmented envelope/LFO generators: `[esi+0x12c]` = phase (24-bit, `&0xffffff`), `[esi+0x134]` =
segment end, `[esi+0x140]` = segment index, `[esi+0x144]`/`[esi+0x148]` = segment rate/target.
They read a **curve LUT `0x5445e0`** (`[edx*4 + 0x5445e0]`, clamped 0x3c00=15360). → This is the
ENVELOPE/LFO shape table. Each generator writes its value into the voice mod array `[esi+slot]`.

## 2. Oscillator  (~0x4DD3C0)  ← the exact osc
- **Wavetables `0x6dd2c0`, `0x6de2c0`** (and neighbours `0x6db2c0/0x6dc2c0`, 0x1000 = 1024 int32
  apart = **mip/band-limit levels**, selected by `cmp eax,ebp`).
- **24-bit phase**, table index = `phase >> 13` → 1024 entries; low 13 bits = interpolation fraction.
- **Pulse = `osc_a − osc_b`** (two reads at phase and phase−pw) → CONFIRMS the clone's saw−saw method
  is correct (the clone's pw=0.5 render bug was impl-specific, NOT the algorithm).
- osc outputs → `[esi+0x24]`, `[esi+0x2c]`.

## 3. Mixer  (~0x4DD4AB)
3 channels: each `src=[ebx+0x218/0x228/0x238]`, `level=[ebx+0x220/0x230/...]`, pre/post flag
`[ebx+0x21c/0x22c/...]`. `ecx` = pre-filter sum, `edi` = post-filter sum. (matches the clone's mix1/2/3 + Post.)

## 4. Filter  (~0x4DD560)  ← decoded earlier
`cutoff = base[+0x268] + Σ modval[src]·amt>>2` (3 cutoff-mod slots). Then the 22-mode biquad
(resonator b0/a1/a2 from the 2D tables, cubic `x−x³/2` — see reimpl-roadmap.md). Mode = `[ebx+0x258]`
(cmp 0x2d/0x12/2). Coef tables `0x5535e4` (reso), `0x5945e4/0x5d45e4/0x6145e4/0x6545e4/0x6945e4` (biquad banks).

## 5. Output  (~0x4DE286)
HP one-pole (`[esi+0x1a4..0x1b0]`, coef `0x6df6c4`) + **cubic soft-clip** (`ebp − ebp³` @0x4DE2EE) +
amp modulation (`[ebx+0x298]` src · `[ebx+0x29c]` amt). The main env1 VCA multiply is just after.

## All LUTs are BSS (runtime-built)
`0x5445e0` (curve), `0x5535e4`+ (filter), `0x6dd2c0`+ (waves) — none statically extractable.
**Builder fn @~0x4D46xx** (just before the cutoff builder 0x4D4720) fills them. → To get the EXACT
envelope curve + wave shapes, decode that builder (it generates the curve LUT 0x5445e0 + the wavetables).

## ⭐ The clean exact-extraction path: dump the BSS tables from the RUNNING process
The builder fn @~0x4D46xx is **const-pool-interleaved** (string fragments "Patc"/"bend" sit inside it)
→ a linear capstone disassembler desyncs on it (a full Ghidra project with code/data analysis would
handle it, but the lighter tool can't). **Better: the LUTs are BSS = filled at startup, so just READ
them from a running Vaz2010.exe** (OpenProcess + ReadProcessMemory at the Core.dll base + the offsets):
`0x5445e0` (curve/env LUT), `0x6dd2c0`+ (wavetables), `0x5535e4`/`0x69×5e4` (filter coef tables, to
cross-check the decoded biquad). That yields the EXACT built data (not the formula) — the cleanest route.

## Status / next exact-extraction targets
- **Envelope decay (the failed measurement):** the segmented generator @0x4DD2D0 + its curve LUT
  `0x5445e0` (built @0x4D46xx). Decode the builder's curve-fill + the generator's segment rates →
  exact attack/decay/release maps + the curve shape. (Validates/replaces the measured x⁴/x⁶.)
- **Wave shapes:** decode the wavetable builder (fills 0x6dd2c0+) → exact saw/tri/pulse.
- **Pulse:** method confirmed (saw−saw) — re-fix the clone's pw=0.5 render bug.
