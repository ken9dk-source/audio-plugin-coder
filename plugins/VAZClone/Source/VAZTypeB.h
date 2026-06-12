// VAZTypeB.h — BIT-EXACT VAZ Type-A/B filter (all taps) — the modes < 0x13 branch of FUN_004dbddc.
// Same 2x-oversampled biquad as Type-A (tables 0x55/0x59/0x5d, state (acc>>32)<<4) + two input
// one-poles (coef kRC[cutoff]) + a resonance mix — but with LP/BP/HP tap routings, and (for the B
// family) the biquad resonance-ROW is taken from Bandwidth (0xff - bw) instead of Resonance.
// Type-A LP (mode 0) keeps its own VAZTypeA.h; this covers A-BP/HP (modes 5/4) and B-LP/BP/HP (1/7/6).
//   tap: 0 = LP, 1 = BP, 2 = HP.  rowReso255 = resonance row (A: reso*255, B: 255-bw*255).
//   mixReso255 = the resonance-mix amount (always the Resonance param * 255).
#pragma once
#include <cstdint>
#include <cmath>
#include <algorithm>
#include "VAZTypeATables.h"   // shared A/B biquad b0/a1/a2 + kRC (0x55/0x59/0x5d, 0x5535e4)

struct VAZTypeB
{
    double  sr = 44100.0;
    int32_t s1 = 0, s2 = 0;        // biquad state
    uint32_t acclo = 0;            // 64-bit accumulator low word
    int32_t p1 = 0, p2 = 0;        // the two one-pole states (esi+0x17c / +0x180)
    static constexpr double SCALE = 65536.0;

    void prepare (double s) noexcept { sr = s > 0.0 ? s : 44100.0; reset(); }
    void reset()           noexcept { s1 = s2 = 0; acclo = 0; p1 = p2 = 0; }

    static inline int32_t mulhi (int32_t a, int32_t b) noexcept { return (int32_t) (((int64_t) a * b) >> 32); }
    static inline int32_t shl   (int32_t v, int n)     noexcept { return (int32_t) ((uint32_t) v << n); }
    static inline int32_t add32 (int32_t a, int32_t b) noexcept { return (int32_t) ((uint32_t) a + (uint32_t) b); }
    static inline int32_t sub32 (int32_t a, int32_t b) noexcept { return (int32_t) ((uint32_t) a - (uint32_t) b); }

    double process (int tap, double in, double fc, int mixReso255, int rowReso255) noexcept
    {
        int ci = (int) std::lround (1024.0 * std::log (std::clamp (fc, 1.0, sr * 0.49)) / 10.24);
        ci = std::clamp (ci, 0, 1023);
        const int ri  = std::clamp (rowReso255, 0, 255) >> 2;
        const int idx = ri * 1024 + ci;
        const int32_t a1 = VAZAType::kA1[idx], a2 = VAZAType::kA2[idx], b0 = VAZAType::kB0[idx];
        const int32_t rc = VAZAType::kRC[ci];
        const int32_t rmix = (int32_t) ((uint32_t) std::clamp (mixReso255, 0, 255) << 23);

        int32_t x  = (int32_t) std::lround (std::clamp (in, -2.0, 2.0) * SCALE);
        int32_t xb = shl (x, 2);

        // ── 2x-oversampled biquad (identical to Type-A) ──
        int64_t acc = (int64_t) acclo + (int64_t) s2 * a2 + (int64_t) s1 * a1 + (int64_t) xb * b0;
        int32_t h1  = (int32_t) (acc >> 32);
        s2 = s1; s1 = shl (h1, 4);
        acc = (int64_t) (uint32_t) acc + (int64_t) s2 * a2 + (int64_t) s1 * a1 + (int64_t) xb * b0;
        int32_t h2  = (int32_t) (acc >> 32);
        acclo = (uint32_t) acc; s2 = s1; s1 = shl (h2, 4);
        const int32_t bq = (int32_t) (((int64_t) h1 + h2) >> 1);

        int32_t edi = x, out;
        if (tap == 0)                                                   // ── LP (= Type-A) ──
        {
            int32_t a = shl (sub32 (p1, edi), 2); edi = add32 (edi, mulhi (a, rc)); p1 = edi;
            a         = shl (sub32 (p2, edi), 2); edi = add32 (edi, mulhi (a, rc)); p2 = edi;
            out = add32 (edi, mulhi (rmix, sub32 (bq, edi)));
        }
        else if (tap == 1)                                             // ── BP ──
        {
            edi = add32 (edi, edi >> 1);                                // input * 1.5
            int32_t a = shl (sub32 (p1, edi), 2); edi = add32 (edi, mulhi (a, rc)); p1 = edi;
            edi = add32 (edi, mulhi (rmix, shl (sub32 (bq, edi), 1)));  // reso mix * 2
            out = mulhi (shl (add32 (p2, edi), 2), rc);
            p2  = sub32 (out, edi);
        }
        else                                                           // ── HP ──
        {
            edi = add32 (edi, mulhi (rmix, sub32 (shl (bq, 1), edi)));  // reso mix on (2*bq - in)
            int32_t t = mulhi (shl (add32 (p1, edi), 2), rc);
            p1  = sub32 (t, edi);
            out = mulhi (shl (add32 (p2, t), 2), rc);
            p2  = sub32 (out, t);
        }
        return (double) out / SCALE;
    }
};
