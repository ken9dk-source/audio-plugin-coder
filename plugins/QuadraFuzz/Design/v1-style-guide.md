# QuadraFuzz Style Guide v1

## Color Palette
| Role            | Hex       | Usage                              |
|-----------------|-----------|------------------------------------|
| Background      | #1A1A1A   | Plugin body                        |
| Panel surface   | #242424   | EQ display area, button panels     |
| Knob body       | #2E2E2E   | Rotary control background          |
| Knob indicator  | #D4A84B   | Pointer line, value text           |
| Accent / hot    | #C8702A   | Active shape btn, focus ring       |
| Text primary    | #E0E0E0   | Labels, knob names                 |
| Text secondary  | #888888   | Subtitles, secondary labels        |
| Grid lines      | #333333   | EQ canvas grid                     |
| Band 1 (Low)    | #E05050   | Red EQ curve                       |
| Band 2 (LowMid) | #50C878   | Green EQ curve                     |
| Band 3 (HiMid)  | #4FA3E0   | Blue EQ curve                      |
| Band 4 (High)   | #D4A84B   | Amber EQ curve                     |

## Typography
- **Font stack:** `'Consolas', 'Courier New', monospace`
- Plugin name: 15px bold, #E0E0E0
- "Filterbank" subtitle: 9px, #666666, letter-spacing: 3px, uppercase
- Knob label: 9px, #999999, uppercase, center-aligned below knob
- Value display: 10px mono, #D4A84B
- "by Craig Andersen": 9px italic, #444444

## Knob Design
- Diameter: 48px (band knobs), 52px (Gain/Out knobs)
- Body: radial gradient #383838 → #2A2A2A
- Track arc: #1A1A1A, 3px stroke, 220° sweep
- Value arc: #C8702A, 3px stroke (filled to current value)
- Indicator: white dot at tip, 2px
- Glow on hover: 0 0 8px rgba(200,112,42,0.4)

## Shape Buttons
- Size: 28×28px, 4px border-radius
- Off: background #333, border 1px #444
- On: background #C8702A, border 1px #D4A84B
- Icons (SVG paths for 4 waveshaper shapes):
  - 0 Soft: smooth S-curve
  - 1 Hard: stepped line
  - 2 Asym: asymmetric clip
  - 3 Fold: folded wave

## EQ Canvas
- Background: #1E1E1E
- Border: 1px solid #333
- Grid: 5 horizontal + 5 vertical lines at #2A2A2A
- Frequency axis: 20Hz–20kHz (log scale)
- Gain axis: ±24dB
- 0dB line: #444, 1px dashed
- Each band: semi-transparent fill (25% alpha) + 2px solid line
- Crossover handles: small draggable circles on freq axis

## Checkbox Style (Delete / Create / Solo)
- 14×14px custom checkbox
- Unchecked: #333 border, transparent fill
- Checked: #C8702A fill, white checkmark
- Label: 9px #999999

## Preset Dropdown
- Width: 120px
- Background: #2A2A2A
- Border: 1px #444
- Text: 11px #D4A84B
- Arrow: ▾ in #888

## Spacing
- Outer padding: 12px
- Between knobs: 20px horizontal
- Knob to label: 4px
- Section separator: 1px #333
