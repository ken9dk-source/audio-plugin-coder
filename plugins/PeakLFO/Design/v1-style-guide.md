# Style Guide v1 — PeakLFO

## Theme
Classic "meter-box": dark, technical, functional. Green/yellow signal accent.

## Color Palette (0xAARRGGBB for Visage `canvas.setColor`)
| Role | Hex | Visage |
|------|-----|--------|
| Background     | `#2B2F33` | `0xff2b2f33` |
| Panel fill     | `#23262A` | `0xff23262a` |
| Panel border   | `#3A3F45` | `0xff3a3f45` |
| Wheel body     | `#1C1F22` | `0xff1c1f22` |
| Wheel ring     | `#4A5058` | `0xff4a5058` |
| Accent green   | `#7EC850` | `0xff7ec850` |
| Accent yellow  | `#E8D24A` | `0xffe8d24a` |
| Clip red       | `#D9534F` | `0xffd9534f` |
| Text           | `#D8DDE2` | `0xffd8dde2` |
| Text dim       | `#7A828A` | `0xff7a828a` |

## Typography
- Title: bold, ~18px, letter-spaced ("PEAK LFO").
- Panel headers ("PEAK", "LFO"): bold small-caps, ~11px, dim.
- Wheel labels: ~12px, text color; value read-out ~11px, dim.
- Fonts loaded via project pattern (`updateFonts()` / `fonts_ready_`), same as gnarly3.

## Wheels (vector, matches gnarly3 draw idiom)
- Body: filled `circle`, ring `ring(…, 2px)`.
- Value arc: 270° sweep from ~7:30, green; pointer `segment` from center, len ≈ 0.7·r.
- Bipolar wheels (base/amount/tension): center detent at 12:00 (value 0.5 = 0 effect);
  arc drawn from center outward both directions.
- Diameter ≈ 56px in panels; spacing ≈ 20px.

## Meter strip
- Peak meter: horizontal segmented bar, green up to ~0.8, yellow 0.8–0.95, red above.
- LFO scope: one cycle of current shape, green line (`segment` polyline).
- Output bar: combined Peak+LFO level, green.
- Refresh ~30fps from processor snapshot (atomic floats).

## Spacing / layout rules
- Outer padding: 12px. Panel gap: 10px. Panel corner: square (the reference style), 1px border.
- Title bar 28px; meter strip 78px; output row 24px; panels fill the rest.

## States
- Wheel hover: ring brightens to `#5A616A`.
- Wheel dragging: pointer green→brighter; cursor hidden (relative mode).
- CV OUT active: toggle fills green, label dark.
- Output LEDs pulse with each source's current level.
