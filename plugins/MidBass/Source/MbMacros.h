#pragma once
//==============================================================================
// MidBass macros + sweet spots — Phase 7. DATA-DRIVEN, single header (approval
// condition): every macro→parameter mapping and every sweet-spot range lives in
// this file and nowhere else; the processor applies them through one function.
//
// MACRO ORTHOGONALITY STATEMENT (condition e) — what each macro touches, and
// the intentional overlaps:
//   Punch  → filter env amount ↑, filter decay ↓ (mul), transient attack ↑
//   Bite   → pre-filter drive ↑, cutoff ↑ (oct), saturation drive ↑
//   Warmth → saturation mix+drive ↑ (tube-flavoured intent: works with the
//            selected type), low shelf ↑, high shelf ↓
//   Snap   → amp attack ↓ (mul), transient attack ↑, filter attack ↓ (mul)
//   Body   → mid peak gain ↑ (at the USER'S mid frequency — default 300 Hz;
//            Body does not move the frequency, user keeps authority),
//            osc balance toward osc1 (+ sub level slightly)
//   Width  → unison spread ↑, chorus mix ↑
// INTENTIONAL OVERLAPS: Punch and Snap both raise transient attack (Punch is
// the filter-side pluck, Snap the amplitude-side snap; together they compound
// by design). Bite and Warmth both raise saturation drive (Bite pushes edge,
// Warmth pushes density; compound is intended). All sums are CLAMPED at the
// target parameter's bounds — pinning allowed, wraparound impossible (cond d).
//
// WIDTH / MONO SINGLE AUTHORITY (condition b): the low-frequency mono rule is
// OWNED by the FX bass-split at 250 Hz (MbFx.h BassSplit) — chorus content
// below the corner never modulates, so Width is inherently bass-safe. Width's
// unison-spread component is inert while uni_mono is on (the mono-unison rule,
// Phase 1). The old "~200 Hz Width bypass" spec note is retired by this corner.
//==============================================================================
#include <algorithm>

namespace mb
{
namespace Macro { enum { Punch = 0, Bite, Warmth, Snap, Body, Width, Count }; }

// Offsets produced by the macro set (all six applied together, each 0..1).
// Additive fields are in the target's natural unit; *Mul fields are in OCTAVES
// of time scaling (negative = faster), applied as pow(2, x).
struct MacroOffsets
{
    float fltEnvAmt = 0;      // bipolar amount units (-1..1 domain)
    float cutoffOct = 0;      // octaves on base cutoff
    float drivePre = 0, satDrive = 0, satMix = 0;   // 0..1 domains
    float eqLsDb = 0, eqMidDb = 0, eqHsDb = 0;      // dB
    float transAtk = 0;                             // -1..1 domain
    float fltDecayOct = 0, fltAttackOct = 0, ampAttackOct = 0;   // time multipliers (oct)
    float oscBal = 0;                               // + = toward osc1 (and sub)
    float uniSpread = 0, choMix = 0;                // 0..1 domains
};

// THE MacroMap. One row = one macro contribution: offset += amount * macroValue.
inline void computeMacroOffsets (const float m[Macro::Count], MacroOffsets& o)
{
    o = {};
    // Punch — the filter-side pluck
    o.fltEnvAmt   += 0.30f * m[Macro::Punch];
    o.fltDecayOct += -0.80f * m[Macro::Punch];
    o.transAtk    += 0.50f * m[Macro::Punch];
    // Bite — edge and aggression
    o.drivePre    += 0.50f * m[Macro::Bite];
    o.cutoffOct   += 1.20f * m[Macro::Bite];
    o.satDrive    += 0.35f * m[Macro::Bite];
    // Warmth — density and tilt
    o.satMix      += 0.40f * m[Macro::Warmth];
    o.satDrive    += 0.15f * m[Macro::Warmth];      // intentional overlap with Bite
    o.eqLsDb      += 4.0f  * m[Macro::Warmth];
    o.eqHsDb      += -3.0f * m[Macro::Warmth];
    // Snap — amplitude-side snap
    o.ampAttackOct += -1.50f * m[Macro::Snap];
    o.transAtk     += 0.60f * m[Macro::Snap];       // intentional overlap with Punch
    o.fltAttackOct += -1.00f * m[Macro::Snap];
    // Body — 200-400 Hz weight
    o.eqMidDb     += 4.5f  * m[Macro::Body];
    o.oscBal      += 0.25f * m[Macro::Body];
    // Width — stereo (bass-safe by the 250 Hz FX split; inert spread in mono mode)
    o.uniSpread   += 0.60f * m[Macro::Width];
    o.choMix      += 0.35f * m[Macro::Width];
}

//==============================================================================
// SWEET SPOTS (data only — the GUI draws teal arcs from this table in Phase 8).
// Ranges are PLAIN parameter values inside each parameter's bounds; validated
// by test (condition f: data verified like data).
struct SweetSpot { const char* paramID; float lo; float hi; };

inline constexpr SweetSpot kSweetSpots[] = {
    { "flt_cutoff",    180.0f, 900.0f },   // the classic mid-bass window
    { "flt_reso",       10.0f,  45.0f },
    { "flt_drive_pre",   5.0f,  40.0f },
    { "flt_env_amt",    25.0f,  65.0f },
    { "fenv_d",         60.0f, 200.0f },   // pluck decays
    { "fenv_a",          0.05f,  2.0f },
    { "aenv_r",          5.0f,  60.0f },
    { "sat_drive",      10.0f,  45.0f },
    { "osc_drift",       5.0f,  25.0f },
    { "uni_detune",     10.0f,  35.0f },
};
inline constexpr int kNumSweetSpots = (int) (sizeof (kSweetSpots) / sizeof (SweetSpot));
} // namespace mb
