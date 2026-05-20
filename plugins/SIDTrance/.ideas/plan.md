# Implementation Plan: SID Trance Machine

## Complexity Score: 5 / 5

## Implementation Strategy: Phased (5 phases)

Following the roadmap from the AI Guide:

---

## Phase A: Core Mono Engine (Fase 1 equivalent)
**Goal:** Single-voice, single-oscillator pipeline that produces audible output.

- [ ] `SIDTranceAudioProcessor` skeleton (JUCE boilerplate)
- [ ] APVTS with all parameters registered
- [ ] `SIDOscillator` — SAW + PULSE + TRI waveforms via phase accumulator
- [ ] `SIDFilter` — LP24 Huovilainen-style with resonance
- [ ] `AmpEnvelope` — ADSR with linear/exponential segments
- [ ] `SIDVoice` — Wire OSC → Filter → AmpEnv
- [ ] Basic mono MIDI note-on/off → single voice
- [ ] Verify: DAW loads plugin, plays a note, outputs audio

---

## Phase B: Full Polyphony + All Oscillators (Fase 2 equivalent)
**Goal:** 16-voice polyphony with all 3 oscillators and per-osc envelopes.

- [ ] `PolyEngine` — voice allocator, 16 voices, oldest-steal
- [ ] OSC2 and OSC3 added to SIDVoice
- [ ] Per-osc ADSR envelopes (independent from amp env)
- [ ] Oscillator mixer (3-osc → 1 signal path → filter)
- [ ] All waveform types: NOISE (LFSR 23-bit), RING MOD, SAW+TRI blend
- [ ] `osc_semi` / `osc_fine` pitch offsets applied
- [ ] PULSE WIDTH modulation working
- [ ] Filter types: LP12, LP24, HP, BP, Notch
- [ ] Key track + velocity sensitivity on filter

---

## Phase C: Modulation Engine (Fase 3 equivalent)
**Goal:** LFOs, Mod Matrix, and Arp fully operational.

- [ ] `LFOEngine` × 2 — all shapes, free rate and host-sync
- [ ] LFO retrig on note-on
- [ ] `ModMatrix` — 4 slots, source/amount/destination routing
- [ ] Parameter smoothing on all mod destinations (no zipper noise)
- [ ] `ArpSequencer` — 16-step on/off, Up/Down/UpDown/Random modes
- [ ] Arp tempo sync via `AudioPlayHead`
- [ ] Gate + Swing implementation

---

## Phase D: Unison + FX Chain + SID Specials (Fase 4 equivalent)
**Goal:** Professional trance sound — stereo width, FX, unique features.

- [ ] `SID WIDTH` — unison sub-voices (up to 7), Haas stereo spread
- [ ] `TRANCE DRIFT` — per-voice LF noise source → pitch offset
- [ ] `DIGITAL AGE` — bitcrusher (bit depth + sample rate reduction)
- [ ] `ANALOG GLOW` — tanh saturation post-voice-sum
- [ ] `SID MODE` switch — Classic (aliasing allowed) / Modern (bandlimited) / Trance (all features)
- [ ] `FXChain::Chorus` — 2-tap BBD with LFO modulation
- [ ] `FXChain::Delay` — ping-pong, tempo sync, filtered feedback
- [ ] `FXChain::Reverb` — Freeverb Schroeder network
- [ ] `MasterSection` — Output gain + lookahead limiter
- [ ] Preset system — APVTS XML save/load, FACTORY + USER banks, INIT

---

## Phase E: GUI + Polish (Fase 5 equivalent)
**Goal:** Full UI matching the reference design image + final optimization.

- [ ] Waveform oscilloscope display (top panel — real-time FFT/waveform)
- [ ] All knobs, sliders, buttons wired to APVTS parameters
- [ ] Preset browser (name display, bank dropdown, SAVE / SAVE AS / INIT)
- [ ] MOD MATRIX routing UI (source + dest dropdowns, amount sliders)
- [ ] ARP / SEQUENCER 16-step button grid
- [ ] Final visual polish matching reference: dark navy + cyan neon aesthetic
- [ ] CPU profiling — ensure < 15% @ 16 voices, 44.1kHz
- [ ] Cross-platform testing (Windows VST3, macOS AU + VST3)

---

## JUCE Modules Required

```cmake
target_link_libraries(SIDTrance PRIVATE
    juce::juce_audio_basics
    juce::juce_audio_processors
    juce::juce_dsp
    juce::juce_gui_basics
    juce::juce_gui_extra        # WebView2 (if WebView chosen)
    juce::juce_audio_formats
)
```

## Dependencies
| Dependency | Purpose | Status |
|---|---|---|
| JUCE 8 | Framework | Required |
| JUCE APVTS | Parameter system + preset | Built-in |
| JUCE DSP | Oversampling, filter utilities | Built-in |
| Freeverb | Reverb algorithm | Embed source (~200 lines) |

No Eigen or xsimd required for Phase A–D. SIMD optimization is a Phase E task.

---

## Risk Assessment

**High Risk:**
- SID filter non-linearity — the 6581's character is hard to approximate faithfully.
  Mitigation: Use Huovilainen's ladder model as base, tune by ear.
- Real-time oscilloscope display — must be lock-free. Use `juce::AbstractFifo`.
- Unison voice count × 16 poly voices = up to 112 DSP instances. Profile early.

**Medium Risk:**
- Mod matrix parameter smoothing — 4 destinations updating every buffer = zipper noise risk.
  Mitigation: `juce::SmoothedValue<float>` on all mod targets.
- Arp DAW sync — `AudioPlayHead` may not always be available.
  Mitigation: Graceful fallback to internal clock.
- Preset XML compatibility — parameter name changes break saved presets.
  Mitigation: Finalize all parameter IDs in APVTS before Phase E.

**Low Risk:**
- Basic oscillators (well-understood algorithms)
- Chorus / Delay / Reverb (standard implementations)
- MIDI voice allocation (standard JUCE pattern)

---

## Suggested Starting Preset: "TRANCE_LEAD_01"
Match the state shown in the reference UI image:
- OSC1: SAW, 0 semi, 0 fine, PW 50%
- OSC2: TRI, 0 semi, -7 cents, PW 50%
- OSC3: NOISE, 0 semi, 0 fine
- Filter: LP 24dB, Cutoff 2480Hz, Res 35%
- LFO1: Sine → Filter Cutoff +37.5%
- LFO2: Triangle → OSC1 PW +25%
- Arp: Up, 1 Oct, Gate 75%, Swing 0%, Tempo 140 BPM, Sync ON
- FX: Chorus ON, Delay 1/4 ON, Reverb ON
