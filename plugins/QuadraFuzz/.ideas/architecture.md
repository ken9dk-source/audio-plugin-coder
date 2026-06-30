# QuadraFuzz — DSP Architecture

## Core Components

1. **Input Gain Stage** — linear gain applied before band-split
2. **4-Band Linkwitz-Riley Crossover** — splits signal into 4 non-overlapping bands
3. **Per-Band Waveshaper** — gain + shape-selected saturation per band
4. **Band Solo/Enable Router** — mutes bands per Solo/Enable flags
5. **Band Summer** — sums all 4 band outputs
6. **Multipass Loop Controller** — re-runs steps 2–5 N times in series
7. **Output Gain Stage** — post-sum level trim
8. **Hard Clipper** — optional ±1.0 hard limiter (Clip param)
9. **Dry/Wet Mixer** — blends original input with processed signal

## Processing Chain

```
Input
  │
  ▼
[Input Gain]
  │
  ┌─── MULTIPASS LOOP (N times) ──────────────────────────────────┐
  │                                                               │
  │  [LR4 Crossover bank]                                         │
  │    ├──► Band 1 (LP  @ freq1)  ──► [Gain → Waveshaper] ──►┐   │
  │    ├──► Band 2 (BP  @ freq1..freq2) ─► [Gain → Waveshaper]►┤  │
  │    ├──► Band 3 (BP  @ freq2..freq3) ─► [Gain → Waveshaper]►┤  │
  │    └──► Band 4 (HP  @ freq3)  ──► [Gain → Waveshaper] ──►┤   │
  │                                      [Solo/Enable Router]  │   │
  │                                         [Band Summer] ◄───┘   │
  │                                              │                 │
  │                                    [Metapass gain trim]       │
  └───────────────────────────────────────────────────────────────┘
  │
  ▼
[Output Gain]
  │
  ▼
[Hard Clipper] (optional)
  │
  ▼
[Dry/Wet Mixer] ◄── (original input)
  │
  ▼
Output
```

## Crossover Design

**Filter type:** 4th-order Linkwitz-Riley (LR4) = two cascaded 2nd-order Butterworth sections  
**JUCE class:** `juce::dsp::LinkwitzRileyFilter<float>`

Band construction from 3 crossover frequencies (f1, f2, f3):
```
Band 1 (Low):      LP  at f1
Band 2 (Low Mid):  HP  at f1  →  LP  at f2
Band 3 (High Mid): HP  at f2  →  LP  at f3
Band 4 (High):     HP  at f3  (extends to Nyquist)
```

Default crossovers (from binary): **136.24 Hz / 742.46 Hz / 4,046 Hz**

## Per-Band Waveshaper

```
bandGainLinear = dBToLinear(bandN_gain)   // 0dB → 1.0, +24dB → 15.85

float waveshape(float x, float drive, int shape) {
    switch (shape) {
        case 0: return tanh(drive * x);                         // Soft clip
        case 1: return clamp(drive * x, -1.f, 1.f);            // Hard clip
        case 2: return (x >= 0) ? tanh(drive * x)              // Asymmetric
                                : clamp(drive * x, -1.f, 0.f);
        case 3: return sin(x * drive * π/2);                   // Fold/wrap
    }
}
```

Drive = linear gain. When `bandN_gain = 0 dB`, drive = 1.0 (unity, no saturation).  
Saturation onset: ~+6 dB. Heavy fuzz: +18–24 dB.

## Multipass Loop

```
for (int pass = 0; pass < multipass; ++pass) {
    runCrossoverAndWaveshape(buffer);        // full band chain
    if (pass < multipass - 1)
        applyGain(buffer, metapass_dB);      // inter-pass trim
}
```

`metapass_dB` prevents gain runaway between passes (typically −18 dB in DrumSmasher).

## Solo Logic

```
bool anySolo = band1_solo || band2_solo || band3_solo || band4_solo;
for each band:
    if (anySolo && !bandN_solo)  → output = 0
    if (!bandN_enable)           → output = 0
```

## Parameter Mapping

| Parameter     | Component           | Function                            |
|---------------|---------------------|-------------------------------------|
| `in_gain`     | Input Gain Stage    | Pre-amp before band split           |
| `bandN_freq`  | LR4 Crossover       | Sets crossover frequencies          |
| `bandN_gain`  | Per-Band Waveshaper | Drive amount (gain → saturation)    |
| `bandN_shape` | Per-Band Waveshaper | Selects waveshaping algorithm       |
| `bandN_solo`  | Solo/Enable Router  | Isolates one band                   |
| `bandN_enable`| Solo/Enable Router  | Bypasses band                       |
| `multipass`   | Loop Controller     | Number of chain iterations          |
| `metapass_db` | Loop Controller     | Inter-pass gain trim                |
| `out_gain`    | Output Gain Stage   | Post-processing level               |
| `clip`        | Hard Clipper        | Enables ±1.0 limiter                |
| `dry_wet`     | Dry/Wet Mixer       | Blends input and output             |

## Complexity Assessment

**Score: 3 / 5 (Advanced)**

Rationale:
- Multi-band processing with LR4 crossovers (non-trivial filter cascade)
- 4× waveshaper paths with parameter-driven shape selection
- Multipass feedback loop (stateful, order-dependent)
- 21 automatable parameters (4 bands × 5 + 6 global)
- Per-sample smoothing required on gain/freq changes to avoid zipper noise
- Solo/Enable routing logic across 4 bands
- Factory preset system with 14 presets

Not Level 4 because: no synthesis, no FFT, no ML — purely sample-domain processing.
