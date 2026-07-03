// VazReverbEngine.h — fixed-point (Q31/Q28) port of VAZ 2010 TFXReverb render FUN_005228a4 @0x5228a4.
// Pure C++/no JUCE so the oracle can include it and diff against an independent reference transcription.
//
// Topology (NOT juce::Reverb/Freeverb): mono feed (L+R)>>6 → 9 parallel PLAIN feedback combs → 2 asymmetric
// integer weighted sums (pseudo-stereo) → 4 series allpass per channel (shared gain 0.65) → 1 global one-pole
// damping LP per channel (AFTER the allpasses) → linear dry/wet.  100% integer; every op mirrors the decompile.
//
// Lengths = round(SR·tuning/44100) (FUN_00522c60, IMUL tuning + FDIV 44100 @0x522c84 — SR-dependent).
//   comb tunings @44.1k: 1116 1187 1277 1356 1422 1491 1557 1617 1203 (1527 alloc'd, comb#10 unused)
//   allpass tunings:     307 97 71 53  (L chain) | 307 97 71 53 (R chain)   [0x133 0x61 0x47 0x35 @0x522c60]
// Allpass gain g = 0.65 = 0x53333333 Q31 (DAT_0052ba54).  Damping: damp1=2^28−coef, damp2=coef (Q28, FUN_00523194).
#pragma once
#include <cstdint>
#include <cstring>
#include <cmath>

struct VazReverbEngine
{
    static constexpr int   kCombs        = 9;          // 10 allocated (VAZ layout), 9 processed
    static constexpr int   kCombBuf      = 4096;       // mask 0xfff
    static constexpr int   kAllpass      = 8;          // 4 series per channel
    static constexpr int   kApBuf        = 1024;       // mask 0x3ff
    static constexpr int32_t kApGainQ31  = 0x53333333; // 0.65 (DAT_0052ba54 @0x52ba54)
    static constexpr int   kCombTune[10] = { 1116, 1187, 1277, 1356, 1422, 1491, 1557, 1617, 1203, 1527 };
    static constexpr int   kApTune[8]    = { 307, 97, 71, 53,  307, 97, 71, 53 };

    int32_t  combBuf[10][kCombBuf];
    int32_t  apBuf[8][kApBuf];
    int32_t  combLen[10] = {0}, combCoef[10] = {0};
    int32_t  apLen[8] = {0};
    int32_t  damp1 = 0, damp2 = 0;      // damp1 = 2^28 − coef ; damp2 = coef
    int32_t  mixMul = 0;                // mix << 23
    int32_t  stateL = 0, stateR = 0;    // damping one-pole states (+0x302e8 / +0x302ec)
    uint32_t counter = 0;               // comb phase (+0x26c)

    void clearBuffers() { std::memset (combBuf, 0, sizeof combBuf); std::memset (apBuf, 0, sizeof apBuf);
                          stateL = stateR = 0; counter = 0; }

    // Q31 multiply exactly as the binary: (int32)(( (int64)(x*2) * coef ) >> 32).
    static inline int32_t smul (int32_t x, int32_t coefQ31) noexcept
    { return (int32_t) (((int64_t) ((int64_t) x * 2) * (int64_t) coefQ31) >> 32); }
    // 32-bit "*4"/"<<2" that wraps like SHL/LEA (avoids signed-overflow UB).
    static inline int32_t mul4 (int32_t x) noexcept { return (int32_t) ((uint32_t) x << 2); }
    static inline int32_t mul2 (int32_t x) noexcept { return (int32_t) ((uint32_t) x << 1); }

    void setLengths (double sr) noexcept
    {
        for (int i = 0; i < 10; ++i) combLen[i] = (int32_t) std::llround (sr * kCombTune[i] / 44100.0);
        for (int i = 0; i < 8;  ++i) apLen[i]   = (int32_t) std::llround (sr * kApTune[i]   / 44100.0);
    }

    // One stereo frame (int32 samples), processed in place — exact transcription of FUN_005228a4's loop body.
    inline void processFrame (int32_t& L, int32_t& R) noexcept
    {
        const uint32_t u4 = counter + 1;          // unmasked (+0x26c + 1)
        const uint32_t u5 = u4 & 0xfff;           // comb index
        counter = u5;
        const int32_t inL = L, inR = R;
        const int32_t mono = (inL + inR) >> 6;    // arithmetic >>6

        int32_t c[9];
        for (int k = 0; k < 9; ++k)
        {
            c[k] = smul (combBuf[k][u5], combCoef[k]);                       // (buf*2*coef)>>32
            combBuf[k][(uint32_t) (combLen[k] + u5) & 0xfff] = c[k] + mono;  // buf[len+idx] = damped + mono
        }
        // pseudo-stereo weighted sums (render lines 119 & 125):
        const int32_t Lsum = c[0]*2 + c[1]   + c[2]*4 + c[3]*2 + c[4]*3 + c[5]*4          + c[7]*2;
        const int32_t Rsum = c[0]*2 + c[1]*3          + c[3]*2 + c[4]            + c[6]*4 + c[7]*2 + c[8]*4;

        const uint32_t ai = u4 & 0x3ff;           // allpass index
        // 4 series allpass per channel: out = read − in ; buf[len+idx] = g·read + in.
        int32_t x = Lsum;
        for (int k = 0; k < 4; ++k)
        {
            const int32_t r = apBuf[k][ai];
            const int32_t y = r - x;
            apBuf[k][(uint32_t) (apLen[k] + ai) & 0x3ff] = smul (r, kApGainQ31) + x;
            x = y;
        }
        const int32_t apOutL = x;                 // = r3 − ap2out (render line 161: iVar9 − iVar8)
        x = Rsum;
        for (int k = 4; k < 8; ++k)
        {
            const int32_t r = apBuf[k][ai];
            const int32_t y = r - x;
            apBuf[k][(uint32_t) (apLen[k] + ai) & 0x3ff] = smul (r, kApGainQ31) + x;
            x = y;
        }
        const int32_t apOutR = x;

        // global one-pole damping per channel (AFTER allpasses): y = (x*4·damp1 + yPrev*4·damp2) >> 32.
        const int32_t dampL = (int32_t) (((int64_t) mul4 (apOutL) * damp1) >> 32)
                            + (int32_t) (((int64_t) mul4 (stateL) * damp2) >> 32);
        stateL = dampL;
        const int32_t dampR = (int32_t) (((int64_t) mul4 (apOutR) * damp1) >> 32)
                            + (int32_t) (((int64_t) mul4 (stateR) * damp2) >> 32);
        stateR = dampR;

        // linear dry/wet: out = in + ((dampOut − in)*2 · mixMul) >> 32.
        L = inL + (int32_t) (((int64_t) mul2 (dampL - inL) * mixMul) >> 32);
        R = inR + (int32_t) (((int64_t) mul2 (dampR - inR) * mixMul) >> 32);
    }

    // ── Parameter mapping from the .v2p reverb fields (size/damp/mix are 0..255) ────────────────────────
    // size (+0x260) → comb feedback coefs; damp (+0x264) → damping coef; mix (+0x268) → mixMul = mix<<23.
    // The coef/damp curves use VAZ's 80-bit x87 float (FUN_00522fcc / FUN_00523194) which MSVC cannot
    // bit-reproduce; ported here in double as the documented residual. Lengths + render remain bit-exact.
    void setParams (double sr, int size, int damp, int mix) noexcept
    {
        setLengths (sr);
        // Comb feedback coef. VAZ's FUN_00522fcc computes it in 80-bit x87 float (K80=2^31−1 @0x52315c, exponent
        // −1.3520996e-5·tuning/((size·16+500)·sr) through FUN_00402ba8) — NOT reproducible in MSVC, and a literal
        // read degenerates to coef≈1 (infinite tail). Substitute a musically-equivalent RT60 decay so 'size' sets
        // the tail length, uniform across combs: coef[i] = 10^(−3·(len[i]/sr)/RT60), RT60 = 0.25..8 s from size.
        const double rt60 = 0.25 + (double) size / 255.0 * 7.75;   // 0.25 s (size 0) … 8 s (size 255)
        for (int i = 0; i < 10; ++i)
        {
            const double delaySec = (double) combLen[i] / sr;
            double cf = std::pow (10.0, -3.0 * delaySec / rt60);
            if (cf > 0.9995) cf = 0.9995;                          // guard: never exactly 1.0 (stability)
            if (cf < 0.0)    cf = 0.0;
            combCoef[i] = (int32_t) std::llround (2147483647.0 * cf);
        }
        // FUN_00523194: damp2 = coef (Q28), damp1 = 2^28 − coef. Map damp 0..255 → coef fraction of 2^28.
        double d = (double) damp / 255.0; if (d < 0.0) d = 0.0; if (d > 1.0) d = 1.0;
        damp2 = (int32_t) std::llround ((double) 0x10000000 * d);
        damp1 = 0x10000000 - damp2;
        mixMul = mix << 23;
    }
};
