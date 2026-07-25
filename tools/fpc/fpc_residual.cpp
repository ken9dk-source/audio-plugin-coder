// PeakLFO Phase 3 — residual vs the FL-decompiled literals.
// Compares the shipping engine (FPCEngine.h) against an INDEPENDENT double-precision
// reference of the decompiled formulas. Residual ~ float epsilon => implementation
// equals the decompiled literals (no inferred values in the curves themselves).
#define _USE_MATH_DEFINES
#include "../../plugins/PeakLFO/Source/FPCEngine.h"
#include <cmath>
#include <cstdio>
#include <algorithm>

// ---- independent references (decompiled literals, written out fresh) ----
// VOL taper (FUN_00705f30): amount = (6^(|raw|/256) - 1) * amtScale ; amtScale_LFO/2^30.
static double volTaperRef(double k) {
    const double amtScale = 214748368.0 / 1073741824.0;   // decompiled LFO amtScale / 2^30
    const double s = (k < 0) ? -1.0 : 1.0;
    return s * (std::pow(6.0, std::abs(k)) - 1.0) * amtScale;   // |raw|/256 == |k|
}
// Tension magnitude (FUN_00705f30): T = (1001^(|raw|/128) - 1) * 0.1 ; |raw|=128*|t|.
static double tensionMagRef(double t) {
    return (std::pow(1001.0, std::abs(t)) - 1.0) * 0.1;         // |raw|/128 == |t|
}
// Normalised tension warp (FUN_00706c00 warp / T coupling):
static double warpRef(double s, double t) {
    if (t == 0.0) return s;
    const double T = tensionMagRef(t);
    if (T <= 0.0) return s;
    if (t > 0) return (T - (std::pow(T + 1.0, 1.0 - s) - 1.0)) / T;
    return          (std::pow(T + 1.0, s) - 1.0) / T;
}

int main() {
    // --- VOL taper residual over the full bipolar knob range ---
    double vMax = 0, vSum = 0; int vN = 0;
    for (int i = -1000; i <= 1000; ++i) {
        double k = i / 1000.0;
        double e = FPCEngine::volumeTaper((float) k);
        double r = volTaperRef(k);
        double d = std::abs(e - r); vMax = std::max(vMax, d); vSum += d * d; ++vN;
    }
    printf("VOL taper   : max residual = %.3e   RMS = %.3e   (n=%d)\n", vMax, std::sqrt(vSum / vN), vN);

    // --- tension warp residual over (tension, phase) grid, using the Saw shape (s == phase) ---
    double tMax = 0, tSum = 0; int tN = 0; double worstT = 0, worstS = 0;
    for (int ti = -128; ti <= 128; ti += 2) {
        double t = ti / 128.0;                 // knob -1..1
        FPCEngine e; e.prepare(48000.0); e.setShape(FPCEngine::Saw);
        e.setLfoTension((int) std::lround(t * 128.0));   // TENSION_FULL = 128
        for (int si = 1; si < 512; ++si) {
            double s = si / 512.0;
            double got = e.evalLfoUnipolar(s);            // saw: raw shape == s
            double ref = warpRef(s, t);
            double d = std::abs(got - ref);
            if (d > tMax) { tMax = d; worstT = t; worstS = s; }
            tSum += d * d; ++tN;
        }
    }
    printf("Tension warp: max residual = %.3e   RMS = %.3e   (n=%d, worst @ t=%.3f s=%.3f)\n",
           tMax, std::sqrt(tSum / tN), tN, worstT, worstS);

    printf("\nInterpretation: residuals at float-epsilon => the shipping curves ARE the\n");
    printf("decompiled FL formulas (base 6 /256 x0.2 taper; base 1001 /128 x0.1 tension + warp).\n");
    printf("Only un-decompilable DOF: absolute knob->raw span (tension TENSION_FULL) — runtime FL capture, spec 7.\n");
    return 0;
}
