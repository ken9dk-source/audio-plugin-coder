// PeakLFO — Phase 1 tests (Catch2 v3). Engine is header-only (no JUCE).
// Covers: 5 shapes render, phase offset shifts start across full range,
//         negative Volume inverts, Base sets the output floor, taper properties.
#define _USE_MATH_DEFINES
#include <catch2/catch_test_macros.hpp>
#include <catch2/catch_approx.hpp>
#include <cmath>
#include "FPCEngine.h"

using Catch::Approx;

static FPCEngine makeEngine (int shape = FPCEngine::Sine) {
    FPCEngine e; e.prepare (48000.0); e.setLfoTension (0); e.setShape (shape); return e;
}

TEST_CASE ("all five shapes render in [0,1]", "[shapes]") {
    for (int sh = 0; sh < FPCEngine::kNumShapes; ++sh) {
        auto e = makeEngine (sh);
        for (int i = 0; i < 256; ++i) {
            float s = e.evalLfoUnipolar ((double) i / 256.0);
            REQUIRE (s >= -0.001f);
            REQUIRE (s <=  1.001f);
        }
    }
}

TEST_CASE ("saw is a rising ramp; square is bimodal; sine peaks at 1/4", "[shapes]") {
    auto saw = makeEngine (FPCEngine::Saw);
    float prev = saw.evalLfoUnipolar (0.0);
    for (int i = 1; i < 200; ++i) {           // within one cycle, before wrap
        float v = saw.evalLfoUnipolar ((double) i / 256.0);
        REQUIRE (v >= prev - 1e-4f);          // monotonic non-decreasing
        prev = v;
    }

    auto sq = makeEngine (FPCEngine::Square);
    REQUIRE (sq.evalLfoUnipolar (0.25) > 0.9f);   // high half
    REQUIRE (sq.evalLfoUnipolar (0.75) < 0.1f);   // low half

    auto sn = makeEngine (FPCEngine::Sine);
    REQUIRE (sn.evalLfoUnipolar (0.25) == Approx (1.0f).margin (0.01)); // sine peak
    REQUIRE (sn.evalLfoUnipolar (0.75) == Approx (0.0f).margin (0.01)); // sine trough
    REQUIRE (sn.evalLfoUnipolar (0.0)  == Approx (0.5f).margin (0.01)); // zero-cross
}

TEST_CASE ("shapes are distinct", "[shapes]") {
    auto sn = makeEngine (FPCEngine::Sine);
    auto sw = makeEngine (FPCEngine::Saw);
    auto sq = makeEngine (FPCEngine::Square);
    REQUIRE (sn.evalLfoUnipolar (0.1) != Approx (sw.evalLfoUnipolar (0.1)).margin (0.02));
    REQUIRE (sw.evalLfoUnipolar (0.6) != Approx (sq.evalLfoUnipolar (0.6)).margin (0.02));
}

TEST_CASE ("phase offset shifts start position across the full 0..1 range", "[phase]") {
    auto e = makeEngine (FPCEngine::Sine);
    // offsetting the phase equals evaluating at the shifted (wrapped) position
    for (double off : { 0.0, 0.1, 0.25, 0.5, 0.75, 0.9 }) {
        REQUIRE (e.evalLfoUnipolar (0.3 + off) == Approx (e.evalLfoUnipolar (std::fmod (0.3 + off, 1.0))));
    }
    // different offsets genuinely move the start (0.1 rising vs 0.6 falling on a sine)
    REQUIRE (e.evalLfoUnipolar (0.1)  != Approx (e.evalLfoUnipolar (0.6)).margin (0.1));
    // full range is usable (no cap at 0.75)
    REQUIRE (e.evalLfoUnipolar (0.9) >= 0.0f);
    REQUIRE (e.evalLfoUnipolar (0.9) <= 1.0f);
}

TEST_CASE ("volume taper: 0->0, +-1->+-1, monotonic, sign-preserving", "[volume]") {
    REQUIRE (FPCEngine::volumeTaper (0.0f)  == Approx (0.0f));
    REQUIRE (FPCEngine::volumeTaper (1.0f)  == Approx (1.0f));
    REQUIRE (FPCEngine::volumeTaper (-1.0f) == Approx (-1.0f));
    REQUIRE (FPCEngine::volumeTaper (0.3f)  <  FPCEngine::volumeTaper (0.6f));   // monotonic
    REQUIRE (FPCEngine::volumeTaper (-0.5f) == Approx (-FPCEngine::volumeTaper (0.5f)));  // odd symmetry
    // "typed value != knob position" — the taper is not the identity (it curves)
    REQUIRE (FPCEngine::volumeTaper (0.5f) != Approx (0.5f).margin (0.02));
}

TEST_CASE ("negative Volume inverts the wave around Base", "[volume]") {
    const float base = 0.5f;
    const float vol  = FPCEngine::volumeTaper (0.5f);   // positive swing
    for (float s : { 0.0f, 0.25f, 0.5f, 0.75f, 1.0f }) {
        const float up   = FPCEngine::outputGain (base, s,  vol);
        const float down = FPCEngine::outputGain (base, s, -vol);
        REQUIRE (down == Approx (2.0f * base - up).margin (1e-4)); // mirror about Base (no clamp at base=0.5)
    }
}

TEST_CASE ("Base sets the output floor for positive Volume", "[base]") {
    const float vol = FPCEngine::volumeTaper (0.8f);
    for (float base : { 0.0f, 0.2f, 0.5f }) {
        REQUIRE (FPCEngine::outputGain (base, 0.0f, vol) == Approx (base)); // shape=0 -> floor = Base
        for (float s : { 0.1f, 0.5f, 1.0f })
            REQUIRE (FPCEngine::outputGain (base, s, vol) >= base - 1e-4f);  // never dips below floor
    }
}
