// VAZTypeC.h — BIT-EXACT VAZ Type-C filter (2P/4P resonant LP cascade with Separation).
// Ported from Vaz2010Core.dll voice render (FUN_004dbddc, the internal-mode 0x20-0x2d branch).
// C reuses Type-R's resonator biquad EXACTLY (same 0x69/0x65/0x61 coef tables + cubic x-x^3 soft-
// clip, section1 >>1 / section2 >>2). The ONLY differences vs Type-R:
//   • the two 2x-oversampled sections run at cutoff indices offset by ±Separation (4-pole); a
//     2-pole mode runs section 2 only at the base cutoff.
//   • the closing one-pole uses kRC[ +0x274 param * 4 ] (a LINEAR index) instead of R's log-freq HP.
// Internal modes: 0x20 = C 2P, 0x24 = C 2P (HM), 0x28 = C 4P, 0x2c = C 4P (SM). Integer math verbatim.
#pragma once
#include <cstdint>
#include <cmath>
#include <algorithm>
#include "VAZTypeRTables.h"   // R biquad b0/a1/a2 (0x69/0x61/0x65) — shared with Type-R
#include "VAZTypeATables.h"   // kRC (0x5535e4) — the closing one-pole coef table

struct VAZTypeC
{
    double  sr = 44100.0;
    int32_t s1a = 0, s2a = 0, s1b = 0, s2b = 0, hpS = 0;   // section1, section2, closing one-pole states
    uint32_t accA = 0, accB = 0;                            // 64-bit accumulator low words
    static constexpr double SCALE = 65536.0;

    void prepare (double s) noexcept { sr = s > 0.0 ? s : 44100.0; reset(); }
    void reset()           noexcept { s1a = s2a = s1b = s2b = hpS = 0; accA = accB = 0; }

    static inline int32_t mulhi (int32_t a, int32_t b) noexcept { return (int32_t) (((int64_t) a * b) >> 32); }
    static inline int32_t shl   (int32_t v, int n)     noexcept { return (int32_t) ((uint32_t) v << n); }
    static inline int32_t add32 (int32_t a, int32_t b) noexcept { return (int32_t) ((uint32_t) a + (uint32_t) b); }
    static inline int32_t cubic (int32_t v, int sh)    noexcept
    { int32_t t = mulhi (v, v); t = mulhi (v, t); return (int32_t) ((uint32_t) v - (uint32_t) (t >> sh)); }

    // One 2x-oversampled biquad section + cubic feedback (identical to Type-R's). Returns h1+h2.
    inline int32_t section (int32_t& s1, int32_t& s2, uint32_t& acclo, int32_t xb, int idx, int cubSh) noexcept
    {
        const int32_t b0 = VAZTypeRT::kB0[idx], a1 = VAZTypeRT::kA1[idx], a2 = VAZTypeRT::kA2[idx];
        int64_t acc = (int64_t) acclo + (int64_t) xb * b0 + (int64_t) s2 * a2 + (int64_t) s1 * a1;
        int32_t h1 = (int32_t) (acc >> 32);
        s2 = s1; s1 = cubic (shl (h1, 2), cubSh);
        acc = (int64_t) (uint32_t) acc + (int64_t) xb * b0 + (int64_t) s2 * a2 + (int64_t) s1 * a1;
        int32_t h2 = (int32_t) (acc >> 32); acclo = (uint32_t) acc;
        s2 = s1; s1 = cubic (shl (h2, 2), cubSh);
        return (int32_t) (((int64_t) h1 + h2));
    }

    // poles 2/4. fc Hz, reso 0..1, sepNorm 0..1 (Separation = flt_aux), finalNorm 0..1 (+0x274 = hp_cutoff).
    double process (int poles, double in, double fc, double reso, double sepNorm, double finalNorm) noexcept
    {
        int ci = (int) std::lround (1024.0 * std::log (std::clamp (fc, 1.0, sr * 0.49)) / 10.24);
        ci = std::clamp (ci, 0, 1023);
        int ri = ((int) std::lround (std::clamp (reso, 0.0, 1.0) * 255.0)) >> 2; ri = std::clamp (ri, 0, 63);
        const int sep255 = (int) std::lround (std::clamp (sepNorm, 0.0, 1.0) * 255.0);
        const int sepIdx  = sep255 << 1;                                   // (sep255<<15)>>14
        const int fin255  = (int) std::lround (std::clamp (finalNorm, 0.0, 1.0) * 255.0);
        const int32_t finCoef = VAZAType::kRC[std::clamp (fin255 << 2, 0, 1023)];   // kRC[+0x274 * 4]

        int32_t x   = (int32_t) std::lround (std::clamp (in, -2.0, 2.0) * SCALE);
        int32_t edi = shl (x, 3);                                          // input << 3
        if (poles >= 4)
        {
            const int idx1 = std::clamp (ci + sepIdx, 0, 0x3ff) + ri * 1024;   // section 1 = cutoff + Separation
            const int idx2 = std::max (ci - sepIdx, 0)         + ri * 1024;   // section 2 = cutoff − Separation
            edi = shl (section (s1a, s2a, accA, edi, idx1, 1), 2);          // section 1 (cubic >>1), out<<2
            edi = section (s1b, s2b, accB, edi, idx2, 2) >> 1;             // section 2 (cubic >>2), averaged
        }
        else
            edi = section (s1b, s2b, accB, edi, ci + ri * 1024, 2) >> 1;   // 2-pole: section 2 only, base cutoff

        // closing one-pole:  m = ((hpS+edi)<<2)*finCoef>>32 ;  hpS = m-edi ;  out = m
        int32_t m = mulhi (shl (add32 (hpS, edi), 2), finCoef);
        hpS = (int32_t) ((uint32_t) m - (uint32_t) edi);
        return (double) m / SCALE;
    }
};
