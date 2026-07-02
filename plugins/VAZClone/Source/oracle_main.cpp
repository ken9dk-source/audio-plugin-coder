// VazOracle — decompiled reference-oracle parity harness (DEL 2).
// Diffs the clone's DSP primitives against reference implementations transcribed straight from the
// Ghidra decomp + raw constants (reference/vaz_constants.h). Each primitive reports:
//   BIT-EXACT              — identical to the last ULP over the tested range
//   DEVIATION (max=…)      — differs; the max sample difference is reported
//   NOT TESTED (reason)    — reference not yet extractable
// The clone is never the reference; vazref::* is.
#include "Synth.h"                       // clone VAZEnv + tables (VAZEnvTables.h)
#include "../reference/vaz_constants.h"
#include <cstdint>
#include <cstdio>
#include <cmath>
#include <vector>
#include <string>

using namespace vazref;

static void row (const char* name, const std::string& status, const std::string& note)
{ std::printf ("  %-26s %-22s %s\n", name, status.c_str(), note.c_str()); }

// ── Reference envelope recurrence, transcribed from vaz_big.c:405-443 (NOT from the clone) ──────────
struct RefEnv
{
    enum { Idle, Attack, Decay, Sustain, Release, PreAttack };
    int stage = Idle; int64_t L = 0;
    int32_t atk = 0, dec = 0, rel = 0; int64_t sus = 0; bool reset = false;
    void noteOn ()  { stage = reset ? PreAttack : Attack; }
    void noteOff () { stage = Release; }
    double step ()
    {
        switch (stage)
        {
            case Attack:    L += ((int64_t) atk * (kEnvAtkTgt - L)) >> 32; if (L > kEnvCap) { L = kEnvCap; stage = Decay; } break;
            case Decay:     L += ((int64_t) dec * ((sus - kEnvOvs) - L)) >> 32; if (L <= sus) { L = sus; stage = Sustain; } break;
            case Release:   L -= ((int64_t) rel * (kEnvOvs + L)) >> 32; if (L < 1) { L = 0; stage = Idle; } break;
            case PreAttack: L -= kStage0Dec; if (L < 1) { L = 0; stage = Attack; } break;
            default: break;
        }
        return (double) L / (double) kEnvOne;
    }
};

int main()
{
    std::printf ("=== VazOracle: decompiled reference vs clone (DEL 2) ===\n\n");
    std::printf ("  %-26s %-22s %s\n", "PRIMITIVE", "STATUS", "NOTE");
    std::printf ("  %-26s %-22s %s\n", "--------------------------", "----------------------", "----");

    // ── 1. Cutoff base-cutoff smoother (one-pole slew) ──────────────────────────────────────────────
    {
        const double aRef = kCutoffSmoothAlpha;    // 6603751/2^32
        const double aClone = 0.00154;             // SynthVoice cutAlpha literal
        double sRef = 0.0, sClone = 0.0, maxd = 0.0; const double tgt = 1.0;
        for (int i = 0; i < 4000; ++i)
        { sRef += (tgt - sRef) * aRef; sClone += (tgt - sClone) * aClone; maxd = std::max (maxd, std::abs (sRef - sClone)); }
        char b[128]; std::snprintf (b, sizeof b, "ref alpha=%.10f  clone=0.00154 (rounded); max slew diff over 4000 smp", aRef);
        row ("cutoff_smoother", "DEVIATION (max=" + std::to_string (maxd) + ")", b);
    }

    // ── 2. Envelope per-sample step (attack→decay→release) ──────────────────────────────────────────
    {
        // Same coefs for both, taken from the clone's setADSR so we isolate the RECURRENCE, not the map.
        VAZEnv ce; ce.setSampleRate (44100.0); ce.setModes (false, false, false);
        ce.setADSR (0.15f, 0.30f, 0.60f, 0.25f, false);
        RefEnv re; re.atk = ce.atkRate; re.dec = ce.decRate; re.rel = ce.relRate; re.sus = ce.susTarget; re.reset = false;
        ce.noteOn(); re.noteOn();
        int64_t maxd = 0; int held = 8000, tail = 8000;   // compare the integer Q30 STATE (the float return is separately lossy)
        for (int i = 0; i < held; ++i) { ce.getNextSample(); re.step(); maxd = std::max<int64_t> (maxd, std::llabs (ce.L - re.L)); }
        ce.noteOff(); re.noteOff();
        for (int i = 0; i < tail; ++i) { ce.getNextSample(); re.step(); maxd = std::max<int64_t> (maxd, std::llabs (ce.L - re.L)); }
        row ("envelope_step", maxd == 0 ? "BIT-EXACT" : "DEVIATION (Ldiff=" + std::to_string (maxd) + ")",
             "clone VAZEnv Q30 state vs transcribed vaz_big.c:405-443 recurrence over full ADSR (same kRate)");
    }

    // ── 3. Detune spread (poly + unison) ────────────────────────────────────────────────────────────
    {
        // VAZ deterministic spread (FUN_004e0618): for N voices, detuneAmt, produce per-voice cents-ish
        // offsets from kDetuneScale[N] * kDetuneOrder[k]. The clone uses a SEEDED-RANDOM spread instead,
        // so the two are not sample-comparable — this is a structural algorithm divergence.
        const int N = 7, detuneAmt = 40;
        long scale = (long) kDetuneScale[N] * detuneAmt;
        std::string vals;
        for (int k = 0; k < N; ++k) { long off = ((long) kDetuneOrder[k] * (scale >> 3)) >> 10; vals += std::to_string (off) + " "; }
        row ("detune_poly_unison", "DEVIATION (algorithm)",
             "VAZ = deterministic bit-reversed table (kDetuneScale x kDetuneOrder); clone = seeded random. VAZ offs[N=7]: " + vals);
    }

    // ── 4. Osc3 footage → pitch ─────────────────────────────────────────────────────────────────────
    row ("osc3_footage_pitch", "NOT TESTED", "VAZ footage LUT address not yet confirmed (manual: 32'=48..2'=240); clone uses osc3FootMul");

    std::printf ("\n  Constants sourced: cutoff-smooth DAT_006d45e4, detune DAT_0052b168/0x52b0ec, env-rate DAT_006db7e8, stage0 DAT_006dc0bc.\n");
    std::printf ("=== oracle complete ===\n");
    return 0;
}
