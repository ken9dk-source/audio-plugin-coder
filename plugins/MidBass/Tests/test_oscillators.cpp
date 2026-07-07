// Phase 1 — oscillators + sub + unison (test-first).
// Covers the roadmap gates AND the 2026-07-07 approval conditions:
//  * pitch accuracy ±0.5 cent            * saw alias floor < −60 dB @ high pitch
//  * FM/ring alias checks @ high pitch   * FM depth capped to the "subtle" range
//  * sub true-bypass (zero processing)   * mono-unison rule (spread=0, 1/√n sum, ≤1 dB)
//  * drift bounded + actually moving     * hard sync osc2→osc1
#include <catch2/catch_test_macros.hpp>
#include <catch2/catch_approx.hpp>
#include <juce_dsp/juce_dsp.h>
#include "MbOscillators.h"
#include <cmath>
#include <vector>

using Catch::Approx;
namespace
{
constexpr double kSR = 44100.0;

mb::OscEngine::Config sawOnly()
{
    mb::OscEngine::Config c;                 // defaults: everything off/neutral
    c.osc[0].wave  = mb::OscWave::Saw;
    c.osc[0].level = 1.0f;
    return c;
}

std::vector<float> renderMono (mb::OscEngine& eng, int numSamples)
{
    std::vector<float> out ((size_t) numSamples);
    for (int i = 0; i < numSamples; ++i)
    {
        float l = 0.0f, r = 0.0f;
        eng.process (l, r);
        out[(size_t) i] = 0.5f * (l + r);
    }
    return out;
}

// Rising zero-crossing pitch estimate (linear interp). Works on any single-crossing-per-period wave.
double measureHz (const std::vector<float>& x)
{
    double tFirst = -1.0, tLast = -1.0;
    int crossings = 0;
    for (size_t i = 1; i < x.size(); ++i)
        if (x[i - 1] <= 0.0f && x[i] > 0.0f)
        {
            const double t = (double) (i - 1) + (double) (-x[i - 1]) / (double) (x[i] - x[i - 1]);
            if (tFirst < 0.0) tFirst = t;
            tLast = t;
            ++crossings;
        }
    REQUIRE (crossings > 2);
    return (double) (crossings - 1) / ((tLast - tFirst) / kSR);
}

struct Spectrum
{
    static constexpr int kOrder = 16, kN = 1 << kOrder;
    std::vector<float> mag;                          // linear magnitude, kN/2 bins
    double binHz;

    explicit Spectrum (const std::vector<float>& x) : mag ((size_t) kN / 2, 0.0f), binHz (kSR / kN)
    {
        REQUIRE ((int) x.size() >= kN);
        juce::dsp::FFT fft (kOrder);
        std::vector<float> buf ((size_t) kN * 2, 0.0f);
        for (int i = 0; i < kN; ++i)
        {
            // 4-term Blackman-Harris: -92 dB sidelobes, so window skirts can't fake aliasing.
            const double t = 2.0 * juce::MathConstants<double>::pi * (double) i / (double) kN;
            const double w = 0.35875 - 0.48829 * std::cos (t) + 0.14128 * std::cos (2.0 * t) - 0.01168 * std::cos (3.0 * t);
            buf[(size_t) i] = x[(size_t) i] * (float) w;
        }
        fft.performFrequencyOnlyForwardTransform (buf.data());
        for (int i = 0; i < kN / 2; ++i) mag[(size_t) i] = buf[(size_t) i];
    }

    int   binFor (double hz) const  { return (int) std::lround (hz / binHz); }
    float magAt (double hz) const                       // peak within ±2 bins (leakage)
    {
        float m = 0.0f;
        for (int b = binFor (hz) - 2; b <= binFor (hz) + 2; ++b)
            if (b > 0 && b < (int) mag.size()) m = std::max (m, mag[(size_t) b]);
        return m;
    }

    // Loudest bin that is NOT within ±guard bins of any harmonic k*f0 (k=1..) and not near DC.
    // guard=8 clears the Blackman-Harris mainlobe (±4 bins) with margin.
    float maxNonHarmonic (double f0, int guard = 8) const
    {
        float worst = 0.0f;
        for (int b = 8; b < (int) mag.size(); ++b)
        {
            const double hz = b * binHz;
            const double k  = hz / f0;
            const double distBins = std::abs (k - std::round (k)) * f0 / binHz;
            if (std::round (k) >= 1.0 && distBins <= (double) guard) continue;
            worst = std::max (worst, mag[(size_t) b]);
        }
        return worst;
    }
};

double dB (float a, float ref) { return 20.0 * std::log10 ((double) std::max (a, 1.0e-12f) / (double) std::max (ref, 1.0e-12f)); }
} // namespace

//==============================================================================
TEST_CASE ("osc: pitch accurate within 0.5 cent")
{
    mb::OscEngine eng;
    eng.prepare (kSR);
    auto c = sawOnly();
    c.osc[0].wave = mb::OscWave::Triangle;   // one rising crossing per period
    eng.setConfig (c);
    eng.setPitch (220.0);

    auto x = renderMono (eng, (int) (2.0 * kSR));
    const double cents = 1200.0 * std::log2 (measureHz (x) / 220.0);
    REQUIRE (std::abs (cents) < 0.5);
}

TEST_CASE ("osc: octave/semi/fine offsets are exact")
{
    mb::OscEngine eng;
    eng.prepare (kSR);
    auto c = sawOnly();
    c.osc[0].wave = mb::OscWave::Triangle;
    c.osc[0].oct = 1; c.osc[0].semi = 7; c.osc[0].fineCents = 50.0f;
    eng.setConfig (c);
    eng.setPitch (110.0);

    const double expected = 110.0 * std::pow (2.0, 1.0 + 7.0 / 12.0 + 50.0 / 1200.0);
    auto x = renderMono (eng, (int) (2.0 * kSR));
    const double cents = 1200.0 * std::log2 (measureHz (x) / expected);
    REQUIRE (std::abs (cents) < 0.5);
}

TEST_CASE ("osc: saw alias floor below -60 dB at high pitch")
{
    mb::OscEngine eng;
    eng.prepare (kSR);
    eng.setConfig (sawOnly());
    const double f0 = 2093.0;               // ~C7
    eng.setPitch (f0);

    Spectrum s (renderMono (eng, Spectrum::kN + 1024));
    REQUIRE (dB (s.maxNonHarmonic (f0), s.magAt (f0)) < -60.0);
}

TEST_CASE ("osc: FM depth stays in the subtle range and FM output stays clean")
{
    // Approval condition #5: subtle = peak frequency deviation capped well below a fifth.
    REQUIRE (mb::kMaxFmDepth <= 0.25f);

    mb::OscEngine eng;
    eng.prepare (kSR);
    auto c = sawOnly();
    c.osc[1].wave = mb::OscWave::Saw;       // modulator runs at the same pitch → harmonic sidebands
    c.fm = 1.0f;
    eng.setConfig (c);
    const double f0 = 2093.0;
    eng.setPitch (f0);

    Spectrum s (renderMono (eng, Spectrum::kN + 1024));
    REQUIRE (dB (s.maxNonHarmonic (f0), s.magAt (f0)) < -40.0);

    // Average pitch is unchanged by symmetric FM: fundamental still dominates at f0.
    mb::OscEngine low; low.prepare (kSR); low.setConfig (c); low.setPitch (110.0);
    auto x = renderMono (low, Spectrum::kN + 1024);
    Spectrum sl (x);
    REQUIRE (sl.magAt (110.0) > 0.5f * sl.maxNonHarmonic (110.0, 1));
}

TEST_CASE ("osc: ring mod output stays clean at high pitch")
{
    mb::OscEngine eng;
    eng.prepare (kSR);
    auto c = sawOnly();
    c.osc[1].wave = mb::OscWave::Saw;
    c.ring = 1.0f;
    eng.setConfig (c);
    const double f0 = 2093.0;
    eng.setPitch (f0);

    Spectrum s (renderMono (eng, Spectrum::kN + 1024));
    REQUIRE (dB (s.maxNonHarmonic (f0), s.magAt (f0)) < -40.0);
}

TEST_CASE ("osc: hard sync locks osc1 to the osc2 period")
{
    mb::OscEngine eng;
    eng.prepare (kSR);
    auto c = sawOnly();
    c.osc[1].wave = mb::OscWave::Saw;
    c.osc[1].semi = 7;                       // master a fifth up → slave period = master period
    c.osc[1].level = 0.0f;                   // only the synced slave is audible
    c.sync = true;
    eng.setConfig (c);
    const double f0 = 220.0;
    const double fMaster = f0 * std::pow (2.0, 7.0 / 12.0);
    eng.setPitch (f0);

    Spectrum s (renderMono (eng, Spectrum::kN + 1024));
    REQUIRE (dB (s.magAt (f0), s.magAt (fMaster)) < -20.0);   // f0 is gone; fundamental = master
}

TEST_CASE ("sub: tracks one/two octaves down")
{
    for (int oct = 1; oct <= 2; ++oct)
    {
        mb::OscEngine eng;
        eng.prepare (kSR);
        mb::OscEngine::Config c;             // all main oscs silent
        c.subOn = true; c.subWave = mb::SubWave::Sine; c.subOctDown = oct; c.subLevel = 1.0f;
        eng.setConfig (c);
        eng.setPitch (220.0);

        auto x = renderMono (eng, (int) (2.0 * kSR));
        const double cents = 1200.0 * std::log2 (measureHz (x) / (220.0 / (oct == 1 ? 2.0 : 4.0)));
        REQUIRE (std::abs (cents) < 0.5);
    }
}

TEST_CASE ("sub: true bypass — zero processing when off")
{
    mb::OscEngine eng;
    eng.prepare (kSR);
    auto c = sawOnly();
    c.subOn = false; c.subLevel = 1.0f;      // level up, but OFF
    eng.setConfig (c);
    eng.setPitch (110.0);

    const double phaseBefore = eng.subPhaseForTest();
    (void) renderMono (eng, 4096);
    REQUIRE (eng.subPhaseForTest() == phaseBefore);   // the sub osc never ran
}

TEST_CASE ("unison: mono mode sums to identical channels with detune still active")
{
    mb::OscEngine eng;
    eng.prepare (kSR);
    auto c = sawOnly();
    c.uniVoices = 6; c.uniDetune = 0.5f; c.uniSpread = 1.0f; c.uniMono = true;
    eng.setConfig (c);
    eng.setPitch (110.0);

    bool detuneAudible = false;
    float prevMag = -1.0f;
    for (int i = 0; i < 44100; ++i)
    {
        float l = 0.0f, r = 0.0f;
        eng.process (l, r);
        REQUIRE (l == r);                              // spread forced to zero, exact mono sum
        const float m = std::abs (l);
        if (prevMag >= 0.0f && m != prevMag) detuneAudible = true;
        prevMag = m;
    }
    REQUIRE (detuneAudible);                           // signal is alive (beating detuned stack)
}

TEST_CASE ("unison: toggling mono changes perceived level by no more than ~1 dB")
{
    auto rmsOf = [] (bool mono)
    {
        mb::OscEngine eng;
        eng.prepare (kSR);
        auto c = sawOnly();
        c.uniVoices = 8; c.uniDetune = 0.5f; c.uniSpread = 1.0f; c.uniMono = mono;
        eng.setConfig (c);
        eng.setPitch (110.0);
        double acc = 0.0; const int n = (int) kSR;
        for (int i = 0; i < n; ++i)
        {
            float l = 0.0f, r = 0.0f;
            eng.process (l, r);
            acc += 0.5 * ((double) l * l + (double) r * r);
        }
        return std::sqrt (acc / n);
    };
    const double diffDb = 20.0 * std::log10 (rmsOf (true) / rmsOf (false));
    REQUIRE (std::abs (diffDb) <= 1.0);
}

TEST_CASE ("drift: bounded, seed-stable at zero, and actually moving at full amount")
{
    mb::DriftGen d;
    d.prepare (kSR, 1234u);
    double mn = 1.0e9, mx = -1.0e9;
    for (int i = 0; i < (int) (10.0 * kSR); ++i)
    {
        const double c = d.nextCents (1.0f);           // full amount
        mn = std::min (mn, c); mx = std::max (mx, c);
    }
    REQUIRE (mx <= mb::kMaxDriftCents + 1.0e-6);
    REQUIRE (mn >= -mb::kMaxDriftCents - 1.0e-6);
    REQUIRE (mx - mn > 0.5);                           // it drifts

    mb::DriftGen z;
    z.prepare (kSR, 99u);
    for (int i = 0; i < 1000; ++i)
        REQUIRE (z.nextCents (0.0f) == 0.0);           // amount 0 → bit-exact no drift
}
