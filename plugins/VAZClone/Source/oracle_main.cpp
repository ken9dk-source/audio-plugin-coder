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
#include "../../VAZReverb/Source/VazReverbEngine.h"       // the REAL clone reverb engine (tested below)
#include "../../VAZDecimator/Source/VazDecimatorEngine.h"  // the REAL clone decimator engine (tested below)
#include "../../VAZChorus/Source/VazChorusEngine.h"         // the REAL clone chorus engine (tested below)
#include "../../VAZPhaser/Source/VazPhaserEngine.h"          // the REAL clone phaser engine (tested below)
#include "../../VAZDelay/Source/VazDelayEngine.h"            // the REAL clone delay engine (tested below)
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

// Independent reference transcription of the decimator render FUN_0051dbcc @0x51dbcc (exact decompile expressions).
struct RefDecimator
{
    int32_t rate = 2048, mask = -1, bias = 0, coef = 0, acc = 0, hL = 0, hR = 0, sL = 0, sR = 0;
    void frame (int32_t& L, int32_t& R)
    {
        int32_t a = (acc & 0x7ff) + rate; acc = a;                                  // (acc & 0x7ff)+rate
        if (0x7ff < a) { hL = (L & mask) + bias; hR = (R & mask) + bias; }           // S&H + (in & mask)+bias
        int32_t o1 = (int32_t) (((int64_t) coef * (int64_t) (int32_t) ((uint32_t) (sL + hL) << 4)) >> 32);
        L = o1; sL = o1 - hL;
        int32_t o2 = (int32_t) (((int64_t) coef * (int64_t) (int32_t) ((uint32_t) (sR + hR) << 4)) >> 32);
        R = o2; sR = o2 - hR;
    }
};

// Independent reference transcription of the chorus render FUN_00518ad8 @0x518ad8 (delay core lines 3641-3676,
// LFO modes 1/2 lines 3516-3558). iVar/offset style. Tests the mono-line + 3-combined-tap topology bit-exact.
struct RefChorus
{
    int32_t buf[8192]; uint32_t wpos = 0, ph1 = 0, ph2 = 0x80000000u;
    uint32_t inc1 = 0, inc2 = 0; int32_t depth = 0, level = 0, level2 = 0, base = 0, lrPhase = 0, gain = 0;
    int mode1 = 2, mode2 = 2; static constexpr int msk = 8191;
    void clear () { std::memset (buf, 0, sizeof buf); wpos = 0; ph1 = 0; ph2 = 0x80000000u; }
    static int32_t tap12 (uint32_t p, int m, int32_t s)      // LFO tap, modes 1 (trapezoid) / 2 (triangle)
    {
        int32_t sgn = (int32_t) p >> 31, ab = ((int32_t) p ^ sgn) - sgn;
        if (m == 1) { int32_t t = ab - 0x20000000; if (t < 0) t = 0; if (t > 0x40000000) t = 0x40000000;
                      return (int32_t) (((int64_t) t * (int64_t) s) >> 32); }
        return (int32_t) (((int64_t) (ab >> 1) * (int64_t) s) >> 32);
    }
    void frame (int32_t& L, int32_t& R)
    {
        ph1 += inc1; ph2 += inc2;
        int32_t s1 = depth * level;
        int32_t l28 = tap12 (ph1, mode1, s1), l24 = tap12 (ph1 + 0x55555554u, mode1, s1), l20 = tap12 (ph1 + 0xaaaaaaacu, mode1, s1);
        if (level2 > 0) { int32_t s2 = level2 * level;
            l28 += tap12 (ph2, mode2, s2); l24 += tap12 (ph2 + 0x55555554u, mode2, s2); l20 += tap12 (ph2 + 0xaaaaaaacu, mode2, s2); }
        int32_t iv5 = base;
        uint32_t u10 = (wpos - 1) & (uint32_t) msk; wpos = u10;
        uint32_t u7 = ((uint32_t) (l28 >> 16) + (uint32_t) iv5 + u10) & (uint32_t) msk;
        int32_t i8 = buf[u7];
        uint32_t u9 = ((uint32_t) (l20 >> 16) + (uint32_t) iv5 + u10) & (uint32_t) msk;
        int32_t i4 = buf[u9]; i4 += (int32_t) (((int64_t) (int32_t) ((l20 & 0xffff) << 15) * (int64_t) ((buf[(u9 + 1) & msk] - i4) * 2)) >> 32);
        u9 = ((uint32_t) (l24 >> 16) + (uint32_t) iv5 + u10) & (uint32_t) msk;
        int32_t i5 = buf[u9]; i5 += (int32_t) (((int64_t) (int32_t) ((l24 & 0xffff) << 15) * (int64_t) ((buf[(u9 + 1) & msk] - i5) * 2)) >> 32);
        i8 += (int32_t) (((int64_t) (int32_t) ((l28 & 0xffff) << 15) * (int64_t) ((buf[(u7 + 1) & msk] - i8) * 2)) >> 32) + i4 + i5;
        int32_t st = (int32_t) (((int64_t) (lrPhase << 22) * (int64_t) ((i4 - i5) * 4)) >> 32);
        int32_t inL = L, inR = R;
        buf[wpos] = (inL + inR) >> 1;
        L = (int32_t) (((int64_t) (gain << 22) * (int64_t) (st + i8 + inL * -4)) >> 32) + (inL * 4 >> 2);
        R = (int32_t) (((int64_t) (gain << 22) * (int64_t) ((i8 - st) + inR * -4)) >> 32) + (inR * 4 >> 2);
    }
};

// Independent reference transcription of the phaser render FUN_005218d8 @0x5218d8 (iVar/offset style, per channel).
struct RefPhaser
{
    int32_t coefLut[512]; int32_t apL[12] = {0}, apR[12] = {0}, fbL = 0, fbR = 0;
    uint32_t phase = 0, inc = 0; int32_t depth = 128, center = 96, numStages = 4, fbGain = 0, inGain = 0x40000000, mix = 0, lrPhase = 0;
    void loadLut () { for (int i = 0; i < 512; ++i) coefLut[i] = (int32_t) vazfx::kPhaserCoefLUT[i]; }
    static int32_t q (int32_t v, int32_t c) { return (int32_t) (((int64_t) (int32_t) ((uint32_t) v << 2) * (int64_t) c) >> 32); }
    int32_t idx (uint32_t ph) { int32_t s = (int32_t) ph >> 31, ab = ((int32_t) ph ^ s) - s;
        int32_t i = ((ab >> 16) * depth + center * 0x8000) >> 15; if (i < 0) i = 0; if (i > 511) i = 511; return i; }
    int32_t ch (int32_t rin, uint32_t ph, int32_t* ap, int32_t& fb)
    {
        int32_t i1 = coefLut[idx (ph)], i5 = q (rin, inGain), i8 = numStages, i4 = i5 + q (fb, fbGain), i3;
        do { i8--; i3 = ap[i8] - q (i4, i1); ap[i8] = q (i3, i1) + i4; i4 = i3; } while (i8 != 0);
        fb = i3;
        return i5 + (int32_t) (((int64_t) ((uint32_t) mix << 22) * (int64_t) (int32_t) ((uint32_t) (i3 - i5) << 2)) >> 32);
    }
    void frame (int32_t& L, int32_t& R)
    {
        phase += inc;
        L = ch (L, phase, apL, fbL);
        R = ch (R, phase + (uint32_t) (lrPhase * -0x1000000), apR, fbR);
    }
};

// Independent reference transcription of the delay render FUN_0051bba8 @0x51bba8 (iVar/offset style, 3 modes).
struct RefDelay
{
    std::vector<int32_t> buf; int32_t mask = 0; uint32_t wpos = 0; int mode = 0;
    int32_t delayL = 1, delayR = 1, fbL = 0, fbR = 0, dampL = 0, dampR = 0, stateL = 0, stateR = 0, dryL = 0, dryR = 0, wetL = 0, wetR = 0;
    void prep (int nFrames) { buf.assign ((size_t) nFrames * 2, 0); mask = nFrames - 1; wpos = 0; stateL = stateR = 0; }
    static int32_t mh (int32_t a, int32_t b) { return (int32_t) (((int64_t) a * (int64_t) b) >> 32); }
    static int32_t sh (int32_t v, int n) { return (int32_t) ((uint32_t) v << n); }
    void frame (int32_t& L, int32_t& R)
    {
        uint32_t u9 = (wpos - 1) & (uint32_t) mask; wpos = u9;
        int32_t iv6, iv8, iv5, iv7;
        int32_t tL = buf[(size_t) (((uint32_t) delayL + u9) & (uint32_t) mask) * 2 + 0];
        int32_t tR = buf[(size_t) (((uint32_t) delayR + u9) & (uint32_t) mask) * 2 + 1];
        if (mode == 1)
        {
            iv6 = L; iv8 = tL; iv5 = tR;
            iv7 = iv6 + mh (sh (iv5, 2), sh (fbL, 22));
            iv7 = mh (sh (stateL - iv7, 4), dampL) + iv7; stateL = iv7; buf[(size_t) u9 * 2 + 0] = iv7;
            L = mh (dryL, sh (iv6, 2)) + mh (sh (iv8, 2), wetL);
            iv6 = R;
            iv8 = iv6 + mh (sh (iv8, 2), sh (fbR, 22));
            iv8 = mh (sh (stateR - iv8, 4), dampR) + iv8; stateR = iv8; buf[(size_t) u9 * 2 + 1] = iv8;
            R = mh (dryR, sh (iv6, 2)) + mh (sh (iv5, 2), wetR);
        }
        else if (mode < 2)
        {
            iv6 = L; iv8 = tL;
            iv5 = iv6 + mh (sh (iv8, 2), sh (fbL, 22));
            iv5 = mh (sh (stateL - iv5, 4), dampL) + iv5; stateL = iv5; buf[(size_t) u9 * 2 + 0] = iv5;
            L = mh (dryL, sh (iv6, 2)) + mh (sh (iv8, 2), wetL);
            iv6 = R; iv8 = tR;
            iv5 = iv6 + mh (sh (iv8, 2), sh (fbR, 22));
            iv5 = mh (sh (stateR - iv5, 4), dampR) + iv5; stateR = iv5; buf[(size_t) u9 * 2 + 1] = iv5;
            R = mh (dryR, sh (iv6, 2)) + mh (sh (iv8, 2), wetR);
        }
        else
        {
            iv6 = L; iv8 = R; iv5 = tL;
            iv7 = iv6 + iv8 + mh (sh (iv5, 2), sh (fbL, 22));
            iv7 = mh (sh (stateL - iv7, 4), dampL) + iv7; stateL = iv7; buf[(size_t) u9 * 2 + 0] = iv7;
            iv8 = mh (dryL, sh (iv6 + iv8, 2)) + mh (sh (iv5, 2), wetL);
            iv6 = tR;
            iv5 = iv8 + mh (sh (iv6, 2), sh (fbR, 22));
            iv5 = mh (sh (stateR - iv5, 4), dampR) + iv5; stateR = iv5; buf[(size_t) u9 * 2 + 1] = iv5;
            iv6 = mh (dryR, sh (iv8, 2)) + mh (sh (iv6, 2), wetR);
            L = iv6; R = iv6;
        }
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

    // ── FX 1b. Flanger feedback max — clone now 255/256 (was 0.92) matches VAZ (param<<23)/2^31 @0x52059c ──
    {
        double maxd = 0.0;
        for (int v = 0; v <= 255; ++v)
        {
            const double vaz   = (double) ((int64_t) v << 23) / 2147483648.0;   // VAZ [+0x264]<<23 as Q31 fraction
            const double clone = (v / 255.0) * (255.0 / 256.0);                  // clone fFb·255/256 (fFb=v/255)
            maxd = std::max (maxd, std::abs (vaz - clone));
        }
        row ("fx_flanger_feedback", maxd < 1e-9 ? "BIT-EXACT" : "DEVIATION (max=" + std::to_string (maxd) + ")",
             "clone fFb·255/256 == VAZ (param<<23)/2^31 @0x52059c (both = param/256); was a 0.92 guess");
    }

    // ── FX 1c. Flanger BPM-sync ·48 — clone (bpm/60)/periodBeats matches VAZ BPM·48/(60·period) ──────────
    {
        double maxd = 0.0; const double BPM = 140.0;
        for (int P = 1; P <= 96; ++P)                                            // VAZ period units (1/48-note)
        {
            const double vazHz   = BPM * 48.0 / (60.0 * P);                      // SR·60·period/(BPM·48) → rateHz form
            const double cloneHz = (BPM / 60.0) / ((double) P / 48.0);           // clone with periodBeats = period/48
            maxd = std::max (maxd, std::abs (vazHz - cloneHz) / vazHz);
        }
        row ("fx_flanger_sync", maxd < 1e-12 ? "VERIFIED (·48 unit)" : "DEVIATION",
             "VAZ inc=BPM·48·(2^31-1)/(SR·60·period) [·48 @0x5204a9-ae, ·60 @0x520493]; clone (bpm/60)/periodBeats == it (periodBeats=period/48)");
    }

    // ── FX 2. Chorus base-delay — clone now uses VAZ's exact integer formula (FUN_00518fbc @0x518fbc) ──
    {
        const int srI = 44100; long maxd = 0;
        for (int v = 0; v <= 255; ++v)
        {
            const long vaz   = (long) ((srI * 50) / 256000) * (v + 1);   // VAZ: ((sr·0x32)/0x3e800)·(param+1)
            const long clone = (long) ((srI * 50) / 256000) * (v + 1);   // clone PluginProcessor.cpp (same int div)
            long d = vaz - clone; if (d < 0) d = -d; if (d > maxd) maxd = d;
        }
        row ("fx_chorus_basedelay", maxd == 0 ? "BIT-EXACT" : "DEVIATION (max=" + std::to_string (maxd) + ")",
             "clone now = VAZ base = ((sr·50)/256000)·(delay+1) @0x518fbc (was 5+25·f) — FORKERT KONSTANT fixed");
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
             + " peak=" + std::to_string (peakMax) + " (EXACT runtime-dumped coefs)");
    }

    // ── FX 5b. Reverb coefs — setParams uses the EXACT runtime-dumped LUT (was RT60 approximation) ────────
    {
        VazReverbEngine e;
        e.setParams (44100.0, 255, 0, 255);
        bool ok = ((uint32_t) e.combCoef[0] == 0x7B34E281u) && ((uint32_t) e.damp2 == 0x0F9F8172u);
        e.setParams (44100.0, 0, 255, 128);
        ok = ok && ((uint32_t) e.combCoef[0] == 0x5A3C109Du) && ((uint32_t) e.damp2 == 0x02BF40CEu);
        row ("fx_reverb_coef_exact", ok ? "VERIFIED (dumped LUT)" : "DEVIATION",
             "setParams(44100) == runtime dump: size255 coef0=0x7B34E281 damp0 damp2=0x0F9F8172; size0 coef0=0x5A3C109D (vaz_coef_dump.exe)");
    }

    // ── FX 6. Decimator render — clone VazDecimatorEngine vs independent transcription of FUN_0051dbcc ──
    {
        VazDecimatorEngine eng; RefDecimator ref;
        eng.reset();
        // Identical rate/mask/bias/coef on both sides (tests the render: S&H + truncation-crush + DC-block).
        eng.rate = ref.rate = 517;                                   // partial SR reduction (S&H fires ~1/4 samples)
        const int shift = 24 - 10;                                   // 10-bit crush
        eng.mask = ref.mask = (int32_t) ((0xFFFFFFFFu >> shift) << shift);
        eng.bias = ref.bias = (int32_t) (((1u << shift) - 1u) >> 1);
        eng.coef = ref.coef = 0x0FFC0000;                            // DC-blocker coef (< 2^28)
        uint32_t rng = 0x1234abcdu; long maxd = 0;
        for (int i = 0; i < 120000; ++i)
        {
            int32_t s = (int32_t) ((rng = rng * 1664525u + 1013904223u) >> 8) - (1 << 23);   // full-scale noise
            int32_t eL = s, eR = s ^ 0x5a5a, rL = s, rR = s ^ 0x5a5a;
            eng.processFrame (eL, eR); ref.frame (rL, rR);
            long d1 = (long) eL - (long) rL; if (d1 < 0) d1 = -d1;
            long d2 = (long) eR - (long) rR; if (d2 < 0) d2 = -d2;
            if (d1 > maxd) maxd = d1; if (d2 > maxd) maxd = d2;
        }
        row ("fx_decimator_render", maxd == 0 ? "BIT-EXACT" : "DEVIATION (max=" + std::to_string (maxd) + ")",
             "clone VazDecimatorEngine vs independent transcription of FUN_0051dbcc; in&mask+bias + DC-block, 120k-smp noise");
    }

    // ── FX 7. Chorus render — clone VazChorusEngine vs independent transcription of FUN_00518ad8 (modes 1&2) ──
    {
        long maxd = 0;
        for (int mode = 1; mode <= 2; ++mode)      // integer LFO modes (mode 0 = sine LUT is the 80-bit residual)
        {
            VazChorusEngine eng; RefChorus ref;
            eng.clearBuffers(); ref.clear();
            // identical params on both sides:
            eng.inc1 = ref.inc1 = 0x00120000u; eng.inc2 = ref.inc2 = 0x001d0000u;
            eng.depth = ref.depth = 90000;  eng.level = ref.level = 20000;  eng.level2 = ref.level2 = 15000;
            eng.base = ref.base = 700;  eng.lrPhase = ref.lrPhase = 0x30000000; eng.gain = ref.gain = 0x60;
            eng.mode1 = ref.mode1 = mode; eng.mode2 = ref.mode2 = mode;
            uint32_t rng = 0xC0FFEEu;
            for (int i = 0; i < 100000; ++i)
            {
                int32_t s = (i == 0) ? (1 << 21)
                                     : (int32_t) ((rng = rng * 1664525u + 1013904223u) >> 9) - (1 << 21);
                int32_t eL = s, eR = s + 12345, rL = s, rR = s + 12345;
                eng.processFrame (eL, eR); ref.frame (rL, rR);
                long d1 = (long) eL - (long) rL; if (d1 < 0) d1 = -d1;
                long d2 = (long) eR - (long) rR; if (d2 < 0) d2 = -d2;
                if (d1 > maxd) maxd = d1; if (d2 > maxd) maxd = d2;
            }
        }
        row ("fx_chorus_render", maxd == 0 ? "BIT-EXACT" : "DEVIATION (max=" + std::to_string (maxd) + ")",
             "clone VazChorusEngine vs independent transcription of FUN_00518ad8 (mono line + 3 combined taps + stereo), modes 1&2, impulse+noise");
    }

    // ── FX 8. Phaser coef LUT — engine's sr=44100 LUT == the runtime dump (vaz_phaser_coef_lut.h) ──────────
    {
        VazPhaserEngine e; e.setSampleRate (44100.0);
        bool ok = ((uint32_t) e.coefLut[0] == 0x3FFE1880u) && ((uint32_t) e.coefLut[511] == 0x3846D580u)
               && ((uint32_t) e.coefLut[256] == vazfx::kPhaserCoefLUT[256]);
        row ("fx_phaser_coef_lut", ok ? "VERIFIED (dumped LUT)" : "DEVIATION",
             "512-entry allpass coef LUT (FUN_00521aa0 @0x521aa0): coef[0]=0x3FFE1880, coef[511]=0x3846D580 (runtime-dumped, replaces tan())");
    }

    // ── FX 9. Phaser render — clone VazPhaserEngine vs independent transcription of FUN_005218d8 ──────────
    {
        VazPhaserEngine eng; RefPhaser ref;
        eng.clearBuffers(); eng.setSampleRate (44100.0); ref.loadLut();
        // identical params on both sides (LFO moving so the LUT index sweeps):
        eng.inc = ref.inc = 0x00300000u; eng.depth = ref.depth = 200; eng.center = ref.center = 90;
        eng.numStages = ref.numStages = 8; eng.fbGain = ref.fbGain = 90 << 23; eng.inGain = ref.inGain = 0x40000000;
        eng.mix = ref.mix = 200; eng.lrPhase = ref.lrPhase = 40;
        uint32_t rng = 0xBEEF01u; long maxd = 0;
        for (int i = 0; i < 200000; ++i)
        {
            int32_t s = (i == 0) ? (1 << 21) : (int32_t) ((rng = rng * 1664525u + 1013904223u) >> 9) - (1 << 21);
            int32_t eL = s, eR = s ^ 0x1234, rL = s, rR = s ^ 0x1234;
            eng.processFrame (eL, eR); ref.frame (rL, rR);
            long d1 = (long) eL - (long) rL; if (d1 < 0) d1 = -d1;
            long d2 = (long) eR - (long) rR; if (d2 < 0) d2 = -d2;
            if (d1 > maxd) maxd = d1; if (d2 > maxd) maxd = d2;
        }
        row ("fx_phaser_render", maxd == 0 ? "BIT-EXACT" : "DEVIATION (max=" + std::to_string (maxd) + ")",
             "clone VazPhaserEngine vs independent transcription of FUN_005218d8 (N allpass + fb + linear mix + stereo), LFO moving, impulse+noise");
    }

    // ── DIAG. Phaser stages — RMS audibility of a 2↔12 stages change at DEFAULT vs STRONG settings ─────────
    {
        auto runStages = [] (int stagesParam, int depthP, int centerP, int mixP, int fbP, std::vector<double>& out)
        {
            VazPhaserEngine e; e.clearBuffers(); e.setSampleRate (44100.0);
            e.setParams (stagesParam, fbP, false, depthP, centerP, 64, mixP, 255, 175284u);   // rate≈1.8 Hz, gain 0 dB
            uint32_t rng = 0x1357u; out.clear();
            for (int i = 0; i < 40000; ++i)
            {
                int32_t s = (int32_t) ((rng = rng * 1664525u + 1013904223u) >> 9) - (1 << 21);   // steady noise (no impulse)
                int32_t L = s, R = s; e.processFrame (L, R);
                if (i >= 2000) out.push_back ((double) L / 8388608.0);   // skip settling
            }
        };
        auto rmsRatio = [&] (int depthP, int centerP, int mixP, int fbP) -> double
        {
            std::vector<double> a, b; runStages (0, depthP, centerP, mixP, fbP, a); runStages (5, depthP, centerP, mixP, fbP, b);
            double sd = 0, ss = 0; for (size_t i = 0; i < a.size(); ++i) { const double d = a[i] - b[i]; sd += d * d; ss += b[i] * b[i]; }
            return ss > 0 ? std::sqrt (sd / ss) : 0.0;   // RMS(out2−out12) / RMS(out12)
        };
        const double def = rmsRatio (153, 128, 128, 50);   // plugin defaults: freq 0.5, depth 0.6, mix 0.5, fb 0.5
        const double str = rmsRatio (255, 220, 200, 80);   // strong: high freq/depth/mix/fb
        row ("fx_phaser_stages_diag", (def > 0.02) ? "OK (audible at default)" : "WEAK at default (VAZ coef≈1 → narrow low-freq notches)",
             "RMS(st2−st12)/RMS: default=" + std::to_string (def) + "  strong=" + std::to_string (str)
             + "  (engine responds; low default = coef-LUT is 0.88..0.9999 = low-freq allpass)");
    }

    // ── FX. Phaser gain curve — engine gain LUT (FUN_00521d44) == the runtime-dumped anchors ─────────────────
    {
        VazPhaserEngine e;
        e.setParams (1, 0, false, 200, 90, 64, 200, 255, 0u); const uint32_t g255 = (uint32_t) e.inGain;
        e.setParams (1, 0, false, 200, 90, 64, 200, 225, 0u); const uint32_t g225 = (uint32_t) e.inGain;
        e.setParams (1, 0, false, 200, 90, 64, 200,   0, 0u); const uint32_t g0   = (uint32_t) e.inGain;
        const bool ok = (g255 == 0x40000000u) && (g225 == 0x2D413CCDu) && (g0 == 0x035D13F3u);
        row ("fx_phaser_gain_curve", ok ? "VERIFIED (dumped LUT)" : "DEVIATION",
             "gain FUN_00521d44: inGain=2^((b−255)/60)·2^30 — b=255→0dB(0x40000000), b=225→−3dB(0x2D413CCD), b=0→−25.6dB(0x035D13F3)");
    }

    // ── DIAG. Phaser param sweep — feedback/depth/mix/gain RESPONSE across the full range (not just default),
    //    each point bit-exact vs the independent FUN_005218d8 transcription so the numbers reflect VAZ's render. ──
    {
        // render steady noise through a fresh engine + RefPhaser (same params); collect engine L after settling,
        // track engine-vs-ref bit-exactness (maxd). stages=4, rate≈1.8 Hz, moving LFO so the notch sweeps.
        auto render = [] (int fbP, int depthP, int centerP, int mixP, int gainB, long& maxdOut) -> std::vector<double>
        {
            VazPhaserEngine e; RefPhaser r; e.clearBuffers(); e.setSampleRate (44100.0); r.loadLut();
            e.setParams (1, fbP, false, depthP, centerP, 64, mixP, gainB, 175284u);
            r.numStages = e.numStages; r.fbGain = e.fbGain; r.depth = e.depth; r.center = e.center;
            r.lrPhase = e.lrPhase; r.mix = e.mix; r.inGain = e.inGain; r.inc = e.inc;
            uint32_t rng = 0x1357u; std::vector<double> out; long maxd = 0;
            for (int i = 0; i < 40000; ++i)
            {
                int32_t s = (int32_t) ((rng = rng * 1664525u + 1013904223u) >> 9) - (1 << 21);
                int32_t eL = s, eR = s, rL = s, rR = s;
                e.processFrame (eL, eR); r.frame (rL, rR);
                long d = (long) eL - (long) rL; if (d < 0) d = -d; if (d > maxd) maxd = d;
                if (i >= 2000) out.push_back ((double) eL / 8388608.0);
            }
            if (maxd > maxdOut) maxdOut = maxd;
            return out;
        };
        auto rel = [] (const std::vector<double>& a, const std::vector<double>& b) -> std::string
        {
            double sd = 0, ss = 0; for (size_t i = 0; i < a.size() && i < b.size(); ++i) { const double d = a[i]-b[i]; sd += d*d; ss += b[i]*b[i]; }
            return std::to_string (ss > 0 ? std::sqrt (sd/ss) : 0.0).substr (0, 5);
        };
        auto rmsRatio = [] (const std::vector<double>& v, double ref) -> std::string
        {
            double r = 0; for (double x : v) r += x*x; r = v.empty() ? 0 : std::sqrt (r / (double) v.size());
            return std::to_string (ref > 0 ? r/ref : 0.0).substr (0, 5);
        };
        long maxd = 0;
        // feedback: deviation from fb=0 as feedback rises (depth/center/mix fixed, gain 0 dB)
        const auto fb0 = render (0, 200, 90, 200, 255, maxd);
        std::string fbs; for (int fb : { 25, 50, 75, 100 }) fbs += rel (render (fb, 200, 90, 200, 255, maxd), fb0) + " ";
        // depth: deviation from depth=0 (static notch) as the LFO sweep widens
        const auto dp0 = render (60, 0, 90, 200, 255, maxd);
        std::string dps; for (int dp : { 64, 128, 192, 255 }) dps += rel (render (60, dp, 90, 200, 255, maxd), dp0) + " ";
        // mix: deviation from mix=0 (pure dry) as wet rises
        const auto mx0 = render (60, 200, 90, 0, 255, maxd);
        std::string mxs; for (int mx : { 64, 128, 192, 255 }) mxs += rel (render (60, 200, 90, mx, 255, maxd), mx0) + " ";
        // gain: output RMS ratio vs 0 dB — should track 2^((b−255)/60) = {0.231, 0.483, 0.707, 1.0}
        const auto g255 = render (60, 200, 90, 200, 255, maxd);
        double rms255 = 0; for (double x : g255) rms255 += x*x; rms255 = std::sqrt (rms255 / (double) g255.size());
        std::string gns; for (int gb : { 128, 192, 225, 255 }) gns += rmsRatio (render (60, 200, 90, 200, gb, maxd), rms255) + " ";
        row ("fx_phaser_param_sweep", maxd == 0 ? "BIT-EXACT + full range" : "DEVIATION (max=" + std::to_string (maxd) + ")",
             "vs ref @every point maxd=" + std::to_string (maxd) + "; fb{25,50,75,100}=" + fbs + "| depth=" + dps + "| mix=" + mxs + "| gainRMS=" + gns);
    }

    // ── FX 10. Delay render — clone VazDelayEngine vs independent transcription of FUN_0051bba8, all 3 modes ──
    {
        long maxd = 0;
        for (int mode = 0; mode <= 2; ++mode)          // 0 Stereo · 1 Ping-Pong · 2 serial Double
        {
            VazDelayEngine eng; RefDelay ref;
            eng.prepare (44100.0); ref.prep ((int) (eng.buf.size() / 2));   // same pow2 buffer/mask
            eng.mode = ref.mode = mode;
            eng.delayL = ref.delayL = 11025; eng.delayR = ref.delayR = 8267;   // ~250/187 ms taps
            eng.fbL = ref.fbL = 170; eng.fbR = ref.fbR = 150;                  // feedback (fb/256)
            eng.dampL = ref.dampL = 0x0C000000; eng.dampR = ref.dampR = 0x0A000000;   // Q28 damping
            eng.dryL = ref.dryL = 0x40000000; eng.dryR = ref.dryR = 0x40000000;       // Q30 unity dry
            eng.wetL = ref.wetL = 0x20000000; eng.wetR = ref.wetR = 0x20000000;       // Q30 0.5 wet
            uint32_t rng = 0xDECAF0u;
            for (int i = 0; i < 200000; ++i)           // long enough for the feedback tail
            {
                int32_t s = (i == 0) ? (1 << 21) : (int32_t) ((rng = rng * 1664525u + 1013904223u) >> 9) - (1 << 21);
                int32_t eL = s, eR = s ^ 0x7f, rL = s, rR = s ^ 0x7f;
                eng.processFrame (eL, eR); ref.frame (rL, rR);
                long d1 = (long) eL - (long) rL; if (d1 < 0) d1 = -d1;
                long d2 = (long) eR - (long) rR; if (d2 < 0) d2 = -d2;
                if (d1 > maxd) maxd = d1; if (d2 > maxd) maxd = d2;
            }
        }
        row ("fx_delay_render", maxd == 0 ? "BIT-EXACT" : "DEVIATION (max=" + std::to_string (maxd) + ")",
             "clone VazDelayEngine vs independent transcription of FUN_0051bba8 (stereo/ping-pong/serial-double + damped fb), 3 modes, impulse+noise");
    }

    // ── FX 10b. Delay tone→damp LUT (dumped) + linear-ms delay-time (both extracted, replace approximations) ──
    {
        VazDelayEngine e; e.prepare (44100.0);
        const bool okD = ((uint32_t) e.dampLut[0] == 0x0FFFBA21u) && ((uint32_t) e.dampLut[255] == 0x0FD37981u);
        const bool okT = (e.delaySamplesFromMs (100.0) == 4402) && (e.delaySamplesFromMs (500.0) == 22002);   // ms·44+2
        row ("fx_delay_maps", (okD && okT) ? "VERIFIED (dumped damp + linear ms)" : "DEVIATION",
             "tone→damp runtime-dumped k[0]=0x0FFFBA21 k[255]=0x0FD37981 (FUN_0051c298); delay-time LINEAR ms·(sr/1000)+((sr/1000)>>4) (FUN_0051c1cc)");
    }

    // ── FX 11. Autopan pan-law — verified LINEAR from VAZ's code (NOT the assumed equal-power) ──────────────
    {
        // VAZ pan table (ctor FUN_00517ae4 @0x517b1c): table[i] = (i/255)·K1·K2, K1=0.5 @0x517c3c, K2=2^31−1 @0x517c48,
        // NO cos/sin/sqrt. So table[i]/2^30 = i/255 (linear gain). render: gainR = table[idx], gainL = table[255−idx].
        double maxd = 0.0;
        for (int i = 0; i <= 255; ++i)
        {
            const double vaz   = ((double) i / 255.0) * 0.5 * 2147483647.0 / 1073741824.0;   // VAZ table[i]/2^30 = gain
            const double clone = (double) i / 255.0;                                          // clone's new linear gain (gR=pan)
            maxd = std::max (maxd, std::abs (vaz - clone));
        }
        row ("fx_autopan_panlaw", maxd < 1e-6 ? "VERIFIED (LINEAR, not equal-power)" : "DEVIATION",
             "VAZ pan table[i]=(i/255)·0.5·(2^31−1) LINEAR (ctor @0x517b1c, no cos/sin); clone fixed to gL=1−pan gR=pan (was cos/sin)");
    }

    std::printf ("\n  Constants sourced: cutoff-smooth DAT_006d45e4, detune DAT_0052b168/0x52b0ec, env-rate DAT_006db7e8, stage0 DAT_006dc0bc, flanger delay 0x52076c, chorus delay 0x518fbc.\n");
    std::printf ("=== oracle complete ===\n");
    return 0;
}
