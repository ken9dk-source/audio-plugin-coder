# Style Guide v1 — SID Trance Machine (C-MOS)
**Framework:** Visage (native C++)

---

## Color Palette (ARGB hex, 0xAARRGGBB format for Visage canvas)

### Backgrounds
| Name | Visage ARGB | Description |
|---|---|---|
| `BG_VOID` | `0xFF060E1E` | Deepest background — window fill |
| `BG_PANEL` | `0xFF0A1628` | Standard panel background |
| `BG_PANEL_DARK` | `0xFF071120` | Darker panel variant (OSC scopes) |
| `BG_PANEL_HOVER` | `0xFF0D1F3C` | Panel hover/focus state |
| `BG_SECTION` | `0xFF0F1E35` | Sub-section background |

### Borders & Structure
| Name | Visage ARGB | Description |
|---|---|---|
| `BORDER_PANEL` | `0xFF1A3A6B` | Panel outer border |
| `BORDER_INNER` | `0xFF0F2A50` | Inner sub-panel border |
| `BORDER_ACTIVE` | `0xFF004488` | Border when section is focused |
| `SEPARATOR` | `0xFF152840` | Thin line between sections |

### Primary Accent (Cyan — interactive controls, labels, scope lines)
| Name | Visage ARGB | Description |
|---|---|---|
| `ACCENT_CYAN_BRIGHT` | `0xFF00D4FF` | Knob ring when active / scope line |
| `ACCENT_CYAN` | `0xFF0099CC` | Standard knob ring |
| `ACCENT_CYAN_DIM` | `0xFF005577` | Inactive ring / track fill |
| `ACCENT_CYAN_GLOW` | `0x4400D4FF` | Glow overlay (44 = ~27% alpha) |

### Resonance Accent (Green — only on RESONANCE knob ring)
| Name | Visage ARGB | Description |
|---|---|---|
| `ACCENT_GREEN` | `0xFF00FF7F` | Resonance knob ring |
| `ACCENT_GREEN_DIM` | `0xFF006633` | Resonance track |

### Controls
| Name | Visage ARGB | Description |
|---|---|---|
| `KNOB_BODY` | `0xFF1A2840` | Knob face fill |
| `KNOB_BODY_LARGE` | `0xFF101E32` | Large knob (Cutoff/Res) fill |
| `KNOB_INDICATOR` | `0xFFE8F4FD` | Knob indicator dot/line |
| `FADER_TRACK` | `0xFF0C1A30` | Fader track background |
| `FADER_THUMB` | `0xFF1E3A5A` | Fader thumb fill |
| `FADER_THUMB_ACTIVE` | `0xFF2A5080` | Fader thumb hover |

### Step Sequencer
| Name | Visage ARGB | Description |
|---|---|---|
| `STEP_ACTIVE` | `0xFF1E6FBF` | Step button ON (matches reference) |
| `STEP_INACTIVE` | `0xFF0A1E38` | Step button OFF |
| `STEP_PLAYING` | `0xFF00D4FF` | Currently playing step highlight |
| `STEP_BORDER` | `0xFF1A4080` | Step button border |

### Text
| Name | Visage ARGB | Description |
|---|---|---|
| `TEXT_PRIMARY` | `0xFFE8F4FD` | Primary labels, values |
| `TEXT_LABEL` | `0xFF8BAFD4` | Section headers, param labels |
| `TEXT_DIM` | `0xFF4A6A8A` | Placeholder, disabled text |
| `TEXT_ACCENT` | `0xFF00D4FF` | Active value text (hover state) |

### Buttons & Toggles
| Name | Visage ARGB | Description |
|---|---|---|
| `BTN_OFF_BG` | `0xFF0A1628` | Button not pressed |
| `BTN_OFF_BORDER` | `0xFF1A3A6B` | Button border unpressed |
| `BTN_ON_BG` | `0xFF003366` | Button pressed background |
| `BTN_ON_BORDER` | `0xFF0066CC` | Button pressed border |
| `BTN_ON_TEXT` | `0xFF00D4FF` | Button pressed text |
| `LED_ON` | `0xFFFFAA00` | Limiter LED on (amber) |
| `LED_OFF` | `0xFF332200` | Limiter LED off |

### Waveform / Scope
| Name | Visage ARGB | Description |
|---|---|---|
| `SCOPE_LINE` | `0xFF00D4FF` | Oscilloscope waveform line |
| `SCOPE_FILL` | `0x2200D4FF` | Waveform area fill (alpha 34%) |
| `SCOPE_GRID` | `0xFF0A1E35` | Scope grid lines |
| `SCOPE_BG` | `0xFF060E1E` | Scope background |

---

## Typography

All fonts use embedded Lato (BinaryData).

| Role | Size | Color | Usage |
|---|---|---|---|
| Plugin Name | 28px bold | `TEXT_PRIMARY` | "SID TRANCE MACHINE" (top right) |
| Codename | 18px | `TEXT_LABEL` | "C-MOS" subtitle |
| Section Header | 10px | `TEXT_LABEL` | "OSCILLATOR 1", "FILTER", etc. (ALL CAPS) |
| Param Label | 9px | `TEXT_LABEL` | "SEMI", "FINE", "PW", "CUTOFF" |
| Value Display | 11px | `TEXT_PRIMARY` | Numeric readout on hover |
| Button Text | 9px | see BTN colors | "SYNC", "RETRIG", "ON/OFF" |
| Preset Name | 14px | `TEXT_PRIMARY` | Preset name in bar |

---

## Spacing & Sizing Rules

| Element | Rule |
|---|---|
| Panel corner radius | 4 px |
| Section header height | 18 px |
| Standard knob size | 48 × 48 px |
| Large knob size (Cutoff/Res) | 76 × 76 px |
| Fader thumb width | 24 px |
| Fader track width | 6 px |
| Between-knob gap | 8 px |
| Between-panel gap | 4 px |
| Step button size | 36 × 36 px |
| Step button corner radius | 3 px |
| Toggle button height | 20 px |
| Dropdown height | 20 px |

---

## Knob Design Rules (Visage draw())

1. Draw `KNOB_BODY` filled circle
2. Draw arc ring from 135° to 135°+270°×value in `ACCENT_CYAN` (3px wide)
3. Draw track arc (full 270°) in `ACCENT_CYAN_DIM` (2px, behind ring)
4. Draw indicator line from center to edge at current angle in `KNOB_INDICATOR`
5. On hover: add `ACCENT_CYAN_GLOW` filled circle behind ring (soft glow)

**Large knob (Cutoff):** Same but ring width = 5px, glow stronger
**Resonance knob:** Ring color = `ACCENT_GREEN` instead of cyan

---

## Fader Design Rules

1. Draw track: `FADER_TRACK` rounded rect, center of control, 6px wide
2. Draw fill: `ACCENT_CYAN_DIM` from bottom to thumb position
3. Draw thumb: `FADER_THUMB` rounded rect, 24×10px, at value position
4. On hover: thumb = `FADER_THUMB_ACTIVE`

---

## Panel Structure Rules

Every panel:
1. Background: `BG_PANEL` filled rounded rect (radius 4)
2. Border: `BORDER_PANEL` stroked rounded rect (1px)
3. Header: `TEXT_LABEL` text, ALL CAPS, 10px, left-aligned at y=5
4. Thin separator line under header: `SEPARATOR`, y=18, full width

---

## Waveform Selector Icons

Six buttons, icon-only, 30×24 each:
- **SAW:** Right-facing sawtooth wave drawn as a polyline
- **TRI:** Triangle wave V shape
- **PULSE:** Rectangular pulse shape
- **NOISE:** Random jagged line segments
- **SAW+TRI:** Combination wave sketch
- **RING:** Two overlapping sine arches

Active waveform button: filled `BTN_ON_BG` + `BTN_ON_BORDER` + icon in `ACCENT_CYAN`
Inactive: `BTN_OFF_BG` + `BTN_OFF_BORDER` + icon in `TEXT_DIM`

---

## Oscilloscope Display Rules

The top three OSC scope windows show a real-time waveform preview:
- Background: `SCOPE_BG`
- Horizontal center line: `SCOPE_GRID` at 1px
- Waveform: `SCOPE_LINE` polyline, 1px stroke
- Fill below waveform: `SCOPE_FILL` (alpha ~34%)
- Label top-left: "OSC 1: SAW" in `TEXT_LABEL` 9px
- Updated from audio thread via `juce::AbstractFifo` lock-free FIFO

---

## Section Color Coding
| Section | Accent / Highlight Color |
|---|---|
| OSC 1 | `ACCENT_CYAN` (standard) |
| OSC 2 | `ACCENT_CYAN` (standard) |
| OSC 3 | `ACCENT_CYAN` (standard) |
| Filter Cutoff | `ACCENT_CYAN_BRIGHT` (slightly brighter) |
| Filter Resonance | `ACCENT_GREEN` |
| LFO indicators | `ACCENT_CYAN` |
| Step SEQ active | `STEP_ACTIVE` (medium blue) |
| Mod amount sliders | `ACCENT_CYAN` when positive, `0xFFFF6644` when negative |
