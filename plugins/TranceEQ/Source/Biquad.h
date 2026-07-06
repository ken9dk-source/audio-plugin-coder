#pragma once
//==============================================================================
// Biquad.h — RBJ Audio-EQ-Cookbook biquad (the reference design named in the PDF).
// Transposed-Direct-Form-II, double-precision state. One Biquad == one band == one channel.
//
// The exact same coefficient formulas are mirrored in the WebView (index.html, makeCoeffs())
// so the on-screen EQ curve is a pixel-accurate match of what the audio path actually does.
//==============================================================================
#include <cmath>

namespace teq
{
    inline constexpr double kPi = 3.14159265358979323846;   // M_PI isn't guaranteed by the C++ standard

    enum class FilterType { HPF = 0, LowShelf, Peak, Notch, HighShelf, LPF };

    // a0-normalised biquad coefficients.
    struct Coeffs { double b0 = 1, b1 = 0, b2 = 0, a1 = 0, a2 = 0; };

    // Compute RBJ cookbook coefficients. freq in Hz, gain in dB (used by Peak/shelves), Q > 0.
    inline Coeffs makeCoeffs (FilterType type, double freqHz, double gainDb, double q, double sampleRate) noexcept
    {
        // Guard against silly inputs that would blow up the math.
        const double nyq = 0.5 * sampleRate;
        if (freqHz < 10.0)        freqHz = 10.0;
        if (freqHz > nyq * 0.999) freqHz = nyq * 0.999;
        if (q < 0.05)             q = 0.05;

        const double w0    = 2.0 * kPi * freqHz / sampleRate;
        const double cosw0 = std::cos (w0);
        const double sinw0 = std::sin (w0);
        const double alpha = sinw0 / (2.0 * q);
        const double A     = std::pow (10.0, gainDb / 40.0);   // shelf/peak amplitude
        const double sqA   = std::sqrt (A);

        double b0 = 1, b1 = 0, b2 = 0, a0 = 1, a1 = 0, a2 = 0;

        switch (type)
        {
            case FilterType::LPF:
                b0 = (1.0 - cosw0) * 0.5; b1 = 1.0 - cosw0; b2 = b0;
                a0 = 1.0 + alpha;         a1 = -2.0 * cosw0; a2 = 1.0 - alpha;
                break;

            case FilterType::HPF:
                b0 = (1.0 + cosw0) * 0.5; b1 = -(1.0 + cosw0); b2 = b0;
                a0 = 1.0 + alpha;         a1 = -2.0 * cosw0;   a2 = 1.0 - alpha;
                break;

            case FilterType::Notch:
                b0 = 1.0;        b1 = -2.0 * cosw0; b2 = 1.0;
                a0 = 1.0 + alpha; a1 = -2.0 * cosw0; a2 = 1.0 - alpha;
                break;

            case FilterType::Peak:
                b0 = 1.0 + alpha * A; b1 = -2.0 * cosw0; b2 = 1.0 - alpha * A;
                a0 = 1.0 + alpha / A; a1 = -2.0 * cosw0; a2 = 1.0 - alpha / A;
                break;

            case FilterType::LowShelf:
                b0 =      A * ((A + 1.0) - (A - 1.0) * cosw0 + 2.0 * sqA * alpha);
                b1 =  2.0 * A * ((A - 1.0) - (A + 1.0) * cosw0);
                b2 =      A * ((A + 1.0) - (A - 1.0) * cosw0 - 2.0 * sqA * alpha);
                a0 =          (A + 1.0) + (A - 1.0) * cosw0 + 2.0 * sqA * alpha;
                a1 = -2.0 *   ((A - 1.0) + (A + 1.0) * cosw0);
                a2 =          (A + 1.0) + (A - 1.0) * cosw0 - 2.0 * sqA * alpha;
                break;

            case FilterType::HighShelf:
                b0 =      A * ((A + 1.0) + (A - 1.0) * cosw0 + 2.0 * sqA * alpha);
                b1 = -2.0 * A * ((A - 1.0) + (A + 1.0) * cosw0);
                b2 =      A * ((A + 1.0) + (A - 1.0) * cosw0 - 2.0 * sqA * alpha);
                a0 =          (A + 1.0) - (A - 1.0) * cosw0 + 2.0 * sqA * alpha;
                a1 =  2.0 *   ((A - 1.0) - (A + 1.0) * cosw0);
                a2 =          (A + 1.0) - (A - 1.0) * cosw0 - 2.0 * sqA * alpha;
                break;
        }

        const double inv = 1.0 / a0;
        return { b0 * inv, b1 * inv, b2 * inv, a1 * inv, a2 * inv };
    }

    // Single-channel TDF-II biquad. Coefficients are swapped in atomically per block.
    struct Biquad
    {
        Coeffs c;
        double z1 = 0.0, z2 = 0.0;   // state

        void reset() noexcept { z1 = z2 = 0.0; }
        void setCoeffs (const Coeffs& nc) noexcept { c = nc; }

        inline float process (float in) noexcept
        {
            const double x = (double) in;
            const double y = c.b0 * x + z1;
            z1 = c.b1 * x - c.a1 * y + z2;
            z2 = c.b2 * x - c.a2 * y;
            return (float) y;
        }
    };
}
