# MidBass — Development Roadmap

A dedicated Trance Mid-Bass synthesizer (2004–2010 uplifting/hard trance off-beat
bass). JUCE 8 / VST3 + Standalone, native JUCE GUI, monorepo plugin at
`plugins/MidBass/`. Reuse-first: verified DSP is ported from VAZClone / TranceAcid /
TranceEQ / TranceKick (see `.ideas/vaz-reuse-audit.md`).

**Discipline:** one phase = one git commit; tests green before commit; test-first
(failing test precedes the DSP); verify-before-edit.

---

## Phase 0 — Scaffold  ✅ (this commit)
- Plugin CMake target (VST3 + Standalone, IS_SYNTH, native JUCE — no WebView).
- **Full APVTS parameter layout up front** (~131 params, `Source/ParameterIDs.hpp`
  + `Source/Params.h`): OSC1-3, sub, unison, filter + drives, 2×ADSR, 2×LFO,
  6-slot mod matrix, saturation, EQ, transient, voice/output, 6 macros, FX chain.
- Placeholder editor (GenericAudioProcessorEditor) — real GUI is Phase 8.
- Catch2 test target (`Tests/`, `-DAPC_BUILD_TESTS=ON`): layout sanity, fast-envelope
  ranges present, state save/recall round-trip, silent+finite processBlock.
- pluginval pass.

## Phase 1 — Oscillators + Sub + Unison
- Port `OscBlock` (VAZClone wavetable, anti-aliased) + `VazOsc` unison stack (extend
  to 8 voices, add mono-unison sum). Sub osc wrapper (sine/tri/square, −1/−2 oct).
- Hard sync (osc2→osc1), light FM, ring mod. Analog drift (per-voice, VAZ-style).
- Tests: pitch accuracy (±0.5 cent via zero-crossing/FFT), alias floor of saw at
  high pitch (< −60 dB above Nyquist-folded region), drift bounded, unison mono-sum
  null test (L==R when mono).
- **Approval conditions (2026-07-07):**
  - FM + ring mod get aliasing-level tests at high fundamentals; FM amount range
    verified to stay in the "subtle" musical range.
  - Sub "fully defeatable" = true bypass: zero processing when off (asserted by test).
  - Mono-unison rule, explicit in code + tests: detune stays active, stereo spread
    forced to zero, output summed with the 1/√n norm; toggling mono-unison changes
    perceived level by ≤ ~1 dB.

## Phase 2 — Filter + Drive stages
- VAZ Type-R (bit-exact) for LP24/LP12; ZDF SVF for HP12/BP12. Drive PRE and POST
  as separate stages (VAZ cubic clip). Keytrack, bipolar filter-env amount input.
- Tests: magnitude response vs analytic reference within tolerance per mode
  (−3 dB point, slope), self-oscillation stays bounded/finite at reso=1, drive
  stages monotonic & finite.
- **Approval conditions (2026-07-07):**
  - Engine switching (Type-R ↔ SVF) must be click-free (state reset or short
    crossfade); test switches modes mid-note and asserts no discontinuity
    above −60 dBFS.
  - SVF path gets an equivalent drive/saturation structure so HP12/BP12 match
    the perceived drive behaviour of the Type-R modes; intentional differences
    documented.
  - Keytrack + filter-env amount map to identical cutoff scaling across both
    engines; test: same patch/note/env → cutoff trajectory within tolerance
    regardless of engine.

## Phase 3 — Envelopes + Voice management
- Port TranceAcid ADSR; ranges: attack 0.05 ms–2 s, decay 2 ms–2 s. Retrig vs
  legato mode. Mono voice + glide (constant-rate + legato-only option, from
  VAZClone SynthVoice). Voice stack 1x/2x/4x with micro-detune.
- Tests: attack/decay timing within 5 %, click-free retrigger (max sample delta
  bound), legato does not retrigger, glide reaches target.

## Phase 4 — LFOs + Mod matrix
- Port Lfo (5 waves + S&H), tempo sync (host BPM → Hz), note-on retrigger.
  6-slot matrix (vel/wheel/AT/fenv/LFO1/LFO2 → cutoff/reso/pitch/PWM/amp/drive).
- Tests: LFO rate accuracy, sync division math, matrix routing math (superposition),
  bipolar amounts.

## Phase 5 — Saturation + EQ + Transient shaper
- 5-type saturator (Tape/Tube/Diode/Soft/Hard from TranceKick+FxRack transfer
  functions), Drive + parallel Mix, 2x `juce::dsp::Oversampling`.
- EQ: RBJ low-shelf / mid-peak(f,g,Q) / high-shelf from TranceEQ Biquad.
- Transient shaper: attack/sustain two-follower design (base: TranceKick punch).
- Tests: THD ranking per type, alias suppression with OS on vs off, EQ magnitude
  response vs `makeCoeffs` analytic values, transient attack gain bound.

## Phase 6 — FX chain
- Fixed order Chorus → Phaser → Flanger → Delay (tempo-synced) → Reverb →
  Compressor, each bypassable, 3–5 knobs each. Port VazChorusEngine /
  VazPhaserEngine / VazDelayEngine / VazReverbEngine (bit-exact fixed-point) +
  flanger/compressor from their VAZ plugin ports.
- Tests: parity vs the source engines (impulse/noise diff ≈ 0 where algorithms
  identical), bypass = exact passthrough, delay division → samples math.

## Phase 7 — Macros + Sweet spots + Factory presets
- `MacroMap` table (single tweakable source of truth): Punch/Bite/Warmth/Snap/
  Body/Width → weighted multi-parameter offsets (Width bypassed in mono <200 Hz).
- `SweetSpots` table (knob → [lo,hi] arc) — data only, GUI reads it in Phase 8.
- **Approval condition (2026-07-07):** MacroMap and SweetSpots are data-driven
  tables (single header) with ALL mappings in one place — no macro logic
  scattered in the processor.
- **Approval condition (2026-07-07, carried from Phase 5):** the Diode/HardClip
  THD gap at the reference drive is only 0.6 dB — if sweet-spot/macro tuning
  touches the saturator drive gains, the THD ranking test MUST be re-run and
  the ranking preserved.
- 10 factory presets as one-click snapshots: Classic Juno, Choose&F, Tukan, ATN,
  Kandi, Nitrous, Pluck, Bass, Wide, Mono. APVTS-based browser (save/load).
- Tests: macro math (offset application + clamping), preset load → all params
  land, state versioning.

### CPU-rule breach acknowledgment (2026-07-07)
The Phase 6 deferral condition "Phases 7-8 must not increase audio-thread cost"
was BROKEN in Phase 7: the CPU spot moved 8.74 % → 10.65 %. The additions were
correctness fixes (per-effect engage crossfades, saturator wet ramp, phaser
limit-cycle cleanup) and correctness wins — but the breach is stated here, not
footnoted. Methodology hardened from Phase 8 on: 5 runs, median reported (with
min/max spread once to establish the measurement noise floor), and the
crossfade steady-state cost decomposed with a force-disable flag; if it costs
> 0.5 % steady-state it is a NAMED Phase 9 target, not "variance".
Phase 8 finding: ABSOLUTE medians swing with machine state across sessions
(observed 6.5-10.7 % for identical code) while within-batch spread stays
±0.3 % — therefore cost attributions are only ever made from SAME-BATCH A/B
toggles (crossfade flag, analyzer-tap flag), never from cross-session medians.

## Phase 8 — GUI
- Custom LookAndFeel: dark grey/brushed aluminium, orange LEDs, teal highlights,
  large round knobs. ONE fixed panel, zones: header/presets · osc row · filter+env
  hero row · LFO+matrix row · sat/EQ/transient/voice row · 6 macro knobs ·
  analyzer+FX strip · MidiKeyboardComponent.
- Sweet-spot teal arcs from the SweetSpots table. FFT analyzer (TranceEQ capture
  path) with 100–900 Hz mid-bass highlight.
- Tests: editor opens/closes without leaks (pluginval), attachment completeness
  check (every param reachable).

## Phase 9 — Integration
- CPU profile: < 5 % single core @ 8-voice unison (release build measurement).
- pluginval strictness 10, denormal protection (ScopedNoDenormals + flush checks),
  state save/recall stress, render-harness audition (offline WAVs per preset).

---

### Parameter budget (Phase 0 layout)
Global/voice 6 · OSC 22 · Sub 4 · Unison 4 · Filter 7 · Envelopes 8 · LFO 14 ·
Matrix 18 · Saturation 3 · EQ 7 · Transient 2 · Macros 6 · FX 30 ≈ **131**.

### Deviations from the brief (flagged)
- ROADMAP.md / CLAUDE.md live in `plugins/MidBass/` (this is a monorepo — the
  repository root serves ~20 plugins; a root CLAUDE.md would leak into every
  other plugin's sessions).
- Filter: HP12/BP12 come from the verified ZDF SVF, not Type-R (Type-R has no
  HP/BP taps) — see audit "Key architecture decision".
