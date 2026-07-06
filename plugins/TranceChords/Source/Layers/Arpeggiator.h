#pragma once
#include <random>
#include <algorithm>
#include "../Midi/MidiExport.h"

//==============================================================================
// Arpeggiator generator — turns each chord's voiced notes into a stepped arp.
// Patterns: Up / Down / UpDown / Converge / Random. Rate 1/8, 1/16 or 1/8T,
// with an octave range and gate. Deterministic (fixed seed) so the audible
// preview matches the exported/dragged MIDI exactly.
//==============================================================================
namespace tc
{
    enum class ArpPattern { Up = 0, Down, UpDown, Converge, Random, NumPatterns };
    enum class ArpRate    { Eighth = 0, Sixteenth, EighthTriplet, NumRates };

    inline const char* arpPatternName (ArpPattern p) noexcept
    {
        switch (p) { case ArpPattern::Up: return "Up"; case ArpPattern::Down: return "Down";
                     case ArpPattern::UpDown: return "Up/Down"; case ArpPattern::Converge: return "Converge";
                     case ArpPattern::Random: return "Random"; default: return "?"; }
    }
    inline const char* arpRateName (ArpRate r) noexcept
    {
        switch (r) { case ArpRate::Eighth: return "1/8"; case ArpRate::Sixteenth: return "1/16";
                     case ArpRate::EighthTriplet: return "1/8T"; default: return "?"; }
    }
    inline double arpRateBeats (ArpRate r) noexcept
    {
        switch (r) { case ArpRate::Eighth: return 0.5; case ArpRate::Sixteenth: return 0.25;
                     case ArpRate::EighthTriplet: return 1.0 / 3.0; default: return 0.25; }
    }

    struct ArpParams
    {
        ArpPattern pattern = ArpPattern::Up;
        ArpRate    rate    = ArpRate::Sixteenth;
        int   octaves     = 1;     // 1..2 — how many octaves the arp spans
        int   octaveShift = 1;     // lift above the chord register
        float gate        = 0.70f;
        float swing       = 0.0f;  // 0..0.66 groove on off-steps
        int   velocity    = 96;
    };

    inline NoteLayer generateArp (const std::vector<VoicedChord>& voiced, const ArpParams& ap)
    {
        NoteLayer layer;
        const double step = arpRateBeats (ap.rate);
        const float  gate = juce::jlimit (0.1f, 1.0f, ap.gate);
        const float  swing = juce::jlimit (0.0f, 0.66f, ap.swing);
        std::mt19937 rng (0xA5EED1);

        for (const auto& vc : voiced)
        {
            if (vc.notes.empty()) continue;

            std::vector<int> pool;
            for (int o = 0; o < juce::jmax (1, ap.octaves); ++o)
                for (int n : vc.notes)
                    pool.push_back (juce::jlimit (0, 127, n + ap.octaveShift * 12 + o * 12));
            std::sort (pool.begin(), pool.end());
            pool.erase (std::unique (pool.begin(), pool.end()), pool.end());
            if (pool.empty()) continue;

            std::vector<int> seq;
            switch (ap.pattern)
            {
                case ArpPattern::Up:   seq = pool; break;
                case ArpPattern::Down: seq = pool; std::reverse (seq.begin(), seq.end()); break;
                case ArpPattern::UpDown:
                    seq = pool;
                    for (int i = (int) pool.size() - 2; i > 0; --i) seq.push_back (pool[(size_t) i]);
                    break;
                case ArpPattern::Converge:
                {
                    int a = 0, b = (int) pool.size() - 1;
                    while (a <= b) { seq.push_back (pool[(size_t) a]); if (a != b) seq.push_back (pool[(size_t) b]); ++a; --b; }
                    break;
                }
                case ArpPattern::Random: default: seq = pool; break;
            }
            if (seq.empty()) seq = pool;

            const int nsteps = (int) std::floor ((vc.lengthBeats + 1.0e-6) / step);
            for (int s = 0; s < nsteps; ++s)
            {
                const int note = (ap.pattern == ArpPattern::Random)
                               ? pool[(size_t) (rng() % pool.size())]
                               : seq[(size_t) (s % seq.size())];
                const double sw = (s % 2 == 1) ? step * swing : 0.0;
                layer.push_back ({ vc.startBeat + s * step + sw, step * gate, note, ap.velocity });
            }
        }
        return layer;
    }
}
