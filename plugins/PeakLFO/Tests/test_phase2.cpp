// PeakLFO — Phase 2 tests (Catch2 v3): tension warp properties.
// Uses Saw as the base shape (evalLfoUnipolar(ph) == ph when unwarped), so deviation
// from the base is easy to measure.
#define _USE_MATH_DEFINES
#include <catch2/catch_test_macros.hpp>
#include <catch2/catch_approx.hpp>
#include <cmath>
#include "FPCEngine.h"

using Catch::Approx;

static FPCEngine sawEngine(int rawTens) {
    FPCEngine e; e.prepare(48000.0); e.setShape(FPCEngine::Saw); e.setLfoTension(rawTens); return e;
}
static double deviationFromSaw(int rawTens) {
    auto e = sawEngine(rawTens);
    double d = 0.0;
    for (int i = 1; i < 512; ++i) { double ph = (double) i / 512.0; d += std::abs((double) e.evalLfoUnipolar(ph) - ph); }
    return d;
}

TEST_CASE("tension = 0 leaves the shape unchanged", "[tension]") {
    auto e = sawEngine(0);
    for (int i = 1; i < 512; ++i) { double ph = (double) i / 512.0; REQUIRE(e.evalLfoUnipolar(ph) == Approx(ph).margin(1e-4)); }
    REQUIRE(deviationFromSaw(0) == Approx(0.0).margin(1e-3));
}

TEST_CASE("endpoints stay fixed for all tension (amplitude preserved)", "[tension]") {
    for (int t : { -64, -32, 0, 32, 64 }) {
        auto e = sawEngine(t);
        REQUIRE(e.evalLfoUnipolar(0.0005) == Approx(0.0f).margin(0.01));
        REQUIRE(e.evalLfoUnipolar(0.9995) == Approx(1.0f).margin(0.01));
        // stays within [0,1] across the cycle
        for (int i = 0; i < 512; ++i) { float v = e.evalLfoUnipolar((double) i / 512.0); REQUIRE(v >= -0.001f); REQUIRE(v <= 1.001f); }
    }
}

TEST_CASE("deviation increases monotonically with |tension|", "[tension]") {
    const double d0  = deviationFromSaw(0);
    const double d16 = deviationFromSaw(16);
    const double d32 = deviationFromSaw(32);
    const double d48 = deviationFromSaw(48);
    const double d64 = deviationFromSaw(64);
    REQUIRE(d0  <  d16);
    REQUIRE(d16 <  d32);
    REQUIRE(d32 <  d48);
    REQUIRE(d48 <  d64);
    // negative tension warps by the same magnitude (odd symmetry of the curvature)
    REQUIRE(deviationFromSaw(-32) == Approx(d32).margin(d32 * 0.02 + 1e-3));
}

TEST_CASE("warp is continuous (no discontinuities) even at max tension", "[tension]") {
    auto e = sawEngine(64);
    float prev = e.evalLfoUnipolar(0.0);
    for (int i = 1; i <= 1024; ++i) {
        float v = e.evalLfoUnipolar((double) i / 1024.0);
        // within one cycle consecutive samples must not jump (saw wraps once at the end)
        if (i < 1024) REQUIRE(std::abs(v - prev) < 0.05f);
        prev = v;
    }
}

TEST_CASE("positive and negative tension curve in opposite directions", "[tension]") {
    const float mid = 0.5f;
    const float pos = sawEngine(48).evalLfoUnipolar(mid);
    const float neg = sawEngine(-48).evalLfoUnipolar(mid);
    REQUIRE(pos > mid + 0.02f);   // convex: pushed up
    REQUIRE(neg < mid - 0.02f);   // concave: pulled down
    REQUIRE(pos == Approx(1.0f - neg).margin(1e-3)); // mirror symmetry about 0.5
}
