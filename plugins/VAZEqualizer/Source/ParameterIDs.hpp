#pragma once
// VAZEqualizer parameter IDs — match the data-param IDs in Source/ui/public/index.html.
// 4 bands (Low / Lo-Mid / Hi-Mid / High), each Gain / Freq / Q. All params normalised 0..1;
// the processor maps them to dB / Hz / Q. (VAZ EQ — Core.dll 0x517 region, RBJ biquads.)
namespace ParameterIDs
{
    // Low band — Q full-CCW = low-shelf, full-CW = high-pass (morph)
    constexpr auto low_gain   = "low_gain";     // 0..1 → -18..+18 dB
    constexpr auto low_freq   = "low_freq";     // 0..1 → 20 Hz..20 kHz (log)
    constexpr auto low_q      = "low_q";        // 0..1 → shelf(0) .. high-pass(1)

    // Lo-Mid band — always peaking
    constexpr auto lomid_gain = "lomid_gain";
    constexpr auto lomid_freq = "lomid_freq";
    constexpr auto lomid_q    = "lomid_q";      // 0..1 → Q 0.3..10 (log)

    // Hi-Mid band — always peaking
    constexpr auto himid_gain = "himid_gain";
    constexpr auto himid_freq = "himid_freq";
    constexpr auto himid_q    = "himid_q";

    // High band — Q full-CCW = high-shelf, full-CW = low-pass (morph)
    constexpr auto high_gain  = "high_gain";
    constexpr auto high_freq  = "high_freq";
    constexpr auto high_q     = "high_q";       // 0..1 → shelf(0) .. low-pass(1)
}
