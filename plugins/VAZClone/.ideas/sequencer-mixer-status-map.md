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

---

# 5. DESIGN + SCOPING (2026-07-14) — bottom tabs + Sequencer; Mixer tab assessed

## 5.0 ⛔ MIXER TAB — VERDICT: DROP IT (redundant, no new value)
Honest conclusion up front (the deliverable asked for this): **a "Mixer" tab adds ZERO parameters
the Synth panel doesn't already show.**
- The **osc-mixer IS already the Synth panel's "Mixer" column** (`index.html`:501): `mix1/2/3_src`
  (Osc1/Osc2/Osc3/RingMod/Noise/Ext/ModAmp sources), `mix1/2/3_post` (Post = post-filter),
  `o1_level`/`o2_level`/`noise_level` (channel levels).
- Per-instance **master volume = `amp_level`** (Amplifier col); **pan = `pan_mod`** (Amplifier).
- Grep of `ParameterIDs.hpp` + the whole `.v2p` field list: **no separate output / pan / dry-wet /
  send / sub-out param exists** at the instance level. VAZ's only extra "mixer" is the OUTER
  standalone rack (sends, multi-instance) — out of scope for a single-instance VST (§2C).

➡ **Recommendation: build TWO tabs — `Synth | Sequencer` — not three.** A Mixer tab would be a
duplicate re-presentation of existing controls. (If a "zoomed mixer" view is ever wanted for UX, it
re-skins existing params — cosmetic, low value, defer.) **The Sequencer is the only real new panel.**

## 5.1 TAB SHELL — fits the WebView GUI without disturbing the just-cleaned-up JS
Why it's low-risk: **all controls bind to the APVTS via relays** (`SliderState`/`ComboBoxState`
keyed by param name, `__juce__.initialisationData`), so the **state lives in the C++ backend, not
the DOM**. Therefore:
- **Tab switch = pure CSS show/hide of sibling panes.** Nothing unbinds; no synth state is lost when
  you leave/return to the Synth tab. The relabel/label JS (`o1labels`, the mix3→Osc3 relabel, the
  filter-label maps) operate on element IDs that persist while hidden and only fire on clicks in the
  *visible* pane → **the tab code never touches that JS.**
- **Structure:** wrap the current `.synth-body` (5-col grid) in `<div class="tab-pane" id="tab-synth">`;
  add `<div class="tab-pane" id="tab-seq" hidden>`. Put a `.tabbar` at the BOTTOM (per "faneblade
  nederst"), just above the existing `.transport`, with `Synth | Sequencer` buttons.
- **New JS = one isolated `initTabs()`** (~12 lines: toggle `.tab-pane[hidden]` + button `.active`).
  Additive, no edits to existing binding/relabel functions.
- **Transport (Menu / Pitch Bend) stays global** below the tabs — visible on both.
- **Height:** editor is fixed 726×610 (content 604×508). The Sequencer pane must be designed to the
  SAME footprint; if it genuinely needs more, bump the editor height once (like the menu fix) — both
  panes share it. Guard with `capture_vazclone.py` (the menu-clip lesson, [[feedback_vazclone_menu_clip]]).

## 5.2 SEQUENCER — UI layout (within the 604×508 footprint)
- **Pattern grid** (the core): N steps (VAZ pattern length is variable via `cbStartStep`/`cbEndStep`/
  `edFinalStep`; confirm max = 16 or 32 by RE). Each step column = a **note/pitch cell** + **Accent /
  Slide / Rest** toggles (`cbAccent`/`cbSlide`/`cbRest`) + optional **Seq A / Seq B** value cells (the
  two mod lanes). Start/End markers frame the active range.
- **Song chain:** a strip of the 34 slots (`edSongStep1..34`) selecting which pattern plays per song
  step (+ loop). Paginate/scroll to fit width.
- **Transport row:** Run/Stop, Timebase / Swing / Free / Perf2 (rate + swing), `cbLoadSeq`/`cbSaveSeq`,
  note range (`cbNoteLow`/`cbNoteHigh`), `cbThruChannel`, Arp FreeRun tie-in.

## 5.3 ENGINE HOOKS — no restructure of the bit-exact core (per §3B)
- **Notes:** a `SequencerEngine` in the processor, clocked by host tempo + the sample counter (the
  SAME mechanism as the existing LFO tempo-sync). On each step boundary emit `juce::Synthesiser::
  noteOn/noteOff` (the clone's equivalent of `FUN_004db3a8`): **Accent** → velocity/level scale,
  **Slide** → legato/portamento (hold, glide, no note-off), **Rest** → skip. Runs in `processBlock`,
  drives the SAME voices — zero DSP-core change.
- **Seq A / Seq B / Accent mod sources:** the `SequencerEngine` exposes the current step's Accent
  Level + Seq A + Seq B; wire them into the **3 currently-stubbed `ModBus::value` cases** (return 0
  today, `VAZ_Clone_Audit.md:23`). No new modulation path.

## 5.4 PARAM / STATE MODEL — decision
16 steps × (~5 fields) + 34 song slots + timing ≈ 110+ values — **too many for automatable APVTS
params**, and VAZ's sequencer steps aren't host-automatable anyway. ➡ Store the sequencer in a
**separate `ValueTree` branch** (a few automatable *global* params only: run/stop, rate, swing),
serialised in `getStateInformation` and appended to the `.v2p` save (extend the now-complete writer,
[[project_vazclone_v2p_loader]]). Keeps the APVTS lean and matches VAZ (steps are patch data, not
automation lanes).

## 5.5 PHASED PLAN (docs → oracle/tests → implementation — same discipline as the rest of the project)
- **Phase A — DESIGN / RE:** DFM ranges via `dump-dfm.ps1` (the §1B names); RPM/VMT-dump `TSequencer`
  off the running synth (reuse `dump_engine_vmt.py`) for the exact step-tick, Timebase/Swing/Free/
  Perf2 timing, and the Seq A/B value semantics. Finalise the pattern step count + song-chain rules.
- **Phase B — ORACLE / REGRESSION TESTS (before impl):** (1) headless `SequencerEngine` test — given
  a pattern + tempo, assert the emitted note/velocity/timing stream matches a transcribed reference
  (like the FX render oracles); (2) sequencer-state round-trip test (ValueTree save/reload) as a new
  target alongside `VazV2PRoundtrip`; (3) mod-source test — Seq A/B/Accent feed `ModBus` correctly.
- **Phase C — IMPLEMENTATION:** `SequencerEngine` (clock → step → note emit → mod outputs) → wire the
  3 `ModBus` cases → GUI: tab shell (§5.1) + step grid + song chain (§5.2) → bind global params +
  ValueTree → `.v2p`/state persistence. **Mixer tab: not built (§5.0).**

**Deliverable status:** design/scoping only, no code. Headline: **Mixer tab dropped as redundant;
scope is the Sequencer + a 2-tab shell**, and both integrate through hooks the bit-exact engine
already exposes (§3), so no core restructure.

---

# 6. PHASE-A RE RESULTS (2026-07-14) — Sequencer data model + controls (from the decompile)

## 6.1 Step / pattern data model — `vaz_big.c`:3600-3800 (the pattern-load loop)
- **Pattern = 0x237 (567) B**; nested load loop **patterns (`iVar9`) × steps (`iVar8`)**, step stride
  **0x22 (34) B**, step data begins at pattern **+0x34**. `(0x237-0x34)/0x22 = 15.1` → **~16 steps/pattern**.
- **Per step** (offsets = pattern base + field + n·0x22):

  | field | offset | meaning |
  |---|---|---|
  | **Pitch** | +0x34 | note (byte) |
  | **Flags** | +0x35 | bitfield **Double(1) · Rest(2) · Slide(4) · Accent(8)** |
  | **SeqControl1 = Seq A** | +0x54 | per-step mod-lane A value (byte) |
  | **SeqControl2 = Seq B** | +0x55 | per-step mod-lane B value (byte) |
  | **Transpose** | (`edTransStep`) | per-step transpose (from DFM) |

  Settings keys confirm it: `"Pitch N M"`, `"Double/Rest/Slide/Accent N M"`, `"SeqControl1/2"`
  (v100) or `"Control 1/2 N M"` (older), grouped under section `"Sequencer"` / `"Sequencer Pattern N"`.
- **Multiple patterns** (`iVar9` outer loop; count TBD) + a **34-slot SONG chain** (`edSongStep1..34`)
  + **Loop Start/End Step** (`vaz_big.c`:3873 "Loop Start Step").

## 6.2 Controls (DFM — `tools/dump-dfm-seq.ps1`)
- **Timing:** `msTempo` (BPM), `msTimebase` (note-division/timebase), `sbSwing`(+`sbSwingT`),
  `sbGateTime` (note-length %), `btFreeRunning` (Free vs host-synced).
- **Steps:** `udFinalStep` (pattern length), `cbStartStep`/`cbEndStep` (loop range), `sbStepPage`
  (pages the step view in groups — `lbStep14`/`lbStep58`/`lbStep912` = "1-4"/"5-8"/"9-12"),
  `sbAccentLevel` (global accent amount applied when a step's Accent bit is set).
- **Per-step edit:** `edTransStep` (transpose) + the Double/Rest/Slide/Accent toggles + the pitch cell.
- **Song:** `edSongStep1..34` + `udSongStep1..` + `lbSongStep1..` (per-slot edit/updown/label).
- **Routing:** `cbNoteLow`/`cbNoteHigh` (note range), `cbThruChannel`, `cbLoadSeq`/`cbSaveSeq`.

## 6.3 STILL OPEN — needs the running-synth RPM/VMT dump (Phase-A tail)
The pattern LOAD is decompiled (above); the per-sample ADVANCE / clock is not, so still to confirm:
- **Step-tick timing math:** the `msTimebase` division table, the `sbSwing` curve, the
  `btFreeRunning` free-rate law, and **Perf2** — all live in TSequencer's clock/advance callback.
  Dump via `dump_engine_vmt.py` against the running standalone while a pattern plays.
- Exact **step count** (15 vs 16), **pattern count** (`iVar9` bound), and **Seq A/B range/semantics**
  (0..255? bipolar? what they map to in the mod matrix).
- **DFM numeric ranges** (Min/Max/Increment): the byte dump gives control ORDER/layout; exact ranges
  need a proper Borland-DFM parse or the running form.

## 6.4 Design impact — confirms §5, no surprises
A pattern = **16 steps × {pitch, flags(Double/Rest/Slide/Accent), transpose, Seq A, Seq B}**, N
patterns, a 34-slot song chain, + global timing — fits the §5.4 ValueTree plan directly, and the
per-step **Seq A/B are exactly the two ModBus lanes** (§3B). The only additions vs the §5 sketch:
a per-step **Transpose** and a **Double** flag + a global **Accent Level**; fold these into the
step-cell UI and the ModBus (Accent Level scales the Accent bit). Nothing changes the §5 architecture.

---

# 7. PHASE-B — TEST STRUCTURE BUILT (2026-07-14, no engine DSP yet)

`Source/SequencerEngine.h` (data model + interface + **PLACEHOLDER** timing) + `VazSeqTest`
(`Source/seq_test_main.cpp`, CMake target) — **3 oracles, all green:**
- **note-stream** — asserts note ORDER / Rest-skips / Accent-velocity now; Slide-legato + exact
  sample-positions tagged ` AWAITING RUNTIME DUMP `.
- **state round-trip** — `SeqState → ValueTree → SeqState` identity over patterns/steps/song/timing;
  fully functional (feeds the §5.4 ValueTree persistence).
- **ModBus** — Seq A/B → 0..1 normalise, Accent → level; value RANGE tagged ` AWAITING RUNTIME DUMP `.

All placeholder timing is isolated in `SeqTimingPLACEHOLDER` (3 functions: `stepSeconds`,
`swingDelaySeconds`, free-rate) — **only those change when the dump lands; the engine interface + the
oracles are final.** Phase C = accurate engine + voice-integration + the GUI tab (§5).

## ✅ RUNTIME-DUMP SESSION ATTEMPTED (2026-07-14) — outcome: route WORKS, but targets are STATIC → use disasm
Time-boxed per request. **The live MIDI route is RELIABLE + reproducible** — `tools/dump_engine_vmt.py`
(shared MidiOut, note 60) makes the note SOUND: **~14,086 heap words move**, per-voice DSP-state clusters
(repeated 0xEFC spans) visible. The §3.0 Run-3 route blocker is genuinely solved.

**BUT the live keystone is the WRONG TOOL for these three — this reframes the deferred work:**
- They are **static tables/formulas, not runtime-varying state.** The timebase = musical divisions
  (strings in Core.dll: **1/1, 1/2, 1/4, 1/4D(dotted), 2/3, 3/4, 5/6, triplets**); the swing curve,
  free-run law, Seq A/B range, LFO waveform tables, and Osc3 footage are constants — not moving words.
- The **sequencer isn't running under a note-on** (note-on → a synth VOICE, not the sequencer
  transport), so its timing never appears in the live moving-state at all.
➡ **Correct path = STATIC disassembly** (the technique that nailed the autopan sqrt-law, the LFO rate
curve `0.02·e^0.036·sel`, the coef LUTs); the sequencer is partly in the decompiled subset
(`vaz_big.c`:3600-3960). This is **incremental per-target RE, NOT a single blocked live-route session** —
the plan's "one MIDI-route effort" is superseded.

**Status of the three (reclassified: live-blocked → static-RE):**
1. **Sequencer step-tick timing** — read the FULL serializer `vaz_big.c`:3600-4060 (2026-07-14).
   **✅ Now CONFIRMED:** 16 patterns (`iVar9 != 0x10`) × 16 steps; per-pattern Gate Time(+0x259) + Accent
   Level(+0x25a) + Loop Start/Step + Step Mode; global Tempo · Play Mode(→`DAT_00529e54`) · **256 Song
   Steps** + Song Transpose · Loop Start/At Pattern · Arp Mode/Range · Clock To Trig. **✅ Seq A/B RANGE =
   0..255 byte** (SeqControl1/2 @ +0x54/+0x55) — one of the four items CLOSED; **Seq A→Tempo** ("Control 1
   To Tempo"), **Seq B→Gate** ("Control 2 To Gate"), both mod-matrix sources. `SequencerEngine.h` modSeqA/B
   + `VazSeqTest` updated to CONFIRMED.
   **❌ STILL open — timebase index→value, swing curve, free-run law:** there is **NO `Timebase`/`Swing`/
   `FreeRun` key anywhere in the serializer** → their timing math is in the sequencer **real-time CLOCK**, a
   SEPARATE function (app-shell or un-decompiled Core.dll), NOT `vaz_big.c`:3600-4060. `SeqTimingPLACEHOLDER`
   LEFT AS-IS (not guessed). **Next disasm target: locate the sequencer clock** (candidate near the settings
   code; the `msTimebase`/`sbSwing` handlers write a field the clock reads).
2. **Mod-LFO waveform shapes** (§3.0 #8) — static tables; disasm the mod-LFO gen in the voice render.
3. **Osc3 footage → pitch** (#9) — static formula; disasm the osc3 increment calc.

**Do NOT retry the live route for these** (it works but yields nothing for static tables). Each is a
bounded static-disasm task on its own; the live keystone stays available (documented, reproducible) for
any genuinely-runtime state need later.
