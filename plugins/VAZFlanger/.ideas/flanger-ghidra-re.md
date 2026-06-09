# VAZ Flanger — Ghidra RE (fresh, from Vaz2010Core.dll)

Reverse-engineered directly from the binary (not prior notes). Function region ~`0x51E868`–`0x51F0xx`.

## Topology
**4 cascaded RBJ-style biquad sections** (a resonant comb), NOT a pure delay line.
Coefficients are computed in float then **stored as Q20 fixed-point** integers — the inner
per-sample loop is integer DSP, so a bit-exact match is impossible, but the coefficient-domain
math below is fully recovered.

| Address | What it does |
|---|---|
| `0x51EB34` | per-section coefficient setup (called 4× from `0x51F430`) |
| `0x51EBA7` | `Power((delay·18/5080), 10)` → section feedback/Q  (`0x42c220`=pow, `0x51f094`=5080) |
| `0x51EBB0` | **center freq `f = 20·exp(0.027·X)`**  (`0x402ba8`=exp, `0x51f0a4`=20.0) |
| `0x51EC30` | **α = 0.5** for modes 1-4 (mode 0 = `exp(param·0.018)·0.05`) |
| `0x51EC7F` | 5-way mode jump → `0x51ec9c / edae / eec0 / ef3c / efb7` |
| `0x51F038` | coefficients ×`1048576` (2²⁰) → `0x402bf4` round → **Q20 int** at `[ebx+0x20/24/28]` |

## Coefficient math (mode 1 @ `0x51EC9C`, RBJ form, C = cosω, α = section alpha)
```
a0   = (C-1)·α + (C+1) + D·E
norm = 1 / a0
b1   = -2·norm·[ (C+1)·α + (C-1) ]
...   (5 modes compute different (C±1)·α combinations = LP/HP/notch/AP/shelf variants)
```

## Mode selection
`esi` (0-4) from `[ebx+8]` (section base type 1/2) and **`[ebx+0x14]` (0 or 0xff = Feedback Phase)**.
→ In the real VAZ, **Feedback Phase changes the biquad TYPE**, not just the feedback sign.

## Constants
`5080.0` (max delay samples, normaliser) · `2²⁰` (coef Q20) · `2³⁰`/`-1.07e8` (state) ·
`20.0` (freq floor ×) · `0.027` (freq exp coeff) · `0.5` (α) · `±2.0, ±1.0` (biquad).

## Applied to the clone
- `f0Base = 20·exp(0.027·200·(1-fDelay))` — real's exact 20 + 0.027 constants, ~20 Hz floor.
- `α = 0.5` (already matched).
- 4 sections + geometric sweep (already matched).

## Full coefficient decode (all 5 modes) — `0x51EB34`

### Setup (`0x51EBBB`–`0x51EC7E`) — stack-slot map
```
[esp]      P     = Power(delay·18/5080, 10)            ; delay-derived, 0..~0.37
[esp+0x18] S     = sin(ω)            ω = 2π·f/sr (clamped 0.45), f = 20·exp(0.027·param)
[esp+0x20] C     = cos(ω)
[esp+0x28] alpha = 0.5 (modes 1-4)  |  exp(param·0.018)·0.05 (mode 0)
[esp+0x30] A30   = S / (2·alpha)     = sin(ω)            (RBJ bandwidth term, alpha=0.5)
[esp+0x38] A38   = sqrt(P) / alpha   = 2·sqrt(P)
```
### Mode select (`esi`): [ebx+8]∈{1,2} (section base type) × [ebx+0x14]∈{0,0xff} (**Feedback Phase**)
`(1,0)→m1  (1,0xff)→m4  (2,0)→m2  (2,0xff)→m3  else→m0`.  So Feedback Phase swaps m1↔m4 / m2↔m3;
m0 is a fallback (likely unused). The 4 main modes are RBJ-family variants of the (P±1)C ± A38·S form.

### Mode 0 (`0x51EFB7`, fully decoded) — resonant biquad
```
norm = 1/(1 + A30/P)
b = [ (1+A30·P)·norm , -2C·norm , (1-A30·P)·norm ]      a = [1, -2C·norm·…, (1-A30/P)·norm]
```
### Mode 1 (`0x51EC9C`, fully decoded)
```
a0   = (P-1)·C + (P+1) + A38·S ;  norm = 1/a0
c1   = -2·norm·[ (P+1)·C + (P-1) ]
c2   = norm·[ (P-1)·C + (P+1) - A38·S ]            ; (P+1±A38·S form, P±1 on the C terms)
…3 coefs ×2^20 → Q20 int [ebx+0x20/24/28]
```
Modes 2/3/4 (`0x51EDAE / 0x51EEC0 / 0x51EF3C`) = the same family with sign permutations on the C / A38·S terms.

## ⛔ IMPLEMENTATION BLOCKER (why this is NOT ported to the clone)
The **per-sample process loop is NOT in the dump** (`vaz_fx_all.c` has the SETUP/init only; the hot
DSP is a separate fixed-point routine). So the *topology* — how the 3 Q20 coefs `[ebx+0x20/24/28]`
combine per sample (makeup gain? feedback comb? subtraction notch?) — is unknown.

**Python check (tools, 2026-06-09):** modelling the decoded coefs as a naive biquad cascade
(4 harmonic sections) is STABLE (pole r≈0.999) but gives **−46 to −83 dB insertion loss** at P=0.2
(−6..−12 dB at P=0.8) — a real flanger is ~unity-gain with notches. So the decoded coefs are
applied in a *different* topology (makeup/feedback), which is not recoverable from the available code.

→ **Decision:** keep the current working flanger (allpass comb — RE-confirmed correct family: 4 RBJ
sections, α=0.5, geometric `f=20·exp(0.027·X)` freq). Do NOT ship the reconstruction (would make it
near-silent). The freq-mapping fix (commit fd411fe) stands. A faithful port needs the fixed-point
per-sample routine (not in this dump) or real-VAZ effect A/B (not available — effects can't be
headless-rendered like the synth).

## Remaining (blocked)
- The 5 modes' per-sample topology (the makeup-gain/feedback structure) — un-dumped fixed-point.
- Exact per-section freqs (patch-stream filler, not cleanly decompilable).
