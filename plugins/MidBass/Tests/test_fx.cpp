// Phase 6 — FX chain (test-first). Conditions (2026-07-07):
//  a) parity vs source VAZ engines (bit-exact / stated gates for adapted ports)
//  b) bypass = bit-exact pass-through; all-bypassed chain transparent
//  c) cumulative latency accounted in one place, reported == measured
//  d) tempo-synced delay: division math + click-free retime (glide strategy)
//  e) tails decay to exact 0.0f under FTZ; finite at parameter extremes
//  f) mono compatibility of modulated FX below 300 Hz (stated gates)
//  g) compressor static curve 1 dB / timing 10% / no GR zipper
//  h) CPU spot check vs the < 5 % single-core target
#include <catch2/catch_test_macros.hpp>
#include <catch2/catch_approx.hpp>
#include <juce_audio_processors/juce_audio_processors.h>
#include <juce_dsp/juce_dsp.h>
#include "MbFx.h"
#include "PluginProcessor.h"
#include "Biquad.h"
#include <chrono>
#include <cmath>
#include <vector>

using Catch::Approx;
namespace
{
constexpr double kSR = 44100.0;

std::vector<float> noiseBuf (int n, uint32_t seed, float amp = 0.4f)
{
    std::vector<float> v ((size_t) n);
    uint32_t rng = seed;
    for (auto& s : v)
    {
        rng ^= rng << 13; rng ^= rng >> 17; rng ^= rng << 5;
        s = (float) ((int32_t) rng) * (amp / 2147483648.0f);
    }
    return v;
}
} // namespace

//==============================================================================
TEST_CASE ("fx: parity — wrappers are bit-exact against the raw VAZ engines")   // condition a
{
    const auto in = noiseBuf (8192, 42u);

    SECTION ("chorus")
    {
        mb::MbChorus w;
        w.bassSafe = false;                     // parity = raw engine path
        w.prepare (kSR);
        w.setParams (0.8f, 0.5f, 0.4f);

        VazChorusEngine raw;                    // identical field setup (mirrors MbChorus::setParams)
        raw.clearBuffers(); raw.buildSineLut();
        const int srI = (int) std::llround (kSR);
        raw.base = ((srI * 50) / 256000) * 47;
        raw.mode1 = raw.mode2 = 2;
        raw.inc1 = (uint32_t) (int64_t) (0.8 / kSR * 4294967296.0);
        raw.inc2 = (uint32_t) (int64_t) (0.8 * 1.13 / kSR * 4294967296.0);
        raw.level = 0x8000;
        raw.depth = raw.level2 = (int32_t) std::llround (0.5 * 0.04 * kSR);
        raw.lrPhase = (int32_t) std::llround (0.5 * 1073741824.0);
        raw.gain = 102;

        for (float s : in)
        {
            float l = s, r = -s;
            w.processSample (l, r);
            int32_t li = (int32_t) std::llround ((double) s * mb::kQ23);
            int32_t ri = (int32_t) std::llround ((double) -s * mb::kQ23);
            raw.processFrame (li, ri);
            REQUIRE (l == (float) ((double) li / mb::kQ23));
            REQUIRE (r == (float) ((double) ri / mb::kQ23));
        }
    }
    SECTION ("phaser")
    {
        mb::MbPhaser w;
        w.bassSafe = false;
        w.dcBlock = false;                      // parity = raw engine path
        w.prepare (kSR);
        w.setParams (0.5f, 0.6f, 0.3f, 0.5f);

        VazPhaserEngine raw;
        raw.clearBuffers(); raw.setSampleRate (kSR);
        raw.setParams (1, 30, false, 153, 128, 128, 128,   // lrPhase: lround(0.5*255) = 128
                       (uint32_t) (int64_t) (0.5 / kSR * 4294967296.0));

        for (float s : in)
        {
            float l = s, r = -s;
            w.processSample (l, r);
            int32_t li = (int32_t) std::llround ((double) s * mb::kQ23);
            int32_t ri = (int32_t) std::llround ((double) -s * mb::kQ23);
            raw.processFrame (li, ri);
            REQUIRE (l == (float) ((double) li / mb::kQ23));
            REQUIRE (r == (float) ((double) ri / mb::kQ23));
        }
    }
    SECTION ("delay (static tap) and reverb")
    {
        mb::MbDelayFx wd;
        wd.dcBlock = false;                     // parity = raw engine path
        wd.prepare (kSR);
        wd.setParams (128.0, 1, 0.4f, 0.4f, 0.5f, true);
        wd.curDel = wd.tgtDel;                  // parity without the retime glide

        VazDelayEngine rawD;
        rawD.prepare (kSR);
        rawD.mode = 1;
        rawD.fbL = rawD.fbR = 102;
        rawD.dampL = rawD.dampR = wd.e.dampL;
        rawD.dryL = rawD.dryR = (int32_t) 0x40000000;
        rawD.wetL = rawD.wetR = (int32_t) std::llround (0.5 * (double) 0x40000000);

        mb::MbReverbFx wr;
        wr.dcBlock = false;
        wr.prepare (kSR);
        wr.setParams (0.5f, 0.5f, 0.4f);

        VazReverbEngine rawR;
        rawR.clearBuffers();
        rawR.setParams (kSR, 128, 128, 102);

        for (float s : in)
        {
            float l = s, r = -s;
            wd.processSample (l, r);
            int32_t li = (int32_t) std::llround ((double) s * mb::kQ23);
            int32_t ri = (int32_t) std::llround ((double) -s * mb::kQ23);
            rawD.delayL = rawD.delayR = std::clamp ((int) std::lround (wd.tgtDel), 1, rawD.mask - 2);
            rawD.processFrame (li, ri);
            REQUIRE (l == (float) ((double) li / mb::kQ23));
            REQUIRE (r == (float) ((double) ri / mb::kQ23));

            float l2 = s, r2 = -s;
            wr.processSample (l2, r2);
            int32_t li2 = (int32_t) std::llround ((double) s * mb::kQ23);
            int32_t ri2 = (int32_t) std::llround ((double) -s * mb::kQ23);
            rawR.processFrame (li2, ri2);
            REQUIRE (l2 == (float) ((double) li2 / mb::kQ23));
            REQUIRE (r2 == (float) ((double) ri2 / mb::kQ23));
        }
    }
    SECTION ("flanger (adapted, bassSafe off) — bit-exact vs the lifted VAZ loop")
    {
        mb::MbFlanger w;
        w.bassSafe = false;
        w.prepare (kSR);
        w.setParams (0.3f, 0.5f, 0.4f, 0.25f);

        // reference = plugins/VAZFlanger FlangerChannel::process, verbatim
        struct RefCh
        {
            std::vector<float> buf; int mask = 0, wpos = 0;
            void prepare (double sr)
            {
                int n = 1; const int need = (int) (0.090 * sr) + 4;
                while (n < need) n <<= 1;
                buf.assign ((size_t) n, 0.0f); mask = n - 1; wpos = 0;
            }
            double process (double inv, double delaySamples, double feedback, double mix)
            {
                double rp = (double) wpos - delaySamples;
                const double sz = (double) (mask + 1);
                while (rp < 0.0) rp += sz;
                const int i0 = (int) rp;
                const double frac = rp - (double) i0;
                const double s0 = (double) buf[(size_t) (i0 & mask)];
                const double s1 = (double) buf[(size_t) ((i0 + 1) & mask)];
                const double delayed = s0 + frac * (s1 - s0);
                double wv = inv + feedback * delayed;
                wv = std::clamp (wv, -4.0, 4.0);
                if (std::abs (wv) < 1.0e-20) wv = 0.0;            // MidBass flush (only sub-1e-20 diff)
                buf[(size_t) wpos] = (float) wv;
                wpos = (wpos + 1) & mask;
                return inv + mix * (delayed - inv);
            }
        } refL, refR;
        refL.prepare (kSR); refR.prepare (kSR);
        double phase = 0.0;
        // float-seeded constants: the wrapper receives FLOAT params, and
        // double(0.3f) != 0.3 — parity is about the DSP loop, not literals
        const double inc = (double) 0.3f / kSR, base = 0.004 * kSR, depth = (double) 0.5f * 0.003 * kSR;
        const double fbRef = (double) 0.4f * 0.9, mixRef = (double) 0.25f;

        for (float s : in)
        {
            float l = s, r = -s;
            w.processSample (l, r);
            const double tri  = 2.0 * std::abs (2.0 * phase - 1.0) - 1.0;
            const double triR = 2.0 * std::abs (2.0 * (phase >= 0.75 ? phase - 0.75 : phase + 0.25) - 1.0) - 1.0;
            phase += inc; if (phase >= 1.0) phase -= 1.0;
            const float el = (float) refL.process ((double) s, std::max (1.0, base + tri * depth), fbRef, mixRef);
            const float er = (float) refR.process ((double) -s, std::max (1.0, base + triR * depth), fbRef, mixRef);
            REQUIRE (l == el);
            REQUIRE (r == er);
        }
    }
    SECTION ("compressor (adapted) — bit-exact vs the lifted VAZ loop")
    {
        mb::MbComp w;
        w.prepare (kSR);
        w.setParams (-18.0f, 4.0f, 5.0f, 80.0f, 3.0f);

        float envGr = 0.0f;
        const float slope = 1.0f - 1.0f / 4.0f;
        const float atk = std::exp (-1.0f / (5.0f * 0.001f * (float) kSR));
        const float rel = std::exp (-1.0f / (80.0f * 0.001f * (float) kSR));
        const float makeup = std::pow (10.0f, 3.0f / 20.0f);

        for (float s : in)
        {
            float l = s * 2.0f, r = -s * 2.0f;
            w.processSample (l, r);

            const float detect = std::max (std::abs (s * 2.0f), std::abs (-s * 2.0f));
            const float levelDb = 20.0f * std::log10 (std::max (detect, 1.0e-7f));
            const float over = levelDb - (-18.0f);
            const float target = over > 0.0f ? over * slope : 0.0f;
            const float coef = (target > envGr) ? atk : rel;
            envGr = coef * envGr + (1.0f - coef) * target;
            if (envGr < 1.0e-6f) envGr = 0.0f;
            const float g = makeup * std::pow (10.0f, -envGr / 20.0f);
            REQUIRE (l == s * 2.0f * g);
            REQUIRE (r == -s * 2.0f * g);
        }
    }
}

TEST_CASE ("fx: bypass is bit-exact; all-bypassed chain transparent; zero added latency")   // conditions b + c
{
    mb::MbFxChain chain;
    chain.prepare (kSR);                        // everything defaults to off
    const auto in = noiseBuf (8192, 7u);
    for (float s : in)
    {
        float l = s, r = -s;
        chain.processSample (l, r);
        REQUIRE (l == s);                       // untouched, sample for sample
        REQUIRE (r == -s);
    }

    // condition c: no effect adds latency — an impulse through the ALL-ON chain
    // appears at index 0 (every dry path is instantaneous). Total plugin latency
    // therefore remains MbSaturator::kLatency, accounted in one place.
    chain.reset();
    chain.onChorus = chain.onPhaser = chain.onFlanger = true;
    chain.onDelay = chain.onReverb = chain.onComp = true;
    chain.chorus.setParams (0.6f, 0.3f, 0.3f);
    chain.phaser.setParams (0.4f, 0.5f, 0.3f, 0.3f);
    chain.flanger.setParams (0.3f, 0.5f, 0.4f, 0.25f);
    chain.delay.setParams (128.0, 1, 0.35f, 0.4f, 0.2f, true);
    chain.reverb.setParams (0.5f, 0.5f, 0.15f);
    chain.comp.setParams (-12.0f, 3.0f, 5.0f, 100.0f, 0.0f);
    float l = 1.0f, r = 1.0f;
    chain.processSample (l, r);
    CHECK (std::abs (l) > 0.05f);               // impulse passes in the same frame
}

TEST_CASE ("fx: delay division math + echo lands on the grid")   // condition d
{
    using mb::MbDelayFx;
    CHECK (MbDelayFx::divBeats (0) == Approx (1.0));
    CHECK (MbDelayFx::divBeats (1) == Approx (0.5));
    CHECK (MbDelayFx::divBeats (2) == Approx (0.75));       // 1/8.
    CHECK (MbDelayFx::divBeats (3) == Approx (1.0 / 3.0));  // 1/8T
    CHECK (MbDelayFx::divBeats (4) == Approx (0.25));
    CHECK (MbDelayFx::divBeats (5) == Approx (0.375));      // 1/16.
    CHECK (MbDelayFx::divBeats (6) == Approx (0.75));       // 3/16

    mb::MbDelayFx d;
    d.prepare (kSR);
    d.setParams (128.0, 1, 0.0f, 0.0f, 1.0f, false);        // 1/8 @128 → 10336 samples
    d.curDel = d.tgtDel;
    const int expected = (int) std::lround (0.5 * (60.0 / 128.0) * kSR);
    int firstEcho = -1;
    for (int i = 0; i < expected + 500; ++i)
    {
        float l = (i == 0) ? 0.8f : 0.0f, r = l;
        d.processSample (l, r);
        if (i > 100 && firstEcho < 0 && std::abs (l) > 0.05f) firstEcho = i;
    }
    INFO ("expected " << expected << " first echo " << firstEcho);
    CHECK (std::abs (firstEcho - expected) <= 2);
}

TEST_CASE ("fx: delay retime on BPM change is click-free (glide strategy)")   // condition d
{
    // Strategy (VAZDelay-proven): the tap glides through a ~20 ms one-pole →
    // repeats re-pitch briefly (tape-style), feedback history is preserved, no
    // buffer garbage. Isolation gate: HF at 6 kHz vs the constant-BPM baseline.
    auto maxHf = [] (bool changeBpm)
    {
        mb::MbDelayFx d;
        d.prepare (kSR);
        d.setParams (140.0, 1, 0.5f, 0.3f, 0.6f, true);
        d.curDel = d.tgtDel;
        teq::Biquad hp;
        hp.setCoeffs (teq::makeCoeffs (teq::FilterType::HPF, 6000.0, 0.0, 0.707, kSR));
        float worst = 0.0f;
        for (int i = 0; i < (int) (4.0 * kSR); ++i)
        {
            if (changeBpm && i == (int) (2.0 * kSR))
                d.setParams (120.0, 1, 0.5f, 0.3f, 0.6f, true);
            const float in = (i % 11025 < 2205)              // 150 Hz bursts → audible repeats
                           ? 0.5f * std::sin (2.0f * juce::MathConstants<float>::pi * 150.0f * (float) i / (float) kSR)
                           : 0.0f;
            float l = in, r = in;
            d.processSample (l, r);
            REQUIRE (std::isfinite (l));
            const float hf = std::abs (hp.process (l));
            if (i > (int) (1.9 * kSR)) worst = std::max (worst, hf);
        }
        return worst;
    };
    const float base = maxHf (false), changed = maxHf (true);
    INFO ("baseline HF " << base << ", retimed HF " << changed);
    CHECK (changed < 0.001f + 2.0f * base);
}

TEST_CASE ("fx: tails decay to exact zero under FTZ; finite at extremes")   // condition e
{
    juce::ScopedNoDenormals ftz;
    mb::MbFxChain chain;
    chain.prepare (kSR);
    chain.onChorus = chain.onPhaser = chain.onFlanger = true;
    chain.onDelay = chain.onReverb = chain.onComp = true;
    chain.chorus.setParams (0.6f, 0.3f, 0.3f);
    chain.phaser.setParams (0.4f, 0.5f, 0.3f, 0.3f);
    chain.flanger.setParams (0.3f, 0.5f, 0.4f, 0.25f);
    chain.delay.setParams (128.0, 1, 0.5f, 0.4f, 0.3f, true);
    chain.reverb.setParams (0.5f, 0.5f, 0.2f);
    chain.comp.setParams (-12.0f, 3.0f, 5.0f, 100.0f, 0.0f);

    for (int i = 0; i < 22050; ++i)                         // 0.5 s excitation
    {
        float l = 0.6f * std::sin (0.04f * (float) i), r = l;
        chain.processSample (l, r);
    }
    int zeroAt = -1;
    float l = 1.0f, r = 1.0f;
    for (int i = 0; i < (int) (10.0 * kSR); ++i)            // stated bound: 10 s
    {
        l = 0.0f; r = 0.0f;
        chain.processSample (l, r);
        if (zeroAt < 0 && l == 0.0f && r == 0.0f) zeroAt = i;
        else if (l != 0.0f || r != 0.0f) zeroAt = -1;       // must STAY zero
    }
    {
        float sl[6][2];
        auto probe = [&] (int idx, auto& fxu) { float a = 0, b = 0; fxu.processSample (a, b); sl[idx][0] = a; sl[idx][1] = b; };
        probe (0, chain.chorus); probe (1, chain.phaser); probe (2, chain.flanger);
        probe (3, chain.delay);  probe (4, chain.reverb); probe (5, chain.comp);
        char diag[256];
        std::snprintf (diag, sizeof (diag),
            "final l=%.9e r=%.9e zeroAt=%d | cho %.3e pha %.3e fla %.3e dly %.3e rev %.3e cmp %.3e",
            (double) l, (double) r, zeroAt, (double) sl[0][0], (double) sl[1][0], (double) sl[2][0],
            (double) sl[3][0], (double) sl[4][0], (double) sl[5][0]);
        INFO (diag);
        CHECK (l == 0.0f);
        CHECK (r == 0.0f);
        CHECK (zeroAt >= 0);
    }

    // extremes: everything maxed, hot input → finite
    chain.reset();
    chain.flanger.setParams (5.0f, 1.0f, 0.95f, 1.0f);
    chain.delay.setParams (300.0, 6, 0.95f, 0.0f, 1.0f, true);
    chain.reverb.setParams (1.0f, 0.0f, 1.0f);
    chain.comp.setParams (-40.0f, 10.0f, 0.1f, 20.0f, 12.0f);
    chain.chorus.setParams (5.0f, 1.0f, 1.0f);
    chain.phaser.setParams (5.0f, 1.0f, 0.95f, 1.0f);
    for (int i = 0; i < (int) (5.0 * kSR); ++i)
    {
        float ll = 0.95f * std::sin (0.3f * (float) i), rr = -ll;
        chain.processSample (ll, rr);
        REQUIRE (std::isfinite (ll));
        REQUIRE (std::isfinite (rr));
    }
}

TEST_CASE ("fx: mono compatibility below 300 Hz at default settings")   // condition f
{
    // The instrument's output is mono-summed constantly in use. Gate: at the
    // DEFAULT parameter values, the mono sum of each modulated effect must not
    // dip more than 3 dB below the dry mono response anywhere in 20-300 Hz
    // (time-averaged; the flanger achieves this via its bass-safe wet HP).
    auto monoDipDb = [] (int which)
    {
        constexpr int kN = 1 << 14, kAvg = 10;
        std::vector<double> accWet ((size_t) kN / 2, 0.0), accDry ((size_t) kN / 2, 0.0);

        mb::MbChorus cho; cho.prepare (kSR); cho.setParams (0.6f, 0.3f, 0.3f);
        mb::MbPhaser pha; pha.prepare (kSR); pha.setParams (0.4f, 0.5f, 0.3f, 0.3f);
        mb::MbFlanger fla; fla.prepare (kSR); fla.setParams (0.3f, 0.5f, 0.4f, 0.25f);

        uint32_t rng = 99u;
        juce::dsp::FFT fft (14);
        for (int seg = 0; seg < kAvg; ++seg)
        {
            std::vector<float> wet ((size_t) kN), dry ((size_t) kN);
            for (int i = 0; i < kN; ++i)
            {
                rng ^= rng << 13; rng ^= rng >> 17; rng ^= rng << 5;
                const float s = (float) ((int32_t) rng) * (0.4f / 2147483648.0f);
                float l = s, r = s;
                if (which == 0) cho.processSample (l, r);
                if (which == 1) pha.processSample (l, r);
                if (which == 2) fla.processSample (l, r);
                wet[(size_t) i] = 0.5f * (l + r);
                dry[(size_t) i] = s;
            }
            auto spec = [&fft] (std::vector<float>& x)
            {
                std::vector<float> buf ((size_t) kN * 2, 0.0f);
                for (int i = 0; i < kN; ++i)
                {
                    const double t = 2.0 * juce::MathConstants<double>::pi * i / kN;
                    buf[(size_t) i] = x[(size_t) i] * (float) (0.5 - 0.5 * std::cos (t));
                }
                fft.performFrequencyOnlyForwardTransform (buf.data());
                return buf;
            };
            auto ws = spec (wet), ds = spec (dry);
            for (int b = 0; b < kN / 2; ++b)
            {
                accWet[(size_t) b] += (double) ws[(size_t) b] * ws[(size_t) b];
                accDry[(size_t) b] += (double) ds[(size_t) b] * ds[(size_t) b];
            }
        }
        double worst = 0.0;
        const double binHz = kSR / (1 << 14);
        for (int b = std::max (1, (int) (20.0 / binHz)); b * binHz <= 300.0; ++b)
        {
            const double d = 10.0 * std::log10 (std::max (accWet[(size_t) b], 1.0e-30)
                                              / std::max (accDry[(size_t) b], 1.0e-30));
            worst = std::min (worst, d);
        }
        return worst;
    };

    const double cho = monoDipDb (0), pha = monoDipDb (1), fla = monoDipDb (2);
    INFO ("worst mono dip 20-300 Hz: chorus " << cho << " dB, phaser " << pha << " dB, flanger " << fla << " dB");
    CHECK (cho > -3.0);
    CHECK (pha > -3.0);
    CHECK (fla > -3.0);
}

TEST_CASE ("fx: compressor static curve, timing, and zipper")   // condition g
{
    SECTION ("static curve within 1 dB (hard knee, documented)")
    {
        for (float ratio : { 2.0f, 8.0f })
            for (float overDb : { 6.0f, 12.0f })
            {
                mb::MbComp c;
                c.prepare (kSR);
                c.setParams (-20.0f, ratio, 1.0f, 50.0f, 0.0f);
                const float amp = std::pow (10.0f, (-20.0f + overDb) / 20.0f);
                double inRms = 0.0, outRms = 0.0;
                for (int i = 0; i < 44100; ++i)
                {
                    const float s = amp * std::sin (2.0f * juce::MathConstants<float>::pi * 200.0f * (float) i / (float) kSR);
                    float l = s, r = s;
                    c.processSample (l, r);
                    if (i > 22050) { inRms += (double) s * s; outRms += (double) l * l; }
                }
                const double grMeas = -10.0 * std::log10 (outRms / inRms);
                const double grWant = overDb * (1.0 - 1.0 / ratio);
                INFO ("ratio " << ratio << " over " << overDb << ": GR " << grMeas << " want " << grWant);
                CHECK (grMeas == Approx (grWant).margin (1.0));
            }
    }
    SECTION ("attack/release timing within 10%")
    {
        mb::MbComp c;
        c.prepare (kSR);
        c.setParams (-20.0f, 4.0f, 20.0f, 150.0f, 0.0f);
        // SQUARE drive: |sample| is constant, so the GR target is a clean step
        // (a sine's instantaneous peak dips through zero every half-cycle and
        // drags the measured trajectory toward the release coefficient).
        int t63a = -1, t63r = -1;
        float targetGr = 12.0f * 0.75f;
        for (int i = 0; i < (int) kSR; ++i)
        {
            const float amp = (i < 11025) ? 0.05f : std::pow (10.0f, -8.0f / 20.0f);
            const float sq = ((i / 22) % 2 == 0) ? amp : -amp;    // ~1 kHz square
            float l = sq, r = sq;
            c.processSample (l, r);
            if (i >= 11025 && t63a < 0 && c.envGr >= targetGr * 0.632f) t63a = i - 11025;
        }
        const float grAtRelease = c.envGr;
        for (int i = 0; i < (int) kSR; ++i)
        {
            float l = ((i / 22) % 2 == 0) ? 0.001f : -0.001f, r = l;
            c.processSample (l, r);
            if (t63r < 0 && c.envGr <= grAtRelease * (1.0f - 0.632f)) t63r = i;
        }
        const double atkMs = 1000.0 * t63a / kSR, relMs = 1000.0 * t63r / kSR;
        INFO ("attack t63 " << atkMs << " ms (want 20), release t63 " << relMs << " ms (want 150)");
        CHECK (atkMs == Approx (20.0).epsilon (0.10));
        CHECK (relMs == Approx (150.0).epsilon (0.10));
    }
    SECTION ("no GR zipper (isolation gate)")
    {
        auto maxHf = [] (bool compOn)
        {
            mb::MbComp c;
            c.prepare (kSR);
            c.setParams (-20.0f, 4.0f, 5.0f, 80.0f, 0.0f);
            teq::Biquad hp;
            hp.setCoeffs (teq::makeCoeffs (teq::FilterType::HPF, 6000.0, 0.0, 0.707, kSR));
            float worst = 0.0f;
            for (int i = 0; i < 88200; ++i)
            {
                const float am = 0.3f + 0.25f * std::sin (2.0f * juce::MathConstants<float>::pi * 4.0f * (float) i / (float) kSR);
                float l = am * std::sin (2.0f * juce::MathConstants<float>::pi * 150.0f * (float) i / (float) kSR), r = l;
                if (compOn) c.processSample (l, r);
                const float hf = std::abs (hp.process (l));
                if (i > 22050) worst = std::max (worst, hf);
            }
            return worst;
        };
        const float off = maxHf (false), on = maxHf (true);
        INFO ("HF off " << off << " on " << on);
        CHECK (on < 0.001f + 2.0f * off);
    }
}

TEST_CASE ("fx: CPU spot check — full chain + 8-voice unison vs the 5% target")   // condition h
{
    MidBassAudioProcessor proc;
    proc.setPlayConfigDetails (0, 2, 44100.0, 512);
    proc.prepareToPlay (44100.0, 512);
    auto set = [&proc] (const char* id, float plain)
    {
        auto* par = proc.apvts.getParameter (id);
        par->setValueNotifyingHost (par->convertTo0to1 (plain));
    };
    set (mb::pid::uni_voices, 8.0f);
    set (mb::pid::uni_detune, 50.0f);
    set (mb::pid::sat_drive, 60.0f);
    set (mb::pid::trans_attack, 50.0f);
    for (const char* id : { mb::pid::fx_cho_on, mb::pid::fx_pha_on, mb::pid::fx_fla_on,
                            mb::pid::fx_dly_on, mb::pid::fx_rev_on, mb::pid::fx_cmp_on })
        set (id, 1.0f);

    juce::AudioBuffer<float> buf (2, 512);
    juce::MidiBuffer midi;
    midi.addEvent (juce::MidiMessage::noteOn (1, 45, (juce::uint8) 100), 0);
    proc.processBlock (buf, midi); midi.clear();

    const int blocks = (int) (2.0 * 44100.0 / 512.0);
    const auto t0 = std::chrono::steady_clock::now();
    for (int b = 0; b < blocks; ++b) { proc.processBlock (buf, midi); }
    const auto t1 = std::chrono::steady_clock::now();
    const double wall = std::chrono::duration<double> (t1 - t0).count();
    const double audio = blocks * 512.0 / 44100.0;
    const double pct = 100.0 * wall / audio;
    INFO ("CPU: " << pct << "% of one core (target < 5%), audio " << audio << " s in " << wall << " s");
    CHECK (pct < 50.0);                                     // loose CI gate; actual value reported
}
