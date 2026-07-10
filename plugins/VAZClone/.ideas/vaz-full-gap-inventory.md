# VAZ 2010 → clone — PARITY MATRIX (rewritten 2026-06-23)

**Status legend (strict — we have claimed "parameter-complete" twice and been wrong):**

| Status | Means |
|--------|-------|
| **VERIFICERET** | A concrete automated test (VazV2PAudit / VazOracle / VazDSPAudit) OR a line-by-line bit-exact address citation proves it. |
| **TILNÆRMET** | Implemented but knowingly differs from the binary (rounded constant, float port of integer DSP, different algorithm). |
| **PÅSTÅET** | Asserted to match but NOT verified line-by-line and NOT covered by a test. Treat as unproven. |
| **MANGLER** | Field/feature exists in VAZ, not implemented in the clone. |
| **NOT TESTED** | Reference not yet extractable. |

**Verification assets:** `VazV2PAudit` (DEL 1 — every .v2p byte on all 260 factory files is param-mapped or whitelisted), `VazOracle` (DEL 2 — primitives diffed vs raw Ghidra constants), `VazDSPAudit` (click/aliasing/DC/gain), plus the module RE in `[[project_vazclone_juce]]` memory.

---

## 1. Preset parsing — param → byte mapping  (all rows VERIFICERET by VazV2PAudit)

Every mapping below is proven by the no-discard audit: the labeled mirror consumes the byte-identical range as the real `parseV2P` across all 260 factory presets (v103–v202). Clone loader = `PluginProcessor.cpp` parseV2P/loadV2P.

| Parameter | Ghidra ref | Clone (parseV2P line) | Status | Method |
|-----------|-----------|----------------------|--------|--------|
| lfo1 rate/wave/shape/trig | osc-loader stream | :397-399 | VERIFICERET | VazV2PAudit |
| lfo2 rate/trig/mode/delay | stream | :402-407 | VERIFICERET | VazV2PAudit |
| lfo3 sel/wave | +0x108/+0x10c | :407 | VERIFICERET | VazV2PAudit |
| env1 A/D/S/R + mode | stream (ver-gated) | :409-413 | VERIFICERET | VazV2PAudit |
| env2 A/D/S/R + mode + seg-mod | +0x174 block | :417-424 | VERIFICERET | VazV2PAudit |
| ModAmp1/2 in/sq/am | e0460… | :425-429 | VERIFICERET | VazV2PAudit |
| osc1/2 tune/wave/shape/sync + FM×2 + PWM | stream | :431-442 | VERIFICERET | VazV2PAudit |
| mixer ch1/2/3 src/level/post | dfd2c… | :444-448 | VERIFICERET | VazV2PAudit |
| filterMode/cutoff/reso/bw/hp | +0x258… | :449-450 | VERIFICERET | VazV2PAudit |
| cutoff-mod ×3, res-mod, amp-mod ×3 | mod slots | :451-457 | VERIFICERET | VazV2PAudit |
| overdrive/voiceMode/bendRange | e0530/e0610 | :458-462 | VERIFICERET | VazV2PAudit |
| uniVoices/uniDetune/polyDetune/portamento | e095c/p2f4/p2f0 | :463-466 | VERIFICERET | VazV2PAudit |

**Read-and-DISCARDED bytes (documented in the audit whitelist; [FLAGGED] = candidate to wire up):**

Classified 2026-06-23 (Step 4 — report only, no fixes):

| Discarded field | Ghidra ref | parseV2P line | Verdict | Finding |
|-----------------|-----------|---------------|---------|---------|
| **voice_count** | v≥0x6d u32 | :394 | REAL (per-preset) | Clone uses a global Voices param; VAZ stores it per preset. |
| **+0x94** | v≥0xc9 u32 | :396 | **REAL — Osc3** | LFO1 audio-rate FOOTAGE. Setter FUN_004d6c3c:125-129; USED in the Osc3 increment x87 calc @0x4dbf1b. Part of the Osc3 gap. |
| **ded84 byte** | v≥0xc9 byte | :400 | **REAL — Osc3/LFO1** | Osc v2.0 property via setter FUN_004ded84 (FUN_004d6c3c:148-152), in the LFO1/Osc3 load region. |
| **+0xe0** | v≥0xc9 u32 | :401 | **REAL — Osc3** | 2nd audio-rate FOOTAGE (mirrors +0x94; render +0xe8 block @vaz_big.c:69-79). |
| LFO2-section mod src+depth | v≥200 | :403 | NICHE (unconfirmed) | A modsrc+depth pair loaded in the LFO2 block; exact destination not pinned. |
| e04f4/e0504 mod src+depth | v≥0x65 | :459-460 | NICHE (unconfirmed) | Extra mod-src+depth slot; destination unconfirmed (not one of the clone's 3 cut / res / amp slots). |
| e05f0/e0600 | u32+byte | :461 | NICHE (performance) | Copied to +0x70c/+0x71c; +0x600 → FUN_004a2d44 (vaz_big.c:5885). Voice/performance flags (glide/legato/Link-ish). |

> **Takeaway:** three flagged fields (**+0x94, +0xe0, ded84**) are all **Osc3 / audio-rate footage** — confirming Osc3 is a real *data-carrying* gap (the .v2p stores footage the clone discards, and there is no Osc3 DSP). The rest are niche mod-slots / performance flags. **None fixed this session (report only).**
| sample One-Shot/No-Trigger flags | MSmp trailing byte | :436,442 | MANGLER | Sample-osc loop-mode bits; clone sample osc is basic. |
| env/filter trailing flag bytes | various | :414,421,449… | PÅSTÅET | Assumed inert; not individually confirmed. |

---

## 2. Oscillators

| Parameter | Ghidra ref | Clone | Status | Method |
|-----------|-----------|-------|--------|--------|
| Wavetable interpolation | vaz_big.c:171-173 = 2-pt LINEAR, 256-entry | Synth.h:76 (linear, post-fix) | VERIFICERET | line-by-line address citation |
| Sample-osc interpolation | vaz_big.c:979-991 = 4-pt cubic | Synth.h:108 (cubic, post-fix) | VERIFICERET | address citation |
| Multi-saw detune ±36c | FUN_004dbddc:386-432 (not decoded to cents) | Synth.h:171-184 | TILNÆRMET | from FFT measurement, not the binary |
| Pulse width map | osc case, waveshape→pw | Synth.h:160-169 | PÅSTÅET | square-at-0.5 asserted; VAZ pulse path not located |
| Osc phase (32-bit, free-run) | +0x110/+0x114 | Synth.h OscBlock | VERIFICERET | address citation |

## 3. Filter (6 engines)

| Engine / part | Ghidra ref | Clone | Status | Method |
|---------------|-----------|-------|--------|--------|
| Dispatch (mode=param+0x258, tap=mode&3) | vaz_big.c:1093,1391 | VAZMultiFilter::setMode | VERIFICERET | address citation |
| Cutoff map fc=exp(10.24·idx/1024) | @0x4D4720 | SynthVoice coNorm→cutHz | VERIFICERET | address citation |
| 2D coef index ri·1024+ci | vaz_big.c:1114 | VAZTypeR.h:49 | VERIFICERET | address citation |
| **Type A LP** | vaz_big.c:1112-1150 (0x5d/59/55) | VAZTypeA.h | **VERIFICERET** | line-by-line |
| **Type R** (default) | vaz_big.c:1199-1282 (0x69/65/61) | VAZTypeR.h | **VERIFICERET** | line-by-line |
| Type A HP/BP + Type B | 0x5d/59/55 taps | VAZTypeB.h | PÅSTÅET | structure matches; not line-by-line |
| Type C (biquad + separation) | 0x13-0x2d biquad group | VAZTypeC.h | PÅSTÅET | family correct; not line-by-line |
| Type D LP/HP/BP | 0x6d67e8/77e8 | VAZTypeD.h | PÅSTÅET | not line-by-line |
| Type K (Sallen-Key) | jump table | VAZTypeK.h | PÅSTÅET | not line-by-line |
| Type D HP+LP series | (2-section) | Synth.h:522-529 FLOAT | TILNÆRMET | float fallback (no factory patch) |
| Comb | ≥0x5e path | Synth.h:534 FLOAT | TILNÆRMET | float fallback |
| **cutoff base smoother** | DAT_006d45e4=6603751 (Q32=0.0015375556) | SynthVoice.h:148 cutAlpha (exact, SR-scaled) | **VERIFICERET** | VazOracle cutoff_smoother BIT-EXACT — FIXED |

> ⚠️ The previous inventory said *"all 22 filter modes bit-exact"* — that was overclaimed. Only **A + R** (≈95% of factory patches) are line-by-line verified; **B/C/D/K are PÅSTÅET**; D-HP+LP and Comb are float.

## 4. Envelope

| Part | Ghidra ref | Clone | Status | Method |
|------|-----------|-------|--------|--------|
| ADSR per-sample recurrence | vaz_big.c:405-443 | VAZEnv::getNextSample | **VERIFICERET** | VazOracle env-step BIT-EXACT + address |
| Rate table (720, Q32) | DAT_006db7e8 | VAZEnvTables kRate | VERIFICERET | runtime dump == clone table |
| Attack index offset +0xc | vaz_big.c:489 | kRate[12+a] | VERIFICERET | address citation |
| Reset stage-0 ramp | DAT_006dc0bc=48695774 | Synth.h PreAttack (post-fix) | VERIFICERET | raw constant + address |
| Curve-mode sustain LUT | DAT_006dc0bc region | kSusCurve | PÅSTÅET | dumped table used, not diffed |

## 5. LFO

| Part | Ghidra ref | Clone | Status | Method |
|------|-----------|-------|--------|--------|
| Sync rate base (24 PPQN) | FUN_004a073c (X·15360/(bpm·24)) | lfoRate lambda | PÅSTÅET | structure only; ratio table not diffed |
| 8 waveforms | LFO generator (not located) | ModLFO | PÅSTÅET | RE'd earlier, not re-verified vs binary |
| LFO3 rate table | DAT_006dc4c0 (255, dumped) | lfo3 | PÅSTÅET | dumped, not diffed |

## 6. Modulation matrix

| Part | Ghidra ref | Clone | Status | Method |
|------|-----------|-------|--------|--------|
| Mod sources BIPOLAR (env/CC/AT) | vaz_big.c:445,541; FUN_004db9b8 | SynthVoice mv()/ModBus (post-P1) | VERIFICERET | binary + before/after render demo |
| 3-slot matrix per dest (src/depth/sign) | +0x218/21c/220 | cut/fm/amp slots | VERIFICERET | address citation |
| Depth = |v| + direction bit | FUN_004de75c | loadV2P SD() | VERIFICERET | address + VazV2PAudit |
| velocity/keytrack polarity | (not confirmed) | unipolar | PÅSTÅET | VAZ bipolarity NOT verified |

## 7. Mixer / Voice / MIDI

| Part | Ghidra ref | Clone | Status | Method |
|------|-----------|-------|--------|--------|
| Mixer 3-ch + pre/post | vaz_big.c:1020-1054 | SynthVoice:274-278 | VERIFICERET | address citation |
| Mixer generic src per ch | +0x218/28/38 (any mod idx) | 3 hardcoded opts | TILNÆRMET | clone restricts src choices |
| Voice steal = oldest-busy | FUN_004db3a8:80-98 | pickVoice | VERIFICERET | address citation |
| Mono note-priority Hi/Lo/Last | param+0x2e0 | notePrio | VERIFICERET | address citation |
| Pitch-bend (8192-centred×range) | FUN_004db958 | handlePitchWheel | VERIFICERET | address citation |
| Sustain CC64 (deferred note-off) | FUN_004db8d4 | sustainPedal (post-fix) | VERIFICERET | address citation |
| **Detune spread** | FUN_004e0618 (vaz_prims.c:3316-3388); poly=DAT_0052b168[polyN], unison=(uniDetune<<9)/N; order DAT_0052b0ec | reference/vaz_detune.h + SynthVoice.h detuneOff | **VERIFICERET** | VazOracle detune BIT-EXACT (port vs literal decomp, N=2..31 × amt=0..255) — FIXED |
| Dynamic voice-free on env-close | FUN_004de6cc (param+0x78) | full-pool + release-free | TILNÆRMET | semantics differ |

---

## 8. Missing features (unchanged from prior research — all MANGLER)

- **Osc3 audio-rate** (LFO1 key-tracked; footage 32'=48..2'=240) — mixer opt exists, no DSP. `osc3_footage` = **⏸ ANKER-KONSISTENT (PARKERET)** — clone's `2^((byte−144)/48)` is one of the 2 anchor-surviving hypotheses (so functionally consistent, not proven wrong); not closable bit-exact without a runtime dump that repeatedly failed to locate the DSP struct. Parked by decision after 3 location attempts; resume only if the DSP-engine VMT/global is RE'd.
  - **Empirical decode (2026-06-23, VazOracle):** ported VAZ's Osc3-increment x87 sequence `round(2^31·48·rateVal / (60·noteVal·footage))` (0x4dbf12-0x4dbf43; C=INT_MAX@0x4de53c, 60.0@0x4de538). Ran an anchor test (32'=byte48→f/4, 8'=byte144→f, 2'=byte240→4f) over 6 operand-role hypotheses: **TWO survive** — `rate=footMul, foot=1` (**= the clone's `2^((byte−144)/48)`**) AND `rate=footMul·b, foot=b`; both give exactly 0.25/1/4. → the octave exponential lives in **rateVal** (voice+0xbc = FUN_004a0a68 LFO1 audio-rate value), NOT in this render ratio, so the anchors can't disambiguate. **The clone's formula is anchor-CONSISTENT (a surviving hypothesis), NOT proven wrong** (earlier "form deviation" retracted). **NOT fixed — ≥2 survive (per "gæt ikke").**
  - **rateVal source trace (2026-07-02, step-5 outcome):** `rateVal` = voice+0xbc = `FUN_004a0a68(LFO1)` → LFO obj+4 (free) or obj+0x68 (synced). obj+4 is set by `FUN_004a073c` (`vaz_prims.c` @0x4a073c line `*(obj+4)=param2`). Found all 6 callers of 0x4a073c (call-scan): the externals set the rate to **fixed/field values** — 0x4cbc0e passes `edx=0x78`(=120), 0x4f60bd passes an object field `[..+0x3e8]+0x1c0`; none pass a footage exponential. The footage (voice+0x94) is a **raw byte** (osc-loader `FUN_004d6c3c:125-129`) sitting in the render-ratio denominator. **→ the octave exponential is NOT in the traced chain** (rate setters use constants/fields; footage is raw). It's entangled with the LFO object rate/sync state (obj+4/0x68/0x78) and the per-note recompute — an **unexpected chain (step 5): REPORT ONLY, no fix.** Deeper RE (full LFO audio-rate object state + `param+0x234` per-osc semantics + obj+0x68 writers) required before Osc3 can be reimplemented bit-exact. Caveat: MSVC `long double`=64-bit vs x87 80-bit. **Source mismatch still noted:** clone reads footage from `lfo_rate`, not the `.v2p +0x94` field (discarded).
  - **Runtime-inspection attempt (2026-07-02, ReadProcessMemory):** launched VAZ standalone + wrote a ctypes RPM finder. Tried to locate the live DSP synth/voice struct via the HWND anchor (synth+0x508 = MIDI dispatcher `param_1[0x142]`) → the voice array (synth+0x2534). **BLOCKED:** the HWND appears in ~33 places, and with a strict signature (synth+0x2534 = 32 valid+distinct heap ptrs, voice[0] readable, voice[0]+0x28 = valid LFO ptr) → **0 valid candidates**. Root cause: `param_1[0x142]` (HWND, in `FUN_004db9b8` = the GUI/instrument object) and `param_1+0x2534` (voice array, in `FUN_004de6cc` = the DSP-engine object) are **two DIFFERENT objects**, so the +0x508 anchor doesn't reach the DSP struct. **To close via runtime dump:** first RE the DSP-synth object's VMT (fixed .rdata VA) or the global pointer the audio callback loads it from, then scan for THAT + hold a live note. Not guessed. VAZ process closed after.
  - **Attempt 2 + PARK decision (2026-07-02):** relaunched + enumerated ALL windows incl. children (59 HWNDs), re-scanned with the strict signature → still only false positives (voice[0] values were UTF-16 Delphi widestrings, not voice pointers). Then tried the static VMT path via Delphi RTTI: the class-name search returns only **GUI control classes** (`TVTMidSynth*`: SlidePot/Button/Switch/Led/… incl. `TVTMidSynthFootageButton` @0x489a44 — confirming the footage IS a real control), **not** the DSP-engine class. 3 location attempts, none unique. **DECISION: park as ANKER-KONSISTENT, no third round** (clone's formula is anchor-consistent). Resume path: RE the DSP-engine class's VMT (search for `mov [obj+0], <.rdata>` in the constructor that also inits +0x2534) or the global the audio callback loads, then repeat the runtime dump with a held Osc3 note.
- **Microtuning / .tun loader** — clone is hard 12-TET.
- **Full Sample Loader** (multisample/Drums/loop modes).
- **Arp**: Random 2, Trigger-Free, exact Range semantics.
- **Sequencer** (whole subsystem) → brings Accent / Seq A / Seq B mod sources online (currently 0).
- **MIDI CC-map dialog**, Program-Change patch switch, per-synth MIDI channel.

## 9. Honest headline

- **Proven bit-exact (test or line-by-line):** .v2p param mapping (all 260 files), envelope recurrence, **detune spread (poly + unison)**, **cutoff base smoother**, filter **A + R**, cutoff dispatch/map, mod-source bipolarity, mixer/voice/MIDI structure. *(detune + smoother FIXED this session, VazOracle BIT-EXACT.)*
- **Known deviations (TILNÆRMET):** D-HP+LP / Comb (float), mixer src restriction, Dynamic-mode semantics.
- **⏸ PARKED (ANKER-KONSISTENT):** **Osc3 footage** — anchor test leaves 2 surviving operand hypotheses (one IS the clone's formula → anchor-consistent, NOT proven wrong). Static trace hit LFO-object entanglement; 3 runtime-dump location attempts failed (HWND anchor false-positives, RTTI finds only GUI classes). Parked by decision — no fix, no guess. Resume only if the DSP-engine VMT/global is RE'd.
- **Unproven (PÅSTÅET) — do NOT call these done:** filter B/C/D/K line-by-line, LFO 8-waveforms + sync ratios, pulse-width map, velocity/keytrack polarity.
- **Missing (MANGLER) — Osc3 is a confirmed data-carrying gap:** Osc3 DSP + its footage fields (+0x94/+0xe0/ded84), microtuning, sample loader, arp completeness, sequencer, MIDI-map.

**Prioritisation is yours** — this matrix only reports; no deviations were fixed in this session.

---

## 10. FX PARITY — 7 effects (collection phase, started 2026-07-02)

**Source map (verified):** the FX DSP the clone was built from lives in **Core.dll** as Borland `TFX*` classes
(`TFXFlanger@0x51FCD4 · TFXChorus@0x5184D8 · TFXPhaser@0x52107C · TFXEqualizer@0x51E0C0 · TFXDelay@0x51AF18 ·
TFXReverb@0x522530`), decompiled in `tools/vaz_fx_all.c` (172 fns) + `.ideas` RE docs for Flanger/Chorus/Phaser.
`VAZ2010Effect.dll` (89 KB) is a **separate self-contained VST2** (exports only `main`, no `TFX` RTTI, imports no
Core.dll) — NOT the clone's source; a distinct future RE target. Effects are per-effect below.

### FX-A — Flanger (`TFXFlanger`, per-sample @0x5204F4, setup @0x520418) — AUDITED

| Param / part | Ghidra ref | Clone (VAZFlanger) | Status | Method |
|---|---|---|---|---|
| delay_time → base delay samples | `FUN_0052076c` = (sr·25/256000)·(v+1) | PluginProcessor.cpp:71 `(v+1)/10.24·sr/1000` | **VERIFICERET** | VazOracle fx_flanger_delaytime BIT-EXACT (25/256000==1/10240) |
| Topology (delay line + triangle LFO + linear interp + feedback comb + linear dry/wet + L/R phase) | @0x5204F4-0x5205C2 | FlangerChannel (matches) | **TILNÆRMET** | topology-exact float port (VAZ is Q23 fixed-point → not bit-exact per-sample) |
| Triangle LFO = abs(phase acc), NOT sine | @0x520531-34 | clone triangle | VERIFICERET | address citation |
| Mix law = LINEAR `in+mix·(delayed−in)` | @0x5205B5 | clone linear | VERIFICERET | address citation |
| Feedback max | feedback coef = [+0x264]<<23 → max 255/256 (render @0x52059c; param 0..255, linear) | :77 `fFb·255/256·sign` (was 0.92) | ✅ **VERIFICERET/FIXED** | VazOracle `fx_flanger_feedback` BIT-EXACT (both = param/256) |
| BPM sync rate | inc = round(BPM·**48**·(2^31−1)/(SR·60·period[+0x274])); ·48 @0x5204a9-ae, ·60 @0x520493 (FUN_00520418) | :83-99 `periodBeats[24]`, `(bpm/60)/beats` | ✅ **VERIFICERET** | `fx_flanger_sync` VERIFIED — clone == VAZ (periodBeats = period/48; ·48 = the 1/48-note unit) |
| Params present (delay/fb/rate/depth/lr_phase/mix/gain/fb_phase/sync/period) | TFXFlanger fields +0x264…+0x2b0 | createParameterLayout:24-35 | VERIFICERET | field set matches RE |

### FX-B — Reverb (`TFXReverb@0x522530`, render `FUN_005228a4` @0x5228a4) — AUDITED · fixed-point (Q31/Q28)

Full render loop force-decompiled (virtual method, not in prior corpus): `tools/vaz_reverb_render.c`. VAZ's reverb
is a **custom integer Schroeder network**, NOT juce::Reverb/Freeverb. ✅ **FIXED (fix #1)** — clone now ships
`VazReverbEngine` (fixed-point Q31/Q28 port; replaced juce::Reverb). Render bit-exact vs an independent transcription
(VazOracle `fx_reverb_render` BIT-EXACT over 300k-smp impulse+noise) + `fx_reverb_stable` VERIFIED.

| Param / part | Ghidra ref (VAZ) | Clone (VAZReverb / juce::Reverb) | Deviation class | Status |
|---|---|---|---|---|
| Engine | 9 parallel PLAIN Schroeder combs + 4 series allpass/ch + 1 global damping LP; integer Q31/Q28 | Freeverb: 8 **lowpass**-combs ×2ch + 4 allpass/ch; float | **FORKERT TOPOLOGI** (høj) | render @0x5228a4 line-by-line |
| Comb count / feed | 9 combs (10 coefs alloc'd @+0x274…+0x242bc, render sums 9), mono feed `(L+R)>>6` | 8 combs, independent L/R banks | **FORKERT TOPOLOGI** (høj) | @0x5228a4:82-127 |
| Comb type / damping placement | plain `buf[w]=coef·buf[r]+in`; ONE global one-pole LP *after* allpasses | Freeverb damps *inside* each comb | **FORKERT TOPOLOGI** (høj — changes decay spectrum) | @0x5228a4 (no per-comb LP) vs FUN_00523194 |
| Comb tunings (lengths) | {1116,**1187**,1277,1356,1422,1491,1557,1617, +1203,1527} @0x523158-88 | 1116,**1188**,1277,1356,1422,1491,1557,1617 (JUCE) | **FORKERT KONSTANT** + 2 extra combs | constant pool 0x523158-88 |
| Comb feedback coef | per size/decay: seed −1.35e-5 (DAT_0052314c) /((size·16+500)·sr) ·tuning[i], Q31 | roomSize·0.28+0.7 (JUCE) | TILNÆRMET ALGORITME (different decay control) | FUN_00522fcc @0x522fcc:6527 |
| Pseudo-stereo | 2 asymmetric int weighted sums of the 9 combs (L={2,1,4,2,3,4,·,2,·} R={2,3,·,2,1,·,4,2,4}) | width param on independent L/R | **FORKERT TOPOLOGI** (different stereo image) | @0x5228a4:119,125 |
| Allpass gain | g = **0.65** (DAT_0052ba54 = 0x53333333 Q31) | 0.5 (JUCE fixed) | **FORKERT KONSTANT** | DLL read @0x52ba54 |
| Allpass lengths | **{307,97,71,53}×2ch**, SR-scaled `round(SR·t/44100)`, buffers 1024 | 556/441/341/225 (JUCE) | ✅ now ported exactly | length-setter FUN_00522c60 (IMUL 0x133/0x61/0x47/0x35) |
| Dry/wet | linear `dry+mix·(wet−dry)`, param +0x268 `<<23` | juce::Reverb wet/dry (equal-power-ish) | TILNÆRMET (mix law) | @0x5228a4:172-176 |
| Sample data format | 100% integer Q31 (combs) / Q28 (damping) | float | TILNÆRMET (float choice; moot until topology matches) | @0x5228a4 all `>>0x20` |
| Params | +0x260 size, +0x264 damp, +0x268 mix (3), version-gate <300 (FUN_005232a4) | roomSize/damping/wet/dry/width/freeze | MANGLENDE/EXTRA PARAM (clone has width+freeze; VAZ has 3) | FUN_005232a4 @0x5232a4 |

**Verdict:** ✅ **FIXED** — `VazReverbEngine` (`plugins/VAZReverb/Source/VazReverbEngine.h`) is a fixed-point port of
the exact topology: 9 plain combs (SR-scaled tunings) + weighted-sum pseudo-stereo + 4 series allpass/ch (g=0.65) +
global Q28 damping + linear mix. The old "faithful Freeverb match" comment is gone.
**Bit-exactness:** the integer RENDER + all delay lengths are bit-exact (VazOracle `fx_reverb_render`; `fx_reverb_stable`
VERIFIED). ✅ **Coef residual RESOLVED (step #4):** the size→coef and damp→coef curves are VAZ's 80-bit x87 float
(FUN_00522fcc/FUN_00523194, not MSVC-reproducible), so they were **runtime-dumped** — `tools/vaz_coef_dump.cpp`
LoadLibrary's Core.dll, resolves its IAT by hand (skips the crashing DllMain), and calls the real setters at every
size/damp 0..255 → `reference/vaz_reverb_coef_lut.h` (validated: lengths dumped = tunings). The engine now uses the
exact LUT (SR-adjusted by `^(44100/sr)` since the exponent ∝ 1/sr). `fx_reverb_coef_exact` VERIFIED. Also fixed the
damp direction (was inverted). Only mode-0 sine LUT... n/a (reverb has none).

### FX-C — Chorus (`TFXChorus`, `FUN_00518ad8` @0x518ad8) — ✅ **FIXED (#4 topology + #5 base + #2 waveform)** · fixed-point

Clone now ships `VazChorusEngine` (fixed-point port; render bit-exact vs an independent transcription:
VazOracle `fx_chorus_render` BIT-EXACT over impulse+noise, modes 1&2).

| Part | Ghidra ref (VAZ) | Clone (VazChorusEngine) | Status |
|---|---|---|---|
| Delay line | ONE shared circular buf, input **(L+R)>>1 mono**; stereo via tap-diff `lrPhase·(tapC−tapB)` | now identical (mono line + tap-diff stereo) | ✅ FIXED (was 2 independent L/R bufs) |
| Tap count | **3 combined taps**, offset = base+(LFO1_k+LFO2_k)>>16 (2 LFOs summed) + linear interp | now identical (3 combined taps) | ✅ FIXED (was 6 independent reads) |
| Base delay | `((sr·50)/256000)·(delay+1)` int-div (FUN_00518fbc) | now identical | ✅ FIXED (#5) — VazOracle `fx_chorus_basedelay` BIT-EXACT (was 5+25·f) |
| Waveform modes | 0 sine-LUT · 1 trapezoid @0x518BA4 · 2 triangle @0x518C1F | now identical (engine mode = param idx) | ✅ FIXED (#2) — `fx_chorus_waveform_map` VERIFIED |
| 3 tap phases 0/120/240° | LUT step 0x55; phase ±0x55555554 | now identical | ✅ ported exactly |
| Mix / stereo law | linear, gain[+0x280]; stereo via lrPhase[+0x27c] | now identical (output filter transcribed) | ✅ ported exactly |
| **2-LFO rate (dual-LFO)** | inc1 = 80-bit(rate [+0x268], FUN_00518ffc @0x518ffc); inc2 = 80-bit(**own param [+0x274]**, FUN_00519098 @0x519098) — two INDEPENDENT LFO rates | ✅ **FIXED (struct)** — added independent `rate2` param (LFO2), inc2 = f(rate2) not inc1·1.27 → true free dual-LFO beating. Wired: layout + processBlock + WebSliderRelay + index.html slider | dual-LFO **FIXED**; the shared rate→inc *curve* f() stays **TILNÆRMET-ACCEPTERET** (below) |
| rate→inc curve + depth/level scalings | f(param) is 80-bit x87 (FUN_00518ffc/FUN_00519098/FUN_005208f0/FUN_0051a5fc); depth[+0x26c]/level2[+0x278] = raw params | **APPROX** rate≈fRate²·6, depth≈f·0.04·sr | **TILNÆRMET-ACCEPTERET** — 80-bit AND leaf setters fault standalone (div-by-0 on DllMain-uninit global; only reverb's self-contained FUN_00522c60 was dumpable). Sub-audible residual (continuous mod rates, not discrete pitch/length). No object-ctor harness (same ctor→VMT blocker as Osc3). |
| Sine LUT (mode 0) | +0x2a8 256-entry, 80-bit-built | `sin()` double LUT | TILNÆRMET (80-bit residual; modes 1/2 bit-exact) |
| Sample format | integer Q-format (all `>>0x20`) | float | TILNÆRMET | @0x518ad8 |
| Params | delay/rate/depth/lr_phase/mix/gain + 2 mode fields (+0x264,+0x270) + base(+0x260) | delay/rate/depth/lr_phase/mix/gain/waveform/sync/period | VERIFICERET (superset; clone adds sync — VAZ sync TBD) | createParameterLayout:24-35 |

**Status:** the topology (mono line + 3 combined taps + tap-diff stereo), base delay, and waveform selection are
now VAZ-exact (`VazChorusEngine`, `fx_chorus_render`/`fx_chorus_basedelay`/`fx_chorus_waveform_map` all green).
**Status:** dual-LFO structure ✅ FIXED (independent `rate2` param). The rate→inc *curve* + depth/lr/gain scalings
stay TILNÆRMET-ACCEPTERET (80-bit, not isolable-dumpable, sub-audible for continuous mod rates). Mode-0 sine LUT is
an 80-bit residual (modes 1/2 bit-exact).

### FX-D — Phaser (`TFXPhaser`, render `FUN_005218d8` @0x5218d8) — ✅ **FIXED (bit-exact)** · fixed-point (Q30)

Clone now ships `VazPhaserEngine` (fixed-point port; render bit-exact vs an independent transcription: VazOracle
`fx_phaser_render` BIT-EXACT over impulse+noise with the LFO sweeping). Replaced the float `tan()`-bilinear path.

| Part | Ghidra ref (VAZ) | Clone (VazPhaserEngine) | Status |
|---|---|---|---|
| Engine | N-stage 1st-order allpass (Q30), coef from 512-LUT, feedback, linear mix | now identical (fixed-point transcription) | ✅ FIXED — `fx_phaser_render` BIT-EXACT |
| Allpass coef | 512-LUT `(1−i·5·ln2·440/255/sr)·2^30` (FUN_00521aa0) | **runtime-dumped LUT** (`vaz_phaser_coef_lut.h`), SR-adjusted `2^30−(2^30−c)·(44100/sr)` | ✅ FIXED (was tan()) — `fx_phaser_coef_lut` VERIFIED |
| LFO→coef idx | `((\|ph\|>>16)·depth[+0x280]+center[+0x264]·0x8000)>>15` | now identical | ✅ ported exactly | 
| Stages | N = (stagesParam+1)·2 → 2..12 (FUN_00521b68 @0x521b68) | (round(f·5)+1)·2 | ✅ **VERIFICERET** (clone `2+2·round(f·5)` == VAZ) |
| Feedback | fbGain = clamp(param,±100)<<23 → max **±0.78125** (FUN_00521bf4 @0x521bf4) | param(0..100)<<23, ±invert | ✅ **FIXED** (was 0.72 guess → 100/128=0.78125) |
| LFO / mix / stereo | triangle=abs, linear mix (mix<<22 → /256), separate banks +0x2b0/+0x2e0 + lrPhase·−2^24 | now identical (transcribed) | ✅ VERIFICERET |
| inGain [+0x298] / rate inc [+0x294] | Q30 input scale / 80-bit LFO rate | inGain=unity 2^30 (no setter writes +0x298); rate≈fRate²·20 | inGain ASSUMED unity; **rate TILNÆRMET-ACCEPTERET** (80-bit, sub-audible) |

### FX-E — Delay (`TFXDelay`, render `FUN_0051bba8` @0x51bba8) — ✅ **FIXED (bit-exact)** · fixed-point

Clone now ships `VazDelayEngine` (fixed-point port; render bit-exact vs an independent transcription: VazOracle
`fx_delay_render` BIT-EXACT over impulse+noise, **all 3 modes**). VAZ uses INTEGER taps (no interpolation).

| Part | Ghidra ref (VAZ) | Clone (VazDelayEngine) | Status |
|---|---|---|---|
| Engine | stereo circular buf (int taps), damped feedback 1-pole, sep L/R | now identical (fixed-point transcription) | ✅ FIXED — `fx_delay_render` BIT-EXACT |
| Modes [+0x260] | 0 Stereo (self fb) · 1 Ping-Pong (**cross** fb: L←tapR, R←tapL) · **≥2 = serial Double** (mono → L-delay → R-delay → dual-mono) | now all 3 identical | ✅ FIXED — *(corrects my earlier audit: mode 2 is a **serial double**, not simple mono; the clone's "Double" was close but the exact chaining differed)* |
| Feedback damping | 1-pole `w = x + (state−x)·k`, k = damp/2^28 (Q28, FUN_0051c298 80-bit) | now uses the **runtime-dumped** tone→damp LUT (`vaz_delay_damp_lut.h`, 256 vals, SR-adjusted) | ✅ **FIXED** — `fx_delay_maps` VERIFIED (was a too-bright approx: VAZ k=0.989..0.9999, always dark) |
| Q-formats | feedback tap·fb/256 (fb 0..255), dry/wet Q30 (v·g/2^30) | now identical | ✅ ported exactly |
| Delay-time (free) | **LINEAR** in ms: delayL = ms·(SR/1000)+((SR/1000)>>4) (FUN_0051c1cc @0x51c1cc, INTEGER) | now exact (was logarithmic 5·1200^p) | ✅ **FIXED** — `fx_delay_maps` VERIFIED (extracted, not dumped — it's integer) |
| Delay-time (sync) | 80-bit BPM (FUN_0051b9b0 @0x51b9b0, FUN_00402bf4) | note-table × musical mult | TILNÆRMET-ACCEPTERET (80-bit BPM, like flanger/phaser sync) |
| Buffer | next_pow2(sr·2550/1000) (FUN_0051bf78) | now identical | ✅ exact |

### FX-F — Autopan (`TFXAutopan`, render `FUN_00517d34` @0x517d34) — AUDITED · fixed-point · **cleanest match**

| Part | Ghidra ref (VAZ) | Clone (VAZAutopan) | Deviation class | Status |
|---|---|---|---|---|
| Pan law | CONSTANT-POWER via 257-LUT: gainL=LUT[255−idx], gainR=LUT[idx] | `gL=cos(pan·π/2), gR=sin(pan·π/2)` | **TILNÆRMET-BEVIDST** (cos/sin = exact continuous form of the LUT; float chosen, law verified) | @0x517d34:713-726 vs .cpp:91-92 |
| LFO waveform | mode0 triangle (`\|ph\|>>9`) · mode1 256-entry sine LUT (+0x684) | triangle default · sine bool | VERIFICERET (matches) | @0x517d34:699-708 vs .cpp:88-89 |
| Pan range | pos = min[+0x260] + lfo·(max[+0x264]−min) | leftP + lfo·(rightP−leftP) | VERIFICERET | @0x517d34:709 vs .cpp:90 |
| Rate | inc[+0x27c] (map not extracted) | 0.1..20 Hz (f²) / sync | **PÅSTÅET** (VAZ rate map not pinned) | .cpp:76 |

### FX-G — Decimator (`TFXDecimator`, render `FUN_0051dbcc` @0x51dbcc) — ✅ **FIXED (fix #3)** · fixed-point

Clone now ships `VazDecimatorEngine` (render bit-exact vs an independent transcription: VazOracle `fx_decimator_render`).

| Part | Ghidra ref (VAZ) | Clone (VazDecimatorEngine) | Status |
|---|---|---|---|
| Bit reduction | **truncation** `held=(in & mask)+bias`; mask=`(0xFFFFFFFF>>s)<<s`, bias=`((1<<s)−1)>>1`, s=24−bits (FUN_0051dd44) | now identical (truncation + half-LSB bias) | ✅ FIXED (was round-to-level) |
| Post filter | DC-blocker after S&H: `out=(coef·(state+held)·16)>>32; state=out−held` (coef +0x280) | now ported (DC-blocker; coef from SR — 80-bit residual) | ✅ FIXED (was missing) |
| SR reduction | 11-bit acc [+0x268]>0x7ff → S&H; rate=(srP+1)·192000/SR (FUN_0051dd14) | now identical (VAZ rate formula + accumulator) | ✅ FIXED (was float accum) |
| Params | 2: SR-reduction (+0x260, dflt 0xff) + bits (+0x264, dflt 16 via 0x10) | sample_rate→srParam(0..255), bit_depth→bits(1..16) | VERIFICERET |

**Residual:** the smoothing-filter coef is VAZ's 80-bit x87 (FUN_0051dc7c). Runtime dump was **attempted** (step #4,
same harness as reverb) but FUN_0051dc7c → FUN_0051dd14 faults with an integer divide-by-zero even with the SR double-
indirection set — the decimator object needs more fields initialised than we can safely fake standalone. Per the
"no-guess" fallback, we KEEP the DC-blocker substitute (render algebra fixes DC-gain=0 for any coef<2^28, so it IS a
DC-blocker). Render + mask + bias + rate remain bit-exact.

### FX still pending (out of the 7 for this request)
| Effect | Core.dll | Clone | Status |
|---|---|---|---|
| Equalizer | `TFXEqualizer@0x51E0C0` (4 RBJ biquads) | VAZEqualizer | not audited (separate from the 7; TFXComp @0x519E9C also un-audited) |

---

## 11. PRIORITISED FX DEVIATIONS (all 7 audited — audibility × frequency)

**Every VAZ FX core is INTEGER fixed-point (Q-format); every clone is float.** Where topology+constants are
verified and float is a deliberate choice → *TILNÆRMET-BEVIDST* (Autopan pan-law, Flanger/Phaser topology); a
later fixed-point port to bit-exactness is a separate prioritisation decision, not done here.

**HØJESTE (wrong engine / audible on ~every preset):**
1. ✅ **FIXED — Reverb** — `VazReverbEngine`: exact 9-comb + weighted-sum pseudo-stereo + global-damping Schroeder
   (allpass g=0.65, SR-scaled tunings incl. allpass {307,97,71,53}). Render BIT-EXACT (`fx_reverb_render`) + stable.
   **Coefs now EXACT** (runtime-dumped, step #4 → `vaz_reverb_coef_lut.h`; `fx_reverb_coef_exact` VERIFIED) — no residual.
2. ✅ **FIXED — Chorus waveform mode 1↔2 swap** — now idx1=trapezoid, idx2=triangle (VAZ order).
   VazOracle `fx_chorus_waveform_map` VERIFIED; both suites green. @0x518ad8 modes 1/2.
   *NB:* clone trapezoid slope is still an approximation of VAZ's `clamp(2·tri−0.5,0,1)` — deferred to the chorus round.
3. ✅ **FIXED — Decimator** — replaced round-to-level with VAZ's `(in & mask)+bias` truncation + the post-S&H
   DC-blocker + VAZ's 11-bit rate accumulator (`VazDecimatorEngine`). Render BIT-EXACT (VazOracle `fx_decimator_render`).
   Residual: smoothing coef (80-bit) → DC-blocker substitute.
4. ✅ **FIXED — Chorus topology** — `VazChorusEngine`: mono-summed shared delay + 3 combined-LFO taps + tap-diff
   stereo (replaced the independent-L/R 6-tap). Render BIT-EXACT (VazOracle `fx_chorus_render`, modes 1/2).

**MEDIUM (audible on some presets):**
5. ✅ **FIXED — Chorus base-delay** — clone now uses VAZ's exact `((sr·50)/256000)·(delay+1)` (FUN_00518fbc);
   VazOracle `fx_chorus_basedelay` BIT-EXACT.
6. ✅ **FIXED — Delay** — `VazDelayEngine` (fixed-point): all 3 modes bit-exact (`fx_delay_render`), incl. mode 2 =
   VAZ's exact **serial double** (mono→L-delay→R-delay→dual-mono; my earlier "mode 2 = mono" audit was wrong). Q28
   damping + Q30 dry/wet exact. Residual: delay-time + tone→damp curves (80-bit, accepted).
7. ✅ **FIXED — Phaser** — `VazPhaserEngine` (fixed-point): render BIT-EXACT (`fx_phaser_render`), coef from the
   **runtime-dumped 512-entry LUT** (`fx_phaser_coef_lut` VERIFIED, replaced tan()), feedback fixed 0.72→0.78125,
   stages verified. Residual: rate inc (80-bit, accepted) + inGain assumed unity.
8. ✅ **VERIFICERET — Flanger BPM-sync** — VAZ `inc=BPM·48·(2^31−1)/(SR·60·period)` (·48 @0x5204a9-ae, FUN_00520418);
   clone `(bpm/60)/periodBeats` is equivalent (periodBeats = period/48). `fx_flanger_sync` VERIFIED.
   Also ✅ **flanger feedback** fixed 0.92 → 255/256 (`fx_flanger_feedback` BIT-EXACT, render `<<23` @0x52059c).

**LAV / bevidst / inaudible:**
9. Autopan pan-law (257-LUT vs cos/sin) — mathematically the same equal-power law.
10. Reverb comb 1188→1187 & allpass 0.5→0.65 — subsumed by #1.
11. Q-format→float across all 7 — deliberate; only matters after topology matches.

**PÅSTÅET rest (step #3) — final decoded status:** flanger sync+feedback ✅ VERIFICERET/FIXED. The remaining rate/scale
maps are all decoded to their setters (with addresses) and confirmed **80-bit x87**: chorus rate `FUN_00518ffc`
@0x518ffc & inc2 `FUN_00519098` @0x519098 (inc2 is an **independent** param [+0x274] — clone's ·1.27 ratio is
structurally wrong), phaser/flanger rate `FUN_005208f0` @0x5208f0, autopan rate `FUN_0051a5fc` @0x51a5fc (also uses a
global `PTR_DAT_0052c0fc`). depth[+0x26c]/level2[+0x278] are raw params (`FUN_00519078`/`FUN_00519114`).
**Runtime dump does NOT extend to these:** unlike the reverb's self-contained `FUN_00522c60` (SR from the object), the
leaf rate-setters divide by **DllMain-initialised global/object state** that's zero standalone → they fault (integer
div-by-0) when called in isolation (confirmed: chorus + decimator both crash). Exact values would need a full
object-construction harness (ctor→VMT, the remaining blocker) or a running VAZ host. Per no-guess: **kept the
approximate maps (structure+addresses documented); status TILNÆRMET, not VERIFICERET.**

> **Scope:** all 7 requested effects audited (Flanger, Reverb, Chorus, Phaser, Delay, Autopan, Decimator).
> Render loops for the 6 non-Flanger effects were force-decompiled from the virtual methods (`vaz_reverb_render.c`,
> `vaz_fx_render4.c`, `vaz_delay_render.c`; `vaz_fx_all.c` for Chorus). Constants → `reference/vaz_fx_constants.h`.
> Collection phase — **no DSP fixes applied.** EQ + TFXComp remain for a later pass.
