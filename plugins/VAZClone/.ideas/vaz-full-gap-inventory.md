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

- **Osc3 audio-rate** (LFO1 key-tracked; footage 32'=48..2'=240) — mixer opt exists, no DSP. `osc3_footage` = **UNVERIFIED (anchors ambiguous)**.
  - **Empirical decode (2026-06-23, VazOracle):** ported VAZ's Osc3-increment x87 sequence `round(2^31·48·rateVal / (60·noteVal·footage))` (0x4dbf12-0x4dbf43; C=INT_MAX@0x4de53c, 60.0@0x4de538). Ran an anchor test (32'=byte48→f/4, 8'=byte144→f, 2'=byte240→4f) over 6 operand-role hypotheses: **TWO survive** — `rate=footMul, foot=1` (**= the clone's `2^((byte−144)/48)`**) AND `rate=footMul·b, foot=b`; both give exactly 0.25/1/4. → the octave exponential lives in **rateVal** (voice+0xbc = FUN_004a0a68 LFO1 audio-rate value), NOT in this render ratio, so the anchors can't disambiguate. **The clone's formula is anchor-CONSISTENT (a surviving hypothesis), NOT proven wrong** (earlier "form deviation" retracted). **NOT fixed — ≥2 survive (per "gæt ikke").** To resolve: RE where rateVal is set from the footage byte (the LFO1 audio-rate setup, upstream of this render). Caveat: MSVC `long double`=64-bit vs VAZ's x87 80-bit → port is best-effort. **Separate note:** the clone sources footage from the `lfo_rate` param, not the `.v2p +0x94` field (which it discards) — a source mismatch to confirm if Osc3 DSP is ever wired.
- **Microtuning / .tun loader** — clone is hard 12-TET.
- **Full Sample Loader** (multisample/Drums/loop modes).
- **Arp**: Random 2, Trigger-Free, exact Range semantics.
- **Sequencer** (whole subsystem) → brings Accent / Seq A / Seq B mod sources online (currently 0).
- **MIDI CC-map dialog**, Program-Change patch switch, per-synth MIDI channel.

## 9. Honest headline

- **Proven bit-exact (test or line-by-line):** .v2p param mapping (all 260 files), envelope recurrence, **detune spread (poly + unison)**, **cutoff base smoother**, filter **A + R**, cutoff dispatch/map, mod-source bipolarity, mixer/voice/MIDI structure. *(detune + smoother FIXED this session, VazOracle BIT-EXACT.)*
- **Known deviations (TILNÆRMET):** D-HP+LP / Comb (float), mixer src restriction, Dynamic-mode semantics.
- **Unresolved (needs more RE):** **Osc3 footage** — anchor test leaves 2 surviving operand hypotheses (one IS the clone's formula, so it's anchor-consistent, NOT proven wrong); disambiguation needs the LFO1 audio-rate `rateVal` source decoded. No fix (per gæt-ikke).
- **Unproven (PÅSTÅET) — do NOT call these done:** filter B/C/D/K line-by-line, LFO 8-waveforms + sync ratios, pulse-width map, velocity/keytrack polarity.
- **Missing (MANGLER) — Osc3 is a confirmed data-carrying gap:** Osc3 DSP + its footage fields (+0x94/+0xe0/ded84), microtuning, sample loader, arp completeness, sequencer, MIDI-map.

**Prioritisation is yours** — this matrix only reports; no deviations were fixed in this session.
