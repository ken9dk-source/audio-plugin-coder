// Stability sanity-check for the bit-exact Type-A/B filter port (no WAV — bounded/NaN check only).
#define _USE_MATH_DEFINES
#include <cmath>
#include "../plugins/VAZClone/Source/VAZTypeB.h"
#include <cstdio>
int main()
{
    VAZTypeB f; f.prepare (48000.0);
    int fails = 0;
    const char* tn[] = { "LP", "BP", "HP" };
    for (int tap = 0; tap < 3; ++tap)
        for (int row : { 0, 128, 255 })          // resonance-row (A: reso, B: 255-bw)
            for (double cut : { 120.0, 1000.0, 6000.0 })
            {
                f.reset();
                double maxOut = 0.0, sum = 0.0;
                for (int i = 0; i < 48000; ++i)
                {
                    const double in = 0.5 * std::sin (2.0 * M_PI * 220.0 * i / 48000.0);
                    const double o  = f.process (tap, in, cut, 128, row);   // mixReso=128
                    if (std::isnan (o) || std::isinf (o) || std::fabs (o) > 1e6)
                    { std::printf ("BLOWUP tap=%s row=%d cut=%.0f i=%d o=%g\n", tn[tap], row, cut, i, o); fails++; break; }
                    maxOut = std::fmax (maxOut, std::fabs (o)); sum += o * o;
                }
                if (cut == 1000.0)
                    std::printf ("tap=%s row=%-3d cut=%5.0f -> max=%6.3f rms=%.4f\n", tn[tap], row, cut, maxOut, std::sqrt (sum / 48000.0));
            }
    std::printf (fails ? "\nFAIL: %d blow-ups\n" : "\nOK: stable across the grid\n", fails);
    return fails ? 1 : 0;
}
