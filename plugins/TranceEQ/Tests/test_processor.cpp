#include <catch2/catch_test_macros.hpp>
#include <catch2/matchers/catch_matchers_floating_point.hpp>
#include "TestUtils.h"
#include "../Source/PluginProcessor.h"
#include "../Source/ParameterIDs.hpp"

using Catch::Matchers::WithinAbs;

// Set an APVTS parameter from its *scaled* value (Hz / dB / choice index).
static void setScaled (TranceEQAudioProcessor& p, const juce::String& id, float scaled)
{
    auto* prm = p.apvts.getParameter (id);
    REQUIRE (prm != nullptr);
    prm->setValueNotifyingHost (prm->convertTo0to1 (scaled));
}
static void runBlock (TranceEQAudioProcessor& p, int numCh, int n)
{
    juce::AudioBuffer<float> b (numCh, n); b.clear();
    juce::MidiBuffer m; p.processBlock (b, m);
}

TEST_CASE ("Latency is reported and stays constant when bypass is toggled", "[processor][latency]")
{
    juce::ScopedJuceInitialiser_GUI init;
    TranceEQAudioProcessor p;
    p.setPlayConfigDetails (2, 2, 48000, 512);
    p.prepareToPlay (48000, 512);

    setScaled (p, ParameterIDs::os, 0.0f);                 // Off
    REQUIRE (p.getLatencySamples() == 0);

    setScaled (p, ParameterIDs::os, 1.0f);                 // 2x
    runBlock (p, 2, 512);                                   // OS change applies on the next block
    const int lat2 = p.getLatencySamples();
    REQUIRE (lat2 > 0);

    setScaled (p, ParameterIDs::bypass, 1.0f);             // bypass must NOT change reported latency
    runBlock (p, 2, 512);
    REQUIRE (p.getLatencySamples() == lat2);

    setScaled (p, ParameterIDs::os, 2.0f);                 // 4x
    runBlock (p, 2, 512);
    REQUIRE (p.getLatencySamples() > lat2);
}

TEST_CASE ("Processes mono and stereo (incl. oversampling) without NaNs", "[processor][channels]")
{
    juce::ScopedJuceInitialiser_GUI init;
    for (int nch : { 1, 2 })
    {
        TranceEQAudioProcessor p;
        p.setPlayConfigDetails (nch, nch, 48000, 512);
        p.prepareToPlay (48000, 512);
        setScaled (p, ParameterIDs::os, 2.0f);             // 4x — also exercises the mono-host oversampler path

        juce::AudioBuffer<float> b (nch, 512);
        for (int ch = 0; ch < nch; ++ch)
            for (int i = 0; i < 512; ++i)
                b.setSample (ch, i, 0.5f * std::sin (2.0 * tt::kPi * 440.0 * i / 48000.0));
        juce::MidiBuffer m;
        p.processBlock (b, m);

        for (int ch = 0; ch < nch; ++ch)
            for (int i = 0; i < 512; ++i)
                REQUIRE (std::isfinite (b.getSample (ch, i)));
    }
}

TEST_CASE ("Oversampling preserves the in-band frequency response", "[processor][oversampling]")
{
    juce::ScopedJuceInitialiser_GUI init;
    const double fs = 48000.0;
    const int    order = 15, N = 1 << order;

    auto measure = [&] (int osIndex)
    {
        TranceEQAudioProcessor p;
        p.setPlayConfigDetails (1, 1, fs, 512);
        p.prepareToPlay (fs, 512);
        // Controlled curve entirely below Nyquist/2: a single 1 kHz peak. This isolates the
        // oversampler's own transparency from the (intended) cramping change of a near-Nyquist
        // shelf — the default Lead curve's 12 kHz shelf legitimately differs across OS factors.
        for (int b = 0; b < ParameterIDs::kNumBands; ++b) setScaled (p, ParameterIDs::bandOn (b), 0.0f);
        setScaled (p, ParameterIDs::bandOn   (2), 1.0f);
        setScaled (p, ParameterIDs::bandType (2), 2.0f);   // Peak
        setScaled (p, ParameterIDs::bandFreq (2), 1000.0f);
        setScaled (p, ParameterIDs::bandGain (2), 6.0f);
        setScaled (p, ParameterIDs::bandQ    (2), 1.0f);
        setScaled (p, ParameterIDs::bandTrack(2), 0.0f);
        setScaled (p, ParameterIDs::os, (float) osIndex);
        runBlock (p, 1, 512);                              // warm-up: engage OS / settle latency
        return tt::magnitudeDbFromIR (tt::renderIR (p, N, 1), order);
    };

    const auto m0 = measure (0), m1 = measure (1), m2 = measure (2);

    double devLow = 0.0, devHalf = 0.0;                    // <6 kHz (tight) and <Nyquist/2 (loose)
    for (int k = 1; k < N / 2; ++k)
    {
        const double f = (double) k * fs / N;
        if (f < 50.0) continue;
        const double d = juce::jmax (std::abs (m1[(size_t) k] - m0[(size_t) k]),
                                     std::abs (m2[(size_t) k] - m0[(size_t) k]));
        if (f < 6000.0)      devLow  = juce::jmax (devLow,  d);
        if (f < fs / 4.0)    devHalf = juce::jmax (devHalf, d);
    }
    INFO ("max deviation <6kHz=" << devLow << " dB, <Nyquist/2=" << devHalf << " dB");
    REQUIRE (devLow  < 0.1);
    REQUIRE (devHalf < 1.0);
}

TEST_CASE ("Bypass is transparent: nulls against the latency-delayed dry input", "[processor][bypass]")
{
    juce::ScopedJuceInitialiser_GUI init;
    const double fs = 48000.0; const int block = 256;

    for (int osIndex : { 0, 1, 2 })
    {
        TranceEQAudioProcessor p;
        p.setPlayConfigDetails (2, 2, fs, block);
        p.prepareToPlay (fs, block);
        setScaled (p, ParameterIDs::os, (float) osIndex);
        setScaled (p, ParameterIDs::bypass, 1.0f);

        juce::Random rng (1234);
        const int total = block * 64;
        std::vector<float> in ((size_t) total);
        for (auto& s : in) s = rng.nextFloat() * 2.0f - 1.0f;

        std::vector<float> out; out.reserve ((size_t) total);
        juce::AudioBuffer<float> buf (2, block);
        juce::MidiBuffer midi;
        for (int pos = 0; pos < total; pos += block)
        {
            const int n = juce::jmin (block, total - pos);
            buf.setSize (2, n, false, false, true);
            for (int i = 0; i < n; ++i) { buf.setSample (0, i, in[(size_t) (pos + i)]); buf.setSample (1, i, in[(size_t) (pos + i)]); }
            midi.clear();
            p.processBlock (buf, midi);
            for (int i = 0; i < n; ++i) out.push_back (buf.getSample (0, i));
        }

        const int lat = p.getLatencySamples();
        const int start = block * 8 + lat + 64;            // skip latency priming + crossfade settle
        double maxDiff = 0.0;
        for (int t = start; t < total; ++t)
            maxDiff = juce::jmax (maxDiff, (double) std::abs (out[(size_t) t] - in[(size_t) (t - lat)]));
        const double nullDb = 20.0 * std::log10 (maxDiff + 1e-30);
        INFO ("os=" << osIndex << " latency=" << lat << " null=" << nullDb << " dBFS");
        REQUIRE (nullDb < -120.0);
    }
}
