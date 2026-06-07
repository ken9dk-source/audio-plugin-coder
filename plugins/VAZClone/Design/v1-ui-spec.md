# UI Specification v1 — VAZClone

Faithful replica of the VAZ 2010 editor. Source mockup: `v1-test.html` (= `VAZ2010_v4.html`).
This is a **clone**, so the aesthetic is fixed; the spec documents it and maps controls to
`parameter-spec.md` IDs for WebView↔DSP binding (wired in /impl Phase G).

## Layout
- **Window:** ~600 × 560 px (fixed). Host titlebar + patch bar on top, transport bar at bottom.
- **Main grid:** 5 columns framed by mahogany wood edges:
  `[OSC1 122] [OSC2 122] [Mixer 86] [Filter 130] [Amplifier 122]`
- Navy panel background; orange italic-underlined section headers; horizontal tick-mark sliders.

## Sections & Controls (→ parameter ID)

### Col 1 — Oscillator 1
| Control | Type | Param ID |
|---|---|---|
| Tuning 32'/16'/8'/4'/2' | button group | `o1_octave` |
| Coarse (−12..+12) | center slider | `o1_coarse` |
| Fine (−1..+1) | center slider | `o1_fine` |
| Waveform (5 icon btns) | button group | `o1_wave` |
| Wave (Sine/Tri/Saw/Sq/Pulse) | dropdown | `o1_base` |
| Modifier (Position/Pulsewidth/Detune/WT-pos) | slider | `o1_mod` |
| Frequency Modulation ×2 (src + depth) | mod rows | `o1_fm1/2` (v2) |
| Modulation (src + depth) | mod row | `o1_mod_src` (v2) |
| LFO1 Rate/Sync/Wave | row+slider | `lfo1_rate/sync/wave` |
| LFO3 Waveform/Rate | row+slider | (v2) |

### Col 2 — Oscillator 2
Same as OSC1 plus: **Waveshape Modulation** row; **OSC2 Sync** (`o2_sync`); fine→`o2_fine`,
detune→`o2_detune`. LFO2 with **Rate Modulation** + **Dest** dropdown.

### Col 3 — Mixer + Envelopes
| Control | Type | Param ID |
|---|---|---|
| Ch1/2/3 source + Post + level | dropdown+btn+slider | `o1_level`,`o2_level`,`noise_level`,`ringmod` |
| Envelope 1: Attack/Decay/Sustain/Release | 4 env sliders | `e1_attack/decay/sustain/release` |
| Env1 Multi/Reset/Cycle/Curve | buttons | `e1_curve` (+v2) |
| Envelope 2: A/D/S/R + Env Mod + Dest | sliders+row | `e2_*`,`e2_dest` |

### Col 4 — Filter
| Control | Type | Param ID |
|---|---|---|
| Mode (22 types, value 0-21) | dropdown | `filter_mode` |
| Cutoff | slider | `cutoff` |
| Resonance | slider | `resonance` |
| Unused | slider | — |
| Highpass Cutoff | slider | `hp_cutoff` |
| Cutoff Modulation ×2 | mod rows | `cutmod_src`,`cutmod_amt` |
| Resonance Modulation | mod row | (v2) |
| Mod Amplifier 2, Lag Processor | rows | (v2) |

### Col 5 — Amplifier + Performance
| Control | Type | Param ID |
|---|---|---|
| Amplitude Modulation ×2 | mod rows | (Env1 default) |
| Pan Modulation | mod row | `pan` |
| Overdrive | slider | `overdrive` |
| Mod Amplifier 1 | rows | (v2) |
| Mono/Poly/Unison/Duo + High/Low/Last | radios | `voice_mode` |
| Poly Detune | slider | `poly_detune` |
| Portamento + Auto/Exp | slider+btns | `portamento`,`porta_mode` |
| UniVce / Bend / Voices / Midi | numeric | `uni_voices`,`bend_range` |
| Arp ♩/♫/Arp/Mode | buttons | (v2) |

### Transport bar
Play / Stop / Sync / Tempo / Menu / Mixer-FX / Synth name / **Value** readout / MIDI.

## Color Palette
- Window bg: `#1c1c2c`   Panel navy: `#23284e`
- Orange text/headers: `#e89020`   Orange button: `#d8810d` → `#f0a030`
- Dark fields: `#0a0c18`   Slider track: `#161a38` + `#5560a0` ticks
- Labels: `#c8c8d8`   Wood edges: mahogany gradient (`#2a160a`→`#8a5028`)

## Style Notes
- Skeuomorphic late-'90s/2000s VA softsynth. Tahoma 9–11px, tight spacing.
- Horizontal sliders with metal thumb + tick background (not rotary knobs).
- WebView integration: each control gets `id`/`data-param` = the Param ID above; values
  shown in the transport "Value" box on hover/drag. Bridge via `window.__JUCE__` in /impl.
