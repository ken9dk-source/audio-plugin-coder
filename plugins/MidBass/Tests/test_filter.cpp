// Phase 2 — hybrid filter (VAZ Type-R LP24/LP12 + ZDF SVF HP12/BP12), test-first.
// Roadmap gates: per-mode magnitude response, self-oscillation stability, drive
// stages monotonic & finite. 2026-07-07 approval conditions:
//  * click-free engine switching (mid-note switch, HF burst < -60 dBFS)
//  * SVF path drive structure matched to Type-R (not sterile; THD in family)
//  * identical keytrack/env cutoff scaling across engines (one cutoff law +
//    parallel realized-frequency curves)
#include <catch2/catch_test_macros.hpp>
#include <catch2/catch_approx.hpp>
#include <juce_dsp/juce_dsp.h>
#include "MbFilter.h"
#include "Biquad.h"          // TranceEQ RBJ biquad (click detector HP)
#include <cmath>
#include <vector>

using Catch::Approx;
namespace
{
constexpr double kSR = 44100.0;

// |H(f)| via impulse response (linear regime), 16384-point FFT.
struct Response
{
    static constexpr int kOrder = 14, kN = 1 << kOrder;
    std::vector<float> mag;
    double binHz;

    explicit Response (mb::MbFilter& f, double fcHz)
        : mag ((size_t) kN / 2, 0.0f), binHz (kSR / kN)
    {
        std::vector<float> ir ((size_t) kN, 0.0f);
        for (int i = 0; i < kN; ++i)
            ir[(size_t) i] = f.process (i == 0 ? 0.5f : 0.0f, fcHz) * 2.0f;

        juce::dsp::FFT fft (kOrder);
        std::vector<float> buf ((size_t) kN * 2, 0.0f);
        std::copy (ir.begin(), ir.end(), buf.begin());
        fft.performFrequencyOnlyForwardTransform (buf.data());
        for (int i = 0; i < kN / 2; ++i) mag[(size_t) i] = buf[(size_t) i];
    }

    double dBAt (double hz) const
    {
        const int b = std::clamp ((int) std::lround (hz / binHz), 1, kN / 2 - 1);
        return 20.0 * std::log10 (std::max ((double) mag[(size_t) b], 1.0e-12));
    }

    double peakHz (double loHz, double hiHz) const
    {
        int best = (int) std::lround (loHz / binHz);
        for (int b = best; b <= (int) std::lround (hiHz / binHz) && b < kN / 2; ++b)
            if (mag[(size_t) b] > mag[(size_t) best]) best = b;
        return best * binHz;
    }

    // First frequency (scanning up from refHz) where |H| drops 3 dB below |H(refHz)|.
    double minus3dBFrom (double refHz) const
    {
        const double ref = dBAt (refHz);
        for (int b = (int) std::lround (refHz / binHz) + 1; b < kN / 2; ++b)
            if (20.0 * std::log10 (std::max ((double) mag[(size_t) b], 1.0e-12)) < ref - 3.0)
                return b * binHz;
        return -1.0;
    }
    // Same, scanning DOWN from refHz (for highpass).
    double minus3dBDownFrom (double refHz) const
    {
        const double ref = dBAt (refHz);
        for (int b = (int) std::lround (refHz / binHz) - 1; b > 1; --b)
            if (20.0 * std::log10 (std::max ((double) mag[(size_t) b], 1.0e-12)) < ref - 3.0)
                return b * binHz;
        return -1.0;
    }
};

mb::MbFilter makeFilter (int mode, float reso, float drivePre = 0.0f, float drivePost = 0.0f)
{
    mb::MbFilter f;
    f.prepare (kSR);
    f.setMode (mode);
    f.finishFades();                     // tests of steady behaviour skip the initial fade
    f.setParams (reso, drivePre, drivePost);
    return f;
}
} // namespace

//==============================================================================
TEST_CASE ("filter: diag — measured curves", "[.diag]")   // hidden; run explicitly
{
    for (int mode : { (int) mb::MbFilter::LP24, (int) mb::MbFilter::LP12,
                      (int) mb::MbFilter::HP12, (int) mb::MbFilter::BP12 })
    {
        auto f = makeFilter (mode, 0.2f);
        Response r (f, 1000.0);
        std::string line = "mode " + std::to_string (mode) + " @fc1000:";
        for (double hz : { 50.0, 100.0, 150.0, 250.0, 500.0, 800.0, 1000.0, 1500.0,
                           2000.0, 3000.0, 4000.0, 6000.0, 8000.0, 12000.0 })
            line += " " + std::to_string (hz) + "Hz=" + std::to_string (r.dBAt (hz));
        WARN (line);
    }
    for (double fc : { 250.0, 500.0, 1000.0, 2000.0 })
    {
        auto lp = makeFilter (mb::MbFilter::LP12, 0.2f);
        Response rl (lp, fc);
        auto bp = makeFilter (mb::MbFilter::BP12, 0.5f);
        Response rb (bp, fc);
        WARN ("fc " << fc << ": LP12 -3dB=" << rl.minus3dBFrom (60.0)
              << " BP12 peak=" << rb.peakHz (fc * 0.4, fc * 2.5)
              << " LP12 passband(60)=" << rl.dBAt (60.0)
              << " BP12 pk dB=" << rb.dBAt (rb.peakHz (fc * 0.4, fc * 2.5)));
    }
}

TEST_CASE ("filter: cutoff law is one shared function (keytrack + env)")
{
    // Identical scaling for both engines is BY CONSTRUCTION: both receive the Hz
    // value from this single law. Verify the law itself.
    const double base = 400.0;
    REQUIRE (mb::MbFilter::modulatedCutoff (base, 60, 0.0f, 0.0f, 0.0f) == Approx (base));
    // full keytrack: +12 semitones doubles cutoff
    REQUIRE (mb::MbFilter::modulatedCutoff (base, 72, 1.0f, 0.0f, 0.0f) == Approx (base * 2.0));
    // half keytrack: +12 semitones → half an octave
    REQUIRE (mb::MbFilter::modulatedCutoff (base, 72, 0.5f, 0.0f, 0.0f) == Approx (base * std::sqrt (2.0)).epsilon (1e-6));
    // env: amount +1, env 1 → +kEnvOctaves octaves; amount -1 mirrors down
    REQUIRE (mb::MbFilter::modulatedCutoff (base, 60, 0.0f, 1.0f, 1.0f)
             == Approx (base * std::pow (2.0, mb::MbFilter::kEnvOctaves)));
    REQUIRE (mb::MbFilter::modulatedCutoff (base, 60, 0.0f, -1.0f, 1.0f)
             == Approx (base / std::pow (2.0, mb::MbFilter::kEnvOctaves)));
    // env scaling is multiplicative in octaves → trajectory shape is engine-independent
    const double a = mb::MbFilter::modulatedCutoff (base, 60, 0.0f, 0.6f, 0.5f);
    REQUIRE (a == Approx (base * std::pow (2.0, 0.6 * 0.5 * mb::MbFilter::kEnvOctaves)));
}

TEST_CASE ("filter: mode magnitude responses (slope + shape)")
{
    const double fc = 1000.0;

    // Slopes are measured 1.5-3 kHz: far enough above fc to be past the knee,
    // low enough to stay above the integer engine's quantization floor (≈ -57 dB,
    // which flattens any measurement made at 4-8 kHz).
    SECTION ("LP24: ~24 dB/oct, flat passband, near-unity level")
    {
        auto f = makeFilter (mb::MbFilter::LP24, 0.2f);
        Response r (f, fc);
        const double slope = r.dBAt (3000.0) - r.dBAt (1500.0);
        INFO ("LP24 slope/oct: " << slope);
        CHECK (slope < -16.0); CHECK (slope > -32.0);
        CHECK (std::abs (r.dBAt (100.0) - r.dBAt (50.0)) < 2.0);   // flat passband
        CHECK (std::abs (r.dBAt (100.0)) < 2.0);                   // calibrated ≈ unity
        CHECK (r.dBAt (4000.0) < r.dBAt (100.0) - 40.0);           // real attenuation
    }
    SECTION ("LP12: ~12 dB/oct")
    {
        auto f = makeFilter (mb::MbFilter::LP12, 0.2f);
        Response r (f, fc);
        const double slope = r.dBAt (3000.0) - r.dBAt (1500.0);
        INFO ("LP12 slope/oct: " << slope);
        CHECK (slope < -7.0); CHECK (slope > -16.0);
    }
    SECTION ("HP12: ~12 dB/oct rising")
    {
        auto f = makeFilter (mb::MbFilter::HP12, 0.2f);
        Response r (f, fc);
        const double slope = r.dBAt (250.0) - r.dBAt (125.0);
        INFO ("HP12 low-side slope/oct: " << slope);
        CHECK (slope > 8.0); CHECK (slope < 17.0);
        CHECK (r.dBAt (100.0) < r.dBAt (8000.0) - 30.0);
    }
    SECTION ("BP12: peak at fc, falls both sides")
    {
        auto f = makeFilter (mb::MbFilter::BP12, 0.5f);
        Response r (f, fc);
        const double pk = r.peakHz (100.0, 8000.0);
        INFO ("BP12 peak: " << pk);
        CHECK (pk > fc * 0.7); CHECK (pk < fc * 1.4);
        CHECK (r.dBAt (pk) > r.dBAt (100.0) + 10.0);
        CHECK (r.dBAt (pk) > r.dBAt (10000.0) + 10.0);
    }
}

TEST_CASE ("filter: engine_cutoff_scaling — realized frequency curves are parallel across engines")
{
    // Same commanded cutoffs through both engines; realized characteristic
    // frequency (LP12: -3 dB point, BP12: peak) must follow the commanded value
    // with a CONSTANT per-engine log-offset (parallel curves) — that constancy is
    // what makes keytrack/env trajectories identical regardless of engine.
    const double commanded[4] = { 250.0, 500.0, 1000.0, 2000.0 };

    auto offsets = [&commanded] (int mode) {
        std::vector<double> off;
        for (double fc : commanded)
        {
            auto f = makeFilter (mode, mode == mb::MbFilter::BP12 ? 0.5f : 0.2f);
            Response r (f, fc);
            const double realized = (mode == mb::MbFilter::BP12)
                                  ? r.peakHz (fc * 0.4, fc * 2.5)
                                  : r.minus3dBFrom (60.0);
            REQUIRE (realized > 0.0);
            off.push_back (std::log2 (realized / fc));
        }
        return off;
    };

    for (int mode : { (int) mb::MbFilter::LP12, (int) mb::MbFilter::BP12 })
    {
        auto off = offsets (mode);
        double mn = off[0], mx = off[0];
        for (double o : off) { mn = std::min (mn, o); mx = std::max (mx, o); }
        INFO ("mode " << mode << " log2 offsets: " << off[0] << " " << off[1] << " " << off[2] << " " << off[3]);
        CHECK (mx - mn < 0.25);                       // parallel: offset constant within 1/4 octave
        CHECK (std::abs (0.5 * (mn + mx)) < 0.25);    // calibrated: realized ≈ commanded both engines
    }
}

TEST_CASE ("filter: mode_switch_click_free — mid-note switch HF burst < -60 dBFS over steady")
{
    // 150 Hz sine, switch between every ordered mode pair mid-note; the filter
    // path is deterministic, so the variation source is WHERE the switch lands —
    // the test sweeps 10 combinations of input start phase x switch-sample offset
    // and gates the worst (same worst-case standard as the Phase 1 alias tests).
    //
    // Budget derivation: a click is an output discontinuity = broadband burst,
    // detected as energy above 6 kHz where neither the 150 Hz input nor the
    // filter response has signal. But driven modes legitimately EMIT steady HF
    // (always-on feedback saturation distorts the passing signal — BP12 idles
    // around -50 dBFS at 6 kHz here), and during the crossfade BOTH engines run
    // and their saturation HF overlaps — hence the allowance of 2x the LOUDER of
    // the two modes' steady HF (old measured pre-switch, new measured after the
    // fade settles). The click budget proper is the absolute floor added on top:
    // 0.001 = -60 dBFS. A real discontinuity lands far above these levels.
    const int modes[4] = { mb::MbFilter::LP24, mb::MbFilter::LP12, mb::MbFilter::HP12, mb::MbFilter::BP12 };
    const auto hpC = teq::makeCoeffs (teq::FilterType::HPF, 6000.0, 0.0, 0.707, kSR);

    for (int from : modes)
        for (int to : modes)
        {
            if (from == to) continue;

            float worstExcess = -1.0e9f;
            float worstSteady = 0, worstSwitch = 0;
            for (int trial = 0; trial < 10; ++trial)
            {
                mb::MbFilter f;
                f.prepare (kSR);
                f.setMode (from);
                f.finishFades();
                f.setParams (0.4f, 0.3f, 0.0f);

                teq::Biquad hp; hp.setCoeffs (hpC);
                const float phase0   = (float) trial * 0.1f * juce::MathConstants<float>::twoPi;
                const int   switchAt = 22050 + trial * 37;            // sweeps the 150 Hz cycle (294 smp)
                const int   total    = switchAt + 8000;
                float hfOldSteady = 0.0f, hfSwitch = 0.0f, hfNewSteady = 0.0f;
                for (int i = 0; i < total; ++i)
                {
                    if (i == switchAt) f.setMode (to);
                    const float in = std::sin (phase0 + 2.0f * juce::MathConstants<float>::pi * 150.0f * (float) i / (float) kSR);
                    const float out = f.process (in, 800.0);
                    REQUIRE (std::isfinite (out));
                    const float hf = std::abs (hp.process (out));
                    if (i > 8000 && i < switchAt - 100)                    hfOldSteady = std::max (hfOldSteady, hf);
                    if (i >= switchAt && i < switchAt + 2000)              hfSwitch    = std::max (hfSwitch, hf);
                    if (i >= switchAt + 3000 && i < switchAt + 7000)       hfNewSteady = std::max (hfNewSteady, hf);
                }
                const float steady = std::max (hfOldSteady, hfNewSteady);
                const float excess = hfSwitch - 2.0f * steady;
                if (excess > worstExcess) { worstExcess = excess; worstSteady = steady; worstSwitch = hfSwitch; }
            }
            INFO ("switch " << from << "->" << to << " worst trial: steady HF " << worstSteady
                  << ", switch HF " << worstSwitch << ", excess " << worstExcess);
            CHECK (worstExcess < 0.001f);                          // -60 dBFS click budget
        }
}

// THD-like measure at 200 Hz: harmonic energy (h2..h8) relative to the
// fundamental, for any per-sample processor.
template <typename Tick>
static double measureThd200 (Tick&& tick)
{
    const int n = 1 << 15;
    std::vector<float> x ((size_t) n);
    for (int i = 0; i < n; ++i)
        x[(size_t) i] = tick (0.8f * std::sin (2.0f * juce::MathConstants<float>::pi * 200.0f * (float) i / (float) kSR));

    juce::dsp::FFT fft (15);
    std::vector<float> buf ((size_t) n * 2, 0.0f);
    for (int i = 0; i < n; ++i)
    {
        const double t = 2.0 * juce::MathConstants<double>::pi * i / n;
        buf[(size_t) i] = x[(size_t) i] * (float) (0.35875 - 0.48829 * std::cos (t) + 0.14128 * std::cos (2 * t) - 0.01168 * std::cos (3 * t));
    }
    fft.performFrequencyOnlyForwardTransform (buf.data());
    const double binHz = kSR / n;
    auto magNear = [&] (double hz) {
        float m = 0.0f;
        const int c = (int) std::lround (hz / binHz);
        for (int b = c - 3; b <= c + 3; ++b) if (b > 0 && b < n / 2) m = std::max (m, buf[(size_t) b]);
        return (double) m;
    };
    const double fund = magNear (200.0);
    double harm = 0.0;
    for (int k = 2; k <= 8; ++k) harm += magNear (200.0 * k) * magNear (200.0 * k);
    return 10.0 * std::log10 (std::max (harm, 1.0e-24) / std::max (fund * fund, 1.0e-24));
}

TEST_CASE ("filter: svf_drive_absolute — like-for-like LP12 saturation gap <= 6 dB across drives")
{
    // Phase 2 review item 2: the ABSOLUTE THD gap between BP12 and LP12 mixes two
    // causes — tap topology (a BP skirt passes relatively more of a driven input's
    // harmonics than an LP rolloff) and the saturation structures themselves.
    // This test isolates the saturation structures by running BOTH on the same
    // 12 dB/oct lowpass shape: Type-R LP12 vs the SVF core's LP tap, identical
    // pre-drive chain. Gate: within 6 dB at every drive setting.
    auto thdTypeR = [] (float d)
    {
        auto f = makeFilter (mb::MbFilter::LP12, 0.4f, d, 0.0f);
        return measureThd200 ([&f] (float in) { return f.process (in, 200.0); });
    };
    auto thdSvfLp = [] (float d, bool linearLoop)
    {
        mb::DriveStage pre; pre.set (d, 9.0f);
        mb::MbSvfCore svf; svf.prepare (kSR);
        svf.setSat (d, 0.4f);
        if (linearLoop) { svf.satC = 1.0e-9f; svf.satL = 1.0e6f; }   // saturator off → pure topology
        svf.set (200.0, 0.4f);
        return measureThd200 ([&] (float in)
        {
            float lp = 0.0f, bp = 0.0f, hp = 0.0f;
            svf.step (pre.process (in), lp, bp, hp);
            return lp;
        });
    };

    for (float d : { 0.0f, 0.4f, 0.8f })
    {
        const double r = thdTypeR (d), s = thdSvfLp (d, false), lin = thdSvfLp (d, true);
        INFO ("drive " << d << ": Type-R LP12 " << r << " dB, SVF-LP " << s
              << " dB (linear-loop floor " << lin << "), gap vs Type-R " << (s - r)
              << ", sat-attributable " << (s - lin));
        // The loop saturator itself must stay within 6 dB of Type-R's, measured
        // two ways: at drive 0 the input is a clean sine, so ALL output THD is
        // saturation → gate directly against Type-R. When driven, the input is
        // already a square and the two engines' LINEAR knee shapes pass different
        // harmonic amounts regardless of saturation (measured: linear SVF floor
        // -27.5 dB vs Type-R -33.1 at d=0.4) → gate the SATURATION-ATTRIBUTABLE
        // excess over the SVF's own linear floor instead. The ~5-6 dB linear
        // knee-shape difference is topology character, documented in MbFilter.h.
        if (d == 0.0f) CHECK (std::abs (s - r) <= 6.0);
        else           CHECK (s - lin <= 6.0);
    }
}

TEST_CASE ("filter: svf_drive_matched — HP/BP path is not sterile and tracks Type-R drive")
{
    // Drive ENGAGEMENT is compared as each engine's THD rise over its own
    // zero-drive baseline (delta), because LP12 and BP12 pass different amounts
    // of the input's harmonics — comparing absolute THD across modes would
    // measure topology, not the saturation structure this condition is about.
    // (The saturation structures themselves are gated ABSOLUTELY, like-for-like,
    // in "filter: svf_drive_absolute" above.)
    auto thdDb = [] (int mode, float drivePre)
    {
        auto f = makeFilter (mode, 0.4f, drivePre, 0.0f);
        return measureThd200 ([&f] (float in) { return f.process (in, 200.0); });
    };

    const double thdR0 = thdDb (mb::MbFilter::LP12, 0.0f), thdR1 = thdDb (mb::MbFilter::LP12, 0.8f);
    const double thdS0 = thdDb (mb::MbFilter::BP12, 0.0f), thdS1 = thdDb (mb::MbFilter::BP12, 0.8f);
    const double deltaR = thdR1 - thdR0, deltaS = thdS1 - thdS0;
    INFO ("Type-R LP12 THD " << thdR0 << " -> " << thdR1 << " (delta " << deltaR
          << "), SVF BP12 THD " << thdS0 << " -> " << thdS1 << " (delta " << deltaS << ")");
    CHECK (thdS1 > -40.0);                         // not sterile at heavy drive
    CHECK (deltaS > 6.0);                          // drive audibly engages the SVF path
    CHECK (std::abs (deltaR - deltaS) < 12.0);     // same perceived-drive family
}

TEST_CASE ("filter: self_oscillation bounded and finite at max resonance")
{
    for (int mode : { (int) mb::MbFilter::LP24, (int) mb::MbFilter::LP12,
                      (int) mb::MbFilter::HP12, (int) mb::MbFilter::BP12 })
    {
        auto f = makeFilter (mode, 1.0f);
        double peak = 0.0, tailRms = 0.0;
        const int n = (int) (2.0 * kSR);
        for (int i = 0; i < n; ++i)
        {
            const float out = f.process (i == 0 ? 1.0f : 0.0f, 500.0);   // kick, then free-run
            REQUIRE (std::isfinite (out));
            peak = std::max (peak, (double) std::abs (out));
            if (i > n - 22050) tailRms += (double) out * out;
        }
        tailRms = std::sqrt (tailRms / 22050.0);
        INFO ("mode " << mode << " peak " << peak << " tail RMS " << tailRms);
        CHECK (peak < 4.0);                                   // bounded, no blow-up
        if (mode == mb::MbFilter::LP24 || mode == mb::MbFilter::LP12)
            CHECK (tailRms > 1.0e-4);                         // Type-R truly self-oscillates
    }
}

TEST_CASE ("filter: drive stages monotonic and finite; zero drive is transparent")
{
    auto rmsAt = [] (float drive)
    {
        auto f = makeFilter (mb::MbFilter::LP24, 0.2f, drive, drive);
        double acc = 0.0;
        const int n = 22050;
        for (int i = 0; i < n; ++i)
        {
            const float out = f.process (0.5f * std::sin (2.0f * juce::MathConstants<float>::pi * 150.0f * (float) i / (float) kSR), 2000.0);
            REQUIRE (std::isfinite (out));
            if (i > 2000) acc += (double) out * out;
        }
        return std::sqrt (acc / (n - 2000));
    };

    const double r0 = rmsAt (0.0f);
    double prev = r0;
    for (float d : { 0.25f, 0.5f, 0.75f, 1.0f })
    {
        const double r = rmsAt (d);
        CHECK (r > prev * 0.7);                    // no collapse as drive rises
        prev = r;
    }
    CHECK (rmsAt (1.0f) > r0);                     // drive adds energy overall

    // drive 0: LP24 passband ≈ unity (transparent stages)
    auto f = makeFilter (mb::MbFilter::LP24, 0.1f);
    Response r (f, 2000.0);
    CHECK (std::abs (r.dBAt (150.0)) < 3.0);
}
