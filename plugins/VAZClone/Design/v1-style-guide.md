# Style Guide v1 — VAZClone

Exact visual reference extracted from `v1-test.html` (VAZ 2010 replica).

## Color Palette (hex)
| Role | Hex |
|---|---|
| Window background | `#1c1c2c` |
| Panel navy (main) | `#23284e` |
| Panel navy (alt) | `#2a2f6a` / `#232858` |
| Orange text / section headers | `#e89020` |
| Orange accent (knob/btn light) | `#f0a030` |
| Orange button (base) | `#d8810d` |
| Orange button border (dark) | `#803e00` |
| Dark field (dropdown/numeric) | `#0a0c18` |
| Slider track fill | `#161a38` |
| Slider tick lines | `#5560a0` |
| Slider thumb | `#e0e0e0`→`#808080` (metal gradient) |
| Label grey-blue | `#c8c8d8` / `#9090b0` |
| Grey button (Post/Sync/Link) | `#c8c8c8`→`#909090` |
| Octave/wave button (off) | `#454b6a`→`#2e3350` |
| Wood edge | `#2a160a`→`#6a3818`→`#8a5028` (mahogany) |
| Host titlebar grey | `#5a5a5a`→`#3a3a3a` |
| Value readout (green LED) | text `#0c0` on `#000` |

## Typography
- Family: `'Tahoma','MS Sans Serif',Arial,sans-serif` (monospace `tempo`/`value` boxes)
- Base size: **10px**; labels **9px**; section headers **11px bold italic underline**
- Wave-button glyphs: monospace 10px (◣ ⊓ ⋀ ↔)

## Control Styles
- **Section header (`.sh`)**: orange, bold, italic, underlined, 1px top padding.
- **Slider (`.slider`)**: 14px tall, navy track with 6px-spaced vertical ticks, 8px metal thumb
  (bevelled: light top/left). Center variant puts thumb at 46% (for ±-range controls).
- **Octave/Wave buttons**: flex row, beveled navy; `.on` = orange gradient, dark text, bold.
- **Dropdown (`.dd-field`)**: dark `#0a0c18`, orange text 9px, 15px tall, inset border.
- **Grey button (`.gbtn`)**: 8px, grey gradient, used for Post/Sync/Trig/Link/Multi/etc; `.on` inverts.
- **+/- mod buttons (`.pm-btn`)**: 15px wide; `+` orange, `−` grey; precede a source dropdown.
- **Envelope rows (`.env-row`)**: full-width slider with overlaid left-aligned label (Attack…Release).
- **Radio groups (`.radio-grp`)**: 9px labels, 9px radios (voice modes).

## Spacing / Layout
- Window width **600px**; main grid columns `122 122 86 130 122`px.
- Section block padding `1px 4px 3px`. Slider margin-bottom `2px`. Tight vertical rhythm (~12–16px rows).
- Wood edges **11px** wide on left/right of the 5-column grid.

## States
- Active toggle = orange gradient + dark text (octave/wave/grey `.on`).
- LFO header shows a **red dot** (`#d02020`) when the LFO is active.
- Hover on menu items = orange `#d8810d` background, black text.

## WebView notes
- All CSS embedded in `<style>` (no external deps). `<script type="module">` for the JUCE bridge.
- Control `id` = parameter ID (see v1-ui-spec.md). Production HTML mirrors this file 1:1.
