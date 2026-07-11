// VazDSPAudit — failing audio test suite for the VAZClone synth engine.
// Instantiates the real processor headlessly, renders targeted scenarios and runs DSP detectors
// (click / aliasing / voice-drop / envelope-glitch / DC / polyphony gain). Prints PASS/FAIL and
// returns the number of failures. Several tests are EXPECTED to fail — they pin the audited bugs.
//   build target: VazDSPAudit   (see plugins/VAZClone/CMakeLists.txt)
#include "PluginProcessor.h"
#include <juce_dsp/juce_dsp.h>
#include <iostream>
#include <vector>
#include <cmath>

using juce::MidiMessage;

static int g_fail = 0, g_total = 0;
static void check(const char* name, bool ok, const std::string& detail)
{
    ++g_total; if (!ok) ++g_fail;
    std::cout << (ok ? "[PASS] " : "[FAIL] ") << name << "   " << detail << "\n";
}

static void setP(VAZCloneAudioProcessor& p, const char* id, float v)
{
    if (auto* q = p.apvts.getParameter(id)) q->setValueNotifyingHost(juce::jlimit(0.0f, 1.0f, v));
}

// Render a MIDI sequence (vector of {sampleTime, message}) for `totalSec`. Resets the processor first.
static juce::AudioBuffer<float> render(VAZCloneAudioProcessor& proc, double sr,
                                       const std::vector<std::pair<int, MidiMessage>>& events,
                                       double totalSec, int block = 64)
{
    proc.setPlayConfigDetails(0, 2, sr, block);
    proc.prepareToPlay(sr, block);
    const int total = (int)(totalSec * sr);
    juce::AudioBuffer<float> out(2, total); out.clear();
    juce::AudioBuffer<float> buf(2, block);
    for (int pos = 0; pos < total; pos += block)
    {
        const int n = juce::jmin(block, total - pos);
        buf.setSize(2, n, false, false, true); buf.clear();
        juce::MidiBuffer midi;
        for (auto& e : events)
            if (e.first >= pos && e.first < pos + n) midi.addEvent(e.second, e.first - pos);
        proc.processBlock(buf, midi);
        for (int ch = 0; ch < 2; ++ch) out.copyFrom(ch, pos, buf, ch, 0, n);
    }
    return out;
}

static double maxStep(const float* x, int a, int b)
{ double m = 0; for (int i = juce::jmax(1, a); i < b; ++i) m = juce::jmax(m, (double)std::abs(x[i] - x[i - 1])); return m; }
static double rms(const float* x, int a, int b)
{ double s = 0; for (int i = a; i < b; ++i) s += (double)x[i] * x[i]; return std::sqrt(s / juce::jmax(1, b - a)); }
static double dc(const float* x, int a, int b)
{ double s = 0; for (int i = a; i < b; ++i) s += x[i]; return s / juce::jmax(1, b - a); }
static double peak(const float* x, int a, int b)
{ double m = 0; for (int i = a; i < b; ++i) m = juce::jmax(m, (double)std::abs(x[i])); return m; }

// Inharmonic (aliasing) energy ratio of a steady tone at fundamental f0.
static double aliasingRatio(const float* x, int start, double sr, double f0)
{
    constexpr int ORDER = 14, N = 1 << ORDER;       // 16384-pt FFT
    juce::dsp::FFT fft(ORDER);
    std::vector<float> buf((size_t)N * 2, 0.0f);
    for (int i = 0; i < N; ++i)
    {
        const double w = 0.5 - 0.5 * std::cos(2.0 * juce::MathConstants<double>::pi * i / (N - 1)); // Hann
        buf[(size_t)i] = x[start + i] * (float)w;
    }
    fft.performRealOnlyForwardTransform(buf.data());
    double harm = 0, total = 0;
    const double binHz = sr / N;
    for (int k = 1; k < N / 2; ++k)
    {
        const double mag = std::hypot(buf[(size_t)k * 2], buf[(size_t)k * 2 + 1]);
        const double e = mag * mag;
        if (k * binHz < 60.0) continue;             // ignore DC/sub region
        total += e;
        const double ratioToF0 = (k * binHz) / f0;
        if (std::abs(ratioToF0 - std::round(ratioToF0)) < 0.05 && std::round(ratioToF0) >= 1.0)
            harm += e;                              // within 5% of a harmonic
    }
    return total > 0 ? (total - harm) / total : 0.0;
}

int main()
{
    juce::ScopedJuceInitialiser_GUI juceInit;
    const double sr = 48000.0;
    std::cout << "=== VAZClone synth-engine DSP audit (sr=" << sr << ") ===\n";

    // ---- 1. zero-attack note-on click (expected OK: VAZ fastest attack ~0.5 ms exp) ----
    {
        VAZCloneAudioProcessor p;
        setP(p, "e1_attack", 0.0f); setP(p, "e1_decay", 0.4f); setP(p, "e1_sustain", 0.8f);
        const int on = (int)(0.10 * sr);
        auto b = render(p, sr, {{on, MidiMessage::noteOn(1, 60, 0.9f)}}, 0.30);
        const float* x = b.getReadPointer(0);
        const double onsetStep = maxStep(x, on, on + (int)(0.003 * sr));
        const double steadyStep = maxStep(x, on + (int)(0.05 * sr), on + (int)(0.12 * sr));
        check("zero_attack_no_click", onsetStep <= steadyStep * 2.5 + 1e-6,
              "onsetStep=" + std::to_string(onsetStep) + " steadyStep=" + std::to_string(steadyStep));
    }

    // ---- 2. voice-steal click (EXPECTED FAIL: stolen voice is hard-cut, amp.reset()) ----
    {
        VAZCloneAudioProcessor p;
        setP(p, "voices", 2.0f / 32.0f);            // limit polyphony to 2 -> 3rd note steals
        setP(p, "e1_attack", 0.2f); setP(p, "e1_sustain", 0.9f); setP(p, "e1_release", 0.4f);
        const int s = (int)(0.05 * sr);
        auto b = render(p, sr, {{s, MidiMessage::noteOn(1, 55, 0.9f)},
                                {2 * s, MidiMessage::noteOn(1, 60, 0.9f)},
                                {3 * s, MidiMessage::noteOn(1, 64, 0.9f)}}, 0.40);   // 3rd steals
        const float* x = b.getReadPointer(0);
        const double stealStep = maxStep(x, 3 * s, 3 * s + (int)(0.002 * sr));
        const double steadyStep = maxStep(x, 4 * s, 4 * s + (int)(0.05 * sr));
        check("voice_steal_no_click", stealStep <= steadyStep * 2.5 + 1e-6,
              "stealStep=" + std::to_string(stealStep) + " steadyStep=" + std::to_string(steadyStep));
    }

    // ---- 3. oscillator aliasing on a high note (expected OK: mip-mapped wavetables) ----
    {
        VAZCloneAudioProcessor p;
        setP(p, "e1_attack", 0.1f); setP(p, "e1_sustain", 1.0f);   // default saw
        const int on = (int)(0.05 * sr);
        auto b = render(p, sr, {{on, MidiMessage::noteOn(1, 103, 1.0f)}}, 0.70);   // ~3.13 kHz
        const double f0 = 440.0 * std::pow(2.0, (103 - 69) / 12.0);
        const double alias = aliasingRatio(b.getReadPointer(0), on + (int)(0.1 * sr), sr, f0);
        check("aliasing_below_5pct", alias < 0.05, "inharmonic energy ratio=" + std::to_string(alias));
    }

    // ---- 4. voice-drop on a held chord (expected OK) ----
    {
        VAZCloneAudioProcessor p;
        setP(p, "e1_attack", 0.05f); setP(p, "e1_sustain", 1.0f);
        const int on = (int)(0.05 * sr);
        auto b = render(p, sr, {{on, MidiMessage::noteOn(1, 48, 0.8f)},
                                {on, MidiMessage::noteOn(1, 55, 0.8f)},
                                {on, MidiMessage::noteOn(1, 60, 0.8f)}}, 0.80);
        const float* x = b.getReadPointer(0);
        const int w = (int)(0.05 * sr); double lo = 1e9, hi = 0;
        for (int t = on + w; t + w < b.getNumSamples(); t += w)
        { double r = rms(x, t, t + w); lo = juce::jmin(lo, r); hi = juce::jmax(hi, r); }
        check("no_voice_drop", hi < lo * 1.6 + 1e-9, "rms hi/lo=" + std::to_string(hi / juce::jmax(1e-9, lo)));
    }

    // ---- 5. DC offset on a narrow-pulse + overdrive patch (EXPECTED FAIL: post-clip DC not removed) ----
    {
        VAZCloneAudioProcessor p;
        setP(p, "o1_wave", 1.0f / 4.0f);            // Pulse
        setP(p, "o1_shape", 0.92f);                 // narrow (asymmetric)
        setP(p, "overdrive", 0.7f);
        setP(p, "e1_attack", 0.05f); setP(p, "e1_sustain", 1.0f); setP(p, "cutoff", 0.95f);
        const int on = (int)(0.05 * sr);
        auto b = render(p, sr, {{on, MidiMessage::noteOn(1, 45, 1.0f)}}, 0.60);
        const float* x = b.getReadPointer(0);
        const double off = dc(x, on + (int)(0.2 * sr), b.getNumSamples());
        const double pk = peak(x, on + (int)(0.2 * sr), b.getNumSamples());
        check("dc_offset_below_1pct", std::abs(off) < 0.01 * juce::jmax(1e-6, pk),
              "DC=" + std::to_string(off) + " (" + std::to_string(100 * std::abs(off) / juce::jmax(1e-6, pk)) + "% of peak)");
    }

    // ---- 5b. DEFAULT-patch Pulse must be AUDIBLE (regression guard for the b/256 duty-map: Modifier=0 default
    //          → pulseWidth(0)=0 → difference-of-saws = 0 = silence, unless VAZ's min-edge clamp is applied) ----
    {
        VAZCloneAudioProcessor p;
        setP(p, "o1_wave", 1.0f / 4.0f);            // select Pulse — leave OSC1 Waveshape at its DEFAULT (0.0 = Modifier 0)
        setP(p, "e1_attack", 0.02f); setP(p, "e1_sustain", 1.0f);   // sustain so we measure the oscillator, not env decay
        const int on = (int)(0.05 * sr);
        auto b = render(p, sr, {{on, MidiMessage::noteOn(1, 60, 1.0f)}}, 0.40);
        const double r = rms(b.getReadPointer(0), on + (int)(0.1 * sr), b.getNumSamples());
        check("pulse_default_audible", r > 1e-3, "RMS=" + std::to_string(r) + " (default Pulse waveform must not be silent)");
    }

    // ---- 6. polyphony gain / clipping (EXPECTED FAIL: no master limiter; unison sums > 1) ----
    {
        VAZCloneAudioProcessor p;
        setP(p, "voice_mode", 1.0f);                // Unison (0=Mono 1=Poly 2=Unison -> /2)
        setP(p, "uni_voices", 1.0f);                // 32 voices
        setP(p, "e1_attack", 0.05f); setP(p, "e1_sustain", 1.0f); setP(p, "uni_detune", 0.4f);
        const int on = (int)(0.05 * sr);
        auto b = render(p, sr, {{on, MidiMessage::noteOn(1, 50, 1.0f)},
                                {on, MidiMessage::noteOn(1, 57, 1.0f)}}, 0.50);
        const double pk = juce::jmax(peak(b.getReadPointer(0), on, b.getNumSamples()),
                                     peak(b.getReadPointer(1), on, b.getNumSamples()));
        check("polyphony_no_clip", pk <= 1.0, "peak=" + std::to_string(pk) + " (>1.0 clips at the output)");
    }

    std::cout << "\n=== " << (g_total - g_fail) << "/" << g_total << " passed, " << g_fail
              << " failed (failures pin audited bugs) ===\n";
    return g_fail;
}
