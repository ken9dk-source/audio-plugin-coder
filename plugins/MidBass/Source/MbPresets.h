#pragma once
//==============================================================================
// MidBass factory presets — Phase 7. Ten full-snapshot starting points (spec:
// starting points, not demos), exposed as JUCE programs (hosts + the Phase 8
// quick buttons both call setCurrentProgram). Values are PLAIN parameter
// values; unlisted parameters stay at their defaults. Each preset carries its
// design intent (condition g).
//==============================================================================
#include "ParameterIDs.hpp"
#include <iterator>

namespace mb
{
struct PresetValue { const char* paramID; float value; };
struct FactoryPreset { const char* name; const PresetValue* values; int count; };

namespace preset_detail
{
namespace pid = mb::pid;

// Classic Juno — the 80s-polysynth mid-bass: saw + square sub, mild LP24,
// slow chorus, mono. The "warm classic uplifting" starting point.
inline constexpr PresetValue kClassicJuno[] = {
    { pid::output, -1.5f },   // gain discipline: pattern-render peak ~0.85
    { pid::sub_on, 1 }, { pid::sub_wave, 2 }, { pid::sub_level, 45 },
    { pid::flt_cutoff, 520 }, { pid::flt_reso, 15 }, { pid::flt_env_amt, 45 },
    { pid::fenv_d, 140 }, { pid::aenv_r, 25 },
    { pid::fx_cho_on, 1 }, { pid::fx_cho_rate, 0.45f }, { pid::fx_cho_depth, 35 }, { pid::fx_cho_mix, 30 },
    { pid::osc_drift, 15 },
};
// Choose&F — driven, forward off-beat bass: detuned saw pair, hot pre-drive,
// short pluck decay. The "hard uplifting 2006" starting point.
inline constexpr PresetValue kChooseF[] = {
    { pid::output, -4.5f },   // gain discipline: pattern-render peak ~0.85
    { pid::osc2_level, 80 }, { pid::osc2_fine, 8 }, { pid::osc1_fine, -8 },
    { pid::flt_cutoff, 340 }, { pid::flt_reso, 22 }, { pid::flt_env_amt, 55 },
    { pid::flt_drive_pre, 30 }, { pid::fenv_d, 95 },
    { pid::sat_type, 0 }, { pid::sat_drive, 25 },
    { pid::macro_punch, 25 },
};
// Tukan — brighter, hollow pulse flavour: pulse + saw, higher cutoff, a bit of
// resonance. The "melodic Dutch trance" starting point.
inline constexpr PresetValue kTukan[] = {
    { pid::output, -3.5f },   // gain discipline: pattern-render peak ~0.85
    { pid::osc1_wave, 1 }, { pid::osc1_pw, 38 }, { pid::osc2_level, 60 },
    { pid::flt_cutoff, 880 }, { pid::flt_reso, 30 }, { pid::flt_env_amt, 40 },
    { pid::fenv_d, 120 }, { pid::eq_mid_gain, 2.5f },
    { pid::osc_drift, 12 },
};
// ATN — tight and aggressive: diode saturation, post-drive, very short decay.
// The "hard trance stab-bass" starting point.
inline constexpr PresetValue kATN[] = {
    { pid::output, -7.5f },   // gain discipline: pattern-render peak ~0.85
    { pid::flt_cutoff, 300 }, { pid::flt_reso, 18 }, { pid::flt_env_amt, 60 },
    { pid::flt_drive_post, 35 }, { pid::fenv_d, 70 }, { pid::fenv_a, 0.05f },
    { pid::sat_type, 2 }, { pid::sat_drive, 30 }, { pid::sat_mix, 80 },
    { pid::trans_attack, 30 },
};
// Kandi — soft and warm: tube density, gentle EQ tilt, slower envelope.
// The "euphoric mid-tempo" starting point.
inline constexpr PresetValue kKandi[] = {
    { pid::output, -5.0f },   // gain discipline: pattern-render peak ~0.85
    { pid::flt_cutoff, 450 }, { pid::flt_reso, 12 }, { pid::flt_env_amt, 35 },
    { pid::fenv_d, 180 }, { pid::aenv_a, 2.0f },
    { pid::sat_type, 1 }, { pid::sat_drive, 22 }, { pid::sat_mix, 60 },
    { pid::eq_ls_gain, 3.0f }, { pid::eq_hs_gain, -2.0f },
    { pid::macro_warmth, 30 },
};
// Nitrous — sync growl: osc2 a fifth up as hard-sync master, big filter env.
// The "aggressive lead-bass" starting point.
inline constexpr PresetValue kNitrous[] = {
    { pid::output, -2.0f },   // gain discipline: pattern-render peak ~0.85
    { pid::osc2_level, 0 }, { pid::osc2_semi, 7 }, { pid::osc_sync, 1 },
    { pid::flt_cutoff, 400 }, { pid::flt_reso, 28 }, { pid::flt_env_amt, 65 },
    { pid::fenv_d, 110 }, { pid::flt_drive_pre, 20 },
    { pid::sat_type, 4 }, { pid::sat_drive, 18 },
    { pid::macro_bite, 20 },
};
// Pluck — the pure percussive pluck: zero sustain everywhere, fast attack,
// filter does all the movement. The "off-beat pluck-bass" starting point.
inline constexpr PresetValue kPluck[] = {
    { pid::output, -10.0f },   // gain discipline: pattern-render peak ~0.85
    { pid::flt_cutoff, 250 }, { pid::flt_reso, 25 }, { pid::flt_env_amt, 70 },
    { pid::fenv_a, 0.05f }, { pid::fenv_d, 60 }, { pid::fenv_s, 0 },
    { pid::aenv_s, 70 }, { pid::aenv_d, 220 },
    { pid::trans_attack, 40 },
    { pid::macro_punch, 35 },
};
// Bass — deep and plain: sub on, dark filter, no FX. The "layer under a lead"
// starting point.
inline constexpr PresetValue kBass[] = {
    { pid::output, -4.5f },   // gain discipline: pattern-render peak ~0.85
    { pid::sub_on, 1 }, { pid::sub_wave, 0 }, { pid::sub_level, 60 },
    { pid::flt_cutoff, 330 }, { pid::flt_reso, 8 }, { pid::flt_env_amt, 30 },
    { pid::fenv_d, 130 }, { pid::eq_ls_gain, 2.0f },
};
// Wide — stereo ensemble: 6-voice unison, spread open (mono-unison OFF),
// chorus. The "breakdown/FX-section" starting point (NOT for the mono drop).
inline constexpr PresetValue kWide[] = {
    { pid::output, -3.5f },   // gain discipline: pattern-render peak ~0.85
    { pid::uni_voices, 6 }, { pid::uni_detune, 30 }, { pid::uni_spread, 80 },
    { pid::uni_mono, 0 },
    { pid::flt_cutoff, 600 }, { pid::flt_env_amt, 40 }, { pid::fenv_d, 150 },
    { pid::fx_cho_on, 1 }, { pid::fx_cho_mix, 35 },
    { pid::macro_width, 40 },
};
// Mono — the drop workhorse: 4-voice mono-unison (detune beats, zero width),
// tight decay. The "mainroom mono mid-bass" starting point.
inline constexpr PresetValue kMono[] = {
    { pid::output, -1.5f },   // gain discipline: pattern-render peak ~0.85
    { pid::uni_voices, 4 }, { pid::uni_detune, 25 }, { pid::uni_mono, 1 },
    { pid::flt_cutoff, 380 }, { pid::flt_reso, 20 }, { pid::flt_env_amt, 50 },
    { pid::fenv_d, 100 }, { pid::flt_drive_pre, 15 },
    { pid::sat_type, 0 }, { pid::sat_drive, 20 },
};
} // namespace preset_detail

inline constexpr FactoryPreset kFactoryPresets[] = {
    { "Classic Juno", preset_detail::kClassicJuno, (int) std::size (preset_detail::kClassicJuno) },
    { "Choose&F",     preset_detail::kChooseF,     (int) std::size (preset_detail::kChooseF) },
    { "Tukan",        preset_detail::kTukan,       (int) std::size (preset_detail::kTukan) },
    { "ATN",          preset_detail::kATN,         (int) std::size (preset_detail::kATN) },
    { "Kandi",        preset_detail::kKandi,       (int) std::size (preset_detail::kKandi) },
    { "Nitrous",      preset_detail::kNitrous,     (int) std::size (preset_detail::kNitrous) },
    { "Pluck",        preset_detail::kPluck,       (int) std::size (preset_detail::kPluck) },
    { "Bass",         preset_detail::kBass,        (int) std::size (preset_detail::kBass) },
    { "Wide",         preset_detail::kWide,        (int) std::size (preset_detail::kWide) },
    { "Mono",         preset_detail::kMono,        (int) std::size (preset_detail::kMono) },
};
inline constexpr int kNumFactoryPresets = (int) std::size (kFactoryPresets);
} // namespace mb
