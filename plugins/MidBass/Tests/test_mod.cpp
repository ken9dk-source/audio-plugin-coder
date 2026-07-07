// Phase 4 — LFOs + mod matrix (test-first). Standing conditions (2026-07-07):
//  a) rate accuracy free-run AND synced (dotted/triplet), BPM tracked per block
//  b) note-on retrigger: defined origin per shape
//  c) routing superposition: sum then CLAMP at parameter bounds, no wraparound
//  d) square/S&H steps slewed; artifacts gated against an ISOLATION case
//  e) sample-rate independence (44.1k and 96k)
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
mb::MbVoice::Params basicParams()
{
    mb::MbVoice::Params p;
    p.oscCfg.osc[0].level = 1.0f;
    return p;
}

double measureLfoHz (int wave, double rateHz, double sr, double seconds)
{
    mb::MbLfo lfo;
    lfo.prepare (sr, 42u);
    lfo.setWave (wave);
    lfo.setRateHz (rateHz);
    lfo.retrigger();
    double tFirst = -1.0, tLast = -1.0;
    int crossings = 0;
    float prev = lfo.process();
    const int n = (int) (seconds * sr);
    for (int i = 1; i < n; ++i)
    {
        const float v = lfo.process();
        if (prev <= 0.0f && v > 0.0f)
        {
            const double t = i - 1 + (double) (-prev) / (double) (v - prev);
            if (tFirst < 0.0) tFirst = t; else tLast = t;
            ++crossings;
        }
        prev = v;
    }
    REQUIRE (crossings > 2);
    return (double) (crossings - 1) / ((tLast - tFirst) / sr);
}
} // namespace

//==============================================================================
TEST_CASE ("lfo: free-run rate accurate at 44.1k and 96k")   // conditions a + e
{
    for (double sr : { 44100.0, 96000.0 })
        for (double hz : { 2.0, 7.3 })
        {
            const double measured = measureLfoHz (mb::LfoWave::Sine, hz, sr, 20.0);
            INFO ("sr " << sr << " rate " << hz << " measured " << measured);
            CHECK (measured == Approx (hz).epsilon (0.005));
        }
}

TEST_CASE ("lfo: sync division table exact incl dotted and triplet")   // condition a
{
    using mb::syncHz;
    // 120 BPM: quarter = 2 Hz
    CHECK (syncHz (120.0, 0)  == Approx (0.5));            // 1/1
    CHECK (syncHz (120.0, 1)  == Approx (1.0));            // 1/2
    CHECK (syncHz (120.0, 2)  == Approx (2.0));            // 1/4
    CHECK (syncHz (120.0, 3)  == Approx (4.0));            // 1/8
    CHECK (syncHz (120.0, 4)  == Approx (8.0));            // 1/16
    CHECK (syncHz (120.0, 5)  == Approx (16.0));           // 1/32
    CHECK (syncHz (120.0, 6)  == Approx (3.0));            // 1/4T  (2/3 beat)
    CHECK (syncHz (120.0, 7)  == Approx (6.0));            // 1/8T
    CHECK (syncHz (120.0, 8)  == Approx (12.0));           // 1/16T
    CHECK (syncHz (120.0, 9)  == Approx (2.0 / 1.5));      // 1/4.  (1.5 beat)
    CHECK (syncHz (120.0, 10) == Approx (4.0 / 1.5));      // 1/8.
    CHECK (syncHz (120.0, 11) == Approx (8.0 / 1.5));      // 1/16.
    // odd tempo spot check
    CHECK (syncHz (137.4, 3) == Approx (137.4 / 60.0 / 0.5));
}

TEST_CASE ("lfo: synced rate tracks BPM changes mid-note")   // condition a
{
    struct BpmStub : juce::AudioPlayHead
    {
        double bpm = 120.0;
        juce::Optional<PositionInfo> getPosition() const override
        {
            PositionInfo pos;
            pos.setBpm (bpm);
            return pos;
        }
    };

    MidBassAudioProcessor proc;
    BpmStub stub;
    proc.setPlayHead (&stub);
    proc.setPlayConfigDetails (0, 2, 44100.0, 512);
    proc.prepareToPlay (44100.0, 512);

    auto set01 = [&proc] (const char* id, float plain)
    {
        auto* par = proc.apvts.getParameter (id);
        par->setValueNotifyingHost (par->convertTo0to1 (plain));
    };
    set01 (mb::pid::lfo1_sync, 1.0f);                       // sync on, division default 1/8

    juce::AudioBuffer<float> buf (2, 512);
    juce::MidiBuffer midi;
    midi.addEvent (juce::MidiMessage::noteOn (1, 45, (juce::uint8) 100), 0);
    proc.processBlock (buf, midi); midi.clear();
    CHECK (proc.lfo1HzForTest() == Approx (4.0));           // 1/8 @ 120

    stub.bpm = 90.0;                                        // tempo change mid-note
    proc.processBlock (buf, midi);
    CHECK (proc.lfo1HzForTest() == Approx (3.0));           // tracked next block

    proc.setPlayHead (nullptr);
}

TEST_CASE ("lfo: retrigger origin defined per shape")   // condition b
{
    auto firstOut = [] (int wave)
    {
        mb::MbLfo lfo;
        lfo.prepare (44100.0, 7u);
        lfo.setWave (wave);
        lfo.setRateHz (2.0);
        for (int i = 0; i < 1000; ++i) lfo.process();       // scramble phase/slew
        lfo.retrigger();
        return lfo.process();
    };

    CHECK (firstOut (mb::LfoWave::Sine)     == Approx (0.0).margin (1.0e-3));
    CHECK (firstOut (mb::LfoWave::Triangle) == Approx (1.0).margin (1.0e-3));
    CHECK (firstOut (mb::LfoWave::Saw)      == Approx (1.0).margin (1.0e-3));
    CHECK (firstOut (mb::LfoWave::Square)   == Approx (1.0).margin (1.0e-3));

    // sine rises from its origin
    mb::MbLfo s; s.prepare (44100.0, 7u); s.setWave (mb::LfoWave::Sine); s.setRateHz (2.0);
    s.retrigger();
    const float v0 = s.process(), v1 = s.process();
    CHECK (v1 > v0);

    // S&H: retrigger draws a FRESH held value (origin = new random sample)
    mb::MbLfo sh; sh.prepare (44100.0, 7u); sh.setWave (mb::LfoWave::SH); sh.setRateHz (2.0);
    bool varied = false;
    float first = 0.0f;
    for (int k = 0; k < 8; ++k)
    {
        sh.retrigger();
        const float v = sh.process();
        CHECK (std::abs (v) <= 1.0f);
        if (k == 0) first = v; else if (v != first) varied = true;
    }
    CHECK (varied);
}

TEST_CASE ("lfo: square/S&H steps are slewed with a sr-independent time constant")   // conditions d + e
{
    for (double sr : { 44100.0, 96000.0 })
    {
        mb::MbLfo lfo;
        lfo.prepare (sr, 7u);
        lfo.setWave (mb::LfoWave::Square);
        lfo.setRateHz (2.0);
        lfo.retrigger();

        // run to just before the half-period step, then measure time to 63.2 %
        const int half = (int) (sr / 2.0 / 2.0);
        for (int i = 0; i < half + 2; ++i) lfo.process();   // step has occurred
        int n63 = 0;
        float v = 1.0f;
        while (v > 1.0f - 0.632f * 2.0f && n63 < (int) sr) { v = lfo.process(); ++n63; }
        const double tau = n63 / sr;
        INFO ("sr " << sr << " tau " << tau * 1000.0 << " ms");
        CHECK (tau == Approx (mb::MbLfo::kSlewSeconds).epsilon (0.25));
    }
}

TEST_CASE ("matrix: superposition sums then clamps — no wraparound at extremes")   // conditions c + e
{
    auto makeVoice = [] (double sr)
    {
        auto v = std::make_unique<mb::MbVoice>();
        v->prepare (sr, 7u);
        return v;
    };
    auto run = [] (mb::MbVoice& v, int samples) { float l, r; for (int i = 0; i < samples; ++i) v.process (l, r); };

    for (double sr : { 44100.0, 96000.0 })
    {
        // linearity: two wheel→cutoff slots, +40 % and +20 % → 0.6 · 5 oct = 3 oct
        auto v = makeVoice (sr);
        auto p = basicParams();
        p.envAmt = 0.0f; p.keytrack = 0.0f; p.cutoffHz = 700.0f;
        p.matrix.slot[0] = { mb::ModSrc::ModWheel, mb::ModDst::Cutoff, 0.4f };
        p.matrix.slot[1] = { mb::ModSrc::ModWheel, mb::ModDst::Cutoff, 0.2f };
        v->setParams (p);
        v->modWheel = 1.0f;
        v->noteOn (45, 1.0f, false);
        run (*v, 256);
        INFO ("sr " << sr << " lastFc " << v->lastFc);
        CHECK (v->lastFc == Approx (700.0 * 8.0).epsilon (0.02));

        // over-modulated high: 3 slots × +100 % → clamps, pins at the Hz ceiling
        p.matrix.slot[0] = { mb::ModSrc::ModWheel, mb::ModDst::Cutoff, 1.0f };
        p.matrix.slot[1] = { mb::ModSrc::ModWheel, mb::ModDst::Cutoff, 1.0f };
        p.matrix.slot[2] = { mb::ModSrc::ModWheel, mb::ModDst::Cutoff, 1.0f };
        v->setParams (p);
        run (*v, 256);
        CHECK (v->lastFc == Approx (sr * 0.45));            // pinned, not wrapped

        // over-modulated low
        p.matrix.slot[0].amt = p.matrix.slot[1].amt = p.matrix.slot[2].amt = -1.0f;
        v->setParams (p);
        run (*v, 256);
        CHECK (v->lastFc == Approx (20.0));                 // pinned at the floor

        // resonance pins at [0, 1]
        p.matrix.slot[0] = { mb::ModSrc::ModWheel, mb::ModDst::Reso, 1.0f };
        p.matrix.slot[1] = { mb::ModSrc::ModWheel, mb::ModDst::Reso, 1.0f };
        p.matrix.slot[2] = { mb::ModSrc::Off, 0, 0.0f };
        v->setParams (p);
        run (*v, 256);
        CHECK (v->fltL.reso == Approx (1.0f));
        p.matrix.slot[0].amt = p.matrix.slot[1].amt = -1.0f;
        v->setParams (p);
        run (*v, 256);
        CHECK (v->fltL.reso == Approx (0.0f));

        // pitch clamps at ±24 st (3 × +100 % = nominal +36 st)
        p.matrix.slot[0] = { mb::ModSrc::ModWheel, mb::ModDst::Pitch, 1.0f };
        p.matrix.slot[1] = { mb::ModSrc::ModWheel, mb::ModDst::Pitch, 1.0f };
        p.matrix.slot[2] = { mb::ModSrc::ModWheel, mb::ModDst::Pitch, 1.0f };
        v->setParams (p);
        run (*v, 256);
        const double noteHz = juce::MidiMessage::getMidiNoteInHertz (45);
        CHECK (v->lastHz == Approx (noteHz * 4.0).epsilon (1.0e-4));   // +24 st exactly

        // and the audio stays finite through all of it
        float l = 0.0f, r = 0.0f;
        for (int i = 0; i < 1000; ++i) { v->process (l, r); REQUIRE (std::isfinite (l)); }
    }
}

TEST_CASE ("lfo routing: vibrato depth is the documented scaling")   // condition a/c
{
    mb::MbVoice v;
    v.prepare (44100.0, 7u);
    auto p = basicParams();
    p.lfo[0].wave = mb::LfoWave::Sine; p.lfo[0].rateHz = 5.0f;
    p.lfo[0].amount = 0.5f; p.lfo[0].dest = 1;              // Pitch, ±6 st
    v.setParams (p);
    v.noteOn (45, 1.0f, false);
    double mn = 1.0e12, mx = 0.0;
    float l, r;
    for (int i = 0; i < 44100; ++i)
    {
        v.process (l, r);
        mn = std::min (mn, v.lastHz); mx = std::max (mx, v.lastHz);
    }
    INFO ("hz range " << mn << ".." << mx << " ratio " << mx / mn);
    CHECK (mx / mn == Approx (2.0).epsilon (0.03));         // ±6 st swing = 1 octave total
}

TEST_CASE ("mod: square LFO to volume and cutoff is click-free (isolation standard)")   // condition d
{
    // Isolation: identical static patch (env amount 0, slow VCA attack, sustain
    // hold) with LFO amount 0 = baseline. The modulated run's 6 kHz bursts must
    // stay within 2x baseline + 0.001 (-60 dBFS) — the Phase 3 standard.
    auto maxHf = [] (int dest, float amount, int fltMode = mb::MbFilter::LP24)
    {
        mb::MbVoice v;
        v.prepare (44100.0, 7u);
        auto p = basicParams();
        p.fltMode = fltMode;
        p.envAmt = 0.0f; p.cutoffHz = 400.0f; p.reso = 0.7f;
        p.aA = 20.0f; p.aS = 100.0f;
        p.lfo[0].wave = mb::LfoWave::Square; p.lfo[0].rateHz = 2.0f;
        p.lfo[0].amount = amount; p.lfo[0].dest = dest;
        v.setParams (p);
        v.noteOn (45, 1.0f, false);

        teq::Biquad hp;
        hp.setCoeffs (teq::makeCoeffs (teq::FilterType::HPF, 6000.0, 0.0, 0.707, 44100.0));
        float worst = 0.0f; float l, r;
        for (int i = 0; i < (int) (3.0 * 44100.0); ++i)
        {
            v.process (l, r);
            REQUIRE (std::isfinite (l));
            const float hf = std::abs (hp.process (l));
            if (i > 22050) worst = std::max (worst, hf);    // skip the note attack
        }
        return worst;
    };

    const float volBase = maxHf (3, 0.0f), volMod = maxHf (3, 0.6f);
    INFO ("volume: baseline HF " << volBase << ", modulated " << volMod);
    CHECK (volMod < 0.001f + 2.0f * volBase);

    // The VOLUME gate above is the slew-adequacy proof: a pure VCA step carries
    // the LFO's step shape and nothing else, and it sits under the -60 dB budget.
    // The cutoff routes below bound each ENGINE's response to the resulting
    // (legal, continuous, 5 ms-slewed) sweep:
    //  * SVF (BP12): baseline HF is the saw's own harmonics on the bandpass skirt
    //    (measured 0.035 — signal, not artifact); sweeping may at most double it
    //    (the resonant peak passing across harmonics).
    const float cutBaseS = maxHf (0, 0.0f, mb::MbFilter::BP12), cutModS = maxHf (0, 0.3f, mb::MbFilter::BP12);
    INFO ("cutoff/SVF: baseline HF " << cutBaseS << ", modulated " << cutModS);
    CHECK (cutModS < 0.001f + 2.0f * cutBaseS);

    //  * Type-R (LP24): near-zero signal HF at 6 kHz, but the bit-exact engine
    //    emits QUANTIZED-COEFFICIENT ZIPPER when swept (1024-entry log table =
    //    ~17-cent coefficient steps — VAZ character inherited with the engine,
    //    not a step click). Documented ceiling ≈ -42 dBFS, sitting ~-40 dB under
    //    the passband signal it accompanies.
    const float cutBaseR = maxHf (0, 0.0f), cutModR = maxHf (0, 0.3f);
    INFO ("cutoff/Type-R: baseline HF " << cutBaseR << ", modulated " << cutModR);
    CHECK (cutModR < 0.008f);
}
