#pragma once
// VAZCompressor parameter IDs — match the data-param IDs in Source/ui/public/index.html.
// VAZ Compressor (Core.dll ~0x521-0x522): stereo-linked dynamics. All params normalised 0..1;
// the processor maps them to the VAZ GIF ranges. Defaults match the VAZ GIF readouts.
namespace ParameterIDs
{
    constexpr auto threshold = "threshold";   // 0..1 → -60..+6 dB  (default +1.0 dB = near top)
    constexpr auto ratio     = "ratio";       // 0..1 = slope (1-1/ratio): 0=1:1, 0.5=2:1, 1=inf:1 (limiter)
    constexpr auto attack    = "attack";      // 0..1 → 0.1..120 ms (log, default 5 ms)
    constexpr auto release   = "release";     // 0..1 → 5 ms..6 s   (log, default 50 ms)
    constexpr auto makeup    = "makeup";      // 0..1 → 0..+24 dB   (default 0 dB)
}
