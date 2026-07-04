// vaz_fx_constants.h — RAW constants for the 7 VAZ effects, extracted from Vaz2010Core.dll (the FX DSP
// lives in Core.dll's TFX* classes; VAZ2010Effect.dll is a separate self-contained VST2 wrapper that was
// NOT the clone's source). Bit patterns / exact ratios with Ghidra addresses. Collection phase — no fixes.
//   TFX class-name RTTI: TFXFlanger@0x51FCD4  TFXChorus@0x5184D8  TFXPhaser@0x52107C
//                        TFXEqualizer@0x51E0C0  TFXDelay@0x51AF18  TFXReverb@0x522530
#pragma once
#include <cstdint>

namespace vazfx
{
    // ── FLANGER (TFXFlanger, per-sample @0x5204F4, block setup @0x520418) ─────────────────────────────
    // Delay-time from the 0..255 knob value: samples = (sr · 25/256000) · (value+1)  (FUN_0052076c @0x52076c).
    // 25/256000 == 1/10240, i.e. delay_ms = (value+1)/10.24 → 0.098 … 25.0 ms. VERIFIED == clone (SynthVoice/Flanger).
    inline constexpr double kFlangerDelayCoef = 25.0 / 256000.0;     // = 1/10240 (samples per (value+1) per sr)
    // BPM sync: cycleSamples = (sr · 60 · period[+0x274]) / (BPM[+0x2b0] · 48);  inc[+0x290] = round(2^31 / cycleSamples)
    inline constexpr double kFlangerSyncDiv   = 48.0;                // @0x520418 (period→cycle divisor)
    inline constexpr uint32_t kFlangerPhaseFull = 2147483647u;       // 2^31-1 (INT_MAX) phase full-scale @0x5204BB
    // Feedback comb: buf[w] = in·inGain[+0x294] + feedback[+0x264]·delayed ; dry/wet: out = in + mix[+0x284]·(delayed−in)
    // (LINEAR mix law). Triangle LFO = abs(phase accumulator) @0x520531-34 (NOT sine). Linear-interp fractional read.

    // ── REVERB (TFXReverb@0x522530) — render FUN_005228a4 @0x5228a4, size-setter FUN_00522fcc @0x522fcc, ─────
    //    damp-setter FUN_00523194 @0x523194, param-load FUN_005232a4 @0x5232a4. 100% INTEGER fixed-point
    //    (Q31 combs, Q28 damping) — NOT float. This is NOT juce::Reverb/Freeverb (see matrix §10 FORKERT TOPOLOGI).
    //
    // Signal chain per sample (FUN_005228a4): monoIn = (L+R)>>6 ; 9 parallel PLAIN Schroeder combs (no per-comb
    // LP) → 2 asymmetric weighted sums (pseudo-stereo) → 4 series allpass per channel (g=0.65) → 1 one-pole
    // damping LP per channel → linear dry/wet. Params: +0x260 size, +0x264 damp, +0x268 mix.
    //
    // Comb tunings (delay lengths, samples @44.1k), 0x523158..0x523188 — Freeverb's 8 with 1188→1187, +2 extra:
    inline constexpr int kReverbCombTuning[10] = {                  // 10 coefs allocated; render sums 9 (idx0..8)
        1116, 1187, 1277, 1356, 1422, 1491, 1557, 1617,             // @0x523158,0x523168,0x52316c,0x523170,
        1203, 1527 };                                               //   0x523174,0x523178,0x52317c,0x523180 / +0x523184,0x523188
    // Comb feedback coef[i] (Q31, +0x274,+0x427c,… stride 0x4008) computed per size/decay param in FUN_00522fcc
    // from seed −1.3520996e-5 (DAT_0052314c @0x52314c) / ((size·16+500)·sampleRate)  [FUN_00522fcc:6527], ·tuning[i].
    inline constexpr float  kReverbCoefSeed   = -1.3520995707949623e-05f;   // DAT_0052314c @0x52314c
    inline constexpr int    kReverbCombBufLen = 4096;               // buffer size, mask 0xfff (each comb 0x4008 bytes)
    // Pseudo-stereo weighted sums of the 9 comb damped-reads c0..c8 (render lines 119 & 125):
    //   L = 2·c0 + 1·c1 + 4·c2 + 2·c3 + 3·c4 + 4·c5        + 2·c7
    //   R = 2·c0 + 3·c1        + 2·c3 + 1·c4        + 4·c6 + 2·c7 + 4·c8
    // Allpass: 8 total = 4 series per channel, buffers 1024 (mask 0x3ff), +0x282c4..+0x2f2e0 stride 0x1004.
    //   out = delayed − in ; buf[w] = g·delayed + in.  Shared gain g = 0.65 (Freeverb uses 0.5):
    inline constexpr int    kReverbAllpassGainQ31 = 0x53333333;     // DAT_0052ba54 @0x52ba54 = 0.65·2^31
    inline constexpr double kReverbAllpassGain    = 0.65;           //   (= 1395864371 / 2^31)
    // Damping one-pole (FUN_00523194): damp_b = coef @+0x302e4 ; damp_a = 2^28 − coef @+0x302e0 (Q28 = 0x10000000).
    inline constexpr int    kReverbDampQ28 = 0x10000000;            // 2^28 unity @0x522fc0 / 0x523280
    // ALL delay LENGTHS = round(SR · tuning / 44100)  (length-setter FUN_00522c60: IMUL tuning, FILD, FDIV 44100 @0x522c84).
    //   → SR-DEPENDENT (scaled from the 44.1k tunings).  Comb tunings above; 8 allpass tunings (0x133/0x61/0x47/0x35):
    inline constexpr int kReverbAllpassTune[8] = { 307, 97, 71, 53,  307, 97, 71, 53 };  // {L chain}{R chain}, @0x522c60
    // Comb feedback coef (FUN_00522fcc @0x522fcc):  coef[i] = round( (2^31−1) · 2^( −1.3520996e-5·tuning[i]
    //   / ((size·16+500)·SR) ) )  (K80=2^31−1 @0x52315c; exp confirmed: exponent ∝ 1/(size·16+500)).
    //   ✅ RESIDUAL RESOLVED: the exact coefs were RUNTIME-DUMPED (tools/vaz_coef_dump.cpp loads Core.dll, resolves
    //   its IAT without DllMain, and calls the real 80-bit setters) → reference/vaz_reverb_coef_lut.h (256 sizes +
    //   256 damps @sr=44100; validated: lengths came out = tunings). Engine SR-adjusts coef by ^(44100/sr). BIT-EXACT.

    // ── CHORUS (TFXChorus@0x5184D8, per-sample FUN_00518ad8 @0x518ad8) — fixed-point Q-format ──────────────
    //   Topology: ONE shared circular delay line (+0x2a4, pow2, mask +0x2a0), input = (L+R)>>1 (MONO summed).
    //   3 audio taps read with linear interp; each tap's offset = base + (mod_k>>16), where mod_k =
    //   LFO1(phase+k·120°) + LFO2(phase2+k·120°) summed into ONE accumulator (2 LFOs → 3 combined taps, not 6).
    //   Stereo via tap difference (tapB−tapC)·lrPhase[+0x27c] (NOT independent L/R). Linear dry/wet mix[+0x280].
    //   Base delay (FUN_00518fbc @0x518fbc): baseSamp = (sr·50/256000)·(delay_param+1) = (delay+1)/5.12 ms.
    inline constexpr double kChorusDelayCoef = 50.0 / 256000.0;     // = 1/5120  (2× the flanger's 25/256000)
    //   3 taps at 0°/120°/240°: LUT index step 0x55 (=85/256≈⅓) @0x518B?? ; phase step 0x55555554 (120° of 2^32).
    inline constexpr uint32_t kChorusTapPhase120 = 0x55555554u;     // +120° in 32-bit phase (±0x55555554 = ±120°)
    inline constexpr int      kChorusLutTapStep  = 0x55;            // 85/256 sine-LUT tap step (mode 0)
    //   Waveform mode ([+0x264] LFO1, [+0x270] LFO2) — NOTE order vs clone:
    //     mode 0 = sine LUT read (+0x2a8, 256-entry, interp)           @0x518B29
    //     mode 1 = TRAPEZOID  clamp(|phase|−2^29, 0, 2^30)·depth        @0x518BA4   ← clone maps 1→Triangle (SWAPPED)
    //     mode 2 = TRIANGLE   (|phase|>>1)·depth                        @0x518C1F   ← clone maps 2→Trapezoid (SWAPPED)
    inline constexpr int kChorusTrapClampBias = 0x20000000;         // 2^29 (trapezoid |ph|−2^29)
    inline constexpr int kChorusTrapClampMax  = 0x40000000;         // 2^30 (trapezoid upper clamp)

    // ── PHASER (TFXPhaser@0x52107C, render FUN_005218d8 @0x5218d8, coef-LUT FUN_00521aa0 @0x521aa0) ─────────
    //   N-stage 1st-order allpass chain (N = [+0x2a0]); coef from a 512-entry LUT (+0x310) indexed by the LFO:
    //     idx = ( (|phase|>>16)·depth[+0x280] + center[+0x264]·0x8000 ) >> 15      (LFO = abs = triangle)
    //   allpass: y = buf − coef·x ; buf = coef·y + x.  Feedback [+0x29c] on last stage; linear mix [+0x288].
    //   Stereo = SEPARATE banks (+0x2b0 L / +0x2e0 R), R phase offset lrPhase[+0x284]. Effect-type +0x58 = 5.
    //   LUT builder: coef[i] = (1.0 − (i·5·ln2·K)·440/255 / sr) · 2^30, clamped ≥ 0   (i = 0..511):
    inline constexpr float kPhaserLutFreqBase = 440.0f;   // DAT_00521b50 (A440 reference)
    inline constexpr float kPhaserLutDiv255   = 255.0f;   // DAT_00521b4c
    inline constexpr int   kPhaserLutQ30       = 0x40000000; // DAT_00521b58 = 2^30 (Q30 coef scale)
    inline constexpr int   kPhaserLutEntries   = 0x200;   // 512-entry coef LUT (clone uses per-sample tan() bilinear)

    // ── DELAY (TFXDelay@0x51AF18, render FUN_0051bba8 @0x51bba8, prepare FUN_0051bf78 @0x51bf78) ───────────
    //   Stereo circular buffer (+0x2e0, 8 B/frame, size pow2 mask +0x2dc = (sr·0x9f6/1000) rounded up).
    //   Separate L/R read taps (+0x2cc,+0x2d0); feedback [+0x27c] L / [+0x298] R with a ONE-POLE damping LP
    //   in the loop (coef +0x2a8/+0x2ac, state +0x2b0/+0x2b4). Dry gain +0x2c0, wet gain +0x2b8. Type +0x58 = 2.
    //   MODES ([+0x260]):  0 = Stereo (independent L/R) · 1 = Ping-Pong (cross-feedback) · ≥2 = MONO (L+R sum,
    //   one delay, dual-mono out).  ⚠ clone mode 2 = "Double" (series delays) ≠ VAZ mode 2 = mono.
    inline constexpr int kDelayBufCoef = 0x9f6;           // 2550: bufSamples = sr·2550/1000 (≈2.55 s) → pow2

    // ── AUTOPAN (TFXAutopan@~0x517598, render FUN_00517d34 @0x517d34, ctor FUN_00517ae4 @0x517ae4) ────────
    //   LFO → pan pos = min[+0x260] + lfo·(max[+0x264]−min[+0x260]).  LFO: mode0 = triangle (|phase|>>9),
    //   mode1 = 256-entry sine LUT (+0x684, interp).  CONSTANT-POWER pan via a 257-entry LUT (+0x280 / +0x284):
    //   gainL = LUT[255−idx], gainR = LUT[idx]  (idx = pan>>22, 8-bit).  Type +0x58 = 8.
    //   Clone uses cos/sin(pan·π/2) = the exact continuous form of the same equal-power law (TILNÆRMET-BEVIDST).
    inline constexpr int kAutopanPanLutEntries = 0x101;   // 257-entry constant-power pan LUT

    // ── DECIMATOR (TFXDecimator@~0x51D8C8, render FUN_0051dbcc @0x51dbcc, quant-LUT FUN_0051d784) ─────────
    //   Sample-rate reduction: 11-bit accumulator [+0x268] += rate[+0x26c]; when > 0x7ff → sample-&-hold.
    //   BITCRUSH = truncation: held = (input & mask[+0x270]) + bias[+0x274]   ← clone uses round(x/q)·q instead.
    //   Post S&H there is a SMOOTHING one-pole (coef[+0x280], state[+0x284]/[+0x288]) that the clone OMITS.
    //   Quant/level table (FUN_0051d784): table[i] = i·ln2·K / 256 / 2  (257-entry exp).  Type +0x58 = 10.
    inline constexpr int kDecimatorRateMask = 0x7ff;      // 11-bit S&H accumulator threshold (2047)
    inline constexpr int kDecimatorQuantEntries = 0x101;  // 257-entry exp bit-depth LUT

    // Shared FX float-pool note: f32 = -0.241 recurs at 0x519054/0x51c11c/0x51e954/0x51f098/0x52089c (Chorus/
    // EQ/Flanger/Phaser) — role UNVERIFIED (needs per-site decode; do NOT assume it's a coefficient yet).
}
