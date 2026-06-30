# QuadraFuzz — Parameter Spec

Source: reverse-engineered from QuadraFuzz.dll (binary float extraction + string analysis)

## Per-Band Parameters (×4: band1, band2, band3, band4)

| ID              | Name         | Type  | Range           | Default      | Unit | Notes                              |
|:----------------|:-------------|:------|:----------------|:-------------|:-----|:-----------------------------------|
| `bandN_gain`    | Band N Gain  | Float | -24.0 .. +24.0  | 0.0          | dB   | Drives waveshaper; 0 dB = no fuzz  |
| `bandN_freq`    | Band N Freq  | Float | 20 .. 20000     | see below    | Hz   | Crossover / shelf frequency        |
| `bandN_shape`   | Band N Shape | Int   | 0 .. 3          | 0            | —    | 0=soft 1=hard 2=asym 3=fold        |
| `bandN_solo`    | Band N Solo  | Bool  | false/true      | false        | —    | Mute all other bands               |
| `bandN_enable`  | Band N On    | Bool  | false/true      | true         | —    | Enable/bypass this band            |

### Default crossover frequencies (confirmed from Default preset binary data)
| Band | Name      | Default Freq |
|------|-----------|-------------|
| 1    | Low       | 136.24 Hz   |
| 2    | Low Mid   | 742.46 Hz   |
| 3    | High Mid  | 4,046.14 Hz |
| 4    | High      | 22,050 Hz   |

Band 4 always acts as a high shelf (extends to Nyquist).

## Global Parameters

| ID             | Name       | Type  | Range         | Default | Unit | Notes                              |
|:---------------|:-----------|:------|:--------------|:--------|:-----|:-----------------------------------|
| `in_gain`      | Input Gain | Float | -24.0 .. +24.0| 0.0     | dB   | Pre-processing gain                |
| `out_gain`     | Output     | Float | -24.0 .. +24.0| 0.0     | dB   | Post-processing output level       |
| `dry_wet`      | Mix        | Float | 0.0 .. 1.0    | 1.0     | —    | 0=dry, 1=wet                       |
| `clip`         | Clip       | Bool  | false/true    | false   | —    | Hard output limiter at ±1.0        |
| `multipass`    | Multipass  | Int   | 1 .. 8        | 1       | —    | Passes through band chain (serial) |
| `metapass_db`  | Metapass   | Float | -24.0 .. 0.0  | 0.0     | dB   | Inter-pass gain trim               |
| `preset_nr`    | Preset     | Int   | 0 .. 13       | 0       | —    | Factory preset index               |

## Shape Waveshaper Definitions

| Value | Name         | Formula                                              |
|-------|--------------|------------------------------------------------------|
| 0     | Soft clip    | `tanh(drive * x)`                                    |
| 1     | Hard clip    | `clamp(drive * x, -1, 1)`                            |
| 2     | Asymmetric   | positive: `tanh(drive*x)`, negative: `clamp(drive*x,-1,0)` |
| 3     | Fold/wrap    | `sin(x * drive * π/2)`                               |

## Display Formats (from binary strings)

```
Frequency ≥ 1000 Hz  →  "%2.1f kHz"   e.g. "4.0 kHz"
Frequency <  1000 Hz  →  "%3.0f Hz"    e.g. "136 Hz"
Gain                  →  "%4.1f dB"    e.g. " 0.0 dB"
```

## Factory Presets (decoded from binary)

| # | Name         | Band Gains (dB)         | Crossovers (Hz)                    |
|---|--------------|-------------------------|------------------------------------|
| 0 | Default      | 0, 0, 0, 0              | 136, 742, 4046, 22050              |
| 1 | DrumSmasher  | +17.7, +12, 0, +20      | 96, 4046, 6459, 13028              |
| 2 | AnalogDrums  | (medium saturation)     | (shifted lows)                     |
| 3 | DrumsOfDoom  | (heavy low fuzz)        |                                    |
| 4 | DrumSqueeze  | (tight drum compression)|                                    |
| 5 | Tighten      | (mid-forward)           |                                    |
| 6 | GrungeKord   | (chord-friendly grunge) |                                    |
| 7 | Smasher      | (all-out fuzz wall)     |                                    |
| 8 | dRez         | (digital resonance)     |                                    |
| 9 | fuzz         | (classic fuzz)          |                                    |
|10 | BigNoiseKord |                         |                                    |
|11 | Acoustifuzz  |                         |                                    |
|12 | ChordRez     |                         |                                    |
|13 | PowerChord   |                         |                                    |
