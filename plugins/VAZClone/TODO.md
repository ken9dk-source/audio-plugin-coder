# VAZClone — TODO (prioritised)

Prioritised implementation roadmap derived from `MISSING_FEATURES.md` + `BUGS.md` +
`GHIDRA_GAP_ANALYSIS.md`. Priority = impact on faithful sound/behaviour ÷ effort.

---

## ⭐ P0 — Highest impact (do first)

- [ ] **Per-voice filter** (BUG B-01). Move `VAZMultiFilter` + a per-voice filter envelope into
      `VAZVoice`. Unlocks correct chord/legato filter behaviour, per-voice key-track, and the
      Osc1/Osc2/Voice# mod sources. *Biggest single accuracy win; large refactor.*
- [ ] **Extend `.v2p` load map** (BUG B-02). Add OSC1 level/coarse/fine, OSC2 oct/wave/fine,
      mod-routing sources+depths, `hp_cutoff`, `flt_aux`, FM, waveshape-mod. Method: export one
      isolated `.v2p` per parameter from real VAZ (as in `Desktop\Vaz Parameters`), diff vs Init,
      add the offset to `loadV2P`/`buildV2P`. → makes the 601 factory presets load accurately.

## ⭐ P1 — High impact

- [ ] **Stereo voices + Pan Modulation + Voice Number** (D2, P-stereo). Render voices to stereo,
      add `pan_mod_src/amt`, wire Voice# as a per-voice mod source. Enables the classic Unison-pan sound.
- [ ] **Sample oscillator + Sample Loader** (D3/O2). Load a single-cycle/multisample (`wavetbl2.wav`),
      play it in osc mode 3 (currently sine only).
- [ ] **Env 2 Dest** (D12): `e2_mod_src/amt` + `e2_dest` (A/D/R) modulating Env2 times.
- [ ] **3rd cutoff-mod input** (D11) + **2nd/3rd FM slot** (D10).
- [ ] Wire remaining **mod sources** (after per-voice filter): Osc1, Osc2, External, Voice#, MIDI-Ctrl-B.

## ⭐ P2 — Medium

- [ ] **Arpeggiator polish**: As-played order, Random 1/2, an Arp-Mode popup.
- [ ] **Performance completeness**: Duo mode (P1), UniV count (P2), separate Poly/Unison detune (P3),
      Portamento Auto/Exp (P4), **bend range** param (P5), 32 voices + Dynamic (P6).
- [ ] **Osc 3 mode** (D4): LFO1 at audio rate tracking the keyboard (footage table 32'=48…2'=240).
- [ ] **Link** button (O1/G4): Osc1 mod-input 1 → Osc2/Osc3.
- [ ] **Mixer Post** buttons (G3): per-channel pre/post-filter routing (needs a 2nd accumulation bus).
- [ ] **Multi** envelope button (G2): mono-legato retrigger.

## ⭐ P3 — Lower / niche

- [ ] **Step Sequencer** (D6) → unlocks Accent + Seq A/B mod sources.
- [ ] **Comb Damping + I/O Mix** (D8); **Mod-Amp SQ** mode (D13).
- [ ] **Microtuning** `.tun` loading (D7).
- [ ] **External Input** osc/mixer source (D5).
- [ ] Widen **LFO sync** period range to 32nd…256 beats (B-09/G5).
- [ ] **Slew-limit** on cutoff for A-C as an *optional* toggle (B-03/D9) — keep instant DONK default.
- [ ] Calibrate envelope curves (B-06) + cutoff→Hz + resonance by **measuring** real VAZ output (FFT).

## 🧹 Polish / non-functional
- [ ] Performance flip-lever graphic (G8) — radios are functionally correct.
- [ ] Level-match A/B/C/D vs R/K if the type-switch jump (B-03) is annoying.
- [ ] One global noise stream (B-04); velocity as a mod route not hard-wired (B-05).

---

### Suggested next sprint
1. **Per-voice filter** (P0) — biggest correctness gain.
2. **`.v2p` map completion** (P0) — makes the whole factory library usable.
3. **Stereo + Pan + Voice#** (P1) — the signature Unison sound.
