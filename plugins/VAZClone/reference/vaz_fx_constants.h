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
    // NOTE: allpass delay LENGTHS (+0x282c0 etc.) are constructor-set object fields — not yet extracted (BSS/runtime).
    //   Whatever they are, they differ from JUCE's 556/441/341/225 and the whole topology already deviates.

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

    // ── (constants for PHASER / DELAY / AUTOPAN / DECIMATOR / EQ — pending extraction) ──
    // Shared FX float-pool note: f32 = -0.241 recurs at 0x519054/0x51c11c/0x51e954/0x51f098/0x52089c (Chorus/
    // EQ/Flanger/Phaser) — role UNVERIFIED (needs per-site decode; do NOT assume it's a coefficient yet).
}
