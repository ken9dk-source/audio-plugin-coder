// VAZTypeD.h - BIT-EXACT VAZ Type-D filter (state-variable / Chamberlin SVF).
// Ported from Vaz2010Core.dll (disasm @0x4DDE5C-0x4DDF3F): a 2x-oversampled 2-pole SVF.
// Per pass: resoFB = clamp(bandpass<<4 * resoGainCoef, +-0x1000000); hp = in - resoFB - lp;
// lp += coefA*(hp<<2); bp += coefA*((lp + resoFB - bp)<<2). coefA=0x6d67e8[cutIdx] (integrator),
// resoGainCoef = coefB(0x6d77e8[cutIdx])*reso. Output = avg of the 2 sub-steps' tap (0=LP,1=HP,2=BP).
// No post-HP (the SVF tap is the output). Integer arithmetic verbatim.
//
// (This engine is also VAZ's K family — .v2p 15/16 route here; filter_k proves process(tap 2) == the K
//  SVF handler 0x4ddcfe.) mode 0x44 (K HP+LP, .v2p 16) prepends a LINEAR HP pre-section at a SEPARATE
//  cutoff/reso (vaz_big.c:1394-1451): HP cutoff = param[0x270]*4 (= aux, byte<<2), HP reso = param[0x274]
//  (= hpHz byte); its 2x-averaged HP output feeds the main K section at the normal cut/reso. See processHPLP.
#pragma once
#include <cstdint>
#include <cmath>
#include <algorithm>
#include "VAZTypeDTables.h"

struct VAZTypeD
{
    double  sr = 44100.0;
    int32_t lp = 0, bp = 0;                 // main section states (0x17c / 0x180)
    int32_t h1 = 0, h2 = 0, h3 = 0;         // mode-0x44 HP pre-section states (0x184 / 0x188 / 0x18c = stored hp)
    static constexpr double  SCALE = 65536.0;
    static constexpr int32_t CHI = 0x1000000, CLO = (int32_t) 0xff000000;   // resoFB clamp +-16.7M

    void prepare (double s) noexcept { sr = s > 0.0 ? s : 44100.0; reset(); }
    void reset()           noexcept { lp = bp = 0; h1 = h2 = h3 = 0; }

    static inline int32_t mulhi (int32_t a, int32_t b) noexcept { return (int32_t) (((int64_t) a * b) >> 32); }
    static inline int32_t shl   (int32_t v, int n)     noexcept { return (int32_t) ((uint32_t) v << n); }
    static inline int32_t sub32 (int32_t a, int32_t b) noexcept { return (int32_t) ((uint32_t) a - (uint32_t) b); }
    static inline int32_t add32 (int32_t a, int32_t b) noexcept { return (int32_t) ((uint32_t) a + (uint32_t) b); }

    inline int cutIdx (double fc) const noexcept
    { return std::clamp ((int) std::lround (1024.0 * std::log (std::clamp (fc, 1.0, sr * 0.49)) / 10.24), 0, 1023); }
    static inline int32_t resoGain (int32_t coefB, double reso) noexcept   // (coefB * (r255<<22))>>32 << 2
    { return shl (mulhi (coefB, shl ((int) std::lround (std::clamp (reso, 0.0, 1.0) * 255.0), 22)), 2); }

    // 2x-oversampled main K/SVF section on a raw int32 input (no input clamp). tap 0=LP,1=HP,2=BP. Returns averaged int.
    inline int32_t mainCoreInt (int tap, int32_t inp, int32_t coefA, int32_t rgc) noexcept
    {
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
        return (int32_t) (((int64_t) v0 + v1) >> 1);
    }

    // tap: 0 = lowpass, 1 = highpass, 2 = bandpass
    double process (int tap, double in, double fc, double reso) noexcept
    {
        const int ci = cutIdx (fc);
        const int32_t inp = (int32_t) std::lround (std::clamp (in, -2.0, 2.0) * SCALE);
        return (double) mainCoreInt (tap, inp, VAZTypeDT::kCoefA[ci], resoGain (VAZTypeDT::kCoefB[ci], reso)) / SCALE;
    }

    // mode 0x44 — K HP+LP: a LINEAR 2x-oversampled HP pre-section (own cutoff/reso, states h1/h2/h3) feeds the
    // main K section at the normal cut/reso. hpCutNorm = HP cutoff 0..1 (param[0x270]=aux, linear byte<<2 index);
    // hpResoNorm = HP resonance 0..1 (param[0x274]=hpHz byte). (vaz_big.c:1394-1451. mod term 0x290/0x294 omitted.)
    double processHPLP (double in, double fc, double reso, double hpCutNorm, double hpResoNorm) noexcept
    {
        const int hci = std::clamp (((int) std::lround (std::clamp (hpCutNorm, 0.0, 1.0) * 255.0)) << 2, 0, 0x3ff);
        const int32_t hCoefA = VAZTypeDT::kCoefA[hci];
        const int32_t hRgc   = resoGain (VAZTypeDT::kCoefB[hci], hpResoNorm);

        const int32_t inp = (int32_t) std::lround (std::clamp (in, -2.0, 2.0) * SCALE);
        // --- HP pre-section, 2x oversample. h1=sA(0x184), h2=sB(0x188), h3=sC(0x18c, prev-sample hp) ---
        int32_t q  = std::clamp (mulhi (shl (h3, 4), hRgc), CLO, CHI);                       // pass1 reso: from prev-sample hp
        h1 = add32 (h1, mulhi (shl (sub32 (sub32 (0, h1), q), 2), hCoefA));                  // sA += A*((-sA - q)<<2)
        h2 = add32 (h2, mulhi (shl (sub32 (sub32 (add32 (h1, q), h2), inp), 2), hCoefA));    // sB += A*(((sA+q) - sB - in)<<2)
        const int32_t hp1 = add32 (h2, inp);
        q  = std::clamp (mulhi (shl (hp1, 4), hRgc), CLO, CHI);                              // pass2 reso: from this-sample hp1
        h1 = add32 (h1, mulhi (shl (sub32 (sub32 (0, h1), q), 2), hCoefA));
        h2 = add32 (h2, mulhi (shl (sub32 (sub32 (add32 (h1, q), h2), inp), 2), hCoefA));
        const int32_t hp2 = add32 (h2, inp);
        h3 = hp2;                                                                            // store for next sample
        const int32_t preOut = add32 (hp2, hp1) >> 1;                                        // (hp2 + hp1) >> 1

        const int ci = cutIdx (fc);
        return (double) mainCoreInt (2, preOut, VAZTypeDT::kCoefA[ci], resoGain (VAZTypeDT::kCoefB[ci], reso)) / SCALE;
    }
};
