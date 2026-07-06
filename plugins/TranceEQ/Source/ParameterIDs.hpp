#pragma once
#include <juce_core/juce_core.h>
//==============================================================================
// TranceEQ parameter IDs. Global IDs are constexpr; the 6 bands are generated
// (b0_freq, b0_gain, ...). The same IDs are used verbatim as [data-param] /
// [data-combo] / [data-toggle] attributes in Source/ui/public/index.html.
//==============================================================================
namespace ParameterIDs
{
    inline constexpr int kNumBands = 6;

    // ---- global ----
    inline constexpr auto mode    = "mode";     // choice: Lead / Pluck / Midbass
    inline constexpr auto bypass  = "bypass";   // bool
    inline constexpr auto output  = "output";   // dB
    inline constexpr auto kt_on   = "kt_on";    // bool  — key-track engage
    inline constexpr auto kt_src  = "kt_src";   // choice: Audio / MIDI
    inline constexpr auto kt_amt  = "kt_amt";   // 0..1  — tracking amount
    inline constexpr auto kt_ref  = "kt_ref";   // Hz    — reference pitch
    inline constexpr auto os      = "os";       // choice: Off / 2x / 4x — oversampling

    // ---- per-band ID builders ----
    inline juce::String bandOn   (int b) { return "b" + juce::String (b) + "_on"; }
    inline juce::String bandType (int b) { return "b" + juce::String (b) + "_type"; }
    inline juce::String bandFreq (int b) { return "b" + juce::String (b) + "_freq"; }
    inline juce::String bandGain (int b) { return "b" + juce::String (b) + "_gain"; }
    inline juce::String bandQ    (int b) { return "b" + juce::String (b) + "_q"; }
    inline juce::String bandTrack(int b) { return "b" + juce::String (b) + "_track"; }
}
