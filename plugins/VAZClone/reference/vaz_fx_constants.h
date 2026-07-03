// vaz_fx_constants.h — RAW constants for the 7 VAZ effects, extracted from Vaz2010Core.dll (the FX DSP
// lives in Core.dll's TFX* classes; VAZ2010Effect.dll is a separate self-contained VST2 wrapper that was
// NOT the clone's source). Bit patterns / exact ratios with Ghidra addresses. Collection phase — no fixes.
//   TFX class-name RTTI: TFXFlanger@0x51FCD4  TFXChorus@0x5184D8  TFXPhaser@0x52107C
//                        TFXEqualizer@0x51E0C0  TFXDelay@0x51AF18  TFXReverb@0x522530
#pragma once
#include <cstdint>

namespace vazfx
{
    // ── FLANGER (TFXFlanger, per-sample @0x5204F4, block setup @0x520418) ─────────────────────────────
    // Delay-time from the 0..255 knob value: samples = (sr · 25/256000) · (value+1)  (FUN_0052076c @0x52076c).
    // 25/256000 == 1/10240, i.e. delay_ms = (value+1)/10.24 → 0.098 … 25.0 ms. VERIFIED == clone (SynthVoice/Flanger).
    inline constexpr double kFlangerDelayCoef = 25.0 / 256000.0;     // = 1/10240 (samples per (value+1) per sr)
    // BPM sync: cycleSamples = (sr · 60 · period[+0x274]) / (BPM[+0x2b0] · 48);  inc[+0x290] = round(2^31 / cycleSamples)
    inline constexpr double kFlangerSyncDiv   = 48.0;                // @0x520418 (period→cycle divisor)
    inline constexpr uint32_t kFlangerPhaseFull = 2147483647u;       // 2^31-1 (INT_MAX) phase full-scale @0x5204BB
    // Feedback comb: buf[w] = in·inGain[+0x294] + feedback[+0x264]·delayed ; dry/wet: out = in + mix[+0x284]·(delayed−in)
    // (LINEAR mix law). Triangle LFO = abs(phase accumulator) @0x520531-34 (NOT sine). Linear-interp fractional read.

    // ── (constants for CHORUS / PHASER / REVERB / DELAY / AUTOPAN / DECIMATOR / EQ — pending extraction) ──
    // Shared FX float-pool note: f32 = -0.241 recurs at 0x519054/0x51c11c/0x51e954/0x51f098/0x52089c (Chorus/
    // EQ/Flanger/Phaser) — role UNVERIFIED (needs per-site decode; do NOT assume it's a coefficient yet).
}
