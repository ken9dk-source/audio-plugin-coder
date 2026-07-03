// VazDecimatorEngine.h — fixed-point port of VAZ 2010 TFXDecimator render FUN_0051dbcc @0x51dbcc.
// Pure C++/no JUCE so the oracle can include it and diff against an independent reference transcription.
//
// Per sample: (1) rate accumulator [+0x268] += rate[+0x26c]; when >0x7ff → sample-&-hold (11-bit → SR reduction).
//   (2) BITCRUSH by truncation: held = (in & mask[+0x270]) + bias[+0x274].  (3) output DC-blocker (removes the
//   rounding-bias DC): out = (coef[+0x280]·(state+held)·16)>>32 ; state = out − held.
// Param maps (exact, integer): FUN_0051dd44  shift=24−bits, mask=(0xFFFFFFFF>>shift)<<shift, bias=((1<<shift)−1)>>1;
//   FUN_0051dd14  rate=(srParam+1)·192000/SR (0x2ee00).  coef (FUN_0051dc7c) is 80-bit x87 → not MSVC-reproducible;
//   the render algebra fixes DC-gain=0 for any coef<2^28, so it is a DC-blocker — corner set from SR (documented).
#pragma once
#include <cstdint>
#include <cmath>

struct VazDecimatorEngine
{
    int32_t  rate = 2048, mask = -1, bias = 0, coef = 0x0FFF0000;   // +0x26c / +0x270 / +0x274 / +0x280
    int32_t  acc = 0, heldL = 0, heldR = 0, stateL = 0, stateR = 0; // +0x268 / +0x278 / +0x27c / +0x284 / +0x288

    void reset () noexcept { acc = 0; heldL = heldR = stateL = stateR = 0; }

    inline void processFrame (int32_t& L, int32_t& R) noexcept
    {
        acc = (acc & 0x7ff) + rate;                    // 11-bit rate accumulator
        if (acc > 0x7ff)                               // sample-&-hold + bitcrush
        {
            heldL = (L & mask) + bias;
            heldR = (R & mask) + bias;
        }
        // out = (coef · ((state+held)·16)) >> 32 ; state = out − held   (·16 wraps in 32-bit, like the binary)
        const int32_t oL = (int32_t) (((int64_t) coef * (int64_t) (int32_t) ((uint32_t) (stateL + heldL) << 4)) >> 32);
        L = oL; stateL = oL - heldL;
        const int32_t oR = (int32_t) (((int64_t) coef * (int64_t) (int32_t) ((uint32_t) (stateR + heldR) << 4)) >> 32);
        R = oR; stateR = oR - heldR;
    }

    // srParam 0..255 (VAZ default 0xff), bits 1..24 (VAZ default 0x10 = 16). sr = host sample rate.
    void setParams (double sr, int srParam, int bits) noexcept
    {
        if (bits < 1)  bits = 1;
        if (bits > 24) bits = 24;
        const int shift = 24 - bits;                                   // FUN_0051dd44
        mask = (int32_t) ((0xFFFFFFFFu >> (shift & 31)) << (shift & 31));
        bias = (int32_t) ((((1u << (shift & 31)) - 1u)) >> 1);         // half-LSB rounding bias
        rate = (int32_t) (((int64_t) (srParam + 1) * 192000) / (int64_t) sr);   // FUN_0051dd14: (p+1)·0x2ee00/SR
        if (rate < 1) rate = 1;
        // DC-blocker coef (80-bit residual): pole ≈ 2^28·e^(−2π·fc/sr), fc≈20 Hz → removes bias DC, else transparent.
        coef = (int32_t) std::llround ((double) 0x10000000 * std::exp (-2.0 * 3.14159265358979323846 * 20.0 / sr));
    }
};
