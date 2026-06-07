# Implementation Plan — VAZClone

## Complexity Score: 4 / 5 (Expert) → **Phased Implementation**

## Implementation Strategy (phased)

### Phase A — Oscillator engine (THE priority)
- [ ] PolyBLEP saw/square/pulse + sine + wavetable
- [ ] 2× oversampling wrapper + steep half-band downsample (juce::dsp::Oversampling)
- [ ] Analog HF tilt to match measured saw rolloff (−3dB@H16, −6dB@H24)
- [ ] Detune ensemble (N unison saws) for Waveform mode 2
- [ ] **Acceptance: render saw/square/C3 → FFT match vs `VAZ_OSC_Analysis.md`** (aliasing ≈ −42dB)

### Phase B — Filter
- [ ] Type R 4-pole ZDF ladder (default 19) — resonance, self-osc, tuning
- [ ] 2-pole SVF for Type A/B/D LP/BP/HP
- [ ] 1-pole highpass (HP Cutoff) for C/R types
- [ ] Cutoff modulation input (from mod router)
- [ ] Remaining modes select-and-fallback (Comb/K/C full = v2)

### Phase C — Envelopes + Amp
- [ ] ADSR ×2 (exp/lin curve, cycle), 16-bit-accurate time mapping vs .v2p
- [ ] VCA (Env1) → Overdrive (tanh waveshaper) → Pan
- [ ] Noise + Ring mod

### Phase D — Voicing
- [ ] Voice manager: Mono/Poly/Unison/Duo, steal modes
- [ ] Unison voices + detune, Poly detune, Portamento (Auto/Exp), Pitch bend

### Phase E — Modulation
- [ ] LFO1 (8 waves, tempo sync) + mod router (cutoff, pitch, amp, pan)

### Phase F — Preset import
- [ ] `.v2p` parser (landmark PRST/MSmp, byte map §10) → APVTS state
- [ ] Map filter-mode/voice-mode/ADSR(LE16)/cutoff/res/etc.

### Phase G — UI integration
- [ ] WebView host loads `VAZ2010_v4.html`
- [ ] Bridge: control ↔ APVTS parameter (window.__JUCE__), value display

## Dependencies
**Required JUCE modules:**
- juce_audio_basics, juce_audio_processors, juce_dsp (oversampling, ladder/SVF, waveshaper)
- juce_gui_basics, **juce_gui_extra** (WebBrowserComponent for WebView)
- juce_core, juce_data_structures (APVTS, .v2p binary parse)

**Build:** VST3, `NEEDS_WEBVIEW2 TRUE` (Win) / `NEEDS_WEB_BROWSER TRUE` (Linux); JUCE 8; CMake.

## Risk Assessment
**High Risk:**
- Oscillator spectral match (the whole point) — mitigated by reference FFT + Quality param
- Ladder filter stability/tuning at high resonance/self-oscillation
- 2× oversampling CPU with high unison voice counts

**Medium Risk:**
- `.v2p` import fidelity (envelope LE16 → time curve, filter-mode enum)
- WebView ↔ DSP parameter sync latency/threading
- Matching VAZ's exact detune-voice spread (needs 50%/100% renders)

**Low Risk:**
- Envelopes, noise, amp/pan, parameter management, basic mod routing

## UI Framework: **WebView**
Rationale: a faithful VAZ panel is already built in HTML/CSS (`VAZ2010_v4.html`) with correct
palette, layout, and byte-accurate enum ordering. WebView (juce_gui_extra) reuses it directly;
the dense custom panel (5 columns, dropdowns, sliders) suits HTML over per-pixel C++ redraw.
Implementation strategy: **phased**.
