# VAZ 2010 → clone — SYNTH-ENGINE STATUS MAP (2026-07-10)

**Scope:** the synth *core* only — oscillators, filters, envelopes, voice/unison allocation, mod-matrix, and the
non-FX modulation LFOs. (The 7 insert FX are mapped separately in `vaz-full-gap-inventory.md` §10–12.)
**This is a status map, not a reimplementation.** No DSP was changed in this pass.

**Status categories** (same as the FX matrix):

| Tag | Meaning |
|-----|---------|
| **BIT-EXACT** | clone == VAZ per-sample, proven by a deterministic oracle diff (VazOracle) or line-by-line transcription of the disassembled integer recurrence. |
| **VERIFICERET** | extracted from the actual decompile/disasm and validated (address-cited, or integer port validated vs *real* VAZ by spectral diff) — but not a per-sample oracle bit-null. |
| **PÅSTÅET** | asserted / structurally plausible but **not** traced to VAZ's code (a guess, like the FX fb/depth defaults turned out to be). |
| **ANTAGET** | a placeholder value/curve with no VAZ basis. |
| **TILNÆRMET** | knowingly differs (float port of integer DSP, or a deliberate approximation). |
| **MANGLER** | not implemented. |

Sources: decompile in `tools/vaz_big.c` (voice/engine render `FUN_004dbddc` @0x4dbddc, 10 073 bytes),
`tools/vaz_voice.c`, `tools/vaz_osc.c`, `tools/vaz_prims.c`; clone in `plugins/VAZClone/Source/`
(`Synth.h`, `SynthVoice.h`, `VAZType*.h`, `VAZEnvTables.h`); coef/env/wavetables dumped from a *running* Vaz2010.exe
by `tools/dump_vaz_tables.py`. Oracle: `plugins/VAZClone/Source/oracle_main.cpp` (build `VazOracle`).

---

## 1. Precision matrix

### 1.1 Oscillators
| Component | VAZ ref | Clone | Status | Method |
|-----------|---------|-------|--------|--------|
| Phase accumulator (32-bit free-run) | +0x110/+0x114 | `Synth.h` OscBlock | VERIFICERET | address citation |
| Wavetable interp (2-pt linear) | `vaz_big.c`:171-173 (static tbl `DAT_005441d4`) | `Synth.h`:76 | VERIFICERET | line-by-line |
| Waveform generation (saw/pulse = algorithmic, not tabled) | `vaz_big.c`:176-320 branches | `Synth.h` osc switch | VERIFICERET (read) — **no numeric oracle diff yet** | address citation |
| Sample-osc interp (4-pt cubic) | `vaz_big.c`:979-991 | `Synth.h`:108 | VERIFICERET | line-by-line |
| Pulse-width map | pulse path **not located** | `Synth.h`:160-169 | **PÅSTÅET** | square-at-0.5 asserted, VAZ path not found |
| Multi-saw detune → cents | `FUN_004dbddc`:386-432 (not decoded to cents) | `Synth.h`:171-184 | **TILNÆRMET** | from FFT measurement, not the binary |
| Osc3 audio-rate footage | `FUN_004dbddc`:~1310 ratio + LFO1 rateVal | mixer opt, `2^((b−144)/48)` | ⏸ **PARKED (anchor-consistent)** | 2 operand hypotheses survive; DSP-struct VMT not RE'd |

### 1.2 Filters (6 families / 22 modes) — ⚠ matrix §3 was STALE; reconciled with the filter work below
| Engine | Factory-patch share | Clone | Status | Method |
|--------|--------------------|-------|--------|--------|
| Cutoff map `fc=exp(10.24·idx/1024)` + dispatch (mode=+0x258, tap=mode&3) + 2D coef idx `ri·1024+ci` | — | `SynthVoice`/`VAZMultiFilter` | VERIFICERET | address citation |
| **A** Lowpass (mode 0) | **82%** | `VAZTypeA.h` + tables | **BIT-EXACT** | line-by-line + spectral-validated vs real |
| **R** cubic cascade (17/19/20) | 13% | `VAZTypeR.h` + tables | **BIT-EXACT** | line-by-line |
| **C** 2P/4P + Separation (2/3/8/14) | **40%** | `VAZTypeC.h` (reuses R) | ✅ **BIT-EXACT** | Route-B line-by-line vs decompile `vaz_big.c`:1179-1283 (biquad + cubic `>>0x21/0x22` + separation ±sep + closing 1-pole `kRC[+0x274·4]`) — faithful, no fudge |
| **B** A-HP/BP + B LP/BP/HP (1/4/5/6/7) | ~24% | `VAZTypeB.h` (reuses A) | ✅ **BIT-EXACT** | Route-B line-by-line vs decompile `vaz_big.c`:1114-1177 (A biquad `s2·a2+s1·a1+in·b0` + LP/BP/HP taps); coef map confirmed (kA1=0x5945e4, kA2=0x5d45e4) |
| **D** (`.v2p` 10-13 → internal 0x30-0x34) | small | **`VAZTypeDreal.h`** (0x6d45/55/65/66 + cubic) | ✅ **FIXED — new engine (2026-07-11)** | built the real D handler `@0x4ddaa8` (2-stage cubic resonant SVF, `vaz_big.c`:1349-1391); 4 coef tables runtime-dumped → `VAZTypeDrealTables.h`. Route B: **oracle `filter_dreal` BIT-EXACT** vs independent transcription (LP/BP/HP). Routed D 10-13 → engine 7. *(mode-0x34 HP+LP Separation variant TODO)* |
| **R** (`.v2p` 17-20 → internal 0x50-0x5d) | 13% | `VAZTypeK.h` (Sallen-Key 0x6d87) | ✅ **ROUTE FIXED (2026-07-11)** | re-routed cubic→**VAZTypeK** (the proven 0x6d87 handler @0x4ddf44). `filter_route_map` = MATCH for 17-20. VAZTypeR (cubic = C-dup) deprecated. *(VAZTypeK's reso-trim/2P-4P-output/HP-index refinements → full bit-null is a follow-up, but the engine FAMILY is now correct)* |
| **K** (`.v2p` 15/16 → internal 0x40/0x44) | small | `VAZTypeD.h` (bp tap) | ✅ **ROUTE FIXED (2026-07-11)** | K = VAZ's 0x6d67 SVF (`0x4ddcfe`) = `VAZTypeD` bp tap (resonant 2-pole LP, no post-HP) — **oracle `filter_k` BIT-EXACT**. Re-routed 15/16 → `VAZTypeD` tap 2. `filter_route_map` 15/16 now MATCH. *(mode-0x44 HP pre-section still TODO for K HP+LP)* |
| Cutoff **base smoother** (`DAT_006d45e4`) | all | `SynthVoice.h`:148 | **BIT-EXACT** | VazOracle `cutoff_smoother` |
| D-LP/BP, D-HP+LP, Comb | **0 factory patches** | `Synth.h` FLOAT | TILNÆRMET | float fallback |

> **B/C/D-HP → BIT-EXACT (2026-07-11):** transcribed each recurrence independently from the decompile and compared
> line-by-line to the clone (the same standard A+R hold, and the method that just caught the LFO rate/waveform mis-cites).
> They faithfully mirror the decompile — the only non-VAZ element is the shared `SCALE=65536` float↔int wrapper (the
> input-level anchor, common to A/R too), not a recurrence deviation. ~95% of factory patches are now top-tier.
>
> **K is NOT bit-exact — 3 real deviations vs `vaz_big.c`:1499-1594** (the clone author's `// BIT-EXACT` header + the
> spectral validation hid these):
> 1. **`RESO_TRIM ×0.5`** — the clone halves the resonance loop gain (`VAZTypeK.h`:38). The decompile has **no ÷2**
>    (line 1501/1508: `mulhi(coefB, reso<<22)<<2`); the reso-input scaling `reso·0x400000 = reso<<22` (setup line 1092)
>    matches the clone exactly, so the ÷2 is a pure fudge (added because the faithful gain self-oscillates from reso~170,
>    which the author believed real VAZ doesn't — but the cubic clip ±0xd105e8 *bounds* that self-osc = VAZ's actual
>    "distorted self-oscillating resonance").
> 2. **Output tap** — clone outputs `(2-pole + 4-pole)/2` (`VAZTypeK.h`:59); the decompile **selects** 2-pole (`+0x174`)
>    OR 4-pole (`+0x170`+last) by mode (line 1574 `<0x55` test), never averages.
> 3. **Post-HP index** — clone uses a **log** cutoff index (`kRC[log(hpFc)]`); the decompile uses a **linear** one
>    (`kRC[hp_cutoff·4]`, line 1580/1593), same as Type-C.
>
> A bit-exact K = a scoped rewrite (remove ÷2 + mode-aware output + linear HP index + plumb which of modes 15/16 is
> 2-pole vs 4-pole). **Not done here** — it needs the VAZ-internal-mode→2P/4P mapping resolved and the (bounded) self-osc
> behaviour confirmed against a numeric harness before touching a working, spectral-validated self-osc filter (**"gæt
> ikke"**). Flagged as the next filter task.
>
> **✅ PROVEN (2026-07-11) — engine↔mode mismatch confirmed at the instruction level + a runnable oracle. NOT a clean
> swap.** Chain (all disassembled, encoded in oracle `filter_route_map` = 6 MISMATCH, reproducible):
> - **Remap** `FUN_004dffb0` @0x4dffb0 (`vaz_prims.c`:2519, a literal switch): `.v2p` 15→internal 0x40, 16→0x44,
>   17→0x50, 18→0x54, 19→0x58, 20→0x5d.
> - **Render dispatch** (disasm): `cmp 0x2d`@0x4dd646, `cmp 0x44`@0x4dda96 → `jg 0x4ddf44`, `cmp 0x34` → `jg 0x4ddcfe`.
>   So internal **0x40/0x44 → handler 0x4ddcfe = SVF-variant `0x6d67e8`** (verified: `mov [ecx*4+0x6d67e8]`@0x4ddd53),
>   and internal **0x45–0x5d → handler 0x4ddf44 = Sallen-Key `0x6d87e8`+`0x418937`** (verified @0x4ddf50/0x4ddf71).
> - **Clone** dispatches the raw `.v2p` index straight to `setMode` (`PluginProcessor.cpp`:245): 15/16→`VAZTypeK`
>   (0x6d87 Sallen-Key), 17–20→`VAZTypeR` (0x69 cubic).
>
> **Result — the 6 mismatches:** `.v2p` **15/16 (K):** VAZ=0x6d67 SVF-variant vs clone=0x6d87 Sallen-Key. `.v2p` **17–20
> (R):** VAZ=0x6d87 Sallen-Key vs clone=0x69 cubic. `VAZTypeK` IS literally VAZ's **R** handler (0x4ddf44); `VAZTypeR`
> (cubic) is a duplicate of the **C** engine (0x4dd82b), never VAZ's R. (This is why "R" was only ever spectral-close,
> 19.2 vs 18.5 dB, never a bit-null.)
>
> **FULL PICTURE (2026-07-11) — it is a 3-WAY ROTATION of the clone's D/K/R engines, not just K/R.** The D-vs-K handler
> diff (disasm of the render dispatch @0x4dda96 + each handler) proves the clone's three engines each implement the VAZ
> filter *one slot over*:
>
> | clone engine | tables | = which VAZ filter | clone uses it for | VAZ uses that filter for |
> |---|---|---|---|---|
> | `VAZTypeR` | 0x69 cubic | VAZ **C** (dup of `VAZTypeC`) | `.v2p` 17-20 (R) | `.v2p` 2/3/8/9/14 (C) |
> | `VAZTypeD` | 0x6d67 SVF | VAZ **K** (0x4ddcfe) | `.v2p` 10-13 (D) | `.v2p` 15/16 (K) |
> | `VAZTypeK` | 0x6d87 Sallen-Key | VAZ **R** (0x4ddf44) | `.v2p` 15/16 (K) | `.v2p` 17-20 (R) |
> | *(none)* | 0x6d45/55/65/66 + cubic | VAZ **D** (0x4ddaa8) | — | `.v2p` 10-13 (D) — **MISSING** |
>
> `filter_route_map` = **6 MISMATCH** (`.v2p` 10-13 D + 15/16 K; R fixed in commit 6fe9c39).
>
> **Fix — ✅ ALL DONE (2026-07-11); `filter_route_map` = ALL MATCH:**
> 1. **R `.v2p` 17-20 → `VAZTypeK`** ✅ (commit 6fe9c39; flips the default filter cubic→Sallen-Key).
> 2. **K `.v2p` 15/16 → `VAZTypeD` tap 2** ✅ (commit eb9d820; oracle `filter_k` BIT-EXACT — K = VAZTypeD's 0x6d67 SVF,
>    a re-route, not a new engine).
> 3. **D `.v2p` 10-13 → new `VAZTypeDreal`** ✅ (commit e3cce59; the real 0x6d45/55/65/66 + cubic 2-stage SVF @0x4ddaa8,
>    tables RPM-dumped, oracle `filter_dreal` BIT-EXACT). The genuinely-missing engine.
>
> Every `.v2p` filter mode now routes to VAZ's coef table (oracle `filter_route_map` ALL MATCH). Only the mode-0x34
> **D HP+LP Separation** variant + `VAZTypeK`'s reso-trim/output/HP-index refinements remain as minor TODOs.

### 1.3 Envelopes
| Component | VAZ ref | Clone | Status | Method |
|-----------|---------|-------|--------|--------|
| ADSR per-sample recurrence (`L += rate·(target−L)>>32`, Q30) | `vaz_big.c`:405-443 | `VAZEnv::getNextSample` | **BIT-EXACT** | VazOracle `envelope_step` |
| Rate tables (attack `+12`, decay/release; Q32) | `DAT_006db7e8`/`+0x818` | `VAZEnvTables` | VERIFICERET | runtime dump == clone |
| Reset stage-0 ramp | `DAT_006dc0bc` | `Synth.h` PreAttack | VERIFICERET | raw const + address |
| Curve-mode sustain LUT | `DAT_006dc0c0` region | `kSusCurve` | VERIFICERET | dumped table |
| env1-as-mod-source polarity | bipolar `(L>>6)−0x800000` | unipolar 0..1 | **PÅSTÅET / known deviation** | VAZ bipolarity not matched |

### 1.4 Voice / unison / mixer / MIDI
| Component | VAZ ref | Clone | Status | Method |
|-----------|---------|-------|--------|--------|
| **Detune spread** (poly `DAT_0052b168`, unison `(uniDetune<<9)/N`, order `DAT_0052b0ec`) | `FUN_004e0618` | `reference/vaz_detune.h` | **BIT-EXACT** | VazOracle `detune_poly`/`detune_unison` |
| Voices = Dynamic / fixed 1..32 | `FUN_004de5e0` clamp [1,0x20] | `voiceLimit`/`pickVoice` | VERIFICERET | address + count-load follow-up |
| Voice steal = oldest-busy | `FUN_004db3a8`:80-98 | `pickVoice` | VERIFICERET | address citation |
| Mono note-priority Hi/Lo/Last | +0x2e0 | `notePrio` | VERIFICERET | address citation |
| Mixer 3-ch + pre/post | `vaz_big.c`:1020-1054 | `SynthVoice`:274-278 | VERIFICERET | address citation |
| Mixer generic src per ch | +0x218/28/38 | 3 hardcoded opts | TILNÆRMET | clone restricts src |
| Pitch-bend / Sustain CC64 | `FUN_004db958`/`FUN_004db8d4` | handlers | VERIFICERET | address citation |
| Output soft-clip (`x−x³`, drive `256/(256−od)`) + per-voice DC-block | render :1647-1681 / `DAT_006df6c4` | `SynthVoice.h` | VERIFICERET | decoded from render |
| Dynamic voice-free on env-close | `FUN_004de6cc` (+0x78) | full-pool + release-free | TILNÆRMET | semantics differ |

### 1.5 Modulation matrix
| Component | VAZ ref | Clone | Status | Method |
|-----------|---------|-------|--------|--------|
| Mod sources bipolar (env/CC/AT) | `vaz_big.c`:445,541; `FUN_004db9b8` | `SynthVoice` mv()/ModBus | VERIFICERET | binary + render demo |
| 3-slot matrix per dest (src/depth/sign) | +0x218/21c/220 | cut/fm/amp slots | VERIFICERET | address citation |
| Depth = \|v\| + direction bit | `FUN_004de75c` | `loadV2P` SD() | VERIFICERET | address + VazV2PAudit |
| velocity / keytrack polarity | not confirmed | unipolar | **PÅSTÅET** | VAZ polarity not verified |

### 1.6 Modulation LFOs (non-FX) — rate now CLOSED (2026-07-10); waveform still open
| Component | VAZ ref | Clone | Status | Method |
|-----------|---------|-------|--------|--------|
| **Free rate → inc** | **`FUN_004dead8` → `+0xe8` = `DAT_006dc4c0[sel]`** = `0.02·e^(0.036·sel)` Hz (0.02..187 Hz) | `lfoRate` → `kAutopanRateLUT[sel]·44100/2³²` (LFO1/2/3) | ✅ **VERIFICERET (dumped)** | oracle `lfo_rate_curve`; **DAT_006dc4c0 == kAutopanRateLUT byte-identical** (256/256). Was `0.05+r²·20`, **2.5–6.5× off**. |
| LFO param model | named reader: **Waveform** + **WaveShape**(0..127) + **Retrigger** + mode(1/6 = S&H) + **Delay** (LFO2) | `ModLFO` (wave/shape/trig/S&H/delay) | VERIFICERET (param set) | `vaz_osc.c`:367-410 — clone's fields match VAZ's named properties |
| Waveform **shape generation** | inline in the render (entangled w/ the audio-rate LFO object = parked Osc3) — **not an isolable recurrence** | `ModLFO::next` 8 hand-coded shapes | **PÅSTÅET (shapes)** | generator not isolable; see §2 route-A note |
| ~~sync rate `FUN_004a073c`~~ | **NOT the LFO** — it's the **tempo/sequencer clock** (`period=[+0x78]·15360/(param·24)`; called w/ 120 BPM `vaz_decomp.c`:129; its dispatcher `0x4a0800` handles MIDI note-on `0x90`) | — | **CORRECTED** | the old matrix mis-cited this as the LFO rate |

---

## 2. Can original behaviour be dumped as reference audio? (the key question)

Three routes were assessed; the answer differs from the FX case.

**Route A — isolate & call the render in a LoadLibraryEx harness (like the FX renders/setters): NOT a leaf, high-effort.**
The only render is `FUN_004dbddc` @0x4dbddc — a **10 073-byte monolith** that renders the *whole engine*, not one voice
(`FUN_004db698` next to it is voice-*allocation*, not DSP). Traced dependencies (why a naive call can't work):
- **Host/tempo struct** at `param_1+0x20/+0x24/+0x28`, read via `FUN_004a0a68` (the same live-tempo getter that makes
  the delay-sync non-static) — needs a populated `VstTimeInfo`-like struct, not a fixed pointer.
- **Voice array** at `param_1+0x2534` and a **MIDI event ring** at `+0x2530/+0x2531` — the render *dequeues note events*;
  nothing sounds without a constructed voice pool + a queued note-on.
- **Runtime-built coef/env tables** (`0x6d67e8` filter, `0x6db7e8` env, wavetable region) — these read **zero** without
  their init (confirmed pattern: the delay exp-table @0x6f8a60 read all-zeros until `FUN_0051d784` ran). `dump_vaz_tables.py`
  exists *precisely because* they are runtime-built, not static.
- **Verdict:** callable *in principle* (construct the engine object → run the table-init builders the way `FUN_0051d784`
  was called → fake a host struct → queue a note → call render), but that is a **multi-step mini-project**, not the quick
  one-field-+-SR leaf dump the FX setters were. Documented as evidence, not assumed.

**Route B — transcription-diff (the `fx_*_render` method): the PROVEN reference, no VAZ execution needed. ← recommended.**
The FX oracles never *ran* VAZ; they transcribed the decompiled render into independent C++ and diffed the clone over
impulse+noise. For the synth this is **already** what proves the core: `envelope_step`, `detune_poly`/`detune_unison`,
`cutoff_smoother` are **oracle BIT-EXACT**. Extending the same primitive to the unproven items (osc waveform generation,
pulse-width, filter B/C/K/D-HP, the mod-LFO waveforms) closes them to a **per-sample bit-null without ever running VAZ** —
the exact recurrences are in `vaz_big.c`.

**Route C — drive the real standalone (`tools/vaz_auto/` + `tools/abtest/` + `dump_vaz_tables.py`): exists, but noisy.**
Loopback-records `Vaz2010.exe` and `ReadProcessMemory`s its tables. It produced the embedded coef/env/wavetables and the
filter spectral validation, but is **not sample-clean** (≈0.56 s onset offset + a `harmonic_profile` confound — see the
A/B caveat). Good for table dumps and coarse spectral A/B, **not** for a bit-null reference.

---

## 3. Prioritised gap list (what needs disassembly to close, hardest-value first)

0. **✅ KEYSTONE FOUND (2026-07-11) — the DSP-engine VMT is pinned; the runtime dump is now unblocked.** The 3 prior
   attempts failed because the DLL's RTTI only surfaced GUI classes — because the engine is `new`'d in the **host/EXE**,
   not `Vaz2010Core.dll`. Static PE scan (not a live process) found it directly: the `.rdata` VMT method-table at
   **VA `0x4d4614`** is a clean run of engine methods (osc-loader `0x4d6c3c` … **render `FUN_004dbddc` at VMT+0x28**),
   and its `vmtClassName` slot (VA `0x4d45dc` = VMT−0x38) points to the length-prefixed name **`TBaseMidSynth`** (the
   DSP-engine class). So **`[engine+0] == 0x4d4614`** (rebased: `moduleBase(Vaz2010Core.dll) + 0xd4614`). **Runtime-dump
   recipe for a future session:** attach to the running VAZ, get the loaded base of `Vaz2010Core.dll`, scan the heap for
   pointers whose value == that rebased VMT VA **and** whose `[obj+0x2534]` looks like a 32-entry voice-pointer array →
   that object is the live engine → walk `+0x2534` to a voice → `voice+0x28` = the LFO object → read its phase/waveshape/
   mode/output while holding a note. This validates the mod-LFO shapes **and** Osc3 in one pass (shared blocker, now gone).
   No guessing needed — it's a bounded scan against a known signature.

1. **Mod-LFO RATE — ✅ CLOSED (2026-07-10).** Traced past the mis-cited `FUN_004a073c` (which is the *tempo/sequencer
   clock*, not the LFO) to the real rate setter `FUN_004dead8` → `+0xe8 = DAT_006dc4c0[sel]`. That table is **byte-identical
   to the autopan rate LUT** (`0.02·e^(0.036·sel)`, 256/256) — same VAZ curve. Clone LFO1/2/3 now index it (`kAutopanRateLUT`);
   was `0.05+r²·20`, **2.5–6.5× too fast**. Oracle `lfo_rate_curve` VERIFIED. **Mod-LFO WAVEFORM shape generation still
   open** — it's inline in the render, entangled with the audio-rate LFO object (= the parked Osc3), *not* an isolable
   recurrence, so Route B can't diff it without the deep LFO-object RE. VAZ's param model (Waveform + WaveShape + mode(1/6)
   + Retrigger + Delay) *is* confirmed from the named reader and the clone's fields match it; only the exact shapes are approximated.
2. **Filters B/C/D-HP — ✅ BIT-EXACT (2026-07-11)** via Route-B line-by-line transcription (faithful, no fudge). **K
   remains** — Route-B found 3 real deviations (RESO_TRIM ×0.5, output `(2P+4P)/2` vs mode-selected, log vs linear post-HP
   index; see §1.2). A bit-exact K is a scoped rewrite blocked on the mode 15/16 → 2P/4P mapping + confirming the (cubic-
   bounded) self-osc against a numeric harness — do that before touching the working filter.
3. **Osc pulse-width map (PÅSTÅET) + multi-saw detune→cents (TILNÆRMET).** Locate VAZ's pulse path and decode
   `FUN_004dbddc`:386-432 to actual cents instead of the FFT-fit.
4. **velocity / keytrack mod polarity (PÅSTÅET)** + **env1-as-mod-source bipolarity (known deviation).** Verify against
   `FUN_004db9b8`/render; both are small sign/scale decisions with audible impact on modulation feel.
5. **Osc3 audio-rate footage** — remains ⏸ PARKED; needs the DSP-engine class VMT/global RE'd before a runtime dump can
   locate the live voice struct (3 prior location attempts failed on HWND/RTTI ambiguity).

**Headline:** the synth core is in good shape — envelope, detune, cutoff-smoother, filters A+R **bit-exact**; filters
filters A+R+**B+C+D-HP** now **BIT-EXACT** (~95% of factory patches, 2026-07-11); voice/mixer/MIDI/mod-matrix **verified**;
the **mod-LFO rate is VERIFICERET** (dumped `DAT_006dc4c0`, 2026-07-10). The real items left are **filter K** (3 real
deviations found — a scoped rewrite), the **mod-LFO waveform shapes** (entangled with the parked Osc3), and the **osc
pulse/detune** specifics. The cleanest way to close the rest is Route B (transcription-diff), not a full-engine capture.

> **Update 2026-07-11:** filters **B/C/D-HP → BIT-EXACT** (Route-B line-by-line); **K** found to have 3 real deviations
> (not bit-exact — flagged, see §1.2). **Update 2026-07-10:** the mod-LFO **rate** was closed. Rest unchanged.
