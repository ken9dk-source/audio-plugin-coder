#pragma once
// VAZAutopan parameter IDs — match the data-param/data-toggle IDs in Source/ui/public/index.html.
namespace ParameterIDs
{
    constexpr auto left_limit    = "left_limit";    // 0..1 pan position the sweep reaches on the left  (0 = hard L)
    constexpr auto right_limit   = "right_limit";   // 0..1 pan position the sweep reaches on the right (1 = hard R)
    constexpr auto rate          = "rate";          // 0..1 → LFO 0.1..20 Hz (free)
    constexpr auto waveform_sine = "waveform_sine"; // bool: Triangle (off, constant speed) / Sine (on, slow at limits)
    constexpr auto mod_sync      = "mod_sync";      // bool: tempo-sync the pan rate (replaces Rate) — VAZ "Sync"
    constexpr auto mod_period    = "mod_period";    // choice 0..23: sync division (1/32T..256 beats), default 4 beats
}
