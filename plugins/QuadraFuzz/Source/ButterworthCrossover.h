#pragma once
#include <cmath>
#include <complex>
#include <algorithm>

// Crossover filter sections for the QuadraFuzz clone.
//
// GROUND TRUTH (from decompiling QuadraFuzz.dll): the filterbank is NOT a
// Linkwitz-Riley tree. Each of the 4 bands is an INDEPENDENT band-pass filter
// (FUN_1000a5f0 sets every band's type byte = 3 = BP; FUN_1000a8f0 designs each
// one separately). The analog prototype is a BUTTERWORTH of order N = 4
// (FUN_10009d50 passes order 4 to the ctor; FUN_1000b040's even/odd guard keeps
// 4 even), with poles cos((N-1+2k)*PI/2N) (FUN_1000b420). Coefficients come from
// a bilinear transform with tan() pre-warping (FUN_1000b4b0), recomputed per SR.
//
// => Use BW4 (a true 4th-order Butterworth, section Q's 0.541 & 1.307), NOT LR4
//    (which forces both sections to Q=0.707 and crosses at -6 dB instead of -3).
//    A clean per-band shape fit of the capture confirmed BW4 (err 10.6) beats
//    LR4 (14.2) and BW3 (16.1). LR4 below is retained only for reference.
//
// Transposed-Direct-Form-II biquads, stereo state.
namespace QFX
{
    static constexpr double PI       = 3.14159265358979323846;
    static constexpr double BUTTER_Q = 0.70710678118654752;   // 1/sqrt(2)

    struct Biquad
    {
        double b0 = 1, b1 = 0, b2 = 0, a1 = 0, a2 = 0;
        double z1[2] = { 0, 0 }, z2[2] = { 0, 0 };

        inline float process (float x, int ch) noexcept
        {
            const double y = b0 * x + z1[ch];
            z1[ch] = b1 * x - a1 * y + z2[ch];
            z2[ch] = b2 * x - a2 * y;
            return (float) y;
        }
        void reset() noexcept { z1[0] = z1[1] = z2[0] = z2[1] = 0; }
    };

    inline void biquadLP (Biquad& bq, double fc, double sr, double Q) noexcept
    {
        const double w = 2.0 * PI * fc / sr, cw = std::cos (w), al = std::sin (w) / (2.0 * Q);
        const double a0 = 1.0 + al;
        bq.b0 = ((1.0 - cw) * 0.5) / a0; bq.b1 = (1.0 - cw) / a0; bq.b2 = bq.b0;
        bq.a1 = (-2.0 * cw) / a0;        bq.a2 = (1.0 - al) / a0;
    }
    inline void biquadHP (Biquad& bq, double fc, double sr, double Q) noexcept
    {
        const double w = 2.0 * PI * fc / sr, cw = std::cos (w), al = std::sin (w) / (2.0 * Q);
        const double a0 = 1.0 + al;
        bq.b0 = ((1.0 + cw) * 0.5) / a0; bq.b1 = (-(1.0 + cw)) / a0; bq.b2 = bq.b0;
        bq.a1 = (-2.0 * cw) / a0;         bq.a2 = (1.0 - al) / a0;
    }

    // 4th-order Linkwitz-Riley = two cascaded identical 2nd-order Butterworth
    // sections (Q = 1/sqrt2). LP/HP at the same fc cross at -6 dB and sum flat.
    struct LR4
    {
        Biquad s1, s2;
        void reset() noexcept { s1.reset(); s2.reset(); }
        inline float process (float x, int ch) noexcept { return s2.process (s1.process (x, ch), ch); }

        void designLP (double fc, double sr) noexcept
        {
            biquadLP (s1, fc, sr, BUTTER_Q);
            biquadLP (s2, fc, sr, BUTTER_Q);
        }
        void designHP (double fc, double sr) noexcept
        {
            biquadHP (s1, fc, sr, BUTTER_Q);
            biquadHP (s2, fc, sr, BUTTER_Q);
        }
    };

    // 4th-order BUTTERWORTH = two cascaded 2nd-order sections with the Butterworth
    // pole-pair Q's (NOT equal Q like LR4). This is what QuadraFuzz.dll actually
    // uses. Section Q_k = 1 / (2 cos(theta_k)), theta = (2k-1)*PI/(2N), N = 4:
    //   k=1: theta = PI/8  (22.5 deg) -> Q = 0.5411961
    //   k=2: theta = 3PI/8 (67.5 deg) -> Q = 1.3065630
    // LP+HP at the same fc cross at -3 dB (Butterworth), with the slight pass-band
    // edge shape of the Q=1.307 section — audibly different from LR4's -6 dB.
    struct BW4
    {
        static constexpr double Q1 = 0.54119610014619698;   // 1/(2 cos 22.5deg)
        static constexpr double Q2 = 1.30656296487637652;   // 1/(2 cos 67.5deg)
        Biquad s1, s2;
        void reset() noexcept { s1.reset(); s2.reset(); }
        inline float process (float x, int ch) noexcept { return s2.process (s1.process (x, ch), ch); }

        void designLP (double fc, double sr) noexcept
        {
            biquadLP (s1, fc, sr, Q1);
            biquadLP (s2, fc, sr, Q2);
        }
        void designHP (double fc, double sr) noexcept
        {
            biquadHP (s1, fc, sr, Q1);
            biquadHP (s2, fc, sr, Q2);
        }
    };

    // 4th-order Butterworth BANDPASS — the original QuadraFuzz.dll's actual band
    // realization (FUN_1000b4b0 case 3): a 4th-order Butterworth LP prototype put
    // through the analog LP->BP transform  s_lp -> (s^2 + w0^2)/(BW*s)  with tan()
    // pre-warping, then bilinear. That doubles the order -> 8th-order BP = 4
    // biquads. This is phase-identical to the DLL (unlike a HP4.LP4 cascade, which
    // matches in magnitude but not phase). Designed from (loFreq, hiFreq) in Hz.
    struct BW_BP
    {
        Biquad sec[4];
        void reset() noexcept { sec[0].reset(); sec[1].reset(); sec[2].reset(); sec[3].reset(); }
        inline float process (float x, int ch) noexcept
        {
            x = sec[0].process (x, ch); x = sec[1].process (x, ch);
            x = sec[2].process (x, ch); x = sec[3].process (x, ch);
            return x;
        }

        void design (double flo, double fhi, double sr) noexcept
        {
            using cd = std::complex<double>;
            // EXACT DLL design (QuadraFuzz.dll FUN_1000a8f0): the LOW edge is
            // scaled by 1.15 (const _DAT_100219b0) before a *standard* geometric
            // Butterworth BP; edges are first clamped to SR*0.48 (lo, _DAT_100219c0)
            // and SR*0.49 (hi, _DAT_100219b8). This reproduces the DLL's dumped
            // biquad coeffs to 0.000 dB on every band incl. near-Nyquist band3, and
            // replaces an earlier fit (w0^2=1.1497*Wlo*Whi, BW=0.9665*dW) which was
            // this scaling's small-angle approximation and drifted ~1% near Nyquist.
            const double loS = std::min (flo, sr * 0.48) * 1.15;   // _DAT_100219b0
            const double hiS = std::min (fhi, sr * 0.49);          // _DAT_100219b8
            const double Wlo = std::tan (PI * loS / sr);           // pre-warped edges
            const double Whi = std::tan (PI * hiS / sr);
            const double w02 = Wlo * Whi;                          // standard geometric
            const double BW  = Whi - Wlo;
            // the two Butterworth-4 LP pole-pairs (|pole| = 1, cutoff = 1 rad/s)
            const double Q4[2] = { 0.54119610014619698, 1.30656296487637652 };
            int bi = 0;
            for (int k = 0; k < 2; ++k)
            {
                const double re = -1.0 / (2.0 * Q4[k]);
                const double im = std::sqrt (1.0 - re * re);
                const cd p (re, im);                       // upper LP pole of the pair
                const cd bc = -p * BW;                     // s^2 + bc*s + w02 = 0
                const cd disc = std::sqrt (bc * bc - 4.0 * w02);
                const cd roots[2] = { (-bc + disc) * 0.5, (-bc - disc) * 0.5 };
                for (const cd r : roots)                   // each root -> one real biquad (r, r*)
                {
                    const double a1 = -2.0 * r.real();     // analog: s^2 + a1*s + a0
                    const double a0 = std::norm (r);
                    const double d0 = 1.0 + a1 + a0;        // bilinear denom[0]
                    Biquad& q = sec[bi++];
                    q.b0 =  BW / d0; q.b1 = 0.0; q.b2 = -BW / d0;          // BW*(1 - z^-2)
                    q.a1 = (-2.0 + 2.0 * a0) / d0;
                    q.a2 = ( 1.0 - a1  + a0) / d0;
                }
            }
            // normalise the cascade to 0 dB at the (pre-warped) geometric centre
            const double wd = 2.0 * std::atan (std::sqrt (w02));
            const cd z1 = std::exp (cd (0.0, -wd)), z2 = z1 * z1;
            cd H (1.0, 0.0);
            for (int i = 0; i < 4; ++i)
                H *= (sec[i].b0 + sec[i].b1 * z1 + sec[i].b2 * z2)
                   / (1.0        + sec[i].a1 * z1 + sec[i].a2 * z2);
            const double g = std::pow (1.0 / std::abs (H), 0.25);
            for (int i = 0; i < 4; ++i) { sec[i].b0 *= g; sec[i].b1 *= g; sec[i].b2 *= g; }
        }
    };
}
