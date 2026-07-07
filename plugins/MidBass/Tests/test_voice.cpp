// Phase 3 — envelopes + voice management (test-first).
// Gates: attack/decay timing within 5 %, click-free retrigger AT HIGH RESONANCE
// (review note: retrigger clicks usually come from the filter, not the VCA),
// legato does not retrigger, glide reaches target (legato-only option honored),
// voice stack modes alive, processor produces its first audible signal.
#include <catch2/catch_test_macros.hpp>
#include <catch2/catch_approx.hpp>
#include "MbVoice.h"
#include "PluginProcessor.h"
#include "Biquad.h"
#include <cmath>
#include <vector>

using Catch::Approx;
namespace
{
constexpr double kSR = 44100.0;

mb::MbVoice::Params basicParams()
{
    mb::MbVoice::Params p;                       // defaults: osc1 saw 100 %
    p.oscCfg.osc[0].level = 1.0f;
    return p;
}
} // namespace

//==============================================================================
TEST_CASE ("env: attack and decay timing within 5 %")
{
    // EarLevel exponential ADSR (TranceAcid port): attack hits 1.0 (state flips to
    // Decay) after ~aMs; decay lands on sustain after ~dMs.
    for (float aMs : { 5.0f, 50.0f, 500.0f })
    {
        ta::AdsrEnv e; e.prepare (kSR);
        e.setADSR (aMs, 100.0f, 50.0f, 50.0f);
        e.noteOn();
        int n = 0;
        while (e.state == ta::AdsrEnv::Attack && n < (int) kSR * 3) { e.process(); ++n; }
        const double measuredMs = 1000.0 * n / kSR;
        INFO ("attack " << aMs << " ms measured " << measuredMs);
        CHECK (measuredMs > aMs * 0.95); CHECK (measuredMs < aMs * 1.05);
    }
    for (float dMs : { 20.0f, 200.0f })
    {
        ta::AdsrEnv e; e.prepare (kSR);
        e.setADSR (1.0f, dMs, 30.0f, 50.0f);
        e.noteOn();
        while (e.state == ta::AdsrEnv::Attack) e.process();
        int n = 0;
        while (e.state == ta::AdsrEnv::Decay && n < (int) kSR * 3) { e.process(); ++n; }
        const double measuredMs = 1000.0 * n / kSR;
        INFO ("decay " << dMs << " ms measured " << measuredMs);
        CHECK (measuredMs > dMs * 0.95); CHECK (measuredMs < dMs * 1.05);
    }

    // Sub-millisecond attack really is sub-millisecond (the mid-bass requirement).
    ta::AdsrEnv f; f.prepare (kSR);
    f.setADSR (0.05f, 5.0f, 0.0f, 5.0f);
    f.noteOn();
    int n = 0;
    while (f.state == ta::AdsrEnv::Attack && n < 100) { f.process(); ++n; }
    CHECK (n <= 4);                              // ≤ ~0.09 ms at 44.1k
}

TEST_CASE ("voice: retrigger is artifact-free at high resonance")
{
    // Review note honored: retrigger clicks come from the filter, not the VCA.
    // Two gates, because a fast filter env sweeping a reso-0.95 filter is the
    // INTENDED pluck chirp — broadband by design — and must not be conflated
    // with artifacts:
    //  (a) static-cutoff patch (filter env sustained): a retrigger changes only
    //      the amp env, so ANY HF burst above steady is a true artifact.
    //      Gate: burst ≤ 2x steady + 0.001 (-60 dBFS), the Phase 2 standard.
    //  (b) pluck patch (0.1 ms attack, +3 oct sweep, reso 0.95): a mid-ring
    //      retrigger may not exceed the patch's OWN first-attack-from-silence
    //      transient — the intended chirp defines the ceiling. Gate: ≤ 2x.
    const auto hpC = teq::makeCoeffs (teq::FilterType::HPF, 6000.0, 0.0, 0.707, kSR);

    auto runPatch = [&hpC] (float envAmt, float fSustainPct, float ampAttackMs, float aSustainPct)
    {
        mb::MbVoice v;
        v.prepare (kSR, 7u);
        auto p = basicParams();
        p.reso = 0.95f; p.cutoffHz = 400.0f; p.envAmt = envAmt;
        p.fA = 0.1f; p.fD = 30.0f; p.fS = fSustainPct; p.fR = 30.0f;
        p.aA = ampAttackMs; p.aD = 300.0f; p.aS = aSustainPct; p.aR = 20.0f;
        v.setParams (p);

        teq::Biquad hp; hp.setCoeffs (hpC);
        v.noteOn (45, 1.0f, false);                                          // A2
        float firstAttack = 0.0f, steady = 0.0f, worstRetrig = 0.0f;
        const int period = (int) (0.25 * kSR);
        int sinceTrig = 0;
        for (int i = 0; i < (int) (3.0 * kSR); ++i)
        {
            if (i > 0 && i % (period + 37) == 0) { v.noteOn (45, 1.0f, false); sinceTrig = 0; }
            float l = 0.0f, r = 0.0f;
            v.process (l, r);
            REQUIRE (std::isfinite (l));
            const float hf = std::abs (hp.process (l));
            if (i < 2000)                    firstAttack = std::max (firstAttack, hf);
            else if (sinceTrig > 4000)       steady      = std::max (steady, hf);
            else if (sinceTrig <= 2000)      worstRetrig = std::max (worstRetrig, hf);
            ++sinceTrig;
        }
        return std::make_tuple (firstAttack, steady, worstRetrig);
    };

    // (a) env amount 0 → cutoff truly static at 400 Hz — and a 20 ms amp attack,
    // slow enough that the envelope-implied spectrum has no energy at 6 kHz
    // (a FAST amp attack is an intended broadband snap, covered by (b)). Any
    // burst here is a genuine artifact: a state reset or discontinuity.
    auto [fa1, steady1, burst1] = runPatch (0.0f, 100.0f, 20.0f, 60.0f);
    juce::ignoreUnused (fa1);
    INFO ("(a) static-cutoff: steady HF " << steady1 << ", worst retrigger " << burst1);
    CHECK (burst1 < 0.001f + 2.0f * steady1);

    // (b) pluck: retrigger chirp bounded by the intended first-attack chirp
    auto [firstAttack, steady2, burst2] = runPatch (0.6f, 0.0f, 0.5f, 100.0f);
    juce::ignoreUnused (steady2);
    INFO ("(b) pluck: first-attack HF " << firstAttack << ", worst retrigger " << burst2);
    CHECK (firstAttack > 0.001f);                     // the chirp is real (sanity)
    CHECK (burst2 < 2.0f * firstAttack + 0.001f);
}

TEST_CASE ("voice: legato mode does not retrigger, retrig mode does")
{
    auto stateAfterOverlapNote = [] (int voiceMode)
    {
        mb::MbVoice v;
        v.prepare (kSR, 7u);
        auto p = basicParams();
        p.voiceMode = voiceMode;
        p.aA = 200.0f; p.aD = 300.0f; p.aS = 60.0f; p.aR = 50.0f;
        v.setParams (p);
        v.noteOn (45, 1.0f, false);
        float l, r;
        for (int i = 0; i < (int) kSR; ++i) v.process (l, r);   // well past attack+decay
        const float before = v.ampEnv.output;
        v.noteOn (52, 1.0f, true);                              // overlapping note
        for (int i = 0; i < 32; ++i) v.process (l, r);
        return std::make_tuple (before, v.ampEnv.output, (int) v.ampEnv.state);
    };

    auto [beforeL, afterL, stateL] = stateAfterOverlapNote (1); // Legato
    CHECK (stateL == (int) ta::AdsrEnv::Sustain);               // env untouched
    CHECK (afterL == Approx (beforeL).margin (0.05));
    auto [beforeR, afterR, stateR] = stateAfterOverlapNote (0); // Retrig
    juce::ignoreUnused (beforeR, afterR);
    CHECK (stateR == (int) ta::AdsrEnv::Attack);                // re-entered attack
}

TEST_CASE ("voice: glide reaches target; legato-only option honored")
{
    auto pitchAfter = [] (float glideMs, bool legatoOnly, bool overlap, double seconds)
    {
        mb::MbVoice v;
        v.prepare (kSR, 7u);
        auto p = basicParams();
        p.voiceMode = 1;
        p.glideMs = glideMs; p.glideLegatoOnly = legatoOnly;
        p.aA = 1.0f; p.aD = 100.0f; p.aS = 100.0f; p.aR = 50.0f;
        v.setParams (p);
        v.noteOn (36, 1.0f, false);                             // C2
        float l, r;
        for (int i = 0; i < 2000; ++i) v.process (l, r);
        v.noteOn (48, 1.0f, overlap);                           // C3
        for (int i = 0; i < (int) (seconds * kSR); ++i) v.process (l, r);
        return v.glidedHz;
    };

    const double c3 = juce::MidiMessage::getMidiNoteInHertz (48);
    // 100 ms/octave glide, 1 octave jump: not yet arrived at 50 ms, arrived by 200 ms
    CHECK (pitchAfter (100.0f, true, true, 0.050) < c3 * 0.98);
    CHECK (pitchAfter (100.0f, true, true, 0.200) == Approx (c3).epsilon (0.005));
    // legato-only + detached note → immediate jump
    CHECK (pitchAfter (100.0f, true, false, 0.002) == Approx (c3).epsilon (0.005));
    // glide-always + detached note → still glides
    CHECK (pitchAfter (100.0f, false, false, 0.050) < c3 * 0.98);
}

TEST_CASE ("voice: stack modes are alive and level-sane")
{
    auto rmsOf = [] (int stackIdx)
    {
        mb::MbVoice v;
        v.prepare (kSR, 7u);
        auto p = basicParams();
        p.stack = stackIdx;
        v.setParams (p);
        v.noteOn (45, 1.0f, false);
        double acc = 0.0; float l, r;
        const int n = (int) (4.0 * kSR);         // ±4 cents @A2 beats at ~0.5 Hz → cover 2 beats
        for (int i = 0; i < n; ++i) { v.process (l, r); acc += (double) l * l; }
        return std::sqrt (acc / n);
    };
    const double r1 = rmsOf (0), r2 = rmsOf (1), r4 = rmsOf (2);
    INFO ("stack rms 1x " << r1 << " 2x " << r2 << " 4x " << r4);
    CHECK (r1 > 0.01); CHECK (r2 > 0.01); CHECK (r4 > 0.01);
    CHECK (std::abs (20.0 * std::log10 (r2 / r1)) < 3.0);       // 1/sqrt(n) keeps level in family
    CHECK (std::abs (20.0 * std::log10 (r4 / r1)) < 3.0);
}

TEST_CASE ("processor: first audible signal — note on sounds, note off releases to silence")
{
    MidBassAudioProcessor proc;
    proc.setPlayConfigDetails (0, 2, 44100.0, 512);
    proc.prepareToPlay (44100.0, 512);

    juce::AudioBuffer<float> buf (2, 512);
    juce::MidiBuffer midi;

    auto rmsOfBlocks = [&] (int blocks)
    {
        double acc = 0.0; int cnt = 0;
        for (int b = 0; b < blocks; ++b)
        {
            buf.clear();
            proc.processBlock (buf, midi);
            midi.clear();
            for (int i = 0; i < 512; ++i)
            {
                const float s = buf.getSample (0, i);
                REQUIRE (std::isfinite (s));
                acc += (double) s * s; ++cnt;
            }
        }
        return std::sqrt (acc / cnt);
    };

    // silence before any note
    CHECK (rmsOfBlocks (10) < 1.0e-6);

    midi.addEvent (juce::MidiMessage::noteOn (1, 45, (juce::uint8) 100), 0);
    const double playing = rmsOfBlocks (40);
    INFO ("playing rms " << playing);
    CHECK (playing > 0.01);                                     // it makes sound

    midi.addEvent (juce::MidiMessage::noteOff (1, 45), 0);
    (void) rmsOfBlocks (60);                                    // release + settle
    CHECK (rmsOfBlocks (10) < 1.0e-4);                          // decays to silence
}

TEST_CASE ("processor: mono note stack — release returns to held note (legato)")
{
    MidBassAudioProcessor proc;
    proc.setPlayConfigDetails (0, 2, 44100.0, 512);
    proc.prepareToPlay (44100.0, 512);
    juce::AudioBuffer<float> buf (2, 512);
    juce::MidiBuffer midi;
    auto run = [&] (int blocks) { for (int b = 0; b < blocks; ++b) { buf.clear(); proc.processBlock (buf, midi); midi.clear(); } };

    midi.addEvent (juce::MidiMessage::noteOn (1, 36, (juce::uint8) 100), 0);
    run (10);
    midi.addEvent (juce::MidiMessage::noteOn (1, 48, (juce::uint8) 100), 0);
    run (10);
    CHECK (proc.currentNoteForTest() == 48);
    midi.addEvent (juce::MidiMessage::noteOff (1, 48), 0);
    run (10);
    CHECK (proc.currentNoteForTest() == 36);                    // fell back to the held note
    CHECK (proc.voiceActiveForTest());                          // still sounding (no release)
    midi.addEvent (juce::MidiMessage::noteOff (1, 36), 0);
    run (80);
    CHECK_FALSE (proc.voiceActiveForTest());
}
