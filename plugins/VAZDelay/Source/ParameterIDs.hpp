#pragma once
// VAZDelay parameter IDs — match the data-param/data-combo/data-toggle IDs in index.html.
namespace ParameterIDs
{
    constexpr auto mode  = "mode";   // choice: 0 Stereo / 1 Ping-Pong / 2 Double
    constexpr auto link  = "link";   // bool: L/R controls linked (R uses L's values)
    constexpr auto sync  = "sync";   // bool: tempo-sync the delay times

    constexpr auto note_l  = "note_l";   // choice: sync note value (base) for L
    constexpr auto note_r  = "note_r";   // choice: sync note value (base) for R
    constexpr auto delay_l = "delay_l";  // 0..1 → 5ms..6s (free) OR 50%..200% of the note base (sync)
    constexpr auto fb_l    = "fb_l";     // 0..1 feedback
    constexpr auto tone_l  = "tone_l";   // 0..1 Dark..Bright (1-pole LP in feedback)
    constexpr auto wet_l   = "wet_l";    // 0..1 wet level
    constexpr auto dry_l   = "dry_l";    // 0..1 dry level

    constexpr auto delay_r = "delay_r";
    constexpr auto fb_r    = "fb_r";
    constexpr auto tone_r  = "tone_r";
    constexpr auto wet_r   = "wet_r";
    constexpr auto dry_r   = "dry_r";
}
