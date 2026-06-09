#pragma once
// VAZChorus parameter IDs — match the data-param IDs in Source/ui/public/index.html.
namespace ParameterIDs
{
    constexpr auto delay     = "delay";      // 0..1 → base delay 5..30 ms (chorus centre)
    constexpr auto rate      = "rate";       // 0..1 → LFO speed
    constexpr auto depth     = "depth";      // 0..1 → modulation depth
    constexpr auto lr_phase  = "lr_phase";   // 0..1 → L/R LFO phase offset (stereo width)
    constexpr auto mix       = "mix";        // 0..1 → Dry..Wet
    constexpr auto gain      = "gain";       // 0..1 → output level (−inf..0 dB)
    constexpr auto waveform  = "waveform";   // choice 0..2: Sine / Triangle / Trapezoid (VAZ modes)
    constexpr auto mod_sync   = "mod_sync";    // bool: tempo-sync the modulation rate (replaces Rate)
    constexpr auto mod_period = "mod_period";  // choice 0..23: sync division (1/32T..256 beats)
}
