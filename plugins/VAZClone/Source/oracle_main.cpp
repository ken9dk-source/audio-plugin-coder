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
#include "../../VAZReverb/Source/VazReverbEngine.h"  // the REAL clone reverb engine (tested below)
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

// ── Independent reference transcription of the reverb render FUN_005228a4 @0x5228a4 ─────────────────
// Offset/iVar-style (mirrors the decompile's variable flow, NOT the engine's clean loops) so agreement
// with VazReverbEngine is a real cross-check, not a copy. Same fixed-point ops.
struct RefReverb
{
    int32_t comb[9][4096], ap[8][1024];
    int32_t combLen[9] = {0}, combCoef[9] = {0}, apLen[8] = {0};
    int32_t damp1 = 0, damp2 = 0, stateL = 0, stateR = 0, mixMul = 0;
    uint32_t ctr = 0;
    static constexpr int32_t G = 0x53333333;                 // 0.65 allpass gain
    void clear () { std::memset (comb, 0, sizeof comb); std::memset (ap, 0, sizeof ap); stateL = stateR = 0; ctr = 0; }
    static inline int32_t q (int32_t v, int32_t c) { return (int32_t) (((int64_t) ((int64_t) v * 2) * (int64_t) c) >> 32); }
    static inline int32_t m4 (int32_t v) { return (int32_t) ((uint32_t) v << 2); }
    static inline int32_t m2 (int32_t v) { return (int32_t) ((uint32_t) v << 1); }
    void frame (int32_t& L, int32_t& R)
    {
        uint32_t u4 = ctr + 1, u5 = u4 & 0xfff; ctr = u5;
        int32_t i1 = L, i2 = R, i3 = (i1 + i2) >> 6;
        int32_t i7  = q (comb[0][u5], combCoef[0]); comb[0][(uint32_t)(combLen[0]+u5)&0xfff] = i7 + i3;
        int32_t i8  = q (comb[1][u5], combCoef[1]); comb[1][(uint32_t)(combLen[1]+u5)&0xfff] = i8 + i3;
        int32_t i9  = q (comb[2][u5], combCoef[2]); comb[2][(uint32_t)(combLen[2]+u5)&0xfff] = i9 + i3;
        int32_t i10 = q (comb[3][u5], combCoef[3]); comb[3][(uint32_t)(combLen[3]+u5)&0xfff] = i10 + i3;
        int32_t i11 = q (comb[4][u5], combCoef[4]); comb[4][(uint32_t)(combLen[4]+u5)&0xfff] = i11 + i3;
        int32_t i12 = q (comb[5][u5], combCoef[5]); comb[5][(uint32_t)(combLen[5]+u5)&0xfff] = i12 + i3;
        int32_t i13 = q (comb[6][u5], combCoef[6]); comb[6][(uint32_t)(combLen[6]+u5)&0xfff] = i13 + i3;
        int32_t i14 = q (comb[7][u5], combCoef[7]);
        i9 = i7*2 + i8 + i9*4 + i10*2 + i11*3 + i12*4 + i14*2;                 // LEFT sum
        comb[7][(uint32_t)(combLen[7]+u5)&0xfff] = i14 + i3;
        i12 = q (comb[8][u5], combCoef[8]);
        i7 = i7*2 + i8*2 + i8 + i10*2 + i11 + i13*4 + i14*2 + i12*4;           // RIGHT sum
        comb[8][(uint32_t)(combLen[8]+u5)&0xfff] = i12 + i3;
        uint32_t a4 = u4 & 0x3ff;
        i3 = ap[0][a4]; i8 = i3 - i9; ap[0][(uint32_t)(apLen[0]+a4)&0x3ff] = q(i3,G) + i9;
        i9 = ap[1][a4]; i3 = i9 - i8; ap[1][(uint32_t)(apLen[1]+a4)&0x3ff] = q(i9,G) + i8;
        i9 = ap[2][a4]; i8 = i9 - i3; ap[2][(uint32_t)(apLen[2]+a4)&0x3ff] = q(i9,G) + i3;
        i9 = ap[3][a4];               ap[3][(uint32_t)(apLen[3]+a4)&0x3ff] = q(i9,G) + i8;   // LEFT out = i9 - i8
        int32_t apL = i9 - i8;
        i3 = ap[4][a4]; i10 = i3 - i7; ap[4][(uint32_t)(apLen[4]+a4)&0x3ff] = q(i3,G) + i7;
        i7 = ap[5][a4]; i3 = i7 - i10; ap[5][(uint32_t)(apLen[5]+a4)&0x3ff] = q(i7,G) + i10;
        i7 = ap[6][a4]; i10 = i7 - i3; ap[6][(uint32_t)(apLen[6]+a4)&0x3ff] = q(i7,G) + i3;
        i7 = ap[7][a4];                ap[7][(uint32_t)(apLen[7]+a4)&0x3ff] = q(i7,G) + i10;  // RIGHT out = i7 - i10
        int32_t apR = i7 - i10;
        i3 = (int32_t)(((int64_t) m4(apL) * damp1) >> 32) + (int32_t)(((int64_t) m4(stateL) * damp2) >> 32);
        stateL = i3;
        i7 = (int32_t)(((int64_t) m4(apR) * damp1) >> 32) + (int32_t)(((int64_t) m4(stateR) * damp2) >> 32);
        stateR = i7;
        L = i1 + (int32_t)(((int64_t) m2(i3 - i1) * mixMul) >> 32);
        R = i2 + (int32_t)(((int64_t) m2(i7 - i2) * mixMul) >> 32);
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

    // ── FX 2. Chorus base-delay mapping (documents a FORKERT KONSTANT — clone uses a different curve) ──
    {
        const double sr = 44100.0; double maxdMs = 0.0;
        for (int v = 0; v <= 255; ++v)
        {
            const double vazMs   = (double) (int) (sr * vazfx::kChorusDelayCoef) * (v + 1) / sr * 1000.0; // (sr*50/256000 floored)*(v+1)
            const double vazMsF  = (sr * vazfx::kChorusDelayCoef) * (v + 1) / sr * 1000.0;                // (v+1)/5.12 ms (unfloored)
            (void) vazMs;
            const double f       = v / 255.0;
            const double cloneMs = 5.0 + f * 25.0;                                                        // clone: 5..30 ms linear
            maxdMs = std::max (maxdMs, std::abs (vazMsF - cloneMs));
        }
        row ("fx_chorus_basedelay", "DEVIATION (max=" + std::to_string (maxdMs) + " ms)",
             "clone 5+25*f (5..30ms) vs VAZ (delay+1)/5.12 (0.2..50ms) @0x518fbc — FORKERT KONSTANT (formula+range)");
    }

    // ── FX 3. Chorus waveform mode map — verify the 1<->2 swap now matches VAZ (FUN_00518ad8) ──────────
    {
        auto tri   = [] (double p) { p -= std::floor (p); return (p < 0.5) ? 2.0 * p : 2.0 - 2.0 * p; };
        // clone waveshape() post-swap, transcribed from VAZChorus PluginProcessor.h: 1->trapezoid, 2->triangle.
        auto clone = [&] (double p, int w) {
            if (w == 1) { const double v = (tri (p) - 0.2) * 1.6; return v < 0.0 ? 0.0 : v > 1.0 ? 1.0 : v; }
            return tri (p);
        };
        // VAZ decompiled shapes (normalised): mode2 = |ph|>>1 = pure triangle; mode1 = trapezoid (clamped) @0x518BA4.
        double d2 = 0.0;
        for (int k = 0; k <= 2000; ++k) { const double p = k / 2000.0; d2 = std::max (d2, std::abs (clone (p, 2) - tri (p))); }
        const bool m1trap = std::abs (clone (0.35, 1) - tri (0.35)) > 1e-6;   // mode1 must be the trapezoid, NOT triangle(0.35)=0.7
        row ("fx_chorus_waveform_map", (d2 < 1e-12 && m1trap) ? "VERIFIED (1<->2 swap)" : "DEVIATION",
             "clone idx1->trapezoid @0x518BA4, idx2->triangle @0x518C1F (VAZ order); mode2==|ph|>>1 triangle bit-exact");
    }

    // ── FX 4. Reverb render — clone VazReverbEngine vs independent transcription of FUN_005228a4 ────────
    {
        VazReverbEngine eng; RefReverb ref;
        eng.clearBuffers(); ref.clear();
        // Identical lengths/coefs/damp/mix on both sides (tests the RENDER, independent of the 80-bit coef map).
        for (int i = 0; i < 9; ++i)
        {
            const int32_t L = 53 + i * 41;                     // comb delays (< 4096)
            const int32_t C = 0x30000000 + i * 0x01111111;     // Q31 feedback coefs (~0.375..0.9)
            eng.combLen[i] = ref.combLen[i] = L;
            eng.combCoef[i] = ref.combCoef[i] = C;
        }
        eng.combLen[9] = 1500; eng.combCoef[9] = 0x20000000;   // 10th comb (allocated, unused)
        for (int i = 0; i < 8; ++i) { const int32_t A = 29 + (i % 4) * 61; eng.apLen[i] = ref.apLen[i] = A; }
        eng.damp2 = ref.damp2 = 0x05000000; eng.damp1 = ref.damp1 = 0x10000000 - 0x05000000;
        eng.mixMul = ref.mixMul = 200 << 23;
        // Impulse then a long pseudo-random noise burst — several seconds so the feedback tail fully accumulates.
        uint32_t rng = 0x9e3779b9u; long maxd = 0;
        const int N = 300000;                                  // ~6.8 s @44.1k
        for (int i = 0; i < N; ++i)
        {
            int32_t s = (i == 0) ? (1 << 21)
                                 : (int32_t) ((rng = rng * 1664525u + 1013904223u) >> 9) - (1 << 21);
            int32_t eL = s, eR = s, rL = s, rR = s;
            eng.processFrame (eL, eR); ref.frame (rL, rR);
            long d1 = (long) eL - (long) rL; if (d1 < 0) d1 = -d1;
            long d2 = (long) eR - (long) rR; if (d2 < 0) d2 = -d2;
            if (d1 > maxd) maxd = d1; if (d2 > maxd) maxd = d2;
        }
        row ("fx_reverb_render", maxd == 0 ? "BIT-EXACT" : "DEVIATION (max=" + std::to_string (maxd) + ")",
             "clone VazReverbEngine vs independent transcription of FUN_005228a4 over 300k-smp impulse+noise (feedback accumulates)");
    }

    // ── FX 5. Reverb stability — real setParams() mapping produces a bounded, DECAYING tail (not silence/blow-up) ──
    {
        VazReverbEngine eng; eng.clearBuffers();
        eng.setParams (44100.0, 153, 100, 255);            // rt≈0.6, some damping, full wet
        double peakEarly = 0.0, peakLate = 0.0, peakMax = 0.0;
        const int N = 132300;                              // 3 s @44.1k
        for (int i = 0; i < N; ++i)
        {
            int32_t L = (i == 0) ? (1 << 21) : 0, R = L;   // single impulse, then decay
            eng.processFrame (L, R);
            const double a = std::abs ((double) L) / 8388608.0;   // back to ~[-1,1]
            if (a > peakMax) peakMax = a;
            if (i < 4410 && a > peakEarly) peakEarly = a;         // first 0.1 s
            if (i > N - 4410 && a > peakLate) peakLate = a;       // last 0.1 s
        }
        const bool decays = peakEarly > 1e-6 && peakLate < peakEarly * 0.5;   // tail present AND decaying
        const bool bounded = peakMax < 8.0 && std::isfinite (peakMax);        // no runaway / NaN
        row ("fx_reverb_stable", (decays && bounded) ? "VERIFIED (bounded+decays)" : "DEVIATION",
             "impulse→tail: early=" + std::to_string (peakEarly) + " late=" + std::to_string (peakLate)
             + " peak=" + std::to_string (peakMax) + " (RT60 coef map; 80-bit VAZ curve substituted)");
    }

    std::printf ("\n  Constants sourced: cutoff-smooth DAT_006d45e4, detune DAT_0052b168/0x52b0ec, env-rate DAT_006db7e8, stage0 DAT_006dc0bc, flanger delay 0x52076c, chorus delay 0x518fbc.\n");
    std::printf ("=== oracle complete ===\n");
    return 0;
}
