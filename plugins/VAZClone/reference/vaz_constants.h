// vaz_constants.h — RAW constants extracted from Vaz2010Core.dll, as bit patterns (NOT formulas or
// rounded decimals). Every constant cites its Ghidra address. Static-data values were read from the DLL
// image with pefile; runtime-BSS tables were dumped via ReadProcessMemory (tools/vaz_tables/*.txt).
// This is the reference oracle for the parity harness (oracle_main.cpp) — the clone's values are
// diffed against these, never the other way around.
#pragma once
#include <cstdint>

namespace vazref
{
    // ── Cutoff base-cutoff SMOOTHER coefficient (one-pole slew alpha), Q32 ──────────────────────────
    // DAT_006d45e4 (runtime BSS; dumped: tools/vaz_tables/cutsmooth_006d45e4.txt, first entry).
    // alpha = 6603751 / 2^32 = 0.0015375556.   Clone uses a rounded float 0.00154f (SynthVoice cutAlpha).
    inline constexpr uint32_t kCutoffSmoothQ32 = 6603751u;
    inline constexpr double   kCutoffSmoothAlpha = 6603751.0 / 4294967296.0;   // 0.00153755559...

    // ── Poly/Unison DETUNE spread ──────────────────────────────────────────────────────────────────
    // FUN_004e0618 @0x4e0618:  scale = kDetuneScale[voiceCount] * detuneAmt(param+0x2f0 poly / +0x2f4 uni);
    //   per-voice offset = (kDetuneOrder[k] * (scale>>3) + centering) >> 10,   k iterated in table order.
    // kDetuneScale: DAT_0052b168 (static, VA 0x52b168, file 0x129f68), int32[32], indexed by voice count.
    inline constexpr int32_t kDetuneScale[32] = {
        31, 256, 256, 171, 142, 128, 119, 114, 110, 107, 104, 102, 101, 100, 98, 98,
        97, 96, 95, 95, 94, 94, 93, 93, 93, 92, 92, 92, 92, 91, 91, 91 };
    // kDetuneOrder: DAT_0052b0ec (static, VA 0x52b0ec), int32[32] = BIT-REVERSAL permutation of 0..31.
    // VAZ detunes voices in this low-discrepancy order.  The clone instead uses a SEEDED-RANDOM spread.
    inline constexpr int32_t kDetuneOrder[32] = {
        0, 16, 8, 24, 4, 20, 12, 28, 2, 18, 10, 26, 6, 22, 14, 30,
        1, 17, 9, 25, 5, 21, 13, 29, 3, 19, 11, 27, 7, 23, 15, 31 };

    // ── Envelope one-pole per-sample RATE table (Q32 alpha), 720 entries ────────────────────────────
    // DAT_006db7e8 (runtime BSS; dumped: tools/vaz_tables/env_rate_006db7e8.txt). The clone ships the
    // IDENTICAL table as VAZEnvT::kRate[720] (plugins/VAZClone/Source/VAZEnvTables.h) — so the oracle
    // reuses that copy and the env-step recurrence below is the ground truth (vaz_big.c:405-443).
    //   Attack : L += (kRate[12+a] * (0x44000000 - L)) >> 32 ; cap 0x3fffffff.  (ATKTGT=0x44000000)
    //   Decay  : L += (kRate[d]    * ((sus-0x400000) - L)) >> 32 ; snap at sustain.
    //   Release: L -= (kRate[r]    * (0x400000 + L))     >> 32 ; floor 0.
    inline constexpr int64_t kEnvOne    = 0x40000000;   // 1.0 in Q30
    inline constexpr int64_t kEnvAtkTgt = 0x44000000;   // attack overshoot target (1.0625)
    inline constexpr int64_t kEnvCap    = 0x3fffffff;   // cap
    inline constexpr int64_t kEnvOvs    = 0x400000;     // decay/release overshoot
    inline constexpr int32_t kStage0Dec = 48695774;     // 0x02e709de = DAT_006dc0bc (Reset pre-attack ramp)

    // ── Osc3 FOOTAGE → pitch ───────────────────────────────────────────────────────────────────────
    // NOT YET EXTRACTED as raw table. Manual/help says 32'=48 … 2'=240 (MIDI-note-domain). The clone
    // uses osc3FootMul (a float multiplier). Marked NOT TESTED in the oracle until the VAZ footage LUT
    // address is confirmed.
}
