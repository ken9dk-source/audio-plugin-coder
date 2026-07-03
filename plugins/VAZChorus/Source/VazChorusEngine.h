// VazChorusEngine.h — fixed-point port of VAZ 2010 TFXChorus render FUN_00518ad8 @0x518ad8.
// Pure C++/no JUCE so the oracle can include it and diff against an independent reference transcription.
//
// ONE shared circular delay line, input = (L+R)>>1 (mono). Two LFOs (phase +0x284/+0x28c, inc +0x288/+0x290),
// each producing 3 phase-shifted (0°/120°/240°) modulation values that ADD into 3 accumulators → 3 COMBINED taps
// (not 6). Each tap read at base+(mod>>16) with linear interp (frac=(mod&0xffff)<<15). Stereo = lrPhase·(tapC−tapB).
// Output filter: out = (gain·(±stereo + mono3 − in·4))>>32 + in.  All integer; mirrors the decompile.
//   Waveform mode [+0x264]/[+0x270]:  0 = sine LUT (+0x2a8, 256-entry; 80-bit-built → residual)
//     · 1 = trapezoid clamp(|ph|−2^29,0,2^30) @0x518BA4 · 2 = triangle |ph|>>1 @0x518C1F.
//   Base delay (FUN_00518fbc): base = round((sr·50/256000)·(delay+1)) = round((delay+1)/5.12·sr/1000).
#pragma once
#include <cstdint>
#include <cstring>
#include <cmath>

struct VazChorusEngine
{
    static constexpr int kBuf = 8192;                 // covers 50 ms + mod up to 96 kHz
    static constexpr int kMask = kBuf - 1;
    int32_t  buf[kBuf];
    int32_t  sineLut[257];                            // +0x2a8 (256 sine entries; [256]=[0] for interp wrap)
    uint32_t wpos = 0;                                // +0x294
    uint32_t ph1 = 0, ph2 = 0;                        // +0x284 / +0x28c
    // per-block params:
    uint32_t inc1 = 0, inc2 = 0;                      // +0x288 / +0x290
    int32_t  depth = 0, level = 0, level2 = 0;        // +0x26c / +0x298 / +0x278
    int32_t  base = 0, lrPhase = 0, gain = 0;         // +0x29c / +0x27c / +0x280
    int      mode1 = 2, mode2 = 2;                    // +0x264 / +0x270

    void clearBuffers () noexcept { std::memset (buf, 0, sizeof buf); wpos = 0; ph1 = 0; ph2 = 0x80000000u; }
    void buildSineLut () noexcept                     // 80-bit-built in VAZ → double approx (documented residual)
    { for (int i = 0; i < 257; ++i) sineLut[i] = (int32_t) std::llround (2147483647.0 * std::sin (2.0 * 3.14159265358979323846 * (i & 255) / 256.0)); }

    // 3 phase-shifted modulation values for one LFO, ADDED onto (m0,m1,m2). scale = depth·level (or level2·level).
    inline void lfo3add (uint32_t ph, int mode, int32_t scale, int32_t& m0, int32_t& m1, int32_t& m2) const noexcept
    {
        if (mode == 0)                                // sine LUT: index steps of 0x55 (85/256≈120°), shared frac
        {
            const uint32_t i0 = ph >> 24;
            const int32_t  frac = (int32_t) ((ph & 0xffffff) << 7);
            auto tap = [&] (uint32_t i) -> int32_t {
                i &= 0xff; const int32_t a = sineLut[i], b = sineLut[i + 1];
                const int32_t s = a + (int32_t) (((int64_t) ((int64_t) (b - a) * 2) * (int64_t) frac) >> 32);
                return (int32_t) (((int64_t) scale * (int64_t) s) >> 32);
            };
            m0 += tap (i0); m1 += tap (i0 + 0x55); m2 += tap (i0 + 0xaa);
        }
        else if (mode == 1)                           // trapezoid: phase steps of 0x55555554 (120°)
        {
            auto tap = [&] (uint32_t p) -> int32_t {
                const int32_t sgn = (int32_t) p >> 31;
                int32_t t = (int32_t) (((int32_t) p ^ sgn) - sgn) - 0x20000000;
                if (t < 0) t = 0; if (t > 0x40000000) t = 0x40000000;
                return (int32_t) (((int64_t) t * (int64_t) scale) >> 32);
            };
            m0 += tap (ph); m1 += tap (ph + 0x55555554u); m2 += tap (ph + 0xaaaaaaacu);
        }
        else                                          // triangle |ph|>>1
        {
            auto tap = [&] (uint32_t p) -> int32_t {
                const int32_t sgn = (int32_t) p >> 31;
                const int32_t t = ((int32_t) p ^ sgn) - sgn;
                return (int32_t) (((int64_t) (t >> 1) * (int64_t) scale) >> 32);
            };
            m0 += tap (ph); m1 += tap (ph + 0x55555554u); m2 += tap (ph + 0xaaaaaaacu);
        }
    }

    inline void processFrame (int32_t& L, int32_t& R) noexcept
    {
        ph1 += inc1; ph2 += inc2;
        int32_t m28 = 0, m24 = 0, m20 = 0;            // local_28 / local_24 / local_20 (tapA/C/B mods)
        const int32_t s1 = depth * level;             // iVar5 (LFO1) = depth·level (32-bit product @0x518B12)
        lfo3add (ph1, mode1, s1, m28, m24, m20);
        if (level2 > 0) { const int32_t s2 = level2 * level; lfo3add (ph2, mode2, s2, m28, m24, m20); }  // LFO2 adds

        const int32_t inL = L, inR = R;
        wpos = (wpos - 1) & (uint32_t) kMask;         // decrement write pos (+0x294)
        // 3 combined taps at base + (mod>>16), linear interp with frac=(mod&0xffff)<<15:
        auto readTap = [&] (int32_t mod) -> int32_t {
            const uint32_t idx = ((uint32_t) (mod >> 16) + (uint32_t) base + wpos) & (uint32_t) kMask;
            const int32_t  cur = buf[idx];
            const int32_t  frac = (int32_t) ((mod & 0xffff) << 15);
            return cur + (int32_t) (((int64_t) frac * (int64_t) ((buf[(idx + 1) & kMask] - cur) * 2)) >> 32);
        };
        const int32_t tA = readTap (m28);
        const int32_t tC = readTap (m20);
        const int32_t tB = readTap (m24);
        const int32_t mono3 = tA + tC + tB;                                            // sum of 3 taps
        const int32_t stereo = (int32_t) (((int64_t) (lrPhase << 22) * (int64_t) ((tC - tB) * 4)) >> 32);
        buf[wpos] = (inL + inR) >> 1;                                                   // write mono input
        L = (int32_t) (((int64_t) (gain << 22) * (int64_t) (stereo + mono3 + inL * -4)) >> 32) + (inL * 4 >> 2);
        R = (int32_t) (((int64_t) (gain << 22) * (int64_t) ((mono3 - stereo) + inR * -4)) >> 32) + (inR * 4 >> 2);
    }
};
