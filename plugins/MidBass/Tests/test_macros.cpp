// Phase 7 — macros + sweet spots + factory presets (test-first).
// Conditions (2026-07-07): a) THD-ranking carry (drive gains untouched — the
// ranking test runs in this same suite) · b) Width/mono single authority =
// the 250 Hz FX bass-split · c) macro sweeps artifact-gated vs slow sweeps ·
// d) 2^6 macro corners legal (pin, never wrap; finite) · e) orthogonality
// documented in MbMacros.h · f) SweetSpots validated as data · g) presets
// round-trip bit-repeatably, render non-silent, survive mid-note loads.
#include <catch2/catch_test_macros.hpp>
#include <catch2/catch_approx.hpp>
#include "PluginProcessor.h"
#include "Biquad.h"
#include <cmath>
#include <vector>

using Catch::Approx;
namespace
{
void setPlain (MidBassAudioProcessor& proc, const char* id, float plain)
{
    auto* p = proc.apvts.getParameter (id);
    REQUIRE (p != nullptr);
    p->setValueNotifyingHost (p->convertTo0to1 (plain));
}
float getPlain (MidBassAudioProcessor& proc, const char* id)
{
    auto* p = proc.apvts.getParameter (id);
    return p->convertFrom0to1 (p->getValue());
}
} // namespace

//==============================================================================
TEST_CASE ("macros: width/mono authority is the 250 Hz bass-split")   // condition b
{
    // The corner is data; the Width macro documentation in MbMacros.h points
    // here. If someone moves the corner, this fails and both docs get revisited.
    CHECK (mb::BassSplit::kCornerHz == Approx (250.0));
}

TEST_CASE ("macros: fast sweep is as clean as a slow sweep (isolation gate)")   // condition c
{
    const char* mids[6] = { mb::pid::macro_punch, mb::pid::macro_bite, mb::pid::macro_warmth,
                            mb::pid::macro_snap, mb::pid::macro_body, mb::pid::macro_width };

    auto sweepHf = [&mids] (int macroIdx, double sweepSeconds)
    {
        MidBassAudioProcessor proc;
        proc.setPlayConfigDetails (0, 2, 44100.0, 128);
        proc.prepareToPlay (44100.0, 128);
        juce::AudioBuffer<float> buf (2, 128);
        juce::MidiBuffer midi;
        midi.addEvent (juce::MidiMessage::noteOn (1, 45, (juce::uint8) 100), 0);

        teq::Biquad hp;
        hp.setCoeffs (teq::makeCoeffs (teq::FilterType::HPF, 6000.0, 0.0, 0.707, 44100.0));
        // Render adapts to the sweep so BOTH cases traverse the full 0→1→0 range
        // (comparing a partial slow sweep against a full fast one measures signal
        // level differences, not zipper).
        const int sweepBlocks = (int) (sweepSeconds * 44100.0 / 128.0);
        const int sweepStart  = (int) (1.0 * 44100.0 / 128.0);
        const int blocks      = sweepStart + 2 * sweepBlocks + (int) (0.3 * 44100.0 / 128.0);
        float worst = 0.0f;
        for (int b = 0; b < blocks; ++b)
        {
            if (b >= sweepStart)
            {
                const float t = std::clamp ((float) (b - sweepStart) / (float) sweepBlocks, 0.0f, 2.0f);
                const float v = t <= 1.0f ? t : std::max (0.0f, 2.0f - t);       // up then down
                setPlain (proc, mids[macroIdx], v * 100.0f);
            }
            buf.clear();
            proc.processBlock (buf, midi);
            midi.clear();
            for (int i = 0; i < 128; ++i)
            {
                const float s = buf.getSample (0, i);
                REQUIRE (std::isfinite (s));
                const float hf = std::abs (hp.process (s));
                if (b > sweepStart) worst = std::max (worst, hf);
            }
        }
        return worst;
    };

    for (int mIdx = 0; mIdx < 6; ++mIdx)
    {
        const float slow = sweepHf (mIdx, 1.2f), fast = sweepHf (mIdx, 0.35f);
        INFO ("macro " << mIdx << ": slow-sweep HF " << slow << ", fast-sweep HF " << fast);
        CHECK (fast < 0.001f + 2.0f * slow);
    }
}

TEST_CASE ("macros: all 64 extreme corners legal — pinned, finite, never wrapped")   // condition d
{
    const char* mids[6] = { mb::pid::macro_punch, mb::pid::macro_bite, mb::pid::macro_warmth,
                            mb::pid::macro_snap, mb::pid::macro_body, mb::pid::macro_width };

    MidBassAudioProcessor proc;
    proc.setPlayConfigDetails (0, 2, 44100.0, 512);
    proc.prepareToPlay (44100.0, 512);
    setPlain (proc, mb::pid::flt_env_amt, 90.0f);            // near the rail so Punch MUST pin
    setPlain (proc, mb::pid::sat_drive, 90.0f);
    setPlain (proc, mb::pid::trans_attack, 80.0f);
    juce::AudioBuffer<float> buf (2, 512);
    juce::MidiBuffer midi;
    midi.addEvent (juce::MidiMessage::noteOn (1, 45, (juce::uint8) 100), 0);

    for (int corner = 0; corner < 64; ++corner)
    {
        for (int b = 0; b < 6; ++b)
            setPlain (proc, mids[b], (corner >> b) & 1 ? 100.0f : 0.0f);
        for (int blk = 0; blk < 30; ++blk)                   // let the 30 ms slew settle + render
        {
            buf.clear();
            proc.processBlock (buf, midi);
            midi.clear();
            for (int i = 0; i < 512; ++i)
                REQUIRE (std::isfinite (buf.getSample (0, i)));
        }
        const auto& v = proc.voiceForTest();
        CHECK (v.p.envAmt   >= -1.0f); CHECK (v.p.envAmt   <= 1.0f);
        CHECK (v.p.drivePre >=  0.0f); CHECK (v.p.drivePre <= 1.0f);
        CHECK (v.p.fD >= 2.0f);        CHECK (v.p.fA >= 0.05f);
        CHECK (v.p.aA >= 0.05f);
        CHECK (v.p.cutoffHz >= 20.0f); CHECK (v.p.cutoffHz <= 18000.0f);
        CHECK (v.p.oscCfg.uniSpread >= 0.0f); CHECK (v.p.oscCfg.uniSpread <= 1.0f);
        CHECK (proc.satForTest().drive >= 0.0f); CHECK (proc.satForTest().drive <= 1.0f);
        CHECK (proc.satForTest().mix   >= 0.0f); CHECK (proc.satForTest().mix   <= 1.0f);
        CHECK (proc.transForTest().attackAmt <= 1.0f);
    }
    // pinning proof (not wraparound): base env amt 0.9 + Punch 0.3 → exactly 1.0
    for (int b = 0; b < 6; ++b) setPlain (proc, mids[b], b == mb::Macro::Punch ? 100.0f : 0.0f);
    for (int blk = 0; blk < 60; ++blk) { buf.clear(); proc.processBlock (buf, midi); }
    CHECK (proc.voiceForTest().p.envAmt == Approx (1.0f).margin (1.0e-3));
}

TEST_CASE ("sweetspots: every range is inside its parameter's bounds")   // condition f
{
    MidBassAudioProcessor proc;
    for (int i = 0; i < mb::kNumSweetSpots; ++i)
    {
        const auto& s = mb::kSweetSpots[i];
        INFO ("sweet spot " << s.paramID);
        auto* p = dynamic_cast<juce::RangedAudioParameter*> (proc.apvts.getParameter (s.paramID));
        REQUIRE (p != nullptr);
        CHECK (s.lo < s.hi);
        const auto& r = p->getNormalisableRange();
        CHECK (s.lo >= r.start);
        CHECK (s.hi <= r.end);
    }
}

TEST_CASE ("presets: 10 programs, named, bit-repeatable round-trip, audible, intent-documented")   // condition g
{
    MidBassAudioProcessor proc;
    proc.setPlayConfigDetails (0, 2, 44100.0, 512);
    proc.prepareToPlay (44100.0, 512);
    REQUIRE (proc.getNumPrograms() == 10);
    CHECK (proc.getProgramName (0) == juce::String ("Classic Juno"));
    CHECK (proc.getProgramName (9) == juce::String ("Mono"));

    for (int pr = 0; pr < 10; ++pr)
    {
        proc.setCurrentProgram (pr);
        CHECK (proc.getCurrentProgram() == pr);

        // Round-trip: values equal within a 1-ulp normalized tolerance (XML text
        // serialization through skewed ranges wobbles the last bit), and the
        // STATE ITSELF is bit-repeatable: save→load→save yields identical bytes.
        std::vector<float> before;
        for (auto* rp : proc.getParameters())
            before.push_back (rp->getValue());
        juce::MemoryBlock blob;
        proc.getStateInformation (blob);

        MidBassAudioProcessor other;
        other.setStateInformation (blob.getData(), (int) blob.getSize());
        auto& op = other.getParameters();
        for (int i = 0; i < (int) before.size(); ++i)
            REQUIRE (op[i]->getValue() == Approx (before[i]).margin (1.5e-6));

        juce::MemoryBlock blob2;
        other.getStateInformation (blob2);
        REQUIRE (blob2 == blob);                            // bit-repeatable state

        // renders finite + non-silent on the reference note
        juce::AudioBuffer<float> buf (2, 512);
        juce::MidiBuffer midi;
        midi.addEvent (juce::MidiMessage::noteOn (1, 45, (juce::uint8) 100), 0);
        double acc = 0.0;
        for (int b = 0; b < 60; ++b)
        {
            buf.clear();
            proc.processBlock (buf, midi);
            midi.clear();
            for (int i = 0; i < 512; ++i)
            {
                const float s = buf.getSample (0, i);
                REQUIRE (std::isfinite (s));
                acc += (double) s * s;
            }
        }
        const double rms = std::sqrt (acc / (60.0 * 512.0));
        INFO ("preset " << pr << " (" << proc.getProgramName (pr) << ") rms " << rms);
        CHECK (rms > 1.0e-4);                               // audible starting point
        midi.addEvent (juce::MidiMessage::noteOff (1, 45), 0);
        for (int b = 0; b < 80; ++b) { buf.clear(); proc.processBlock (buf, midi); midi.clear(); }
    }
}

TEST_CASE ("presets: mid-note program change survives (smoke)")   // condition g
{
    MidBassAudioProcessor proc;
    proc.setPlayConfigDetails (0, 2, 44100.0, 512);
    proc.prepareToPlay (44100.0, 512);
    juce::AudioBuffer<float> buf (2, 512);
    juce::MidiBuffer midi;
    midi.addEvent (juce::MidiMessage::noteOn (1, 45, (juce::uint8) 100), 0);

    for (int pr = 0; pr < 10; ++pr)
    {
        proc.setCurrentProgram (pr);
        for (int b = 0; b < 5; ++b)
        {
            buf.clear();
            proc.processBlock (buf, midi);
            midi.clear();
            for (int i = 0; i < 512; ++i)
                REQUIRE (std::isfinite (buf.getSample (0, i)));
        }
    }
    CHECK (proc.voiceActiveForTest());                      // the held note is still alive
}
