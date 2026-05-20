# UI Specification v1 — SID Trance Machine (C-MOS)
**Framework:** Visage (native C++)
**Reference:** reference-design.png (UI mockup provided by user)

---

## Window

| Property | Value |
|---|---|
| Width | 1200 px |
| Height | 740 px |
| Resizable | No (fixed size v1) |
| Border padding | 8 px |
| Section gap | 4 px |

---

## Layout Map (pixel coordinates, origin = top-left)

```
┌──────────────────────────────────────────────────────────────────────────────────────────┐ y=0
│  HEADER / OSCILLOSCOPE BAR                                                               │
│  [Logo 80×110] │ OSC1 waveform 182×110 │ OSC2 182×110 │ OSC3 182×110 │ Filter 282×110  │ y=8..118
│                │                       │              │              │                  │
│                                                                       SID TRANCE MACHINE │
│                                                                       C-MOS logo (right) │
├──────────────────────────────────────────────────────────────────────────────────────────┤ y=122
│  OSC 1 (200×308) │ OSC 2 (200×308) │ OSC 3 (200×308) │ FILTER+AMP (376×308) │ MASTER   │ y=126..434
│                  │                 │                  │                      │ (116×308) │
├──────────────────────────────────────────────────────────────────────────────────────────┤ y=438
│  MOD MATRIX (296×226) │ EFFECTS (196×226) │ LFO1 (160×108)                             │ y=442..668
│                       │                   │ LFO2 (160×108)                             │
│                       │                   ├──────────────────────────────────────────── │ y=554
│                       │                   │ ARP / SEQUENCER (536×114)                  │
├──────────────────────────────────────────────────────────────────────────────────────────┤ y=672
│  PRESET BAR (full width, h=60)                                                           │ y=676..736
└──────────────────────────────────────────────────────────────────────────────────────────┘ y=740
```

---

## Section Coordinates (x, y, w, h)

### Header Bar
| Sub-section | x | y | w | h |
|---|---|---|---|---|
| Commodore Logo | 8 | 8 | 80 | 110 |
| OSC1 Scope | 96 | 8 | 182 | 110 |
| OSC2 Scope | 282 | 8 | 182 | 110 |
| OSC3 Scope | 468 | 8 | 182 | 110 |
| Filter Scope | 654 | 8 | 282 | 110 |
| SID Logo (right) | 940 | 8 | 252 | 110 |

### Middle Section
| Section | x | y | w | h |
|---|---|---|---|---|
| OSCILLATOR 1 | 8 | 126 | 200 | 308 |
| OSCILLATOR 2 | 212 | 126 | 200 | 308 |
| OSCILLATOR 3 | 416 | 126 | 200 | 308 |
| FILTER | 620 | 126 | 220 | 160 |
| AMPLIFIER | 620 | 290 | 220 | 144 |
| MASTER | 844 | 126 | 148 | 308 |
| (spacer right) | 996 | 126 | 196 | 308 |

### Bottom Section
| Section | x | y | w | h |
|---|---|---|---|---|
| MOD MATRIX | 8 | 438 | 296 | 226 |
| EFFECTS | 308 | 438 | 196 | 226 |
| LFO 1 | 508 | 438 | 160 | 108 |
| LFO 2 | 672 | 438 | 160 | 108 |
| ARP / SEQUENCER | 508 | 550 | 684 | 114 |

### Preset Bar
| Section | x | y | w | h |
|---|---|---|---|---|
| PRESET BAR | 8 | 672 | 1184 | 60 |

---

## Controls Per Section

### OSCILLATOR PANEL (×3, identical layout)
Each panel internal layout (local coords, panel = 200×308):

| Control | Type | x | y | w | h | Notes |
|---|---|---|---|---|---|---|
| Waveform selector | Icon buttons (6) | 4 | 28 | 192 | 26 | SAW/TRI/PULSE/NOISE/SAW+TRI/RING |
| SEMI knob | SIDKnob | 8 | 62 | 52 | 52 | -24..+24 |
| FINE knob | SIDKnob | 74 | 62 | 52 | 52 | -100..+100 |
| PW knob | SIDKnob | 140 | 62 | 52 | 52 | 0..100% |
| A fader | SIDFader | 10 | 126 | 28 | 80 | |
| D fader | SIDFader | 52 | 126 | 28 | 80 | |
| S fader | SIDFader | 94 | 126 | 28 | 80 | |
| R fader | SIDFader | 136 | 126 | 28 | 80 | |
| Envelope display | Mini canvas | 4 | 214 | 112 | 50 | ADSR shape preview |
| VOLUME knob | SIDKnob | 140 | 214 | 52 | 52 | |
| Section label | Text | 4 | 4 | 192 | 16 | "OSCILLATOR 1/2/3", caps, centered |

### FILTER PANEL (220×160)
| Control | Type | x | y | w | h |
|---|---|---|---|---|---|
| CUTOFF knob | SIDKnobLarge | 20 | 30 | 80 | 80 |
| RESONANCE knob | SIDKnobLarge | 120 | 30 | 80 | 80 |
| Filter type dropdown | SIDDropdown | 4 | 122 | 100 | 22 |
| Slope dropdown | SIDDropdown | 108 | 122 | 108 | 22 |
| KEY TRACK button | SIDToggleBtn | 4 | 148 | 76 | 22 |
| VELOCITY button | SIDToggleBtn | 84 | 148 | 76 | 22 |

### AMPLIFIER PANEL (220×144)
| Control | Type | x | y | w | h |
|---|---|---|---|---|---|
| ATTACK knob | SIDKnob | 4 | 28 | 40 | 40 |
| DECAY knob | SIDKnob | 50 | 28 | 40 | 40 |
| SUSTAIN knob | SIDKnob | 96 | 28 | 40 | 40 |
| RELEASE knob | SIDKnob | 142 | 28 | 40 | 40 |
| VOLUME knob | SIDKnob | 172 | 28 | 40 | 40 |

### MASTER PANEL (148×308)
| Control | Type | x | y | w | h |
|---|---|---|---|---|---|
| OUTPUT label | Text | 4 | 4 | 140 | 14 | centered |
| Volume fader | SIDFaderV | 60 | 20 | 28 | 180 | vertical |
| dB scale | Text marks | 4 | 20 | 52 | 180 | +6,0,-6,-12,-18,-24,-inf |
| LIMITER label | Text | 4 | 208 | 80 | 14 | |
| Limiter LED | SIDLed | 88 | 210 | 10 | 10 | |
| Limiter toggle | SIDDropdown | 100 | 206 | 44 | 18 | ON/OFF |
| MONO/POLY label | Text | 4 | 232 | 140 | 14 | |
| Poly dropdown | SIDDropdown | 4 | 248 | 140 | 20 | MONO/POLY |
| VOICE COUNT label | Text | 4 | 276 | 140 | 14 | |
| Voice dropdown | SIDDropdown | 4 | 292 | 140 | 20 | 1..16 |

### MOD MATRIX PANEL (296×226)
| Control | Type | x | y | w | h | Notes |
|---|---|---|---|---|---|---|
| Tab: MOD MATRIX | SIDTab | 4 | 4 | 100 | 20 | |
| Tab: ROUTING | SIDTab | 108 | 4 | 80 | 20 | |
| Row 1 Source | SIDDropdown | 4 | 34 | 72 | 20 | LFO1/LFO2/Vel/MW |
| Row 1 Amount | SIDAmountSlider | 80 | 34 | 80 | 20 | -100..+100% |
| Row 1 Dest | SIDDropdown | 164 | 34 | 128 | 20 | |
| (rows 2-4 same, y+=30) | | | | | | |

### EFFECTS PANEL (196×226)
| Control | Type | x | y | w | h |
|---|---|---|---|---|---|
| CHORUS enable | SIDToggleBtn | 4 | 28 | 76 | 20 | |
| Chorus RATE | SIDKnob | 84 | 20 | 34 | 34 | |
| Chorus DEPTH | SIDKnob | 120 | 20 | 34 | 34 | |
| Chorus MIX | SIDKnob | 156 | 20 | 34 | 34 | |
| DELAY enable | SIDToggleBtn | 4 | 88 | 76 | 20 | |
| Delay TIME | SIDDropdown | 84 | 88 | 40 | 20 | 1/16..1/1 |
| Delay FEEDBACK | SIDKnob | 126 | 80 | 34 | 34 | |
| Delay MIX | SIDKnob | 160 | 80 | 34 | 34 | |
| REVERB enable | SIDToggleBtn | 4 | 148 | 76 | 20 | |
| Reverb SIZE | SIDKnob | 84 | 140 | 34 | 34 | |
| Reverb DAMPING | SIDKnob | 118 | 140 | 34 | 34 | |
| Reverb MIX | SIDKnob | 152 | 140 | 34 | 34 | |

### LFO PANEL (160×108, ×2)
| Control | Type | x | y | w | h |
|---|---|---|---|---|---|
| Shape dropdown | SIDDropdown | 4 | 28 | 72 | 20 | |
| RATE knob | SIDKnob | 80 | 20 | 44 | 44 | |
| SYNC button | SIDToggleBtn | 130 | 20 | 26 | 20 | |
| RETRIG button | SIDToggleBtn | 130 | 44 | 26 | 20 | |

### ARP / SEQUENCER PANEL (684×114)
| Control | Type | x | y | w | h |
|---|---|---|---|---|---|
| MODE dropdown | SIDDropdown | 60 | 28 | 72 | 20 | Up/Down/UpDwn/Rand |
| OCTAVE dropdown | SIDDropdown | 170 | 28 | 56 | 20 | 1..4 Oct |
| GATE label+val | SIDDropdown | 256 | 28 | 52 | 20 | 0..100% |
| SWING label+val | SIDDropdown | 336 | 28 | 52 | 20 | 0..50% |
| TEMPO label+val | SIDDropdown | 412 | 28 | 72 | 20 | 60..200 |
| SYNC button | SIDToggleBtn | 492 | 28 | 44 | 20 | |
| Step buttons 1-16 | SIDStepBtn×16 | 4+i*42 | 58 | 38 | 38 | step spacing 42px |

### PRESET BAR (full width, 60px)
| Control | Type | x | y | w | h |
|---|---|---|---|---|---|
| PRESET label | Text | 4 | 16 | 60 | 20 | |
| Preset name | SIDTextDisplay | 68 | 12 | 220 | 28 | |
| Prev/Next arrows | SIDArrowBtn | 294 | 16 | 24 | 20 | |
| BANK label | Text | 330 | 16 | 40 | 20 | |
| Bank dropdown | SIDDropdown | 374 | 12 | 120 | 28 | USER/FACTORY |
| SAVE button | SIDButton | 820 | 12 | 80 | 28 | |
| SAVE AS button | SIDButton | 908 | 12 | 88 | 28 | |
| INIT button | SIDButton | 1004 | 12 | 60 | 28 | |

---

## Visual Hierarchy
1. **Interactive knobs/faders** — brightest, cyan accent ring when hovered
2. **Section labels** — muted blue, ALL CAPS, small
3. **Panel backgrounds** — layered dark navy with subtle border glow
4. **Active elements (buttons ON)** — filled cyan rectangle
5. **Inactive elements** — dark, low contrast
6. **Oscilloscope waveforms** — bright cyan lines on very dark bg
