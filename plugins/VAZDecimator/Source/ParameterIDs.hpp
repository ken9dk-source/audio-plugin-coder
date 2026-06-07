#pragma once
// VAZDecimator parameter IDs — match the data-param IDs in Source/ui/public/index.html.
namespace ParameterIDs
{
    constexpr auto sample_rate = "sample_rate";   // 0..1 → effective SR (heavy .. full/transparent)
    constexpr auto bit_depth   = "bit_depth";     // 0..1 → bits (1-bit .. 16-bit/transparent)
}
