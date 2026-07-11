// VAZTypeK.h — BIT-EXACT VAZ Type-R filter (the clone historically mislabelled it "K"): a distorted, self-oscillating
// 4-pole Sallen-Key/ladder. This is VAZ's handler @0x4ddf44 for .v2p 17-20 (internal 0x50-0x5d) — vaz_big.c:1499-1594.
// (VAZ's actual K is the 0x6d67 SVF = VAZTypeD; VAZ's actual R is THIS.) 2x-oversampled. Per pass: resonance-feedback
// section (states 0x188/0x18c, fixed coef 0x418937) → input − resoFeedback, saturation-clamp ±0xd105e8 → cubic soft-clip
// → 4 cascaded one-pole LPs (coefA = 0x6d87e8[cutIdx]); the 4th one-pole feeds the resonance section back.
//   resoGain = mulhi(0x6d97e8[cutIdx], reso<<22) << 2   (NO ÷2 — the decompile has none, line 1501).
//   2-pole tap (0x174) = both passes' 2nd one-pole; 4-pole tap = pass-1 4th (0x170) + pass-2 4th, accumulated.
//   OUTPUT = mode-SELECT: 2-pole if internal mode < 0x55 (.v2p 17/18), else 4-pole (.v2p 19/20) — never averaged.
//   post-HP one-pole coef = kRC[ hp_cutoff_byte · 4 ]  (LINEAR index, line 1580/1593), applied to (tap >> 1).
#pragma once
#include <cstdint>
#include <cmath>
#include <algorithm>
#include "VAZTypeKTables.h"   // coefA (0x6d87), coefB (0x6d97), cutoff-indexed
#include "VAZTypeATables.h"   // kRC (0x5535e4) for the post-HP one-pole

struct VAZTypeK
{
    double  sr = 44100.0;
    int32_t s17c = 0, s180 = 0, s184 = 0, s188 = 0, s18c = 0, s400 = 0;   // 4 one-poles + reso section + post-HP
    static constexpr double  SCALE = 65536.0;
    static constexpr int32_t KC = 0x418937, CHI = 0xd105e8, CLO = (int32_t) 0xff2efa18;   // CLO = −CHI

    void prepare (double s) noexcept { sr = s > 0.0 ? s : 44100.0; reset(); }
    void reset()           noexcept { s17c = s180 = s184 = s188 = s18c = s400 = 0; }

    static inline int32_t mulhi (int32_t a, int32_t b) noexcept { return (int32_t) (((int64_t) a * b) >> 32); }
    static inline int32_t shl   (int32_t v, int n)     noexcept { return (int32_t) ((uint32_t) v << n); }
    static inline int32_t sub32 (int32_t a, int32_t b) noexcept { return (int32_t) ((uint32_t) a - (uint32_t) b); }
    static inline int32_t add32 (int32_t a, int32_t b) noexcept { return (int32_t) ((uint32_t) a + (uint32_t) b); }

    // fourPole = false → 2-pole tap (.v2p 17/18); true → 4-pole (.v2p 19/20). hpNorm 0..1 = hp_cutoff param.
    double process (bool fourPole, double in, double fc, double reso, double hpNorm) noexcept
    {
        int ci = (int) std::lround (1024.0 * std::log (std::clamp (fc, 1.0, sr * 0.49)) / 10.24);
        ci = std::clamp (ci, 0, 1023);
        const int32_t coefA = VAZTypeKT::kCoefA[ci], coefB = VAZTypeKT::kCoefB[ci];
        const int r255 = (int) std::lround (std::clamp (reso, 0.0, 1.0) * 255.0);
        const int32_t resoGain = shl (mulhi (coefB, shl (r255, 22)), 2);           // faithful — NO ÷2

        const int32_t inp = (int32_t) std::lround (std::clamp (in, -2.0, 2.0) * SCALE);
        int32_t tap2 = 0, p1_4 = 0, iv9 = 0;
        for (int p = 0; p < 2; ++p)                                                 // 2x oversampled
        {
            const int32_t dd = sub32 (s18c, mulhi (s188, KC));                      // resonance feedback section
            s18c = add32 (s188, mulhi (dd, KC));
            int32_t v = std::clamp (sub32 (inp, mulhi (shl (dd, 5), resoGain)), CLO, CHI);   // input − reso, sat-clamp
            const int32_t vc = shl (v, 5); v = sub32 (v, mulhi (mulhi (vc, vc), vc));        // cubic soft-clip
            int32_t x = v, a;                                                       // 4 cascaded one-pole LPs
            a = shl (sub32 (s17c, x), 2); x = add32 (x, mulhi (a, coefA)); s17c = x;
            a = shl (sub32 (s180, x), 2); x = add32 (x, mulhi (a, coefA)); s180 = x;
            if (p == 0) tap2 = x; else tap2 = add32 (tap2, x);                      // 2-pole tap (+0x174, accumulate)
            a = shl (sub32 (s184, x), 2); x = add32 (x, mulhi (a, coefA)); s184 = x;
            a = shl (sub32 (s188, x), 2); x = add32 (x, mulhi (a, coefA)); s188 = x;
            if (p == 0) p1_4 = x; else iv9 = x;                                     // pass-1 4-pole (+0x170) / pass-2 4-pole
        }
        const int32_t tap = fourPole ? add32 (p1_4, iv9) : tap2;                    // mode-SELECT (line 1574), no averaging

        const int hpIdx = std::clamp (((int) std::lround (std::clamp (hpNorm, 0.0, 1.0) * 255.0)) << 2, 0, 1023);   // LINEAR
        const int32_t hpCoef = VAZAType::kRC[hpIdx];
        const int32_t half = tap >> 1;                                             // post-HP runs on tap>>1 (line 1592)
        const int32_t m = mulhi (shl (add32 (s400, half), 2), hpCoef);
        s400 = sub32 (m, half);
        return (double) m / SCALE;
    }
};
