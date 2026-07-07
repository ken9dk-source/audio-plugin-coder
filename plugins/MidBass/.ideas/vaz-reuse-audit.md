# MidBass — VAZ-Clone Reuse Audit

Audit of the existing, verified DSP in this monorepo against the MidBass spec.
Everything listed as PORT has already shipped and/or been validated in another
plugin in this repo (VAZClone bit-exact engines, TranceAcid, TranceEQ, TranceKick).

## A. PORT DIRECTLY (verified, header-only, no rework)

| MidBass module | Source | Notes |
|---|---|---|
| Oscillators (anti-aliased) | `VAZClone/Source/Synth.h` → `OscBlock` | Mip-mapped wavetable osc, validated bit-for-bit against real VAZ. Saw→Tri morph (wave 0 + waveshape), band-limited Pulse w/ PWM (difference-of-saws), sine. Already reused by TranceAcid via `target_include_directories(... ../VAZClone/Source)`. Hard sync hooks exist (`mainWrapped` + `hardReset()`). |
| Unison stack | `TranceAcid/Source/VazOsc.h` | OscBlock wrapped in a JP-8000-style detune/spread stack (equal-power pan, 1/√n normalisation). Extend 7→8 voices + add **mono-unison** sum mode (new, trivial). |
| Filter — LP24/LP12 hero character | `VAZClone/Source/VAZTypeR.h` (+`VAZTypeRTables.h`) | **Bit-exact Roland-style 4-pole** integrator cascade, cubic soft-clip **in the feedback path** (exactly the spec's "analog saturation inside the filter"), self-oscillating, 2x oversampled internally. 2-pole mode = LP12. This is the heart of the instrument, already validated vs real hardware DSP. |
| Filter — HP12/BP12 | `TranceAcid/Source/SvfFilter.h` | TPT/ZDF SVF with tanh in the integrator path; provides HP12/BP12 (Type-R is LP-only). Same prepare/set/process shape → drop-in behind one mode switch. |
| Envelopes | `TranceAcid/Source/Envelope.h` | Exponential-segment ADSR (EarLevel model), click-free, minimum segment = 1 sample → supports sub-ms attack / 5 ms decay natively. Just widen the parameter ranges. |
| LFOs | `TranceAcid/Source/Lfo.h` | Sine/Tri/Saw/Ramp/Square/S&H, bipolar. Tempo-sync conversion already done at processor level in TranceAcid — copy that pattern. |
| Mod matrix | `TranceAcid/Source/ModMatrix.h` | Same accumulate-into-dstVals design; extend 4→6 slots and remap src/dst enums to the MidBass sets. |
| Glide / legato / retrig | `VAZClone/Source/SynthVoice.h` (lines ~155–225) | Constant-rate cents glide + exponential RC glide, portamento-auto (legato-only) logic — exactly the spec's voice-mode requirements. |
| Analog drift | `VAZClone/Source/SynthVoice.h` (per-voice `vd` detune) | Per-voice slow random drift modeled on VAZ Poly mode; add an amount knob. |
| EQ section | `TranceEQ/Source/Biquad.h` | RBJ cookbook biquads (LowShelf/Peak/HighShelf) in TDF-II double precision — exactly the spec's EQ. |
| Spectrum analyzer | `TranceEQ/Source/PluginProcessor.{h,cpp}` (FFT fifo → `vizBins[]`, log 20 Hz–20 kHz) | Capture path ports unchanged; only the display side changes (native Component instead of WebView canvas). |
| FX: Chorus | `VAZChorus/Source/VazChorusEngine.h` | Bit-exact fixed-point port of VAZ TFXChorus (3-tap tri-phase). Pure C++, no JUCE deps. |
| FX: Phaser | `VAZPhaser/Source/VazPhaserEngine.h` | Bit-exact VAZ TFXPhaser (N-stage allpass, exact coef LUT). |
| FX: Delay | `VAZDelay/Source/VazDelayEngine.h` | Bit-exact VAZ TFXDelay (stereo/ping-pong/double, damped feedback). Tempo-sync division→samples done in the wrapper. |
| FX: Reverb | `VAZReverb/Source/VazReverbEngine.h` | Bit-exact VAZ TFXReverb (9 combs + 4 series AP/channel). |
| FX: Flanger, Compressor | `VAZFlanger` / `VAZCompressor` Source | Audited VAZ ports (engine inline in processor) — lift the render loops. |

## B. ADAPT (verified pieces, new glue)

| MidBass module | Base | Adaptation |
|---|---|---|
| Saturation stage (5 types) | `TranceKick/Source/KickEngine.h` `saturate()` (4 drive-as-density types) + `TranceAcid/FxRack.h` `distort()` (tanh/hard/fold/biased-tube/cubic) | Map to Tape/Tube/Diode/SoftClip/HardClip; add parallel Mix and wrap in `juce::dsp::Oversampling` (2x). TranceKick already taught the "drive = density, self-limiting" lesson (see memory/status). |
| Transient shaper | `TranceKick/Source/KickEngine.h` punch/attack-emphasis follower | Add a sustain leg (two-follower attack/sustain differencing). |
| Drive PRE/POST filter | trivial gain + soft-clip stages | Reuse cubic clip from VAZ (`1.5(x−x³/3)`). |
| Hard sync / FM / ring mod | `OscBlock.mainWrapped`/`hardReset()` | Sync exists; light FM = phase-increment offset from osc2 output (small new code); ring = multiply (trivial). |
| Sub oscillator | `OscBlock` (sine/tri) + square via pulse | Fixed -1/-2 octave, own level, defeatable. Thin wrapper. |
| Voice stack 1x/2x/4x | `VazOsc` micro-detune logic | Stack = outer loop over 1/2/4 voice instances with fixed micro-detune table. |
| Test infra | `TranceEQ/Tests/` (Catch2 v3, headless processor compile, `APC_BUILD_TESTS`) + `TranceAcid/render_main.cpp` render harness | Copy wholesale; MidBass gets the same `MIDBASS_HEADLESS=1` trick + an offline render harness for perceptual checks. |

## C. WRITE NEW (no prior art in repo)

- **MacroMap table** — 6 macros → weighted parameter offsets (single `constexpr` table).
- **SweetSpots table** — data-driven teal arc ranges for key knobs.
- **Mono-unison summing + <200 Hz width bypass** (Width macro rule).
- **Keytrack** on cutoff (one line, but new).
- **Factory presets** (10 quick-slots) — APVTS snapshots, authored in Phase 7.
- **Native JUCE GUI** — custom LookAndFeel (rack aesthetic), fixed one-panel layout, `MidiKeyboardComponent` strip. Note: recent plugins here are WebView, but the spec (custom LookAndFeel + MidiKeyboardComponent + fixed panel) calls for **native JUCE**, like TranceChords. `ui_framework: "native-juce"` set in status.json.

## Key architecture decision (flagged for approval)

**Filter core = VAZ Type-R (bit-exact, integer) for LP24/LP12 + ZDF SVF for HP12/BP12.**
Rationale: Type-R *is* the verified Roland-style model with feedback-path saturation
and self-oscillation — the exact character the spec prioritizes — but it has no
HP/BP taps. The SVF covers HP12/BP12 (secondary modes for mid-bass work) with its
own tanh feedback saturation. Alternative (uniform SVF for all modes) would lose
the verified Roland character on the modes that matter most.
