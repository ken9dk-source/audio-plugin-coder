# Parameter Spec — VAZClone (v1 Core)

Parameters map to the original `.v2p` byte positions where known (enables preset import).
Offset bases: **PS** = PRST+12, **sec2** = MSmp1+8+548, **sec3** = MSmp2+8+548.

## Oscillator 1
| ID | Name | Type | Range | Default | .v2p byte |
| :-- | :-- | :-- | :-- | :-- | :-- |
| `o1_octave` | OSC1 Octave | Enum | 32'/16'/8'/4'/2' | 8' | PS+127/128 (cents LE16) |
| `o1_coarse` | OSC1 Coarse | Int | -12..+12 semi | 0 | — |
| `o1_fine` | OSC1 Fine | Float | -100..+100 cents | 0 | — |
| `o1_wave` | OSC1 Waveform | Enum | Sine/Pulse/Detune/Wavetable/Ext | Sine | PS+131 (0-4) |
| `o1_base` | OSC1 Base Wave | Enum | Sine/Tri/Saw/Square/Pulse | Sine | — |
| `o1_mod` | OSC1 Modifier | Float | 0..1 | 0 | PS+135 (0-255) |
| `o1_level` | OSC1 Level | Float | 0..1 | 1.0 | sec3 (mix1) |

## Oscillator 2 (same as OSC1 +)
| ID | Name | Type | Range | Default | .v2p byte |
| :-- | :-- | :-- | :-- | :-- | :-- |
| `o2_fine` | OSC2 Fine | Float | -100..+100 cents | 0 | sec2+1 (160=ctr) |
| `o2_detune` | OSC2 Detune | Float | 0..1 | 0 | PS+135 region |
| `o2_level` | OSC2 Level | Float | 0..1 | 0 | sec3+14 |
| `o2_sync` | OSC2 Hard Sync | Bool | on/off | off | — |

## Noise / Mixer
| ID | Name | Type | Range | Default | .v2p byte |
| :-- | :-- | :-- | :-- | :-- | :-- |
| `noise_level` | Noise Level | Float | 0..1 | 0 | sec3+23 |
| `ringmod` | Ring Mod amount | Float | 0..1 | 0 | — |

## Filter
| ID | Name | Type | Range | Default | .v2p byte |
| :-- | :-- | :-- | :-- | :-- | :-- |
| `filter_mode` | Filter Mode | Enum | 22 types (see spec §4) | R 4P LP Res (19) | sec3+28 (0-21) |
| `cutoff` | Cutoff | Float | 0..1 | 0.75 | sec3+33 |
| `resonance` | Resonance | Float | 0..1 | 0.0 | sec3+37 |
| `hp_cutoff` | Highpass Cutoff | Float | 0..1 | 0.0 | — |
| `cutmod_src` | Cutoff Mod Source | Enum | None/LFO/Env (+) | None | sec3+49 (0/1/5) |
| `cutmod_amt` | Cutoff Mod Amount | Float | 0..1 | 0 | sec3+53 |

## Amplifier
| ID | Name | Type | Range | Default | .v2p byte |
| :-- | :-- | :-- | :-- | :-- | :-- |
| `overdrive` | Overdrive | Float | 0..1 | 0 | sec3+105 |
| `pan` | Pan | Float | -1..+1 | 0 | — |

## Envelope 1 (Amp) — and Envelope 2 (Mod, same layout +Dest)
| ID | Name | Type | Range | Default | .v2p byte |
| :-- | :-- | :-- | :-- | :-- | :-- |
| `e1_attack` | Env1 Attack | Float | 0..1 | 0.0 | PS+54/55 (LE16) |
| `e1_decay` | Env1 Decay | Float | 0..1 | 0.3 | PS+58/59 (LE16) |
| `e1_sustain` | Env1 Sustain | Float | 0..1 | 1.0 | PS+62 |
| `e1_release` | Env1 Release | Float | 0..1 | 0.2 | PS+66/67 (LE16) |
| `e1_curve` | Env1 Curve | Bool | lin/exp | exp | — |
| `e2_*` | Env2 ADSR | Float | 0..1 | — | PS+74/78/82/86 |
| `e2_dest` | Env2 Dest | Enum | Atk/Dec/Sus/Rel/None | None | — |

## LFO 1 (v1: one LFO)
| ID | Name | Type | Range | Default | .v2p byte |
| :-- | :-- | :-- | :-- | :-- | :-- |
| `lfo1_rate` | LFO1 Rate | Float | 0..1 (or sync) | 0.4 | PS+10 |
| `lfo1_sync` | LFO1 Tempo Sync | Bool | on/off | on | — |
| `lfo1_wave` | LFO1 Wave | Enum | 8 waves (spec §7) | Saw/Tri | — |

## Performance / Global
| ID | Name | Type | Range | Default | .v2p byte |
| :-- | :-- | :-- | :-- | :-- | :-- |
| `voice_mode` | Voice Mode | Enum | Mono/Poly/Unison/Duo | Poly | sec3+117 (0-2) |
| `uni_voices` | Unison Voices | Int | 1..32 | 8 | — |
| `uni_detune` | Unison Detune | Float | 0..1 | 0.3 | sec3+134 |
| `poly_detune` | Poly Detune | Float | 0..1 | 0 | — |
| `portamento` | Portamento | Float | 0..1 | 0 | sec3+142 |
| `porta_mode` | Porta Auto/Exp | Enum | Off/Auto/Exp | Off | — |
| `bend_range` | Pitch Bend | Int | 0..24 semi | 2 | — |
| `quality` | OSC Quality | Enum | Low/LoMid/HiMid/High | High | — |

## Notes
- **Quality** = oversampling factor; "High" = the reference target for the OSC spectral match.
- Coarse/fine/pan/sync/curve/uni-voices bytes not yet located — add when test patches isolate them.
