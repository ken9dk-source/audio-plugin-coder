# VAZ 2010 — SECOND-PASS GAP MAP (2026-07-15)

> ## ⚠️ REVISION 2 (2026-07-15) — CLASS-AWARE HARVEST supersedes the name-only pass
> The first sweep (`harvest_vaz_names.py`) collected names **globally with no owning class**. Different
> forms have their own field tables with **overlapping offsets**, so cross-class offset comparison was
> unsafe. `tools/harvest_vaz_classes.py` fixes this by walking Delphi VMTs (self-identifying:
> `u32[V-76]==V`; `-56` FieldTable, `-52` MethodTable, `-44` ClassName, `-40` InstanceSize, `-36`
> Parent; virtual slots at `V+0,4,…`) → **`tools/vaz_classes.json`, 566 classes** with per-class
> fields/methods/VMT-slots.
>
> **Outcome of the re-check:**
> - ✅ **Q3 clustering holds where it matters:** *every* P2 synth control is on ONE class,
>   **`TMidSynthForm`** (instSize `0xa04`, 400 fields, 92 methods, 75 vmt slots) — so those offsets ARE
>   comparable. The concern was real in principle but does not invalidate the synth findings.
> - ❌ **But two P2/P3 items were misfiled:** `cbOversample` **and the whole `Imp*` family** are on
>   **`TBasePreferencesDlg`** (instSize `0x428`) — *not* the synth form.
> - **Class map for orientation:** `TMidSynthForm` (synth, 400 fields) · `TMixerForm` (213 — the OUTER
>   rack mixer, out of scope) · `TSeqForm` (209, P1/parked) · `TSeqSongForm`/`TSeqRandomForm` ·
>   `TFXDelayForm`/`TFXPhaserForm`/`TFXFlangerForm` · `TBaseMainForm`/`TInplaceMainForm`/`TVSTBaseForm`.
>
> ### ✅ `Imp*` FAMILY — SOLVED (was "unexplained"): it is the **Preferences → Import tab**
> `TBasePreferencesDlg` is a tabbed prefs dialog (`tsMidi`/`tsSynth`/`tsImport`/`tsPlugins`). The whole
> family sits on **`tsImport`** = the defaults VAZ applies when **importing** a patch that predates a
> param: `sbImpUnisonDetune@0x384`, `sbImpPolyDetune@0x414`, `cbImpAutoGlide@0x388`,
> `cbImpMultiTrigger@0x37c`, `edImpBendRange@0x390`, `rbImpPriorityHigh/Low/**Duo**/Last@0x370-0x380`.
> **"Imp" = Import. NOT per-patch DSP → not a DSP gap.** Two spin-offs worth keeping:
> - **`rbImpPriorityDuo`** — VAZ has **4** note-priority modes (High/Low/**Duo**/Last); the clone's
>   `note_priority` stub lists 3 (Hi/Lo/Last). **"Duo" is not previously known.**
> - **Reframe:** `cbOversample@0x408`, `cbMicroTuning@0x34c` + `edTuningFile/btTuningFile/odTuningFile`,
>   and `sbImpPolyDetune` are **global preferences**, not patch params. `vaz-full-gap-inventory.md`
>   lists Oversample / microtuning / Poly Detune as missing **DSP** — they are DSP-*affecting* but are
>   set app-globally, which a VST plugin has no equivalent of. Scope decision needed, not DSP RE.

**Find-gaps exercise. No code changes.** A mechanical, name-complete sweep of `Vaz2010Core.dll`,
diffed against everything the project has written down. Supersedes nothing — it *adds* the coverage
denominator the earlier maps never had.

> ## ✅ P2 VERDICTS (2026-07-15) — decided by joining the GUI harvest to the `.v2p` audit whitelist
> **The key move:** `VazV2PAudit` already prints a **WHITELIST = bytes VAZ persists that the clone does
> NOT map** (27 entries, several `[FLAGGED]`). It knew *"there are unmodeled fields here"* but not
> *what they are*. The class-aware GUI harvest supplies the identity. **Joined, they decide P2.**
>
> **Structural discovery (not previously recorded):** every VAZ mod slot is a **triplet**
> `[pm*Sign][ms*Source][sb*Depth]`. The clone models source+depth and folds Sign into its signed
> `amt`/`inv` pair — so **Sign is NOT a gap**. Slot inventory on `TMidSynthForm`:
> `Filter: FM1/FM2/FM3 + ModMod` · `Amp: AM1/AM2 + PM` · `Osc1/Osc2: FM1/FM2 + ModMod` ·
> `ModAmp1/ModAmp2: Source + AMSource` · `LFO2: RM` · `Env2: Mod`.
>
> | P2 item | evidence | verdict |
> |---|---|---|
> | **`msAmpPMSource`** (`pmAmpPMSign@0x8d0` / `msAmpPMSource@0x8d4` / `sbAmpPMDepth@0x8d8`) | ~~a complete extra mod triplet beyond AM1/AM2; matched to whitelisted `e04f4`+`e0504`~~ | ✅ **RETRACTED 2026-07-15 — ALREADY COVERED. My "REAL GAP" call was WRONG (see Item 2 below).** |
> | **`msLFO2RMSource`** (`pmLFO2RMSign@0x7cc`) | == whitelisted **`LFO2 trig-mod src` + `LFO2 trig-mod depth`** (`v>=200`), explicitly *"clone does not model LFO2 trig modulation"* | 🔴 **REAL GAP — confirmed.** The GUI name finally identifies it. |
> | **`btPortaAuto`@0x734 / `btPortaExp`@0x738** (+`sbPortaTime@0x73c`) | ~~whitelisted `e05f0`+`e0600`; clone has porta time only~~ | ✅ **RETRACTED 2026-07-15 — FULLY IMPLEMENTED (param+GUI+DSP). My "REAL GAP" call was WRONG (see Item 3).** |
> | **`msLFOPeriod`@0x788 / `msLFO2Period`@0x78c / `msLFOMode`@0x760 / `msLFO2Mode`@0x794** (+`btLFOSync@0x850`) | the LFO1 block whitelists **three** unmodeled v2.0 fields: `+0x94` *("purpose unconfirmed")*, `+0xded84` flag, `+0xe0`. Handlers exist and are real: `msLFOPeriodChange@0x510e68`, `msLFO2PeriodChange@0x511168`, `btLFOSyncChange@0x511100`, `PeriodPopupItemClick@0x511098` | 🟠 **LIKELY REAL GAP** — the GUI gives the 3 orphan fields an identity (Period / Mode / Sync). **Which-is-which not yet pinned** → do not implement on this alone. NB `+0x94` is the same field the Osc3 measurement proved does **not** drive pitch. |
> | **`msFilterModModSource`**@0x688 (`pmFilterModModSign@0x684`, `sbFilterModModDepth@0x68c`) | the filter's **4th** slot; `.v2p` has exactly 4 (`fcut1/2/3` + `fresS/fresD`) and **all are mapped** → this is the clone's `res_mod_src`/`res_mod_amt` | ✅ **VERIFIED CORRECT 2026-07-15 — naming risk was a FALSE ALARM (see below).** |
>
> ### ✅ Item 1 RESOLVED — `msFilterModModSource`: NO BUG, clone is correct (2026-07-15)
> The suspicion (that slot-3 modulates the filter *Modifier*, not resonance) was **investigated and
> refuted**. Chain of evidence:
> 1. **VAZ property names** (`vaz_osc.c`, the named-property loader): the filter loads `"Cutoff"`,
>    `"Resonance"`, `"Bandwidth"`, `"Slew Limit"`, `"FM Source/Depth 1..3"` and **`"RM Source"/"RM Depth"`**
>    — so slot-3's persisted name is "RM".
> 2. **Setters pin the fields:** `"Cutoff"`→`+0x264`, `"Resonance"`→`+0x26c`, **`"Bandwidth"`→`+0x270`**,
>    `FM1 src/depth`→`+0x278/+0x27c`, **`RM src/depth`→`+0x290/+0x294`**.
> 3. **The render has MULTIPLE branches**, all consuming the same RM slot (`+0x290/+0x294`) but adding it
>    to a **different base per filter mode**: `vaz_big.c`:1183/1290/1397 use `+0x270` (Bandwidth/
>    Separation) as the base; :1094/1185/1270/1582 add it to other bases. *Generalising from one branch
>    is what produced the false alarm.*
> 4. **The decider — VAZ's own mode names** carry the routing as a suffix. Core.dll captions:
>    `"Type R 2P+HP RM"`, `"Type R 2P+HP HM"`, `"Type R 4P+HP RM"`, `"Type R 4P+HP HM"`, and radio
>    buttons `rbTypeC2RM/C2HM/C4RM/C4HM/**C4SM**/R2RM/R2HM/R4RM/R4HM`. **RM = Resonance Mod, HM =
>    Highpass Mod, SM = Separation Mod** — the *filter mode* decides slot-3's target.
> 5. **The clone already models exactly this** — `Synth.h`:430 `modRoute  // 0=Resonance 1=Highpass
>    2=Separation`, assigned per mode (`case 2 → 0 "C 2P+HP RM"`, `case 14 → 1 "HM"`, `case 8 → 2 "SM"`),
>    applied at `SynthVoice.h`:303-307.
> **Verdict: `msFilterModModSource` = the generic slot-3 mod source = clone `res_mod_src` + `modRoute`.
> Correct as shipped. NO CODE CHANGE.** (Unlike the mix3 off-by-one, which was real — flagging both and
> testing each on evidence is the point.)
> | **`Imp*` family** | on `TBasePreferencesDlg` → Import tab | ✅ **NOT DSP** (see Revision 2). |
>
> **Report-before-implement:** nothing above is implemented. Recommended order if we act:
> 1. `msAmpPMSource` (clean, self-contained, whitelist-confirmed), 2. `btPortaAuto/Exp` (2 flags),
> 3. `msLFO2RMSource`, 4. **verify the `msFilterModModSource` naming risk** (cheap, and it's a
> *correctness* issue in shipped code rather than a missing feature), 5. LFO Period/Mode (needs pinning).

> ### ✅ Item 2 RESOLVED — `msAmpPMSource`: **PM = Pan Modulation**, already covered. Claim RETRACTED.
> **`PM` does not mean Phase Mod.** Evidence:
> 1. **VAZ's full property vocabulary** lives in TWO complementary clusters — old `*Source` @0xd90d8-0xd9300,
>    new `*Sign` @0xd3600-0xd3980. The new one is far richer and is the real map:
>    - `Filter: Cutoff, Resonance, **Modifier 1**, **Modifier 2**, FM Sign 3, FM Depth 3, **MM Sign**, **MM Depth**`
>    - `Amplifier: AM Sign 1, AM Depth 1, AM Sign 2, AM Depth 2, **PM Sign**, **PM Depth**, Overdrive`
>    - `Mod Amplifier 1: **SQ Mode**, AM Sign, AM Depth` · `**Lag Processor**: Time`
>    - `Performance: Play Mode, **Note Priority**, **Last Note Priority**, **Voice Detune**,
>       **Portamento Auto**, **Portamento Exp**, **Portamento Time**, Unison Voices, Pitchbend Range`
> 2. **The Amp's slot order is `AM1, AM2, PM, Overdrive`** — and the clone reads `am3s/am3d` (`v>=200`)
>    at *exactly* that position (immediately before `overdrive`).
> 3. **DFM caption @0x180903 = `"Pan Modulation"`** (a `TVTArial8Label` section header on the synth form).
> 4. **There is no static Pan control** on `TMidSynthForm` (`/pan/i` → none) and **no `"Pan Source/Sign/
>    Depth"` property** — i.e. VAZ's pan is **modulation-only**, which is exactly a "Pan Modulation"
>    header + one mod triplet.
> 5. The clone maps `am3s/am3d → pan_mod_src/pan_mod_amt` and applies real panning (`SynthVoice.h`:341).
> **Verdict: `msAmpPMSource` == the clone's `pan_mod`. Correct as shipped. NO CODE CHANGE.**
>
> **⚠ Correction to Finding 2:** I equated `msAmpPMSource` with the whitelisted `e04f4`+`e0504` slot.
> That was wrong — `e04f4/e0504` sit **after** `overdrive`, i.e. **outside** the Amplifier's vocabulary
> (which ends at Overdrive). **`e04f4/e0504` remain a genuine, still-unidentified gap** (candidates from
> the vocabulary order: `Mod Amplifier 1` / `Lag Processor`) — but they are NOT the Amp PM slot.
>
> **✅ Item 3 pre-confirmed by the same vocabulary:** `"Portamento Auto"` and `"Portamento Exp"` are real
> named properties, and the clone whitelists `e05f0`/`e0600` at exactly the matching stream position
> (after `voiceMode`=`Play Mode`, before `bendRange`=`Pitchbend Range`). **btPortaAuto/btPortaExp stay a
> REAL GAP** — that part of Finding 2 holds.
>
> **Scoreboard so far — 2 of 2 "gaps" I flagged were false alarms.** Item 1 (`msFilterModMod`) and Item 2
> (`msAmpPM`) were both already correctly modelled. The clone's earlier RE is better than the name-level
> sweep implied; **GUI control names are unreliable evidence** (`ModMod`≠resonance-vs-modifier question,
> `PM`≠phase-mod). Only the **named-property vocabulary + stream position + DSP branches** decide.

> ### ✅ Item 3 RESOLVED — `btPortaAuto`/`btPortaExp`: **already fully implemented.** Claim RETRACTED.
> **VAZ side (DSP confirmed from FOUR render branches, per the item-1 lesson):** `vaz_big.c`
> :547/:560/:791/:804 = 2 pitch directions × 2 oscillators. The glide step is
> `if (*(param_1+0x304)==0) step = [voice+0x1b8];  else step = (distance>>0x10) * [voice+0x1b8] >> 9;`
> → **`+0x304` = Portamento Exp**: `0` = constant step (**linear**), non-zero = step ∝ remaining distance
> (**exponential**). Setters: `FUN_004e09d8→+0x30c` = Portamento **Time** (from `"Oscillator1"/"Portamento"`),
> `FUN_004e09b8→+0x308` = Porta **Auto** (fed from global `DAT_00543dc9` = the **Import default**, which
> independently corroborates Finding 1's Imp*=Import story), `FUN_004e09c8→+0x304` = Porta **Exp** (default 0).
> Authoritative names come from the **property descriptor table** decoded at `0x52AF9C+`
> (`[section*][prop*][default][A][B]`, 20-byte stride) — `Performance: Play Mode, Note Priority,
> Last Note Priority, Voice Detune, Portamento Auto, Portamento Exp, Portamento Time, Unison Voices,
> Pitchbend Range`. Tool: `tools/dump_prop_table.py`.
>
> **Clone side — it's all there:** `porta_exp` + `porta_auto` params (`PluginProcessor.cpp`:190-191,
> auto default **true**), wired at :274-275, GUI buttons (`index.html`:641 `Auto`/`Exp`), relays
> (`PluginEditor.h`:94-95), and DSP (`SynthVoice.h`:64-65 + :155 legato gate + :215-221 glide math).
> The clone's own comment — *"exponential glide (off = linear constant-rate)"* — matches VAZ exactly.
> **Verdict: NOT a gap. NO CODE CHANGE.**
>
> **Two OPEN questions this raised (flagged, NOT claimed as bugs):**
> 1. 🟠 **Exp-glide domain.** VAZ's exp step is ∝ distance in the domain of `+0xb8/+0xbc`. VAZ's note
>    table is `DAT_006dd0c0[note] = note*128` (**pitch-linear**), so *if* `+0xb8` is that pitch value,
>    VAZ's exp glide is an RC in the **cents** domain — whereas the clone (`SynthVoice.h`:215) does
>    `glidedHz += (noteHz-glidedHz)*(1-exp(...))` = an RC in the **Hz** domain. Those are different
>    curves. **Unconfirmed:** `+0xb8` is written in the note-on path (`vaz_big.c`:258/345/2949), not the
>    render — pinning its domain is a separate trace. The clone's **linear** case already matches
>    (constant cents/s). *Do not act on this without pinning `+0xb8`.*
> 2. 🟡 **Preset wiring.** `e05f0`/`e0600` stay whitelisted, so **real VAZ patches do not carry Porta
>    Auto/Exp into the clone** (they fall back to defaults). But their identity is **unconfirmed**: the
>    Performance section leaves **four** unmapped properties (Note Priority, Last Note Priority, Porta
>    Auto, Porta Exp) for **two** slots, and the stream order does *not* follow the vocabulary order.
>    **Cheapest decisive test:** a VAZ-saved preset pair (Porta Exp on/off) byte-diffed — exactly the
>    method that decoded `mix3src` from the user's Osc3/RingMod pair.
>
> ### 📊 SCOREBOARD: **3 of 3 flagged "gaps" were FALSE ALARMS** — the GUI-name sweep found **zero** real DSP gaps
> Items 1 (`msFilterModMod`), 2 (`msAmpPM`), 3 (`btPortaAuto/Exp`) were **all already correctly
> modelled**. The clone was built from the **`.v2p` param stream + the DSP**, so a **GUI-name** diff
> systematically over-reports: the same feature simply carries a different label (`ModMod`≠modifier-vs-
> resonance, `PM`=**Pan** Mod, Porta flags already shipped). **Only the named-property vocabulary +
> descriptor table + stream position + *all* DSP branches decide.** The sweep's real value turned out to
> be **identification** (naming the `.v2p` audit's orphan fields), not gap-discovery — plus one genuine
> bug found by a different route entirely (the mix3 off-by-one, caught from user-made presets).

## Method (reproducible)
Borland/Delphi publishes RTTI tables we can enumerate exactly:
- `TMethodEntry = [u16 size][u32 addr][u8 len][chars]`, `size == 7+len` → **name → handler address**
- `TFieldEntry = [u32 offset][u16 classIdx][u8 len][chars]` → **name → object offset**

- `tools/harvest_vaz_names.py` → `tools/vaz_name_universe.json` (the complete universe)
- `tools/vaz_gap_diff.py` → diffs it against the corpus: 25 `.ideas`/`tools` docs + 30 decompiled `.c`
  + 40 clone sources. "Never mentioned" = the exact identifier appears **nowhere** in any of them.

## Headline: the coverage denominator we never had
| | published in Core.dll | ever mentioned in the project | **never touched** |
|---|---|---|---|
| **Handlers** (name → code VA) | **413** | 27 | **384** |
| **Controls/fields** (name → obj offset) | **1515** | **47** | **1468** (455 DSP-relevant) |

The clone was built from the **.v2p param stream + the sliders**, not from the control surface. That
shows: **no `sb*` (slider) control is in the never-mentioned set** — the continuous params are well
covered. **The gap is almost entirely buttons / menus / mode-selectors** (sources, sync, link, modes).

---

## Q1 — named controls whose handler was never followed
Effectively **all of them**. Only ~27 of 413 handlers are known. **But priority is low for most**: the
384 unvisited handlers are dominated by VCL/app plumbing (`FormCreate`, `FormResize`,
`File*ItemClick`, `ClientPanel*`, `Bevel*`). The DSP-bearing controls follow the already-decoded
`msTimebaseChange` shape (thin forwarder → property setter), so they're cheap to follow *on demand*.

**Exceptions worth following (real signal):**
| handler | VA | why |
|---|---|---|
| **`FileCaptureSequenceItemClick`** | **0x4F4FD8** | **Capture *Sequence* → renders the sequencer to WAV.** See P1 below. |
| `FileStopCaptureItemClick` | 0x4F5008 | pairs with the above |
| `EnvModPopupItemClick` | 0x511474 | envelope-mod routing popup |
| `ChannelPopupItemClick` / `ChannelOmniItemClick` | 0x4C4280 / 0x4ED7D8 | MIDI channel handling |

## Q2 — addresses referenced but never visited
The 384 above. **Not yet done:** a VMT-slot sweep (walk each known VMT — e.g. `TBaseMidSynth`
@coreBase+0xd4614 — and mark which slots are decompiled). That is the one sub-question this pass did
**not** answer and is a clean follow-up.

## Q3 — offset families (pattern recognition)
Form-object offsets cluster hard by subsystem. This is a **map to navigate by**, and it predicts where
unexamined controls live:
| range | subsystem | examples |
|---|---|---|
| `0x3xx` | app/dialog/global | `btSeqPlayStop@0x3d4`, `btSync@0x3d8`, `cbOversample@0x408`, `edStartStep@0x41c` |
| `0x5xx` | osc sample + mixer + patterns | `btOsc1SampleClear@0x500`, `btOsc2Sync@0x574`, `btPatterns@0x580`, `btMixPost1..3@0x5d0-0x5e4`, `msMixSource3@0x5e0` |
| `0x6xx` | filter + amp + mod-amp + porta | `msFilterMode@0x63c`, `msFilterFMSource1/2/3@0x660/0x66c/0x678`, `msFilterModModSource@0x688`, `msModAmpSource@0x69c`, `msAmpAMSource1/2@0x6c0/0x6cc`, `msLagSource@0x6e8`, `btPortaAuto/Exp@0x734/0x738` |
| `0x7xx` | LFO + env2-mod + mod-amp2 | `msLFOMode@0x760`, `msLFOPeriod@0x788`, `msLFO2Period@0x78c`, `msLFO2Mode@0x794`, `msLFO2RMSource@0x7d0`, `msEnv2ModSource/Dest@0x7dc/0x7e4`, `msModAmp2Source/AM@0x7bc/0x7c4` |
| `0x8xx` | LFO sync/retrig, mix src, osc link | `btLFOSync@0x850`, `btLFO2Retrigger@0x85c`, `msAmpPMSource@0x8d4`, `btOsc1Link@0x8e4`, `btModAmpSQ@0x8e8`, `msMixSource1/2@0x8f0/0x8f4` |
| `0x9xx` | envelope mode buttons | `btEnv1Multi/Reset/Curve/Cycle@0x900-0x90c`, `btEnv2*@0x910-0x91c` |

**Corroborates the just-fixed mixer bug:** `msMixSource1/2@0x8f0/0x8f4` + `msMixSource3@0x5e0` are
three instances of one control family — exactly the "shared popup, item 0 relabelled" structure that
the mix3 off-by-one violated.

---

## Q4 — what is completely missing from the mapping

### 🔴 P1 — UNBLOCKS A PARKED TASK (highest value)
| finding | where | why it matters |
|---|---|---|
| **`btSeqPlayStop`** — **not previously known** | `@obj+0x3d4` | The **sequencer transport button**. The sequencer timing task is parked *precisely because* "the headless standalone doesn't run the transport" (MIDI Start/Clock ignored). This is the control that starts it. |
| **`FileCaptureSequenceItem(2)` / `FileCaptureSequenceItemClick`** — **not previously known** | `@obj+0x394 / 0x504`, handler **0x4F4FD8** | VAZ can **render a running sequence to WAV**. Combined with `btSeqPlayStop`, this makes the parked sequencer timing measurable by **audio-render** — the exact technique that just closed Osc3 #9 (division→samples, swing, free-run all become measurable from step onsets). |
| `btPatterns@0x580`, `btPlayMode@0x584`, `btStepMode@0x5a8`, `edStartStep@0x41c`, `edTransStep1..12@0x3b4-0x3fc` | — | the sequencer surface, with offsets, for driving it |

### 🟠 P2 — DSP params never mentioned anywhere (genuinely new)
| control | offset | note |
|---|---|---|
| **`msAmpPMSource`** | `0x8d4` | **Amp *phase*-mod source.** No trace in any map/clone. The clone models amp AM only. |
| **`msLFOPeriod` / `msLFO2Period`** | `0x788 / 0x78c` | **LFO Period** selector — distinct from the (already-verified) LFO *rate* curve. Suggests a tempo/period mode the clone lacks. |
| **`msLFOMode` / `msLFO2Mode`** | `0x760 / 0x794` | LFO mode selectors (clone has a partial `lfo2mode` in `.v2p` only) |
| **`msLFO2RMSource`** | `0x7d0` | LFO2 **rate-mod** source (LFO rate modulated by another source) |
| **`msFilterModModSource`** | `0x688` | a *third* filter-mod axis beyond the 3 FM sources |
| **`btPortaAuto` / `btPortaExp`** | `0x734 / 0x738` | portamento **Auto** + **Exponential** modes — clone has a single linear glide |
| `msModPeriod@0x3d4`, `btModSync@0x3d8`, `cbRotaryMode@0x3e4`, `cbControlAMode/BMode@0x3f0/0x3f8` | — | global mod/MIDI-control modes |
| `cbImpAutoGlide@0x388`, `edImpBendRange@0x390`, `sbImpUnisonDetune` | — | an **"Imp*" control family** (per-instance/import?) with **no counterpart anywhere** in the project |

### 🟡 P3 — concept already known, but now with a control name + offset
These were listed as gaps in `vaz-full-gap-inventory.md` / `VAZ_Synth_Gap_Analysis.md` **without**
addresses; the sweep now pins them:
`btOsc1Link@0x8e4` (Link) · `btOsc2Sync@0x574` (Osc2 Sync) · `cbOversample@0x408` (Oversample) ·
`msLagSource@0x6e8` + `LagProcessor1@0x4a4` (Lag) · `btModAmpSQ@0x8e8` (ModAmp SQ) ·
`btLoadSample@0x348` + `btOsc1/2SampleClear@0x500/0x578` + `cbSampleType/Rate/Used` (Sample Loader) ·
`btMasterArpeggiate@0x758` + `btMasterArpMode@0x75c` + `LocalArpModeItem@0x4cc` (Arp) ·
`btEnv1/2 Multi/Reset/Curve/Cycle@0x900-0x91c` (env modes — clone HAS these params; the DFM names were
simply never cross-referenced) · `btMixPost1..3@0x5d0-0x5e4` (mix post — clone has them).

### ⚪ P4 — low value
384 unvisited handlers that are VCL/app plumbing; 190 `lb*` labels (cosmetic); VCL properties that
leaked into the field scrape (`Bevel*`, `AlignMode`, `AutoScroll` — scrape noise, not controls).

---

## Recommended priority
1. **P1 — sequencer via `btSeqPlayStop` + Capture-Sequence.** Turns a documented dead-end into the
   same audio-render measurement that just closed #9. Highest value per effort by a distance.
2. **P2 — decide scope.** `msAmpPMSource`, `msLFOPeriod`, `btPortaAuto/Exp` are real VAZ features the
   clone silently lacks. Each needs a handler-follow (cheap: the `msTimebaseChange` forwarder shape)
   to learn its `.v2p` field + DSP effect. Note **all are absent from the `.v2p` param map too** —
   worth checking whether they're preset-persisted at all before investing.
3. **Q2 leftover — VMT-slot sweep.** The one sub-question unanswered; would enumerate DSP-class
   methods rather than GUI handlers, and is the natural complement to this pass.

**Caveat (honesty):** "never mentioned" is a strict *identifier* match. A few P3 items are known
*concepts* under other names (Link, Oversample, Arp) — they are flagged as such, not as new
discoveries. The P1/P2 items were checked to be conceptually absent too.
