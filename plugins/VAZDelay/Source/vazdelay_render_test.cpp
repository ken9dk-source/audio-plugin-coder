// VazDelayRenderTest — end-to-end audio render of the REAL VAZDelayAudioProcessor (headless, no WebView).
//
// Why this exists: the oracle proves VazDelayEngine == the decompiled reference transcription (bit-exact),
// but it drives the ENGINE directly. It cannot see a bug in the PROCESSOR's param->engine mapping, nor prove
// that audible repeats actually leave processBlock. This test instantiates the shipped processor, keeps its
// C++ parameter defaults (the exact state a user hears), feeds an impulse, and asserts a wet delay tail exists.
// Same failure class as pulse-silence / portamento: "oracle green, plugin silent".  Run: VazDelayRenderTest.exe
#include "PluginProcessor.h"
#include "ParameterIDs.hpp"
#include <juce_audio_processors/juce_audio_processors.h>
#include <iostream>
#include <vector>
#include <cmath>
#include <string>

static int fails = 0;
static void check (bool ok, const std::string& m)
{ std::cout << (ok ? "  [PASS] " : "  [FAIL] ") << m << "\n"; if (! ok) ++fails; }

static void setP (VAZDelayAudioProcessor& p, const char* id, float v)
{
    if (auto* prm = p.apvts.getParameter (id))
        prm->setValueNotifyingHost (prm->getNormalisableRange().convertTo0to1 (v));
}

// Render a mono input (duplicated to stereo) through the processor; return per-sample stereo-mean |out|.
static std::vector<float> render (VAZDelayAudioProcessor& p, const std::vector<float>& inMono, int block = 512)
{
    const int n = (int) inMono.size();
    const int padded = ((n + block - 1) / block) * block;
    std::vector<float> in = inMono; in.resize ((size_t) padded, 0.0f);
    std::vector<float> outAbs ((size_t) padded, 0.0f);
    juce::AudioBuffer<float> buf (2, block);
    juce::MidiBuffer midi;
    for (int i = 0; i < padded; i += block)
    {
        for (int s = 0; s < block; ++s) { buf.setSample (0, s, in[(size_t) i + s]); buf.setSample (1, s, in[(size_t) i + s]); }
        midi.clear();
        p.processBlock (buf, midi);
        for (int s = 0; s < block; ++s)
            outAbs[(size_t) i + s] = 0.5f * (std::abs (buf.getSample (0, s)) + std::abs (buf.getSample (1, s)));
    }
    outAbs.resize ((size_t) n);
    return outAbs;
}

static float peakIn (const std::vector<float>& v, int a, int b, int* argmax = nullptr)
{
    float pk = 0.0f; int at = a;
    for (int i = a; i < b && i < (int) v.size(); ++i) if (v[(size_t) i] > pk) { pk = v[(size_t) i]; at = i; }
    if (argmax) *argmax = at;
    return pk;
}

int main()
{
    juce::ScopedJuceInitialiser_GUI init;
    const double sr = 44100.0;
    const int    block = 512;
    std::cout << "=== VazDelayRenderTest — real processor render, impulse -> delay tail ===\n";

    // ---- TEST 1: DEFAULT state must produce an audible delay tail (the decisive check) -----------------
    {
        VAZDelayAudioProcessor p;
        p.setPlayConfigDetails (2, 2, sr, block);
        p.prepareToPlay (sr, block);
        std::vector<float> imp (2 * (int) sr, 0.0f);   // 2 s
        imp[0] = 1.0f;                                 // impulse at t=0 (dry passes it straight through)
        auto out = render (p, imp, block);

        const float dry     = peakIn (out, 0, 64);                 // dry impulse (dry default 1.0)
        int tailAt = 0;
        const float tailPk  = peakIn (out, (int) (0.20 * sr), (int) (1.95 * sr), &tailAt);  // any wet repeat
        std::cout << "  default: dry peak=" << dry << "  tail peak=" << tailPk
                  << " @ " << (tailAt / sr) << " s\n";
        check (dry     > 0.3f,  "dry impulse passes through (dry=1.0 default)");
        check (tailPk  > 0.02f, "DEFAULT params produce an audible delay tail (NOT dry/bypassed)");
    }

    // ---- TEST 2: wet=0 must silence the tail (sanity: wet actually gates the repeats) ------------------
    {
        VAZDelayAudioProcessor p;
        p.setPlayConfigDetails (2, 2, sr, block);
        p.prepareToPlay (sr, block);
        setP (p, ParameterIDs::wet_l, 0.0f);
        setP (p, ParameterIDs::wet_r, 0.0f);
        std::vector<float> imp (2 * (int) sr, 0.0f); imp[0] = 1.0f;
        auto out = render (p, imp, block);
        const float tailPk = peakIn (out, (int) (0.20 * sr), (int) (1.95 * sr));
        std::cout << "  wet=0: tail peak=" << tailPk << "\n";
        check (tailPk < 1.0e-3f, "wet=0 -> no tail (wet gates the repeat)");
    }

    // ---- TEST 3: delay TIME lands where the map says (settled, fixed delay) ----------------------------
    {
        VAZDelayAudioProcessor p;
        p.setPlayConfigDetails (2, 2, sr, block);
        p.prepareToPlay (sr, block);
        const float dp = 0.25f;                        // free mode (sync off default)
        setP (p, ParameterIDs::delay_l, dp);
        setP (p, ParameterIDs::delay_r, dp);
        setP (p, ParameterIDs::fb_l, 0.0f);            // single clean repeat (no feedback tail smearing the peak)
        setP (p, ParameterIDs::fb_r, 0.0f);
        setP (p, ParameterIDs::tone_l, 1.0f);          // bright -> minimal damping, sharp repeat
        setP (p, ParameterIDs::tone_r, 1.0f);

        const int pre = (int) (0.5 * sr);              // pre-roll silence lets the 20 ms delay-time glide settle
        std::vector<float> sig (pre + 2 * (int) sr, 0.0f);
        sig[(size_t) pre] = 1.0f;                      // impulse after the pre-roll
        auto out = render (p, sig, block);

        // expected first repeat = VAZ integer map: samplesPerMs=44, delay = round(ms*44) + (44>>4)
        const double ms  = 1.0 + (double) dp * 1999.0;                 // 500.75 ms
        const int    exp = (int) std::llround (ms * 44.0) + (44 >> 4); // ~22035 samples
        int at = 0;
        const float pk = peakIn (out, pre + 200, pre + exp + 4000, &at);
        const int   err = at - (pre + exp);
        std::cout << "  fixed delay: expect repeat @ +" << exp << " smp (" << ms << " ms), got @ +"
                  << (at - pre) << " (err " << err << " smp, peak " << pk << ")\n";
        check (pk > 0.05f, "fixed-delay repeat is present");
        check (std::abs (err) < 100, "repeat lands within 100 samples (~2 ms) of the mapped delay time");
    }

    // ---- PROBE (diagnostic, non-failing): how audible is the echo on realistic material vs an impulse? ----
    // An impulse is the worst case for a lowpass. Characterise a sustained tone + broadband noise, at the
    // BRIGHTEST tone the LUT offers, to tell "dead" from merely "dark".
    auto tailEnergy = [] (const std::vector<float>& v, int a, int b) {
        double e = 0.0; for (int i = a; i < b && i < (int) v.size(); ++i) e += (double) v[(size_t) i] * v[(size_t) i];
        return e; };
    auto probe = [&] (const char* name, bool noise, float tone) {
        VAZDelayAudioProcessor p;
        p.setPlayConfigDetails (2, 2, sr, block);
        p.prepareToPlay (sr, block);
        setP (p, ParameterIDs::delay_l, 0.25f);  setP (p, ParameterIDs::delay_r, 0.25f);   // ~500 ms
        setP (p, ParameterIDs::fb_l, 0.0f);       setP (p, ParameterIDs::fb_r, 0.0f);       // single echo
        setP (p, ParameterIDs::tone_l, tone);     setP (p, ParameterIDs::tone_r, tone);
        setP (p, ParameterIDs::dry_l, 0.0f);      setP (p, ParameterIDs::dry_r, 0.0f);      // WET ONLY: isolate the echo
        setP (p, ParameterIDs::wet_l, 1.0f);      setP (p, ParameterIDs::wet_r, 1.0f);
        const int dur = (int) (0.30 * sr);                       // 300 ms source burst
        std::vector<float> sig (dur + (int) (1.5 * sr), 0.0f);
        uint32_t rng = 22222;
        for (int i = 0; i < dur; ++i)
            sig[(size_t) i] = noise ? ((rng = rng * 1664525u + 1013904223u) >> 9) * (2.0f / 8388608.0f) - 1.0f
                                    : 0.7f * std::sin (2.0 * 3.14159265 * 150.0 * i / sr);
        double srcE = tailEnergy (sig, 0, dur);
        auto out = render (p, sig, block);
        const int d = (int) std::llround ((1.0 + 0.25 * 1999.0) * 44.0) + 2;   // ~22035
        double echoE = tailEnergy (out, d, d + dur);              // energy in the echo window
        const double ratio = echoE / (srcE + 1e-30);
        std::cout << "  PROBE " << name << " (tone=" << tone << "): echo/src energy = "
                  << ratio << "  (" << 10.0 * std::log10 (ratio + 1e-30) << " dB)\n";
    };
    std::cout << "--- diagnostic probes (wet-only, single echo) ---\n";
    probe ("150Hz sine   ", false, 1.0f);   // brightest
    probe ("150Hz sine   ", false, 0.6f);   // default tone
    probe ("broadband    ", true,  1.0f);   // brightest
    probe ("broadband    ", true,  0.6f);   // default tone

    std::cout << (fails == 0 ? "\nALL PASS\n" : "\n" + std::to_string (fails) + " FAILED\n");
    return fails ? 1 : 0;
}
