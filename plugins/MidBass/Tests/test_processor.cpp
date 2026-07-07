// Processor scaffold: prepares, runs, silent WITHOUT midi (garbage input
// cleared), finite always. Audible behaviour is covered in test_voice.cpp.
#include <catch2/catch_test_macros.hpp>
#include "PluginProcessor.h"

TEST_CASE ("processor: silent without notes, clears garbage, never NaN")
{
    MidBassAudioProcessor p;
    p.setPlayConfigDetails (0, 2, 44100.0, 512);
    p.prepareToPlay (44100.0, 512);

    juce::AudioBuffer<float> buf (2, 512);
    juce::MidiBuffer midi;

    for (int block = 0; block < 20; ++block)
    {
        // Fill with garbage to prove the processor clears its output.
        for (int ch = 0; ch < 2; ++ch)
            for (int i = 0; i < 512; ++i)
                buf.setSample (ch, i, 123.0f);

        p.processBlock (buf, midi);

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
