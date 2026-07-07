// Phase 0 — parameter layout sanity. The full surface is fixed now; these tests
// pin the count, the IDs the later phases depend on, and the ranges that make
// MidBass a mid-bass synth (sub-ms filter attack, 5 ms-capable decay, log cutoff).
#include <catch2/catch_test_macros.hpp>
#include <catch2/catch_approx.hpp>
#include "PluginProcessor.h"

using Catch::Approx;

static juce::RangedAudioParameter* param (MidBassAudioProcessor& p, const char* id)
{
    return p.apvts.getParameter (id);
}

TEST_CASE ("layout: full parameter surface exists exactly once")
{
    MidBassAudioProcessor proc;
    const auto& params = proc.getParameters();
    REQUIRE ((int) params.size() == mb::pid::kExpectedParamCount);

    juce::StringArray ids;
    for (auto* rp : params)
        ids.add (dynamic_cast<juce::RangedAudioParameter*> (rp)->paramID);
    ids.sort (false);
    for (int i = 1; i < ids.size(); ++i)
        REQUIRE (ids[i] != ids[i - 1]);   // no duplicate IDs
}

TEST_CASE ("layout: every section is represented")
{
    MidBassAudioProcessor p;
    for (const char* id : { mb::pid::output, mb::pid::voice_mode, mb::pid::voice_stack,
                            mb::pid::osc1_wave, mb::pid::osc2_pw, mb::pid::osc3_level,
                            mb::pid::osc_sync, mb::pid::osc_fm, mb::pid::osc_ring, mb::pid::osc_drift,
                            mb::pid::sub_on, mb::pid::sub_oct,
                            mb::pid::uni_voices, mb::pid::uni_mono,
                            mb::pid::flt_mode, mb::pid::flt_cutoff, mb::pid::flt_env_amt,
                            mb::pid::flt_drive_pre, mb::pid::flt_drive_post,
                            mb::pid::fenv_a, mb::pid::aenv_r,
                            mb::pid::lfo1_wave, mb::pid::lfo2_dest,
                            mb::pid::mod1_src, mb::pid::mod6_amt,
                            mb::pid::sat_type, mb::pid::sat_mix,
                            mb::pid::eq_ls_gain, mb::pid::eq_mid_q, mb::pid::eq_hs_freq,
                            mb::pid::trans_attack, mb::pid::trans_sustain,
                            mb::pid::macro_punch, mb::pid::macro_width,
                            mb::pid::fx_cho_on, mb::pid::fx_dly_div, mb::pid::fx_rev_mix,
                            mb::pid::fx_cmp_gain })
    {
        INFO (id);
        REQUIRE (param (p, id) != nullptr);
        REQUIRE (param (p, id)->getName (64).isNotEmpty());
    }
}

TEST_CASE ("ranges: filter env is fast enough for mid-bass")
{
    MidBassAudioProcessor p;
    auto* a = dynamic_cast<juce::AudioParameterFloat*> (param (p, mb::pid::fenv_a));
    auto* d = dynamic_cast<juce::AudioParameterFloat*> (param (p, mb::pid::fenv_d));
    REQUIRE (a != nullptr);
    REQUIRE (d != nullptr);
    REQUIRE (a->getNormalisableRange().start <= 0.05f);   // sub-millisecond attack
    REQUIRE (d->getNormalisableRange().start <= 5.0f);    // decay reaches ~5 ms and below
}

TEST_CASE ("ranges: cutoff spans 20 Hz..20 kHz with log-style skew")
{
    MidBassAudioProcessor p;
    auto* c = dynamic_cast<juce::AudioParameterFloat*> (param (p, mb::pid::flt_cutoff));
    REQUIRE (c != nullptr);
    const auto& r = c->getNormalisableRange();
    REQUIRE (r.start == Approx (20.0f));
    REQUIRE (r.end == Approx (20000.0f));
    REQUIRE (r.convertFrom0to1 (0.5f) < 3000.0f);   // midpoint sits low = log feel

    // Bipolar env amount really is bipolar.
    auto* e = dynamic_cast<juce::AudioParameterFloat*> (param (p, mb::pid::flt_env_amt));
    REQUIRE (e->getNormalisableRange().start == Approx (-100.0f));
    REQUIRE (e->getNormalisableRange().end == Approx (100.0f));
}

TEST_CASE ("ranges: choice parameters have the specced options")
{
    MidBassAudioProcessor p;
    auto choices = [&p] (const char* id) {
        auto* c = dynamic_cast<juce::AudioParameterChoice*> (param (p, id));
        REQUIRE (c != nullptr);
        return c->choices;
    };
    REQUIRE (choices (mb::pid::flt_mode) == juce::StringArray { "LP24", "LP12", "HP12", "BP12" });
    REQUIRE (choices (mb::pid::sat_type).size() == 5);
    REQUIRE (choices (mb::pid::voice_stack) == juce::StringArray { "1x", "2x", "4x" });
    REQUIRE (choices (mb::pid::mod1_src).size() == 7);   // Off + 6 sources
    REQUIRE (choices (mb::pid::mod1_dst).size() == 6);
    REQUIRE (choices (mb::pid::lfo1_wave).size() == 5);

    auto* uni = dynamic_cast<juce::AudioParameterInt*> (param (p, mb::pid::uni_voices));
    REQUIRE (uni != nullptr);
    REQUIRE (uni->getRange() == juce::Range<int> (1, 8));
}
