# DSP Architecture Specification — VAZClone (v1 Core)

Faithful VAZ 2010 clone. **OSC sound fidelity is requirement #1** — every choice below serves
matching the measured reference spectra (`Desktop\Vaz Parameters\VAZ_OSC_Analysis.md`).

## Core Components

**Per-voice DSP:**
- **Oscillator ×2** — band-limited VA engine:
  - PolyBLEP **saw / square / pulse** + **sine** + **wavetable** + **detune ensemble**
  - Runs at **2× oversampling** (matches "Oversample x2 below 50kHz")
  - **Analog HF tilt** post-filter on the raw waveform (gentle 1-pole-ish) to match the
    measured −3 dB@H16 / −6 dB@H24 saw rolloff (this is the "warm not harsh" character)
  - Tuning: octave (32'..2'), coarse ±12 semi, fine ±100 cents
  - Modifier knob = pulsewidth / detune-spread / wavetable-pos by mode
- **Noise generator** (white)
- **Ring modulator** (OSC1 × OSC2)
- **Mixer** — 3 channels (OSC1, OSC2, Noise/RingMod), level + pre/post-filter routing
- **Filter** — multimode, 22 algorithms:
  - Priority: **Type R 4-pole ladder** (Zero-Delay-Feedback / TPT, Moog-style) = default 19
  - **2-pole State-Variable** covers Type A/B/D LP/BP/HP
  - Comb / Sallen-K / C-types = phase-2 stubs (selectable, fall back to nearest)
  - Cutoff, Resonance, Highpass Cutoff (1-pole HP for C/R types)
- **Amplifier** — VCA (driven by Env1) → **Overdrive** (tanh-style waveshaper) → Pan
- **Envelope ×2** — analog ADSR, exp/lin Curve, Cycle (loop); Env1=amp, Env2=mod+Dest
- **LFO ×1** — 8 waveforms (Saw/Tri … S&H), free or tempo-sync
- **Modulation router** — source→dest with bipolar depth (v1: cutoff, pitch, amp, pan)

**Voice/global:**
- **Voice manager** — Mono / Poly / Unison / Duo; steal High/Low/Last; Unison voices+detune; Poly detune; Portamento (Auto/Exp)
- **Oversampling wrapper** — 2× (Quality: Low/LoMid/HiMid/High = OS factor)
- **Preset manager** — **.v2p import** (byte map in spec §10) + native APVTS state
- **Parameter management** — JUCE APVTS

## Processing Chain (per voice)
```
 [OSC1]──┐
 [OSC2]──┼─►[Mixer + RingMod]─►[Filter]─►[VCA]─►[Overdrive]─►[Pan]─►voice─┐
 [Noise]─┘                         ▲         ▲                            ├─►Σ─►out
                                   │         │                            │
   mod sources: [Env2][LFO1] ──────┴─────────┘   [Env1]──►VCA gain        │
                                                                   (poly sum)
 entire voice rendered at 2× then downsampled (steep half-band)
```

## Parameter → Component Mapping
| Parameter | Component | Function |
|-----------|-----------|----------|
| o1/o2 octave,coarse,fine | Oscillator | phase increment |
| o1/o2 wave, base, modifier | Oscillator | waveform select + shape/detune/wt-pos |
| o1/o2 level, noise, ringmod | Mixer | channel gains |
| filter_mode | Filter | algorithm select (0-21) |
| cutoff, resonance, hp_cutoff | Filter | coefficients |
| cutmod_src/amt | Mod router→Filter | cutoff modulation |
| overdrive | Amplifier | waveshaper drive |
| pan | Amplifier | stereo position |
| e1_A/D/S/R, curve | Envelope1 | VCA contour |
| e2_A/D/S/R, dest | Envelope2 | aux modulation |
| lfo1_rate/sync/wave | LFO | modulation oscillator |
| voice_mode, uni_voices, uni_detune | Voice manager | allocation + unison |
| poly_detune, portamento, bend | Voice manager | pitch behavior |
| quality | Oversampling | OS factor |

## Complexity Assessment
**Score: 4 / 5 (Expert)**

Rationale: full polyphonic **synthesis engine** — band-limited oversampled oscillators with a
spectral-match acceptance test, a ladder + multimode filter (22 modes), dual envelopes, LFO,
unison voicing, and binary **preset import**. Real-time-safe polyphony + oversampling pushes it
to Expert. (Not a 5: no ML/physical-modeling; v1 defers the full 22-source mod matrix, 3 LFOs,
arp and sequencer to v2.)
