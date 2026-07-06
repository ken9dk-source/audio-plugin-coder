#pragma once
#include <juce_core/juce_core.h>
//==============================================================================
// TranceChords parameter IDs. Ranges/defaults live in createParameterLayout()
// and mirror .ideas/parameter-spec.md. The native editor attaches to these.
//==============================================================================
namespace ParameterIDs
{
    // ---- generation ----
    inline constexpr auto key           = "key";            // choice: C..B (pitch class)
    inline constexpr auto mode          = "mode";           // choice: Ionian..Melodic Minor
    inline constexpr auto section       = "section";        // choice: Verse / Pre-Chorus / Chorus
    inline constexpr auto mood          = "mood";           // choice: Dreamy / Romantic / Euphoric
    inline constexpr auto style         = "style";          // choice: Any / Uplifting / Oldschool 2000
    inline constexpr auto length        = "length";         // choice: 4 / 8 / 16 bars
    inline constexpr auto density       = "density";        // choice: 1 per bar / 2 per bar
    inline constexpr auto energy        = "energy";         // %
    inline constexpr auto complexity    = "complexity";     // %
    inline constexpr auto voice_leading = "voice_leading";  // % (0 free .. 100 strict)
    inline constexpr auto variation     = "variation";      // % (0 template .. 100 free rules)
    inline constexpr auto humanize      = "humanize";       // %
    inline constexpr auto swing         = "swing";          // % groove on rhythmic layers
    inline constexpr auto voicing_style = "voicing_style";  // choice: Close/Open/Drop-2/Wide
    inline constexpr auto octave        = "octave";         // -2..+2

    // ---- chord-type toggles ----
    inline constexpr auto allow_sus      = "allow_sus";      // bool
    inline constexpr auto allow_borrowed = "allow_borrowed"; // bool
    inline constexpr auto forbid_triads  = "forbid_triads";  // bool
    inline constexpr auto no_pop         = "no_pop";         // bool
    inline constexpr auto scale_lock     = "scale_lock";     // bool
    inline constexpr auto modulation     = "modulation";     // bool
    inline constexpr auto sec_dominants  = "sec_dominants";  // bool — secondary dominants (V7/x)

    // ---- bassline layer ----
    inline constexpr auto bass_enable  = "bass_enable";   // bool
    inline constexpr auto bass_pattern = "bass_pattern";  // choice: Sustained/Root 8ths/Offbeat/Rolling
    inline constexpr auto bass_octave  = "bass_octave";   // 1..3
    inline constexpr auto bass_gate    = "bass_gate";     // %

    // ---- arpeggiator layer ----
    inline constexpr auto arp_enable   = "arp_enable";    // bool
    inline constexpr auto arp_pattern  = "arp_pattern";   // choice: Up/Down/UpDown/Converge/Random
    inline constexpr auto arp_rate     = "arp_rate";      // choice: 1/8 / 1/16 / 1/8T
    inline constexpr auto arp_octaves  = "arp_octaves";   // 1..2
    inline constexpr auto arp_gate     = "arp_gate";      // %

    // ---- counter-melody layer ----
    inline constexpr auto counter_enable  = "counter_enable";  // bool
    inline constexpr auto counter_pattern = "counter_pattern"; // choice: Outline/Top Voice/Pulse
    inline constexpr auto counter_rate    = "counter_rate";    // choice: 1/8 / 1/16 / 1/8T

    // ---- melody-aware generation ----
    inline constexpr auto melody_fit = "melody_fit";   // bool — fit chords to a loaded melody

    // ---- song mode (chain sections into a full arrangement) ----
    inline constexpr auto song_mode = "song_mode";     // bool
    inline constexpr auto song_form = "song_form";     // choice: arrangement template

    // ---- layer mixer (preview levels) ----
    inline constexpr auto mix_chords  = "mix_chords";  // %
    inline constexpr auto mix_bass    = "mix_bass";    // %
    inline constexpr auto mix_arp     = "mix_arp";     // %
    inline constexpr auto mix_counter = "mix_counter"; // %

    // ---- preview pad ----
    inline constexpr auto prev_enable  = "prev_enable";   // bool
    inline constexpr auto prev_attack  = "prev_attack";   // ms
    inline constexpr auto prev_release = "prev_release";  // ms
    inline constexpr auto prev_cutoff  = "prev_cutoff";   // Hz
    inline constexpr auto prev_detune  = "prev_detune";   // %
    inline constexpr auto prev_reverb  = "prev_reverb";   // %
    inline constexpr auto prev_chorus  = "prev_chorus";   // %
    inline constexpr auto pump         = "pump";          // % sidechain-style pump
    inline constexpr auto output       = "output";        // dB
}
