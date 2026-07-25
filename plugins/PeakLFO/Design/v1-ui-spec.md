# UI Specification v1 — PeakLFO

## Design Source
Faithful to the classic peak controller (dark "meter-box"), layout taken from the
reverse-engineered DFM (`tools/fpc/FPC_RE_NOTES.md`). Framework: **Visage (pure C++)**.

## Layout
- Window: **600 × 400 px** (fixed; resizable-safe layout via relative math).
- Structure (top → bottom):
  1. **Title bar** (h≈28): "PEAK LFO" left; `CV OUT` toggle right.
  2. **Meter / scope strip** (h≈78, full width): input peak meter (green→yellow→red),
     live LFO scope, and combined-output level bar.
  3. **Two panels** side by side (each ≈282 wide):
     - **PEAK panel** (left): 2×2 vector wheels — Base, Amount, Tension, Decay.
     - **LFO panel** (right): Shape selector (segmented, top) + 4 wheels
       (Base, Amount, Tension, Speed) + Phase wheel.
  4. **Output row** (h≈24): three source LEDs/labels — Peak+LFO · Peak · LFO.

```
┌──────────────────────────────────────────────────────────┐
│ PEAK LFO                                        [ CV OUT ] │  title
├──────────────────────────────────────────────────────────┤
│  ▁▂▃▅▇  peak meter   │   ∿ LFO scope   │  ▇ out level      │  meter strip
├───────────────────────────┬──────────────────────────────┤
│ PEAK                       │ LFO                           │
│  (Base)   (Amount)         │  [Sine|Tri|Sqr|Rnd]  shape    │
│  (Tension)(Decay)          │  (Base)(Amount)(Tension)      │
│                            │  (Speed)(Phase)               │
├───────────────────────────┴──────────────────────────────┤
│   ● Peak+LFO      ● Peak      ● LFO                        │  output row
└──────────────────────────────────────────────────────────┘
```

## Controls
| Parameter | Type | Section | Range | Default |
|-----------|------|---------|-------|---------|
| peak_base    | wheel   | PEAK | -1..1 (bipolar) | 0.0 |
| peak_amount  | wheel   | PEAK | -1..1 | 0.5 |
| peak_tension | wheel   | PEAK | -1..1 | 0.0 |
| peak_decay   | wheel   | PEAK | 0..1 | 0.52 |
| lfo_base     | wheel   | LFO  | -1..1 | 0.0 |
| lfo_amount   | wheel   | LFO  | -1..1 | 0.5 |
| lfo_tension  | wheel   | LFO  | -1..1 | 0.0 |
| lfo_speed    | wheel   | LFO  | 0..1 | 0.5 |
| lfo_shape    | segmented select | LFO | Sine/Triangle/Square/Random | Sine |
| lfo_phase    | wheel   | LFO  | 0..1 | 0.5 |
| cv_out       | toggle  | title | off/on | off |

Wheel interaction: vertical drag (Shift = fine), matches project convention
(`setMouseRelativeMode` + hidden cursor while dragging).

## Color Palette
- Background:  `#2B2F33`
- Panel:       `#23262A`
- Panel border:`#3A3F45`
- Accent green:`#7EC850` (meter/pointer)
- Accent yellow:`#E8D24A` (peak zone)
- Clip red:    `#D9534F`
- Text:        `#D8DDE2`
- Text dim:    `#7A828A`

## Style Notes
- Recessed dark panels with 1px lighter border (the reference meter-box feel).
- Vector wheels: dark body `#1C1F22`, outer ring `#4A5058`, green value arc + pointer.
- Meter strip is the visual centerpiece (like the original) — animated at ~30fps from
  the processor's latest peak/LFO/output values.
