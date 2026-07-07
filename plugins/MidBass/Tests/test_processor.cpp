// Phase 0 — processor scaffold: prepares, runs, stays silent and finite.
#include <catch2/catch_test_macros.hpp>
#include "PluginProcessor.h"

TEST_CASE ("processor: Phase 0 processBlock outputs silence and never NaN")
{
    MidBassAudioProcessor p;
    p.setPlayConfigDetails (0, 2, 44100.0, 512);
    p.prepareToPlay (44100.0, 512);

    juce::AudioBuffer<float> buf (2, 512);
    juce::MidiBuffer midi;
    midi.addEvent (juce::MidiMessage::noteOn (1, 48, (juce::uint8) 100), 0);

    for (int block = 0; block < 20; ++block)
    {
        // Fill with garbage to prove the processor clears its output.
        for (int ch = 0; ch < 2; ++ch)
            for (int i = 0; i < 512; ++i)
                buf.setSample (ch, i, 123.0f);

        p.processBlock (buf, midi);
        midi.clear();

        for (int ch = 0; ch < 2; ++ch)
            for (int i = 0; i < 512; ++i)
            {
                const float v = buf.getSample (ch, i);
                REQUIRE (std::isfinite (v));
                REQUIRE (v == 0.0f);
            }
    }
}

TEST_CASE ("processor: reports synth-style capabilities")
{
    MidBassAudioProcessor p;
    REQUIRE (p.acceptsMidi());
    REQUIRE_FALSE (p.producesMidi());
    REQUIRE (p.getName() == juce::String ("MidBass"));
    REQUIRE (p.getTotalNumOutputChannels() >= 2);
}
