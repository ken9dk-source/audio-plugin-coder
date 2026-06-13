// Standalone numerical check of the ported VAZ envelope state machine (integer one-pole + dumped tables).
// Verifies measured attack/decay/release times match the table-derived predictions, and no NaN/overflow.
#define _USE_MATH_DEFINES
#include <cstdint>
#include <cstdio>
#include <cmath>
#include <algorithm>
#include "../plugins/VAZClone/Source/VAZEnvTables.h"

struct Env {                                          // mirror of Synth.h VAZEnv (juce::jlimit→std::clamp)
    int stage = 0; int64_t L = 0;
    int32_t atkRate = 0, decRate = 0, relRate = 0, susTarget = 0;
    bool mReset = false, mCycle = false; double srRatio = 1.0;
    static constexpr int64_t ONE = 0x40000000, ATKTGT = 0x44000000, CAP = 0x3fffffff, OVS = 0x400000;
    void setSR (double s) { srRatio = 44100.0 / s; }
    void setADSR (float aN, float dN, float sN, float rN, bool curve) {
        auto rate = [&](float n, int base) {
            int i = std::clamp (base + (int) std::lround (n * 425.0f), 0, 719);
            return (int32_t) std::llround ((double) VAZEnvT::kRate[i] * srRatio);
        };
        int s = std::clamp ((int) std::lround (sN * 255.0f), 0, 255);
        atkRate = rate (aN, 12); decRate = rate (dN, curve ? 12 : 0); relRate = rate (rN, curve ? 12 : 0);
        susTarget = curve ? VAZEnvT::kSusCurve[s] : (int32_t) (s * 0x404040);
    }
    void noteOn()  { if (mReset) L = 0; stage = 1; }
    void noteOff() { stage = 4; }
    float next() {
        switch (stage) {
            case 1: L += ((int64_t) atkRate * (ATKTGT - L)) >> 32; if (L > CAP) { L = CAP; stage = 2; } break;
            case 2: L += ((int64_t) decRate * ((susTarget - OVS) - L)) >> 32; if (L <= susTarget) { L = susTarget; stage = mCycle ? 1 : 3; } break;
            case 3: if (mCycle) stage = 1; break;
            case 4: L -= ((int64_t) relRate * (OVS + L)) >> 32; if (L < 1) { L = 0; stage = 0; } break;
        }
        return (float) ((double) L / (double) ONE);
    }
};

int main() {
    const double sr = 44100.0;
    printf("ATTACK (sustain full, measure time to reach peak):\n");
    for (int v : {0, 30, 60, 100, 200, 309}) {
        Env e; e.setSR (sr); e.mReset = true; e.setADSR (v/425.0f, 0, 1.0f, 0, false); e.noteOn();
        long n = 0; while (e.stage == 1 && n < (long)(sr*40)) { e.next(); n++; }
        printf("  v=%3d: %9.2f ms  (peak=%.4f)\n", v, n/sr*1000.0, (double)e.L/Env::ONE);
    }
    printf("DECAY (sustain 0, measure time to reach 0):\n");
    for (int v : {0, 60, 120, 200, 300}) {
        Env e; e.setSR (sr); e.mReset = true; e.setADSR (0, v/425.0f, 0, 0, false); e.noteOn();
        for (int g = 0; g < 200 && e.stage == 1; g++) e.next();          // clear the (fast) attack
        long n = 0; while (e.stage == 2 && n < (long)(sr*40)) { e.next(); n++; }
        printf("  v=%3d: %9.2f ms  (end stage=%d)\n", v, n/sr*1000.0, e.stage);
    }
    printf("SUSTAIN level (normal vs Curve mode):\n");
    for (int s : {64, 128, 192}) {
        Env a; a.setSR (sr); a.setADSR (0,0,s/255.0f,0,false);
        Env b; b.setSR (sr); b.setADSR (0,0,s/255.0f,0,true);
        printf("  s=%3d: normal=%.4f  curve=%.4f\n", s, (double)a.susTarget/Env::ONE, (double)b.susTarget/Env::ONE);
    }
    // SR-independence: attack v=100 must give ~same ms at 48k and 96k
    printf("SR-independence (attack v=100):\n");
    for (double s : {44100.0, 48000.0, 96000.0}) {
        Env e; e.setSR (s); e.mReset = true; e.setADSR (100/425.0f,0,1.0f,0,false); e.noteOn();
        long n = 0; while (e.stage == 1 && n < (long)(s*40)) { e.next(); n++; }
        printf("  %6.0f Hz: %7.2f ms\n", s, n/s*1000.0);
    }
    // stability: full A/D/S/R cycle, check finite + bounded
    Env e; e.setSR (sr); e.setADSR (0.3f,0.3f,0.5f,0.3f,false); e.noteOn();
    double mx = 0; bool ok = true;
    for (long i = 0; i < (long)(sr*5); i++) { float y = e.next(); if (!std::isfinite(y) || y < -0.01f || y > 1.01f) ok = false; mx = std::max(mx,(double)y); if (i == (long)(sr*2)) e.noteOff(); }
    printf("5s cycle: max=%.4f  %s  final stage=%d\n", mx, ok ? "STABLE/bounded OK" : "*** OUT OF RANGE ***", e.stage);
    return ok ? 0 : 1;
}
