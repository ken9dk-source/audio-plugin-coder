# VAZ 2010 — SECOND-PASS GAP MAP (2026-07-15)

**Find-gaps exercise. No code changes.** A mechanical, name-complete sweep of `Vaz2010Core.dll`,
diffed against everything the project has written down. Supersedes nothing — it *adds* the coverage
denominator the earlier maps never had.

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
