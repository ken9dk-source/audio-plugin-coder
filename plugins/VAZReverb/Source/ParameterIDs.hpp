#pragma once
// VAZReverb parameter IDs — match the data-param IDs in Source/ui/public/index.html.
namespace ParameterIDs
{
    constexpr auto reverb_time = "reverb_time";   // 0..1 → ~0.5s..2.5s decay
    constexpr auto tone        = "tone";          // 0..1 → Dark..Bright (damping)
    constexpr auto mix         = "mix";           // 0..1 → Dry..Wet
}
