// Phase 0 — APVTS state save/recall round-trip.
#include <catch2/catch_test_macros.hpp>
#include <catch2/catch_approx.hpp>
#include "PluginProcessor.h"

using Catch::Approx;

TEST_CASE ("state: parameter values survive a save/recall round-trip")
{
    MidBassAudioProcessor src;

    auto set = [&src] (const char* id, float plainValue)
    {
        auto* p = src.apvts.getParameter (id);
        REQUIRE (p != nullptr);
        p->setValueNotifyingHost (p->convertTo0to1 (plainValue));
    };
    set (mb::pid::flt_cutoff, 431.0f);
    set (mb::pid::flt_reso, 66.0f);
    set (mb::pid::fenv_d, 9.0f);
    set (mb::pid::uni_voices, 7.0f);
    set (mb::pid::flt_mode, 2.0f);       // HP12
    set (mb::pid::macro_punch, 80.0f);
    set (mb::pid::fx_dly_fb, 55.0f);

    juce::MemoryBlock blob;
    src.getStateInformation (blob);
    REQUIRE (blob.getSize() > 0);

    MidBassAudioProcessor dst;
    dst.setStateInformation (blob.getData(), (int) blob.getSize());

    auto plain = [] (MidBassAudioProcessor& proc, const char* id)
    {
        auto* p = proc.apvts.getParameter (id);
        return p->convertFrom0to1 (p->getValue());
    };
    REQUIRE (plain (dst, mb::pid::flt_cutoff) == Approx (431.0f).margin (1.0f));
    REQUIRE (plain (dst, mb::pid::flt_reso)   == Approx (66.0f).margin (0.05f));
    REQUIRE (plain (dst, mb::pid::fenv_d)     == Approx (9.0f).margin (0.1f));
    REQUIRE (plain (dst, mb::pid::uni_voices) == Approx (7.0f));
    REQUIRE (plain (dst, mb::pid::flt_mode)   == Approx (2.0f));
    REQUIRE (plain (dst, mb::pid::macro_punch) == Approx (80.0f).margin (0.05f));
    REQUIRE (plain (dst, mb::pid::fx_dly_fb)  == Approx (55.0f).margin (0.1f));
}

TEST_CASE ("state: garbage input is rejected without touching current state")
{
    MidBassAudioProcessor p;
    auto* cutoff = p.apvts.getParameter (mb::pid::flt_cutoff);
    cutoff->setValueNotifyingHost (cutoff->convertTo0to1 (1234.0f));

    const char junk[] = "definitely not a juce state blob";
    p.setStateInformation (junk, (int) sizeof (junk));

    REQUIRE (cutoff->convertFrom0to1 (cutoff->getValue()) == Approx (1234.0f).margin (2.0f));
}
