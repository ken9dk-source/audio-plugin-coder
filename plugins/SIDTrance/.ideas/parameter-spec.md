# Parameter Specification: SID Trance Machine

## Oscillator 1
| ID | Name | Type | Range | Default | Unit |
|---|---|---|---|---|---|
| `osc1_wave` | OSC1 Waveform | Int | 0=SAW, 1=TRI, 2=PULSE, 3=NOISE, 4=SAW+TRI, 5=RING | 0 | — |
| `osc1_semi` | OSC1 Semi | Int | -24 to +24 | 0 | semitones |
| `osc1_fine` | OSC1 Fine | Float | -100.0 to +100.0 | 0.0 | cents |
| `osc1_pw` | OSC1 Pulse Width | Float | 0.0 to 1.0 | 0.5 | % |
| `osc1_attack` | OSC1 Attack | Float | 0.001 to 10.0 | 0.01 | s |
| `osc1_decay` | OSC1 Decay | Float | 0.001 to 10.0 | 0.3 | s |
| `osc1_sustain` | OSC1 Sustain | Float | 0.0 to 1.0 | 0.7 | — |
| `osc1_release` | OSC1 Release | Float | 0.001 to 20.0 | 0.5 | s |
| `osc1_volume` | OSC1 Volume | Float | 0.0 to 1.0 | 0.8 | — |

## Oscillator 2
| ID | Name | Type | Range | Default | Unit |
|---|---|---|---|---|---|
| `osc2_wave` | OSC2 Waveform | Int | 0=SAW, 1=TRI, 2=PULSE, 3=NOISE, 4=SAW+TRI, 5=RING | 1 | — |
| `osc2_semi` | OSC2 Semi | Int | -24 to +24 | 0 | semitones |
| `osc2_fine` | OSC2 Fine | Float | -100.0 to +100.0 | -7.0 | cents |
| `osc2_pw` | OSC2 Pulse Width | Float | 0.0 to 1.0 | 0.5 | % |
| `osc2_attack` | OSC2 Attack | Float | 0.001 to 10.0 | 0.01 | s |
| `osc2_decay` | OSC2 Decay | Float | 0.001 to 10.0 | 0.3 | s |
| `osc2_sustain` | OSC2 Sustain | Float | 0.0 to 1.0 | 0.7 | — |
| `osc2_release` | OSC2 Release | Float | 0.001 to 20.0 | 0.5 | s |
| `osc2_volume` | OSC2 Volume | Float | 0.0 to 1.0 | 0.6 | — |

## Oscillator 3
| ID | Name | Type | Range | Default | Unit |
|---|---|---|---|---|---|
| `osc3_wave` | OSC3 Waveform | Int | 0=SAW, 1=TRI, 2=PULSE, 3=NOISE, 4=SAW+TRI, 5=RING | 3 | — |
| `osc3_semi` | OSC3 Semi | Int | -24 to +24 | 0 | semitones |
| `osc3_fine` | OSC3 Fine | Float | -100.0 to +100.0 | 0.0 | cents |
| `osc3_pw` | OSC3 Pulse Width | Float | 0.0 to 1.0 | 0.5 | % |
| `osc3_attack` | OSC3 Attack | Float | 0.001 to 10.0 | 0.001 | s |
| `osc3_decay` | OSC3 Decay | Float | 0.001 to 10.0 | 0.1 | s |
| `osc3_sustain` | OSC3 Sustain | Float | 0.0 to 1.0 | 0.0 | — |
| `osc3_release` | OSC3 Release | Float | 0.001 to 20.0 | 0.1 | s |
| `osc3_volume` | OSC3 Volume | Float | 0.0 to 1.0 | 0.3 | — |

## Filter (SID 6581-inspired)
| ID | Name | Type | Range | Default | Unit |
|---|---|---|---|---|---|
| `filter_cutoff` | Filter Cutoff | Float | 20.0 to 20000.0 | 2480.0 | Hz |
| `filter_res` | Filter Resonance | Float | 0.0 to 1.0 | 0.35 | — |
| `filter_type` | Filter Type | Int | 0=LP, 1=HP, 2=BP, 3=Notch | 0 | — |
| `filter_slope` | Filter Slope | Int | 0=12dB, 1=24dB | 1 | dB/oct |
| `filter_keytrack` | Key Track | Bool | 0/1 | 0 | — |
| `filter_velocity` | Velocity Sens | Bool | 0/1 | 0 | — |

## Amplifier Envelope (Global)
| ID | Name | Type | Range | Default | Unit |
|---|---|---|---|---|---|
| `amp_attack` | Amp Attack | Float | 0.001 to 10.0 | 0.005 | s |
| `amp_decay` | Amp Decay | Float | 0.001 to 10.0 | 0.3 | s |
| `amp_sustain` | Amp Sustain | Float | 0.0 to 1.0 | 0.8 | — |
| `amp_release` | Amp Release | Float | 0.001 to 20.0 | 0.4 | s |
| `amp_volume` | Amp Volume | Float | 0.0 to 1.0 | 0.85 | — |

## LFO 1
| ID | Name | Type | Range | Default | Unit |
|---|---|---|---|---|---|
| `lfo1_shape` | LFO1 Shape | Int | 0=Sine, 1=Tri, 2=Saw, 3=RevSaw, 4=Square, 5=S&H | 0 | — |
| `lfo1_rate` | LFO1 Rate | Float | 0.01 to 20.0 | 1.0 | Hz |
| `lfo1_sync` | LFO1 Sync | Bool | 0/1 | 0 | — |
| `lfo1_retrig` | LFO1 Retrig | Bool | 0/1 | 1 | — |

## LFO 2
| ID | Name | Type | Range | Default | Unit |
|---|---|---|---|---|---|
| `lfo2_shape` | LFO2 Shape | Int | 0=Sine, 1=Tri, 2=Saw, 3=RevSaw, 4=Square, 5=S&H | 1 | — |
| `lfo2_rate` | LFO2 Rate | Float | 0.01 to 20.0 | 0.25 | Hz |
| `lfo2_sync` | LFO2 Sync | Bool | 0/1 | 0 | — |
| `lfo2_retrig` | LFO2 Retrig | Bool | 0/1 | 0 | — |

## Mod Matrix (4 Slots)
| ID | Name | Type | Range | Default | Unit |
|---|---|---|---|---|---|
| `mod1_src` | Mod1 Source | Int | 0=LFO1, 1=LFO2, 2=Velocity, 3=ModWheel, 4=AmpEnv, 5=Key | 0 | — |
| `mod1_amt` | Mod1 Amount | Float | -1.0 to +1.0 | 0.375 | — |
| `mod1_dst` | Mod1 Destination | Int | 0=FilterCutoff, 1=OSC1PW, 2=OSC2PW, 3=OSC3PW, 4=AmpLevel, 5=FilterRes, 6=OSC1Fine, 7=OSC2Fine | 0 | — |
| `mod2_src` | Mod2 Source | Int | (same as mod1_src) | 1 | — |
| `mod2_amt` | Mod2 Amount | Float | -1.0 to +1.0 | 0.25 | — |
| `mod2_dst` | Mod2 Destination | Int | (same as mod1_dst) | 1 | — |
| `mod3_src` | Mod3 Source | Int | (same as mod1_src) | 2 | — |
| `mod3_amt` | Mod3 Amount | Float | -1.0 to +1.0 | 0.5 | — |
| `mod3_dst` | Mod3 Destination | Int | (same as mod1_dst) | 4 | — |
| `mod4_src` | Mod4 Source | Int | (same as mod1_src) | 3 | — |
| `mod4_amt` | Mod4 Amount | Float | -1.0 to +1.0 | 0.4 | — |
| `mod4_dst` | Mod4 Destination | Int | (same as mod1_dst) | 5 | — |

## Effects
| ID | Name | Type | Range | Default | Unit |
|---|---|---|---|---|---|
| `fx_chorus_on` | Chorus Enable | Bool | 0/1 | 1 | — |
| `fx_chorus_rate` | Chorus Rate | Float | 0.1 to 10.0 | 1.2 | Hz |
| `fx_chorus_depth` | Chorus Depth | Float | 0.0 to 1.0 | 0.4 | — |
| `fx_chorus_mix` | Chorus Mix | Float | 0.0 to 1.0 | 0.3 | — |
| `fx_delay_on` | Delay Enable | Bool | 0/1 | 1 | — |
| `fx_delay_time` | Delay Time | Int | 0=1/16, 1=1/8, 2=1/4, 3=3/8, 4=1/2, 5=3/4, 6=1/1 | 2 | note |
| `fx_delay_feedback` | Delay Feedback | Float | 0.0 to 0.95 | 0.35 | — |
| `fx_delay_mix` | Delay Mix | Float | 0.0 to 1.0 | 0.25 | — |
| `fx_reverb_on` | Reverb Enable | Bool | 0/1 | 1 | — |
| `fx_reverb_size` | Reverb Size | Float | 0.0 to 1.0 | 0.4 | — |
| `fx_reverb_damp` | Reverb Damping | Float | 0.0 to 1.0 | 0.6 | — |
| `fx_reverb_mix` | Reverb Mix | Float | 0.0 to 1.0 | 0.3 | — |

## Master
| ID | Name | Type | Range | Default | Unit |
|---|---|---|---|---|---|
| `master_volume` | Master Output | Float | 0.0 to 1.0 | 0.8 | — |
| `master_limiter` | Limiter On | Bool | 0/1 | 1 | — |
| `master_poly` | Mono/Poly Mode | Int | 0=Mono, 1=Poly | 1 | — |
| `master_voices` | Voice Count | Int | 1 to 16 | 16 | voices |

## Arp / Sequencer
| ID | Name | Type | Range | Default | Unit |
|---|---|---|---|---|---|
| `arp_on` | Arp Enable | Bool | 0/1 | 0 | — |
| `arp_mode` | Arp Mode | Int | 0=Up, 1=Down, 2=UpDown, 3=Random, 4=Order | 0 | — |
| `arp_octave` | Arp Octave | Int | 1 to 4 | 1 | oct |
| `arp_gate` | Arp Gate | Float | 0.0 to 1.0 | 0.75 | % |
| `arp_swing` | Arp Swing | Float | 0.0 to 0.5 | 0.0 | % |
| `arp_tempo` | Arp Tempo | Float | 60.0 to 200.0 | 140.0 | BPM |
| `arp_sync` | Arp Sync to DAW | Bool | 0/1 | 1 | — |
| `seq_step_01` | Step 1 | Bool | 0/1 | 1 | — |
| `seq_step_02` | Step 2 | Bool | 0/1 | 1 | — |
| `seq_step_03` | Step 3 | Bool | 0/1 | 1 | — |
| `seq_step_04` | Step 4 | Bool | 0/1 | 1 | — |
| `seq_step_05` | Step 5 | Bool | 0/1 | 1 | — |
| `seq_step_06` | Step 6 | Bool | 0/1 | 1 | — |
| `seq_step_07` | Step 7 | Bool | 0/1 | 0 | — |
| `seq_step_08` | Step 8 | Bool | 0/1 | 1 | — |
| `seq_step_09` | Step 9 | Bool | 0/1 | 1 | — |
| `seq_step_10` | Step 10 | Bool | 0/1 | 1 | — |
| `seq_step_11` | Step 11 | Bool | 0/1 | 1 | — |
| `seq_step_12` | Step 12 | Bool | 0/1 | 0 | — |
| `seq_step_13` | Step 13 | Bool | 0/1 | 1 | — |
| `seq_step_14` | Step 14 | Bool | 0/1 | 1 | — |
| `seq_step_15` | Step 15 | Bool | 0/1 | 0 | — |
| `seq_step_16` | Step 16 | Bool | 0/1 | 1 | — |

## SID-Specific / Unique Features
| ID | Name | Type | Range | Default | Unit |
|---|---|---|---|---|---|
| `trance_drift` | Trance Drift | Float | 0.0 to 1.0 | 0.2 | — |
| `digital_age` | Digital Age (Bitcrush) | Float | 0.0 to 1.0 | 0.0 | — |
| `sid_width` | SID Width (Unison spread) | Float | 0.0 to 1.0 | 0.5 | — |
| `analog_glow` | Analog Glow (Saturation) | Float | 0.0 to 1.0 | 0.25 | — |
| `sid_mode` | SID Mode | Int | 0=Classic, 1=Modern, 2=Trance | 2 | — |
