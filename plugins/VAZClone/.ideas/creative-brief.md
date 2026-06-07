# Creative Brief — VAZClone

## Hook
A faithful, modern recreation of the **VAZ 2010** virtual-analog synthesizer — the same
oscillators, the same Moog-style ladder filter, the same navy/orange/wood look — rebuilt as a
modern VST3 that loads your original `.v2p` patches.

## Description
VAZClone is a 1:1 clone of Software Technology / Barry Klein's **VAZ 2010** VSTi. The mission is
**sonic identity first**: the oscillators must be indistinguishable from the original, because the
oscillators *are* the VAZ sound. Everything else (filter, envelopes, amp) is built to preserve that
character through the signal chain.

### Non-negotiable requirement #1 — OSC sound fidelity
- VAZ oscillators run at **2× oversampling below 50 kHz** with 4 quality modes (Low→High).
- Band-limited VA waveforms (Sine/Tri/Saw/Square/Pulse) + Ring Mod + Hard Sync + Detune (unison).
- The clone's oscillators are tuned against **reference WAV renders** from the real VAZ
  (FFT spectral match) — see `VAZ2010_Clone_Spec.md` §12. This is the acceptance test for v1.

### Killer feature — original preset compatibility
The full `.v2p` binary format is reverse-engineered (see spec §10). VAZClone **imports original
VAZ patches** directly, giving instant access to 1000+ factory patches and all generated banks.

## Scope — v1 (Core First)
v1 nails the core voice; advanced modulation comes in v2.

**In v1:**
- OSC1 + OSC2 (full waveform modes, tuning, modifier) + Noise
- Mixer (3 ch) + Ring Mod
- Filter: all 22 modes (ladder R-type is the priority/default)
- Amplifier: Overdrive + Pan
- Envelope 1 (amp) + Envelope 2 (mod) — full ADSR + Curve/Cycle
- 1 LFO + cutoff/pitch modulation (enough for movement)
- Performance: Mono/Poly/Unison/Duo, Unison Voices + Detune, Portamento, Poly Detune, Bend
- `.v2p` preset import
- Quality/oversampling control

**Deferred to v2:**
- LFO 2 & 3, full 22-source mod matrix, Mod Amplifiers 1/2, Lag Processor
- Arpeggiator, Accent sequencer, wavetable editor / sample loading

## Reference & aesthetic
- Original GUI replicated in `VAZ2010_v4.html` (navy panel, orange text, mahogany edges).
- Likely **WebView** UI (reuse the HTML) — framework confirmed in /plan.

## Acceptance criteria (v1)
1. A/B oscillator render vs VAZ reference: harmonic spectra match within tolerance.
2. Loads and correctly sounds an original `.v2p` patch.
3. Ladder filter sweep matches VAZ character.
