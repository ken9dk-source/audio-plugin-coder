# DSP Architecture Specification: SID Trance Machine

## Core Components

### 1. SIDOscillator (per voice × 3)
Emulates MOS 6581/8580 oscillator behavior with modern extensions.
- **Waveform generator:** SAW, TRI, PULSE (with PW), NOISE (LFSR), SAW+TRI blend, RING MOD
- **Phase accumulator:** 24-bit fixed-point stepping (authentic SID behavior)
- **TRANCE DRIFT:** Per-voice random pitch micro-wander (±cents, filtered noise source)
- **DIGITAL AGE:** Optional bit-reduction / sample-rate decimation post-waveform
- **Oscillator sync:** Hard sync from OSC1 → OSC2 / OSC1 → OSC3
- **Per-osc ADSR envelope:** Controls individual oscillator amplitude

### 2. SIDFilter (per voice)
Approximation of the non-linear MOS 6581 ladder filter — not pixel-perfect emulation,
but musically accurate. Inspired by Huovilainen's non-linear Moog model adapted for SID.
- **Types:** LP12, LP24, HP12, HP24, BP, Notch
- **Self-oscillation:** Resonance → feedback → self-oscillation at Q > ~0.85
- **Key track:** Cutoff tracks MIDI note ± amount
- **Velocity sens:** Cutoff offset from MIDI velocity
- **Non-linearity:** Soft-clip in feedback path for SID character

### 3. SIDVoice (Voice container)
Bundles OSC1 + OSC2 + OSC3 + Filter + AmpEnvelope per polyphonic voice.
- Manages voice steal, note-on/off routing
- Applies mod matrix values per voice
- Outputs mono float sample per process tick

### 4. PolyEngine (Voice allocator)
- Manages up to 16 `SIDVoice` instances
- Voice allocation strategies: oldest-steal, lowest-priority, round-robin
- **SID WIDTH (Unison):** Up to 7 sub-voices per note, spread via `sid_width` param
- **ANALOG GLOW:** Soft saturation stage post-summing (waveshaper: tanh approximation)
- Mono sum → stereo spread via Haas-effect delay on unison sub-voices

### 5. ModMatrix
4-slot fully declarative modulation router.
- Sources: LFO1, LFO2, Velocity, ModWheel, AmpEnv, MIDI Key
- Destinations: FilterCutoff, OSC1/2/3 PW, OSC1/2/3 Fine, AmpLevel, FilterRes
- Bipolar amount (-1.0 to +1.0) with smoothed parameter output
- Applied before final per-voice DSP computation

### 6. LFOEngine × 2
- Shapes: Sine, Triangle, Saw, RevSaw, Square, Sample & Hold
- Free-running or tempo-sync (subdivisions of host BPM)
- Retrig on note-on
- Rate range: 0.01 Hz – 20 Hz (or tempo-synced divisions)

### 7. ArpSequencer
- 16-step on/off pattern
- Modes: Up, Down, UpDown, Random, As-Played
- Octave range: 1–4 oct
- Gate, Swing, Tempo (internal or DAW-sync via `AudioPlayHead`)
- Generates MIDI-note events fed to PolyEngine

### 8. FXChain (stereo, post-poly)
Serial chain: **Chorus → Delay → Reverb**

| Module | Algorithm |
|---|---|
| **Chorus** | 2-tap BBD-style chorus with LFO-modulated delay lines |
| **Delay** | Ping-pong delay, tempo-sync, filtered feedback path |
| **Reverb** | Freeverb-style Schroeder network (8 combs + 4 allpass) |

Each unit has independent enable/disable and dry/wet mix.

### 9. MasterSection
- Output gain stage
- Hard limiter (lookahead brickwall, 1ms lookahead)
- Mono/Poly mode switch (collapse stereo to mono when needed)

### 10. PresetSystem
- JUCE `AudioProcessorValueTreeState` (APVTS) drives all parameters
- Preset saved as XML to user app-data directory
- Banks: FACTORY and USER
- INIT preset resets all params to defaults

---

## Processing Chain (Signal Flow)

```
MIDI In
  │
  ▼
ArpSequencer ─── (if enabled, rewrites note events)
  │
  ▼
PolyEngine (16 voices)
  │  ┌──────────────────────────────────────────────┐
  │  │  SIDVoice × N                                │
  │  │   OSC1 ──┐                                   │
  │  │   OSC2 ──┼──► Mixer ──► SIDFilter ──► AmpEnv│
  │  │   OSC3 ──┘                                   │
  │  └──────────────────────────────────────────────┘
  │     ▲
  │   ModMatrix (LFO1, LFO2, Velocity, ModWheel)
  │
  ▼
Voice Sum + Unison Spread (SID WIDTH)
  │
  ▼
ANALOG GLOW (tanh saturation)
  │
  ▼
FX Chain: Chorus → Delay → Reverb
  │
  ▼
Master Section: Output Gain → Limiter
  │
  ▼
Audio Out (Stereo)
```

---

## Parameter Mapping

| Parameter | Component | Function |
|---|---|---|
| `osc[1-3]_wave` | SIDOscillator | Selects waveform type |
| `osc[1-3]_semi/fine` | SIDOscillator | Pitch offset (semitone + cents) |
| `osc[1-3]_pw` | SIDOscillator | Pulse width (PULSE wave) |
| `osc[1-3]_a/d/s/r` | SIDVoice per-osc env | Individual osc amplitude shape |
| `osc[1-3]_volume` | SIDVoice mixer | Per-osc output level |
| `filter_cutoff` | SIDFilter | Base cutoff frequency |
| `filter_res` | SIDFilter | Resonance / Q amount |
| `filter_type` | SIDFilter | LP/HP/BP/Notch switch |
| `filter_slope` | SIDFilter | 12dB vs 24dB mode |
| `filter_keytrack` | SIDFilter | MIDI note → cutoff offset |
| `filter_velocity` | SIDFilter | Velocity → cutoff offset |
| `amp_a/d/s/r` | SIDVoice amp env | Global amplitude shape |
| `amp_volume` | PolyEngine | Final voice output scale |
| `lfo[1-2]_*` | LFOEngine | LFO shape, rate, sync, retrig |
| `mod[1-4]_src/amt/dst` | ModMatrix | Routing rules |
| `fx_chorus_*` | FXChain::Chorus | Chorus params |
| `fx_delay_*` | FXChain::Delay | Delay params |
| `fx_reverb_*` | FXChain::Reverb | Reverb params |
| `master_*` | MasterSection | Output, limiter, poly/mono |
| `arp_*` | ArpSequencer | Arp mode, timing, pattern |
| `seq_step_*` | ArpSequencer | 16-step on/off pattern |
| `trance_drift` | SIDOscillator | Pitch micro-wander depth |
| `digital_age` | SIDOscillator | Bitcrush/SR-crush depth |
| `sid_width` | PolyEngine | Unison voice spread |
| `analog_glow` | PolyEngine | Saturation amount |
| `sid_mode` | SIDOscillator + Filter | Classic/Modern/Trance behavior |

---

## Class Structure (C++)

```
SIDTranceAudioProcessor  (PluginProcessor)
├── APVTS (AudioProcessorValueTreeState)
├── PolyEngine
│   ├── SIDVoice[16]
│   │   ├── SIDOscillator osc1, osc2, osc3
│   │   ├── SIDFilter filter
│   │   ├── AmpEnvelope ampEnv
│   │   └── OscEnvelope oscEnv[3]
│   └── ModMatrix
├── LFOEngine lfo1, lfo2
├── ArpSequencer arp
├── FXChain
│   ├── SIDChorus chorus
│   ├── SIDDelay delay
│   └── SIDReverb reverb
└── MasterSection

SIDTranceAudioProcessorEditor  (PluginEditor)
└── [WebView or Visage UI layer]
```

---

## Complexity Assessment

**Score: 5 / 5 — Research/Expert Level**

| Factor | Weight |
|---|---|
| 3 independent oscillators with per-osc envelopes | High |
| SID filter non-linear emulation | High |
| 16-voice polyphony + unison engine | High |
| 4-slot mod matrix with smoothing | Medium |
| 2 LFOs with tempo-sync | Medium |
| 16-step arpeggiator with DAW sync | Medium |
| 3-module FX chain (chorus + delay + reverb) | Medium |
| TRANCE DRIFT (per-voice random noise source) | Medium |
| DIGITAL AGE (real-time bitcrusher) | Low |
| Real-time oscilloscope display | High |
| Preset system with bank management | Medium |

**Total: Complexity 5/5.** This is a full professional synthesizer.
Estimated implementation: 4–6 weeks of focused development.

---

## Performance Targets
- Buffer size: 64–512 samples
- Sample rate: 44100 / 48000 / 96000 Hz
- CPU budget: < 15% single core @ 44.1kHz / 16 voices
- Memory: < 50MB VST3 footprint
- Latency: Zero additional (non-lookahead path), 1ms lookahead for limiter only
