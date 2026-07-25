// PeakLFO — Phase 3 tests (Catch2 v3): residual vs the FL-decompiled literals.
// Independent double-precision reference of the decompiled formulas; asserts the shipping
// engine matches to float precision (< 1e-6). Locks the calibration against future drift.
#define _USE_MATH_DEFINES
#include <catch2/catch_test_macros.hpp>
#include <cmath>
#include "FPCEngine.h"

// ---- independent references (decompiled literals) ----
static double volTaperRef (double k) {
    const double amtScale = 214748368.0 / 1073741824.0;   // decompiled LFO amtScale / 2^30
    const double s = (k < 0) ? -1.0 : 1.0;
    return s * (std::pow (6.0, std::abs (k)) - 1.0) * amtScale;
}
static double tensionMagRef (double t) { return (std::pow (1001.0, std::abs (t)) - 1.0) * 0.1; }
static double warpRef (double s, double t) {
    if (t == 0.0) return s;
    const double T = tensionMagRef (t);
    if (T <= 0.0) return s;
    if (t > 0) return (T - (std::pow (T + 1.0, 1.0 - s) - 1.0)) / T;
    return          (std::pow (T + 1.0, s) - 1.0) / T;
}

TEST_CASE ("VOL taper matches the decompiled FL literal (6^ /256 x0.2)", "[calibrate]") {
    double maxRes = 0.0;
    for (int i = -1000; i <= 1000; ++i) {
        double k = i / 1000.0;
        maxRes = std::max (maxRes, std::abs ((double) FPCEngine::volumeTaper ((float) k) - volTaperRef (k)));
    }
    REQUIRE (maxRes < 1e-6);
}

TEST_CASE ("tension warp matches the decompiled FL literal (1001 /128 x0.1 + warp)", "[calibrate]") {
    double maxRes = 0.0;
    for (int ti = -128; ti <= 128; ti += 2) {
        double t = ti / 128.0;
        FPCEngine e; e.prepare (48000.0); e.setShape (FPCEngine::Saw);
        e.setLfoTension ((int) std::lround (t * 128.0));   // TENSION_FULL = 128
        for (int si = 1; si < 512; ++si) {
            double s = si / 512.0;
            maxRes = std::max (maxRes, std::abs ((double) e.evalLfoUnipolar (s) - warpRef (s, t)));
        }
    }
    REQUIRE (maxRes < 1e-6);
}
