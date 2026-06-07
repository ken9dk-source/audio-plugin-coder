# VAZClone — MISSING FEATURES

Generated from a full audit of `C:\Program Files (x86)\Steinberg\Vstplugins\VAZ Synths\VAZ 2010`
(official CHM manual + Ghidra decompile of `Vaz2010Core.dll` + 601 factory `.v2p`/`.vzp` presets).
Reflects the **current** clone state (osc · 6-engine filter · 8-wave LFOs · ring-mod · mod-matrix ·
arpeggiator · `.v2p` load/save are all already implemented).

Legend: ✅ done · 🟡 partial · ❌ missing

---

## 1. Original install inventory (reference)
| Component | File | Role |
|-----------|------|------|
| Engine | `Vaz2010Core.dll` (1.8 MB, exports `CreateVAZ2010VST`/`Standalone`) | all DSP + GUI logic |
| Skin | `Vaz2010Skin.dll` (623 KB) | bitmap GUI resources |
| VST wrapper | `VAZ2010VSTi.dll` (exports `main`) | thin VST2 shim → Core |
| FX plugin | `VAZ2010Effect.dll` | the bundled effects as a VST |
| Audio I/O | `VazAudioAsio/DS/MME.dll`, `OpenAsio.dll` | standalone audio backends |
| Standalone | `Vaz2010.exe` | host app |
| Docs | `VAZ2010.chm` | official manual (extracted → `tools/chm_out`) |
| Presets | 260 `.v2p` + 341 `.vzp` + 5 `.v2b` banks + 3 `.vzs` | factory sounds |
| Tunings | 4 `.tun` (equal, cmajjust, quart, revequal) | microtuning scales |
| Samples | `xxvoice.wav`, `vcdo.wav`, `wavetbl2.wav` | Sample-oscillator data |

---

## 2. Missing DSP components
| # | Feature | VAZ behaviour (source) | Clone | Notes |
|---|---------|------------------------|-------|-------|
| D1 | **Per-voice filter** | filter is inside each voice (FUN_004dbddc runs per voice) | ❌ global | **Biggest accuracy gap** — filter envelope/keytrack don't retrigger per note in chords/legato |
| D2 | **Pan Modulation** (stereo) | per-voice pan, Voice# → stereo spread | ❌ | needs stereo per-voice rendering (clone sums mono) |
| D3 | **Sample oscillator** | Sample mode loads multisample/wavetable (`wavetbl2.wav`) | 🟡 sine only | Sample Loader dialog + table playback |
| D4 | **Osc 3** | selecting Osc3 in mixer switches LFO1 to audio-rate, tracks keyboard (footage 32'=48…2'=240) | ❌ | mixer src option exists, produces silence |
| D5 | **External Input** | Ext osc waveform + mixer source; ring-mod with Osc2 | ❌ | |
| D6 | **Sequencer** | step seq: Swing, Timebase, Tempo/Local, Free, Performance-2 | ❌ | unlocks Seq A/B + Accent mod sources |
| D7 | **Microtuning** | `.tun` scale files | ❌ | 4 scales shipped |
| D8 | **Comb Damping + I/O Mix** | aux sliders = feedback-LP damping + input/output blend | 🟡 feedback only | manual: Filter Mode dialog |
| D9 | **Slew-limit on cutoff** (A-C) | "Slew Limit Frequency" smooths cutoff | ❌ (intentional) | left off for instant filter "DONK" per user pref |
| D10 | **2nd/3rd FM input** per osc | 2-3 FM slots/osc | 🟡 1 slot | |
| D11 | **3rd cutoff-mod input** | manual: "3 modulation inputs for cutoff" | 🟡 2 slots | |
| D12 | **Env 2 Dest** | Env2 mod input → Attack/Decay/Release self-modulation | ❌ | |
| D13 | **Mod-Amp 1 SQ mode** | single-quadrant scaling (from bottom vs middle) | ❌ | |
| D14 | **Bit-exact filter/osc** | fixed-point Q-format coefficients | 🟡 float approx | bit-exact impossible in float (documented) |

---

## 3. Missing modulation sources (ModBus)
22 sources exist; **9 still return 0** (stubs):
❌ Oscillator 1 · ❌ Oscillator 2 · ❌ External Input · ❌ Accent · ❌ Sequencer A · ❌ Sequencer B ·
❌ MIDI Control B · ❌ Voice Number.
(✅ wired: None, LFO1/2/3, Env1/2, ModAmp1/2, Lag, Osc1-Pitch≈keytrack, Noise, Velocity, MIDI-Pressure, MIDI-Control-A≈modwheel.)
> Osc1/2 as audio-rate mod sources + Voice# require **per-voice** architecture (see D1).

---

## 4. Missing performance / voicing
| # | Feature | Clone |
|---|---------|-------|
| P1 | **Duo** voice mode (Osc1=lowest, Osc2=highest note) | ❌ (Mono/Poly/Unison only) |
| P2 | **UniV** unison-voice count | ❌ (fixed 4) |
| P3 | Separate **Poly Detune** vs **Unison Detune** params | 🟡 one shared `uni_detune` |
| P4 | **Portamento Auto / Exp** switches | 🟡 linear glide only |
| P5 | **Pitch-bend range** control (1-24 st) | 🟡 fixed ±2 st |
| P6 | **32-voice** polyphony / Dynamic voices | 🟡 fixed 16 |

---

## 5. Missing oscillator/osc features
| # | Feature | Clone |
|---|---------|-------|
| O1 | **Link** (Osc1 mod-input 1 → Osc2 + Osc3) | ❌ |
| O2 | Sample/wavetable load (see D3) | 🟡 |

---

## 6. Missing arpeggiator detail
✅ Arp on/off, Up/Down/Up&Down/Random, Rate (1/4–1/32), Octave 1-4, Hold are implemented.
❌ **As-played** order · ❌ **Random 1 vs Random 2** distinction (manual separates them) · ❌ **Trigger-Free**
(arp drives the sequencer) · ❌ Arp **Mode dialog** GUI.

---

## 7. Missing GUI elements
| # | Element | Clone |
|---|---------|-------|
| G1 | **Sample/Wave display** click → Sample Loader | 🟡 static "Sine" label |
| G2 | **Multi** envelope button (mono legato retrigger) | ❌ unbound |
| G3 | Mixer **Post** buttons (pre/post-filter routing) | ❌ decorative |
| G4 | **Link** button (Osc1) | ❌ decorative |
| G5 | LFO sync period range (32nd … 256 beats) | 🟡 4B…1/8 only |
| G6 | **♩ / ♫♫** (single-note / sequencer) buttons | ❌ decorative |
| G7 | Mod-Amp **SQ** button | ❌ decorative |
| G8 | Performance flip-lever graphic (functionally = radios) | 🟡 radio buttons |

---

## 8. Effects (separate standalone plugins — by design)
`VAZ2010Effect.dll` = Chorus, Flanger, Phaser, Reverb, Delay, Autopan, Decimator, Compressor, EQ,
Oscilloscope. **Out of scope for the synth clone** — user is building these as standalone plugins
(VAZReverb + VAZPhaser already done). See `tools/VAZ_Effects_Spec.md`.
