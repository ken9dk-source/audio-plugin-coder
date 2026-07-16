// VazPhaserParamTest — regression guard for the Stages discrete-snap fix (commit f2ec5a5).
// The bug it locks out: Stages reverting to a CONTINUOUS range (interval 0), which the WebView reads
// as "no snap" (index.html snapToLegalValue returns the raw value when interval==0). This test loads
// the REAL shipped parameter (not a copy) and asserts it snaps to exactly the 6 values 2/4/6/8/10/12.
// A UI regression can't be caught by the DSP oracle, so this is its own suite. Run: VazPhaserParamTest.exe
#include "PluginProcessor.h"
#include "ParameterIDs.hpp"
#include <juce_audio_processors/juce_audio_processors.h>
#include <iostream>
#include <set>
#include <cmath>
#include <string>

static int fails = 0;
static void check (bool ok, const std::string& msg)
{
    std::cout << (ok ? "  [PASS] " : "  [FAIL] ") << msg << "\n";
    if (! ok) ++fails;
}

int main()
{
    juce::ScopedJuceInitialiser_GUI init;
    VAZPhaserAudioProcessor proc;
    std::cout << "=== VazPhaserParamTest — Stages discrete-snap regression (ref: f2ec5a5) ===\n";

    auto* base = proc.apvts.getParameter (ParameterIDs::stages);   // RangedAudioParameter* (public getText)
    auto* p = dynamic_cast<juce::AudioParameterFloat*> (base);
    check (p != nullptr, "'stages' parameter exists and is an AudioParameterFloat");
    if (p != nullptr)
    {
        const auto r = p->getNormalisableRange();
        // interval != 0 is what makes the WebView (and the host) snap — a continuous range (interval 0)
        // is exactly the pre-f2ec5a5 regression ("dead zones", no discrete snap).
        check (r.start == 0.0f && r.end == 5.0f && r.interval == 1.0f,
               "range = [0,5] step 1 (interval != 0 -> snaps) -- got [" + std::to_string (r.start) + ","
               + std::to_string (r.end) + "] interval " + std::to_string (r.interval));

        // Sweep the whole normalised range; every reachable value must land on one of exactly 6 steps.
        std::set<int> steps;
        for (int i = 0; i <= 2000; ++i)
        {
            const float v = r.convertFrom0to1 ((float) i / 2000.0f);
            steps.insert ((int) std::lround (r.snapToLegalValue (v)));
        }
        check (steps == (std::set<int> { 0, 1, 2, 3, 4, 5 }),
               "snaps to EXACTLY 6 discrete steps across the full range (" + std::to_string (steps.size()) + " seen)");

        // Display strings must read 2/4/6/8/10/12 (N = (step+1)*2).
        const char* want[6] = { "2", "4", "6", "8", "10", "12" };
        bool disp = true; std::string got;
        for (int s = 0; s <= 5; ++s)
        {
            const juce::String txt = base->getText (r.convertTo0to1 ((float) s), 8);
            got += txt.toStdString() + (s < 5 ? "/" : "");
            if (txt != juce::String (want[s])) disp = false;
        }
        check (disp, "display strings = 2/4/6/8/10/12 (got " + got + ")");
    }

    std::cout << (fails == 0 ? "\nALL PASS\n" : "\n" + std::to_string (fails) + " FAILED\n");
    return fails ? 1 : 0;
}
