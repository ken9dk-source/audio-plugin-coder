# VAZClone — GUI differences vs original VAZ 2010

Reference: the VAZ 2010 synth panel (skin bitmaps in `Vaz2010Skin.dll`) vs the clone's hand-coded
WebView replica (`Source/ui/public/index.html`). The clone is a CSS/HTML re-creation, not a bitmap
rip, so it is *close* but not pixel-identical.

Legend: ✅ matches · 🟡 close · ❌ differs

## Layout & sections
| Element | Original | Clone | |
|---------|----------|-------|---|
| Column order | Osc1 · Osc2 · Mixer · Filter · Amplifier | same | ✅ |
| Lower band | LFO1/2/3 · Env1/2 · ModAmp1/2 · Lag · Performance | same | ✅ |
| Wood side borders | mahogany strips L/R | CSS gradient strips L/R | 🟡 |
| Top window bar | dark bar "Synth 1 New Patch" + icons | plain dark bar (text/icons removed per user) | 🟡 |
| Window surround | none (panel fills) | dark (#15161f), no grey, no scroll | ✅ (fixed) |
| Window size | ~768×533 (skin) | 604×460 (fits content) | 🟡 |

## Colours
| | Original | Clone | |
|--|----------|-------|---|
| Panel navy | ~#23284e | `#23284e` | ✅ |
| Section text | orange, italic, underline | `#e89020` italic underline | ✅ |
| Orange buttons | `#d8810d` | `#d8810d` | ✅ |
| Dark value fields | near-black | `#0a0c14` | ✅ |
| +/- buttons | orange + / grey − | now toggle: active bright, inactive dim | ✅ (fixed) |

## Fonts
| | Original | Clone | |
|--|----------|-------|---|
| Family | embedded bitmap font (MS Sans-ish) | Tahoma / MS Sans Serif | 🟡 |
| Sizes | ~10–11 px | 10 px body, 11 px headers | 🟡 |
| Anti-aliasing | bitmap (crisp) | browser-rendered | 🟡 |

## Controls
| | Original | Clone | |
|--|----------|-------|---|
| Sliders | bitmap horizontal faders w/ groove | CSS groove + 6px thumb | 🟡 (functionally identical) |
| Octave/waveform buttons | bitmap toggle buttons | CSS buttons, correct on-state | ✅ |
| Mode/source dropdowns | native popups | HTML `<select>` | 🟡 |
| Performance lever | 3-pos flip lever graphic | HTML radio buttons (functional) | ❌ visual / ✅ functional |
| Filter "Bandwidth/Sep" label | dynamic per mode | static "Bandwidth / Sep" | 🟡 |

## Remaining visual gaps (low priority)
- Bitmap font + exact glyph rendering (cosmetic).
- The Mono/Poly/Unison **flip-lever** graphic (radios are functionally equivalent).
- Slider thumb / groove styling not pixel-matched to the skin bitmaps.
- Dynamic relabel of the filter slot-3 + Resonance-Mod labels per mode.
- Window is sized to the clone's content (604×460) rather than the skin's native size.

## VERDICT
Layout, section structure, colour palette and all control *positions/functions* match the original
closely. Differences are **cosmetic** (bitmap font, exact pixel metrics, the flip-lever graphic).
A pixel-perfect match would require ripping the `Vaz2010Skin.dll` bitmaps and rebuilding the panel
from images instead of CSS.
