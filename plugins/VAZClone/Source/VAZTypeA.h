// VAZTypeA.h — BIT-EXACT VAZ Type-A Lowpass filter.
// Ports the exact per-sample integer recurrence from Vaz2010Core.dll (the modes 0–18 LP path,
// disasm @0x4DD646–0x4DD826): a 2×-oversampled 2-pole biquad (coefs from the dumped tables) +
// two one-pole LPs on the input + a resonance mix. The biquad state is stored as (acc>>32)<<4 =
// quantized to multiples of 16, which is what limits the resonance to ~16 dB — a float filter
// CANNOT reproduce this, so the integer arithmetic is kept verbatim here.
#pragma once
#include <cstdint>
#include <cmath>
#include <algorithm>
#include "VAZTypeATables.h"

struct VAZTypeA
{
    double  sr = 44100.0;
    int32_t s1 = 0, s2 = 0;        // biquad state (y[n-1], y[n-2])
    uint32_t acclo = 0;            // 64-bit accumulator low word (carries between sub-steps/samples)
    int32_t p1 = 0, p2 = 0;        // the two input one-pole LP states
    static constexpr double SCALE = 65536.0;   // float<->int; sets where the state-quantization bites = the Q

    void prepare (double s) noexcept { sr = s > 0.0 ? s : 44100.0; reset(); }
    void reset()           noexcept { s1 = s2 = 0; acclo = 0; p1 = p2 = 0; }

    static inline int32_t mulhi (int32_t a, int32_t b) noexcept   // ((int64)a*b) >> 32  (the imul/edx pattern)
    { return (int32_t) (((int64_t) a * b) >> 32); }
    static inline int32_t shl   (int32_t v, int n)     noexcept   // 32-bit wrapping left shift
    { return (int32_t) ((uint32_t) v << n); }
    static inline int32_t add32 (int32_t a, int32_t b) noexcept   // 32-bit wrapping add
    { return (int32_t) ((uint32_t) a + (uint32_t) b); }
    static inline int32_t sub32 (int32_t a, int32_t b) noexcept
    { return (int32_t) ((uint32_t) a - (uint32_t) b); }

    // fc in Hz, reso 0..1. Returns the lowpass output (Type A LP).
    double process (double in, double fc, double reso) noexcept
    {
        const double fcl = std::clamp (fc, 1.0, sr * 0.49);
        int ci = (int) std::lround (1024.0 * std::log (fcl) / 10.24);        // cutIdx = cutoff>>19 domain
        ci = std::clamp (ci, 0, 1023);
        int r255 = (int) std::lround (std::clamp (reso, 0.0, 1.0) * 255.0);  // resonance 0..255 (= [esi+0x164])
        const int ri = r255 >> 2;                                           // resoIdx 0..63 (table row)
        const int idx = ri * 1024 + ci;

        const int32_t a1 = VAZAType::kA1[idx], a2 = VAZAType::kA2[idx], b0 = VAZAType::kB0[idx];
        const int32_t rc = VAZAType::kRC[ci];
        const int32_t rmix = (int32_t) ((uint32_t) r255 << 23);

        int32_t x  = (int32_t) std::lround (std::clamp (in, -2.0, 2.0) * SCALE);
        int32_t xb = shl (x, 2);

        // ── 2× oversampled biquad (64-bit accumulate, low word carries) ──
        int64_t acc = (int64_t) acclo + (int64_t) s2 * a2 + (int64_t) s1 * a1 + (int64_t) xb * b0;
        int32_t h1  = (int32_t) (acc >> 32);
        s2 = s1; s1 = shl (h1, 4);
        acc = (int64_t) (uint32_t) acc + (int64_t) s2 * a2 + (int64_t) s1 * a1 + (int64_t) xb * b0;
        int32_t h2  = (int32_t) (acc >> 32);
        acclo = (uint32_t) acc; s2 = s1; s1 = shl (h2, 4);
        int32_t bq  = (int32_t) (((int64_t) h1 + h2) >> 1);

        // ── two one-pole LPs on the raw input (coef rc, cutoff-indexed) ──
        int32_t edi = x, a;
        a   = shl (sub32 (p1, edi), 2); edi = add32 (edi, mulhi (a, rc)); p1 = edi;
        a   = shl (sub32 (p2, edi), 2); edi = add32 (edi, mulhi (a, rc)); p2 = edi;

        // ── resonance mix: edi += (r255<<23)·(biquad − edi) ──
        int32_t m = sub32 (bq, edi);
        edi = add32 (edi, mulhi (rmix, m));

        return (double) edi / SCALE;
    }
};
