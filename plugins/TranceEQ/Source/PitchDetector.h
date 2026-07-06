#pragma once
//==============================================================================
// PitchDetector.h — compact YIN pitch detector for audio-based key-tracking.
// (de Cheveigné & Kawahara, 2002 — the algorithm the PDF names for "følgende EQ".)
// Lives on the audio thread. Uses a circular buffer so feeding it is O(1)/sample;
// the YIN analysis runs once every `hop` samples. Returns 0 when the input has no
// clear pitch (silence / noise).
//==============================================================================
#include <vector>
#include <cmath>
#include <algorithm>

namespace teq
{
    class PitchDetector
    {
    public:
        void prepare (double sampleRate)
        {
            sr = sampleRate;
            ring.assign    ((size_t) N, 0.0f);
            scratch.assign ((size_t) N, 0.0f);
            diff.assign    ((size_t) (N / 2), 0.0f);
            cmnd.assign    ((size_t) (N / 2), 1.0);
            w = filled = sinceLast = 0;
            lastF0 = 0.0;
        }

        void reset() noexcept
        {
            std::fill (ring.begin(), ring.end(), 0.0f);
            w = filled = sinceLast = 0;
            lastF0 = 0.0;
        }

        // Accumulate mono samples; YIN fires whenever `hop` fresh samples have arrived.
        void pushBlock (const float* x, int n) noexcept
        {
            for (int i = 0; i < n; ++i)
            {
                ring[(size_t) w] = x[i];
                w = (w + 1) % N;
                if (filled < N) ++filled;
                if (++sinceLast >= hop && filled >= N) { sinceLast = 0; detect(); }
            }
        }

        double getF0() const noexcept { return lastF0; }   // Hz, or 0 if unvoiced

    private:
        void detect() noexcept
        {
            // Linearise the ring (oldest sample sits at index w when full).
            for (int i = 0; i < N; ++i) scratch[(size_t) i] = ring[(size_t) ((w + i) % N)];

            const int W = N / 2;   // integration window & max lag

            double energy = 0.0;
            for (int i = 0; i < W; ++i) energy += (double) scratch[(size_t) i] * scratch[(size_t) i];
            if (energy < 1.0e-4) { lastF0 = 0.0; return; }

            diff[0] = 0.0f;
            for (int tau = 1; tau < W; ++tau)
            {
                double sum = 0.0;
                for (int i = 0; i < W; ++i)
                {
                    const double d = (double) scratch[(size_t) i] - (double) scratch[(size_t) (i + tau)];
                    sum += d * d;
                }
                diff[(size_t) tau] = (float) sum;
            }

            double running = 0.0;
            cmnd[0] = 1.0;
            for (int tau = 1; tau < W; ++tau)
            {
                running += (double) diff[(size_t) tau];
                cmnd[(size_t) tau] = (running > 0.0) ? (double) diff[(size_t) tau] * tau / running : 1.0;
            }

            const double threshold = 0.15;
            const int tauMin = std::max (2, (int) (sr / 1500.0));   // ceiling ~1500 Hz
            const int tauMax = std::min (W - 1, (int) (sr / 40.0)); // floor   ~40 Hz
            int tauEst = -1;
            for (int tau = tauMin; tau <= tauMax; ++tau)
            {
                if (cmnd[(size_t) tau] < threshold)
                {
                    while (tau + 1 <= tauMax && cmnd[(size_t) (tau + 1)] < cmnd[(size_t) tau]) ++tau;
                    tauEst = tau; break;
                }
            }
            if (tauEst < 0)
            {
                int best = tauMin; double bv = cmnd[(size_t) tauMin];
                for (int tau = tauMin + 1; tau <= tauMax; ++tau)
                    if (cmnd[(size_t) tau] < bv) { bv = cmnd[(size_t) tau]; best = tau; }
                if (bv > 0.30) { lastF0 = 0.0; return; }
                tauEst = best;
            }

            double betterTau = (double) tauEst;
            if (tauEst > tauMin && tauEst < tauMax)
            {
                const double s0 = cmnd[(size_t) (tauEst - 1)];
                const double s1 = cmnd[(size_t)  tauEst];
                const double s2 = cmnd[(size_t) (tauEst + 1)];
                const double denom = 2.0 * (2.0 * s1 - s2 - s0);
                if (std::abs (denom) > 1.0e-12) betterTau += (s2 - s0) / denom;
            }

            lastF0 = (betterTau > 0.0) ? sr / betterTau : 0.0;
        }

        static constexpr int N   = 2048;
        static constexpr int hop = 1024;

        double sr = 44100.0;
        std::vector<float>  ring, scratch, diff;
        std::vector<double> cmnd;
        int w = 0, filled = 0, sinceLast = 0;
        double lastF0 = 0.0;
    };
}
