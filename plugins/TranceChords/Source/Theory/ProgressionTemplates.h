#pragma once
#include <vector>
#include "Chord.h"

//==============================================================================
// The 24 worked example progressions from the PDF (8 verse / 8 pre-chorus /
// 8 chorus). Each chord is stored as (semitone-from-tonic, ChordType) so it
// transposes to any key. Tagged by Section + Mood. The Generator uses these as
// curated seeds (low Variation) and as the basis for preset "feel".
//
// NOTE: a few PDF chords are approximated to the nearest type we model
// (e.g. "add2" -> Add9, "Am(add9)" -> Min9, "E7sus2" -> Sus2).
//==============================================================================
namespace tc
{
    enum class Section { Verse = 0, PreChorus, Chorus, Breakdown, Buildup, Drop, Bridge, NumSections };
    enum class Mood    { Dreamy = 0, Romantic, Euphoric, NumMoods };

    inline const char* sectionName (Section s) noexcept
    {
        switch (s) { case Section::Verse: return "Verse";
                     case Section::PreChorus: return "Pre-Chorus";
                     case Section::Chorus: return "Chorus";
                     case Section::Breakdown: return "Breakdown";
                     case Section::Buildup: return "Build-up";
                     case Section::Drop: return "Drop";
                     case Section::Bridge: return "Bridge"; default: return "?"; }
    }
    inline const char* moodName (Mood m) noexcept
    {
        switch (m) { case Mood::Dreamy: return "Dreamy";
                     case Mood::Romantic: return "Romantic";
                     case Mood::Euphoric: return "Euphoric"; default: return "?"; }
    }

    struct TemplateChord { int semi; ChordType type; };
    struct ProgTemplate  { Section section; Mood mood; bool minorKey; std::vector<TemplateChord> chords; };

    inline const std::vector<ProgTemplate>& progressionTemplates()
    {
        using S = Section; using M = Mood; using C = ChordType;
        static const std::vector<ProgTemplate> t =
        {
            // ---------------- VERSE (low tension, repeating) ----------------
            { S::Verse, M::Dreamy,   false, { {0,C::MajAdd9},{7,C::Sus4},{9,C::Minor},{5,C::Major} } },      // C: Cadd9 Gsus4 Am F
            { S::Verse, M::Romantic, true,  { {0,C::Minor},{10,C::Major},{8,C::Major},{7,C::Minor} } },      // Am: Am G F Em  (i-VII-VI-v)
            { S::Verse, M::Euphoric, false, { {0,C::Major},{9,C::Minor},{2,C::MinAdd9},{7,C::Sus4} } },      // D: D Bm Em(add9) Asus4
            { S::Verse, M::Romantic, true,  { {0,C::MinAdd9},{7,C::Minor},{3,C::MajAdd9},{10,C::Major} } },  // Fm: Fm(add9) Cm Abadd9 Eb
            { S::Verse, M::Dreamy,   false, { {0,C::MajAdd9},{7,C::Sus2},{9,C::Minor},{5,C::MajAdd9} } },    // G: Gadd9 Dsus2 Em Cadd9
            { S::Verse, M::Romantic, true,  { {0,C::Minor},{8,C::Major},{7,C::Sus4},{5,C::Minor} } },        // Em: Em C Bsus4 Am
            { S::Verse, M::Dreamy,   false, { {0,C::Major},{7,C::Major},{9,C::Minor},{4,C::Minor} } },       // Ab: Ab Eb Fm Cm
            { S::Verse, M::Euphoric, false, { {0,C::Major},{7,C::MajAdd9},{9,C::MinAdd9},{4,C::Minor} } },   // Eb: Eb Bbadd9 Cm(add9) Gm

            // ------------- PRE-CHORUS (building, ends near dominant) -------------
            { S::PreChorus, M::Romantic, true,  { {0,C::Minor},{5,C::Sus2},{10,C::MajAdd9},{3,C::Major} } }, // Dm: Dm Gsus2 Cadd9 F
            { S::PreChorus, M::Euphoric, false, { {9,C::Minor},{2,C::Minor},{7,C::Sus4},{0,C::Major} } },    // G: Em Am Dsus4 G
            { S::PreChorus, M::Romantic, false, { {0,C::Major},{11,C::Minor},{9,C::Minor},{7,C::MajAdd9} } },// F: F Em Dm Cadd9
            { S::PreChorus, M::Euphoric, false, { {9,C::Minor},{2,C::Minor},{7,C::Sus4},{0,C::Major} } },    // D: Bm Em Asus4 D
            { S::PreChorus, M::Dreamy,   false, { {0,C::Major},{7,C::Sus2},{9,C::Minor},{4,C::Minor} } },    // Bb: Bb Fsus2 Gm Dm
            { S::PreChorus, M::Euphoric, false, { {9,C::Minor},{2,C::Minor},{7,C::Major},{0,C::Major} } },   // A: F#m Bm E A
            { S::PreChorus, M::Romantic, false, { {9,C::Minor},{2,C::Minor},{7,C::Major},{0,C::Major} } },   // E: C#m F#m B E
            { S::PreChorus, M::Euphoric, false, { {8,C::Major},{1,C::Major},{4,C::Major},{0,C::Minor} } },   // C#: A D E C#m

            // ---------------- CHORUS (euphoric peak) ----------------
            { S::Chorus, M::Euphoric, false, { {0,C::MajAdd9},{9,C::MinAdd9},{5,C::Major},{7,C::Sus4} } },   // C: Cadd9 Am(add9) F Gsus4
            { S::Chorus, M::Euphoric, false, { {0,C::Major},{9,C::Minor},{5,C::MajAdd9},{7,C::Sus4} } },     // G: G Em Cadd9 Dsus4
            { S::Chorus, M::Dreamy,   false, { {0,C::Major},{9,C::Minor},{5,C::Major},{7,C::Sus2} } },       // D: D Bm G Asus2
            { S::Chorus, M::Euphoric, false, { {0,C::Major},{9,C::Minor},{5,C::Major},{7,C::MajAdd9} } },    // F: F Dm Bb Cadd9
            { S::Chorus, M::Euphoric, false, { {0,C::Major},{9,C::Minor},{2,C::MinAdd9},{7,C::Sus2} } },     // A: A F#m Bm(add9) Esus2
            { S::Chorus, M::Euphoric, false, { {0,C::Major},{9,C::Minor},{5,C::Major},{7,C::Sus4} } },       // E: E C#m A Bsus4
            { S::Chorus, M::Dreamy,   false, { {0,C::Major},{9,C::Minor},{5,C::Major},{7,C::Sus2} } },       // F#: F# D#m B C#sus2
            { S::Chorus, M::Euphoric, false, { {0,C::Major},{9,C::Minor},{2,C::MajAdd9},{5,C::Major} } },    // D: D Bm Eadd9 G
        };
        return t;
    }

    //==========================================================================
    // Degree-based phrase library. Each entry is a list of scale degrees (0=i/I,
    // 1=ii, 2=III, 3=iv/IV, 4=V, 5=VI, 6=VII). The generator realises these
    // through the chosen MODE (stacked scale-thirds + complexity/mood/sus), so
    // ONE phrase yields a different real progression in every mode -> hundreds of
    // musically-valid combinations. This is the main variety source.
    //==========================================================================
    struct DegreePhrase { Section section; std::vector<int> degrees; };

    inline const std::vector<DegreePhrase>& degreePhrases()
    {
        using S = Section;
        static const std::vector<DegreePhrase> t =
        {
            // ---- Verse (loops, low tension) ----
            // canonical vocal-trance minor idioms first: i-VI-III-VII, i-VII-VI-VII,
            // i-v-VI-III, i-VI-VII-i
            { S::Verse, { 0, 5, 2, 6 } }, { S::Verse, { 0, 6, 5, 6 } }, { S::Verse, { 0, 4, 5, 2 } },
            { S::Verse, { 0, 5, 6, 0 } }, { S::Verse, { 0, 5, 3, 6 } }, { S::Verse, { 0, 3, 5, 4 } },
            { S::Verse, { 0, 6, 5, 4 } }, { S::Verse, { 0, 2, 5, 3 } },
            { S::Verse, { 0, 3, 4, 0 } }, { S::Verse, { 5, 2, 6, 0 } }, { S::Verse, { 0, 6, 2, 5 } },
            { S::Verse, { 0, 1, 4, 0 } }, { S::Verse, { 0, 5, 4, 3 } }, { S::Verse, { 0, 2, 3, 6 } },
            { S::Verse, { 0, 6, 3, 5 } }, { S::Verse, { 5, 3, 0, 4 } }, { S::Verse, { 0, 3, 6, 5 } },
            { S::Verse, { 2, 5, 0, 3 } },

            // ---- Pre-Chorus (build, lands near the dominant) ----
            { S::PreChorus, { 5, 3, 6, 4 } }, { S::PreChorus, { 0, 3, 1, 4 } }, { S::PreChorus, { 5, 6, 3, 4 } },
            { S::PreChorus, { 2, 5, 3, 4 } }, { S::PreChorus, { 0, 5, 1, 4 } }, { S::PreChorus, { 3, 4, 5, 4 } },
            { S::PreChorus, { 1, 4, 5, 4 } }, { S::PreChorus, { 0, 6, 3, 4 } }, { S::PreChorus, { 5, 2, 3, 4 } },
            { S::PreChorus, { 0, 3, 5, 4 } }, { S::PreChorus, { 3, 5, 1, 4 } }, { S::PreChorus, { 0, 1, 5, 4 } },

            // ---- Chorus (euphoric peak) ----
            { S::Chorus, { 0, 5, 2, 6 } }, { S::Chorus, { 0, 6, 5, 6 } }, { S::Chorus, { 0, 4, 5, 2 } },
            { S::Chorus, { 5, 6, 0, 4 } }, { S::Chorus, { 5, 2, 3, 4 } }, { S::Chorus, { 0, 5, 3, 4 } },
            { S::Chorus, { 3, 0, 4, 5 } }, { S::Chorus, { 0, 5, 6, 4 } }, { S::Chorus, { 2, 5, 6, 0 } },
            { S::Chorus, { 5, 6, 4, 0 } }, { S::Chorus, { 0, 3, 6, 4 } }, { S::Chorus, { 5, 3, 0, 4 } },
            { S::Chorus, { 0, 6, 5, 3 } }, { S::Chorus, { 3, 5, 0, 4 } }, { S::Chorus, { 0, 4, 5, 3 } },
            { S::Chorus, { 5, 0, 3, 4 } }, { S::Chorus, { 0, 2, 5, 4 } },

            // ---- Breakdown (stripped, atmospheric, slow harmonic rhythm) ----
            { S::Breakdown, { 0, 5, 3, 5 } }, { S::Breakdown, { 0, 3, 5, 0 } }, { S::Breakdown, { 0, 5, 2, 5 } },
            { S::Breakdown, { 0, 5, 0, 3 } }, { S::Breakdown, { 5, 3, 0, 0 } }, { S::Breakdown, { 0, 5, 3, 0 } },

            // ---- Build-up (intensifying, lands on the dominant) ----
            { S::Buildup, { 0, 3, 5, 4 } }, { S::Buildup, { 5, 3, 6, 4 } }, { S::Buildup, { 0, 5, 6, 4 } },
            { S::Buildup, { 3, 4, 0, 4 } }, { S::Buildup, { 1, 4, 5, 4 } }, { S::Buildup, { 0, 6, 3, 4 } },

            // ---- Drop (max energy, the main hook) ----
            { S::Drop, { 0, 5, 6, 4 } }, { S::Drop, { 5, 6, 0, 4 } }, { S::Drop, { 5, 3, 0, 4 } },
            { S::Drop, { 0, 5, 3, 4 } }, { S::Drop, { 6, 5, 0, 4 } }, { S::Drop, { 0, 6, 5, 4 } },
            { S::Drop, { 5, 6, 0, 2 } }, { S::Drop, { 0, 5, 2, 6 } },

            // ---- Bridge (contrast, departs from the tonic) ----
            { S::Bridge, { 2, 5, 3, 6 } }, { S::Bridge, { 5, 2, 3, 4 } }, { S::Bridge, { 3, 6, 2, 5 } },
            { S::Bridge, { 2, 6, 5, 3 } }, { S::Bridge, { 5, 3, 2, 6 } }, { S::Bridge, { 0, 2, 5, 3 } },
        };
        return t;
    }

    //==========================================================================
    // Style libraries. Same degree-phrase idea, but grouped by sub-genre so the
    // user can ask for a specific flavour. Best in a minor mode (Aeolian) for
    // Oldschool; Uplifting works in Aeolian or a major mode (Ionian/Lydian).
    //==========================================================================
    enum class Style { Any = 0, Uplifting, Oldschool2000, Psytrance, Festival, Progressive, ClassicASOT, NumStyles };

    inline const char* styleName (Style s) noexcept
    {
        switch (s) { case Style::Any: return "Any";
                     case Style::Uplifting: return "Uplifting";
                     case Style::Oldschool2000: return "Oldschool 2000";
                     case Style::Psytrance: return "Psytrance";
                     case Style::Festival: return "Festival";
                     case Style::Progressive: return "Progressive";
                     case Style::ClassicASOT: return "Classic ASOT"; default: return "?"; }
    }

    struct StyledPhrase { Section section; Style style; std::vector<int> degrees; };

    inline const std::vector<StyledPhrase>& styledPhrases()
    {
        using S = Section; using Y = Style;
        static const std::vector<StyledPhrase> t =
        {
            // ===================== UPLIFTING (euphoric, anthemic) =====================
            // verse (gentle emotional builds)
            { S::Verse, Y::Uplifting, { 0, 5, 2, 6 } }, { S::Verse, Y::Uplifting, { 0, 5, 6, 4 } },
            { S::Verse, Y::Uplifting, { 0, 6, 5, 4 } }, { S::Verse, Y::Uplifting, { 5, 2, 6, 0 } },
            { S::Verse, Y::Uplifting, { 0, 3, 4, 5 } }, { S::Verse, Y::Uplifting, { 0, 2, 5, 6 } },
            // pre-chorus (rising toward V)
            { S::PreChorus, Y::Uplifting, { 5, 6, 3, 4 } }, { S::PreChorus, Y::Uplifting, { 0, 5, 3, 4 } },
            { S::PreChorus, Y::Uplifting, { 3, 4, 5, 4 } }, { S::PreChorus, Y::Uplifting, { 5, 2, 3, 4 } },
            { S::PreChorus, Y::Uplifting, { 0, 3, 1, 4 } }, { S::PreChorus, Y::Uplifting, { 5, 3, 6, 4 } },
            // chorus (the big hands-in-the-air progressions)
            { S::Chorus, Y::Uplifting, { 5, 6, 0, 4 } }, { S::Chorus, Y::Uplifting, { 0, 5, 6, 4 } },
            { S::Chorus, Y::Uplifting, { 5, 2, 6, 0 } }, { S::Chorus, Y::Uplifting, { 0, 6, 5, 4 } },
            { S::Chorus, Y::Uplifting, { 5, 3, 0, 4 } }, { S::Chorus, Y::Uplifting, { 3, 4, 5, 0 } },
            { S::Chorus, Y::Uplifting, { 0, 3, 4, 5 } }, { S::Chorus, Y::Uplifting, { 5, 6, 0, 2 } },
            { S::Chorus, Y::Uplifting, { 6, 5, 0, 4 } }, { S::Chorus, Y::Uplifting, { 0, 5, 3, 4 } },
            { S::Chorus, Y::Uplifting, { 5, 0, 6, 4 } }, { S::Chorus, Y::Uplifting, { 0, 6, 3, 4 } },

            // ================= OLDSCHOOL 2000 (driving minor classics) =================
            // verse (the quintessential 4-chord minor loops)
            { S::Verse, Y::Oldschool2000, { 0, 5, 2, 6 } }, { S::Verse, Y::Oldschool2000, { 0, 6, 5, 6 } },
            { S::Verse, Y::Oldschool2000, { 0, 2, 6, 5 } }, { S::Verse, Y::Oldschool2000, { 0, 5, 6, 2 } },
            { S::Verse, Y::Oldschool2000, { 0, 6, 2, 5 } }, { S::Verse, Y::Oldschool2000, { 0, 3, 5, 6 } },
            { S::Verse, Y::Oldschool2000, { 0, 6, 3, 5 } }, { S::Verse, Y::Oldschool2000, { 0, 5, 6, 0 } },
            // pre-chorus
            { S::PreChorus, Y::Oldschool2000, { 5, 6, 0, 4 } }, { S::PreChorus, Y::Oldschool2000, { 0, 6, 3, 4 } },
            { S::PreChorus, Y::Oldschool2000, { 5, 2, 6, 4 } }, { S::PreChorus, Y::Oldschool2000, { 0, 5, 6, 4 } },
            { S::PreChorus, Y::Oldschool2000, { 3, 5, 6, 4 } },
            // chorus (Gouryella / Push / ATB-era riffs)
            { S::Chorus, Y::Oldschool2000, { 0, 5, 2, 6 } }, { S::Chorus, Y::Oldschool2000, { 0, 6, 5, 2 } },
            { S::Chorus, Y::Oldschool2000, { 0, 2, 6, 5 } }, { S::Chorus, Y::Oldschool2000, { 0, 5, 6, 2 } },
            { S::Chorus, Y::Oldschool2000, { 0, 6, 5, 6 } }, { S::Chorus, Y::Oldschool2000, { 5, 6, 0, 2 } },
            { S::Chorus, Y::Oldschool2000, { 2, 6, 0, 5 } }, { S::Chorus, Y::Oldschool2000, { 0, 6, 3, 5 } },
            { S::Chorus, Y::Oldschool2000, { 5, 2, 0, 6 } }, { S::Chorus, Y::Oldschool2000, { 0, 3, 6, 5 } },

            // ---- more Uplifting ----
            { S::Verse, Y::Uplifting, { 0, 3, 5, 6 } }, { S::Verse, Y::Uplifting, { 0, 2, 3, 5 } },
            { S::PreChorus, Y::Uplifting, { 0, 3, 5, 4 } }, { S::PreChorus, Y::Uplifting, { 5, 6, 0, 4 } },
            { S::Chorus, Y::Uplifting, { 5, 3, 6, 4 } }, { S::Chorus, Y::Uplifting, { 0, 3, 5, 4 } },
            { S::Chorus, Y::Uplifting, { 2, 5, 6, 4 } }, { S::Chorus, Y::Uplifting, { 5, 6, 3, 0 } },
            { S::Drop, Y::Uplifting, { 5, 6, 0, 4 } }, { S::Drop, Y::Uplifting, { 0, 5, 6, 4 } },
            { S::Drop, Y::Uplifting, { 5, 3, 0, 4 } },

            // ---- more Oldschool 2000 ----
            { S::Verse, Y::Oldschool2000, { 0, 2, 5, 6 } }, { S::Verse, Y::Oldschool2000, { 0, 6, 5, 3 } },
            { S::PreChorus, Y::Oldschool2000, { 0, 2, 6, 4 } }, { S::PreChorus, Y::Oldschool2000, { 5, 6, 2, 4 } },
            { S::Chorus, Y::Oldschool2000, { 0, 5, 3, 6 } }, { S::Chorus, Y::Oldschool2000, { 6, 0, 5, 2 } },
            { S::Chorus, Y::Oldschool2000, { 0, 6, 2, 5 } }, { S::Drop, Y::Oldschool2000, { 0, 5, 2, 6 } },
            { S::Drop, Y::Oldschool2000, { 0, 6, 5, 6 } }, { S::Breakdown, Y::Oldschool2000, { 0, 5, 3, 5 } },

            // ===================== PSYTRANCE (dark, hypnotic, minimal — best in Phrygian/minor) =====================
            { S::Verse, Y::Psytrance, { 0, 6, 0, 6 } }, { S::Verse, Y::Psytrance, { 0, 1, 0, 6 } },
            { S::Verse, Y::Psytrance, { 0, 6, 5, 6 } }, { S::Verse, Y::Psytrance, { 0, 5, 0, 1 } },
            { S::Chorus, Y::Psytrance, { 0, 6, 5, 6 } }, { S::Chorus, Y::Psytrance, { 0, 1, 6, 5 } },
            { S::Drop, Y::Psytrance, { 0, 6, 0, 5 } }, { S::Drop, Y::Psytrance, { 0, 1, 6, 0 } },
            { S::Breakdown, Y::Psytrance, { 0, 5, 0, 6 } }, { S::Buildup, Y::Psytrance, { 0, 6, 5, 4 } },

            // ===================== FESTIVAL (mainstage, big + simple) =====================
            { S::Verse, Y::Festival, { 0, 5, 2, 6 } }, { S::Verse, Y::Festival, { 0, 5, 6, 4 } },
            { S::PreChorus, Y::Festival, { 5, 6, 0, 4 } }, { S::PreChorus, Y::Festival, { 3, 4, 5, 4 } },
            { S::Chorus, Y::Festival, { 5, 6, 0, 4 } }, { S::Chorus, Y::Festival, { 0, 5, 6, 4 } },
            { S::Chorus, Y::Festival, { 5, 3, 0, 4 } }, { S::Drop, Y::Festival, { 5, 6, 0, 4 } },
            { S::Drop, Y::Festival, { 0, 5, 3, 4 } }, { S::Drop, Y::Festival, { 6, 5, 0, 4 } },

            // ===================== PROGRESSIVE (deep, smooth, sophisticated, fewer changes) =====================
            { S::Verse, Y::Progressive, { 0, 5, 0, 3 } }, { S::Verse, Y::Progressive, { 0, 3, 0, 5 } },
            { S::Verse, Y::Progressive, { 0, 5, 3, 0 } }, { S::PreChorus, Y::Progressive, { 3, 5, 1, 4 } },
            { S::PreChorus, Y::Progressive, { 0, 1, 5, 4 } }, { S::Chorus, Y::Progressive, { 0, 5, 3, 4 } },
            { S::Chorus, Y::Progressive, { 5, 3, 0, 4 } }, { S::Chorus, Y::Progressive, { 0, 3, 5, 4 } },
            { S::Breakdown, Y::Progressive, { 0, 5, 3, 5 } }, { S::Breakdown, Y::Progressive, { 0, 3, 5, 0 } },

            // ===================== CLASSIC ASOT (emotional uplifting, sus-rich) =====================
            { S::Verse, Y::ClassicASOT, { 0, 5, 2, 6 } }, { S::Verse, Y::ClassicASOT, { 0, 6, 5, 4 } },
            { S::PreChorus, Y::ClassicASOT, { 5, 3, 6, 4 } }, { S::PreChorus, Y::ClassicASOT, { 0, 5, 1, 4 } },
            { S::PreChorus, Y::ClassicASOT, { 3, 4, 5, 4 } }, { S::Chorus, Y::ClassicASOT, { 5, 6, 0, 4 } },
            { S::Chorus, Y::ClassicASOT, { 0, 5, 6, 4 } }, { S::Chorus, Y::ClassicASOT, { 5, 2, 6, 0 } },
            { S::Drop, Y::ClassicASOT, { 6, 5, 0, 4 } }, { S::Drop, Y::ClassicASOT, { 5, 6, 3, 4 } },
            { S::Drop, Y::ClassicASOT, { 0, 6, 5, 4 } },
        };
        return t;
    }
}
