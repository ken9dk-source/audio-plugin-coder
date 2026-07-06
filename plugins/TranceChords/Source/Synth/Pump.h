#pragma once
#include <cmath>

//==============================================================================
// Sidechain-style "pump" envelope + a gentle master soft-clip. Pure functions so
// they can be unit-tested in the render harness.
//==============================================================================
namespace tc
{
    // Ducking gain for a position within the beat: frac 0 = just after the (virtual)
    // kick -> most ducked; frac 1 = fully recovered. depth 0..1.
    inline float pumpGain (float frac, float depth) noexcept
    {
        frac  = frac < 0.0f ? 0.0f : (frac > 1.0f ? 1.0f : frac);
        depth = depth < 0.0f ? 0.0f : (depth > 1.0f ? 1.0f : depth);
        const float rise = 1.0f - (1.0f - frac) * (1.0f - frac) * std::sqrt (1.0f - frac); // ~(1-x)^2.5
        return 1.0f - depth * (1.0f - rise);
    }

    // Transparent below the knee, soft (tanh) above -> safe master ceiling.
    inline float softClip (float x) noexcept
    {
        const float t = 0.9f;
        if (x >  t) return  t + (1.0f - t) * std::tanh ((x - t) / (1.0f - t));
        if (x < -t) return -t + (1.0f - t) * std::tanh ((x + t) / (1.0f - t));
        return x;
    }
}
