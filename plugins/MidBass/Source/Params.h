#pragma once
//==============================================================================
// MidBass APVTS layout — every parameter, fixed in Phase 0.
// Ranges chosen for the 2004-2010 trance mid-bass job:
//  * filter env attack down to 0.05 ms, decay down to 2 ms (click-free exp ADSR)
//  * cutoff log-skewed, defaults sit in the classic mid-bass zone
//  * all bipolar amounts are -100..+100 %
//==============================================================================
#include <juce_audio_processors/juce_audio_processors.h>
#include "ParameterIDs.hpp"

namespace mb
{
using APVTS = juce::AudioProcessorValueTreeState;
using juce::NormalisableRange;

inline APVTS::ParameterLayout createParameterLayout()
{
    std::vector<std::unique_ptr<juce::RangedAudioParameter>> p;
    auto add = [&p] (auto param) { p.push_back (std::move (param)); };

    auto fl = [] (const char* id, const char* name, NormalisableRange<float> r, float def, const char* unit = "")
    {
        return std::make_unique<juce::AudioParameterFloat> (juce::ParameterID { id, 1 }, name, r, def,
                   juce::AudioParameterFloatAttributes().withLabel (unit));
    };
    auto pct = [&fl] (const char* id, const char* name, float def)
    { return fl (id, name, { 0.0f, 100.0f, 0.01f }, def, "%"); };
    auto bip = [&fl] (const char* id, const char* name, float def)
    { return fl (id, name, { -100.0f, 100.0f, 0.01f }, def, "%"); };
    auto ms = [&fl] (const char* id, const char* name, float lo, float hi, float def)
    { return fl (id, name, { lo, hi, 0.0f, 0.35f }, def, "ms"); };
    auto hz = [&fl] (const char* id, const char* name, float lo, float hi, float def)
    { return fl (id, name, NormalisableRange<float> (lo, hi, 0.0f, 0.25f), def, "Hz"); };
    auto db = [&fl] (const char* id, const char* name, float lo, float hi, float def)
    { return fl (id, name, { lo, hi, 0.01f }, def, "dB"); };
    auto ch = [] (const char* id, const char* name, const juce::StringArray& opts, int def)
    { return std::make_unique<juce::AudioParameterChoice> (juce::ParameterID { id, 1 }, name, opts, def); };
    auto bo = [] (const char* id, const char* name, bool def)
    { return std::make_unique<juce::AudioParameterBool> (juce::ParameterID { id, 1 }, name, def); };
    auto in = [] (const char* id, const char* name, int lo, int hi, int def)
    { return std::make_unique<juce::AudioParameterInt> (juce::ParameterID { id, 1 }, name, lo, hi, def); };

    const juce::StringArray oscWaves  { "Saw", "Pulse", "Triangle" };
    const juce::StringArray lfoWaves  { "Sine", "Triangle", "Saw", "Square", "S&H" };
    const juce::StringArray lfoDests  { "Cutoff", "Pitch", "PWM", "Volume" };
    const juce::StringArray syncDivs  { "1/1", "1/2", "1/4", "1/8", "1/16", "1/32", "1/4T", "1/8T", "1/16T", "1/4.", "1/8.", "1/16." };
    const juce::StringArray modSrcs   { "Off", "Velocity", "ModWheel", "Aftertouch", "Filter Env", "LFO1", "LFO2" };
    const juce::StringArray modDsts   { "Cutoff", "Resonance", "Pitch", "PWM", "Amp", "Drive" };

    // ---- global / voice ----
    add (db (pid::output, "Output", -24.0f, 12.0f, 0.0f));
    add (ch (pid::voice_mode, "Voice Mode", { "Retrig", "Legato" }, 0));
    add (ms (pid::glide_time, "Glide Time", 0.0f, 500.0f, 0.0f));
    add (bo (pid::glide_legato, "Glide Legato Only", true));
    add (ch (pid::voice_stack, "Voice Stack", { "1x", "2x", "4x" }, 0));
    add (in (pid::bend_range, "Bend Range", 0, 12, 2));

    // ---- oscillators ----
    struct OscDef { const char* wave; const char* oct; const char* semi; const char* fine; const char* pw; const char* level; float defLevel; };
    const OscDef oscs[3] = {
        { pid::osc1_wave, pid::osc1_oct, pid::osc1_semi, pid::osc1_fine, pid::osc1_pw, pid::osc1_level, 100.0f },
        { pid::osc2_wave, pid::osc2_oct, pid::osc2_semi, pid::osc2_fine, pid::osc2_pw, pid::osc2_level, 0.0f   },
        { pid::osc3_wave, pid::osc3_oct, pid::osc3_semi, pid::osc3_fine, pid::osc3_pw, pid::osc3_level, 0.0f   } };
    for (int i = 0; i < 3; ++i)
    {
        const auto n = juce::String ("Osc ") + juce::String (i + 1) + " ";
        add (ch (oscs[i].wave,  (n + "Wave").toRawUTF8(),   oscWaves, 0));
        add (in (oscs[i].oct,   (n + "Octave").toRawUTF8(), -2, 2, 0));
        add (in (oscs[i].semi,  (n + "Semi").toRawUTF8(),   -12, 12, 0));
        add (fl (oscs[i].fine,  (n + "Fine").toRawUTF8(),   { -100.0f, 100.0f, 0.1f }, 0.0f, "ct"));
        add (fl (oscs[i].pw,    (n + "PW").toRawUTF8(),     { 5.0f, 95.0f, 0.1f }, 50.0f, "%"));
        add (pct (oscs[i].level, (n + "Level").toRawUTF8(), oscs[i].defLevel));
    }
    add (bo  (pid::osc_sync,  "Osc Sync 2>1", false));
    add (pct (pid::osc_fm,    "Osc FM 2>1", 0.0f));
    add (pct (pid::osc_ring,  "Ring Mod", 0.0f));
    add (pct (pid::osc_drift, "Analog Drift", 10.0f));

    // ---- sub ----
    add (bo  (pid::sub_on,    "Sub On", false));
    add (ch  (pid::sub_wave,  "Sub Wave", { "Sine", "Triangle", "Square" }, 0));
    add (ch  (pid::sub_oct,   "Sub Octave", { "-1", "-2" }, 0));
    add (pct (pid::sub_level, "Sub Level", 50.0f));

    // ---- unison ----
    add (in  (pid::uni_voices, "Unison Voices", 1, 8, 1));
    add (pct (pid::uni_detune, "Unison Detune", 20.0f));
    add (pct (pid::uni_spread, "Unison Spread", 0.0f));
    add (bo  (pid::uni_mono,   "Unison Mono", true));

    // ---- filter ----
    add (ch  (pid::flt_mode,     "Filter Mode", { "LP24", "LP12", "HP12", "BP12" }, 0));
    add (hz  (pid::flt_cutoff,   "Cutoff", 20.0f, 20000.0f, 700.0f));
    add (pct (pid::flt_reso,     "Resonance", 20.0f));
    add (pct (pid::flt_keytrack, "Keytrack", 0.0f));
    add (bip (pid::flt_env_amt,  "Filter Env Amt", 40.0f));
    add (pct (pid::flt_drive_pre,  "Drive Pre", 0.0f));
    add (pct (pid::flt_drive_post, "Drive Post", 0.0f));

    // ---- envelopes (filter env supports sub-ms attack / 2 ms decay) ----
    add (ms  (pid::fenv_a, "Filter Attack", 0.05f, 2000.0f, 0.1f));
    add (ms  (pid::fenv_d, "Filter Decay",  2.0f,  2000.0f, 120.0f));
    add (pct (pid::fenv_s, "Filter Sustain", 0.0f));
    add (ms  (pid::fenv_r, "Filter Release", 2.0f, 2000.0f, 50.0f));
    add (ms  (pid::aenv_a, "Amp Attack", 0.05f, 2000.0f, 0.5f));
    add (ms  (pid::aenv_d, "Amp Decay",  2.0f,  2000.0f, 300.0f));
    add (pct (pid::aenv_s, "Amp Sustain", 100.0f));
    add (ms  (pid::aenv_r, "Amp Release", 2.0f, 2000.0f, 10.0f));

    // ---- LFOs ----
    struct LfoDef { const char* wave; const char* sync; const char* rateHz; const char* rateDiv; const char* amount; const char* retrig; const char* dest; };
    const LfoDef lfos[2] = {
        { pid::lfo1_wave, pid::lfo1_sync, pid::lfo1_rate_hz, pid::lfo1_rate_div, pid::lfo1_amount, pid::lfo1_retrig, pid::lfo1_dest },
        { pid::lfo2_wave, pid::lfo2_sync, pid::lfo2_rate_hz, pid::lfo2_rate_div, pid::lfo2_amount, pid::lfo2_retrig, pid::lfo2_dest } };
    for (int i = 0; i < 2; ++i)
    {
        const auto n = juce::String ("LFO ") + juce::String (i + 1) + " ";
        add (ch (lfos[i].wave, (n + "Wave").toRawUTF8(), lfoWaves, 0));
        add (bo (lfos[i].sync, (n + "Sync").toRawUTF8(), false));
        add (fl (lfos[i].rateHz, (n + "Rate").toRawUTF8(), NormalisableRange<float> (0.01f, 40.0f, 0.0f, 0.3f), 2.0f, "Hz"));
        add (ch (lfos[i].rateDiv, (n + "Division").toRawUTF8(), syncDivs, 3));
        add (pct (lfos[i].amount, (n + "Amount").toRawUTF8(), 0.0f));
        add (bo (lfos[i].retrig, (n + "Retrig").toRawUTF8(), true));
        add (ch (lfos[i].dest, (n + "Dest").toRawUTF8(), lfoDests, 0));
    }

    // ---- mod matrix ----
    const char* srcIds[6] = { pid::mod1_src, pid::mod2_src, pid::mod3_src, pid::mod4_src, pid::mod5_src, pid::mod6_src };
    const char* dstIds[6] = { pid::mod1_dst, pid::mod2_dst, pid::mod3_dst, pid::mod4_dst, pid::mod5_dst, pid::mod6_dst };
    const char* amtIds[6] = { pid::mod1_amt, pid::mod2_amt, pid::mod3_amt, pid::mod4_amt, pid::mod5_amt, pid::mod6_amt };
    for (int i = 0; i < 6; ++i)
    {
        const auto n = juce::String ("Mod ") + juce::String (i + 1) + " ";
        add (ch  (srcIds[i], (n + "Source").toRawUTF8(), modSrcs, 0));
        add (ch  (dstIds[i], (n + "Dest").toRawUTF8(),   modDsts, 0));
        add (bip (amtIds[i], (n + "Amount").toRawUTF8(), 0.0f));
    }

    // ---- saturation ----
    add (ch  (pid::sat_type,  "Sat Type", { "Tape", "Tube", "Diode", "Soft Clip", "Hard Clip" }, 0));
    add (pct (pid::sat_drive, "Sat Drive", 0.0f));
    add (pct (pid::sat_mix,   "Sat Mix", 100.0f));

    // ---- EQ ----
    add (hz (pid::eq_ls_freq,  "EQ Low Freq", 40.0f, 500.0f, 120.0f));
    add (db (pid::eq_ls_gain,  "EQ Low Gain", -12.0f, 12.0f, 0.0f));
    add (hz (pid::eq_mid_freq, "EQ Mid Freq", 100.0f, 5000.0f, 300.0f));
    add (db (pid::eq_mid_gain, "EQ Mid Gain", -12.0f, 12.0f, 0.0f));
    add (fl (pid::eq_mid_q,    "EQ Mid Q", NormalisableRange<float> (0.3f, 6.0f, 0.0f, 0.5f), 1.0f));
    add (hz (pid::eq_hs_freq,  "EQ High Freq", 1000.0f, 16000.0f, 6000.0f));
    add (db (pid::eq_hs_gain,  "EQ High Gain", -12.0f, 12.0f, 0.0f));

    // ---- transient ----
    add (bip (pid::trans_attack,  "Transient Attack", 0.0f));
    add (bip (pid::trans_sustain, "Transient Sustain", 0.0f));

    // ---- macros ----
    add (pct (pid::macro_punch,  "Punch", 0.0f));
    add (pct (pid::macro_bite,   "Bite", 0.0f));
    add (pct (pid::macro_warmth, "Warmth", 0.0f));
    add (pct (pid::macro_snap,   "Snap", 0.0f));
    add (pct (pid::macro_body,   "Body", 0.0f));
    add (pct (pid::macro_width,  "Width", 0.0f));

    // ---- FX ----
    add (bo  (pid::fx_cho_on,    "Chorus On", false));
    add (fl  (pid::fx_cho_rate,  "Chorus Rate", NormalisableRange<float> (0.05f, 5.0f, 0.0f, 0.5f), 0.6f, "Hz"));
    add (pct (pid::fx_cho_depth, "Chorus Depth", 30.0f));
    add (pct (pid::fx_cho_mix,   "Chorus Mix", 30.0f));

    add (bo  (pid::fx_pha_on,    "Phaser On", false));
    add (fl  (pid::fx_pha_rate,  "Phaser Rate", NormalisableRange<float> (0.05f, 5.0f, 0.0f, 0.5f), 0.4f, "Hz"));
    add (pct (pid::fx_pha_depth, "Phaser Depth", 50.0f));
    add (fl  (pid::fx_pha_fb,    "Phaser Feedback", { 0.0f, 95.0f, 0.1f }, 30.0f, "%"));
    add (pct (pid::fx_pha_mix,   "Phaser Mix", 30.0f));

    add (bo  (pid::fx_fla_on,    "Flanger On", false));
    add (fl  (pid::fx_fla_rate,  "Flanger Rate", NormalisableRange<float> (0.05f, 5.0f, 0.0f, 0.5f), 0.3f, "Hz"));
    add (pct (pid::fx_fla_depth, "Flanger Depth", 50.0f));
    add (fl  (pid::fx_fla_fb,    "Flanger Feedback", { 0.0f, 95.0f, 0.1f }, 40.0f, "%"));
    add (pct (pid::fx_fla_mix,   "Flanger Mix", 25.0f));

    add (bo  (pid::fx_dly_on,   "Delay On", false));
    add (ch  (pid::fx_dly_div,  "Delay Time", { "1/4", "1/8", "1/8.", "1/8T", "1/16", "1/16.", "3/16" }, 6));
    add (fl  (pid::fx_dly_fb,   "Delay Feedback", { 0.0f, 95.0f, 0.1f }, 35.0f, "%"));
    add (pct (pid::fx_dly_damp, "Delay Damp", 40.0f));
    add (pct (pid::fx_dly_mix,  "Delay Mix", 20.0f));
    add (bo  (pid::fx_dly_ping, "Delay PingPong", true));

    add (bo  (pid::fx_rev_on,   "Reverb On", false));
    add (pct (pid::fx_rev_size, "Reverb Size", 50.0f));
    add (pct (pid::fx_rev_damp, "Reverb Damp", 50.0f));
    add (pct (pid::fx_rev_mix,  "Reverb Mix", 15.0f));

    add (bo (pid::fx_cmp_on,     "Comp On", false));
    add (db (pid::fx_cmp_thresh, "Comp Threshold", -40.0f, 0.0f, -12.0f));
    add (fl (pid::fx_cmp_ratio,  "Comp Ratio", { 1.0f, 10.0f, 0.1f }, 3.0f, ":1"));
    add (ms (pid::fx_cmp_att,    "Comp Attack", 0.1f, 50.0f, 5.0f));
    add (ms (pid::fx_cmp_rel,    "Comp Release", 20.0f, 500.0f, 100.0f));
    add (db (pid::fx_cmp_gain,   "Comp Makeup", 0.0f, 12.0f, 0.0f));

    return { p.begin(), p.end() };
}
} // namespace mb
