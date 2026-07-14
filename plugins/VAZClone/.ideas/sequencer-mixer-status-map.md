# VAZ 2010 → clone — SEQUENCER + MIXER STATUS MAP (2026-07-14)

**Scope:** research/mapping ONLY for two panels slated for LATER implementation — the Step
**Sequencer** and the **Mixer**. **No code in this pass.** Same style + intent as
`synth-engine-status-map.md`: for each panel, two layers — **(A) decompiled DSP/logic** in
`Vaz2010Core.dll`, and **(B) DFM GUI structure** (control names/ranges, same method as the
filter-label DFM extraction, `tools/dump-dfm.ps1`) — plus an initial **status assessment**
(what's known, what's missing) as the starting point for implementation.

## STATUS LEGEND
- ✅ **KNOWN** — located + understood, ready to transcribe
- 🔶 **PARTIAL** — located, exact details still need RE
- ❓ **UNKNOWN** — not yet located in the decompile
- ⬜ **MISSING (clone)** — not present in VAZClone

---

## 1. STEP SEQUENCER — class `TSequencer`

### 1A. DSP / logic (Core.dll)
| Item | Status | Evidence / next |
|---|---|---|
| Class identity | ✅ | `TSequencer` present in decompile symbol refs |
| Pattern container + naming | 🔶 | `tools/vaz_big.c:3762-3917` builds `"Sequencer Pattern N"` names (pattern slots + save/recall loop) — the pattern *container* is here; the per-step data layout still needs mapping |
| Step-advance / timing / sync | ❓ | NOT located. Need the per-clock step tick and the **Timebase / Swing / Free / Perf2** modes (from the gap list, `VAZ_Clone_Audit.md:66`). Method: RPM/VMT-dump `TSequencer` off the running synth (reuse the engine-dump harness, `synth-engine-status-map.md §3.0`) → find the MIDI-clock / audio-block callback that advances the step |
| Per-step output | ❓ | note-out + **Accent / Slide / Rest** application per step; how Slide drives portamento; how Accent scales velocity/level |
| Song / pattern chain | 🔶 | `edSongStep1..34` = a 34-slot song chain of patterns (walk order + loop points TBD) |
| Mod outputs **Seq A / Seq B** | 🔶 | the sequencer emits **two modulation lanes** consumed by the mod-matrix (Seq A/B are mod SOURCES) — separate from the note stream. Values/ranges TBD |

### 1B. GUI / DFM (control-name strings dumped from Core.dll)
- **Step range:** `cbStartStep` · `cbEndStep` · `edFinalStep`
- **Per-step flags:** `cbAccent` · `cbSlide` · `cbRest`
- **Song mode:** `edSongStep1 … edSongStep34` (+ `edChild`, `edExchange` for pattern edit ops)
- **Selection:** `cbSelectFrom` · `cbSelectTo`
- **Pattern I/O:** `cbLoadSeq` · `cbSaveSeq`
- **Note filter / routing:** `cbNoteLow` · `cbNoteHigh` · `cbThruChannel`
- **Arp tie-in:** `cbArpFreeRun` · `cbDouble`
- Exact Min/Max/Position/Increment: extract per name with `tools/dump-dfm.ps1` (anchor on each string).

### 1C. Clone status: ⬜ **MISSING entirely**
Documented gap (`VAZ_Clone_Audit.md:66`, `architecture.md:71` — "arp and sequencer to v2").
No sequencer params, no DSP, no GUI panel in VAZClone. This is a **from-scratch build** — the
biggest single missing subsystem, and it unlocks the **Seq A / Seq B** mod sources + **Accent**.

---

## 2. MIXER — 3-channel oscillator mixer → filter

### 2A. DSP / logic (Core.dll)
| Item | Status | Evidence / next |
|---|---|---|
| Signal position | ✅ | Oscillators → **Mixer (3 ch)** → Filter → Amplifier (`VAZ_Synth_Gap_Analysis.md:9`) |
| Per-channel structure | ✅ (spec) | each of 3 channels = 1-of-**6 sources** {Osc1, Osc2, Osc3, Ring Mod, Noise, Ext, ModAmp1/2} + a **Level** + a **Post** flag |
| **Post** semantics | ✅ | Post = route that channel AFTER the filter (bypass) rather than into it |
| Drive-into-filter | ✅ | summed level into the filter sets Type **K + R** drive character (Mixer.htm; clone already drives K+R — `VAZ_Clone_Audit.md:38`) |
| **Ring Modulator** (Osc1×Osc2) | 🔶 | a mixer source; spec known, exact DSP location ❓ |
| **Osc3** | 🔶 | = LFO1 at **audio rate** tracking the keyboard, footage 32'…2'; exact pitch/footage mapping ❓ |

### 2B. GUI / DFM
- `lbMix` (panel label) · `sbMixDepth1..3` (the three channel Level sliders, from the
  `dump-dfm.ps1` anchor set) · per-channel source dropdowns + Post buttons (RE the exact
  `ms*/cb*` names next).
- ⚠ NB: `msChannel1..16` strings are **MIDI-channel** selectors, **not** the audio mixer.

### 2C. Clone status: 🔶 **PARTIAL — scaffold already present**
`SynthVoice.h:54-56` already declares `mix1Src/mix2Src/mix3Src`, `mix1Post/mix2Post/mix3Post`,
and `osc3FootMul/osc3Wave/osc3Shape`; ParameterIDs + GUI dropdowns exist (`mix1_src` … , the
`mix2_src`/`mix3_src` Track-B audit passed). **Remaining gaps:** verify the 6-source routing is
fully wired, the Post (post-filter) path, Ring Mod, Osc3 audio-rate, and ModAmp-as-source.
Much smaller than the sequencer — mostly finishing + verifying existing scaffolding.

---

## 3. WHEN IMPLEMENTATION STARTS (not now — mapping only)
1. **Sequencer (large, from scratch):** RPM/VMT-dump `TSequencer` off the running synth (reuse
   `tools/dump_engine_vmt.py` machinery) → transcribe the step-tick + Timebase/Swing/Free/Perf2
   timing, per-step Accent/Slide/Rest, the 34-slot song chain, and the Seq A/B mod outputs. Then
   design the GUI panel from the DFM control list above.
2. **Mixer (finish scaffold):** confirm/complete the 6-source routing + Post + Ring Mod + Osc3
   against the decompile; the clone's `SynthVoice.h` fields are already the skeleton.
3. **DFM ranges:** run `tools/dump-dfm.ps1` anchored on the control names in §1B / §2B to pull
   exact Min/Max/Position/Increment for both panels.

**Sources:** `tools/VAZ_Clone_Audit.md`, `tools/VAZ_Synth_Gap_Analysis.md`, `tools/vaz_big.c`
(§3762-3917), Core.dll DFM strings, `tools/dump-dfm.ps1`.
