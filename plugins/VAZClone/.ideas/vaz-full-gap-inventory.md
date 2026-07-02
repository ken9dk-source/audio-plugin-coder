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

| Discarded field | Ghidra ref | parseV2P line | Status | Note |
|-----------------|-----------|---------------|--------|------|
| **voice_count** | v≥0x6d u32 | :394 | MANGLER (per-preset) | Clone uses a global Voices param; VAZ stores it per preset. [FLAGGED] |
| +0x94 / +0xe0 / +0xded84 | v≥0xc9 | :396,400,401 | PÅSTÅET (unknown) | LFO1 v2.0 fields, purpose unconfirmed. [FLAGGED] |
| LFO2 trig-mod src+depth | v≥200 | :403 | MANGLER | Clone does not model LFO2 trig modulation. [FLAGGED] |
| e04f4/e0504 extra mod slot | v≥0x65 | :459-460 | MANGLER | An extra mod-src+depth slot not modeled. [FLAGGED] |
| e05f0/e0600 | u32+byte | :461 | PÅSTÅET | glide/legato flags, not modeled. [FLAGGED] |
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
| **cutoff base smoother** | DAT_006d45e4=6603751 (Q32) | SynthVoice cutAlpha=0.00154 | **TILNÆRMET** | VazOracle max diff 0.000585 (rounded) |

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
| **Detune spread** | FUN_004e0618 + DAT_0052b168/0x52b0ec (deterministic bit-reversed) | seeded random | **TILNÆRMET** | VazOracle: different algorithm |
| Dynamic voice-free on env-close | FUN_004de6cc (param+0x78) | full-pool + release-free | TILNÆRMET | semantics differ |

---

## 8. Missing features (unchanged from prior research — all MANGLER)

- **Osc3 audio-rate** (LFO1 key-tracked; footage 32'=48..2'=240) — mixer opt exists, no DSP. `osc3_footage` = **NOT TESTED** in oracle.
- **Microtuning / .tun loader** — clone is hard 12-TET.
- **Full Sample Loader** (multisample/Drums/loop modes).
- **Arp**: Random 2, Trigger-Free, exact Range semantics.
- **Sequencer** (whole subsystem) → brings Accent / Seq A / Seq B mod sources online (currently 0).
- **MIDI CC-map dialog**, Program-Change patch switch, per-synth MIDI channel.

## 9. Honest headline

- **Proven bit-exact (test or line-by-line):** .v2p param mapping (all 260 files), envelope recurrence, filter **A + R**, cutoff dispatch/map, mod-source bipolarity, mixer/voice/MIDI structure.
- **Known deviations (TILNÆRMET):** cutoff smoother (rounded 0.00154), **detune spread (random vs VAZ's deterministic table)**, D-HP+LP / Comb (float), mixer src restriction, Dynamic-mode semantics.
- **Unproven (PÅSTÅET) — do NOT call these done:** filter B/C/D/K line-by-line, LFO 8-waveforms + sync ratios, pulse-width map, velocity/keytrack polarity, the [FLAGGED] discarded .v2p fields.
- **Missing (MANGLER):** Osc3 DSP, microtuning, sample loader, arp completeness, sequencer, MIDI-map.

**Prioritisation is yours** — this matrix only reports; no deviations were fixed in this session.
