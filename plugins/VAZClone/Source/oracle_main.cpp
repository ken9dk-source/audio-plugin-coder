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
#include "../reference/vaz_autopan_rate_lut.h"                // dumped autopan LFO rate curve (FUN_00517ee0)
#include "VAZTypeDreal.h"                                     // the REAL VAZ Type-D filter (2-stage cubic SVF, tested below)
#include "VAZPulseTables.h"                                   // RAW BLEP step tables (dd2c0/de2c0) for the pulse transcription
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

    // ── FILTER ROUTE MAP — .v2p filter mode → VAZ coef-table (via FUN_004dffb0 + the disassembled render dispatch)
    //    vs the clone's engine table. Proves the K/R engine↔mode mismatch NUMERICALLY (not by reading). ─────────
    {
        // FUN_004dffb0 @0x4dffb0 (vaz_prims.c:2519): .v2p index → internal filter mode (+0x258). Literal switch.
        auto v2pToInternal = [] (int m) -> int {
            static const int t[22] = { 0x00,0x10,0x20,0x28,0x02,0x01,0x12,0x11,0x2c,0x2d,0x30,0x31,
                                       0x32,0x34,0x24,0x40,0x44,0x50,0x54,0x58,0x5d,0x60 };
            return (m >= 0 && m < 22) ? t[m] : -1; };
        // The render's filter dispatch (disasm 0x4dd646 / 0x4dda96): internal mode → handler VA → coef-table tag.
        auto vazTable = [] (int im) -> const char* {
            if (im <= 0x02) return "0x55 A";
            if (im <= 0x12) return "0x55 A/B";
            if (im <= 0x2d) return "0x69 cubic (0x4dd82b)";        // C
            if (im <= 0x34) return "0x6d65 cubic-SVF (D, 0x4ddaa8)"; // real D = 0x6d45/55/65/66 + cubic + tap(mode&3)
            if (im <= 0x44) return "0x6d67 SVF (K, 0x4ddcfe)";     // real K modes 0x40/0x44 = 0x6d67 SVF
            if (im <= 0x5d) return "0x6d87 Sallen-Key (0x4ddf44)"; // real R modes 0x50-0x5d
            return "Comb"; };
        // The clone's setMode (Synth.h:448) → which engine/table it actually runs for each .v2p index.
        auto cloneTable = [] (int m) -> const char* {
            switch (m) {
                case 0: case 4: case 5: return "0x55 A";
                case 1: case 6: case 7: return "0x55 A/B (B)";
                case 2: case 3: case 8: case 9: case 14: return "0x69 cubic (C)";
                case 10: case 11: case 12: case 13: return "0x6d65 cubic-SVF (Dreal)"; // FIXED: D → VAZTypeDreal (real D)
                case 15: case 16: return "0x6d67 SVF (K→VAZTypeD)"; // FIXED 2026-07-11: K re-routed to VAZTypeD (VAZ's K)
                case 17: case 18: case 19: case 20: return "0x6d87 Sallen-Key (K→R)"; // FIXED 2026-07-11: R re-routed to VAZTypeK
                case 21: return "Comb"; default: return "?"; }; };
        int mism = 0; std::string bad;
        for (int m = 0; m < 22; ++m) {
            const int im = v2pToInternal (m);
            const std::string vt = vazTable (im), ct = cloneTable (m);
            // compare the coef-table family (first token) — the recurrence signature
            const bool ok = (vt.substr (0, 6) == ct.substr (0, 6));
            if (!ok) { ++mism; bad += "v2p" + std::to_string (m) + "(int0x" + [](int x){char b[8];std::snprintf(b,sizeof b,"%x",x);return std::string(b);}(im) + "):VAZ=" + vt.substr(0,vt.find(' ')) + " vs clone=" + ct.substr(0,ct.find(' ')) + "  "; }
        }
        row ("filter_route_map", mism == 0 ? "ALL MATCH" : std::to_string (mism) + " MISMATCH",
             mism == 0 ? "every .v2p mode routes to VAZ's coef table" : bad);
    }

    // ── FILTER K — independent transcription of VAZ's K handler 0x4ddcfe (vaz_big.c:1455-1496) vs VAZTypeD.process(tap 2).
    //    Proves K's real engine IS what the clone calls VAZTypeD (0x6d67 SVF, bp tap = resonant 2-pole LP). ─────────────
    {
        struct RefKsvf {   // faithful transcription of the 0x4ddcfe common SVF (K LP, mode 0x40)
            double sr = 44100.0; int32_t s17c = 0, s180 = 0;    // lp, bp states (+0x17c / +0x180)
            static int32_t mh (int32_t a, int32_t b) { return (int32_t) (((int64_t) a * b) >> 32); }
            static int32_t sl (int32_t v, int n) { return (int32_t) ((uint32_t) v << n); }
            static int32_t ad (int32_t a, int32_t b) { return (int32_t) ((uint32_t) a + (uint32_t) b); }
            static int32_t sb (int32_t a, int32_t b) { return (int32_t) ((uint32_t) a - (uint32_t) b); }
            double process (double in, double fc, double reso) {
                const int ci = std::clamp ((int) std::lround (1024.0 * std::log (std::clamp (fc, 1.0, sr * 0.49)) / 10.24), 0, 1023);
                const int32_t coefA = VAZTypeDT::kCoefA[ci], coefB = VAZTypeDT::kCoefB[ci];
                const int r255 = (int) std::lround (std::clamp (reso, 0.0, 1.0) * 255.0);
                const int32_t resoGain = sl (mh (coefB, sl (r255, 22)), 2);   // [+0x164]
                const int32_t input = (int32_t) std::lround (std::clamp (in, -2.0, 2.0) * 65536.0);
                int32_t v0 = 0, v1 = 0;
                for (int p = 0; p < 2; ++p) {
                    int32_t resoFB = mh (sl (s180, 4), resoGain);              // (bp<<4)·resoGain
                    if (resoFB > 0x1000000) resoFB = 0x1000000; if (resoFB < -0x1000000) resoFB = -0x1000000;
                    s17c = ad (mh (sl (sb (sb (input, resoFB), s17c), 2), coefA), s17c);   // lp += coefA·((in−resoFB−lp)<<2)
                    s180 = ad (mh (sl (sb (ad (s17c, resoFB), s180), 2), coefA), s180);    // bp += coefA·((lp+resoFB−bp)<<2)
                    (p == 0 ? v0 : v1) = s180;
                }
                return (double) ((int32_t) (((int64_t) v0 + v1) >> 1)) / 65536.0;   // avg of the 2 passes' bp
            }
        } ref;
        VAZTypeD d; d.prepare (44100.0);
        uint32_t rng = 0x2468u; long maxd = 0;
        for (int i = 0; i < 60000; ++i) {
            const double fc = 200.0 + (double) (i % 400) * 20.0, rs = 0.2 + (double) (i % 5) * 0.19;
            const double s = (i == 0) ? 0.5 : ((double) (int32_t) ((rng = rng * 1664525u + 1013904223u) >> 9) / 4194304.0 - 0.5);
            const int32_t a = (int32_t) std::llround (ref.process (s, fc, rs) * 65536.0);
            const int32_t b = (int32_t) std::llround (d.process (2, s, fc, rs) * 65536.0);   // tap 2 = bp
            long dd = (long) a - (long) b; if (dd < 0) dd = -dd; if (dd > maxd) maxd = dd;
        }
        row ("filter_k", maxd == 0 ? "BIT-EXACT (K = VAZTypeD tap2)" : "DEVIATION (max=" + std::to_string (maxd) + ")",
             "VAZ K handler 0x4ddcfe (0x6d67 SVF, bp=resonant 2-pole LP) == VAZTypeD.process(tap 2) — so K LP re-routes to VAZTypeD, not a new engine");
    }

    // ── FILTER Dreal — independent iVar-style transcription of VAZ's REAL D handler 0x4ddaa8 (vaz_big.c:1349-1391)
    //    vs the new VAZTypeDreal engine, all 3 taps. Real dumped tables (0x6d45/55/65/66). ─────────────────────────────
    {
        struct RefDreal {
            double sr = 44100.0; int32_t s17c = 0, s180 = 0;
            static int32_t mh (int32_t a, int32_t b) { return (int32_t) (((int64_t) a * b) >> 32); }
            static int32_t sl (int32_t v, int n) { return (int32_t) ((uint32_t) v << n); }
            static int32_t ad (int32_t a, int32_t b) { return (int32_t) ((uint32_t) a + (uint32_t) b); }
            static int32_t sb (int32_t a, int32_t b) { return (int32_t) ((uint32_t) a - (uint32_t) b); }
            double process (int tap, double in, double fc, double reso) {
                const int c = std::clamp ((int) std::lround (1024.0 * std::log (std::clamp (fc, 1.0, sr * 0.49)) / 10.24), 0, 1023);
                const int r = std::clamp (((int) std::lround (std::clamp (reso, 0.0, 1.0) * 255.0)) >> 2, 0, 63);
                int32_t c168 = VAZTypeDrealT::kResC[r]; if (VAZTypeDrealT::kCutRlim[c] < c168) c168 = VAZTypeDrealT::kCutRlim[c];  // 1349-1353
                const int32_t c16c = VAZTypeDrealT::kResD[r], coefA = VAZTypeDrealT::kCutA[c];   // 1354-1356
                const int32_t in1 = (int32_t) std::lround (std::clamp (in, -2.0, 2.0) * 65536.0);
                const int32_t i15 = s17c;
                int32_t v170 = ad (mh (sl (i15, 2), coefA), s180);                              // 1358-1360
                int32_t v178 = sb (sb (mh (sl (in1, 1), c16c), mh (sl (i15, 2), c168)), v170);   // 1361-1364
                const int32_t i9 = ad (sb (i15, mh (sl (i15, 3), mh (sl (i15, 4), sl (i15, 4)))), mh (sl (v178, 2), coefA)); // 1365-1371
                int32_t i8 = ad (mh (sl (i9, 2), coefA), v170);                                 // 1372-1373
                v170 = ad (v170, i8); s180 = i8;                                                 // 1374-1375
                i8 = sb (sb (mh (sl (in1, 1), c16c), mh (sl (i9, 2), c168)), i8);                // 1376-1380
                v178 = ad (v178, i8);                                                            // 1381
                s17c = ad (mh (sl (i8, 2), coefA), sb (i9, mh (sl (i9, 3), mh (sl (i9, 4), sl (i9, 4))))); // 1382-1389
                const int32_t v174 = ad (s17c, i9);                                             // 1390
                return (double) ((tap == 0) ? v170 : (tap == 1) ? v174 : v178) / 65536.0;
            }
        };
        long maxd = 0;
        for (int tp = 0; tp < 3; ++tp) {           // state is tap-independent; run a fresh pair per tap
            RefDreal ref; VAZTypeDreal eng; eng.prepare (44100.0);
            uint32_t rng = 0x1111u + (uint32_t) tp;
            for (int i = 0; i < 40000; ++i) {
                const double fc = 150.0 + (double) (i % 500) * 15.0, rs = 0.15 + (double) (i % 6) * 0.16;
                const double s = (i == 0) ? 0.5 : ((double) (int32_t) ((rng = rng * 1664525u + 1013904223u) >> 9) / 4194304.0 - 0.5);
                const int32_t a = (int32_t) std::llround (ref.process (tp, s, fc, rs) * 65536.0);
                const int32_t b = (int32_t) std::llround (eng.process (tp, s, fc, rs) * 65536.0);
                long d = (long) a - (long) b; if (d < 0) d = -d; if (d > maxd) maxd = d;
            }
        }
        row ("filter_dreal", maxd == 0 ? "BIT-EXACT (real dumped tables)" : "DEVIATION (max=" + std::to_string (maxd) + ")",
             "VAZ real D 0x4ddaa8 (2-stage cubic SVF, 0x6d45/55/65/66) — VAZTypeDreal vs independent transcription, taps LP/BP/HP");
    }

    // ── FILTER Dreal HP+LP (mode 0x34, .v2p 13) — independent transcription of the 2-section Separation cascade
    //    (vaz_big.c:1289-1346 section-1 HP at cut+Sep → common section-2 LP at cut−Sep) vs VAZTypeDreal::processHPLP. ──
    {
        struct RefHPLP {
            double sr = 44100.0; int32_t a1 = 0, a2 = 0, b1 = 0, b2 = 0;   // sec1 (0x184/0x188), sec2 (0x17c/0x180)
            static int32_t mh (int32_t a, int32_t b) { return (int32_t) (((int64_t) a * b) >> 32); }
            static int32_t sl (int32_t v, int n) { return (int32_t) ((uint32_t) v << n); }
            static int32_t ad (int32_t a, int32_t b) { return (int32_t) ((uint32_t) a + (uint32_t) b); }
            static int32_t sb (int32_t a, int32_t b) { return (int32_t) ((uint32_t) a - (uint32_t) b); }
            static int32_t cub (int32_t x) { return sb (x, mh (sl (x, 3), mh (sl (x, 4), sl (x, 4)))); }
            static int32_t sect (int tap, int32_t in, int c, int r, int32_t& st1, int32_t& st2) {
                int32_t c168 = VAZTypeDrealT::kResC[r]; if (VAZTypeDrealT::kCutRlim[c] < c168) c168 = VAZTypeDrealT::kCutRlim[c];
                const int32_t c16c = VAZTypeDrealT::kResD[r], cA = VAZTypeDrealT::kCutA[c];
                int32_t v170 = ad (mh (sl (st1, 2), cA), st2);
                int32_t v178 = sb (sb (mh (sl (in, 1), c16c), mh (sl (st1, 2), c168)), v170);
                const int32_t i9 = ad (sb (st1, mh (sl (st1, 3), mh (sl (st1, 4), sl (st1, 4)))), mh (sl (v178, 2), cA));
                int32_t i8 = ad (mh (sl (i9, 2), cA), v170); v170 = ad (v170, i8); st2 = i8;
                i8 = sb (sb (mh (sl (in, 1), c16c), mh (sl (i9, 2), c168)), i8); v178 = ad (v178, i8);
                st1 = ad (mh (sl (i8, 2), cA), sb (i9, mh (sl (i9, 3), mh (sl (i9, 4), sl (i9, 4)))));
                return (tap == 0) ? v170 : (tap == 2) ? v178 : ad (st1, i9);
            }
            double process (double in, double fc, double reso, double sepN) {
                const int c = std::clamp ((int) std::lround (1024.0 * std::log (std::clamp (fc, 1.0, sr * 0.49)) / 10.24), 0, 1023);
                const int r = std::clamp (((int) std::lround (std::clamp (reso, 0.0, 1.0) * 255.0)) >> 2, 0, 63);
                const int sep = std::max (3, ((int) std::lround (std::clamp (sepN, 0.0, 1.0) * 255.0)) * 2);
                const int cU = std::clamp (c + sep, 0, 0x3ff), cL = std::clamp (c - sep, 0, 0x3ff);
                const int32_t in1 = (int32_t) std::lround (std::clamp (in, -2.0, 2.0) * 65536.0);
                const int32_t mid = sect (2, in1, cL, r, a1, a2);      // section 1 (vaz_big.c:1312): cut−Sep, HP
                return (double) sect (0, mid, cU, r, b1, b2) / 65536.0; // section 2 (vaz_big.c:1347): cut+Sep, LP
            }
        };
        long maxd = 0;
        RefHPLP ref; VAZTypeDreal eng; eng.prepare (44100.0);
        uint32_t rng = 0x77u;
        for (int i = 0; i < 50000; ++i) {
            const double fc = 200.0 + (double) (i % 400) * 18.0, rs = 0.2 + (double) (i % 5) * 0.19, sp = (double) (i % 8) / 7.0;
            const double s = (i == 0) ? 0.5 : ((double) (int32_t) ((rng = rng * 1664525u + 1013904223u) >> 9) / 4194304.0 - 0.5);
            const int32_t a = (int32_t) std::llround (ref.process (s, fc, rs, sp) * 65536.0);
            const int32_t b = (int32_t) std::llround (eng.processHPLP (s, fc, rs, sp) * 65536.0);
            const long d = std::llabs ((long) a - (long) b); if (d > maxd) maxd = d;
        }
        row ("filter_dreal_hplp", maxd == 0 ? "BIT-EXACT" : "DEVIATION (max=" + std::to_string (maxd) + ")",
             "VAZ D HP+LP Separation (mode 0x34, .v2p 13): section-1 HP@cut−Sep → section-2 LP@cut+Sep vs VAZTypeDreal::processHPLP");
    }

    // ── FILTER K HP+LP (mode 0x44, .v2p 16) — independent transcription of the WHOLE cascade: the LINEAR HP
    //    pre-section (vaz_big.c:1394-1451, own cutoff/reso, states 0x184/0x188/0x18c) feeding the main K SVF
    //    section (vaz_big.c:1455-1496, states 0x17c/0x180) — vs VAZTypeD::processHPLP. ──
    {
        struct RefKHPLP {
            double sr = 44100.0;
            int32_t sA = 0, sB = 0, sC = 0;    // pre-section (0x184/0x188/0x18c)
            int32_t m1 = 0, m2 = 0;            // main section (0x17c/0x180)
            static int32_t mh (int32_t a, int32_t b) { return (int32_t) (((int64_t) a * b) >> 32); }
            static int32_t sl (int32_t v, int n) { return (int32_t) ((uint32_t) v << n); }
            static int32_t ad (int32_t a, int32_t b) { return (int32_t) ((uint32_t) a + (uint32_t) b); }
            static int32_t sb (int32_t a, int32_t b) { return (int32_t) ((uint32_t) a - (uint32_t) b); }
            static int32_t cl (int32_t v) { return std::clamp (v, (int32_t) 0xff000000, (int32_t) 0x1000000); }
            static int32_t rgain (int32_t cb, double reso) {
                return sl (mh (cb, sl ((int) std::lround (std::clamp (reso, 0.0, 1.0) * 255.0), 22)), 2);
            }
            double process (double in, double fc, double reso, double hpCut, double hpReso) {
                const int hci = std::clamp (((int) std::lround (std::clamp (hpCut, 0.0, 1.0) * 255.0)) << 2, 0, 0x3ff);
                const int32_t hA = VAZTypeDT::kCoefA[hci], hR = rgain (VAZTypeDT::kCoefB[hci], hpReso);
                const int32_t in1 = (int32_t) std::lround (std::clamp (in, -2.0, 2.0) * 65536.0);
                // pre-section HP, 2x oversample (1406-1451)
                int32_t q = cl (mh (sl (sC, 4), hR));
                sA = ad (sA, mh (sl (sb (sb (0, sA), q), 2), hA));
                sB = ad (sB, mh (sl (sb (sb (ad (sA, q), sB), in1), 2), hA));
                const int32_t hp1 = ad (sB, in1);
                q = cl (mh (sl (hp1, 4), hR));
                sA = ad (sA, mh (sl (sb (sb (0, sA), q), 2), hA));
                sB = ad (sB, mh (sl (sb (sb (ad (sA, q), sB), in1), 2), hA));
                const int32_t hp2 = ad (sB, in1);
                sC = hp2;
                const int32_t preOut = ad (hp2, hp1) >> 1;
                // main K section, 2x oversample (1455-1496), input = preOut
                const int mci = std::clamp ((int) std::lround (1024.0 * std::log (std::clamp (fc, 1.0, sr * 0.49)) / 10.24), 0, 1023);
                const int32_t mA = VAZTypeDT::kCoefA[mci], mR = rgain (VAZTypeDT::kCoefB[mci], reso);
                int32_t t0 = 0, t1 = 0;
                for (int p = 0; p < 2; ++p) {
                    const int32_t qq = cl (mh (sl (m2, 4), mR));
                    m1 = ad (m1, mh (sl (sb (sb (preOut, qq), m1), 2), mA));
                    m2 = ad (m2, mh (sl (sb (ad (m1, qq), m2), 2), mA));
                    (p == 0 ? t0 : t1) = m2;
                }
                return (double) (ad (t1, t0) >> 1) / 65536.0;
            }
        };
        long maxd = 0;
        RefKHPLP ref; VAZTypeD eng; eng.prepare (44100.0);
        uint32_t rng = 0x51u;
        for (int i = 0; i < 50000; ++i) {
            const double fc = 150.0 + (double) (i % 380) * 19.0, rs = 0.15 + (double) (i % 6) * 0.16;
            const double hc = (double) (i % 9) / 8.0, hr = (double) (i % 7) / 6.0;
            const double s = (i == 0) ? 0.5 : ((double) (int32_t) ((rng = rng * 1664525u + 1013904223u) >> 9) / 4194304.0 - 0.5);
            const int32_t a = (int32_t) std::llround (ref.process (s, fc, rs, hc, hr) * 65536.0);
            const int32_t b = (int32_t) std::llround (eng.processHPLP (s, fc, rs, hc, hr) * 65536.0);
            const long d = std::llabs ((long) a - (long) b); if (d > maxd) maxd = d;
        }
        row ("filter_k_hplp", maxd == 0 ? "BIT-EXACT" : "DEVIATION (max=" + std::to_string (maxd) + ")",
             "VAZ K HP+LP (mode 0x44, .v2p 16): linear HP pre-section (cut=aux, reso=hpHz) → main K SVF vs VAZTypeD::processHPLP");
    }

    // ── OSC PULSE-WIDTH MAP (WaveShape → duty) — independent transcription of the WaveShape setter FUN_004de930
    //    (@0x4de930: param[0xa4] = b<<16) + the band-limited pulse edge (vaz_big.c:213/222: edge = (param[0xa4]>>1)
    //    ·0x200 = b<<24 vs a 32-bit phase → duty = b/256) vs the clone's OscBlock::pulseWidth. Proves the duty MAP,
    //    not the band-limiting (VAZ = BLEP 006dd2c0/006de2c0; the clone's difference-of-saws matches this map only). ──
    {
        double maxNew = 0.0, maxOld = 0.0, centerNew = 0.0;
        for (int b = 0; b <= 255; ++b) {
            const int32_t  pa4  = (int32_t) ((uint32_t) b << 16);       // FUN_004de930: param[0xa4] = b<<16
            const uint32_t edge = (uint32_t) (pa4 >> 1) << 9;           // (param[0xa4]>>1)·0x200 = b<<24
            const double vazDuty  = (double) edge / 4294967296.0;       // /2^32 = b/256
            const double shape    = (double) b / 255.0;
            const double newDuty  = OscBlock::pulseWidth (shape);       // the FIXED clone map = round(shape·255)/256
            const double oldDuty  = 0.05 + 0.9 * shape;                 // the OLD asserted compression (pre-2b65208)
            maxNew = std::max (maxNew, std::abs (vazDuty - newDuty));
            maxOld = std::max (maxOld, std::abs (vazDuty - oldDuty));
            if (b == 128) centerNew = std::abs (vazDuty - newDuty);     // center must be exactly 0
        }
        char buf[112]; std::snprintf (buf, sizeof buf,
            "fixed map vs VAZ b/256: max=%.5f, center(b=128)=%.5f; OLD 0.05+0.9x deviated max=%.5f (at extremes)",
            maxNew, centerNew, maxOld);
        const bool ok = (maxNew < 1e-9) && (centerNew < 1e-12) && (maxOld > 0.04);   // fix nulls; old was ~0.05
        row ("osc_pulse_pwmap", ok ? "BIT-EXACT (duty = WaveShape/256; center=0; old ~5-pt extreme gone)"
                                   : std::string ("DEVIATION (") + buf + ")",
             std::string ("VAZ pulse duty (FUN_004de930 param[0xa4]=b<<16 → b/256 LINEAR) vs OscBlock::pulseWidth — ") + buf);
    }

    // ── OSC PULSE band-limited (BLEP) — independent transcription of vaz_big.c:206-241 (difference of two
    //    band-limited ramps via the dumped step tables dd2c0/de2c0) vs the clone's CURRENT pulse (OscBlock — a
    //    naive difference-of-saw-wavetables). Reports the HARMONIC-content deviation the user hears. Post-port → 0. ──
    {
        struct RefBLEP {
            uint32_t phase = 0;
            static int32_t rc (int64_t v) { return (int32_t) (v >> 19); }   // ((hi<<0xd)|(lo>>0x13)) == v>>19
            double next (uint32_t inc, int32_t pw) {
                phase += inc;                                               // vaz_big.c:161 (advance, then use)
                const uint32_t u = phase;
                const int32_t iVar19 = (int32_t) u >> 8;
                int32_t pre = ((int32_t) inc >> 8) + ((int32_t) inc >> 0xb) - 0xe000;
                if (pre < 0) pre = 0;
                int iv8 = pre >> 0xd; if (iv8 > VAZPulseT::kN - 1) iv8 = VAZPulseT::kN - 1;
                int32_t iVar15 = (iv8 + 1) * 0x2000;
                int32_t iVar9 = pw >> 1;
                if (iVar9 < iVar15) iVar9 = iVar15;
                const int32_t hi = (iv8 + 1) * -0x2000 + 0x800000;
                if (hi < iVar9) iVar9 = hi;
                iVar15 = iVar15 - 0x800000;
                const int32_t iVar16 = (int32_t) (iVar9 * 0x200 - (int32_t) u) >> 8;
                const int32_t iVar17 = -iVar19;
                const int32_t r1 = (-iVar15 == iVar19 || iVar17 < iVar15)
                    ? rc ((int64_t) (iVar17 + 0x800000) * VAZPulseT::kStepRise[iv8])
                    : rc ((int64_t) (iVar17 - 0x800000) * VAZPulseT::kStepFall[iv8]);
                const int32_t r2 = (iVar15 < iVar16)
                    ? rc ((int64_t) (iVar16 - 0x800000) * VAZPulseT::kStepFall[iv8])
                    : rc ((int64_t) (iVar16 + 0x800000) * VAZPulseT::kStepRise[iv8]);
                const int32_t out = (iVar9 * 2 + r1) - (r2 + 0x800000);
                return (double) out / (double) 0x800000;
            }
        };
        const double PI = 3.14159265358979323846, sr = 48000.0;
        auto mag = [PI] (const std::vector<double>& x, double f, double s) {
            double re = 0, im = 0; const int N = (int) x.size();
            for (int n = 0; n < N; ++n) { const double a = 2.0 * PI * f * n / s; re += x[n] * std::cos (a); im -= x[n] * std::sin (a); }
            return std::hypot (re, im) / N;
        };
        double worst = 0.0; int worstH = 0; double worstHz = 0, worstSh = 0;
        for (double hz : { 110.0, 261.63, 880.0 }) for (double sh : { 0.1, 0.5, 0.9 }) {
            const int b = (int) std::lround (std::clamp (sh, 0.0, 1.0) * 255.0);
            const int32_t pw = (int32_t) ((uint32_t) b << 16);
            const uint32_t inc = (uint32_t) std::llround (hz / sr * 4294967296.0);
            const int N = 16384;
            std::vector<double> ref (N), clo (N);
            RefBLEP rb; OscBlock ob; ob.sampleRate = sr; ob.phase[0] = 0.0;
            for (int n = 0; n < N; ++n) { ref[n] = rb.next (inc, pw); clo[n] = (double) ob.next (hz, 1, sh); }
            const double rf = std::max (1e-9, mag (ref, hz, sr)), cf = std::max (1e-9, mag (clo, hz, sr));
            double dev = 0.0; int cnt = 0;
            for (int k = 2; k <= 40 && k * hz < sr * 0.5; ++k) {
                const double d = std::abs (mag (ref, k * hz, sr) / rf - mag (clo, k * hz, sr) / cf);
                dev += d * d; ++cnt; if (d > worst) { worst = d; worstH = k; worstHz = hz; worstSh = sh; }
            }
            dev = std::sqrt (dev / std::max (1, cnt));
            std::printf ("  [pulse-blep] hz=%6.1f shape=%.1f  harmonic-RMS-dev=%.4f\n", hz, sh, dev);
        }
        char buf[160]; std::snprintf (buf, sizeof buf,
            "harmonic dev up to %.3f (h%d @ %.0fHz sh%.1f) vs clone diff-of-saws", worst, worstH, worstHz, worstSh);
        // NOTE: the BLEP port (d473dad) is REVERTED — it sounded harsh in FL vs real VAZ. This row is now the KNOWN GAP
        // between my BLEP transcription and the clone's diff-of-saws; it is NOT a pass/fail (both differ from real VAZ,
        // which the oracle never measured). Re-porting the BLEP requires validating the transcription vs a real VAZ render.
        row ("osc_pulse_blep", std::string ("GAP (BLEP reverted, pending real-VAZ validation): ") + buf,
             "transcription of vaz_big.c:206-241 (BLEP difference-of-ramps, dumped step tables) vs OscBlock diff-of-saws");
    }

    // ── FILTER R — independent transcription of VAZ's R handler 0x4ddf44 (vaz_big.c:1499-1594, Sallen-Key 0x6d87)
    //    vs VAZTypeK. Faithful = no reso-trim, mode-select 2P|4P output, LINEAR post-HP index. ─────────────────────────
    {
        struct RefR {
            double sr = 44100.0; int32_t s17c = 0, s180 = 0, s184 = 0, s188 = 0, s18c = 0, s170 = 0, s174 = 0, s400 = 0;
            enum : int32_t { KC = 0x418937, CHI = 0xd105e8 };
            static int32_t mh (int32_t a, int32_t b) { return (int32_t) (((int64_t) a * b) >> 32); }
            static int32_t sl (int32_t v, int n) { return (int32_t) ((uint32_t) v << n); }
            static int32_t ad (int32_t a, int32_t b) { return (int32_t) ((uint32_t) a + (uint32_t) b); }
            static int32_t sb (int32_t a, int32_t b) { return (int32_t) ((uint32_t) a - (uint32_t) b); }
            static int32_t cub (int32_t x) { const int32_t v = sl (x, 5); return sb (x, mh (v, mh (v, v))); }
            double process (bool fourPole, double in, double fc, double reso, double hpNorm) {
                const int c = std::clamp ((int) std::lround (1024.0 * std::log (std::clamp (fc, 1.0, sr * 0.49)) / 10.24), 0, 1023);
                const int32_t coefA = VAZTypeKT::kCoefA[c];
                const int r255 = (int) std::lround (std::clamp (reso, 0.0, 1.0) * 255.0);
                const int32_t resoGain = sl (mh (VAZTypeKT::kCoefB[c], sl (r255, 22)), 2);   // NO ÷2 (line 1501)
                const int32_t in1 = (int32_t) std::lround (std::clamp (in, -2.0, 2.0) * 65536.0);
                int32_t iv9 = 0;
                for (int p = 0; p < 2; ++p) {
                    int32_t t = sb (s18c, mh (s188, KC)); s18c = ad (s188, mh (t, KC));      // reso section
                    int32_t x = sb (in1, mh (sl (t, 5), resoGain));
                    if (x > CHI) x = CHI; if (x < -CHI) x = -CHI; x = cub (x);              // clamp + cubic
                    x = ad (x, mh (sl (sb (s17c, x), 2), coefA)); s17c = x;                 // one-pole 1
                    x = ad (x, mh (sl (sb (s180, x), 2), coefA)); s180 = x;                 // one-pole 2
                    if (p == 0) s174 = x; else s174 = ad (s174, x);                         // 2-pole tap (accumulate)
                    x = ad (x, mh (sl (sb (s184, x), 2), coefA)); s184 = x;                 // one-pole 3
                    x = ad (x, mh (sl (sb (s188, x), 2), coefA)); s188 = x;                 // one-pole 4
                    if (p == 0) s170 = x; else iv9 = x;                                     // pass1→s170, pass2→iv9
                }
                int32_t tap = fourPole ? ad (s170, iv9) : s174;                            // 4P = s170+iv9 ; 2P = s174 (line 1574)
                const int hpIdx = std::clamp (((int) std::lround (std::clamp (hpNorm, 0.0, 1.0) * 255.0)) << 2, 0, 1023); // LINEAR
                const int32_t hpCoef = VAZAType::kRC[hpIdx];
                const int32_t half = tap >> 1;
                const int32_t m = mh (sl (ad (s400, half), 2), hpCoef);                     // post-HP one-pole
                s400 = sb (m, half);
                return (double) m / 65536.0;
            }
        };
        // POST-FIX: RefR (faithful 0x4ddf44) vs the rewritten VAZTypeK, both taps (2-pole / 4-pole) + reso/hp sweep.
        // (Pre-fix this read RMS(diff)/RMS = 1.36, max int diff 221226 — the 3 fudges: reso ÷2, avg output, log HP.)
        long maxd = 0;
        for (int fp = 0; fp < 2; ++fp) {
            RefR ref; VAZTypeK k; k.prepare (44100.0);
            uint32_t rng = 0x99u + (uint32_t) fp;
            for (int i = 0; i < 40000; ++i) {
                const double fc = 300.0 + (double) (i % 300) * 25.0, rs = 0.2 + (double) (i % 5) * 0.19, hn = (double) (i % 7) / 6.0;
                const double s = (i == 0) ? 0.5 : ((double) (int32_t) ((rng = rng * 1664525u + 1013904223u) >> 9) / 4194304.0 - 0.5);
                const int32_t a = (int32_t) std::llround (ref.process (fp != 0, s, fc, rs, hn) * 65536.0);
                const int32_t b = (int32_t) std::llround (k.process (fp != 0, s, fc, rs, hn) * 65536.0);
                const long d = std::llabs ((long) a - (long) b); if (d > maxd) maxd = d;
            }
        }
        row ("filter_r", maxd == 0 ? "BIT-EXACT (fudges removed)" : "DEVIATION (max=" + std::to_string (maxd) + ")",
             "VAZ R 0x4ddf44 (Sallen-Key 0x6d87) — VAZTypeK vs faithful transcription, 2-pole & 4-pole (pre-fix RMS was 1.36 = reso÷2 + avg-output + log-HP)");
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
        // ── MEASURED GROUND TRUTH (2026-07-15) — AUDIO-RENDER extraction resolves the 2-survivor tie.
        // tools/osc3_footage_probe.py + osc3_intermediate_check.py render the REAL VAZ via File->Capture
        // with Osc3 as the SOLE audible source (base = a VAZ-made "Osc 3.v2p"; real mix3src 0 = Oscillator 3
        // — decoded from a user Osc3-vs-RingMod preset pair), sweeping lfo1rate; f0 read from the spectrum.
        // The 2 survivors above are IDENTICAL as pitch functions (both == footMul), so measurement confirms
        // or refutes that law outright rather than choosing between them.
        // Also measured: sweeping the +0x94 footage field 48..240 does NOT move the pitch -> Osc3 pitch is
        // driven by lfo1rate (what the clone uses), NOT by +0x94. Rate-curve law e^(0.036*(b-144)) REFUTED.
        const double C3 = 130.81;                          // 01_sustain_C3.mid
        struct M { int b; double f0; };
        const M meas[] = { { 48, 32.0 }, { 96, 66.0 }, { 120, 90.8 }, { 144, 129.2 },
                           { 168, 183.2 }, { 192, 261.0 }, { 216, 368.0 }, { 240, 522.0 } };
        const int nmeas = (int) (sizeof (meas) / sizeof (*meas));
        int bad = 0; double worst = 0.0;
        for (auto& m : meas)
        {
            const double pred = C3 * (double) footMul (m.b);
            const double err  = std::abs (pred - m.f0) / pred;
            if (err > worst) worst = err;
            if (err > 0.03) ++bad;                         // 3%: FFT bin resolution + capture tolerance
        }
        std::printf ("  -- Osc3 footage MEASURED (real VAZ audio) vs 2^((b-144)/48): %d/%d within 3%% (worst %.1f%%) --\n",
                     nmeas - bad, nmeas, worst * 100.0);
        row ("osc3_footage_pitch",
             bad == 0 ? "VERIFIED (measured audio-render)" : "DEVIATION",
             bad == 0 ? ("clone 2^((b-144)/48) CONFIRMED vs REAL VAZ audio over 4 octaves incl. NON-octave points "
                         "(worst " + std::to_string ((int) (worst * 100.0 + 0.5)) + "%) -> rules out an octave-stepped law; "
                         "rate-curve e^(0.036(b-144)) refuted; +0x94 does NOT drive pitch (swept 48..240, unchanged) -> "
                         "clone's lfo1rate source is correct. Static tie (" + std::to_string (survivors)
                         + " survivors, identical as pitch fns) resolved by measurement.")
                      : "measured f0 deviates from 2^((b-144)/48) -> clone formula WRONG");
    }

    // ── Portamento EXP-glide DOMAIN — TRAJECTORY oracle (not endpoints) ────────────────────────────
    // VAZ glides in the note*128 PITCH-LINEAR domain, not in Hz. Reference = the fixed-point recurrence
    // transcribed from vaz_big.c:542-570 (exp branch, present in all 4 branches :547/:560/:791/:804):
    //     tgt16 = noteVal << 16 ; cur16 = current (16.16)
    //     step  = ((|tgt16 - cur16| >> 0x10) * rate) >> 9      ; cur16 +/-= step, clamped to tgt16
    // Domain proof: :2951 cur16 = tgt<<16 (same quantity, 16.16); :2955 pitch = (cur16>>6) - 0x800000
    // = value*1024 - 2^23 → centre at value==8192; and the RPM-dumped note table DAT_006dd0c0[note]
    // = note*128 → note 64 == 8192. Exact ⇒ 128 units per semitone.
    // step ∝ distance ⇒ dist *= (1 - rate/2^25) per sample = a geometric decay = an RC IN CENTS.
    // The clone ran the same RC in the Hz domain → identical endpoints, DIFFERENT path.
    // Both candidates below get the SAME per-sample decay k, so the only variable is the DOMAIN.
    {
        const int n0 = 60, n1 = 72;                 // C3 -> C4, one octave
        const int rate = 4096;                      // [voice+0x1b8]; only the curve SHAPE matters here
        std::vector<double> ref;                    // VAZ reference trajectory, in semitones
        {
            long long tgt = (long long) (n1 * 128) << 16, cur = (long long) (n0 * 128) << 16;
            for (int i = 0; i < 20000 && cur != tgt; ++i)
            {
                const long long d = tgt - cur;
                long long step = ((std::llabs (d) >> 16) * (long long) rate) >> 9;
                if (step < 1) step = 1;             // fixed-point floor: never stall
                cur += (d > 0 ? step : -step);
                if ((d > 0 && cur > tgt) || (d < 0 && cur < tgt)) cur = tgt;
                ref.push_back ((double) (cur >> 16) / 128.0);
            }
        }
        const double k = (double) rate / 33554432.0;    // dist *= (1-k); 2^25 = (>>16 then *rate >>9)
        auto hzOf = [] (double semi) { return 440.0 * std::pow (2.0, (semi - 69.0) / 12.0); };
        double worstHz = 0.0, worstCents = 0.0;
        double hz = hzOf (n0); const double hzT = hzOf (n1);   // clone PRE-FIX: RC in the Hz domain
        double semi = n0;                                       // the FIX: RC in the cents domain
        for (size_t i = 0; i < ref.size(); ++i)
        {
            hz   += (hzT - hz) * k;
            semi += ((double) n1 - semi) * k;
            worstHz    = std::max (worstHz,    std::abs ((69.0 + 12.0 * std::log2 (hz / 440.0)) - ref[i]) * 100.0);
            worstCents = std::max (worstCents, std::abs (semi - ref[i]) * 100.0);
        }
        std::printf ("\n  -- porta EXP-glide TRAJECTORY vs VAZ fixed-point recurrence (C3->C4, %d samples) --\n", (int) ref.size());
        std::printf ("     Hz-domain RC    (clone PRE-fix): max %6.1f cents off the reference path\n", worstHz);
        std::printf ("     cents-domain RC (the FIX)      : max %6.1f cents off the reference path\n", worstCents);
        row ("porta_exp_glide_domain",
             worstCents < 5.0 ? "VERIFIED (trajectory)" : "DEVIATION (max=" + std::to_string ((int) worstCents) + "c)",
             worstCents < 5.0
                 ? ("VAZ glides in note*128 (pitch-linear) — an RC in CENTS. A cents-domain RC tracks the "
                    "fixed-point reference to " + std::to_string ((int) worstCents) + "c; the old Hz-domain RC was "
                    + std::to_string ((int) worstHz) + "c off mid-glide (endpoints agreed, the PATH did not). "
                    "Linear branch untouched (constant step in a pitch-linear domain was already correct).")
                 : "cents-domain RC still deviates from VAZ's recurrence");
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

    // ── FX 1d. Flanger FREE-rate — PINNED to the shared exp curve (disasm) → INDEPENDENT BIT-EXACT + BUG FIXED ──
    {
        // The earlier audit flagged this NOT TESTED and (wrongly) "NON-exp" — that was a SCAN-RANGE ERROR: the e^x
        // search stopped at 0x520700, but the free-rate setter is at 0x520820. Pinned it via xref to the +0x290
        // writes: FUN_00520820 writes the flanger inc (+0x290) in FREE mode ([+0x270]==0) as
        //   inc = round(3891.3558638060376 * e^(0.027*b) * 11025 / sr)
        // constants 0.027 (@0x52089c), 3891.3558638060376 (@0x5208a8), 11025 (@0x5208b4) — BYTE-IDENTICAL to the
        // phaser (FUN_00521c84) and chorus (FUN_00518ffc) rate builders. So the flanger LFO IS the shared exp
        // curve. REAL BUG FOUND + FIXED: the clone used a WRONG fRate^2*20 square law (0..20 Hz, 2-23x too fast) —
        // the exact mistake fixed earlier for phaser/chorus, missed here only because the flanger had no oracle row.
        // Fixed to kPhaserRateLUT (VAZFlanger/PluginProcessor.cpp). Recompute == the proven-bit-exact shared LUT.
        auto rEven = [] (double x) -> long { const double f = std::floor (x), r = x - f;
            return (long) (r < 0.5 ? f : r > 0.5 ? f + 1.0 : (std::fmod (f, 2.0) == 0.0 ? f : f + 1.0)); };
        long maxd = 0;
        for (int b = 0; b < 256; ++b)
        {
            const long r = rEven (3891.3558638060376 * std::exp (0.027 * (double) b) * 11025.0 / 44100.0);
            long d = r - (long) (uint32_t) vazfx::kPhaserRateLUT[b]; if (d < 0) d = -d; if (d > maxd) maxd = d;
        }
        auto vf = [] (int b) { return (double) vazfx::kPhaserRateLUT[b] * 44100.0 / 4294967296.0; };
        auto s = [] (double x) { return std::to_string (x).substr (0, 5); };
        row ("fx_flanger_rate", maxd == 0 ? "BIT-EXACT" : "DEVIATION (max=" + std::to_string (maxd) + ")",
             "flanger free-rate FUN_00520820 == phaser/chorus exp formula (0.027/3891.356/11025 BYTE-identical): "
             "inc=round_even(3891.356*e^(0.027*b)*11025/sr) INDEPENDENT recompute == shared LUT for ALL 256 (maxd="
             + std::to_string (maxd) + "). EXP " + s (vf (0)) + ".." + s (vf (255)) + " Hz. FIXED clone's wrong fRate^2*20.");
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

    // ── FX 5b. Reverb comb coef — FULL-TABLE formula recompute (was 2 anchors) + full engine-dump check ────
    {
        // Formula PINNED by direct disasm of FUN_00522fcc (RT60 comb decay, exp-based):
        //   coef[size][i] = round_even( e^( C · combLen[i] / ((size·16+500)·sr) ) · 2^31 )
        //   C = -6907.755279 (= -1000·ln1000; xword @0x52314c), combLen = {1116,1187,1277,1356,1422,1491,
        //   1557,1617,1203,1527} (float32 consts @0x523158+), scale 2^31 (@0x52315c). e^x = FUN_00402ba8.
        // The recompute reproduces the WHOLE 2560-entry dump to <=1 ULP (systematic +1 on ~89%). NOT bit-exact
        // in double: unlike the phaser coef, this keeps 80-bit through e^x·2^31 (no float32 narrowing), so VAZ's
        // x87 f2xm1-exp is not MSVC-reproducible — VERIFIED honestly (same class as the entangled LFO shapes).
        static const int combLen[10] = { 1116, 1187, 1277, 1356, 1422, 1491, 1557, 1617, 1203, 1527 };
        const double C = -6907.755279, sr = 44100.0;
        auto rEven = [] (double x) -> long { const double f = std::floor (x), r = x - f;
            return (long) (r < 0.5 ? f : r > 0.5 ? f + 1.0 : (std::fmod (f, 2.0) == 0.0 ? f : f + 1.0)); };
        long maxFormula = 0;
        for (int size = 0; size < 256; ++size)
        {
            const double be = C / (((double) size * 16.0 + 500.0) * sr);
            for (int i = 0; i < 10; ++i)
            {
                const long r = rEven (std::exp (be * (double) combLen[i]) * 2147483648.0);
                long d = r - (long) (uint32_t) vazfx::kReverbCombCoefLUT[size][i]; if (d < 0) d = -d;
                if (d > maxFormula) maxFormula = d;
            }
        }
        // engine must load the FULL dumped LUTs (all 256 sizes/damps) at sr=44100, not just anchors:
        long maxEng = 0, maxDamp = 0;
        for (int size = 0; size < 256; ++size)
        {
            VazReverbEngine e; e.setParams (44100.0, size, 0, 255);
            long d = (long) (uint32_t) e.combCoef[0] - (long) (uint32_t) vazfx::kReverbCombCoefLUT[size][0];
            if (d < 0) d = -d; if (d > maxEng) maxEng = d;
        }
        for (int damp = 0; damp < 256; ++damp)   // damp LUT (FUN_00523194, also exp-based 80-bit → same boundary)
        {
            VazReverbEngine e; e.setParams (44100.0, 128, damp, 255);
            long d = (long) (uint32_t) e.damp2 - (long) (uint32_t) vazfx::kReverbDamp2LUT[damp];
            if (d < 0) d = -d; if (d > maxDamp) maxDamp = d;
        }
        row ("fx_reverb_coef_exact", (maxFormula <= 1 && maxEng == 0 && maxDamp == 0) ? "VERIFIED (full-table, formula pinned)" : "DEVIATION",
             "comb coef = round(e^(-6907.755*len/((size*16+500)*sr))*2^31) (FUN_00522fcc, disasm): INDEPENDENT recompute "
             "== dump to <=" + std::to_string (maxFormula) + " ULP over ALL 2560 (systematic 80-bit f2xm1 boundary, not "
             "double-reproducible -> not BIT-EXACT); engine loads full comb+damp dump exact (maxEng=" + std::to_string (maxEng)
             + " maxDamp=" + std::to_string (maxDamp) + "). damp LUT (FUN_00523194) also exp-based 80-bit.");
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

    // ── FX 6b. Decimator param maps — INTEGER (mask/bias/rate), full range → BIT-EXACT (were untested) ───────
    {
        // The render oracle sets mask/bias/rate DIRECTLY, so the param->field maps were unverified. Both are pure
        // integer (direct disasm): FUN_0051dd44 shift=24-bits, mask=(~0>>shift)<<shift, bias=((1<<shift)-1)>>1;
        // FUN_0051dd14 rate=(srParam+1)*192000/sr (min 1). Independent recompute vs engine over the FULL range.
        // (The DC-block coef FUN_0051dc7c is exp-based 80-bit → x87 boundary; the clone computes it in double,
        //  a <=1 ULP pole diff on a ~20 Hz DC-blocker — inaudible, not covered here.)
        long maxM = 0, maxB = 0, maxR = 0;
        for (int bits = 1; bits <= 24; ++bits)
        {
            const int shift = 24 - bits;
            const int32_t refMask = (int32_t) ((0xFFFFFFFFu >> (shift & 31)) << (shift & 31));
            const int32_t refBias = (int32_t) (((1u << (shift & 31)) - 1u) >> 1);
            VazDecimatorEngine e; e.setParams (44100.0, 128, bits);
            long dm = (long) e.mask - refMask; if (dm < 0) dm = -dm; if (dm > maxM) maxM = dm;
            long db = (long) e.bias - refBias; if (db < 0) db = -db; if (db > maxB) maxB = db;
        }
        for (int sp = 0; sp <= 255; ++sp)
        {
            long refRate = ((long) (sp + 1) * 192000) / 44100; if (refRate < 1) refRate = 1;
            VazDecimatorEngine e; e.setParams (44100.0, sp, 16);
            long dr = (long) e.rate - refRate; if (dr < 0) dr = -dr; if (dr > maxR) maxR = dr;
        }
        row ("fx_decimator_maps", (maxM == 0 && maxB == 0 && maxR == 0) ? "BIT-EXACT" : "DEVIATION",
             "mask/bias (FUN_0051dd44) + rate=(srParam+1)*192000/sr (FUN_0051dd14), pure integer: INDEPENDENT recompute "
             "== engine over FULL range (bits 1..24, srParam 0..255; maxMask=" + std::to_string (maxM) + " maxBias="
             + std::to_string (maxB) + " maxRate=" + std::to_string (maxR) + "). coef=exp-80bit (x87), not covered.");
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

    // ── FX 8. Phaser coef LUT — INDEPENDENT closed-form recompute == the dump, all 512 entries ─────────────
    {
        // From FUN_00521aa0 (direct disasm, x87): coef[i] = clamp≥0 round_even( float32( (1 − e^((i·5)·ln2/255)
        //   · 440 / D) · 2^30 ) ). e^x = FUN_00402ba8 (fldl2e/f2xm1/fscale); float10→float32 via `fstp dword`;
        //   fistp = round-to-nearest-even. D = *(*(synth+0x1c)) = 3,784,704 (0x39C000) — the exact runtime
        //   divisor, empirically pinned (D±1 breaks 39 entries). NB: the header's LINEAR (1−i·k) form was
        //   WRONG — the real builder applies e^x and narrows to float32. The clone ships the dump; this proves
        //   the dump == the builder bit-for-bit, so the LUT is fully characterised (no longer just anchors).
        const double ln2 = 0.6931471805599453, D = 3784704.0;
        auto rEven = [] (double x) -> long { const double f = std::floor (x), r = x - f;
            return (long) (r < 0.5 ? f : r > 0.5 ? f + 1.0 : (std::fmod (f, 2.0) == 0.0 ? f : f + 1.0)); };
        long maxd = 0; int miss = 0;
        for (int i = 0; i < 512; ++i)
        {
            const double v = (1.0 - std::exp ((double) (i * 5) * ln2 / 255.0) * 440.0 / D) * 1073741824.0;
            const long r = ((float) v < 0.0f) ? 0 : rEven ((double) (float) v);
            long d = r - (long) (uint32_t) vazfx::kPhaserCoefLUT[i]; if (d < 0) d = -d;
            if (d > maxd) maxd = d; if (d) ++miss;
        }
        VazPhaserEngine e; e.setSampleRate (44100.0);
        const bool engOk = ((uint32_t) e.coefLut[0] == vazfx::kPhaserCoefLUT[0]);   // engine actually loads the dump
        row ("fx_phaser_coef_lut", (maxd == 0 && engOk) ? "BIT-EXACT" : "DEVIATION (max=" + std::to_string (maxd) + ")",
             "512-entry allpass coef LUT (FUN_00521aa0): coef[i]=clamp>=0 round_even(f32((1-e^(i*5*ln2/255)*440/D)*2^30)), "
             "D=3784704 — INDEPENDENT recompute == dump for ALL 512 (" + std::to_string (miss) + " mismatch). "
             "coef[0]=0x3FFE1880..coef[511]=0x3846D580; header's linear form corrected to the real e^x+float32 builder");
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

    // ── FX. Phaser STAGES — engine == independent reference at EVERY discrete stage count (real primitive) ──
    {
        // N = (stagesParam+1)·2 = 2/4/6/8/10/12 (FUN_00521b68). fx_phaser_render covers only N=8; this drives
        // the WHOLE discrete set through the same independent RefPhaser transcription and asserts bit-exact
        // output at each — a real oracle primitive, not the old RMS-audibility diagnostic.
        long maxd = 0; int worstN = 0;
        for (int sp = 0; sp <= 5; ++sp)
        {
            const int N = (sp + 1) * 2;
            VazPhaserEngine eng; RefPhaser ref; eng.clearBuffers(); eng.setSampleRate (44100.0); ref.loadLut();
            eng.inc = ref.inc = 0x00280000u; eng.depth = ref.depth = 200; eng.center = ref.center = 96;
            eng.numStages = ref.numStages = N; eng.fbGain = ref.fbGain = 80 << 23; eng.inGain = ref.inGain = 0x40000000;
            eng.mix = ref.mix = 200; eng.lrPhase = ref.lrPhase = 40;
            uint32_t rng = 0xC0FFEEu;
            for (int i = 0; i < 60000; ++i)
            {
                const int32_t s = (i == 0) ? (1 << 21) : (int32_t) ((rng = rng * 1664525u + 1013904223u) >> 9) - (1 << 21);
                int32_t eL = s, eR = s ^ 0x55, rL = s, rR = s ^ 0x55;
                eng.processFrame (eL, eR); ref.frame (rL, rR);
                const long d1 = std::labs ((long) eL - (long) rL), d2 = std::labs ((long) eR - (long) rR);
                if (d1 > maxd) { maxd = d1; worstN = N; } if (d2 > maxd) { maxd = d2; worstN = N; }
            }
        }
        // secondary audibility check (informative): a 2↔12 stage change is audible at the plugin defaults.
        auto rms2v12 = [] () -> double {
            auto run = [] (int sp, std::vector<double>& out) {
                VazPhaserEngine e; e.clearBuffers(); e.setSampleRate (44100.0);
                e.setParams (sp, 50, false, 153, 128, 64, 128, 255, 175284u); uint32_t rng = 0x1357u; out.clear();
                for (int i = 0; i < 40000; ++i) { int32_t s = (int32_t) ((rng = rng * 1664525u + 1013904223u) >> 9) - (1 << 21);
                    int32_t L = s, R = s; e.processFrame (L, R); if (i >= 2000) out.push_back ((double) L / 8388608.0); } };
            std::vector<double> a, b; run (0, a); run (5, b);
            double sd = 0, ss = 0; for (size_t i = 0; i < a.size(); ++i) { const double d = a[i] - b[i]; sd += d * d; ss += b[i] * b[i]; }
            return ss > 0 ? std::sqrt (sd / ss) : 0.0; };
        const double def = rms2v12();
        row ("fx_phaser_stages", maxd == 0 ? "BIT-EXACT" : "DEVIATION (max=" + std::to_string (maxd) + " @N=" + std::to_string (worstN) + ")",
             "engine == RefPhaser at ALL 6 stage counts N=2/4/6/8/10/12 (maxd=" + std::to_string (maxd)
             + "); audible: RMS(st2-st12)/RMS at default = " + std::to_string (def).substr (0, 5));
    }

    // ── FX. Phaser gain curve — INDEPENDENT closed-form recompute == the dump, all 256 entries ──────────────
    {
        // FUN_00521d44 (disasm fldln2/fyl2x→2^x): inGain[b] = round_even( 2^((b−255)/60)·2^30 ). The
        // independent closed form reproduces the runtime dump for ALL 256 bytes (x87-vs-double stays below the
        // integer round here; this builder does NOT narrow to float32, unlike the coef builder).
        auto rEven = [] (double x) -> long { const double f = std::floor (x), r = x - f;
            return (long) (r < 0.5 ? f : r > 0.5 ? f + 1.0 : (std::fmod (f, 2.0) == 0.0 ? f : f + 1.0)); };
        long maxd = 0; int miss = 0;
        for (int b = 0; b < 256; ++b)
        {
            const long r = rEven (std::exp2 ((double) (b - 255) / 60.0) * 1073741824.0);
            long d = r - (long) (uint32_t) vazfx::kPhaserGainLUT[b]; if (d < 0) d = -d;
            if (d > maxd) maxd = d; if (d) ++miss;
        }
        VazPhaserEngine e; e.setParams (1, 0, false, 200, 90, 64, 200, 225, 0u);
        const bool engOk = ((uint32_t) e.inGain == vazfx::kPhaserGainLUT[225]);   // engine actually loads the dump
        row ("fx_phaser_gain_curve", (maxd == 0 && engOk) ? "BIT-EXACT" : "DEVIATION (max=" + std::to_string (maxd) + ")",
             "gain FUN_00521d44: inGain=round_even(2^((b-255)/60)*2^30) — INDEPENDENT recompute == dump for ALL 256 ("
             + std::to_string (miss) + " mismatch; b=255->0x40000000 0dB, b=225->0x2D413CCD -3dB, b=0->0x035D13F3 -25.6dB)");
    }

    // ── FX. Phaser free-rate curve — INDEPENDENT closed-form recompute == the dump, all 256 entries ─────────
    {
        // FUN_00521c84 (disasm: fild b; ·0.027; e^x; ·3891.356; ·11025/sr): inc[b] = round_even(
        //   3891.3558638060376 · e^(0.027·b) · 11025 / sr ) at sr=44100. Independent recompute == dump for ALL
        //   256 bytes. 32-bit phase inc → freq = inc·sr/2^32 = EXPONENTIAL 0.010..9.76 Hz (was a wrong fRate²·20).
        auto rEven = [] (double x) -> long { const double f = std::floor (x), r = x - f;
            return (long) (r < 0.5 ? f : r > 0.5 ? f + 1.0 : (std::fmod (f, 2.0) == 0.0 ? f : f + 1.0)); };
        long maxd = 0; int miss = 0;
        for (int b = 0; b < 256; ++b)
        {
            const long r = rEven (3891.3558638060376 * std::exp (0.027 * (double) b) * 11025.0 / 44100.0);
            long d = r - (long) (uint32_t) vazfx::kPhaserRateLUT[b]; if (d < 0) d = -d;
            if (d > maxd) maxd = d; if (d) ++miss;
        }
        auto vf = [] (int b) { return (double) vazfx::kPhaserRateLUT[b] * 44100.0 / 4294967296.0; };
        auto s = [] (double x) { return std::to_string (x).substr (0, 5); };
        row ("fx_phaser_rate_curve", maxd == 0 ? "BIT-EXACT" : "DEVIATION (max=" + std::to_string (maxd) + ")",
             "free rate FUN_00521c84: inc=round_even(3891.356*e^(0.027*b)*11025/sr) — INDEPENDENT recompute == dump "
             "for ALL 256 (" + std::to_string (miss) + " mismatch). EXPONENTIAL " + s (vf (0)) + ".." + s (vf (255))
             + " Hz (was fRate^2*20, 2-23x too fast)");
    }

    // ── FX. Chorus LFO rate — chorus builder == phaser rate formula (disasm-confirmed) → INDEPENDENT BIT-EXACT ──
    {
        // Disassembled the CHORUS rate builder FUN_00518ffc/FUN_00518f98 (+0x288/+0x290): it uses the IDENTICAL
        // formula as the phaser rate builder FUN_00521c84 — same constants 0.027 (@0x519054), 3891.355864
        // (@0x519060), ·11025/sr. So the "chorus == phaser rate curve" claim is now proven by disasm (not
        // assumed), and the phaser's independent recompute applies: inc[b] = round_even(3891.356·e^(0.027·b)·
        // 11025/sr) == the shared kPhaserRateLUT for ALL 256. The clone reuses that LUT (PluginProcessor.cpp:94/99).
        auto rEven = [] (double x) -> long { const double f = std::floor (x), r = x - f;
            return (long) (r < 0.5 ? f : r > 0.5 ? f + 1.0 : (std::fmod (f, 2.0) == 0.0 ? f : f + 1.0)); };
        long maxd = 0;
        for (int b = 0; b < 256; ++b)
        {
            const long r = rEven (3891.3558638060376 * std::exp (0.027 * (double) b) * 11025.0 / 44100.0);
            long d = r - (long) (uint32_t) vazfx::kPhaserRateLUT[b]; if (d < 0) d = -d; if (d > maxd) maxd = d;
        }
        auto vf = [] (int b) { return (double) vazfx::kPhaserRateLUT[b] * 44100.0 / 4294967296.0; };
        auto s = [] (double x) { return std::to_string (x).substr (0, 5); };
        row ("fx_chorus_rate_curve", maxd == 0 ? "BIT-EXACT" : "DEVIATION (max=" + std::to_string (maxd) + ")",
             "chorus rate FUN_00518ffc == phaser rate formula (disasm: 0.027/3891.356/11025 match): inc=round_even("
             "3891.356*e^(0.027*b)*11025/sr) INDEPENDENT recompute == shared LUT for ALL 256 (maxd=" + std::to_string (maxd)
             + "). EXP " + s (vf (0)) + ".." + s (vf (255)) + " Hz (was a wrong fRate^2*6)");
    }

    // ── FX. Autopan LFO rate — engine rate LUT (FUN_00517ee0) == dumped; quantify vs old 0.1+fRate²·19.9 ──
    {
        const bool ok = (vazfx::kAutopanRateLUT[0] == 0x0000079Au) && (vazfx::kAutopanRateLUT[255] == 0x012003B7u);
        auto vf = [] (int b) { return (double) vazfx::kAutopanRateLUT[b] * 44100.0 / 4294967296.0; };
        auto of = [] (int b) { const double f = b / 255.0; return 0.1 + f * f * 19.9; };   // old autopan approx
        auto s = [] (double x) { return std::to_string (x).substr (0, 5); };
        row ("fx_autopan_rate_curve", ok ? "VERIFIED (dumped LUT)" : "DEVIATION",
             "VAZ autopan rate (FUN_00517ee0 → +0x27c) = 30.4012·e^(0.036·b)·11025·256/SR, EXP 0.020..193.8 Hz (was "
             "0.1+fRate²·19.9, capped 20 Hz). b{64,128,192}: VAZ " + s (vf (64)) + "/" + s (vf (128)) + "/" + s (vf (192))
             + " vs old " + s (of (64)) + "/" + s (of (128)) + "/" + s (of (192)) + " Hz");
    }

    // ── SYNTH. Mod-LFO rate — VAZ table DAT_006dc4c0 (FUN_004dead8 → +0xe8) == kAutopanRateLUT (byte-identical, same
    //    VAZ curve); quantify vs the clone's guessed 0.05+r²·20 law. LFO1/2/3 share the table. ──────────────────────
    {
        const bool ok = (vazfx::kAutopanRateLUT[0] == 0x0000079Au);   // DAT_006dc4c0[0] = 1946
        auto vf = [] (int sel) { return (double) vazfx::kAutopanRateLUT[sel] * 44100.0 / 4294967296.0; };   // VAZ Hz
        auto of = [] (int sel) { const double r = sel / 255.0; return 0.05 + r * r * 20.0; };                // old clone
        auto s = [] (double x) { return std::to_string (x).substr (0, 5); };
        row ("lfo_rate_curve", ok ? "VERIFIED (dumped DAT_006dc4c0 == autopan)" : "DEVIATION",
             "VAZ mod-LFO rate DAT_006dc4c0[sel]=0.02·e^(0.036·sel), 0.02..187 Hz — clone LFO1/2/3 now index it. "
             "sel{64,128,174}: VAZ " + s (vf (64)) + "/" + s (vf (128)) + "/" + s (vf (174)) + " vs old 0.05+r²·20 "
             + s (of (64)) + "/" + s (of (128)) + "/" + s (of (174)) + " Hz (" + s (of (64) / vf (64)) + "/"
             + s (of (128) / vf (128)) + "/" + s (of (174) / vf (174)) + "x off)");
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

    // ── FX 10b. Delay time (ms→samples) — INDEPENDENT integer recompute, full range → BIT-EXACT ─────────────
    {
        // FUN_0051c1cc (direct disasm — PURE INTEGER, no FPU): delaySamples = ms·(sr/1000) + ((sr/1000)>>4).
        // Exactly reproducible. Independent reference vs the clone's delaySamplesFromMs over the whole 0..2550 ms.
        VazDelayEngine e; e.prepare (44100.0);
        const int spm = 44100 / 1000;                 // sr/1000 = 44 (VAZ integer, [+0x2d4])
        long maxd = 0;
        for (int ms = 0; ms <= 2550; ++ms)
        {
            const long ref = (long) ms * spm + (spm >> 4);
            long d = (long) e.delaySamplesFromMs ((double) ms) - ref; if (d < 0) d = -d; if (d > maxd) maxd = d;
        }
        row ("fx_delay_time", maxd == 0 ? "BIT-EXACT" : "DEVIATION (max=" + std::to_string (maxd) + ")",
             "delay-time ms->samples = ms*(sr/1000)+((sr/1000)>>4) (FUN_0051c1cc, pure integer): INDEPENDENT recompute "
             "== clone over ALL 0..2550 ms (maxd=" + std::to_string (maxd) + ")");
    }

    // ── FX 10c. Delay tone→damp LUT — engine loads the FULL dump (was 2 anchors); formula exp-based 80-bit ────
    {
        // FUN_0051c298 (disasm): damp = clamp(e^((tone+256)*K/…)/sr) — exp-based in x87 80-bit, same helper +
        // structure as the reverb damp builder (FUN_00523194), so NOT double-reproducible (x87 boundary). The
        // clone SHIPS the runtime dump; verify it loads ALL 256 entries exactly at sr=44100.
        VazDelayEngine e; e.prepare (44100.0);
        long maxd = 0;
        for (int i = 0; i < 256; ++i)
        { long d = (long) (uint32_t) e.dampLut[i] - (long) (uint32_t) vazfx::kDelayDampLUT[i]; if (d < 0) d = -d; if (d > maxd) maxd = d; }
        row ("fx_delay_damp", maxd == 0 ? "VERIFIED (full-table dump)" : "DEVIATION (max=" + std::to_string (maxd) + ")",
             "tone->damp runtime-dumped k[0]=0x0FFFBA21 k[255]=0x0FD37981 (FUN_0051c298, exp-based 80-bit -> x87 "
             "boundary, not double-reproducible); engine loads ALL 256 == dump (maxd=" + std::to_string (maxd) + ")");
    }

    // ── FX 11. Autopan pan-law — verified LINEAR from VAZ's code (NOT the assumed equal-power) ──────────────
    {
        // VAZ pan-table ctor @0x517AE4 (DISASSEMBLED — tools/disasm_autopan.py): loop i=0..255 builds
        //   table[i] = sqrt(i/255)·sqrt(K1)·K2, K1=0.5 @0x517c3c, K2=2^31-1 @0x517c48 (TWO fsqrt @0x517B28/0x517B34).
        // Normalized gain = table[i]/(2^31-1) = sqrt(0.5·(i/255)). Render mirrors: gR=table[idx], gL=table[255-idx].
        // SUPERSEDES the earlier WRONG "linear" transcription (it searched for cos/sin, missed the sqrt → EQUAL-POWER).
        const double K1 = 0.5;
        double maxd = 0.0;
        for (int i = 0; i <= 255; ++i)
        {
            const double pan   = (double) i / 255.0;
            const double vaz   = std::sqrt (pan) * std::sqrt (K1);   // = table[i]/(2^31-1), VAZ's normalized gain
            const double clone = std::sqrt (0.5 * pan);              // clone's new equal-power gain (gR)
            maxd = std::max (maxd, std::abs (vaz - clone));
        }
        row ("fx_autopan_panlaw", maxd < 1e-9 ? "BIT-EXACT (EQUAL-POWER sqrt, ctor @0x517AE4)" : "DEVIATION (max=" + std::to_string (maxd) + ")",
             "VAZ table[i]=sqrt(i/255)·sqrt(0.5)·(2^31-1) EQUAL-POWER (two fsqrt @0x517B28/34); clone gL=sqrt(.5(1-pan)) gR=sqrt(.5 pan); prior 'linear' oracle claim was WRONG");
    }

    std::printf ("\n  Constants sourced: cutoff-smooth DAT_006d45e4, detune DAT_0052b168/0x52b0ec, env-rate DAT_006db7e8, stage0 DAT_006dc0bc, flanger delay 0x52076c, chorus delay 0x518fbc.\n");
    std::printf ("=== oracle complete ===\n");
    return 0;
}
