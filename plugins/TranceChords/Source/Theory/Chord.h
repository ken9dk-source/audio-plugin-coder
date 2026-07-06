#pragma once
#include <vector>
#include <juce_core/juce_core.h>
#include "Scales.h"

//==============================================================================
// Chord model for TranceChords. A chord = root pitch-class + ChordType (which
// expands to semitone intervals from the root) + a human label ("Am7","Cmaj9").
// Voicing.h turns this into absolute MIDI notes. Built on the message thread.
//==============================================================================
namespace tc
{
    // Vocal/uplifting-trance whitelist ONLY. No jazz qualities (maj7, dom7, 6,
    // 9/11/13, dim, m7b5, aug, altered). The backbone is minor/major triads +
    // sus + add9; minor-7 is the single (sparingly used) 7th; power-5 for stabs.
    enum class ChordType
    {
        Minor = 0, Major,
        Sus2, Sus4,
        MinAdd9, MajAdd9,
        Min7,
        Power5,
        NumTypes
    };

    // semitone offsets from the root (root always present as 0). Add9 uses the
    // major 9th at +14; nothing extends above the 9th; nothing contains a tritone.
    inline std::vector<int> chordIntervals (ChordType t)
    {
        switch (t)
        {
            case ChordType::Minor:    return { 0, 3, 7 };
            case ChordType::Major:    return { 0, 4, 7 };
            case ChordType::Sus2:     return { 0, 2, 7 };
            case ChordType::Sus4:     return { 0, 5, 7 };
            case ChordType::MinAdd9:  return { 0, 3, 7, 14 };
            case ChordType::MajAdd9:  return { 0, 4, 7, 14 };
            case ChordType::Min7:     return { 0, 3, 7, 10 };
            case ChordType::Power5:   return { 0, 7 };
            default:                  return { 0, 3, 7 };
        }
    }

    inline const char* chordSuffix (ChordType t) noexcept
    {
        switch (t)
        {
            case ChordType::Minor:    return "m";
            case ChordType::Major:    return "";
            case ChordType::Sus2:     return "sus2";
            case ChordType::Sus4:     return "sus4";
            case ChordType::MinAdd9:  return "m(add9)";
            case ChordType::MajAdd9:  return "add9";
            case ChordType::Min7:     return "m7";
            case ChordType::Power5:   return "5";
            default:                  return "";
        }
    }

    inline bool isMinorTriadBase (ChordType t) noexcept
    {
        return t == ChordType::Minor || t == ChordType::MinAdd9 || t == ChordType::Min7;
    }

    struct Chord
    {
        int       rootPc      = 0;            // 0..11
        ChordType type        = ChordType::Minor;
        juce::String label;                   // "Am7"
        int       romanDegree = -1;           // scale degree 0..6, or -1 if borrowed/chromatic
        juce::String roman;                   // "i", "VI", "bVII", "Vsus4"...
        int       bassPc      = -1;           // -1 = root in bass; else slash-chord bass pc
        bool      locked      = false;
        double    startBeat   = 0.0;
        double    lengthBeats = 4.0;

        // When non-empty these override the ChordType intervals — lets the rule
        // engine stack scale-accurate tones for any mode (e.g. Phrygian b9).
        std::vector<int> customIntervals;

        // effective semitone offsets from the root (root = 0 first)
        std::vector<int> intervals() const
        {
            return customIntervals.empty() ? chordIntervals (type) : customIntervals;
        }

        // unique pitch classes in the chord (mod 12), root first
        std::vector<int> pitchClasses() const
        {
            std::vector<int> out;
            for (int iv : intervals())
            {
                int pc = ((rootPc + iv) % 12 + 12) % 12;
                if (std::find (out.begin(), out.end(), pc) == out.end())
                    out.push_back (pc);
            }
            return out;
        }
    };

    inline Chord makeChord (int rootPc, ChordType type, bool preferFlats)
    {
        Chord c;
        c.rootPc = ((rootPc % 12) + 12) % 12;
        c.type   = type;
        c.label  = pcName (c.rootPc, preferFlats) + juce::String (chordSuffix (type));
        return c;
    }
}
