// vaz_detune.h — faithful port of VAZ's deterministic detune spread FUN_004e0618 @0x4e0618
// (decomp: tools/vaz_prims.c:3316-3388). Poly and Unison branches only — the clone's voice modes
// are exclusive, so VAZ's combined poly×unison branch (:3389+) is not needed. Tables from vaz_constants.h.
//   POLY   (uniN==1): scale = kDetuneScale[polyN] * polyAmt, then s3 = scale>>3   (vaz_prims.c:3324-3327)
//   UNISON (polyN==1): s = (uniAmt<<9) / uniN   — NO table, linear                (vaz_prims.c:3360)
// Both: center = trunc(-(N-1)*s / 2); order cursor starts at kDetuneOrder==((N*3)>>2), walks &0x1f to
// the next entry < N; per voice  off = kDetuneOrder[cur]*s + center; if(off<0) off+=0x3ff; out = off>>10.
#pragma once
#include <cstdint>
#include "vaz_constants.h"

namespace vazref
{
    // Poly-mode spread. out[0..polyN-1] = per-voice detune offset (≈ cents). vaz_prims.c:3323-3354.
    inline void detunePoly (int polyN, int polyAmt, int32_t* out)
    {
        if (polyN <= 1) { if (polyN == 1) out[0] = 0; return; }
        const int n = polyN < 32 ? polyN : 31;                 // kDetuneScale has 32 entries [0..31]
        int32_t s8 = kDetuneScale[n] * polyAmt;                // :3324
        if (s8 < 0) s8 += 7;                                   // :3326 (round toward 0 for the >>3)
        const int32_t s3 = s8 >> 3;                            // :3324/3328 uses iVar8>>3
        const int32_t center = (int32_t) (-((int32_t) (polyN - 1) * s3)) / 2;   // :3328-3332 trunc toward 0
        int cur = 0; const int start = (polyN * 3) >> 2;       // :3333-3337
        while (kDetuneOrder[cur] != start) ++cur;
        for (int i = 0; i < polyN; ++i)
        {
            int32_t off = kDetuneOrder[cur] * s3 + center;     // :3343
            if (off < 0) off += 0x3ff;                         // :3344-3346
            out[i] = off >> 10;                                // :3347
            do { cur = (cur + 1) & 0x1f; } while (kDetuneOrder[cur] >= polyN);   // :3348-3350
        }
    }

    // Unison-mode spread. out[0..uniN-1]. vaz_prims.c:3359-3386. (No kDetuneScale — s = (uniAmt<<9)/uniN.)
    inline void detuneUnison (int uniN, int uniAmt, int32_t* out)
    {
        if (uniN <= 1) { if (uniN == 1) out[0] = 0; return; }
        const int32_t s = ((int32_t) uniAmt << 9) / uniN;      // :3360
        const int32_t center = (int32_t) (-((int32_t) (uniN - 1) * s)) / 2;     // :3361-3365 trunc toward 0
        int cur = 0; const int start = (uniN * 3) >> 2;        // :3366-3371
        while (kDetuneOrder[cur] != start) ++cur;
        for (int i = 0; i < uniN; ++i)
        {
            int32_t off = kDetuneOrder[cur] * s + center;      // :3376
            if (off < 0) off += 0x3ff;                         // :3377-3379
            out[i] = off >> 10;                                // :3380
            do { cur = (cur + 1) & 0x1f; } while (kDetuneOrder[cur] >= uniN);    // :3381-3383
        }
    }
}
