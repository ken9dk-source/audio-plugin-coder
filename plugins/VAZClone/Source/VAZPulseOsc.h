// VAZPulseOsc.h — BIT-EXACT VAZ band-limited (BLEP) pulse oscillator.
// Transcribes VAZ's difference-of-two-band-limited-ramps from Vaz2010Core.dll (vaz_big.c:206-241):
//   iv8   = freq-dependent BLEP transition-width index = max(0, ((inc>>8)+(inc>>0xb) - 0xe000)) >> 0xd
//   iVar9 = clamped duty-edge position = clamp(pw>>1, (iv8+1)*0x2000, 0x800000 - (iv8+1)*0x2000)   (pw = WaveShape<<16)
//   ramp1 (edge @ phase 0) and ramp2 (edge @ iVar9) are each band-limited near their edge by the step tables
//   kStepRise (DAT_006dd2c0) / kStepFall (DAT_006de2c0) indexed by iv8; out = (2*iVar9 + ramp1) - (ramp2 + 0x800000).
// The `(hi<<0xd)|(lo>>0x13)` fixed-point recombine == a signed >>19 of the 64-bit product. Phase is a pure uint32
// accumulator (advanced by the caller). Output scaled by 1/0x800000 → ±1.0, matching the other clone oscillators.
#pragma once
#include <cstdint>
#include "VAZPulseTables.h"

namespace VAZPulseOsc
{
    inline int32_t rc (int64_t v) noexcept { return (int32_t) (v >> 19); }

    // uVar11 = the (already-advanced) 32-bit phase; inc = phase increment; pw = WaveShape byte << 16.
    inline double render (uint32_t uVar11, uint32_t inc, int32_t pw) noexcept
    {
        const int32_t iVar19 = (int32_t) uVar11 >> 8;
        int32_t pre = ((int32_t) inc >> 8) + ((int32_t) inc >> 0xb) - 0xe000;
        if (pre < 0) pre = 0;
        int iv8 = pre >> 0xd;
        if (iv8 > VAZPulseT::kN - 1) iv8 = VAZPulseT::kN - 1;               // defensive (VAZ has no clamp; mip bounds it)

        int32_t iVar15 = (iv8 + 1) * 0x2000;
        int32_t iVar9  = pw >> 1;
        if (iVar9 < iVar15) iVar9 = iVar15;
        const int32_t hi = (iv8 + 1) * -0x2000 + 0x800000;
        if (hi < iVar9) iVar9 = hi;
        iVar15 = iVar15 - 0x800000;

        const int32_t iVar16 = (int32_t) (iVar9 * 0x200 - (int32_t) uVar11) >> 8;
        const int32_t iVar17 = -iVar19;

        const int32_t r1 = (-iVar15 == iVar19 || iVar17 < iVar15)
            ? rc ((int64_t) (iVar17 + 0x800000) * VAZPulseT::kStepRise[iv8])
            : rc ((int64_t) (iVar17 - 0x800000) * VAZPulseT::kStepFall[iv8]);
        const int32_t r2 = (iVar15 < iVar16)
            ? rc ((int64_t) (iVar16 - 0x800000) * VAZPulseT::kStepFall[iv8])
            : rc ((int64_t) (iVar16 + 0x800000) * VAZPulseT::kStepRise[iv8]);

        const int32_t out = (iVar9 * 2 + r1) - (r2 + 0x800000);
        return (double) out / (double) 0x800000;
    }
}
