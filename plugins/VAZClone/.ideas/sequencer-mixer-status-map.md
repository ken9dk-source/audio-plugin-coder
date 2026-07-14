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

## 2. OUTER MIXER LAYER — the standalone rack that HOSTS the synth  (CORRECTED SCOPE)

> **Scope correction (2026-07-14):** this is NOT the oscillator-mixer inside the synth voice
> (Osc1/Osc2/Noise → filter). **That internal mixer is already covered** — `SynthVoice.h:54-56`
> (`mix1/2/3Src`, `mix1/2/3Post`, `osc3*`), loaded from `.v2p`, Track-B GUI audit passed → treat it
> as done, see §3A. THIS section is VAZ's **outer Mixer**: the titlebar 'Mixer' with 'Synth 1' /
> 'Synth 2' … nested inside (a rack), possibly multi-timbral.

### 2A. What it is (Core.dll strings)
A full standalone **host/rack subsystem**, not part of the DSP core:
- `MixerCore` · `MixerForm` · `MixerUnit` · `MixerStr` · `MixerMidiControlUnit` · `MixerMidiDlg`
  · `MixerSourcePopup` / `MixerSourceItemClick` — a mixer of **MixerUnit** slots with MIDI-mapped
  sources + per-source popups.
- Per-synth-instance controls: `cbConfirmSynthClose` (a slot is closable), `gbSynthOptions`,
  `gbSynthTuning`, `lbSynthClick` → **multiple, independently-closable synth instances**.
- `PluginChainer` / `FXPluginChainer` / `FXPluginChainerForm` + `CoCreateInstance(Ex)` — it can
  also **host external VST/COM plugins** in the chain.
- `Send to Gate` / `Send to Tempo` — send-style routing between units.

### 2B. Multi-instance vs one multi-timbral engine ❓ (needs RE)
The `MixerUnit` + closable `Synth N` slots strongly imply **N parallel engines** (each a MixerUnit
wrapping a `TBaseMidSynth`, keystone VMT @`coreBase+0xd4614`) — a rack, NOT one multi-timbral
engine. **Not yet in code:** the `MixerCore` instantiation loop / per-unit engine array live in the
app-shell, which is OUTSIDE the decompiled DSP subset (`tools/vaz_*.c` cover the engine, not the
host). Confirming = RPM/VMT walk of the running standalone (count live `TBaseMidSynth` instances) —
same harness as `dump_engine_vmt.py`.

### 2C. VST vs standalone — the decisive integration fact ✅
`CreateVAZ2010Standalone` + `TStandaloneTransport` are STANDALONE-only. The Mixer / PluginChainer /
multi-synth rack is a **standalone HOST feature**. A VST instrument is single-timbral by contract,
so VAZ's VST exposes **ONE** engine — the DAW's mixer replaces VAZ's outer Mixer. **Implication:**
VAZClone is a VST → the outer Mixer is **out of scope / handled by the host DAW**; do NOT rebuild it
inside the plugin. (A standalone multi-timbral rack, if ever wanted, is a host wrapper around N
VAZClone instances — not an engine change.)

---

## 3. INTEGRATION — how the three systems share data with the bit-exact engine

### 3A. (internal) Mixer ↔ Synth — ALREADY bit-exact, for reference
Per-voice render `FUN_004dbddc`: each osc/noise/ring/ext feeds its selected mixer channel; the
summed channels drive the filter (level sets Type K/R drive), Post channels bypass to the amp. This
is **pure routing + gain-staging**, already reproduced in `SynthVoice.h` (mix src/level/post,
drive-into-K/R). No modulation lives in the mixer stage that isn't already modelled → building the
outer rack would not touch this.

### 3B. Sequencer ↔ Synth — slots into EXISTING paths, no restructure ✅
Two clean hooks the engine ALREADY exposes — the sequencer needs no new engine path:
- **Notes:** the sequencer drives the SAME note-on path as the keyboard/MIDI — `FUN_004db3a8`
  (voice note-on) → `FUN_004db288` (voice setup), with `FUN_004a073c` the event dispatcher from the
  LFO/tempo-clock work. A clone sequencer just emits note-on/off into `juce::Synthesiser` (same
  voice trigger) on the tempo grid — no engine change.
- **Seq A / Seq B / Accent as mod sources:** these are ordinary **mod-matrix source indices** (same
  `ModBus` table as LFOs/envs), currently STUBS returning 0 (`VAZ_Clone_Audit.md:23`). The
  sequencer emits an **Accent Level** + the two Seq lanes (`vaz_big.c:3940/3975` — "Sequencer" /
  "Accent Level") the mod matrix already indexes → wiring them = fill 3 existing `ModBus::value`
  cases with per-step outputs, NOT a new modulation path.

**Net:** the Sequencer integrates through the two hooks the engine already has (voice note-on +
mod-matrix source slots); the outer Mixer is a host concern the DAW covers. **Neither forces a
restructure of the bit-exact core** — the point of mapping this before building.

---

## 4. WHEN IMPLEMENTATION STARTS (not now — mapping only)
1. **Sequencer (large, from scratch):** RPM/VMT-dump `TSequencer` (reuse `dump_engine_vmt.py`) →
   transcribe the step-tick + Timebase/Swing/Free/Perf2 timing, per-step Accent/Slide/Rest, the
   34-slot song chain; emit notes via the voice note-on path (§3B) + fill the Seq A/B/Accent
   `ModBus` cases. GUI from the §1B control list.
2. **Outer Mixer:** SKIP inside the VST (host DAW covers it — §2C). Only for a future standalone
   multi-instance rack, and then it's a host wrapper around N engines (confirm the instance model
   by an RPM walk, §2B), not an engine change.
3. **DFM ranges:** `tools/dump-dfm.ps1` anchored on the §1B control names (sequencer panel).

**Sources:** `tools/VAZ_Clone_Audit.md`, `tools/VAZ_Synth_Gap_Analysis.md`, `tools/vaz_big.c`
(§3762-3917, §3940-3975), `tools/vaz_voice.c` / `vaz_decomp.c` (note-on + `FUN_004a073c`), Core.dll
strings (`MixerCore`/`MixerUnit`/`CreateVAZ2010Standalone`), `tools/dump-dfm.ps1`.
