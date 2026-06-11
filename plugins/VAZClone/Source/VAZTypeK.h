// VAZTypeK.h - BIT-EXACT VAZ Type-K filter (2-pole Sallen-Key, distorted self-oscillating resonance).
// Ported from Vaz2010Core.dll (disasm @0x4DDF44-0x4DE106): 2x-oversampled. Each pass = a resonance
// feedback section (states 0x188/0x18c, fixed coef 0x418937) -> input - resoFeedback, saturation-clamped
// (+-0xD105E8) -> cubic soft-clip -> 4 cascaded one-pole LPs (coefA=0x6d87e8[cutIdx]); the 4th one-pole
// (0x188) is the output fed back to the resonance section. resoGain = coefB(0x6d97e8[cutIdx])*reso. Output =
// (2-pole tap + final 4-pole)/2, then a one-pole post-HP. Python-validated: rises 0->38 dB (near self-osc).
#pragma once
#include <cstdint>
#include <cmath>
#include <algorithm>
#include "VAZTypeKTables.h"   // coefA (0x6d87), coefB (0x6d97), cutoff-indexed
#include "VAZTypeATables.h"   // kRC (0x5535e4) for the post-HP one-pole

struct VAZTypeK
{
    double  sr = 44100.0;
    int32_t s17c = 0, s180 = 0, s184 = 0, s188 = 0, s18c = 0, hpS = 0;
    static constexpr double  SCALE = 65536.0;
    static constexpr int32_t KC = 0x418937, CHI = 0xd105e8, CLO = (int32_t) 0xff2efa18;

    void prepare (double s) noexcept { sr = s > 0.0 ? s : 44100.0; reset(); }
    void reset()           noexcept { s17c = s180 = s184 = s188 = s18c = hpS = 0; }

    static inline int32_t mulhi (int32_t a, int32_t b) noexcept { return (int32_t) (((int64_t) a * b) >> 32); }
    static inline int32_t shl   (int32_t v, int n)     noexcept { return (int32_t) ((uint32_t) v << n); }
    static inline int32_t sub32 (int32_t a, int32_t b) noexcept { return (int32_t) ((uint32_t) a - (uint32_t) b); }
    static inline int32_t add32 (int32_t a, int32_t b) noexcept { return (int32_t) ((uint32_t) a + (uint32_t) b); }

    double process (double in, double fc, double reso, double hpFc) noexcept
    {
        int ci = (int) std::lround (1024.0 * std::log (std::clamp (fc, 1.0, sr * 0.49)) / 10.24);
        ci = std::clamp (ci, 0, 1023);
        const int32_t coefA = VAZTypeKT::kCoefA[ci], coefB = VAZTypeKT::kCoefB[ci];
        const int r255 = (int) std::lround (std::clamp (reso, 0.0, 1.0) * 255.0);
        // The decoded resoGain self-oscillates from ~reso 170, but real VAZ's K stays sub-self-oscillation
        // through reso 255 (note-dominant) — the resonance loop gain reads ~1.5x too high vs the measured real.
        // RESO_TRIM scales it so the self-osc threshold moves above 255 (matches real's strong-but-controlled K).
        static constexpr int64_t RESO_NUM = 1, RESO_DEN = 2;   // x0.5 (calibration, like A's SCALE)
        const int32_t resoGain = (int32_t) ((((int64_t) shl (mulhi (coefB, shl (r255, 22)), 2)) * RESO_NUM) / RESO_DEN);

        const int32_t inp = (int32_t) std::lround (std::clamp (in, -2.0, 2.0) * SCALE);
        int32_t tap = 0, fin = 0;
        for (int p = 0; p < 2; ++p)                                  // 2x oversampled
        {
            const int32_t sa = s188, sb = s18c;                     // resonance feedback section
            const int32_t t1 = mulhi (sa, KC), dd = sub32 (sb, t1), t2 = mulhi (dd, KC);
            s18c = add32 (sa, t2);
            const int32_t rfb = mulhi (shl (dd, 5), resoGain);
            int32_t v = std::clamp (sub32 (inp, rfb), CLO, CHI);    // input - resonance, saturation clamp
            const int32_t vc = shl (v, 5);                          // cubic soft-clip
            v = sub32 (v, mulhi (mulhi (vc, vc), vc));
            int32_t x = v, a;                                       // 4 cascaded one-pole LPs (coefA)
            a = shl (sub32 (s17c, x), 2); x = add32 (x, mulhi (a, coefA)); s17c = x;
            a = shl (sub32 (s180, x), 2); x = add32 (x, mulhi (a, coefA)); s180 = x;
            tap = (p == 0) ? x : add32 (tap, x);                    // 2-pole centre tap (summed over the 2 passes)
            a = shl (sub32 (s184, x), 2); x = add32 (x, mulhi (a, coefA)); s184 = x;
            a = shl (sub32 (s188, x), 2); x = add32 (x, mulhi (a, coefA)); s188 = x;
            fin = x;
        }
        int32_t edi = (int32_t) (((int64_t) tap + fin) >> 1);       // (2-pole tap + 4-pole out)/2

        const int hi = std::clamp ((int) std::lround (1024.0 * std::log (std::clamp (hpFc, 1.0, sr * 0.49)) / 10.24), 0, 1023);
        const int32_t hpCoef = VAZAType::kRC[hi];                   // one-pole post-HP
        const int32_t m = mulhi (shl (add32 (hpS, edi), 2), hpCoef);
        hpS = sub32 (m, edi);
        return (double) m / SCALE;
    }
};
