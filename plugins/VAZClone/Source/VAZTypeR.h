// VAZTypeR.h - BIT-EXACT VAZ Type-R filter (4-pole integrator cascade, self-oscillating).
// Ported from Vaz2010Core.dll (disasm @0x4DD82B-0x4DDA91): two cascaded sections, each a
// 2x-oversampled 2-pole biquad (R coef tables 0x69/0x65/0x61, Q30) with a cubic x-x^3>>(64+S)
// soft-clip on the fed-back state (S=1 section 1, S=2 section 2) = the distorted resonance that
// self-oscillates. input<<3, output = avg of the 2 sub-steps, then a one-pole highpass (post-HP).
// 2-pole modes run section 2 only (centre tap); 4-pole runs both. Integer arithmetic kept verbatim.
#pragma once
#include <cstdint>
#include <cmath>
#include <algorithm>
#include "VAZTypeRTables.h"   // R biquad b0/a1/a2 (0x69/0x61/0x65)
#include "VAZTypeATables.h"   // kRC (0x5535e4) reused for the post-HP one-pole coef

struct VAZTypeR
{
    double  sr = 44100.0;
    int32_t s1a = 0, s2a = 0, s1b = 0, s2b = 0, hpS = 0;   // section1, section2, post-HP states
    uint32_t accA = 0, accB = 0;                            // 64-bit accumulator low words
    static constexpr double SCALE = 65536.0;

    void prepare (double s) noexcept { sr = s > 0.0 ? s : 44100.0; reset(); }
    void reset()           noexcept { s1a = s2a = s1b = s2b = hpS = 0; accA = accB = 0; }

    static inline int32_t mulhi (int32_t a, int32_t b) noexcept { return (int32_t) (((int64_t) a * b) >> 32); }
    static inline int32_t shl   (int32_t v, int n)     noexcept { return (int32_t) ((uint32_t) v << n); }
    static inline int32_t add32 (int32_t a, int32_t b) noexcept { return (int32_t) ((uint32_t) a + (uint32_t) b); }
    static inline int32_t cubic (int32_t v, int sh)    noexcept   // v - (v^3 >> 32 >> 32 >> sh)
    { int32_t t = mulhi (v, v); t = mulhi (v, t); return (int32_t) ((uint32_t) v - (uint32_t) (t >> sh)); }

    // one 2x-oversampled biquad section + cubic feedback. xb = section input. Returns h1+h2 (sum of sub-step highs).
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

    // poles = 2 or 4. fc/hpFc in Hz, reso 0..1. Returns the (highpassed) Type-R output.
    double process (int poles, double in, double fc, double reso, double hpFc) noexcept
    {
        int ci = (int) std::lround (1024.0 * std::log (std::clamp (fc, 1.0, sr * 0.49)) / 10.24);
        ci = std::clamp (ci, 0, 1023);
        int ri = ((int) std::lround (std::clamp (reso, 0.0, 1.0) * 255.0)) >> 2; ri = std::clamp (ri, 0, 63);
        const int idx = ri * 1024 + ci;
        int hi = (int) std::lround (1024.0 * std::log (std::clamp (hpFc, 1.0, sr * 0.49)) / 10.24);
        const int32_t hpCoef = VAZAType::kRC[std::clamp (hi, 0, 1023)];

        int32_t x   = (int32_t) std::lround (std::clamp (in, -2.0, 2.0) * SCALE);
        int32_t edi = shl (x, 3);                                   // input << 3
        if (poles >= 4)
            edi = shl (section (s1a, s2a, accA, edi, idx, 1), 2);   // section 1 (cubic >>1), out<<2 -> sect2 in
        edi = section (s1b, s2b, accB, edi, idx, 2) >> 1;           // section 2 (cubic >>2), averaged

        // post one-pole highpass:  m = ((hpS+edi)<<2)*hpCoef>>32 ;  hpS = m-edi ;  out = m
        int32_t m = mulhi (shl (add32 (hpS, edi), 2), hpCoef);
        hpS = (int32_t) ((uint32_t) m - (uint32_t) edi);
        return (double) m / SCALE;
    }
};
