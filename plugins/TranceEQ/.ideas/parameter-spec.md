# TranceEQ — Parameter Spec

## Global
| ID | Name | Type | Range / Choices | Default |
|----|------|------|-----------------|---------|
| `mode`          | Mode            | choice | Lead / Pluck / Midbass | Lead |
| `bypass`        | Bypass          | bool   | —                      | off  |
| `output`        | Output          | float  | −24..+24 dB            | 0 dB |
| `kt_on`         | Key-Track       | bool   | —                      | off  |
| `kt_src`        | Track Source    | choice | Audio / MIDI           | Audio |
| `kt_amt`        | Track Amount    | float  | 0..100 %               | 100 % |
| `kt_ref`        | Track Ref       | float  | 27.5..880 Hz (log)     | 110 Hz |

## Bands — 6 × {on, type, freq, gain, q, track}
Per band `b0..b5`:
- `bN_on`   bool
- `bN_type` choice: HPF / Low Shelf / Peak / Notch / High Shelf / LPF
- `bN_freq` float 20..20000 Hz (log skew)
- `bN_gain` float −18..+18 dB
- `bN_q`    float 0.1..18 (skew ~0.7)
- `bN_track` bool — band follows key-track when global Key-Track is on

### Fixed band roles (consistent across modes — lets Focus-Mode macros map cleanly)
- **b0** Low cut / sub filter (HPF or LPF)
- **b1** Low / body / punch (Peak or shelf)
- **b2** Presence / attack — *key-tracks by default*
- **b3** Clarity / mud control
- **b4** Air / brilliance (High Shelf)
- **b5** Spare extra band (off by default)

## Mode templates (from PDF "Anbefalede EQ-bånd" table)
### Lead (synth)
| band | on | type | freq | gain | Q | track |
|------|----|------|------|------|---|-------|
| b0 | ● | HPF        | 120 Hz  | 0     | 0.70 | — |
| b1 | ● | Low Shelf  | 300 Hz  | +3.0  | 0.70 | — |
| b2 | ● | Peak       | 2000 Hz | +3.5  | 0.90 | ● |
| b3 | ● | Peak       | 4500 Hz | +3.0  | 0.70 | ● |
| b4 | ● | High Shelf | 12000 Hz| +1.5  | 0.70 | — |
| b5 | ○ | Peak       | 800 Hz  | 0     | 1.00 | — |

### Pluck
| band | on | type | freq | gain | Q | track |
|------|----|------|------|------|---|-------|
| b0 | ● | HPF        | 175 Hz  | 0     | 0.70 | — |
| b1 | ● | Peak       | 450 Hz  | 0     | 0.70 | — |
| b2 | ● | Peak       | 6000 Hz | +3.5  | 0.80 | ● |
| b3 | ● | Peak       | 2500 Hz | +1.5  | 0.90 | ● |
| b4 | ● | High Shelf | 12000 Hz| +1.5  | 0.70 | — |
| b5 | ○ | Peak       | 900 Hz  | 0     | 1.00 | — |

### Midbass
| band | on | type | freq | gain | Q | track |
|------|----|------|------|------|---|-------|
| b0 | ● | HPF        | 30 Hz   | 0     | 0.70 | — |
| b1 | ● | Peak       | 150 Hz  | +4.0  | 0.80 | ● |
| b2 | ● | Peak       | 320 Hz  | −1.5  | 0.70 | — |
| b3 | ● | High Shelf | 3500 Hz | +1.0  | 0.70 | — |
| b4 | ○ | LPF        | 8000 Hz | 0     | 0.70 | — |
| b5 | ○ | Peak       | 250 Hz  | 0     | 1.00 | — |

> PDF also notes a sub LPF @ 80–100 Hz to keep the sub separate; available via band type **LPF**.

## Key-tracking math
`ratio = detectedF0 / kt_ref` → effective freq of tracking bands = `freq * ratio^(kt_amt)`,
clamped to [20, 20000]. Audio source = autocorrelation pitch detector on the input mix; MIDI source =
latest note-on (`f = 440·2^((note−69)/12)`). Ratio is smoothed to avoid zipper.

## Spectrum
`dsp::FFT` order 11 (2048), Hann window, magnitudes folded into 96 log-spaced bins (dB, peak-decay),
pushed to the WebView at 30 Hz and drawn under the EQ curve.
