#pragma once
// VAZPhaser parameter IDs — match the data-param IDs in Source/ui/public/index.html.
namespace ParameterIDs
{
    constexpr auto stages    = "stages";     // 0..1 → 2/4/6/8/10/12 allpass stages
    constexpr auto frequency = "frequency";  // 0..1 → notch centre (Min..Max)
    constexpr auto feedback  = "feedback";   // 0..1 → resonance/intensity
    constexpr auto rate      = "rate";       // 0..1 → LFO speed
    constexpr auto depth     = "depth";      // 0..1 → LFO sweep amount
    constexpr auto lr_phase  = "lr_phase";   // 0..1 → L/R LFO phase offset (stereo)
    constexpr auto mix       = "mix";        // 0..1 → Dry..Wet
    constexpr auto gain      = "gain";       // 0..1 → output level (−inf..0 dB)
    constexpr auto feedback_phase = "feedback_phase";  // bool: − = inverted feedback polarity (VAZ Phase ±)
    constexpr auto mod_sync   = "mod_sync";    // bool: tempo-sync the modulation rate (replaces Rate)
    constexpr auto mod_period = "mod_period";  // choice 0..23: sync division (1/32T..256 beats)
}
