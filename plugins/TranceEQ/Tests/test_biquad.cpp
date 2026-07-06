#include <catch2/catch_test_macros.hpp>
#include <catch2/matchers/catch_matchers_floating_point.hpp>
#include "TestUtils.h"

using teq::FilterType;
using Catch::Matchers::WithinAbs;

TEST_CASE ("RBJ coefficients match an independent reference", "[biquad][rbj]")
{
    const double fs = 48000.0;
    struct C { FilterType t; double f, g, q; };
    const std::vector<C> cases = {
        { FilterType::LPF,        1000,   0, 0.707 }, { FilterType::HPF,        120,  0, 0.707 },
        { FilterType::Notch,      1000,   0, 4.0   }, { FilterType::Peak,      2000,  6, 1.0   },
        { FilterType::Peak,       2000,  -6, 1.0   }, { FilterType::LowShelf,   300,  4, 0.707 },
        { FilterType::HighShelf,  8000,  -3, 0.707 }, { FilterType::Peak,        50, 12, 0.30  },
        { FilterType::HighShelf, 15000,   6, 0.9   }, { FilterType::LowShelf,   120, -9, 1.2   },
    };
    for (const auto& c : cases)
    {
        const auto got = teq::makeCoeffs (c.t, c.f, c.g, c.q, fs);
        const auto ref = tt::rbjReference (c.t, c.f, c.g, c.q, fs);
        INFO ("type=" << (int) c.t << " f=" << c.f << " g=" << c.g << " q=" << c.q);
        REQUIRE_THAT (got.b0, WithinAbs (ref.b0, 1e-9));
        REQUIRE_THAT (got.b1, WithinAbs (ref.b1, 1e-9));
        REQUIRE_THAT (got.b2, WithinAbs (ref.b2, 1e-9));
        REQUIRE_THAT (got.a1, WithinAbs (ref.a1, 1e-9));
        REQUIRE_THAT (got.a2, WithinAbs (ref.a2, 1e-9));
    }
}

TEST_CASE ("Biquads are stable across the whole parameter space", "[biquad][stability]")
{
    const double fs = 48000.0;
    for (double f = 20.0; f < 20000.0; f *= 1.5)
        for (double q : { 0.1, 0.5, 0.707, 2.0, 8.0, 18.0 })
            for (double g : { -18.0, -6.0, 0.0, 6.0, 18.0 })
                for (auto t : { FilterType::HPF, FilterType::LPF, FilterType::Peak,
                                FilterType::LowShelf, FilterType::HighShelf, FilterType::Notch })
                {
                    const auto c = teq::makeCoeffs (t, f, g, q, fs);
                    INFO ("f=" << f << " q=" << q << " g=" << g << " type=" << (int) t);
                    REQUIRE (tt::stable (c));
                }
}

TEST_CASE ("DC / Nyquist / centre gains are correct", "[biquad][response]")
{
    const double fs = 48000.0;

    const auto lp = teq::makeCoeffs (FilterType::LPF, 1000, 0, 0.707, fs);
    REQUIRE_THAT (tt::biquadMagDb (lp, 1.0,     fs), WithinAbs (0.0, 0.1));   // passes DC
    REQUIRE      (tt::biquadMagDb (lp, 23000.0, fs) < -40.0);                 // blocks Nyquist

    const auto hp = teq::makeCoeffs (FilterType::HPF, 1000, 0, 0.707, fs);
    REQUIRE      (tt::biquadMagDb (hp, 5.0,     fs) < -40.0);                 // blocks DC
    REQUIRE_THAT (tt::biquadMagDb (hp, 23500.0, fs), WithinAbs (0.0, 0.5));   // passes Nyquist

    const auto pk = teq::makeCoeffs (FilterType::Peak, 2000, 6, 2.0, fs);
    REQUIRE_THAT (tt::biquadMagDb (pk, 2000.0, fs), WithinAbs (6.0, 0.02));   // exact +6 at centre

    const auto ls = teq::makeCoeffs (FilterType::LowShelf, 300, 4, 0.707, fs);
    REQUIRE_THAT (tt::biquadMagDb (ls, 5.0, fs), WithinAbs (4.0, 0.2));       // +4 at DC

    const auto hs = teq::makeCoeffs (FilterType::HighShelf, 8000, -3, 0.707, fs);
    REQUIRE_THAT (tt::biquadMagDb (hs, 23500.0, fs), WithinAbs (-3.0, 0.3));  // -3 at Nyquist
}

TEST_CASE ("Biquad impulse response matches the analytic magnitude", "[biquad][ir]")
{
    const double fs = 48000.0;
    const auto c = teq::makeCoeffs (FilterType::Peak, 2000, 6, 1.0, fs);
    teq::Biquad bq; bq.reset(); bq.setCoeffs (c);

    const int order = 15, N = 1 << order;
    std::vector<float> ir ((size_t) N, 0.0f);
    ir[0] = bq.process (1.0f);
    for (int i = 1; i < N; ++i) ir[(size_t) i] = bq.process (0.0f);

    const auto db = tt::magnitudeDbFromIR (ir, order);
    for (double f : { 200.0, 1000.0, 2000.0, 4000.0, 10000.0 })
    {
        const int k = (int) std::round (f * N / fs);
        INFO ("f=" << f << " measured=" << db[(size_t) k] << " analytic=" << tt::biquadMagDb (c, f, fs));
        REQUIRE_THAT (db[(size_t) k], WithinAbs (tt::biquadMagDb (c, f, fs), 0.2));
    }
}
