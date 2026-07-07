// Phase 5 — saturation + EQ + transient shaper (test-first).
// Conditions (2026-07-07): a) OS latency alignment / null test + host latency
// report · b) per-type alias suppression, Phase 1 methodology · c) documented
// THD ranking + harmonic signatures · d) drive-0 transparency · e) EQ vs
// analytic RBJ at 44.1k/96k · f) transient gain bounds + isolation artifacts ·
// g) denormal/flush behaviour.
#include <catch2/catch_test_macros.hpp>
#include <catch2/catch_approx.hpp>
#include <juce_audio_processors/juce_audio_processors.h>
#include <juce_dsp/juce_dsp.h>
#include "MbTone.h"
#include "PluginProcessor.h"
#include <cmath>
#include <vector>
#include <complex>

using Catch::Approx;
namespace
{
// Blackman-Harris spectrum helper (Phase 1 methodology).
struct BhSpectrum
{
    static constexpr int kOrder = 15, kN = 1 << kOrder;
    std::vector<float> mag;
    double binHz;

    BhSpectrum (const std::vector<float>& x, double sr) : mag ((size_t) kN / 2, 0.0f), binHz (sr / kN)
    {
        REQUIRE ((int) x.size() >= kN);
        juce::dsp::FFT fft (kOrder);
        std::vector<float> buf ((size_t) kN * 2, 0.0f);
        for (int i = 0; i < kN; ++i)
        {
            const double t = 2.0 * juce::MathConstants<double>::pi * i / kN;
            buf[(size_t) i] = x[(size_t) i]
                * (float) (0.35875 - 0.48829 * std::cos (t) + 0.14128 * std::cos (2 * t) - 0.01168 * std::cos (3 * t));
        }
        fft.performFrequencyOnlyForwardTransform (buf.data());
        for (int i = 0; i < kN / 2; ++i) mag[(size_t) i] = buf[(size_t) i];
    }
    float magNear (double hz, int guard = 4) const
    {
        float m = 0.0f;
        const int c = (int) std::lround (hz / binHz);
        for (int b = c - guard; b <= c + guard; ++b)
            if (b > 0 && b < kN / 2) m = std::max (m, mag[(size_t) b]);
        return m;
    }
    float maxNonHarmonic (double f0, int guard = 8) const
    {
        float worst = 0.0f;
        for (int b = 8; b < kN / 2; ++b)
        {
            const double k = b * binHz / f0;
            const double distBins = std::abs (k - std::round (k)) * f0 / binHz;
            if (std::round (k) >= 1.0 && distBins <= (double) guard) continue;
            worst = std::max (worst, mag[(size_t) b]);
        }
        return worst;
    }
};

double dB (double a, double ref) { return 20.0 * std::log10 (std::max (a, 1.0e-15) / std::max (ref, 1.0e-15)); }

// THD (h2..h10 energy vs fundamental) of a saturator at a given type/drive.
double satThd (int type, float drive, double f0 = 200.0, double sr = 44100.0)
{
    mb::MbSaturator s;
    s.prepare (sr);
    s.setParams (type, drive, 1.0f);
    std::vector<float> x ((size_t) BhSpectrum::kN + 4096);
    for (size_t i = 0; i < x.size(); ++i)
    {
        float l = 0.5f * std::sin (2.0f * juce::MathConstants<float>::pi * (float) f0 * (float) i / (float) sr), r = l;
        s.processSample (l, r);
        x[i] = l;
    }
    x.erase (x.begin(), x.begin() + 4096);                  // settle
    BhSpectrum sp (x, sr);
    const double fund = sp.magNear (f0);
    double harm = 0.0;
    for (int k = 2; k <= 10; ++k)
    {
        const double h = sp.magNear (f0 * k);
        harm += h * h;
    }
    return 10.0 * std::log10 (std::max (harm, 1.0e-24) / std::max (fund * fund, 1.0e-24));
}
} // namespace

//==============================================================================
TEST_CASE ("sat: latency-aligned parallel mix nulls at unity transfer")   // condition a
{
    // forceWetForTest sends the signal through the FULL up/down OS chain with an
    // identity shaper; the dry path is the 47-sample delay. Any misalignment or
    // FIR passband error shows as residual. Gate: < -60 dB vs the input spectrum
    // across 20 Hz - 20 kHz, three noise seeds, 44.1k and 96k.
    for (double sr : { 44100.0, 96000.0 })
        for (uint32_t seed : { 1u, 2u, 3u })
        {
            mb::MbSaturator s;
            s.prepare (sr);
            s.setParams (mb::SatType::SoftClip, 0.0f, 0.5f);   // Mix 50 %, drive 0
            s.forceWetForTest = true;

            uint32_t rng = seed * 2654435761u;
            auto noise = [&rng]() { rng ^= rng << 13; rng ^= rng >> 17; rng ^= rng << 5;
                                    return (float) ((int32_t) rng) * (0.5f / 2147483648.0f); };
            const int n = BhSpectrum::kN + mb::MbSaturator::kLatency;
            std::vector<float> in ((size_t) n), out ((size_t) n);
            for (int i = 0; i < n; ++i)
            {
                in[(size_t) i] = noise();
                float l = in[(size_t) i], r = l;
                s.processSample (l, r);
                out[(size_t) i] = l;
            }
            // residual = out[i] - in[i - latency]
            std::vector<float> res ((size_t) BhSpectrum::kN), ref ((size_t) BhSpectrum::kN);
            for (int i = 0; i < BhSpectrum::kN; ++i)
            {
                res[(size_t) i] = out[(size_t) (i + mb::MbSaturator::kLatency)] - in[(size_t) i];
                ref[(size_t) i] = in[(size_t) i];
            }
            BhSpectrum rs (res, sr), is (ref, sr);
            // Attribution (2026-07-07 review note): FIR passband droop lives above
            // ~15 kHz; misalignment would show as BROADBAND comb residue. Gate the
            // sub-15k region harder so a future regression is attributable.
            double worst = -1000.0, worstLow = -1000.0;
            for (int b = std::max (1, (int) (20.0 / rs.binHz)); b * rs.binHz <= 20000.0 && b < BhSpectrum::kN / 2; ++b)
            {
                const double d = dB (rs.mag[(size_t) b], is.mag[(size_t) b]);
                worst = std::max (worst, d);
                if (b * rs.binHz < 15000.0) worstLow = std::max (worstLow, d);
            }
            INFO ("sr " << sr << " seed " << seed << " worst residual " << worst
                  << " dB (below 15 kHz: " << worstLow << " dB)");
            CHECK (worst < -60.0);
            CHECK (worstLow < -70.0);          // alignment-clean; residual = HF droop only
        }
}

TEST_CASE ("processor: reports the OS latency to the host")   // condition a
{
    MidBassAudioProcessor p;
    p.setPlayConfigDetails (0, 2, 44100.0, 512);
    p.prepareToPlay (44100.0, 512);
    CHECK (p.getLatencySamples() == mb::MbSaturator::kLatency);
}

TEST_CASE ("sat: drive 0 is bit-exact vs bypass (per type)")   // condition d
{
    for (int type = 0; type < 5; ++type)
    {
        // Drive 0 must equal a pure 47-sample delay of the input, bit for bit
        // (the wet path is skipped; only the alignment delay runs).
        mb::MbSaturator s2;
        s2.prepare (44100.0);
        s2.setParams (type, 0.0f, 0.3f);
        std::vector<float> inBuf;
        uint32_t rng = 913u;
        bool exact = true;
        for (int i = 0; i < 8192; ++i)
        {
            rng ^= rng << 13; rng ^= rng >> 17; rng ^= rng << 5;
            const float v = (float) ((int32_t) rng) * (0.5f / 2147483648.0f);
            inBuf.push_back (v);
            float l = v, r = v;
            s2.processSample (l, r);
            if (i >= mb::MbSaturator::kLatency)
                if (l != inBuf[(size_t) (i - mb::MbSaturator::kLatency)]) exact = false;
        }
        INFO ("type " << type);
        CHECK (exact);                                     // bit-exact delayed passthrough
    }
}

TEST_CASE ("sat: alias suppression per type — OS on vs base-rate shaping")   // condition b
{
    // Phase 1 methodology at a 3 kHz fundamental. "OS off" reference = the same
    // transfer applied at base rate (no up/down). Per-type gates stated
    // explicitly (measured @2026-07-07: Tape/Tube -80, SoftClip -80, Diode after
    // the C1 fix ~-60s, HardClip -46). HardClip is a brickwall — unbounded
    // spectrum, 2x OS buys ~10 dB — so its improvement gate is 8 dB where the
    // smooth types must gain >= 12 dB.
    const double sr = 44100.0, f0 = 3000.0;
    struct Gate { int type; double osGate; double improveGate; };
    const Gate gates[5] = { { mb::SatType::Tape,     -55.0, 12.0 },
                            { mb::SatType::Tube,     -55.0, 12.0 },
                            { mb::SatType::Diode,    -52.0, 12.0 },   // measured -53.5 (C1 fix)
                            { mb::SatType::SoftClip, -55.0, 12.0 },
                            { mb::SatType::HardClip, -44.0,  8.0 } };

    for (const auto& g : gates)
    {
        auto aliasOf = [&] (bool oversampled)
        {
            mb::MbSaturator s;
            s.prepare (sr);
            s.setParams (g.type, 0.7f, 1.0f);
            std::vector<float> x ((size_t) BhSpectrum::kN + 4096);
            for (size_t i = 0; i < x.size(); ++i)
            {
                const float v = 0.5f * std::sin (2.0f * juce::MathConstants<float>::pi * (float) f0 * (float) i / (float) sr);
                if (oversampled)
                {
                    float l = v, r = v;
                    s.processSample (l, r);
                    x[i] = l;
                }
                else
                    x[i] = s.shape (v * s.gain);           // same transfer, base rate
            }
            x.erase (x.begin(), x.begin() + 4096);
            BhSpectrum sp (x, sr);
            return dB (sp.maxNonHarmonic (f0), sp.magNear (f0));
        };

        const double osOn = aliasOf (true), osOff = aliasOf (false);
        INFO ("type " << g.type << ": OS on " << osOn << " dB, base-rate " << osOff << " dB");
        CHECK (osOn < g.osGate);
        CHECK (osOn < osOff - g.improveGate);
    }
}

TEST_CASE ("sat: THD character ranking documented and stable")   // condition c
{
    // Reference drive 0.5, 200 Hz, 0.5 amplitude. Documented order (gentle→harsh):
    //   SoftClip < Tape < Tube < Diode < HardClip
    // Signatures: SoftClip/Tape/HardClip symmetric (odd-only; h2 negligible);
    // Tube bias-shifted tanh (h2 strong); Diode asymmetric (h2 strongest).
    const double tSoft = satThd (mb::SatType::SoftClip, 0.5f);
    const double tTape = satThd (mb::SatType::Tape,     0.5f);
    const double tTube = satThd (mb::SatType::Tube,     0.5f);
    const double tDio  = satThd (mb::SatType::Diode,    0.5f);
    const double tHard = satThd (mb::SatType::HardClip, 0.5f);
    INFO ("THD dB — Soft " << tSoft << ", Tape " << tTape << ", Tube " << tTube
          << ", Diode " << tDio << ", Hard " << tHard);
    CHECK (tSoft < tTape);
    CHECK (tTape < tTube);
    CHECK (tTube < tDio);
    CHECK (tDio  < tHard);

    // even/odd signature: h2 vs h3 per family
    auto h2h3 = [] (int type)
    {
        mb::MbSaturator s;
        s.prepare (44100.0);
        s.setParams (type, 0.5f, 1.0f);
        std::vector<float> x ((size_t) BhSpectrum::kN + 4096);
        for (size_t i = 0; i < x.size(); ++i)
        {
            float l = 0.5f * std::sin (2.0f * juce::MathConstants<float>::pi * 200.0f * (float) i / 44100.0f), r = l;
            s.processSample (l, r);
            x[i] = l;
        }
        x.erase (x.begin(), x.begin() + 4096);
        BhSpectrum sp (x, 44100.0);
        return dB (sp.magNear (400.0), sp.magNear (600.0));   // h2 vs h3
    };
    // Family signatures (measured 2026-07-07): symmetric types keep h2 >= 20 dB
    // below h3; the asymmetric types carry h2 within ~5-12 dB of h3 (audible
    // warmth) — deep-drive tanh still makes h3 dominate in absolute terms, so
    // the meaningful gate is the FAMILY separation, not h2 > h3.
    CHECK (h2h3 (mb::SatType::Tape) < -20.0);
    CHECK (h2h3 (mb::SatType::SoftClip) < -20.0);
    CHECK (h2h3 (mb::SatType::Tube) > -12.0);
    CHECK (h2h3 (mb::SatType::Diode) > -12.0);
}

TEST_CASE ("eq: magnitude matches analytic RBJ at corners and extremes, 44.1k + 96k")   // condition e
{
    auto analyticDb = [] (const teq::Coeffs& c, double hz, double sr)
    {
        const std::complex<double> z = std::exp (std::complex<double> (0.0, -2.0 * juce::MathConstants<double>::pi * hz / sr));
        const auto num = c.b0 + c.b1 * z + c.b2 * z * z;
        const auto den = 1.0 + c.a1 * z + c.a2 * z * z;
        return 20.0 * std::log10 (std::abs (num / den));
    };

    for (double sr : { 44100.0, 96000.0 })
    {
        struct Cfg { double lsF, lsG, midF, midG, midQ, hsF, hsG; };
        for (const Cfg cfg : { Cfg { 120.0, 12.0, 300.0, -12.0, 0.3, 6000.0, 12.0 },
                               Cfg { 120.0, -12.0, 400.0, 12.0, 6.0, 8000.0, -12.0 } })
        {
            mb::MbToneEq eq;
            eq.prepare (sr);
            eq.setParams (cfg.lsF, cfg.lsG, cfg.midF, cfg.midG, cfg.midQ, cfg.hsF, cfg.hsG);

            constexpr int kN = 1 << 15;
            std::vector<float> ir ((size_t) kN, 0.0f);
            for (int i = 0; i < kN; ++i)
            {
                float l = (i == 0) ? 1.0f : 0.0f, r = l;
                eq.processSample (l, r);
                ir[(size_t) i] = l;
            }
            juce::dsp::FFT fft (15);
            std::vector<float> buf ((size_t) kN * 2, 0.0f);
            std::copy (ir.begin(), ir.end(), buf.begin());
            fft.performFrequencyOnlyForwardTransform (buf.data());

            const auto cLs  = teq::makeCoeffs (teq::FilterType::LowShelf,  cfg.lsF,  cfg.lsG,  0.707, sr);
            const auto cMid = teq::makeCoeffs (teq::FilterType::Peak,      cfg.midF, cfg.midG, cfg.midQ, sr);
            const auto cHs  = teq::makeCoeffs (teq::FilterType::HighShelf, cfg.hsF,  cfg.hsG,  0.707, sr);

            for (double hz : { 40.0, cfg.lsF, cfg.midF, 1000.0, cfg.hsF, 15000.0 })
            {
                const double expected = analyticDb (cLs, hz, sr) + analyticDb (cMid, hz, sr) + analyticDb (cHs, hz, sr);
                const int bin = (int) std::lround (hz / (sr / kN));
                const double measured = 20.0 * std::log10 (std::max ((double) buf[(size_t) bin], 1.0e-12));
                INFO ("sr " << sr << " hz " << hz << " expected " << expected << " measured " << measured);
                CHECK (measured == Approx (expected).margin (0.35));
            }
        }
    }
}

TEST_CASE ("transient: gain bounded, attack enhancement real, no chatter on steady tone")   // condition f
{
    // gain bound: output/input ratio can never exceed kMaxGain
    {
        mb::MbTransient tr;
        tr.prepare (44100.0);
        tr.setParams (1.0f, 1.0f);
        uint32_t rng = 5u;
        for (int i = 0; i < 44100; ++i)
        {
            rng ^= rng << 13; rng ^= rng >> 17; rng ^= rng << 5;
            const float v = ((i / 1000) % 2 == 0) ? (float) ((int32_t) rng) * (0.8f / 2147483648.0f) : 0.001f;
            float l = v, r = v;
            tr.processSample (l, r);
            REQUIRE (std::isfinite (l));
            if (std::abs (v) > 1.0e-4f)
                REQUIRE (std::abs (l) <= std::abs (v) * mb::MbTransient::kMaxGain * 1.001f);
        }
    }

    // attack enhancement on a reference pluck (decaying 150 Hz burst)
    auto attackRatio = [] (float amt)
    {
        mb::MbTransient tr;
        tr.prepare (44100.0);
        tr.setParams (amt, 0.0f);
        double peakEarly = 0.0, rmsLate = 0.0;
        int lateCount = 0;
        for (int i = 0; i < 22050; ++i)
        {
            const float env = std::exp (-(float) i / 8820.0f);
            float l = env * std::sin (2.0f * juce::MathConstants<float>::pi * 150.0f * (float) i / 44100.0f), r = l;
            tr.processSample (l, r);
            if (i < 441) peakEarly = std::max (peakEarly, (double) std::abs (l));
            if (i > 8820) { rmsLate += (double) l * l; ++lateCount; }
        }
        return peakEarly / std::sqrt (rmsLate / lateCount);
    };
    const double off = attackRatio (0.0f), on = attackRatio (1.0f);
    INFO ("attack/sustain ratio off " << off << " on " << on);
    CHECK (on > off * 1.5);                                  // audible punch added

    // isolation: steady sine, full amounts → gain settles, no HF chatter
    {
        mb::MbTransient tr;
        tr.prepare (44100.0);
        tr.setParams (1.0f, 1.0f);
        teq::Biquad hp;
        hp.setCoeffs (teq::makeCoeffs (teq::FilterType::HPF, 6000.0, 0.0, 0.707, 44100.0));
        float worst = 0.0f;
        for (int i = 0; i < 88200; ++i)
        {
            const float in = 0.5f * std::sin (2.0f * juce::MathConstants<float>::pi * 150.0f * (float) i / 44100.0f);
            float l = in, r = in;
            tr.processSample (l, r);
            const float hf = std::abs (hp.process (l));      // a 150 Hz tone has no 6 kHz energy
            if (i > 22050) worst = std::max (worst, hf);
        }
        INFO ("steady-tone HF " << worst);
        CHECK (worst < 0.002f);                              // ≈ -54 dBFS: envelope ripple, no chatter
    }
}

TEST_CASE ("tone chain: denormal-safe — sustained silence flushes to exact zero")   // condition g
{
    juce::ScopedNoDenormals ftz;                             // the processBlock environment

    mb::MbSaturator s; s.prepare (44100.0); s.setParams (mb::SatType::Tube, 0.8f, 0.5f);
    mb::MbToneEq eq;   eq.prepare (44100.0); eq.setParams (120.0, 8.0, 300.0, -6.0, 2.0, 6000.0, 8.0);
    mb::MbTransient tr; tr.prepare (44100.0); tr.setParams (1.0f, -1.0f);

    for (int i = 0; i < 4096; ++i)                           // excite everything
    {
        float l = 0.7f * std::sin (0.05f * (float) i), r = l;
        s.processSample (l, r); eq.processSample (l, r); tr.processSample (l, r);
    }
    float l = 1.0f, r = 1.0f;
    for (int i = 0; i < (int) (2.0 * 44100.0); ++i)          // then 2 s of exact silence
    {
        l = 0.0f; r = 0.0f;
        s.processSample (l, r); eq.processSample (l, r); tr.processSample (l, r);
    }
    CHECK (l == 0.0f);
    CHECK (r == 0.0f);
    CHECK (std::fpclassify (tr.fast) != FP_SUBNORMAL);
    CHECK (std::fpclassify (tr.slow) != FP_SUBNORMAL);
    CHECK (std::fpclassify (eq.ls[0].z1) != FP_SUBNORMAL);
    CHECK (std::fpclassify (eq.mid[0].z1) != FP_SUBNORMAL);
}
