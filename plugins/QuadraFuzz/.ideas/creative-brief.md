# QuadraFuzz — Creative Brief

## Hook
"Four bands of filth. Shape each frequency into its own beast."

## Description
QuadraFuzz is a 4-band fuzz/distortion filterbank. Audio is split into four independent frequency bands (Low, Low Mid, High Mid, High) via Linkwitz-Riley crossover filters. Each band gets its own gain and waveshaping character — from gentle saturation to full meltdown. The bands are summed and fed through a global output stage with optional multi-pass drive for cumulative saturation buildup.

The plugin faithfully recreates the original QuadraFuzz VST2 by Craig Andersen (Houpert Digital Audio) as a modern JUCE 8 VST3.

## Sonic Character
- Per-band fuzz with 4 selectable waveshaper shapes (soft/hard/asymmetric/fold)
- Independent gain per band (-24 to +24 dB, feeds into waveshaper as drive)
- Frequency-selective saturation: crush the low-end while keeping highs clean, or vice versa
- multipass: runs the entire chain N times in series for extreme, progressive saturation
- metapass: inter-pass gain trim to control buildup

## Visual Aesthetic
- Dark background, classic VST2-era aesthetic
- 6 knobs in a row: Gain | Low | Low Mid | High Mid | High | Out
- 4 shape-selector buttons (right column, square)
- EQ frequency response display showing each band's gain curve
- Checkboxes: Delete | Create | Solo (per-band enable/mute)
- Presets dropdown with 14 factory presets

## Target Use Cases
- Drum bus saturation (DrumSmasher, AnalogDrums presets)
- Guitar/bass amp-like tonal shaping
- Extreme sound design (DrumsOfDoom, fuzz presets)
- Frequency-selective parallel distortion
