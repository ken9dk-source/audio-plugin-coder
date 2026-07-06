#pragma once
#include "../Midi/MidiExport.h"

//==============================================================================
// Bassline generator — derives a trance bass line from the chord progression.
// Patterns cover the staple trance feels: Sustained, driving Root 8ths, the
// classic Offbeat ("& of every beat") bass, and a Rolling 16th line.
//==============================================================================
namespace tc
{
    enum class BassPattern { Sustained = 0, Root8ths, Offbeat, Rolling, Octaves, Walking, NumPatterns };

    inline const char* bassPatternName (BassPattern p) noexcept
    {
        switch (p) { case BassPattern::Sustained: return "Sustained";
                     case BassPattern::Root8ths:  return "Root 8ths";
                     case BassPattern::Offbeat:   return "Offbeat";
                     case BassPattern::Rolling:   return "Rolling 16ths";
                     case BassPattern::Octaves:   return "Octaves";
                     case BassPattern::Walking:   return "Walking"; default: return "?"; }
    }

    struct BassParams
    {
        BassPattern pattern = BassPattern::Offbeat;
        int   octave   = 2;     // 1..3 (lower = deeper)
        float gate     = 0.72f; // note length as fraction of the step
        float swing    = 0.0f;  // 0..0.66 groove on off-steps
        int   velocity = 104;
    };

    // place the chord root/bass pitch-class in a low octave
    inline int bassNoteFor (const Chord& c, int octave)
    {
        const int pc   = (c.bassPc >= 0 ? c.bassPc : c.rootPc);
        const int base = 12 + juce::jlimit (1, 3, octave) * 12;          // oct 2 -> C2 (36)
        const int n    = base + (((pc - (base % 12)) % 12) + 12) % 12;   // nearest pc at/above base
        return juce::jlimit (24, 55, n);
    }

    inline NoteLayer generateBass (const Progression& prog, const BassParams& bp)
    {
        NoteLayer layer;
        const float gate  = juce::jlimit (0.1f, 1.0f, bp.gate);
        const float swing = juce::jlimit (0.0f, 0.66f, bp.swing);
        for (const auto& c : prog)
        {
            const int    root  = bassNoteFor (c, bp.octave);
            const double start = c.startBeat, len = c.lengthBeats;
            // i = step index (drives the swing on off-steps); note can vary per step
            auto add = [&] (int i, double offset, double step, int note)
            {
                if (offset >= len - 1.0e-6) return;
                const double sw = (i % 2 == 1) ? step * swing : 0.0;
                layer.push_back ({ start + offset + sw, step * gate, juce::jlimit (24, 67, note), bp.velocity });
            };

            switch (bp.pattern)
            {
                case BassPattern::Sustained: layer.push_back ({ start, len * juce::jmax (gate, 0.95f), root, bp.velocity }); break;
                case BassPattern::Root8ths:  { int i = 0; for (double b = 0.0; b < len - 1e-6; b += 0.5)  add (i++, b, 0.5, root); } break;
                case BassPattern::Offbeat:   { int i = 0; for (double b = 0.5; b < len - 1e-6; b += 1.0)  add (i++, b, 0.5, root); } break;
                case BassPattern::Rolling:   { int i = 0; for (double b = 0.0; b < len - 1e-6; b += 0.25) add (i++, b, 0.25, root); } break;
                case BassPattern::Octaves:   { int i = 0; for (double b = 0.0; b < len - 1e-6; b += 0.5)  { add (i, b, 0.5, root + (i % 2 ? 12 : 0)); ++i; } } break;
                case BassPattern::Walking:   { static const int seq[4] { 0, 7, 12, 7 }; int i = 0; for (double b = 0.0; b < len - 1e-6; b += 0.5) { add (i, b, 0.5, root + seq[i % 4]); ++i; } } break;
                default:                     layer.push_back ({ start, len, root, bp.velocity }); break;
            }
        }
        return layer;
    }
}
