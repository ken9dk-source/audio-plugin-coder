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

## 0. PHASE-1 AUDIT (2026-07-12) — everything NOT recently Route-B-verified, by usage

Systematic-audit inventory. "Route-B-verified" = per-sample oracle bit-null **or** transcription/measurement vs *real* VAZ. Everything below is a Phase-2 target, ordered by factory-patch usage (highest first).

| # | Item | §  | Current tag | Usage | Phase-2 action |
|---|------|----|-------------|-------|----------------|
| 1 | ~~**Saw waveform gen**~~ | 1.1 | ✅ **DONE 2026-07-12 — VERIFICERET vs real VAZ render** (matches ±0.03 @ C3) | ~all patches | none — additive ≈ VAZ BLEP; bit-null gated on BLEP re-port, not required for parity |
| 2 | **Output soft-clip `x−x³` + DC-block** | 1.4 | VERIFICERET (decoded, no oracle) | all patches | oracle `output_clip` → BIT-EXACT |
| 3 | **velocity** mod polarity | 1.5 | ✅ **FIXED 2026-07-13 — was unipolar, now BIPOLAR** | very common | confirmed vs VAZ: mod-source values live at `voice+selector·4`; velocity=sel 17→`voice[0x44]=vel<<17−0x800000` (note-on `FUN_004db288 @0x4DB327`) = bipolar `vel/64−1`. Fixed both `mv()` sites (`voiceVel·2−1`). **keytrack** (case 10 = "Osc1 Pitch") still a TILNÆRMET approximation — separate |
| 4 | ~~env1-as-mod-source bipolarity~~ | 1.3 | ✅ **ALREADY DONE (map was stale)** | common | clone `mv()` cases 4/5 already `env·2−1` (bipolar, `SynthVoice.h`:248-249) matching VAZ `(L>>6)−0x800000`; VCA still uses unipolar env. No action. |
| 5 | ~~Mod-bus depth scaling~~ | 1.5 | ✅ **VERIFIED 2026-07-13 (fixed-point trace)** | all modulated patches | VAZ adds `voice[src]·depth>>s`; `voice[src]=mv·2²³` so mod = `mv·depth` in each engine's base units — the shift `s` compensates for the per-engine base scale (A: base·2²² + `>>1` @1095; another: base·2¹⁵ + `>>8` @1185; differ by 2⁷=8−1 bits). Normalized ÷255 = clone `cutAmt·mv` (`cutAmt=\|v\|/255`), same exp-cutoff Hz shift (10.24·depth/255). Bipolar sources ✓, 3 slots ✓. **Matches.** |
| 6 | **Multi-saw detune → cents** | 1.1 | ⚠ **TILNÆRMET, FFT-validated** | common osc | 0/+24/+48/+72.5c from VAZ's FFT-at-Max; exact detune is ENTANGLED in the render monolith (`FUN_004dbddc`, no isolable sub-osc loop) = same depth as #8. Deferred to the render-RE / VMT-dump track. **Don't guess.** |
| 7 | **Sample-osc 4-pt cubic interp** | 1.1 | ⚠ **coefs = Catmull-Rom EXACT, frac-scaling OPEN** | Sample osc (rare) | VAZ cubic (`vaz_big.c`:979-991) has Catmull-Rom coefs (c1=p2−p0, c2=2p0−5p1+4p2−p3, c3=−p0+3p1−3p2+p3) — **identical to clone**. BUT VAZ's frac = `iVar3[300](24-bit, masked 0xffffff @1001) << 7 >>32 = natural_frac/2` → term weights 0.5/0.25/0.125, vs clone's Catmull 0.5/0.5/0.5. Real discrepancy; resolution needs why the 24-bit frac maps 0..½ (full sample-phase fixed-point). **Don't guess** — low priority (rare osc). |
| 8 | **Mod-LFO waveform shapes** (8 hand-coded) | 1.6 | **PÅSTÅET** — BOTH RE routes blocked (2026-07-14) | common (LFO) | Static: generator caches +0x84 (only save/dispatch/copy read it) → confirmed NOT isolable. Runtime dump: headless standalone can't isolate output word. **Close via audio-render extraction** (see §1.6 ↳). Clone's shapes stay. |
| 9 | **Osc3 audio-rate footage pitch** | 1.1 | ⏸ PARKED (VMT) | uncommon | same VMT dump as #8 (one pass) |
| 10 | Mixer generic src per ch (restricted to 3) | 1.4 | TILNÆRMET | uncommon | low priority |
| 11 | D-LP/BP, D-HP+LP, Comb (float fallback) | 1.2 | TILNÆRMET | **0 factory patches** | lowest priority |

**Already Route-B-verified (no Phase-2 action):** filters A/B/C/D/K/R (all BIT-EXACT), cutoff-smoother, ADSR envelope, detune poly/unison (oracle bit-null); mod-LFO **rate** (dumped `DAT_006dc4c0`); **Pulse** (FIXED 2026-07-12, matches real VAZ render); note→pitch A440 (RPM-dumped `DAT_006dd0c0`). Structural VERIFICERET (phase acc, wavetable interp, voices/steal/priority, pitch-bend/sustain, env rate tables) = low-risk, address-cited, deferred.

**PHASE-3 GUI control-binding audit (2026-07-13):** ✅ `control_binding_audit.test.js` — all 125 bindings cross-checked vs `ParameterIDs.hpp`: **no cross-wiring, no dead/unknown bindings** (the "Rate-fader" bug class is provably absent; that report was a stale plugin cache, [[feedback_vazclone_stale_plugin_cache]]). **Coverage gaps — 12 preset-only params with NO GUI control** (follow-up, not bugs): `cut_mod3_src/amt` (3rd cutoff-mod, DSP uses it), `amp_mod2_src/amt`, `note_priority` (Hi/Lo/Last), `lfo2_shape`, `lfo2_delay`, `lfo3_wave`, `osc2_sync`, `o2_detune`.
**✅ RESOLVED 2026-07-13 (commit 81df17b, built + on Desktop):** classified all 12 against the DSP → 4 real gaps (param created+mapped+CONSUMED) vs 8 stubs (param exists, DSP ignores). Added the 2 clean real gaps to `index.html`: **cut_mod3** (VAZ filter has 3 cutoff-mod slots — `voice-render-re.md`:26 / `ghidra-gap-analysis.md`:130) + **amp_mod2** (VAZ amp has 2 AM sources — same ref). ⚠ **o2_detune was added then REVERTED (2026-07-13):** re-verification proved it a clone-internal ARTIFACT, not a VAZ control — absent from the `.v2p` (PS+135 = `o1_shape` @`PluginProcessor.cpp`:635, NOT o2_detune; `parameter-spec.md`:21's PS+135 note is refuted; no `o1_detune` counterpart, and only L62/L283 reference it). Osc2's real Multi-Saw detune is the **`o2_shape` slider relabeled 'Detune'** via `o1labels[2]` when `o2_wave`==Multi-Saw (conditional, fires for both oscs @L699-703). Guarded by `detune_conditional.test.js`. Skipped **osc2_sync** (already reachable via the Osc2 "Sync" waveform button — a toggle would duplicate). Remaining 5 uncovered are **stubs** (`osc2_sync` + `note_priority`, `lfo2_shape`, `lfo2_delay`, `lfo3_wave`) — a GUI control alone is a no-op; each needs DSP first, and the LFO-shape ones are gated on the engine-dump (§3.0, mod-LFO #8). Audit: 12→6 uncovered (o2_detune reverted as an artifact), PASS.

**PHASE-5 audibility matrix (2026-07-13):** ✅ extended `dsp_audit` to sweep filter mode × cutoff × **resonance**; added guard `all_filters_reso_bounded` (22 modes at reso 0.9 keep peak≤2.0 — cubic-clip-bounded self-osc). **12/12 guards pass.** Remaining open (deep RE, next sessions): **Phase-2 #5** mod-bus depth fixed-point trace, **#6/#8** render-monolith detune/LFO-shapes + `TBaseMidSynth` VMT runtime-dump, **#7** sample-interp oracle; **Phase-4** v2p-loader field-by-field (needs a ground-truth engine-struct dump).

## 1. Precision matrix

### 1.1 Oscillators
| Component | VAZ ref | Clone | Status | Method |
|-----------|---------|-------|--------|--------|
| Phase accumulator (32-bit free-run) | +0x110/+0x114 | `Synth.h` OscBlock | VERIFICERET | address citation |
| **Sine** wavetable (2-pt linear, `DAT_005441d4[256]`) — waveform **3** only (`iVar9==3`, `vaz_big.c`:167-173) | `DAT_005441d4` = RPM-dumped `sin(2πi/256)·2²³` (`saw_0x5441d4.txt`) | `Synth.h` case 3 (sine) | ✅ **VERIFICERET (dumped 2026-07-12)** | table is a pure 256-pt sine (idx1=205867=sin(2π/256)·2²³); *not* the saw (old miscite corrected) |
| **Saw** waveform gen (band-limited ramp) | **BLEP step-tables `DAT_006dd2c0/006de2c0`** via `iv8=((ph>>8)+(ph>>0xb)−0xe000)>>0xd`, `vaz_big.c`:175-196 | `Synth.h` case 0 — **additive** band-limited mip-mapped wavetables | ✅ **VERIFICERET vs real VAZ render (2026-07-12)** | method differs (VAZ BLEP ramp vs clone additive) but **spectra match at C3 within ±0.03** (VAZ `1/.52/.32/.25/.20…` vs clone `1/.49/.34/.23/.20…`, `DEFAULT_01_real` vs `reg_saw`). Per-sample bit-null would need the (gated) BLEP re-port; **not required for parity**. |
| Sample-osc interp (4-pt cubic) | `vaz_big.c`:979-991 | `Synth.h`:108 | VERIFICERET | line-by-line |
| Pulse oscillator | **base duty setter `FUN_004de930` @0x4de930** (odd waveform → `param[0xa4]=0x7f<<16` = 127/256 ≈ 50% square; slider ignored, PWM only via Waveshape-Mod) + `vaz_big.c`:176/213 render reads `osc[0xa4]` | `Synth.h` case 1 (difference-of-saws) + `SynthVoice.h` base `(127/255)·(1−o1Shape)` | ✅ **FIXED + VERIFICERET vs real VAZ (2026-07-12)** | **Root cause of the "octave-high / C64" complaint FOUND & FIXED:** the clone's default Pulse was a ~0.1% impulse (**no fundamental** → thin/bright, *sounds* an octave up). VAZ's setter force-sets the base duty to `0x7f` for odd waveforms (Pulse=1). Fix: `SynthVoice.h` pulse base = `(127/255)·(1−o1Shape)` → Shape=0 → 50% square (**matches real VAZ Pulse render: 131 Hz strong fundamental, odd harmonics — spectra identical**, `tools/vaz_auto` UI-click + `VazRender` A/B), fader sweeps square→thin (VAZ itself is fader-static; documented clone deviation, default matches). Octave itself was never wrong: VAZ note-table `DAT_006dd0c0[note]=note·128` → standard A440 (RPM-dumped), == clone. BLEP artifacts (`VAZPulseOsc.h`, `pulse_step_*`, oracle `osc_pulse_blep`) kept for a future validated re-port; the diff-of-saws recurrence is disasm-EXONERATED. See [[project_vazclone_pulse_octave]]. |
| Multi-saw detune → cents | `FUN_004dbddc`:386-432 (not decoded to cents) | `Synth.h`:171-184 | **TILNÆRMET** | from FFT measurement, not the binary |
| Osc3 audio-rate footage | `FUN_004dbddc`:~1310 ratio + LFO1 rateVal | mixer opt, `2^((b−144)/48)` | ⏸ **PARKED (anchor-consistent)** | 2 operand hypotheses survive; DSP-struct VMT not RE'd. **Mechanism now confirmed (2026-07-11): LFO1↔Osc3 is an explicit mixer-source mode-flag, NOT a rate threshold** — only mix-ch3 offers "Oscillator 3" (DFM `MixerSourcePopup`), audio already keys off `mix3Src==1`. GUI now relabels the LFO1 panel → "Oscillator 3" on that condition (`bindLfo1Identity`, matches Core.dll relabel @1109628/45). Only the *footage-pitch value* stays parked (VMT). |

> **Systemic audibility audit (2026-07-11):** after the Pulse min-edge-clamp bug, `dsp_audit` gained an "audible at
> default/extreme" RMS matrix + guards (`all_osc_audible_at_default`, `all_filters_pass_note_somewhere`, plus the
> printed osc×shape / filter×cutoff table). Result: **the Pulse clamp was the ONLY un-ported guard** — every osc
> waveform is audible at default, every filter passes the note at some musical cutoff, none blow up at max reso
> (peak < 0.5). Full-function re-reads: **A/B** have no output/state clamps in VAZ (`vaz_big.c`:1104-1177) so the
> clone's absence matches; **C** was Route-B re-read (cubic soft-clip `>>0x21`/`0x22` + separation ±sep clamps ported,
> `vaz_big.c`:1179-1235); **D/K/R/Dreal** re-read with their reso/cubic clamps. (Mode-12 D-HP is *correctly* silent
> at max cutoff — the note is below the HP freq, loud at cutoff 0.3-0.5.)

### 1.2 Filters (6 families / 22 modes) — ⚠ matrix §3 was STALE; reconciled with the filter work below
| Engine | Factory-patch share | Clone | Status | Method |
|--------|--------------------|-------|--------|--------|
| Cutoff map `fc=exp(10.24·idx/1024)` + dispatch (mode=+0x258, tap=mode&3) + 2D coef idx `ri·1024+ci` | — | `SynthVoice`/`VAZMultiFilter` | VERIFICERET | address citation |
| **A** Lowpass (mode 0) | **82%** | `VAZTypeA.h` + tables | **BIT-EXACT** | line-by-line + spectral-validated vs real |
| **R** cubic cascade (17/19/20) | 13% | `VAZTypeR.h` + tables | **BIT-EXACT** | line-by-line |
| **C** 2P/4P + Separation (2/3/8/14) | **40%** | `VAZTypeC.h` (reuses R) | ✅ **BIT-EXACT** | Route-B line-by-line vs decompile `vaz_big.c`:1179-1283 (biquad + cubic `>>0x21/0x22` + separation ±sep + closing 1-pole `kRC[+0x274·4]`) — faithful, no fudge |
| **B** A-HP/BP + B LP/BP/HP (1/4/5/6/7) | ~24% | `VAZTypeB.h` (reuses A) | ✅ **BIT-EXACT** | Route-B line-by-line vs decompile `vaz_big.c`:1114-1177 (A biquad `s2·a2+s1·a1+in·b0` + LP/BP/HP taps); coef map confirmed (kA1=0x5945e4, kA2=0x5d45e4) |
| **D** (`.v2p` 10-13 → internal 0x30-0x34) | small | **`VAZTypeDreal.h`** (0x6d45/55/65/66 + cubic) | ✅ **BIT-EXACT — new engine (2026-07-11)** | built the real D handler `@0x4ddaa8` (2-stage cubic resonant SVF, `vaz_big.c`:1305-1391); 4 coef tables runtime-dumped → `VAZTypeDrealTables.h`. **oracle `filter_dreal` BIT-EXACT** (LP/BP/HP) + **`filter_dreal_hplp` BIT-EXACT** (mode-0x34 HP+LP Separation = HP@cut−Sep → LP@cut+Sep, per `vaz_big.c`:1296/1301/1312/1347). Routed 10-13 → engine 7. |
| **R** (`.v2p` 17-20 → internal 0x50-0x5d) | 13% | `VAZTypeK.h` (Sallen-Key 0x6d87) | ✅ **BIT-EXACT (2026-07-11)** | re-routed cubic→`VAZTypeK` (0x6d87 handler @0x4ddf44) **and** rewrote it faithfully: removed the reso ÷2, replaced avg output with mode-SELECT 2P(17/18)\|4P(19/20), linear post-HP index `kRC[hp·4]`. **oracle `filter_r` BIT-EXACT** (2P & 4P; pre-fix RMS was 1.36). VAZTypeR (cubic=C-dup) deprecated. Self-osc bounded by the ±0xd105e8 cubic clip. **R reads NO separation param** for either pole count (handler never touches `param[0x270]`; GUI has no `rbTypeR4SM` radio — only C has a Separation-Modulation variant), so R's aux knob = *Unused*, both 2P & 4P. |
| **K** (`.v2p` 15/16 → internal 0x40/0x44) | small | `VAZTypeD.h` (bp tap + `processHPLP`) | ✅ **BIT-EXACT (2026-07-11)** | K = VAZ's 0x6d67 SVF (`0x4ddcfe`) = `VAZTypeD` bp tap — **oracle `filter_k` BIT-EXACT**; .v2p 15 (K LP) → tap 2. .v2p 16 (K HP+LP, mode 0x44) = a LINEAR HP pre-section (cut=aux/param 0x270·4, reso=hpHz/param 0x274; `vaz_big.c`:1394-1451) → main K — **oracle `filter_k_hplp` BIT-EXACT**. `filter_route_map` 15/16 MATCH. |
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
> Every `.v2p` filter mode now routes to VAZ's coef table (oracle `filter_route_map` ALL MATCH). **The whole D/K/R
> filter family is now BIT-EXACT**, including both HP+LP-Separation variants: D mode-0x34 (HP@cut−Sep → LP@cut+Sep,
> `filter_dreal_hplp`) and K mode-0x44 (linear HP pre-section → main K, `filter_k_hplp`), plus `VAZTypeK`'s
> reso-trim/output/HP-index fudge removals (`filter_r`). No filter TODOs remain.
>
> **Filter mode→label table (GUI):** VAZ dynamically relabels the `flt_aux` / `hp_cutoff` knobs + the filter mod-slot
> per mode. Extracted the authoritative config from Core.dll — the DFM radio captions + the knob-label pool at
> file-off `1111113` (Bandwidth / Separation / Highpass Cutoff / Damping / Unused / Highpass Resonance / Input-Output
> Mix / {Resonance,Separation,Highpass} Modulation) — cross-checked with the DSP param reads (aux=`0x270`, hp=`0x274`).
> Clone had **static** labels; now relabels per-mode (`FILTER_LABELS[22]` in `index.html`, `bindFilterModeLabels`,
> guarded by `Source/ui/test/filter_labels.test.js` — instruments the real getChoiceIndex→apply path, all 22 green).
> Key confirmations vs screenshots: **R 2P/4P aux = Unused** (no separation, see R row); C-4P-Sep & D-HP+LP = Separation;
> K-HP+LP = Highpass Cutoff / Highpass Resonance; Comb = Damping / Input-Output Mix.
> **B family (1/6/7):** VAZ puts **Bandwidth on knob-2** (not aux) with aux=Unused. Since the biquad row (`rowReso`)
> reads `flt_aux` (`VAZTypeB`), knob-2 is **re-targeted** onto `flt_aux` for B (`bindFilterKnob2` / `k2SetTarget`) and
> the aux row hides — matching VAZ's *Cutoff / Bandwidth / Unused* layout. DSP + `.v2p`→param mapping untouched.
> (The `bindFilterModeLabels` indexing was verified clean — the earlier "some modes wrong" was this B knob-position.)

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
| ↳ **CONFIRMED not isolable (2026-07-14, `tools/find_lfo_gen.py`)** | LFO obj fields mapped: **+0x84 Waveform, +0x8c WaveShape, +0xd0 mode(6=S&H), +0x88/+0x90/+0xd4 aux**; ctor @0x4B9C00 (defaults wave=0xff/shape=2/mode=−1), setters @0x4DEAxx. The **+0x84 selector is read by ONLY 3 sites — all non-generators**: 0x41E9A9 (save/UI, RTL string calls), 0x4612F9 (property-copy, `word ptr` field sweep), 0x459006 (**false positive** = `cmp eax,0x84` message-dispatch imm). The audio generator **caches the selector into a local** → shape math not pinned to the field, confirming "not isolable." | — | **BOTH RE routes blocked:** static (above) + headless runtime dump (`dump_seq_runtime.py` — object not isolable from 14k live words / vtable scan). **Realistic close = AUDIO-RENDER extraction** (drive LFO→pitch/cutoff via `vaz_auto`, render, recover shape) OR a GUI-driven dump. Clone's 8 hand-coded shapes STAY (PÅSTÅET). |
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
   > **Harness built + first run (2026-07-13, `tools/dump_engine_vmt.py`):** VMT @ `coreBase+0xd4614` **VERIFIED live**
   > (12/12 slots are Core.dll `.text` pointers). BUT the recipe as written did NOT locate the engine — two refinements
   > learned: **(a)** no heap object has `[0]==base VMT` → the live engine is a **DERIVED** class (its `[0]` = a derived
   > VMT; scan for vtables whose `vmtClassName` chains to `TBaseMidSynth`, not the base value). **(b)** at REST (no note
   > held) the voice pointers at `+0x2534` are **null/unallocated**, so a "consecutive non-null pointers" pattern misses
   > the real engine — **send + hold a MIDI note first** so voices allocate. The voice-array-pattern scan found 83
   > look-alikes (clustered, `voice+0x28`→Core.dll not a heap LFO) = NOT the engine. **Next-session refinement:** hold a
   > note → scan for a derived-VMT object with a (null-tolerant) 32-voice array whose voices share one class vtable →
   > confirm the `voice+0x28` LFO offset against the decompile (may differ in the standalone). Harness has the RPM +
   > VirtualQueryEx + numpy machinery ready.
   > **Run 2 (change-detection added):** hold a MIDI note → two heap snapshots 60 ms apart → keep only objects whose
   > voices have MOVING phase fields (`+0x7c/0x80/0xbc`). This narrowed 149 regions → **1 candidate** (vs 83/223) — the
   > *method* works. BUT only **~100 words moved in 60 ms**; a truly sounding voice moves its phase accumulator every
   > sample (~2600 words/60 ms), so **100 = GUI/meter noise → the held note is NOT reaching VAZ's audio engine**
   > (loopMIDI port almost certainly not wired to the standalone's MIDI-in — `VazAuto(midi_hint='loop')` sets a hint,
   > not a guaranteed route). **Next-session step 1 = make the note audibly sound** (capture audio to confirm, or drive
   > VAZ's on-screen keyboard via computer-use) BEFORE trusting change-detection; then the single moving-voice object is
   > the engine and its true `+0x28`/phase offsets can be read off the live struct. Method proven; input path is the gap.
   > **Turnkey next-session recipe (route now known):** (1) PRECONDITION — VAZ Options→Preferences must have **MIDI In =
   > loopMIDI** + Audio Out = speaker (a one-time manual config the render harnesses already rely on; verify it's saved).
   > (2) Reuse `tools/vaz_auto`'s **shared** MidiOut (not a fresh handle) and its `note_transpose=12`; hold note `48+12`.
   > (3) Add a soundcard **loopback capture** (the `clone_diag.py` pattern: `sc.all_microphones(include_loopback=True)`)
   > and assert peak > −40 dB — this CONFIRMS the voice is sounding before the heap diff. (4) With audio confirmed, the
   > change-detection will show ~5000+ moving words (output ring + voice phase); the one object whose 32-ptr array
   > points at moving voices is the engine. (5) Read the live voice struct to VERIFY `+0x28`(LFO)/`+0x7c`(phase) offsets
   > against the decompile before trusting them. Everything above the audio-confirm is built + committed in
   > `dump_engine_vmt.py`; only the confirmed-audio route + offset verification remain.
   > **Run 3 — AUDIO ROUTE SOLVED ✅ (2026-07-13):** the fix was using `vaz_auto`'s **shared** MidiOut (`shared_midiout('loop')`)
   > instead of a fresh `rtmidi.MidiOut()` handle — the note now reaches VAZ and a voice sounds: **~14 500 heap words move**
   > per 60 ms snapshot pair (was ~100 = silence). The harness now clusters the moving words: the densest cluster
   > (0x…, ~4400 CONTIGUOUS words, smooth waveform ramp) is the **audio output buffer**; per-voice DSP state is in the
   > smaller, repeated-span (0xEFC) clusters. **Remaining = pure offset RE (no longer blocked):** discriminate the sparse
   > voice-struct fields (phase acc = single word w/ big constant delta; LFO phase = single word w/ SMALL constant delta;
   > filter state) from the bulk audio/oscillator buffers, then read the LFO waveshape/mode. The hard part (getting live
   > sounding DSP state out of VAZ) is done and reproducible; unblocks mod-LFO #8 / Osc3 #9 / multi-saw #6 / v2p Phase-4.

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
