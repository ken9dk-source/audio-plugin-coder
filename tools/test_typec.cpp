// Stability sanity-check for the bit-exact Type-C filter port (no WAV, no VAZ comparison —
// just confirms the integer recurrence is bounded / doesn't blow up or NaN across the range).
#define _USE_MATH_DEFINES
#include <cmath>
#include "../plugins/VAZClone/Source/VAZTypeC.h"
#include <cstdio>
#include <cmath>
int main()
{
    VAZTypeC f; f.prepare (48000.0);
    int fails = 0;
    for (double reso : { 0.0, 0.5, 0.9, 1.0 })
        for (double cut : { 120.0, 1000.0, 6000.0 })
            for (double sep : { 0.0, 0.5, 1.0 })
                for (int poles : { 2, 4 })
                {
                    f.reset();
                    double maxOut = 0.0, sum = 0.0;
                    for (int i = 0; i < 48000; ++i)
                    {
                        const double in = 0.5 * std::sin (2.0 * M_PI * 220.0 * i / 48000.0);
                        const double o  = f.process (poles, in, cut, reso, sep, 0.3);
                        if (std::isnan (o) || std::isinf (o) || std::abs (o) > 1e6)
                        { std::printf ("BLOWUP poles=%d cut=%.0f reso=%.1f sep=%.1f i=%d o=%g\n", poles, cut, reso, sep, i, o); fails++; break; }
                        maxOut = std::fmax (maxOut, std::fabs (o)); sum += o * o;
                    }
                    if (poles == 4 && sep == 0.5)
                        std::printf ("poles=%d cut=%5.0f reso=%.1f sep=%.1f -> max=%6.3f rms=%.4f\n",
                                     poles, cut, reso, sep, maxOut, std::sqrt (sum / 48000.0));
                }
    std::printf (fails ? "\nFAIL: %d blow-ups\n" : "\nOK: stable across the whole grid (no NaN/Inf/blow-up)\n", fails);
    return fails ? 1 : 0;
}
