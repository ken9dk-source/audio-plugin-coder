# VAZClone — BUGS & INACCURACIES

Likely bugs, wrong parameter ranges and behavioural differences vs the real VAZ 2010.
Ordered by severity. (Several earlier-reported bugs are already FIXED — listed at the bottom for history.)

Legend: 🔴 high · 🟠 medium · 🟡 low · ✅ fixed

---

## OPEN

### ✅ B-01 — Filter is global, not per-voice — FIXED
**Was:** ONE filter on the summed voice mix; filter env/key-track didn't retrigger per note in
chords/legato; signal order was also wrong (osc→amp→filter).
**Fix:** moved `VAZMultiFilter` + a per-voice `VAZEnv` (Env2) into `VAZVoice`, in the **correct VAZ
order osc→filter→amp**. Each note now has its own filter envelope, key-track (per-voice note) and
velocity. Removed the global per-sample filter loop in `processBlock` (the global `filterEnv` is kept
only to feed the *bus* Env2 mod source for the Mod-Amps). Built + deployed.

### 🟠 B-02 — `.v2p` preset load is only partial
**Symptom:** Loading a factory preset gets the main sound right (filter, cutoff, res, ADSR×2,
levels, voice-mode, LFO rate, osc wave, octave) but **fine details are wrong**.
**Not yet mapped:** OSC1 level/coarse/fine, OSC2 octave/wave/fine, all mod-routing sources & depths,
`hp_cutoff`, `flt_aux`, waveshape-mod, FM. Envelope LE16→time uses a `/425` approximation (curve differs).
**Fix:** extend `loadV2P`/`buildV2P` byte map (`PluginProcessor.cpp`) — need isolated test `.v2p` for each
unmapped offset (the `Vaz Parameters` folder method).

### 🟠 B-03 — Filter level jumps between types
**Symptom:** Switching filter type changes loudness (Type R/K are louder).
**Status:** Actually **faithful** — manual: "level sent to the Filter affects the resonance character of
Type K and R; levels around half give the standard drive level." R/K are drive-sensitive, A/B/C/D are unity.
Documented, not a true bug, but can feel jarring. Consider a small makeup on A/B/C/D if desired.

### 🟡 B-04 — Noise scales with polyphony
Noise is generated **per voice**, so N held notes = N× noise level. VAZ has one global noise generator.
**Fix:** generate one noise stream in the processor; voices read it (or scale `noise/√N`).

### 🟡 B-05 — Velocity hard-wired to amplitude
`level = max(0.05, velocity)` always scales output. VAZ makes velocity a **mod route** (Amp-Mod slot),
so un-routed velocity should not change level. Soft MIDI notes are always quiet in the clone.

### 🟡 B-06 — Envelope times are approximations
Clone amp env = `0.001 + x²·3 s`, filter env = `0.0002 + x²·4 s`. VAZ stores 16-bit time values
(max ≈ 425 internal units) with its own non-linear curve. Times are ballpark, not exact.

### 🟡 B-07 — Filter coefficients are not bit-exact
VAZ filter DSP is fixed-point Q-format with runtime-computed 10-bit coefficient tables; the clone is
float. Cutoff pole coefficient + cubic *shape* match, but resonance amount, cubic constant and the
exact cutoff→Hz map are float approximations. **Bit-exact is fundamentally impossible** (documented in
`tools/VAZ_Clone_Audit.md`).

### 🟡 B-08 — Type C extra soft-clip
Manual calls Type C a *resonant lowpass*; clone adds a `soft()` saturation that may not belong.
Minor character difference.

### 🟡 B-09 — LFO sync period range too narrow
Clone offers 4B…1/8; VAZ offers 32nd-note … 256 beats. Limits very slow/fast synced LFOs.

### 🟡 B-10 — Octave on `.v2p` load assumes one tuning base
`loadV2P` maps OSC1 octave from `LE16` cents with hard-coded base `63136` (from the 1565-byte Init).
Factory presets with a different internal base may load at the wrong octave.

---

## WRONG / UNVERIFIED PARAMETER RANGES
| Param | Clone range | VAZ | Action |
|-------|-------------|-----|--------|
| Pitch-bend | fixed ±2 st | 1–24 st (Bend control) | add `bend_range` param |
| Unison voices | fixed 4 | UniV 1–32 (≤ Voices) | add `uni_voices` |
| Voices | fixed 16 | up to 32 + Dynamic | raise `kNumVoices`, add Dynamic |
| LFO sync period | 6 steps | 32nd…256 beats | widen `*_period` choices |
| Cutoff→Hz | `20·900^x` (20 Hz–18 kHz) | unverified vs VAZ | measure self-osc to calibrate |
| Arp rate | 6 divisions | full set + triplets/dotted | extend if needed |

---

## ✅ FIXED (recent)
- **Amplitude does not affect output** (PRIMARY BUG): the Amplifier → Amplitude-Modulation **slot-1
  depth slider** (= the master volume) had NO `data-param` (unwired); the voice used a fixed `×0.5`.
  → added `amp_level` param, wired the slider, voice now `× p.ampLevel`. Volume slider works.
- **Unison silent/collapsed**: `juce::Synthesiser::noteOn` stops same-note voices → 4 unison notes
  became 1. → `VAZSynth` subclass skips the stop-ringing-note loop; +5c baseline detune for audibility.
- **+/- mod sign**: were decorative (always +). Now a working toggle (− inverts) on Cutoff-Mod 1/2,
  Res-Mod, Amp-Mod, Pan-Mod; all +/- toggle visually.
- **LFO sync periods**: 6 → full VAZ 24 (1/32T … 256 beats).
- **GUI**: removed grey surround + scroll; dark top bar; window fits content (604×460); removed
  stray box-shadow "black square" corner.

## ✅ FIXED THIS PROJECT (history)
- Filter "fade" on ENV2 attack=0 → custom `VAZLadder` (no cutoff smoothing) → instant DONK.
- OSC fine-tuning not working / retrigger click → free-running phases + quantised coarse/fine.
- Multi-saw detune → FFT-matched (±36c, 24c spacing).
- Pitch bend DEAD (empty `pitchWheelMoved`) → implemented ±2 st.
- `note_priority` dead → High/Low/Last wired in mono.
- Filter distortion on Type C/D → removed 4× drive from clean engines; added no-makeup `soft()`.
- Type C separation never implemented → 2nd-stage cutoff offset by `aux`.
- Type D constant self-oscillation → tamed q (≥0.08) + removed ±2 boost.
- Type K wrong topology (4-pole) → **2-pole Sallen-Key** (manual-confirmed).
- Detune inaudible (clustered random voice positions) → fixed WIDE spread array.
- GUI chrome clutter (fake titlebar/transport) → removed.
