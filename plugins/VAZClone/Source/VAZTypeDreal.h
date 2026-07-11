// VAZTypeDreal.h — BIT-EXACT VAZ Type-D (the REAL D filter): a 2-stage resonant SVF with cubic distortion.
// This is VAZ's handler @0x4ddaa8 for .v2p 10-13 (internal 0x30-0x34) — vaz_big.c:1349-1391. It is NOT the clean
// 0x6d67 SVF the clone previously mislabelled "VAZTypeD" (that one is actually VAZ's K, now on 15/16). Coef tables
// (0x6d45/55/65/66) runtime-dumped → VAZTypeDrealTables.h. Per sample (single pass, two integrator stages):
//   coefA = kCutA[cutIdx];  coefC = min(kResC[resoIdx], kCutRlim[cutIdx]);  coefD = kResD[resoIdx]
//   tmp = mulhi(s1<<2,coefA) + s2                                  (LP tap +0x170, pre)
//   hp  = mulhi(in<<1,coefD) − mulhi(s1<<2,coefC) − tmp           (HP tap +0x178, pre)
//   i9  = cubic(s1) + mulhi(hp<<2,coefA)
//   b   = mulhi(i9<<2,coefA) + tmp ;  s2=b ;  LP += b
//   hp += mulhi(in<<1,coefD) − mulhi(i9<<2,coefC) − b
//   s1  = mulhi(hp2<<2,coefA) + cubic(i9) ;  BP(+0x174) = s1 + i9
//   out = {LP,BP,HP}[mode&3]      cubic(x) = x − mulhi(x<<3, mulhi(x<<4,x<<4))
// tap: 0 = LP (mode 0x30), 1 = BP (0x31), 2 = HP (0x32). (mode-0x34 D HP+LP Separation variant = TODO.)
#pragma once
#include <cstdint>
#include <cmath>
#include <algorithm>
#include "VAZTypeDrealTables.h"

struct VAZTypeDreal
{
    double  sr = 44100.0;
    int32_t s1 = 0, s2 = 0;                 // integrator states (+0x17c / +0x180)
    static constexpr double SCALE = 65536.0;

    void prepare (double s) noexcept { sr = s > 0.0 ? s : 44100.0; reset(); }
    void reset()           noexcept { s1 = s2 = 0; }

    static inline int32_t mh (int32_t a, int32_t b) noexcept { return (int32_t) (((int64_t) a * b) >> 32); }
    static inline int32_t sl (int32_t v, int n)     noexcept { return (int32_t) ((uint32_t) v << n); }
    static inline int32_t ad (int32_t a, int32_t b) noexcept { return (int32_t) ((uint32_t) a + (uint32_t) b); }
    static inline int32_t sb (int32_t a, int32_t b) noexcept { return (int32_t) ((uint32_t) a - (uint32_t) b); }
    static inline int32_t cubic (int32_t x) noexcept { return sb (x, mh (sl (x, 3), mh (sl (x, 4), sl (x, 4)))); }

    // tap: 0 = LP, 1 = BP, 2 = HP
    double process (int tap, double in, double fc, double reso) noexcept
    {
        const int ci = std::clamp ((int) std::lround (1024.0 * std::log (std::clamp (fc, 1.0, sr * 0.49)) / 10.24), 0, 1023);
        const int ri = std::clamp (((int) std::lround (std::clamp (reso, 0.0, 1.0) * 255.0)) >> 2, 0, 63);
        const int32_t coefA = VAZTypeDrealT::kCutA[ci];
        const int32_t coefC = std::min (VAZTypeDrealT::kResC[ri], VAZTypeDrealT::kCutRlim[ci]);
        const int32_t coefD = VAZTypeDrealT::kResD[ri];

        const int32_t input = (int32_t) std::lround (std::clamp (in, -2.0, 2.0) * SCALE);
        int32_t tmp = ad (mh (sl (s1, 2), coefA), s2);                                 // LP (pre)
        int32_t hp  = sb (sb (mh (sl (input, 1), coefD), mh (sl (s1, 2), coefC)), tmp); // HP (pre)
        const int32_t i9 = ad (cubic (s1), mh (sl (hp, 2), coefA));
        const int32_t b  = ad (mh (sl (i9, 2), coefA), tmp);
        const int32_t lp = ad (tmp, b);                                                // LP tap (+0x170)
        s2 = b;
        hp = ad (hp, sb (sb (mh (sl (input, 1), coefD), mh (sl (i9, 2), coefC)), b));   // HP tap (+0x178)
        s1 = ad (mh (sl (sb (sb (mh (sl (input, 1), coefD), mh (sl (i9, 2), coefC)), b), 2), coefA), cubic (i9));
        const int32_t bp = ad (s1, i9);                                                // BP tap (+0x174)
        const int32_t out = (tap == 0) ? lp : (tap == 1) ? bp : hp;
        return (double) out / SCALE;
    }
};
