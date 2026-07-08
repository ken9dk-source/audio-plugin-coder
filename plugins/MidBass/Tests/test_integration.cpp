// Phase 9 — integration gates: denormal sweep (whole plugin flushes to exact
// zero) and state stress (rapid preset/state/param churn stays finite).
#include <catch2/catch_test_macros.hpp>
#include "PluginProcessor.h"

TEST_CASE ("integration: full plugin flushes to exact zero after release (denormal sweep)")
{
    MidBassAudioProcessor proc;
    proc.setPlayConfigDetails (0, 2, 44100.0, 512);
    proc.prepareToPlay (44100.0, 512);
    auto set = [&proc] (const char* id, float plain)
    {
        auto* par = proc.apvts.getParameter (id);
        par->setValueNotifyingHost (par->convertTo0to1 (plain));
    };
    set (mb::pid::sat_drive, 60.0f);
    set (mb::pid::trans_attack, 50.0f);
    set (mb::pid::trans_sustain, -30.0f);
    set (mb::pid::fx_dly_fb, 50.0f);
    for (const char* id : { mb::pid::fx_cho_on, mb::pid::fx_pha_on, mb::pid::fx_fla_on,
                            mb::pid::fx_dly_on, mb::pid::fx_rev_on, mb::pid::fx_cmp_on })
        set (id, 1.0f);

    juce::AudioBuffer<float> buf (2, 512);
    juce::MidiBuffer midi;
    midi.addEvent (juce::MidiMessage::noteOn (1, 45, (juce::uint8) 100), 0);
    for (int b = 0; b < 40; ++b) { buf.clear(); proc.processBlock (buf, midi); midi.clear(); }
    midi.addEvent (juce::MidiMessage::noteOff (1, 45), 0);

    int zeroBlocks = 0;
    for (int b = 0; b < (int) (12.0 * 44100.0 / 512.0); ++b)
    {
        buf.clear();
        proc.processBlock (buf, midi);
        midi.clear();
        bool allZero = true;
        for (int ch = 0; ch < 2; ++ch)
            for (int i = 0; i < 512; ++i)
            {
                const float v = buf.getSample (ch, i);
                REQUIRE (std::isfinite (v));
                if (v != 0.0f) allZero = false;
            }
        zeroBlocks = allZero ? zeroBlocks + 1 : 0;
    }
    CHECK (zeroBlocks > 80);                          // ended in sustained EXACT silence (~1 s+)
}

TEST_CASE ("integration: state/preset/param churn under audio stays finite (stress)")
{
    MidBassAudioProcessor proc;
    proc.setPlayConfigDetails (0, 2, 44100.0, 512);
    proc.prepareToPlay (44100.0, 512);
    juce::AudioBuffer<float> buf (2, 512);
    juce::MidiBuffer midi;
    uint32_t rng = 0xC0FFEEu;
    auto rnd = [&rng] { rng ^= rng << 13; rng ^= rng >> 17; rng ^= rng << 5; return rng; };

    const auto& params = proc.getParameters();
    for (int iter = 0; iter < 50; ++iter)
    {
        proc.setCurrentProgram ((int) (rnd() % 10));
        for (int t = 0; t < 8; ++t)                    // random parameter twiddles
        {
            auto* p = params[(int) (rnd() % (juce::uint32) params.size())];
            p->setValueNotifyingHost ((float) (rnd() % 1000) / 1000.0f);
        }
        if ((iter % 3) == 0)                           // state round-trip mid-stream
        {
            juce::MemoryBlock blob;
            proc.getStateInformation (blob);
            proc.setStateInformation (blob.getData(), (int) blob.getSize());
        }
        midi.addEvent ((iter & 1) ? juce::MidiMessage::noteOff (1, 40 + (int) (rnd() % 24))
                                  : juce::MidiMessage::noteOn (1, 40 + (int) (rnd() % 24), (juce::uint8) 100), 0);
        for (int b = 0; b < 4; ++b)
        {
            buf.clear();
            proc.processBlock (buf, midi);
            midi.clear();
            for (int i = 0; i < 512; ++i)
                REQUIRE (std::isfinite (buf.getSample (0, i)));
        }
    }
}
