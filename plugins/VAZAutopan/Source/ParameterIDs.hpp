#pragma once
// VAZAutopan parameter IDs — match the data-param/data-toggle IDs in Source/ui/public/index.html.
namespace ParameterIDs
{
    constexpr auto left_limit    = "left_limit";    // 0..1 pan position the sweep reaches on the left  (0 = hard L)
    constexpr auto right_limit   = "right_limit";   // 0..1 pan position the sweep reaches on the right (1 = hard R)
    constexpr auto rate          = "rate";          // 0..1 → LFO 0.1..20 Hz
    constexpr auto waveform_sine = "waveform_sine"; // bool: Triangle (off, constant speed) / Sine (on, slow at limits)
}
