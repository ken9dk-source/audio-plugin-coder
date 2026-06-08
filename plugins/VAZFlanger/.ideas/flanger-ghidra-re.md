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

## Remaining (deeper, risky without effect-level A/B)
- The 5 biquad MODES (LP/HP/notch/AP/shelf) selected by Feedback Phase — clone uses allpass+sign only.
- Exact per-section freqs (come from a patch-stream filler not cleanly decompilable).
