#pragma once
#include <array>
#include <juce_core/juce_core.h>

//==============================================================================
// Scales / modes for TranceChords. Pitch classes are 0..11 (C=0). A "scale" is
// 7 semitone offsets from the tonic. Diatonic chord qualities are NOT hard-coded
// per mode — they are derived by stacking scale-thirds in Generator.h, so every
// mode (incl. harmonic/melodic minor) gets correct chords automatically.
//==============================================================================
namespace tc
{
    enum class Mode
    {
        Ionian = 0, Dorian, Phrygian, Lydian, Mixolydian, Aeolian, Locrian,
        HarmonicMinor, MelodicMinor,
        NumModes
    };

    inline const char* modeName (Mode m) noexcept
    {
        switch (m)
        {
            case Mode::Ionian:        return "Ionian";
            case Mode::Dorian:        return "Dorian";
            case Mode::Phrygian:      return "Phrygian";
            case Mode::Lydian:        return "Lydian";
            case Mode::Mixolydian:    return "Mixolydian";
            case Mode::Aeolian:       return "Aeolian";
            case Mode::Locrian:       return "Locrian";
            case Mode::HarmonicMinor: return "Harmonic Minor";
            case Mode::MelodicMinor:  return "Melodic Minor";
            default:                  return "?";
        }
    }

    // semitone offsets from tonic for the 7 scale degrees
    inline std::array<int, 7> scaleSemitones (Mode m) noexcept
    {
        switch (m)
        {
            case Mode::Ionian:        return { 0, 2, 4, 5, 7, 9, 11 };
            case Mode::Dorian:        return { 0, 2, 3, 5, 7, 9, 10 };
            case Mode::Phrygian:      return { 0, 1, 3, 5, 7, 8, 10 };
            case Mode::Lydian:        return { 0, 2, 4, 6, 7, 9, 11 };
            case Mode::Mixolydian:    return { 0, 2, 4, 5, 7, 9, 10 };
            case Mode::Aeolian:       return { 0, 2, 3, 5, 7, 8, 10 };
            case Mode::Locrian:       return { 0, 1, 3, 5, 6, 8, 10 };
            case Mode::HarmonicMinor: return { 0, 2, 3, 5, 7, 8, 11 };
            case Mode::MelodicMinor:  return { 0, 2, 3, 5, 7, 9, 11 };
            default:                  return { 0, 2, 4, 5, 7, 9, 11 };
        }
    }

    // "minor-ish" modes default to a slightly lower output octave & flat spelling bias
    inline bool isMinorMode (Mode m) noexcept
    {
        return m == Mode::Dorian || m == Mode::Phrygian || m == Mode::Aeolian
            || m == Mode::Locrian || m == Mode::HarmonicMinor || m == Mode::MelodicMinor;
    }

    // the 7 absolute pitch classes (0..11) of key+mode
    inline std::array<int, 7> scalePitchClasses (int tonicPc, Mode m) noexcept
    {
        const auto s = scaleSemitones (m);
        std::array<int, 7> out {};
        for (int i = 0; i < 7; ++i)
            out[(size_t) i] = ((tonicPc + s[(size_t) i]) % 12 + 12) % 12;
        return out;
    }

    inline bool scaleContains (const std::array<int, 7>& pcs, int pc) noexcept
    {
        pc = ((pc % 12) + 12) % 12;
        for (int p : pcs) if (p == pc) return true;
        return false;
    }

    // snap a pitch class to the nearest scale pitch class (for scale-lock)
    inline int nearestScalePc (const std::array<int, 7>& pcs, int pc) noexcept
    {
        pc = ((pc % 12) + 12) % 12;
        int best = pcs[0], bestDist = 12;
        for (int p : pcs)
        {
            int d = std::abs (p - pc);
            d = std::min (d, 12 - d);
            if (d < bestDist) { bestDist = d; best = p; }
        }
        return best;
    }

    // pretty pitch-class name. preferFlats chooses Eb/Ab/Bb spellings.
    inline juce::String pcName (int pc, bool preferFlats) noexcept
    {
        pc = ((pc % 12) + 12) % 12;
        static const char* sharp[12] = { "C","C#","D","D#","E","F","F#","G","G#","A","A#","B" };
        static const char* flat [12] = { "C","Db","D","Eb","E","F","Gb","G","Ab","A","Bb","B" };
        return juce::String (preferFlats ? flat[pc] : sharp[pc]);
    }

    // conventional flat-spelled tonics (so e.g. an Ab/Eb/Bb key reads with flats)
    inline bool keyPrefersFlats (int tonicPc, Mode m) noexcept
    {
        // flat keys (major-equivalent): F, Bb, Eb, Ab, Db, Gb  -> pc 5,10,3,8,1,6
        // minor-ish: spell with flats more often (d, g, c, f, bb minors etc.)
        static const std::array<int, 6> flatMajors { 5, 10, 3, 8, 1, 6 };
        for (int p : flatMajors) if (p == tonicPc) return true;
        if (isMinorMode (m))
        {
            // c, f, bb, eb, ab, d, g minors tend to read flat
            static const std::array<int, 7> flatMinors { 0, 5, 10, 3, 8, 2, 7 };
            for (int p : flatMinors) if (p == tonicPc) return true;
        }
        return false;
    }
}
