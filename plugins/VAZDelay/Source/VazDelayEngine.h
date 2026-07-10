// VazDelayEngine.h — fixed-point port of VAZ 2010 TFXDelay render FUN_0051bba8 @0x51bba8.
// Pure C++/no JUCE so the oracle can include it and diff against an independent reference transcription.
//
// Stereo circular delay (interleaved int32 L,R). One-pole damping LP in each feedback path. Dry/wet per channel.
// 3 modes [+0x260]:  0 = Stereo (independent L/R self-feedback) · 1 = Ping-Pong (CROSS feedback: L←tapR, R←tapL)
//   · ≥2 = serial Double (mono-in → L-delay → R-delay → dual-mono out).  100% integer; mirrors the decompile.
// Q-formats:  feedback used as (tap<<2 · fb<<22)>>32 = tap·fb/256  (fb 0..255) ; damping (Δ<<4 · coef)>>32 (coef Q28) ;
//   dry/wet (v<<2 · g)>>32 = v·g/2^30 (Q30).  Buffer = next_pow2(sr·2550/1000), min 32768 frames (FUN_0051bf78).
#pragma once
#include <cstdint>
#include <vector>
#include <cmath>
#include "../../VAZClone/reference/vaz_delay_damp_lut.h"   // EXACT runtime-dumped tone→damp curve (Q28, sr=44100)

struct VazDelayEngine
{
    std::vector<int32_t> buf;                 // interleaved: buf[f*2]=L, buf[f*2+1]=R  (+0x2e0)
    int32_t  mask = 0;                        // +0x2dc  (frames − 1, pow2)
    uint32_t wpos = 0;                        // +0x2c8
    int      mode = 0;                        // +0x260
    int32_t  delayL = 1, delayR = 1;          // +0x2cc / +0x2d0  (tap offsets, frames)
    int32_t  fbL = 0, fbR = 0;                // +0x27c / +0x298  (raw param, used <<22 → /256)
    int32_t  dampL = 0, dampR = 0;            // +0x2a8 / +0x2ac  (Q28 one-pole coef)
    int32_t  stateL = 0, stateR = 0;          // +0x2b0 / +0x2b4  (damping states)
    int32_t  dryL = 0, dryR = 0;              // +0x2c0 / +0x2c4  (Q30)
    int32_t  wetL = 0, wetR = 0;              // +0x2b8 / +0x2bc  (Q30)

    int32_t  dampLut[256];                    // SR-adjusted tone→damp (built in prepare)
    int      samplesPerMs = 44;               // [+0x2d4] = SR/1000 (FUN_0051bf78); used by the delay-time map

    void prepare (double sr)                  // buffer sizing (FUN_0051bf78): next pow2 ≥ sr·2550/1000, min 0x8000
    {
        int need = (int) ((double) sr * 2550.0 / 1000.0);
        int n = 0x8000; while (n < need) n <<= 1;
        buf.assign ((size_t) n * 2, 0);
        mask = n - 1; wpos = 0; stateL = stateR = 0;
        samplesPerMs = (int) sr / 1000;       // integer, exactly as VAZ
        const double adj = 44100.0 / sr;      // k_sr = 2^28·(k_44k/2^28)^(44100/sr)
        for (int i = 0; i < 256; ++i)
        {
            const double k44 = (double) vazfx::kDelayDampLUT[i] / (double) 0x10000000;
            const double k   = (sr == 44100.0) ? k44 : std::pow (k44, adj);
            dampLut[i] = (int32_t) std::llround (k * (double) 0x10000000);
        }
    }
    // Delay-time (FREE): VAZ delayL = ms·(SR/1000) + ((SR/1000)>>4)  (LINEAR in ms; FUN_0051c1cc @0x51c1cc).
    int32_t delaySamplesFromMs (double ms) const noexcept
    { return (int32_t) std::llround (ms * (double) samplesPerMs) + (samplesPerMs >> 4); }
    void setTone (int toneL, int toneR) noexcept
    { dampL = dampLut[toneL < 0 ? 0 : toneL > 255 ? 255 : toneL]; dampR = dampLut[toneR < 0 ? 0 : toneR > 255 ? 255 : toneR]; }
    void reset () { std::fill (buf.begin(), buf.end(), 0); wpos = 0; stateL = stateR = 0; }

    static inline int32_t mh (int32_t a, int32_t b) noexcept { return (int32_t) (((int64_t) a * (int64_t) b) >> 32); }
    static inline int32_t sh (int32_t v, int n) noexcept { return (int32_t) ((uint32_t) v << n); }   // 32-bit shift (wraps)
    inline int32_t damp (int32_t& st, int32_t coef, int32_t x) noexcept   // x + ((st−x)·16 · coef)>>32 ; st = result
    { const int32_t r = mh (sh (st - x, 4), coef) + x; st = r; return r; }

    inline void processFrame (int32_t& L, int32_t& R) noexcept
    {
        wpos = (wpos - 1) & (uint32_t) mask;
        const int32_t inL = L, inR = R;
        const int32_t tapL = buf[(size_t) (((uint32_t) delayL + wpos) & (uint32_t) mask) * 2 + 0];
        const int32_t tapR = buf[(size_t) (((uint32_t) delayR + wpos) & (uint32_t) mask) * 2 + 1];

        if (mode == 1)                        // Ping-Pong: cross feedback
        {
            int32_t wL = damp (stateL, dampL, inL + mh (sh (tapR, 2), sh (fbL, 22)));
            buf[(size_t) wpos * 2 + 0] = wL;
            L = mh (dryL, sh (inL, 2)) + mh (sh (tapL, 2), wetL);
            int32_t wR = damp (stateR, dampR, inR + mh (sh (tapL, 2), sh (fbR, 22)));
            buf[(size_t) wpos * 2 + 1] = wR;
            R = mh (dryR, sh (inR, 2)) + mh (sh (tapR, 2), wetR);
        }
        else if (mode < 2)                    // Stereo: independent self-feedback
        {
            int32_t wL = damp (stateL, dampL, inL + mh (sh (tapL, 2), sh (fbL, 22)));
            buf[(size_t) wpos * 2 + 0] = wL;
            L = mh (dryL, sh (inL, 2)) + mh (sh (tapL, 2), wetL);
            int32_t wR = damp (stateR, dampR, inR + mh (sh (tapR, 2), sh (fbR, 22)));
            buf[(size_t) wpos * 2 + 1] = wR;
            R = mh (dryR, sh (inR, 2)) + mh (sh (tapR, 2), wetR);
        }
        else                                  // serial Double: mono → L-delay → R-delay → dual-mono
        {
            const int32_t monoIn = inL + inR;
            int32_t wL = damp (stateL, dampL, monoIn + mh (sh (tapL, 2), sh (fbL, 22)));
            buf[(size_t) wpos * 2 + 0] = wL;
            const int32_t mid = mh (dryL, sh (monoIn, 2)) + mh (sh (tapL, 2), wetL);
            int32_t wR = damp (stateR, dampR, mid + mh (sh (tapR, 2), sh (fbR, 22)));
            buf[(size_t) wpos * 2 + 1] = wR;
            const int32_t out = mh (dryR, sh (mid, 2)) + mh (sh (tapR, 2), wetR);
            L = out; R = out;
        }
    }
};
