// VAZTypeDreal.h — BIT-EXACT VAZ Type-D (the REAL D filter): a 2-stage resonant SVF with cubic distortion.
// VAZ's handler @0x4ddaa8 for .v2p 10-13 (internal 0x30-0x34) — vaz_big.c:1305-1391. (NOT the 0x6d67 SVF the clone
// once mislabelled "VAZTypeD" — that is VAZ's K.) Coef tables (0x6d45/55/65/66) runtime-dumped → VAZTypeDrealTables.h.
// One SVF section (stepCi) per sample: coefA=kCutA[cut]; coefC=min(kResC[reso],kCutRlim[cut]); coefD=kResD[reso];
//   tmp = mulhi(s1<<2,coefA)+s2 ; hp = mulhi(in<<1,coefD) − mulhi(s1<<2,coefC) − tmp ; i9 = cubic(s1)+mulhi(hp<<2,coefA)
//   b = mulhi(i9<<2,coefA)+tmp ; s2=b ; LP=tmp+b ; hp += mulhi(in<<1,coefD) − mulhi(i9<<2,coefC) − b
//   s1 = mulhi(hp2<<2,coefA)+cubic(i9) ; BP = s1+i9 ; out = {LP,BP,HP}[tap]     cubic(x)=x−mulhi(x<<3,mulhi(x<<4,x<<4))
// modes 0x30/0x31/0x32 = LP/BP/HP (tap = mode&3). mode 0x34 (D HP+LP Separation, vaz_big.c:1289-1346) = TWO cascaded
// sections: section-1 (states s1b/s2b) at cutoff+Sep outputs its HP → section-2 (states s1/s2) at cutoff−Sep → LP.
#pragma once
#include <cstdint>
#include <cmath>
#include <algorithm>
#include "VAZTypeDrealTables.h"

struct VAZTypeDreal
{
    double  sr = 44100.0;
    int32_t s1 = 0, s2 = 0;                 // main section states (+0x17c / +0x180)
    int32_t s1b = 0, s2b = 0;               // section-1 states for the HP+LP cascade (+0x184 / +0x188)
    static constexpr double SCALE = 65536.0;

    void prepare (double s) noexcept { sr = s > 0.0 ? s : 44100.0; reset(); }
    void reset()           noexcept { s1 = s2 = s1b = s2b = 0; }

    static inline int32_t mh (int32_t a, int32_t b) noexcept { return (int32_t) (((int64_t) a * b) >> 32); }
    static inline int32_t sl (int32_t v, int n)     noexcept { return (int32_t) ((uint32_t) v << n); }
    static inline int32_t ad (int32_t a, int32_t b) noexcept { return (int32_t) ((uint32_t) a + (uint32_t) b); }
    static inline int32_t sb (int32_t a, int32_t b) noexcept { return (int32_t) ((uint32_t) a - (uint32_t) b); }
    static inline int32_t cubic (int32_t x) noexcept { return sb (x, mh (sl (x, 3), mh (sl (x, 4), sl (x, 4)))); }

    inline int cutIdx (double fc) const noexcept
    { return std::clamp ((int) std::lround (1024.0 * std::log (std::clamp (fc, 1.0, sr * 0.49)) / 10.24), 0, 1023); }
    static inline int resoIdx (double reso) noexcept
    { return std::clamp (((int) std::lround (std::clamp (reso, 0.0, 1.0) * 255.0)) >> 2, 0, 63); }

    // One SVF section at cutoff index ci / reso index ri, states st1/st2. Returns tap 0=LP / 1=BP / 2=HP.
    inline int32_t stepCi (int tap, int32_t input, int ci, int ri, int32_t& st1, int32_t& st2) noexcept
    {
        const int32_t coefA = VAZTypeDrealT::kCutA[ci];
        const int32_t coefC = std::min (VAZTypeDrealT::kResC[ri], VAZTypeDrealT::kCutRlim[ci]);
        const int32_t coefD = VAZTypeDrealT::kResD[ri];
        const int32_t tmp = ad (mh (sl (st1, 2), coefA), st2);                            // LP (pre)
        int32_t hp = sb (sb (mh (sl (input, 1), coefD), mh (sl (st1, 2), coefC)), tmp);   // HP (pre)
        const int32_t i9 = ad (cubic (st1), mh (sl (hp, 2), coefA));
        const int32_t b  = ad (mh (sl (i9, 2), coefA), tmp);
        const int32_t lp = ad (tmp, b);                                                   // LP tap (+0x170)
        st2 = b;
        const int32_t hp2 = sb (sb (mh (sl (input, 1), coefD), mh (sl (i9, 2), coefC)), b);
        hp = ad (hp, hp2);                                                                // HP tap (+0x178)
        st1 = ad (mh (sl (hp2, 2), coefA), cubic (i9));
        const int32_t bp = ad (st1, i9);                                                  // BP tap (+0x174)
        return (tap == 0) ? lp : (tap == 1) ? bp : hp;
    }

    // tap: 0 = LP, 1 = BP, 2 = HP (modes 0x30/0x31/0x32).
    double process (int tap, double in, double fc, double reso) noexcept
    {
        const int32_t input = (int32_t) std::lround (std::clamp (in, -2.0, 2.0) * SCALE);
        return (double) stepCi (tap, input, cutIdx (fc), resoIdx (reso), s1, s2) / SCALE;
    }

    // mode 0x34 — D HP+LP Separation: section-1 HP at cutoff+Sep → section-2 LP at cutoff−Sep. sepNorm = Separation 0..1.
    double processHPLP (double in, double fc, double reso, double sepNorm) noexcept
    {
        const int ci = cutIdx (fc), ri = resoIdx (reso);
        const int sep = std::max (3, ((int) std::lround (std::clamp (sepNorm, 0.0, 1.0) * 255.0)) * 2);   // (sepParam<<15)>>14 = ·2, min 3
        const int ciU = std::clamp (ci + sep, 0, 0x3ff), ciL = std::clamp (ci - sep, 0, 0x3ff);
        const int32_t input = (int32_t) std::lround (std::clamp (in, -2.0, 2.0) * SCALE);
        const int32_t mid = stepCi (2, input, ciU, ri, s1b, s2b);    // section 1: cut+Sep, HP tap → feeds section 2
        const int32_t out = stepCi (0, mid,   ciL, ri, s1,  s2);     // section 2: cut−Sep, LP tap
        return (double) out / SCALE;
    }
};
