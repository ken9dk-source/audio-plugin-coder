// VazPhaserEngine.h — fixed-point port of VAZ 2010 TFXPhaser render FUN_005218d8 @0x5218d8.
// Pure C++/no JUCE so the oracle can include it and diff against an independent reference transcription.
//
// N-stage 1st-order all-pass chain (N = (stagesParam+1)·2, FUN_00521b68), coef from a 512-entry EXACT LUT
// (runtime-dumped: vaz_phaser_coef_lut.h, Q30). LFO (triangle = |phase|) modulates the LUT index. Feedback
// coef = param<<23 clamped ±0.78125 (FUN_00521bf4). Linear dry/wet (mix<<22 → wet/256). Stereo = separate
// all-pass banks with the R LFO phase offset (phase + lrPhase·−2^24). 100% integer Q30; mirrors the decompile.
#pragma once
#include <cstdint>
#include <cstring>
#include <cmath>
#include "../../VAZClone/reference/vaz_phaser_coef_lut.h"
#include "../../VAZClone/reference/vaz_phaser_gain_lut.h"
#include "../../VAZClone/reference/vaz_phaser_rate_lut.h"

struct VazPhaserEngine
{
    static constexpr int kMaxStages = 12;                 // banks are 12 ints (stride +0x2b0 → +0x2e0)
    int32_t  coefLut[512];                                // SR-adjusted from kPhaserCoefLUT (built in setSampleRate)
    int32_t  apL[kMaxStages] = {0}, apR[kMaxStages] = {0};
    int32_t  fbL = 0, fbR = 0;                            // feedback states (+0x2a4 / +0x2a8)
    uint32_t phase = 0;                                   // LFO phase accumulator (+0x290)
    // per-block params:
    uint32_t inc = 0;                                     // +0x294 (LFO rate; 80-bit → approximate)
    int32_t  depth = 128, center = 96, numStages = 4;     // +0x280 / +0x264 / +0x2a0
    int32_t  fbGain = 0, inGain = 0x40000000, mix = 0;    // +0x29c (Q30) / +0x298 (Q30 gain, LUT) / +0x288 (0..255)
    int32_t  lrPhase = 0;                                 // +0x284

    void clearBuffers () noexcept { std::memset (apL, 0, sizeof apL); std::memset (apR, 0, sizeof apR); fbL = fbR = 0; phase = 0; }
    void setSampleRate (double sr) noexcept               // coef = 2^30 − (2^30 − coef44k)·(44100/sr)  (exact linear sr-adjust)
    {
        const double adj = 44100.0 / sr;
        for (int i = 0; i < 512; ++i)
        {
            int64_t c = (int64_t) vazfx::kPhaserCoefLUT[i];
            if (sr != 44100.0) c = (int64_t) std::llround ((double) 0x40000000 - ((double) 0x40000000 - (double) c) * adj);
            if (c < 0) c = 0; if (c > 0x40000000) c = 0x40000000;
            coefLut[i] = (int32_t) c;
        }
    }

    static inline int32_t sQ30 (int32_t v, int32_t c) noexcept   // (v<<2 · c) >> 32  = v·c/2^30 (32-bit v<<2 wraps like the binary)
    { return (int32_t) (((int64_t) (int32_t) ((uint32_t) v << 2) * (int64_t) c) >> 32); }

    inline int32_t idxOf (uint32_t ph) const noexcept     // idx = ((|ph|>>16)·depth + center·0x8000) >> 15, clamped [0,511]
    {
        const int32_t s = (int32_t) ph >> 31;
        const int32_t ab = ((int32_t) ph ^ s) - s;
        int32_t idx = ((ab >> 16) * depth + center * 0x8000) >> 15;
        if (idx < 0) idx = 0; if (idx > 511) idx = 511;   // LUT bound (VAZ relies on params; clamp = safety)
        return idx;
    }

    inline int32_t chan (int32_t rawIn, uint32_t ph, int32_t* ap, int32_t& fb) noexcept
    {
        const int32_t coef = coefLut[idxOf (ph)];
        const int32_t in   = sQ30 (rawIn, inGain);                    // input · inGain (Q30)
        int32_t x = in + sQ30 (fb, fbGain);                          // + feedback
        for (int n = numStages - 1; ; --n)                          // N all-pass stages, buf[N-1]..buf[0]
        {
            const int32_t y = ap[n] - sQ30 (x, coef);               // y = buf − coef·x
            ap[n] = sQ30 (y, coef) + x;                             // buf = coef·y + x
            x = y;
            if (n == 0) break;
        }
        fb = x;                                                      // last stage output → feedback
        // linear dry/wet: out = in + (mix<<22 · (apOut−in)·4) >> 32  = in + (apOut−in)·mix/256
        return in + (int32_t) (((int64_t) ((uint32_t) mix << 22)
                              * (int64_t) (int32_t) ((uint32_t) (x - in) << 2)) >> 32);
    }

    inline void processFrame (int32_t& L, int32_t& R) noexcept
    {
        phase += inc;                                                // advance LFO (L uses phase; R offset below)
        L = chan (L, phase, apL, fbL);
        R = chan (R, phase + (uint32_t) (lrPhase * -0x1000000), apR, fbR);   // R LFO phase offset (+0x284)
    }

    // ── param mapping ────────────────────────────────────────────────────────────────────────────────────
    // stages (FUN_00521b68): N = (stagesParam+1)·2 → 2..12.  feedback (FUN_00521bf4): fbGain = clamp(p,±100)<<23
    //   (Q30 → max ±0.78125).  depth[+0x280]/center[+0x264]/lrPhase[+0x284]/mix[+0x288] = raw params (0..255).
    //   gain[+0x298] (FUN_00521d44): inGain = kPhaserGainLUT[gainByte] = 2^((b−255)/60)·2^30 = −25.6dB(0)…0dB(255),
    //   default b=225 → −3dB; applied to the INPUT (dry+wet scale equally). rate/inc = 80-bit (approximate).
    void setParams (int stagesParam, int fbParam, bool fbInvert, int depthP, int centerP, int lrP, int mixP, int gainByte, uint32_t incPhase) noexcept
    {
        if (stagesParam < 0) stagesParam = 0; if (stagesParam > 5) stagesParam = 5;
        numStages = (stagesParam + 1) * 2;
        int fp = fbParam; if (fp > 100) fp = 100; if (fp < 0) fp = 0;
        fbGain  = (fbInvert ? -fp : fp) << 23;                       // param<<23, ±100 clamp = ±0.78125 Q30
        depth   = depthP  < 0 ? 0 : depthP  > 255 ? 255 : depthP;
        center  = centerP < 0 ? 0 : centerP > 511 ? 511 : centerP;
        lrPhase = lrP;
        mix     = mixP < 0 ? 0 : mixP > 255 ? 255 : mixP;
        gainByte = gainByte < 0 ? 0 : gainByte > 255 ? 255 : gainByte;
        inGain  = (int32_t) vazfx::kPhaserGainLUT[gainByte];         // exact dumped gain curve (Q30, replaces linear)
        inc     = incPhase;
    }
};
