# VAZ Phaser — Ghidra RE (Vaz2010Core.dll)

Found the real module by RTTI class-name scan (`tools/find-fx-classes.py`): **`TFXPhaser`**,
class-name string @`0x521078` (`\x09 "TFXPhaser"`). VMT method pointers around it →
per-sample DSP @`0x5218D8`. (An earlier "phaser RE" pointed at `0x51A5FC`, which is in the
Chorus/Delay region — wrong module, like the flanger-was-the-equalizer mix-up.)

## TFXPhaser per-sample DSP @`0x5218D8` — classic all-pass phaser
| Stage | Address | What it does |
|---|---|---|
| Triangle LFO | `0x521902-05` | `cdq; xor eax,edx; sub eax,edx` = **abs(phase accumulator)** → unipolar triangle (NOT sine). phase += inc `[+0x294]` |
| Sweep → coef | `0x52190A-1F` | `idx = (base[+0x264]<<15 + |phase|·depth[+0x280]) >> 15`; `g = coefTable[ebx + idx·4 + 0x310]` (precomputed freq→all-pass-coef LUT) |
| Feedback in | `0x521938-49` | `x = in·inGain[+0x298] + feedback[+0x29c]·fbState[+0x2a4]` |
| Stage loop | `0x521951-76` | for `stages = [+0x2a0]`: `y = apState[k] − g·x ; apState[k] = x + g·y ; x = y`  (1st-order all-pass) |
| Feedback store | `0x521978` | `fbState = lastStageOutput` |
| Dry/wet | `0x521982-9B` | `out = in + mix[+0x288]·(cascadeOut − in)` |
| Stereo | (2nd pass) | re-runs with an L/R phase offset on the LFO |

## Clone status — structure was ALREADY correct, only the LFO shape was wrong
The clone's `APStage` (`y = −a·x + z ; z = x + a·y`), the N-stage cascade, the feedback-before-cascade,
and the dry/wet are all **identical** to the real. The only mismatch was the **LFO: clone used a
bipolar sine, the real uses a unipolar triangle** (`abs(phase)`, sweeps UP from a base).

### Fixed (2026-06-09, PluginProcessor.cpp)
- LFO `std::sin(2π·phase)` → **unipolar triangle** `ph<0.5 ? 2ph : 2−2ph` (0→1→0).
- Sweep is now **upward from a floor** (matching `idx = base + |phase|·depth`):
  `fcBase = 80·25^Freq` (80 Hz..2 kHz floor), `depthOct = Depth·2.5` (0..+2.5 oct).
- All-pass coef: clone computes `g = (1−tan)/(1+tan)` (standard bilinear) where the real does a LUT;
  same 1st-order all-pass family, so left computed (the LUT is just an optimisation of the same curve).

**Verified in Python** (`tools`): bounded (peak ≤1.36 at fb=±0.72), unity-ish gain, all-finite,
6-stage static phaser shows exactly **3 notches** (N/2). Stable; topology-exact (not bit-exact).
