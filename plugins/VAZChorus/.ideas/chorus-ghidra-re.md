# VAZ Chorus — Ghidra RE (Vaz2010Core.dll)

Real module **`TFXChorus`** (RTTI class-name @`0x5184D4`, found via `tools/find-fx-classes.py`).
Per-sample DSP @`0x518AD8`. **No clone exists yet — this RE defines a faithful VAZChorus.**

## It's a dual-LFO ENSEMBLE chorus
A single delay line read by **two LFOs**, each tapping **three voices at 0° / ±120°**, summed → a
thick 6-tap ensemble (string-machine style).

| Element | Address | Detail |
|---|---|---|
| LFO 1 | `0x518AEE` | phase1 `[+0x284]` += inc1 `[+0x288]` |
| LFO 2 | `0x518B00` | phase2 `[+0x28c]` += inc2 `[+0x290]` (different rate → beating) |
| depth·level | `0x518B12` | `ebp = depth[+0x26c] · level[+0x298]` |
| Delay line | `[ebx + (phase>>24)·4 + 0x2a8]` | 256-entry circular buffer; **read index = LFO phase>>24**, fractional part = `(phase<<8)>>1` → **linear interpolation** (`s0 + frac·(s1−s0)`) |
| 3 voices | `edi += 0x55` (×2) | taps at +0°, +120°, +240° of the buffer (0x55 = 85/256 ≈ ⅓) |
| Combine | `0x518C56` | LFO 2 repeats the 3-voice read and **adds** to the same 3 accumulators `[esp+0xc/10/14]` |

## Modulation modes  (`[+0x264]` for LFO1, `[+0x270]` for LFO2)
The mode picks the LFO **waveform** that offsets the read phase (each at 0°/±120° via `esi ± 0x55555554`):
- **mode 0** (`0x518B29`): table lookup `[+0x2a8]` (precomputed waveform — sine-ish)
- **mode 1** (`0x518BA4`): **clamped triangle / trapezoid** — `clamp(|phase|−2^29, 0, 2^30)·depth`
- **mode 2** (`0x518C1F`): **triangle** — `(|phase|/2)·depth`

## → Faithful VAZChorus design (to build, scaffold from VAZFlanger)
- One circular delay line (~10–20 ms); **2 LFOs** at slightly different rates (Rate + a fixed detune).
- Each LFO modulates **3 read taps at 0°/±120°**, linear-interpolated → 6 taps summed.
- Params: **Rate, Depth, (Delay/)Base, Mix, Gain**, + a **Mode/Waveform** choice (sine / triangle),
  + optional Sync (host BPM) like the other FX.
- Output = dry + mix·(ensembleSum). Triangle/sine LFO per the mode.
- Topology-exact (float vs VAZ fixed-point Q-format → not bit-exact).
