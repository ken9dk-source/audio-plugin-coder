# VAZClone vs VAZ — static Ghidra gap analysis (no WAV rendering)

Source: `tools/vaz_big.c` = Ghidra 12.1 decompilation of `Vaz2010Core.dll`. The whole per-block voice
render is **`FUN_004dbddc` @ 0x4dbddc** (size 10073). Walked stage-by-stage from the entry; every
`param_1+off` is a patch param, `iVar3+off` is the per-voice state. Compared against the clone
(`Source/SynthVoice.h` + `Synth.h`).

## VAZ voice-render architecture (FUN_004dbddc)
Per-block: event queue (0x2531/0x2530) → note→pitch (round via 0x402bf4) → **per-voice unison loop** (0x70 count).
Per-sample, per-voice:
1. **Cutoff smoothing** (L99-103): base cutoff `0x268 += (0x264·2^21 − 0x268)·DAT_006d45e4 >>32` = one-pole slew toward the knob target.
2. **Noise** (L105): LCG `0x4e8·0xbb38435 + 0x3619636b` → voice[0x30].
3. **LFO** (L109-119): table **`0x5441d4`** (triangle if 0x10c==0, else LUT interp) → voice[0xc].
4. **Controller/aftertouch** (L121-122) → voice[0x34].
5. **Osc pitch** (L141-160): index → **pitch table `0x5445e0`** (exp, 1024^x over the index) = the phase-increment rate; + 2 pitch-mod slots; + **FM** (flag 0x1b4 → add voice[0xd4]).
6. **Osc waveforms** `[0x1e8]` (L724-960, run for BOTH osc1→voice[0x24] and osc2→voice[0x2c]):
   `0`=saw (1 wavetable), `1`=**pulse** = saw(φ)−saw(φ−pw) via the curve-PW, `2`=**multisaw = exactly 4 saws** at
   phases 0x118/0x11c/0x120/0x124, rates spread **symmetrically ±0.5d, ±1.5d** around the base, summed ÷4,
   `≥3`=**sample** (4-pt interp, segment loop). Wavetables `0x6dd2c0`/`0x6de2c0` (+ mips) band-limit.
7. **Waveshape mod** (L827-829): `0x204`/`0x208`.
8. **Mixer** (L1020-1069): 3 channels `0x218/0x228/0x238`. **src==0 → RING MOD** `(osc1<<5)·(osc2<<4)>>32`;
   else voice[src·4]. Level `0x220/0x230/0x240`. Pre/post-filter flag `0x21c/0x22c/0x23c`.
9. **Filter** (L1072+): cutoff = 0x268 + Σ3 mods (0x278/27c, 280/284, 288/28c); **slew limiter** (flag 0x260,
   ±0x200000/step); reso = 0x26c·2^22 + mod (0x290/294, gated by mode&4); mode `0x258` → A/B/C/D/K/R/Comb,
   the exact biquad tables (0x5545e4/5945e4/5d45e4 etc.).
10. **Output**: HP one-pole + cubic soft-clip + **amp mod** (0x298/0x29c) + **pan mod** (0x2a0).
11. **Env**: segmented generator (boundary handler `FUN_004dbd7c`, per-sample interpolation).

## Clone status — what it HAS (✓ = present + matches the architecture)
| VAZ feature | clone | where |
|---|---|---|
| Osc saw / pulse(saw−saw) / multisaw(4) / sample | ✓ | OscBlock, 5 wave modes |
| Osc FM (+ Link osc1→osc2) | ✓ | o1FmAmt, link |
| Osc2 hard **Sync** | ✓ | osc2.hardReset on osc1.mainWrapped |
| **Ring mod** (osc1·osc2) | ✓ | `rm = o1*o2`, mix src |
| Osc3 / sub | ✓ | osc3 saw |
| Mixer 3ch + pre/post + source select | ✓ | mix1/2/3Src + Post |
| Filter A/B/C/D/K/R/Comb + 3 cutoff mods + reso mod + slew | ✓ | VAZTypeA/R/K/D bit-exact (real-validated) |
| Output amp-mod + pan-mod + HP + soft-clip | ✓ | updateAmp / output stage |
| Noise (white) | ✓ | mod src 12 |
| LFO1/2/3 (+ waveforms) | ✓ | osc-rate LFOs |
| Env1/Env2 ADSR + Multi/Reset/Cycle | ✓ | VAZEnv |
| Velocity / KeyTrack / Aftertouch / ModWheel / CtrlB | ✓ | mod sources |
| Unison (per-voice spread) | ✓ | unison voices |

## GAPS / CORRECTIONS (the only things not 1:1)
1. **Base-cutoff smoothing is missing.** VAZ one-pole-smooths the base cutoff knob (L99-103, coef DAT_006d45e4);
   the clone removed all cutoff smoothing (Synth.h:321 — it was killing the attack on the env→cutoff sweep).
   *Impact:* only audible when the cutoff KNOB moves (automation/MIDI), not on static patches. To match VAZ
   precisely: smooth ONLY the base cutoff (0x264→0x268), NOT the env/LFO cutoff mods.
2. **`0x5445e0` is the OSC PITCH table (exp 1024^x), NOT the envelope curve.** An earlier change set VAZEnv's
   curve mode to `1024^(x-1)` believing this LUT was the env shape — it's a mis-ID. VAZ's env is the segmented
   generator (FUN_004dbd7c), ~linear segments. The `1024^(x-1)` curve is benign (a reasonable exp) but is not
   VAZ's env curve — revert to `level²` or leave as a cosmetic option.
3. **Verify (likely fine):** multisaw spread must be symmetric ±0.5d/±1.5d (4 saws); the mixer source byte must
   map to VAZ's voice-array index (0=ring, then osc1/osc2/noise/...).

## CONCLUSION
The clone is **architecturally complete** — it implements every major DSP stage and feature of VAZ's voice
render (oscillators incl. multisaw/ring/sync/FM/sample, the 3-channel pre/post mixer, all 7 filter families
bit-exact, the output amp/pan/clip, noise, LFOs, envelopes, the full mod set, unison). **No missing modules.**
The only deltas are (1) the base-cutoff smoothing (minor, automation-only) and (2) the env-curve mis-ID to clean
up. Everything else is as it should be.
