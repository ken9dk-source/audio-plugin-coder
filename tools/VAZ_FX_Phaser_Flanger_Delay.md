# VAZ 2010 — Phaser / Flanger / Delay : full RE spec for precise cloning

**Purpose:** everything needed to build faithful standalone VAZ Phaser, Flanger and Delay plugins later.
**Sources (all in `tools/`):**
- Manual: `chm_out/Using VAZ 2010/Effects Windows/VAZ {Phaser,Flanger,Delay}.htm` (authoritative topology + params).
- GUI bitmaps: `chm_out/.../VAZ{Phaser,Flanger,Delay}.gif` (exact control set + default value displays).
- Ghidra: `vaz_fx_all.c` — **253 functions + 156 float constants** dumped from Vaz2010Core.dll region 0x510000–0x524000 (`ghidra_scripts/DumpFXAll.java`, `run-ghidra-fxall.bat`). DSP lives in **Core.dll**; VAZ2010Effect.dll is a thin Delphi VST wrapper (its strings are pure RTL).
- Param-range strings: `vaz_skin_strings.txt` / `vaz_core_strings.txt`.

**Hard limit (same as the synth filter/osc):** the per-sample loops are **fixed-point Q-format integer** — only the *setup/coefficient* functions decompile as float. So exact bit-level cloning is impossible; the recipe below = correct **topology + parameter ranges + the Ghidra-confirmed constants**, which is "as precise as possible". Implement with standard float DSP using these numbers.

**Decompile noise to ignore:** the recurring PUSH constants `-4.367598, -1050.6976, -221.34904, 4.430098` appear at the top of nearly every Core.dll function = Delphi stack-frame/exception magic, **NOT DSP**.

---

## 0. Shared infrastructure (used by Phaser, Flanger, Delay, Chorus, Autopan)

### LFO (Phaser/Flanger/Autopan/Chorus)
- Region ~0x51a6xx. Phase is a **32-bit fixed-point accumulator**; top 8 bits (`>>0x18`) index a wave table (same scheme as the oscillators). Scales seen: `8388608 = 2^23`, `3.3554432e7 = 2^25`, `6.7108864e7 = 2^26`, `2.6843546e8 = 2^28`, `1.07374184e8 ≈ 2^30/10`.
- Rate constants: **20, 80, 200** (Hz-range scalers) + **0.25** → free Rate ≈ **0.1–20 Hz** (matches Autopan/Phaser docs).
- Recurring LFO shape coefficients **−0.241** and **0.5476425** (in every LFO setup: Chorus 0x519054/60, Delay-mod, Flanger 0x51e954, Autopan 0x52089c/521cf0) → a parabolic/triangle→sine shaping pair. Treat as: a triangle phase fed through `y = a·x² + b·x` shaping (a≈−0.241, b≈0.5476) for the smooth sweep. `L/R Phase` = an offset added to the right channel's phase accumulator before table lookup.

### Tempo / Sync (note → samples)
- `FUN_004a0a68` = note-length → samples converter (host BPM). Sync constants: **60** (60/BPM seconds, @0x51bba4/0x51c468/0x51c518/0x51c97c), **1000** (ms, @0x51c074), sample-rate **11025** and **44100** show up in setups.
- Period popup (Phaser/Flanger "4Bt" + Flanger/Delay note value) uses the synth's existing 24-entry note table (1/32T…256 beats — already in the clone as `lfoPeriods`).

---

## 1. VAZ PHASER  (Insert "VAZ Phaser", region ~0x51a5e0–0x51ab00)

**Topology (manual):** input → **N cascaded all-pass filters** (each a very short frequency-dependent delay) → feedback path (fraction of output back to input, with **polarity ±**) → wet, then dry/wet mix → output gain. The all-pass centre frequency is swept by the LFO (the classic phaser sweep).

**Controls + ranges (GIF + skin):**
| Control | Range / display | Notes |
|---|---|---|
| **Stages** | default **(4)** | number of all-pass filters; more = deeper notches |
| **Frequency** | Min…Max, **0.1 ms … 120 ms** (`0.1ms|120ms`) | all-pass delay → comb-notch frequency |
| **Feedback** | Min…Max | + **Phase ± toggle** (`Feedback Phase` — positive/negative feedback) |
| **Modulation: Rate** | LFO ~0.1–20 Hz | ignored when **Sync** on |
| **Sync** + **Period** | note/beats ("4Bt"…) | replaces Rate |
| **Depth** | 0…Max | LFO sweep amount |
| **L/R Phase** | 0…Max | stereo LFO offset |
| **Mix** | Dry…Wet, default **50:50** | |
| **Gain** | default **−3 dB** | output trim (needed at high feedback) |

**Ghidra:** parameter setters `FUN_0051a5fc/a6bc/a708/a7e8/a8bc` store the GUI value at struct `+0x260/+0x264/+0x268/+0x26c/+0x270` and precompute coefficients at `+0x274…+0x28c` via sin/cos helpers `FUN_0042c200/0042c220`. Param values are normalised `/255`.

**Recipe:** per channel, chain `Stages` one-pole all-passes `y = -g·x + d` where the all-pass coefficient `g` (or the delay) is set by `Frequency` and modulated by `LFO·Depth`; sum feedback `±Feedback·lastOut` into the input; `out = dry·(1-Mix) + wet·Mix`, then `·10^(Gain/20)`.

---

## 2. VAZ FLANGER  (region ~0x51e9xx–0x51f5f8)

**Topology (manual):** input → **single short modulated delay line** + feedback (±) → comb filtering (metallic); LFO sweeps the delay time. (= Phaser but a real delay line instead of all-pass stages.)

**Ghidra-confirmed:** **max delay line = 5080 samples ≈ 115 ms @44.1 kHz** (const `0x51f094 = 5080.0`, used @0x51eb9a). Fixed-point delay scale `2^20 = 1048576` (0x51f0e8). Per-section coefficient setup `FUN_0051eb34` computes **5 coefficients** (struct `+0x18/+0x1c/+0x20/+0x24/+0x28`) from a mode index 0–4 (derived from `+8` and `+0x14∈{0,0xff}` = the feedback-phase/polarity); `FUN_0051f430` initialises **4** internal sections (`do…while !=4`). LFO shape coef −0.241/0.5476425 (0x51e954/…).

**Controls + ranges (GIF):**
| Control | Range / display | Notes |
|---|---|---|
| **Delay Time** | Min…Max (short; line max ≈115 ms) | comb spacing |
| **Feedback** | Min…Max + **Phase ± toggle** | intensity; clipping risk high |
| **Rate / Sync+Period / Depth / L/R Phase** | as Phaser | LFO sweep |
| **Mix** | Dry…Wet, default **50:50** | |
| **Gain** | default **−3 dB** | output trim |

**Recipe:** fractional-delay line (linear/cubic interp), length = `DelayTime + LFO·Depth` samples (≤5080); `out = dry + delayed`; feedback `delayIn = in ± Feedback·delayed`; mix + gain as Phaser.

---

## 3. VAZ DELAY  (region ~0x51b9b0–0x51d6d0)

**Topology (manual):** **two delay lines (L/R)**, 3 routing modes:
- **Stereo** — independent L and R delays.
- **Ping-Pong** — feedback crosses to the opposite channel.
- **Double** — the two delays in series (mono, complex patterns).

**Ghidra-confirmed:** L/R are symmetric. Per-channel struct fields: write phase `+0x2d8` (fixed-point, `>>0x18` = integer sample index, increment `*0x800000`=2^23/sample-ish), **L length `+0x2cc`, R length `+0x2d0`, max-buffer length `+0x2dc`** (delay clamped to it). `FUN_0051b9b0` = per-block time update: when delay/feedback/tone changed it re-derives length via the tempo converter `FUN_004a0a68`, with a smooth cross-fade ramp (`+0x308/+0x30c/+0x310/+0x314`) to avoid clicks on delay-time change. Setters run under an `EnterCriticalSection` (`FUN_0051c090`). Constants: **1000** (ms, 0x51c074), **60** (BPM, 0x51c468/518), **512** (max seconds-ish / 0x51c398/784), 256, 4, sample-rate 44100.

**Controls + ranges (GIF + skin):**
| Control | Range / display | Notes |
|---|---|---|
| **Mode** | Stereo / Ping-Pong / Double | |
| **Link** | toggle | L/R controls move together |
| **Sync** | toggle | tempo follow |
| **Delay** (per L/R) | **5 ms … 6 s** (`5ms|6s`), default **500 ms**; Sync on → note value (def **1/4**) + slider **50 %…200 %** of base | |
| **Feedback** (per L/R) | Min…Max | multiple repeats |
| **Tone** (per L/R) | **Dark…Bright** | one-pole **low-pass in the feedback path** |
| **Wet Level** (per L/R) | default **−6 dB** | |
| **Dry Level** (per L/R) | default **0 dB** | |

**Recipe:** two delay buffers sized for 6 s (≈264 600 samp @44.1 k). Read `delayed = buf[w - len]` (interp); `fbIn = in + Feedback·tone_LP(delayed)`; route fbIn back to **same** ch (Stereo), **opposite** ch (Ping-Pong), or **next** delay (Double). `out = in·Dry + delayed·Wet`. Tone LP: `z += (x - z)·tone` (tone 0=Dark→small coef, 1=Bright→~1). Smooth delay-length changes with a cross-fade (VAZ does — `+0x308…`).

---

## 4. Build notes
- All three are **stereo in/out**, share the LFO + tempo-sync infra (build once, reuse). Period selector = the synth's 24-note table.
- Suggested build order (easiest→hardest): **Delay → Flanger → Phaser** (Delay is the most standard; Phaser's swept all-pass chain is the fiddliest).
- Reuse the VAZReverb plugin scaffold (`plugins/VAZReverb/`) — same APC WebView editor pattern; GUI bitmaps/layout from the `.gif`s.
- Raw decompile for reference: `tools/vaz_fx_all.c` (constant pool at top, then all 253 functions). Re-generate with `run-ghidra-fxall.bat`.

---

## 5. DEEP DISASSEMBLY PASS 2026-06-08 (user: "den FLANGER er slet ikke tæt på identisk"; x87 + fixed-point read via `py disasm-region.py <addr> <len>`)

> **⚠️ CORRECTION 2026-06-15 — THIS SECTION MIS-IDENTIFIED THE FLANGER. IGNORE IT FOR THE FLANGER.**
> Ghidra cross-check (`vaz_fx_all.c`): `FUN_0051eb34` (×4 via `FUN_0051f430`, 5 modes by fields +8/+0x14,
> 5 coefs via cos/sin/Power) sits between `TFXEqualizer @0x51E0C0` and `TFXFlanger @0x51FCD4` → it is the
> **EQUALIZER** (4 bands × 5 modes = peak / low-shelf / high-shelf / low-pass / high-pass; built as
> `plugins/VAZEqualizer`). The **real flanger** is `FUN_00520418 @0x520418` (fixed-point LFO phase-accum 2^23
> + tempo Sync `FUN_004a0a68` + delay helper `FUN_004c3ad0`) = a **delay-line**, which `plugins/VAZFlanger`
> already implements (see its `PluginProcessor.h` header, 2026-06-09). Do NOT rebuild the flanger as a biquad comb.

**Why the simple-delay-line flanger clone is wrong — the REAL flanger (FUN_0051eb34 @ 0x51eb34, per-section coef setup, called ×4 by FUN_0051f430):**
- **NOT a plain delay line.** It builds a **biquad per section** RBJ-style: computes `ω`, `cos(ω)` (`call 0x402bc0`), `sin(ω)` (`call 0x402b98`), an `alpha`/Q term, and **5 coefficients** (`+0x18/+0x1c/+0x20/+0x24/+0x28`). **4 cascaded sections.**
- **Notch freq comes from Delay Time**: `[ebx+0xc] ×2 ×9 = ×18`, `/5080.0` (max-line const @0x51f094), then `Power()` (`0x42c220` = `base^exp`, confirmed) → **logarithmic** delay→freq. Normalised freq **clamped to 0.45** (≈Nyquist) @0x51f0a8.
- **Filter MODE (1–4) selected by Feedback Phase** (`[ebx+0x14] ∈ {0,0xff}`) × the `[ebx+8]` field (1/2) → 5-way jump table, each mode a different biquad type. So "Phase ±" **changes the filter type**, not just polarity.
- Mod term: `[ebx+0x10] ×0.027 ×20 / sampleRate` (**0.027** depth const SHARED with Chorus/Autopan/Compressor).
- GUI readouts are separate formatter funcs (e.g. 0x51e868: `×0.027×20` then idiv 1000/100/10 = digit extraction). **DumpFXAll's `f32=−0.241/0.5476425` "LFO coef" notes were WRONG** — those are 80-bit `fld`s (real value 0.027); ignore them in §0.

**Phaser per-sample loop (FUN_0051a3f0 @ 0x51a3f0) + all FX inner loops = FIXED-POINT Q32** integer math (`(longlong)a*b >>0x20`) driven by **baked ROM lookup tables** (`DAT_006f8250/006f824c`, `DAT_006f8654/006f8650` = precomputed log2/exp2 + interp curves; CLZ `for(;x>>i==0;i--)` = fast log2).

**CONCLUSION (firsthand-confirmed):** the inner DSP is **fixed-point + ROM curve tables → not clone-able bit-exact OR precisely structure-for-structure from the binary** (the float algorithm is gone; only integer ops + table indices remain). Ghidra gives topology + ranges + a few consts and no more. **Reliable route to "identical" = measurement-matching** (like the oscillators: `tools/measure-osc.py`, `tools/vaz_ref/*.wav`): render the real VAZ Flanger on a known test signal → analyse notch spacing / LFO rate / feedback / delay range → tune the clone. Stop mining the binary for the inner loop.
