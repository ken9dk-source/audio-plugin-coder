#pragma once
//==============================================================================
// Shared test helpers for the TranceEQ suite.
//  - An INDEPENDENT re-transcription of the RBJ cookbook (so a typo in either the
//    plugin's Biquad.h or here is caught by the comparison test).
//  - Analytic biquad magnitude, pole-stability check.
//  - Headless render helpers (impulse response, FFT magnitude, RMS, null).
//==============================================================================
#include <juce_dsp/juce_dsp.h>
#include <juce_audio_processors/juce_audio_processors.h>
#include <cmath>
#include <vector>
#include "../Source/Biquad.h"

namespace tt
{
    inline constexpr double kPi = 3.14159265358979323846;

    struct RefCoeffs { double b0, b1, b2, a1, a2; };

    // Independent RBJ Audio-EQ-Cookbook reference (a0-normalised).
    inline RefCoeffs rbjReference (teq::FilterType type, double f, double gainDb, double Q, double fs)
    {
        using FT = teq::FilterType;
        const double A  = std::pow (10.0, gainDb / 40.0);
        const double w0 = 2.0 * kPi * f / fs;
        const double cw = std::cos (w0), sw = std::sin (w0);
        const double alpha = sw / (2.0 * Q);
        const double ss = 2.0 * std::sqrt (A) * alpha;

        double b0 = 1, b1 = 0, b2 = 0, a0 = 1, a1 = 0, a2 = 0;
        switch (type)
        {
            case FT::LPF:   b0 = (1 - cw) / 2; b1 = 1 - cw;    b2 = (1 - cw) / 2; a0 = 1 + alpha; a1 = -2 * cw; a2 = 1 - alpha; break;
            case FT::HPF:   b0 = (1 + cw) / 2; b1 = -(1 + cw); b2 = (1 + cw) / 2; a0 = 1 + alpha; a1 = -2 * cw; a2 = 1 - alpha; break;
            case FT::Notch: b0 = 1;            b1 = -2 * cw;   b2 = 1;            a0 = 1 + alpha; a1 = -2 * cw; a2 = 1 - alpha; break;
            case FT::Peak:  b0 = 1 + alpha * A; b1 = -2 * cw;  b2 = 1 - alpha * A; a0 = 1 + alpha / A; a1 = -2 * cw; a2 = 1 - alpha / A; break;
            case FT::LowShelf:
                b0 = A * ((A + 1) - (A - 1) * cw + ss);
                b1 = 2 * A * ((A - 1) - (A + 1) * cw);
                b2 = A * ((A + 1) - (A - 1) * cw - ss);
                a0 = (A + 1) + (A - 1) * cw + ss;
                a1 = -2 * ((A - 1) + (A + 1) * cw);
                a2 = (A + 1) + (A - 1) * cw - ss;
                break;
            case FT::HighShelf:
                b0 = A * ((A + 1) + (A - 1) * cw + ss);
                b1 = -2 * A * ((A - 1) + (A + 1) * cw);
                b2 = A * ((A + 1) + (A - 1) * cw - ss);
                a0 = (A + 1) - (A - 1) * cw + ss;
                a1 = 2 * ((A - 1) - (A + 1) * cw);
                a2 = (A + 1) - (A - 1) * cw - ss;
                break;
        }
        const double inv = 1.0 / a0;
        return { b0 * inv, b1 * inv, b2 * inv, a1 * inv, a2 * inv };
    }

    // Analytic |H(e^jω)| in dB for a normalised biquad.
    inline double biquadMagDb (const teq::Coeffs& c, double f, double fs)
    {
        const double w = 2 * kPi * f / fs, cw = std::cos (w), sw = std::sin (w), c2 = std::cos (2 * w), s2 = std::sin (2 * w);
        const double nr = c.b0 + c.b1 * cw + c.b2 * c2, ni = -(c.b1 * sw + c.b2 * s2);
        const double dr = 1 + c.a1 * cw + c.a2 * c2,    di = -(c.a1 * sw + c.a2 * s2);
        return 20.0 * std::log10 (std::sqrt ((nr * nr + ni * ni) / (dr * dr + di * di)) + 1e-30);
    }

    // Pole stability: |a2| < 1 and |a1| < 1 + a2.
    inline bool stable (const teq::Coeffs& c) { return std::abs (c.a2) < 1.0 && std::abs (c.a1) < (1.0 + c.a2); }

    // Render the impulse response of a prepared processor (channel 0).
    inline std::vector<float> renderIR (juce::AudioProcessor& p, int N, int numCh, int block = 512)
    {
        std::vector<float> ir; ir.reserve ((size_t) N);
        juce::AudioBuffer<float> buf (numCh, block);
        juce::MidiBuffer midi;
        bool impulse = false; int produced = 0;
        while (produced < N)
        {
            const int n = juce::jmin (block, N - produced);
            buf.setSize (numCh, n, false, false, true); buf.clear();
            if (! impulse) { for (int ch = 0; ch < numCh; ++ch) buf.setSample (ch, 0, 1.0f); impulse = true; }
            midi.clear();
            p.processBlock (buf, midi);
            for (int i = 0; i < n; ++i) ir.push_back (buf.getSample (0, i));
            produced += n;
        }
        return ir;
    }

    // |FFT| of an impulse response, in dB. Bin k corresponds to freq k*fs/fftSize.
    inline std::vector<double> magnitudeDbFromIR (const std::vector<float>& ir, int fftOrder)
    {
        const int fftSize = 1 << fftOrder;
        juce::dsp::FFT fft (fftOrder);
        std::vector<float> data ((size_t) fftSize * 2, 0.0f);
        const int n = juce::jmin ((int) ir.size(), fftSize);
        for (int i = 0; i < n; ++i) data[(size_t) i] = ir[(size_t) i];
        fft.performFrequencyOnlyForwardTransform (data.data());
        std::vector<double> db ((size_t) (fftSize / 2));
        for (int k = 0; k < fftSize / 2; ++k) db[(size_t) k] = 20.0 * std::log10 ((double) data[(size_t) k] + 1e-30);
        return db;
    }
}
