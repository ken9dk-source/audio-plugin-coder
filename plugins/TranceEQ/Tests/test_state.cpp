#include <catch2/catch_test_macros.hpp>
#include <catch2/matchers/catch_matchers_floating_point.hpp>
#include "TestUtils.h"
#include "../Source/PluginProcessor.h"
#include "../Source/ParameterIDs.hpp"

using Catch::Matchers::WithinAbs;

static void setScaled (TranceEQAudioProcessor& p, const juce::String& id, float scaled)
{
    auto* prm = p.apvts.getParameter (id);
    REQUIRE (prm != nullptr);
    prm->setValueNotifyingHost (prm->convertTo0to1 (scaled));
}

TEST_CASE ("APVTS state round-trips exactly", "[state][roundtrip]")
{
    juce::ScopedJuceInitialiser_GUI init;

    TranceEQAudioProcessor a;
    a.setPlayConfigDetails (2, 2, 48000, 512);
    a.prepareToPlay (48000, 512);

    setScaled (a, ParameterIDs::os,          2.0f);
    setScaled (a, ParameterIDs::output,     -6.0f);
    setScaled (a, ParameterIDs::kt_on,       1.0f);
    setScaled (a, ParameterIDs::kt_src,      1.0f);
    setScaled (a, ParameterIDs::bandFreq (2), 3333.0f);
    setScaled (a, ParameterIDs::bandGain (2), 5.5f);
    setScaled (a, ParameterIDs::bandQ (2),    4.2f);
    setScaled (a, ParameterIDs::bandType (3), 2.0f);
    setScaled (a, ParameterIDs::bandOn (5),   1.0f);

    juce::MemoryBlock mb;
    a.getStateInformation (mb);

    TranceEQAudioProcessor b;
    b.setStateInformation (mb.getData(), (int) mb.getSize());

    const juce::StringArray ids {
        ParameterIDs::os, ParameterIDs::output, ParameterIDs::kt_on, ParameterIDs::kt_src,
        ParameterIDs::bandFreq (2), ParameterIDs::bandGain (2), ParameterIDs::bandQ (2),
        ParameterIDs::bandType (3), ParameterIDs::bandOn (5)
    };
    for (const auto& id : ids)
    {
        auto* pa = a.apvts.getParameter (id);
        auto* pb = b.apvts.getParameter (id);
        REQUIRE (pb != nullptr);
        INFO ("param " << id);
        REQUIRE_THAT (pb->getValue(), WithinAbs (pa->getValue(), 1e-6));
    }
}

// Guards preset compatibility: a state saved by v1 must keep loading in every future phase.
// The fixture is generated once (first run), committed, then only ever LOADED.
TEST_CASE ("v1 preset fixture still loads", "[state][compat]")
{
    juce::ScopedJuceInitialiser_GUI init;
    const auto fixture = juce::File (TRANCEEQ_TEST_DIR).getChildFile ("fixtures").getChildFile ("preset_v1.state");

    if (! fixture.existsAsFile())
    {
        TranceEQAudioProcessor gen;
        gen.setPlayConfigDetails (2, 2, 48000, 512);
        gen.prepareToPlay (48000, 512);
        auto* op = gen.apvts.getParameter (ParameterIDs::output);
        op->setValueNotifyingHost (op->convertTo0to1 (-3.0f));
        auto* tp = gen.apvts.getParameter (ParameterIDs::bandType (2));
        tp->setValueNotifyingHost (tp->convertTo0to1 (2.0f));   // Peak
        juce::MemoryBlock mb; gen.getStateInformation (mb);
        fixture.getParentDirectory().createDirectory();
        fixture.replaceWithData (mb.getData(), mb.getSize());
        WARN ("generated v1 fixture: " << fixture.getFullPathName());
    }

    juce::MemoryBlock mb;
    REQUIRE (fixture.loadFileAsData (mb));

    TranceEQAudioProcessor q;
    q.setStateInformation (mb.getData(), (int) mb.getSize());

    auto* op = q.apvts.getParameter (ParameterIDs::output);
    REQUIRE_THAT (op->convertFrom0to1 (op->getValue()), WithinAbs (-3.0f, 0.05f));
}
