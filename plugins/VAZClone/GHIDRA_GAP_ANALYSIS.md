# VAZClone — Ghidra ↔ Clone gap analysis

Function-level comparison of the decompiled `Vaz2010Core.dll` against the clone's C++ source.
Decompiles: `tools/vaz_big.c` (engine dump), `tools/vaz_osc.c`, `tools/vaz_coef.c`, `tools/vaz_decomp.c`;
strings: `tools/vaz_core_strings.txt`. Manual cross-check: `tools/chm_out/`.

`Vaz2010Core.dll` exports `CreateVAZ2010VST` / `CreateVAZ2010Standalone`; `VAZ2010VSTi.dll` is a thin
shim exporting `main`. Engine sections: `CODE` ≈ 1.2 MB, `.rsrc` ≈ 0.47 MB.

---

## The central function: `FUN_004dbddc` (0x4dbddc)
This is the **per-voice render**: oscillators → mixer → filter → amplifier, with the full per-sample
modulation matrix inline. The clone splits this into `VAZVoice::renderNextBlock` (osc+amp) and a
**global** `VAZMultiFilter` in the processor — the single biggest structural difference (see GAP-1).

Voice/param struct offsets identified in `FUN_004dbddc`:
| Offset | Meaning | Clone |
|--------|---------|-------|
| `+0x258` | filter mode index (0x00–0x5e) | `filter_mode` 0-21 ✅ exact (verified vs 24 `.v2p` bytes) |
| `+0x270` | resonance | `resonance` ✅ |
| `+0x274` | drive (mixer level into filter) | `fltDrive` ✅ (R/K only, manual-confirmed) |
| `+0x290/+0x294` | a mod source ptr + depth | mod matrix ✅ (see GAP-7) |
| `+0x218/+0x21c/+0x220` | mod {source, sign, depth} triplets | `ModBus.value()` 🟡 (13/22 sources) |
| `0x158…0x1b0` | filter state words | per-engine state in `VAZMultiFilter` ✅ |

---

## Module-by-module

### OSC — `vaz_osc.c`, FUN_004dbddc osc section | clone `Synth.h` (WaveTables/OscBlock)
- ✅ **Wavetable osc, 4-point cubic** interpolation (Ghidra Catmull/Hermite read) → `WaveTables::read` cubic.
- ✅ **Multi-Saw** 4 detuned saws, ±36c / 24c spacing — FFT-confirmed AND Ghidra distribution match.
- ✅ Free-running phase (no note-on reset) → matches (no click).
- ✅ OSC2 hard **Sync** (slave phase reset on master wrap) → `osc2.hardReset()`.
- 🟡 **Sample mode** (loads `wavetbl2.wav` multisample) → clone plays sine only. **GAP-3**.
- ❌ **Link** (Osc1 mod-input → all oscs). **GAP-O1**.
- 🟡 FM: 1 slot vs 2-3. **GAP-D10**.

### FILTER — FUN_004dbddc 0x1093-0x1655 dispatch on `uVar12` | clone `VAZMultiFilter`
Six engines decoded from the dispatch (each with its own coef-table set):
| Type | Ghidra topology | Tables | Clone |
|------|-----------------|--------|-------|
| A | clean 2-pole biquad, 3 taps | `005d/0059/0055` | TPT-SVF ✅ |
| B | biquad + bandwidth | (A family) | SVF, aux→Q ✅ (manual-confirmed) |
| C | cascaded biquad + cubic + separation | `0069/0065/0061` | cascade SVF + sep ✅ (🟡 extra soft-clip) |
| D | Chamberlin SVF + soft-clip ±0xD105E8 | `006d67/77` | Chamberlin ✅ |
| K | **2-pole Sallen-Key**, dirty self-osc | `006d87/97`* | ✅ FIXED (was 4-pole) |
| R | 4-pole integrator cascade + cubic + 1-pole HP | `006da7` interp | `VAZLadder` ✅ |
| Comb | tuned delay + feedback | delay buf @0x19c&0x7fff | ✅ (🟡 no Damping/Mix) |
- ✅ Cutoff pole coefficient **exact** (`g = 1 − ((2−cosω) − √((2−cosω)²−1))`, Ghidra @0x4D4720).
- ✅ Cubic saturation *shape* `x − k·x³` matches.
- 🔴 Coefficient **tables filled via indirect addressing** → not extractable; **fixed-point ≠ float**, so
  resonance amount / cutoff→Hz are approximations. Bit-exact impossible. **BUG B-07**.
- 🔴 Filter is **per-voice** in VAZ, **global** in clone. **GAP-1 / BUG B-01**.

### ENVELOPES — clone `VAZEnv`
- ✅ ADSR + Reset (zero-before-attack) + Cycle (loop = LFO) + Curve. Manual-confirmed semantics.
- ❌ **Multi** (mono legato retrigger). **GAP-G2**. ❌ **Env2 Dest** (A/D/R self-mod). **GAP-D12**.
- 🟡 Time curve is an approximation (16-bit ≤425 units in `.v2p`). **BUG B-06**.

### LFOs — clone `ModLFO`
- ✅ 8 waveforms (Saw/Tri morph, +Delay fade-in ×4, Pulse morph, S&H+Lag/+Delay) + shape + **Trig** + sync.
- ✅ LFO2 **Rate Modulation** (per-sample). ✅ LFO3 tri/sine.
- ❌ **Osc3 mode** (LFO1 audio-rate keyboard-tracking). **GAP-D4**. 🟡 sync range 4B…1/8 vs 32nd…256b.

### MOD MATRIX — FUN_004dbddc {src,sign,depth} triplets | clone `ModBus`
- ✅ Shape matches (per-destination {source idx, ± sign, depth} reading a source array).
- ✅ Mod Amplifiers 1/2 (In×AM) + Lag (1-pole slew) — manual-confirmed.
- 🟡 13/22 sources wired; **9 stubs** (Osc1/2, Ext, Accent, SeqA/B, Voice#, MIDI-CtrlB). **GAP-3**.
- ❌ Mod-Amp **SQ** mode. **GAP-D13**.

### ARP / VOICE MGMT — clone `processArp` + `juce::Synthesiser`
- ✅ Arpeggiator: held-note set → sorted+octave+mode sequence → tempo-synced clock → noteOn/Off.
  Modes Up/Down/Up&Down/Random, Rate 1/4-1/32, Octaves 1-4, Hold.
- ❌ As-played, Random1/2 split, Trigger-Free, Arp-Mode dialog. **GAP §6**.
- ✅ Voice modes Mono (note-priority Last/High/Low) / Poly / Unison; pitch-bend ±2.
- ❌ **Duo** mode, UniV count, 32 voices, Dynamic-voice cull. **GAP P1/P2/P6**.

### PRESET I/O — clone `loadV2P`/`buildV2P`
- ✅ `.v2p` container parsed via PRST + double-MSmp anchors (works on 1565 B AND 11 KB factory files).
- 🟡 ~21/≈60 params mapped; fine details unmapped → presets load **approximate**. **BUG B-02**.

---

## Confidence
| Area | Confidence | Basis |
|------|-----------|-------|
| Filter mode mapping (0-21) | **Exact** | 24 isolated `.v2p` byte diffs |
| Filter topologies (6 engines) | High | Ghidra dispatch + manual Filter-Mode-Dialog |
| Oscillator (cubic, multisaw, sync) | High | Ghidra + FFT of real renders |
| Filter coefficients (exact values) | Low | fixed-point tables, indirect addressing — unrecoverable |
| Envelope/LFO curves | Medium | manual + structure; exact curve unmeasured |
| Mod-matrix shape | High | FUN_004dbddc triplets |

## Top structural gaps (ranked)
1. **Per-voice filter** (GAP-1) — correctness of chords/legato + unlocks per-voice mod sources.
2. **`.v2p` map completion** — usability of the 601 factory presets.
3. **Stereo + Pan + Voice#** — the signature Unison-pan sound.
4. **Sample oscillator**, **Env2 Dest**, **Sequencer**.
