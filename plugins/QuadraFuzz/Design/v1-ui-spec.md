# QuadraFuzz UI Specification v1

## Layout
- Window: 620×380px
- Background: dark charcoal #1A1A1A
- Title bar strip (top): plugin name + "Filterbank" label
- Main area: 3 columns

## Sections
1. **Left strip (80px):** Global Gain knob + In label
2. **Center (360px):**
   - Top row: 4 band knobs (Low, Low Mid, High Mid, High) with dB readout below each
   - Middle: EQ frequency response canvas (fills width, 120px tall)
   - Bottom row: Delete | Create | Solo checkboxes + Presets button
3. **Right strip (180px):**
   - Output knob + Out label
   - 4 Shape buttons (stacked vertically, labeled ■ with tooltip)

## Controls

| Parameter        | Type         | Position                  | Default  |
|------------------|--------------|---------------------------|----------|
| `in_gain`        | Rotary knob  | Left strip, center        | 0.0 dB   |
| `band1_gain`     | Rotary knob  | Center top, slot 1        | 0.0 dB   |
| `band2_gain`     | Rotary knob  | Center top, slot 2        | 0.0 dB   |
| `band3_gain`     | Rotary knob  | Center top, slot 3        | 0.0 dB   |
| `band4_gain`     | Rotary knob  | Center top, slot 4        | 0.0 dB   |
| `out_gain`       | Rotary knob  | Right strip, top          | 0.0 dB   |
| `band1_shape`..4 | Button group | Right strip, 4 buttons    | 0        |
| `bandN_solo`     | Checkbox     | Bottom row                | false    |
| `bandN_enable`   | Checkbox     | Bottom row (Create/Delete)| true     |
| `preset_nr`      | Dropdown     | Bottom row right          | Default  |
| EQ canvas        | Display only | Center middle             | —        |

## Color Palette
- Background:      #1A1A1A
- Panel surface:   #242424
- Knob body:       #2E2E2E
- Knob indicator:  #D4A84B  (warm amber)
- Accent / active: #C8702A  (burnt orange — fuzz heat)
- Text primary:    #E0E0E0
- Text secondary:  #888888
- Grid lines:      #333333
- Band 1 curve:    #E05050  (red — Low)
- Band 2 curve:    #50C878  (green — Low Mid)
- Band 3 curve:    #4FA3E0  (blue — High Mid)
- Band 4 curve:    #D4A84B  (amber — High)
- Shape btn off:   #333333
- Shape btn on:    #C8702A

## Typography
- Plugin name: "QuadraFuzz" — 16px, bold, #E0E0E0
- Subtitle: "Filterbank" — 10px, #888888, letter-spacing 2px
- Knob label: 9px, #888888, uppercase
- Value readout: 10px mono, #D4A84B
- Author: "by Craig Andersen" — 9px, italic, #555555

## Style Notes
- Inspired by classic VST2 hardware-emulation aesthetic
- Flat, no gradients (except subtle knob highlight)
- EQ canvas draws 4 colored curves: one per band, showing gain response vs frequency
- Shape buttons are square (28×28px), 4 stacked vertically
- Knobs: arc-style, dark body, amber indicator
- All knobs double-click to reset to default
