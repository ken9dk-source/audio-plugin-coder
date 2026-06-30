# QuadraFuzz — Implementation Plan

## Complexity Score: 3 / 5

## UI Framework: WebView

**Rationale:** QuadraFuzz has a frequency-response EQ display that updates in real-time as crossover frequencies change. This kind of canvas-based curve rendering is far simpler in HTML5 Canvas than in Visage custom drawing. The original plugin's aesthetic (dark, VST2-era) is easy to replicate with CSS. WebView2 is the right call.

---

## Implementation Strategy: Phased

### Phase 3.1: Core DSP (PluginProcessor)

- [ ] Scaffold `CMakeLists.txt` with `NEEDS_WEBVIEW2 TRUE`
- [ ] Create `BandProcessor` class (LR4 crossover pair + waveshaper)
- [ ] Wire 4× `BandProcessor` instances in `PluginProcessor`
- [ ] Implement multipass loop
- [ ] Implement all 21 parameters via `AudioProcessorValueTreeState`
- [ ] Parameter smoothing (`SmoothedValue<float>`) on gain and freq changes
- [ ] Solo/Enable routing logic
- [ ] Hard clipper + Dry/Wet mixer
- [ ] Factory presets (14 presets, hardcoded from binary data)

### Phase 3.2: UI (WebView + HTML/JS)

- [ ] `PluginEditor` sets up `juce::WebBrowserComponent`
- [ ] `Design/index.html` layout:
  - 6 knobs (Gain, Low, LowMid, HiMid, High, Out)
  - 4 shape-selector buttons per band (or global Shape column)
  - EQ frequency response canvas (draws band gains as curves)
  - Delete | Create | Solo checkboxes
  - Presets dropdown
- [ ] JS ↔ JUCE parameter binding via `window.__JUCE__`
- [ ] Real-time EQ curve updates (redraws on freq/gain param changes)

### Phase 3.3: Polish

- [ ] Preset load/save via APVTS state
- [ ] Tooltip display format strings (%2.1f kHz / %3.0f Hz / %4.1f dB)
- [ ] Clip indicator (visual flash on hard clip events)
- [ ] Build and install test in FL Studio

---

## File Structure

```
plugins/QuadraFuzz/
├── CMakeLists.txt
├── status.json
├── .ideas/
│   ├── creative-brief.md
│   ├── parameter-spec.md
│   ├── architecture.md
│   └── plan.md
├── Design/
│   └── index.html         ← WebView UI
└── Source/
    ├── PluginProcessor.h
    ├── PluginProcessor.cpp
    ├── PluginEditor.h
    ├── PluginEditor.cpp
    └── BandProcessor.h    ← per-band DSP unit (header-only)
```

---

## Required JUCE Modules

```cmake
juce_audio_basics
juce_audio_processors
juce_dsp              # LinkwitzRileyFilter, SmoothedValue
juce_gui_extra        # WebBrowserComponent
juce_gui_basics
```

## CMake Key Flags

```cmake
juce_add_plugin(QuadraFuzz
    FORMATS VST3
    PRODUCT_NAME "QuadraFuzz"
    NEEDS_WEBVIEW2 TRUE
)
target_compile_definitions(QuadraFuzz PUBLIC JUCE_WEB_BROWSER=1)
```

---

## Risk Assessment

**High:**
- LR4 crossover coefficient updates mid-playback (freq param changes) must be done safely — use `SmoothedValue` on cutoff, update filter coefficients on audio thread only when stable
- Multipass loop with high pass counts (>4) may cause CPU spikes on dense material

**Medium:**
- WebView2 JS↔JUCE parameter binding latency (use polling or push events, not pull)
- EQ display canvas redraw performance (throttle to ~30fps, not per-sample)

**Low:**
- Waveshaper math (all closed-form, no lookup tables needed)
- Preset system (hardcoded values, no file I/O)
- Solo/enable routing (simple boolean logic)

---

## Factory Preset Values (to hardcode)

Confirmed from binary (Default + DrumSmasher exact; others approximated):

```cpp
struct Preset {
    const char* name;
    float band_gain[4];   // dB
    float band_freq[4];   // Hz
    int   band_shape[4];
    float in_gain, out_gain, dry_wet;
    int   multipass;
    float metapass_db;
};

Preset presets[] = {
    { "Default",      {0,0,0,0},          {136.24f,742.46f,4046.14f,22050.f}, {0,0,0,0}, 0.f,0.f,1.f, 1,  0.f },
    { "DrumSmasher",  {17.67f,12.f,0.f,20.f},{95.93f,4046.f,6459.f,13028.f}, {1,1,0,1},-12.f,12.f,1.f,4,-18.f },
    // ... remaining 12 presets
};
```
