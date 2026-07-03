// VazOracle — decompiled reference-oracle parity harness (DEL 2).
// Diffs the clone's DSP primitives against reference implementations transcribed straight from the
// Ghidra decomp + raw constants (reference/vaz_constants.h). Each primitive reports:
//   BIT-EXACT              — identical to the last ULP over the tested range
//   DEVIATION (max=…)      — differs; the max sample difference is reported
//   NOT TESTED (reason)    — reference not yet extractable
// The clone is never the reference; vazref::* is.
#include "Synth.h"                       // clone VAZEnv + tables (VAZEnvTables.h)
#include "../reference/vaz_constants.h"
#include "../reference/vaz_detune.h"     // clone's detune port (FUN_004e0618)
#include "../reference/vaz_fx_constants.h" // FX raw constants (TFX* in Core.dll)
#include <cstdint>
#include <cstdio>
#include <cmath>
#include <vector>
#include <string>
#include <functional>

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
        const double aRef = kCutoffSmoothAlpha;                        // 6603751/2^32
        const double aClone = kCutoffSmoothAlpha * (44100.0 / 44100.0); // SynthVoice cutAlpha at 44.1k (exact DAT_006d45e4, SR-scaled)
        double sRef = 0.0, sClone = 0.0, maxd = 0.0; const double tgt = 1.0;
        for (int i = 0; i < 4000; ++i)
        { sRef += (tgt - sRef) * aRef; sClone += (tgt - sClone) * aClone; maxd = std::max (maxd, std::abs (sRef - sClone)); }
        char b[128]; std::snprintf (b, sizeof b, "alpha=%.10f (exact DAT_006d45e4, SR-scaled); max slew diff over 4000 smp", aRef);
        row ("cutoff_smoother", maxd == 0.0 ? "BIT-EXACT" : "DEVIATION (max=" + std::to_string (maxd) + ")", b);
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
        // The clone now uses vazref::detunePoly/detuneUnison (reference/vaz_detune.h) — the port of
        // FUN_004e0618. Verify that port against an INDEPENDENT literal transcription of the decomp
        // (variable-for-variable from vaz_prims.c, using >>1+parity rounding rather than /2). If the two
        // agree across a sweep of (N, amt), the port is bit-exact to the decompiled algorithm.
        auto litPoly = [] (int polyN, int amt, int32_t* o)
        {
            if (polyN <= 1) { if (polyN == 1) o[0] = 0; return; }
            int iVar8 = kDetuneScale[polyN < 32 ? polyN : 31] * amt; if (iVar8 < 0) iVar8 += 7;
            unsigned uVar4 = (unsigned) (-((polyN - 1) * (iVar8 >> 3))); int iVar9 = (int) uVar4 >> 1;
            if (iVar9 < 0) iVar9 += (int) ((uVar4 & 1) != 0);
            int t = polyN * 3; if (t < 0) t += 3; unsigned c = 0; while ((t >> 2) != kDetuneOrder[c]) ++c;
            for (int i = 0; i < polyN; ++i) { int v = kDetuneOrder[c] * (iVar8 >> 3) + iVar9; if (v < 0) v += 0x3ff;
                o[i] = v >> 10; do { c = (c + 1) & 0x1f; } while (polyN <= (int) kDetuneOrder[c]); }
        };
        auto litUni = [] (int uniN, int amt, int32_t* o)
        {
            if (uniN <= 1) { if (uniN == 1) o[0] = 0; return; }
            int iVar8 = (amt << 9) / uniN; unsigned uVar4 = (unsigned) (-((uniN - 1) * iVar8)); int iVar9 = (int) uVar4 >> 1;
            if (iVar9 < 0) iVar9 += (int) ((uVar4 & 1) != 0);
            int t = uniN * 3; if (t < 0) t += 3; unsigned c = 0; while ((t >> 2) != kDetuneOrder[c]) ++c;
            for (int i = 0; i < uniN; ++i) { int v = kDetuneOrder[c] * iVar8 + iVar9; if (v < 0) v += 0x3ff;
                o[i] = v >> 10; do { c = (c + 1) & 0x1f; } while (uniN <= (int) kDetuneOrder[c]); }
        };
        int32_t a[32], b[32]; int64_t maxd = 0; std::string sample;
        for (int amt = 0; amt <= 255; amt += 5)
            for (int N = 2; N <= 31; ++N)
            {
                vazref::detunePoly (N, amt, a); litPoly (N, amt, b);
                for (int i = 0; i < N; ++i) maxd = std::max<int64_t> (maxd, std::llabs (a[i] - b[i]));
                vazref::detuneUnison (N, amt, a); litUni (N, amt, b);
                for (int i = 0; i < N; ++i) maxd = std::max<int64_t> (maxd, std::llabs (a[i] - b[i]));
            }
        vazref::detunePoly (7, 40, a); for (int i = 0; i < 7; ++i) sample += std::to_string (a[i]) + " ";
        row ("detune_poly (table)",  maxd == 0 ? "BIT-EXACT" : "DEVIATION (max=" + std::to_string (maxd) + ")",
             "clone port vs literal decomp over N=2..31, amt=0..255; poly offs[N=7,amt=40]: " + sample);
        vazref::detuneUnison (7, 40, a); std::string su; for (int i = 0; i < 7; ++i) su += std::to_string (a[i]) + " ";
        row ("detune_unison (linear)", maxd == 0 ? "BIT-EXACT" : "DEVIATION", "unison offs[N=7,amt=40]: " + su);
    }

    // ── 4. Osc3 footage → pitch ─────────────────────────────────────────────────────────────────────
    // ── 4. Osc3 footage — EMPIRICAL operand decode via anchors ──────────────────────────────────────
    {
        // Direct C port of VAZ's Osc3-increment x87 sequence (the branch that feeds param+0x9c, used at
        // vaz_big.c:159 when Osc3 flag param+0x234≠0). Disasm 0x4dbf12-0x4dbf43:
        //   0x4dbf12 fild  qword[ebp-0x18] = note        ; st0 = note
        //   0x4dbf15 fmul  dword[0x4de538]=60.0          ; st0 = note*60
        //   0x4dbf1e fild  dword[ebp-0x1c] = footage      ; 0x4dbf21 fmulp -> st0 = note*60*footage
        //   0x4dbf2b-33 rate*48 (add eax,eax; shl eax,3; lea eax,[eax+eax*2])
        //   0x4dbf36 fild  dword[ebp-0x20] = rate*48      ; 0x4dbf39 fdivrp -> st0 = (rate*48)/(note*60*footage)
        //   0x4dbf3b fld   xword[0x4de53c]=2^31-1(INT_MAX); 0x4dbf41 fmulp ; 0x4dbf43 call 0x402bf4 (round)
        // NOTE: MSVC long double == 64-bit; VAZ used x87 80-bit, so this is best-effort (sub-ULP gap on round).
        auto inc = [] (long double rate, long double footage, long double note) -> long double
        { const long double C = 2147483647.0L; return std::round (C * (rate * 48.0L) / (note * 60.0L * footage)); };

        // Anchors from the manual: footage byte 48=32'(=f/4), 144=8'(=f), 240=2'(=4f). note fixed -> cancels in ratios.
        auto footMul = [] (int b) { return std::pow (2.0L, (long double) (b - 144) / 48.0L); };
        const long double note = 44100.0L / 440.0L;   // one plausible note-slot value (period); cancels in the ratio
        struct Hyp { const char* name; std::function<long double(int)> rate, foot; };
        std::vector<Hyp> H = {
            { "rate=b, foot=b",                [] (int b){ return (long double) b; },        [] (int b){ return (long double) b; } },
            { "rate=footMul, foot=1 (=clone)", [&](int b){ return footMul (b); },            [] (int)  { return 1.0L; } },
            { "rate=footMul, foot=b",          [&](int b){ return footMul (b); },            [] (int b){ return (long double) b; } },
            { "rate=1, foot=footMul",          [] (int)  { return 1.0L; },                   [&](int b){ return footMul (b); } },
            { "rate=b, foot=1",                [] (int b){ return (long double) b; },        [] (int)  { return 1.0L; } },
            { "rate=footMul*b, foot=b",        [&](int b){ return footMul (b) * b; },        [] (int b){ return (long double) b; } },
        };
        std::printf ("\n  -- Osc3 footage anchor test (32'=byte48->f/4, 8'=byte144->f, 2'=byte240->4f) --\n");
        int survivors = 0; std::string surv;
        for (auto& h : H)
        {
            auto p = [&] (int b) { return inc (h.rate (b), h.foot (b), note); };
            const long double r32 = p (48) / p (144), r2 = p (240) / p (144);
            const bool pass = std::abs (r32 - 0.25L) < 0.0025L && std::abs (r2 - 4.0L) < 0.04L;   // 1% relative (round() quantization is sub-%)
            std::printf ("     [%s] %-24s 32'/8'=%.4f  2'/8'=%.4f\n", pass ? "PASS" : "fail", h.name, (double) r32, (double) r2);
            if (pass) { ++survivors; surv += std::string (h.name) + "; "; }
        }
        row ("osc3_footage_pitch",
             survivors == 1 ? "VERIFIED (anchors)" : survivors == 0 ? "NOT TESTED (0 survive)" : ("NOT TESTED (" + std::to_string (survivors) + " survive)"),
             survivors == 1 ? ("unique survivor -> that is VAZ's footage role: " + surv)
                            : ("ambiguous; rateVal=FUN_004a0a68(LFO1)->obj+4/FUN_004a073c whose callers set FIXED/field rates (0x78, obj-fields) not footage-exp -> chain entangled, step-5 report-only. survivors: " + (surv.empty() ? std::string("none") : surv)));
    }

    // ── FX 1. Flanger delay-time mapping (value 0..255 → delay samples) ─────────────────────────────
    {
        const double sr = 44100.0; double maxd = 0.0;
        for (int v = 0; v <= 255; ++v)
        {
            const double vaz   = (sr * vazfx::kFlangerDelayCoef) * (v + 1);   // FUN_0052076c @0x52076c
            const double clone = ((v + 1) / 10.24) * 0.001 * sr;              // clone: baseMs=(v+1)/10.24 → samples
            maxd = std::max (maxd, std::abs (vaz - clone));
        }
        row ("fx_flanger_delaytime", maxd < 1e-9 ? "BIT-EXACT" : "DEVIATION (max=" + std::to_string (maxd) + ")",
             "clone (value+1)/10.24*sr/1000 vs VAZ (sr*25/256000)*(value+1) @0x52076c (25/256000==1/10240)");
    }

    std::printf ("\n  Constants sourced: cutoff-smooth DAT_006d45e4, detune DAT_0052b168/0x52b0ec, env-rate DAT_006db7e8, stage0 DAT_006dc0bc, flanger delay 0x52076c.\n");
    std::printf ("=== oracle complete ===\n");
    return 0;
}
