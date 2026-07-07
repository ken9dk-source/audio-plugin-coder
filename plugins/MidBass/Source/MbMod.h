#pragma once
//==============================================================================
// MidBass modulation — Phase 4. LFOs (adapted from TranceAcid's verified ta::Lfo)
// + tempo-sync division table + 6-slot mod matrix.
//
// Retrigger origins are DEFINED and tested per shape (first sample after
// retrigger()): Sine → 0 rising · Triangle → +1 (falling) · Saw → +1 (falling
// ramp) · Square → +1 · S&H → a freshly drawn value. The step-shaped waves
// (Square, S&H) pass through a ~2 ms one-pole slew so routing them to cutoff or
// volume cannot click (Phase 4 condition d); the slew state SNAPS to the raw
// value on retrigger (the note transient masks it, and steady-state steps are
// what matter). Continuous shapes bypass the slew entirely.
//==============================================================================
#include <cmath>
#include <cstdint>
#include <algorithm>

namespace mb
{
namespace LfoWave { enum { Sine = 0, Triangle, Saw, Square, SH }; }

// Division indices match Params.h syncDivs:
//   { 1/1, 1/2, 1/4, 1/8, 1/16, 1/32, 1/4T, 1/8T, 1/16T, 1/4., 1/8., 1/16. }
// beats-per-cycle: straight = 4/denominator·4 … dotted = x1.5, triplet = x2/3.
inline double syncBeats (int divIdx)
{
    switch (divIdx)
    {
        case 0:  return 4.0;            // 1/1
        case 1:  return 2.0;            // 1/2
        case 2:  return 1.0;            // 1/4
        case 3:  return 0.5;            // 1/8
        case 4:  return 0.25;           // 1/16
        case 5:  return 0.125;          // 1/32
        case 6:  return 2.0 / 3.0;      // 1/4T
        case 7:  return 1.0 / 3.0;      // 1/8T
        case 8:  return 1.0 / 6.0;      // 1/16T
        case 9:  return 1.5;            // 1/4.
        case 10: return 0.75;           // 1/8.
        default: return 0.375;          // 1/16.
    }
}
inline double syncHz (double bpm, int divIdx)
{
    return (bpm / 60.0) / syncBeats (divIdx);
}

//==============================================================================
struct MbLfo
{
    // Step-shape smoothing (condition d). 5 ms measured: at 2 ms the slewed steps
    // still burst ~-45 dBFS at 6 kHz through volume/cutoff routes (gate is -60 dB
    // over the isolation baseline); 5 ms passes with margin and is inaudible as
    // lag against even an 8 Hz square (62 ms half-period).
    static constexpr double kSlewSeconds = 0.005;

    double   sr = 44100.0, phase = 0.0, inc = 0.0, curHz = 0.0;
    int      wave = LfoWave::Sine;
    float    shVal = 0.0f, slewState = 0.0f;
    float    slewCoef = 1.0f;
    uint32_t rng = 0x9E3779B9u;

    void prepare (double sampleRate, uint32_t seed)
    {
        sr = sampleRate > 0.0 ? sampleRate : 44100.0;
        rng = seed != 0 ? seed : 1;
        slewCoef = 1.0f - (float) std::exp (-1.0 / (kSlewSeconds * sr));
        phase = 0.0; slewState = 0.0f;
    }

    void setWave (int w)       { wave = w; }
    void setRateHz (double hz) { curHz = hz; inc = hz / sr; }

    float draw()               // xorshift32 → [-1, 1)
    {
        rng ^= rng << 13; rng ^= rng >> 17; rng ^= rng << 5;
        return (float) ((int32_t) rng) * (1.0f / 2147483648.0f);
    }

    float raw() const
    {
        const float p = (float) phase;
        switch (wave)
        {
            case LfoWave::Sine:     return std::sin (6.2831853f * p);
            case LfoWave::Triangle: return 2.0f * std::fabs (2.0f * p - 1.0f) - 1.0f;
            case LfoWave::Saw:      return 1.0f - 2.0f * p;
            case LfoWave::Square:   return (p < 0.5f) ? 1.0f : -1.0f;
            default:                return shVal;
        }
    }

    void retrigger()
    {
        phase = 0.0;
        if (wave == LfoWave::SH) shVal = draw();
        slewState = raw();                         // snap: no stale-slew ramp into the new note
    }

    float process()
    {
        const float v = raw();
        phase += inc;
        if (phase >= 1.0) { phase -= 1.0; shVal = draw(); }

        if (wave == LfoWave::Square || wave == LfoWave::SH)
        {
            slewState += (v - slewState) * slewCoef;
            return slewState;
        }
        return v;
    }
};

//==============================================================================
// 6-slot matrix. Orders MUST match Params.h modSrcs/modDsts choice arrays.
namespace ModSrc { enum { Off = 0, Velocity, ModWheel, Aftertouch, FilterEnv, LFO1, LFO2, Count }; }
namespace ModDst { enum { Cutoff = 0, Reso, Pitch, PWM, Amp, Drive, Count }; }

struct ModMatrix6
{
    struct Slot { int src = 0, dst = 0; float amt = 0.0f; };   // amt bipolar -1..+1
    Slot slot[6];

    // Sources SUM into each destination; the VOICE clamps the summed result at
    // the parameter's legal bounds (Phase 4 condition c: clamp, never wrap).
    void apply (const float* srcVals, float* dstVals) const
    {
        for (const auto& s : slot)
            if (s.src > 0 && s.amt != 0.0f)
                dstVals[s.dst] += srcVals[s.src] * s.amt;
    }
};
} // namespace mb
