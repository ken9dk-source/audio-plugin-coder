# VAZClone — DSP differences vs original VAZ 2010

Original DSP reverse-engineered from `Vaz2010Core.dll` (Ghidra: `tools/vaz_big.c`, `vaz_osc.c`,
`vaz_coef.c`) + FFT of real renders + the official manual. Clone DSP = `Source/Synth.h`,
`SynthVoice.h`, `PluginProcessor.cpp`.

Legend: ✅ faithful · 🟡 approximate · ❌ different/missing

## Fundamental
| | Original | Clone | |
|--|----------|-------|---|
| Numeric format | **fixed-point Q-format** (integer) | 32/64-bit **float** | ❌ → bit-exact impossible |
| Coefficient tables | runtime-computed 10-bit lookup (indirect addressing) | closed-form float formulas | 🟡 |
| Oversample | 2× (string "Oversample x2 below 50kHz") | partial (Type K/R internal 2×) | 🟡 |

## Oscillators
| Feature | Original | Clone | |
|---------|----------|-------|---|
| Engine | mip-mapped wavetable, 32-bit phase, 4-pt cubic | same (512-pt tables, Catmull-Rom) | ✅ |
| Multi-Saw | 4 saws, ±36c / 24c spacing | FFT-matched ±36.25c | ✅ |
| Saw/square spectra | 1/n + gentle HF tilt, alias floor ≈ −42 dB | PolyBLEP-ish + tilt | 🟡 |
| Pulse | band-limited variable width | saw−saw difference | ✅ |
| OSC2 hard Sync | phase reset on master cycle | `osc2.hardReset()` | ✅ |
| Ring Mod | Osc1×Osc2 | ✅ | ✅ |
| Sample mode | loadable multisample/wavetable | sine only (no loader) | ❌ |
| FM | exponential, 2-3 inputs/osc | exponential, 1 input/osc | 🟡 |

## Filter (6 engines decoded)
| Type | Original | Clone | |
|------|----------|-------|---|
| A | 2-pole multimode | TPT-SVF LP/HP/BP | ✅ |
| B | 2-pole + Bandwidth | SVF, aux→Q | ✅ |
| C | resonant LP 2P/4P + 1-pole HP + Separation | cascaded SVF + sep + HP | 🟡 (extra soft-clip) |
| D | state-variable + soft-clip | Chamberlin SVF + clip | ✅ |
| K | **2-pole Sallen-Key**, dirty self-osc | 2-pole + cubic feedback | ✅ |
| R | 4-pole integrator cascade + cubic + 1-pole HP | `VAZLadder` ladder+cubic | ✅ |
| Comb | tuned delay + Damping + I/O Mix | delay + Damping (no I/O-mix) | 🟡 |
- **Per-voice** filter ✅ (correct osc→filter→amp order). Cutoff pole coefficient = exact.
- Resonance amount, cutoff→Hz map, exact cubic constant = **float approximations** (fixed-point tables unrecoverable).
- "Slew-limit on cutoff" (modes A–C) deliberately omitted (user wants instant filter DONK).

## Envelopes / LFOs
| | Original | Clone | |
|--|----------|-------|---|
| Env type | ADSR, 16-bit times (≤425), Multi/Reset/Cycle/Curve | ADSR + Reset/Cycle/Curve | 🟡 (Multi gap; curve approx) |
| Env2 self-mod (Dest A/D/R) | yes | ✅ | ✅ |
| LFO waveforms | 8 (morph + Delay + S&H) | ✅ 8 | ✅ |
| LFO sync periods | 24 (1/32T … 256 beats) | ✅ 24 | ✅ |
| LFO Trig | yes | ✅ | ✅ |

## Amplifier / routing
| | Original | Clone | |
|--|----------|-------|---|
| Amp-Mod slot1 = master volume | depth slider = volume | **`amp_level` (now wired)** | ✅ (was the amplitude bug — FIXED) |
| Amp-Mod slot2 = fractional tremolo | yes | `amp_mod_src/amt` (bipolar) | ✅ |
| Pan Mod (stereo, Voice#) | yes | ✅ | ✅ |
| Overdrive | 48 dB + soft-clip | tanh, 48 dB | ✅ |
| Mod matrix | 22 sources × bipolar {src,sign,depth} | 16/22 wired; ± sign on 5 main slots | 🟡 |
| Mod Amps 1/2 (+SQ) · Lag | yes | ✅ | ✅ |

## Voicing
| | Original | Clone | |
|--|----------|-------|---|
| Mono/Poly/Unison + High/Low/Last/Duo | yes | ✅ (Unison fixed via VAZSynth) | ✅ |
| UniV count | 1–32 | fixed 4 | ❌ |
| Voices | up to 32 + Dynamic | fixed 16 | 🟡 |
| Pitch-bend range | 1–24 st | fixed ±2 st | 🟡 |
| Detune | separate Poly/Unison | one shared (Unison min 5c baseline) | 🟡 |

## NET
Sound architecture (osc · 6-engine per-voice filter · env · 8-wave LFO · mod matrix · ring-mod ·
amp/pan/overdrive · arp) is **faithful in character**. Hard differences: fixed-point vs float
(no bit-exactness), Sample-osc, UniV/32-voice/bend-range ranges, a few mod-source stubs.
