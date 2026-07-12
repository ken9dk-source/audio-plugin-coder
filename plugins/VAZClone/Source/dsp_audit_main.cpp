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
#include <cstdio>

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

    // ---- 3b. PULSE aliasing across the note range (BLEP port — user reports "C64/aliased" in FL Studio) ----
    {
        std::cout << "--- PULSE aliasing sweep (o1_wave=Pulse) ---\n";
        double worst = 0.0; int worstNote = 0; double worstSh = 0;
        for (double sh : { 0.1, 0.5, 0.9 })
        for (int note : { 36, 48, 60, 72, 84, 90, 96, 103 }) {
            VAZCloneAudioProcessor p;
            setP(p, "o1_wave", 1.0f / 4.0f); setP(p, "o1_shape", (float) sh);
            setP(p, "e1_attack", 0.05f); setP(p, "e1_sustain", 1.0f);
            const int on = (int)(0.05 * sr);
            auto b = render(p, sr, {{on, MidiMessage::noteOn(1, note, 1.0f)}}, 0.50);
            const double f0 = 440.0 * std::pow(2.0, (note - 69) / 12.0);
            const double alias = aliasingRatio(b.getReadPointer(0), on + (int)(0.1 * sr), sr, f0);
            std::printf("  shape=%.1f note %3d (%6.0f Hz)  aliasing=%.4f%s\n", sh, note, f0, alias, alias > 0.05 ? "  <== ALIASED" : "");
            if (alias > worst) { worst = alias; worstNote = note; worstSh = sh; }
        }
        check("pulse_aliasing_below_5pct", worst < 0.05,
              "worst=" + std::to_string(worst) + " @ note " + std::to_string(worstNote) + " shape " + std::to_string(worstSh));
    }

    // ---- 3d. PULSE harmonic ROLLOFF vs an ideal square (the real-VAZ render was FLAT = harsh "C64"; VAZ rolls off ~1/k) ----
    {
        std::cout << "--- PULSE harmonic rolloff (o1_wave=Pulse shape 0.5 square, note 48 ~130Hz, filter OPEN, 1 voice) ---\n";
        VAZCloneAudioProcessor p;
        setP(p, "o1_wave", 1.0f / 4.0f); setP(p, "o1_shape", 0.5f);
        setP(p, "cutoff", 1.0f); setP(p, "resonance", 0.0f);            // filter wide open — measure the bare osc
        setP(p, "e1_attack", 0.02f); setP(p, "e1_sustain", 1.0f);
        const int on = (int)(0.05 * sr);
        auto b = render(p, sr, {{on, MidiMessage::noteOn(1, 48, 1.0f)}}, 0.5);
        const double f0 = 440.0 * std::pow(2.0, (48 - 69) / 12.0);      // ~130.8 Hz
        const float* x = b.getReadPointer(0); const int a0 = on + (int)(0.12 * sr), a1 = b.getNumSamples();
        auto mag = [&](double f) { double re = 0, im = 0; for (int n = a0; n < a1; ++n) { double a = 2*M_PI*f*(n-a0)/sr; re += x[n]*std::cos(a); im -= x[n]*std::sin(a); } return std::hypot(re, im); };
        const double h1 = mag(f0);
        std::string row2;
        for (int k = 1; k <= 16; ++k) { char c[16]; std::snprintf(c, sizeof c, "%.3f ", mag(k*f0)/std::max(1e-9,h1)); row2 += c; }
        std::printf("  clone h1..h16 (norm): %s\n", row2.c_str());
        std::printf("  ideal square h1..h16: 1.000 0.000 0.333 0.000 0.200 0.000 0.143 0.000 0.111 0.000 0.091 0.000 0.077 0.000 0.067 0.000\n");
        // flatness metric: mean of h9..h15 (odd) relative to h1 — a proper square is ~1/k (small); flat/harsh is large
        const double hi = (mag(9*f0)+mag(11*f0)+mag(13*f0)+mag(15*f0))/4.0/std::max(1e-9,h1);
        check("pulse_rolls_off_like_square", hi < 0.20, "mean(h9,11,13,15)/h1=" + std::to_string(hi) + " (ideal ~0.09; flat/harsh >> that)");
    }

    // ---- 3e. reproduce the described FL patch (pulse + unison4 + R 4P LP Res Mod) — is IT flat/harsh + quiet? ----
    {
        std::cout << "--- FULL PATCH: pulse + unison4 + R 4P LP Res Mod, note 48, sweep reso ---\n";
        const double f0 = 440.0 * std::pow(2.0, (48 - 69) / 12.0);
        for (double reso : { 0.0, 0.5, 0.9 }) {
            VAZCloneAudioProcessor p;
            setP(p, "o1_wave", 1.0f / 4.0f); setP(p, "o1_shape", 0.5f);
            setP(p, "voice_mode", 1.0f); setP(p, "uni_voices", 3.0f / 31.0f); setP(p, "uni_detune", 0.3f);
            setP(p, "filter_mode", 19.0f / 21.0f);                  // R 4P LP Res Mod
            setP(p, "cutoff", 0.6f); setP(p, "resonance", (float) reso);
            setP(p, "e1_attack", 0.02f); setP(p, "e1_sustain", 1.0f);
            const int on = (int)(0.05 * sr);
            auto b = render(p, sr, {{on, MidiMessage::noteOn(1, 48, 1.0f)}}, 0.5);
            const float* x = b.getReadPointer(0); const int a0 = on + (int)(0.12 * sr), a1 = b.getNumSamples();
            const double r = rms(x, a0, a1), pk = peak(x, a0, a1);
            const double al = aliasingRatio(x, a0, sr, f0);
            std::printf("  reso=%.1f  RMS=%.4f peak=%.4f aliasing=%.4f\n", reso, r, pk, al);
        }
        std::puts("  (real-VAZ render was RMS 0.142; clone render RMS 0.0099 = ~14x quieter)");
    }

    // ---- 3c. reproduce the FL patch (unison + pulse) and test 44.1 kHz (FL default) ----
    {
        std::cout << "--- PULSE unison + sample-rate repro ---\n";
        const double f0 = 440.0 * std::pow(2.0, (60 - 69) / 12.0);
        { VAZCloneAudioProcessor p; setP(p, "o1_wave", 0.25f); setP(p, "o1_shape", 0.5f);
          setP(p, "voice_mode", 1.0f); setP(p, "uni_voices", 3.0f / 31.0f); setP(p, "uni_detune", 0.3f);
          setP(p, "e1_attack", 0.05f); setP(p, "e1_sustain", 1.0f);
          const int on = (int)(0.05 * sr); auto b = render(p, sr, {{on, MidiMessage::noteOn(1, 60, 1.0f)}}, 0.5);
          const double al = aliasingRatio(b.getReadPointer(0), on + (int)(0.1 * sr), sr, f0);
          const double pk = peak(b.getReadPointer(0), on + (int)(0.1 * sr), b.getNumSamples());
          std::printf("  unison4 pulse note60 @48000  aliasing=%.4f peak=%.4f%s\n", al, pk, al > 0.05 ? "  <== ALIASED" : ""); }
        for (double sr2 : { 44100.0, 96000.0 }) {
          VAZCloneAudioProcessor p; setP(p, "o1_wave", 0.25f); setP(p, "o1_shape", 0.5f);
          setP(p, "e1_attack", 0.05f); setP(p, "e1_sustain", 1.0f);
          const int on = (int)(0.05 * sr2); auto b = render(p, sr2, {{on, MidiMessage::noteOn(1, 60, 1.0f)}}, 0.5);
          const double al = aliasingRatio(b.getReadPointer(0), on + (int)(0.1 * sr2), sr2, f0);
          std::printf("  single  pulse note60 @%5.0f  aliasing=%.4f%s\n", sr2, al, al > 0.05 ? "  <== ALIASED" : ""); }
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

    // ==== AUDIBILITY MATRIX (surfaces missing clamp/guard bugs like the Pulse min-edge one) ====
    // For every osc waveform + every filter mode, render default & extreme settings and print RMS/peak.
    // A SILENT cell (RMS<1e-4) or a BLOWUP cell (peak>4 or non-finite) at a musically-valid setting = a suspect.
    auto measure = [&](VAZCloneAudioProcessor& p, int note) {
        setP(p, "e1_attack", 0.02f); setP(p, "e1_sustain", 1.0f);
        const int on = (int)(0.05 * sr);
        auto b = render(p, sr, {{on, MidiMessage::noteOn(1, note, 1.0f)}}, 0.35);
        const int a0 = on + (int)(0.12 * sr), a1 = b.getNumSamples();
        return std::pair<double,double>{ rms(b.getReadPointer(0), a0, a1), peak(b.getReadPointer(0), a0, a1) };
    };
    auto flag = [](double r, double pk) { return r < 1e-4 ? "  <== SILENT" : (!std::isfinite(pk) || pk > 4.0 ? "  <== BLOWUP" : ""); };

    std::cout << "\n--- OSC waveform audibility (A-LP open, note 60, sustain) ---\n";
    { const char* wn[5] = {"Saw","Pulse","MultiSaw","Sample","Ext"};
      for (int w = 0; w < 5; ++w) for (double sh : {0.0, 0.5, 1.0}) {
        VAZCloneAudioProcessor p; setP(p, "o1_wave", w / 4.0f); setP(p, "o1_shape", (float) sh);
        auto [r, pk] = measure(p, 60);
        std::printf("  %-9s shape=%.1f   RMS=%.5f  peak=%.4f%s\n", wn[w], sh, r, pk, flag(r, pk)); } }

    std::cout << "--- FILTER audibility (Saw source, note 60, sustain; reso 0, cutoff mid & high) ---\n";
    for (int m = 0; m <= 21; ++m) for (double cf : {0.5, 0.9}) {
        VAZCloneAudioProcessor p; setP(p, "o1_wave", 0.0f); setP(p, "filter_mode", m / 21.0f);
        setP(p, "resonance", 0.0f); setP(p, "cutoff", (float) cf);
        auto [r, pk] = measure(p, 60);
        std::printf("  mode %2d cutoff=%.1f   RMS=%.5f  peak=%.4f%s\n", m, cf, r, pk, flag(r, pk)); }

    // Regression guards: every waveform (default shape) and every filter mode (default open cutoff, reso 0) must be
    // audible at default settings — this is the class of bug the Pulse min-edge clamp fell into.
    { std::string bad;
      for (int w = 0; w < 5; ++w) { VAZCloneAudioProcessor p; setP(p, "o1_wave", w / 4.0f); auto [r, pk] = measure(p, 60);
        if (r <= 1e-3) bad += " w" + std::to_string(w) + "(RMS=" + std::to_string(r) + ")"; }
      check("all_osc_audible_at_default", bad.empty(), bad.empty() ? "5 waveforms, default shape, open filter" : "SILENT:" + bad); }
    // (HP modes are correctly silent at max cutoff — the note is below the HP freq — so sweep cutoffs and require
    //  the filter to pass the note at SOME musical setting. A mode silent at every cutoff is the real bug.)
    { std::string bad;
      for (int m = 0; m <= 21; ++m) { double best = 0.0;
        for (double cf : {0.3, 0.5, 0.7}) { VAZCloneAudioProcessor p; setP(p, "o1_wave", 0.0f); setP(p, "filter_mode", m / 21.0f); setP(p, "cutoff", (float) cf); auto [r, pk] = measure(p, 60); best = juce::jmax(best, r); }
        if (best <= 1e-3) bad += " m" + std::to_string(m) + "(maxRMS=" + std::to_string(best) + ")"; }
      check("all_filters_pass_note_somewhere", bad.empty(), bad.empty() ? "22 modes audible at cutoff 0.3/0.5/0.7" : "SILENT AT ALL CUTOFFS:" + bad); }

    std::cout << "\n=== " << (g_total - g_fail) << "/" << g_total << " passed, " << g_fail
              << " failed (failures pin audited bugs) ===\n";
    return g_fail;
}
