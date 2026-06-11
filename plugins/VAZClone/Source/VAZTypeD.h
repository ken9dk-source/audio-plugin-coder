// VAZTypeD.h - BIT-EXACT VAZ Type-D filter (state-variable / Chamberlin SVF).
// Ported from Vaz2010Core.dll (disasm @0x4DDE5C-0x4DDF3F): a 2x-oversampled 2-pole SVF.
// Per pass: resoFB = clamp(bandpass<<4 * resoGainCoef, +-0x1000000); hp = in - resoFB - lp;
// lp += coefA*(hp<<2); bp += coefA*((lp + resoFB - bp)<<2). coefA=0x6d67e8[cutIdx] (integrator),
// resoGainCoef = coefB(0x6d77e8[cutIdx])*reso. Output = avg of the 2 sub-steps' tap (0=LP,1=HP,2=BP).
// No post-HP (the SVF tap is the output). Integer arithmetic verbatim. (The HP+LP series mode = a
// 2nd cascaded section, not used by any factory patch, left to the float fallback.)
#pragma once
#include <cstdint>
#include <cmath>
#include <algorithm>
#include "VAZTypeDTables.h"

struct VAZTypeD
{
    double  sr = 44100.0;
    int32_t lp = 0, bp = 0;
    static constexpr double  SCALE = 65536.0;
    static constexpr int32_t CHI = 0x1000000, CLO = (int32_t) 0xff000000;   // resoFB clamp +-16.7M

    void prepare (double s) noexcept { sr = s > 0.0 ? s : 44100.0; reset(); }
    void reset()           noexcept { lp = bp = 0; }

    static inline int32_t mulhi (int32_t a, int32_t b) noexcept { return (int32_t) (((int64_t) a * b) >> 32); }
    static inline int32_t shl   (int32_t v, int n)     noexcept { return (int32_t) ((uint32_t) v << n); }
    static inline int32_t sub32 (int32_t a, int32_t b) noexcept { return (int32_t) ((uint32_t) a - (uint32_t) b); }
    static inline int32_t add32 (int32_t a, int32_t b) noexcept { return (int32_t) ((uint32_t) a + (uint32_t) b); }

    // tap: 0 = lowpass, 1 = highpass, 2 = bandpass
    double process (int tap, double in, double fc, double reso) noexcept
    {
        int ci = (int) std::lround (1024.0 * std::log (std::clamp (fc, 1.0, sr * 0.49)) / 10.24);
        ci = std::clamp (ci, 0, 1023);
        const int32_t coefA = VAZTypeDT::kCoefA[ci], coefB = VAZTypeDT::kCoefB[ci];
        const int r255 = (int) std::lround (std::clamp (reso, 0.0, 1.0) * 255.0);
        const int32_t rgc = shl (mulhi (coefB, shl (r255, 22)), 2);

        const int32_t inp = (int32_t) std::lround (std::clamp (in, -2.0, 2.0) * SCALE);
        int32_t v0 = 0, v1 = 0;
        for (int p = 0; p < 2; ++p)                                  // 2x oversampled
        {
            const int32_t resoFB = std::clamp (mulhi (shl (bp, 4), rgc), CLO, CHI);
            const int32_t hp = sub32 (sub32 (inp, resoFB), lp);
            lp = add32 (lp, mulhi (shl (hp, 2), coefA));
            bp = add32 (bp, mulhi (shl (sub32 (add32 (lp, resoFB), bp), 2), coefA));
            const int32_t val = (tap == 0) ? lp : (tap == 1) ? hp : bp;
            if (p == 0) v0 = val; else v1 = val;
        }
        return (double) ((int32_t) (((int64_t) v0 + v1) >> 1)) / SCALE;
    }
};
