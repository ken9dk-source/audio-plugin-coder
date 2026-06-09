# VAZ Flanger — Ghidra RE (Vaz2010Core.dll)

## ⚠️ CORRECTION (2026-06-09)
An earlier pass decoded the region around `0x51E868`–`0x51F0xx` as "the flanger" — that is
actually **`TFXEqualizer`** (RTTI class-name string @`0x51E0C0`). Same RBJ-biquad coef shape,
wrong module. That's why the reconstruction gave −46 dB insertion loss: it's an EQ with shelf/
peak bands, not a flanger. The **real flanger is `TFXFlanger`** (class-name @`0x51FCD4`).

Found via RTTI class-name scan — `tools/find-fx-classes.py` (Borland Pascal strings):
`TFXFlanger 0x51FCD8 · TFXChorus 0x5184D8 · TFXPhaser 0x52107C · TFXEqualizer 0x51E0C0 ·
TFXDelay 0x51AF18 · TFXReverb 0x522530`.  VMT + methods via `tools/find-callers.py`
(note: VAZ is Borland C++ Builder → code section is `CODE`, not `.text`).

## TFXFlanger DSP — per-sample loop @`0x5204F4`  (the real thing)
A **textbook delay-line flanger** (fixed-point Q23 inner loop):

| Stage | Address | What it does |
|---|---|---|
| Triangle LFO | `0x520531-34` | `cdq; xor eax,edx; sub eax,edx` = **abs(32-bit phase accumulator)** → triangle (NOT sine). Phase += inc `[+0x290]`. |
| Delay amount | `0x520555-5E` | `(triangle·depth[+0x2a0]) >> 17  +  baseDelay[+0x29c]  +  writePos[+0x298]` |
| Fractional read | `0x52056A-7F` | `s0=buf[i]; s1=buf[i+1]; delayed = s0 + frac·(s1−s0)` — **linear interpolation** |
| Feedback comb | `0x520581-A9` | `buf[writePos] = in·inGain[+0x294] + feedback[+0x264]·delayed` |
| Dry/wet | `0x5205B5-C2` | `out = in + mix[+0x284]·(delayed − in)` = (1−mix)·in + mix·delayed |
| Stereo | `0x5205DB+` | second channel re-runs the LFO with an **L/R phase offset** `[+0x280]` |

### Per-block setup @`0x520418` (LFO rate, incl. BPM sync)
```
sync ([+0x270]≠0):  cycleSamples = (sr·60·period[+0x274]) / (BPM[+0x2b0]·48)
                    inc[+0x290]  = round( 2^31 / cycleSamples )          ; @0x5204BB ×2147483647
free:               inc from the Rate knob
```
Buffer = circular, power-of-2 (mask `[+0x2ac]`), entries are 8-byte (stereo/64-bit) slots.

## ✅ Implemented in the clone (PluginProcessor.h / .cpp, 2026-06-09)
Rewrote `FlangerChannel` from the (wrong) 4-biquad allpass comb to the **real delay-line flanger**:
- circular buffer (≤60 ms), **triangle LFO** (`abs`, not sine), **linear-interp** fractional delay,
  **feedback comb** `buf[w]=in+fb·delayed`, **dry/wet** `out=in+mix·(delayed−in)`, L/R phase, BPM sync.
- Delay Time → 0.5..5.5 ms min delay; Depth → sweep above it; Feedback ±0.92 (Phase = sign).
- Reverted the EQ-based `20·exp(0.027·X)` freq map (that was the equalizer's band frequency).

**Verified in Python** (`tools`): bounded output (peak ≤1.15 even at fb=±0.92), unity-ish gain,
comb notches present, all-finite. Stable, topology-exact (float vs VAZ's fixed-point → not bit-exact).
