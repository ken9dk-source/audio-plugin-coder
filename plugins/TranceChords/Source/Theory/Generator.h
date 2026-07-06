#pragma once
#include <random>
#include <array>
#include <vector>
#include <cmath>
#include "Scales.h"
#include "Chord.h"
#include "ProgressionTemplates.h"

//==============================================================================
// TranceChords generation engine. GenParams -> Progression (vector<Chord>).
//
//  * Diatonic foundation: chords are stacked scale-thirds, so every mode
//    (incl. Phrygian/Dorian/harmonic minor) yields correct, in-scale chords.
//  * Section-weighted functional walk (Verse=loops, Pre=builds to dominant,
//    Chorus=euphoric cadences).
//  * Mood/complexity/sus/borrowed/forbid-triads/no-pop/scale-lock controls.
//  * Variation blends curated PDF templates (low) <-> free rule engine (high).
//
// Runs on the message thread (Generate button) — std::vector / juce::String OK.
//==============================================================================
namespace tc
{
    using Progression = std::vector<Chord>;

    // ---- song / arrangement forms (ordered sections, each with a bar count) ----
    struct SongSection { Section type; int bars; };

    inline const char* songFormName (int index) noexcept
    {
        switch (index) { case 0: return "Verse-Build-Drop";
                         case 1: return "Club (Build/Drop x2)";
                         case 2: return "Full Track";
                         case 3: return "Vocal Song";
                         case 4: return "Intro-Build-Drop-Outro"; default: return "?"; }
    }
    inline constexpr int kNumSongForms = 5;

    inline std::vector<SongSection> songForm (int index)
    {
        using S = Section;
        switch (index)
        {
            case 0: return { {S::Verse,8},{S::Buildup,8},{S::Drop,8} };
            case 1: return { {S::Buildup,8},{S::Drop,8},{S::Breakdown,8},{S::Buildup,8},{S::Drop,8} };
            case 2: return { {S::Breakdown,8},{S::Verse,8},{S::PreChorus,8},{S::Drop,8},{S::Breakdown,8},{S::Buildup,8},{S::Drop,8} };
            case 3: return { {S::Verse,8},{S::PreChorus,8},{S::Chorus,8},{S::Verse,8},{S::PreChorus,8},{S::Chorus,8},{S::Bridge,8},{S::Chorus,8} };
            case 4: return { {S::Breakdown,8},{S::Buildup,8},{S::Drop,8},{S::Bridge,8},{S::Drop,8},{S::Breakdown,8} };
            default: return { {S::Verse,8},{S::Buildup,8},{S::Drop,8} };
        }
    }

    struct GenParams
    {
        int   tonicPc      = 0;
        Mode  mode         = Mode::Aeolian;
        Section section     = Section::Verse;
        Mood    mood        = Mood::Dreamy;
        Style   style       = Style::Any;
        int   lengthBars   = 8;
        int   chordsPerBar = 1;      // density: 1 or 2
        float energy       = 0.40f;  // 0..1
        float complexity   = 0.45f;  // 0..1
        float voiceLeading = 0.70f;  // 0..1
        float variation    = 0.35f;  // 0..1
        float humanize     = 0.20f;  // 0..1
        int   octave       = 0;      // -2..+2
        bool  allowSus      = true;
        bool  allowBorrowed = true;
        bool  forbidTriads  = false;
        bool  noPop         = true;
        bool  scaleLock     = true;
        bool  modulation    = false;
        bool  secDominants  = false;
    };

    //========================================================================
    // helpers
    //========================================================================
    inline int wrap12 (int x) noexcept { return ((x % 12) + 12) % 12; }

    inline int weightedPick (const std::array<float, 7>& w, std::mt19937& rng)
    {
        float sum = 0.0f; for (float x : w) sum += std::max (0.0f, x);
        if (sum <= 0.0f) return (int) (rng() % 7);
        std::uniform_real_distribution<float> d (0.0f, sum);
        float r = d (rng);
        for (int i = 0; i < 7; ++i) { r -= std::max (0.0f, w[(size_t) i]); if (r <= 0.0f) return i; }
        return 6;
    }

    // classify an interval set (from root, 0 included) into a ChordType for labelling
    inline ChordType classifyChord (const std::vector<int>& iv)
    {
        bool min3=false,maj3=false,sus2=false,sus4=false,p5=false,b5=false,a5=false;
        bool b7=false,maj7=false,sixth=false,nine=false;
        for (int x : iv)
        {
            if (x == 0) continue;
            if (x == 3) min3 = true; else if (x == 4) maj3 = true;
            else if (x == 2) sus2 = true; else if (x == 5) sus4 = true;
            else if (x == 6) b5 = true; else if (x == 7) p5 = true; else if (x == 8) a5 = true;
            else if (x == 9) sixth = true; else if (x == 10) b7 = true; else if (x == 11) maj7 = true;
            else if (x == 13 || x == 14 || x == 15) nine = true;
        }
        // whitelist-only: jazz colours (maj7, dom7, 6, dim/aug, extensions>9) are
        // never emitted — they are simply ignored in the classification.
        juce::ignoreUnused (b5, a5, maj7, sixth);
        const bool third = min3 || maj3;
        if (! third)
        {
            if (sus4) return ChordType::Sus4;
            if (sus2) return ChordType::Sus2;
            return ChordType::Power5;               // root + 5th only
        }
        if (min3)
        {
            if (nine) return ChordType::MinAdd9;
            if (b7)   return ChordType::Min7;        // the single allowed 7th
            return ChordType::Minor;
        }
        // maj3
        if (nine) return ChordType::MajAdd9;
        return ChordType::Major;
    }

    // stacked scale-thirds chord (scale-accurate, in-mode). numTones 3..5.
    inline std::vector<int> stackScaleChord (const std::array<int,7>& pcs, int degree, int numTones)
    {
        static const int step[5] = { 0, 2, 4, 6, 8 }; // 1,3,5,7,9 in scale steps
        const int rootPc = pcs[(size_t) (degree % 7)];
        std::vector<int> iv; iv.push_back (0);
        int prev = 0;
        for (int k = 1; k < juce::jlimit (3, 5, numTones); ++k)
        {
            const int pc = pcs[(size_t) ((degree + step[k]) % 7)];
            int semi = wrap12 (pc - rootPc);
            while (semi <= prev) semi += 12;
            iv.push_back (semi);
            prev = semi;
        }
        return iv;
    }

    // Rebuild as a *clean* sus chord: root, sus2 (=2) or sus4 (=5), perfect 5th.
    // Only applies to perfect-5th chords whose scale yields a consonant sus tone
    // (so Lydian #4 / Phrygian b2 don't create harsh tritone/semitone "sus").
    // keepSeventh adds the existing b7/maj7 back (-> 7sus4) for cadential dominants.
    // Returns false (chord unchanged) when no clean sus is available.
    inline bool applySus (std::vector<int>& iv, const std::array<int,7>& pcs, int degree, bool preferSus4, bool keepSeventh = false)
    {
        const int rootPc = pcs[(size_t) (degree % 7)];
        bool b7 = false, maj7 = false, hasP5 = false;
        for (int x : iv) { if (x < 13) { if (x == 10) b7 = true; else if (x == 11) maj7 = true; else if (x == 7) hasP5 = true; } }
        if (! hasP5) return false;

        const int sus2i = wrap12 (pcs[(size_t) ((degree + 1) % 7)] - rootPc);
        const int sus4i = wrap12 (pcs[(size_t) ((degree + 3) % 7)] - rootPc);
        int chosen = -1;
        if (preferSus4 && sus4i == 5)       chosen = 5;
        else if (! preferSus4 && sus2i == 2) chosen = 2;
        if (chosen < 0) { if (sus4i == 5) chosen = 5; else if (sus2i == 2) chosen = 2; }
        if (chosen < 0) return false;

        std::vector<int> out { 0, chosen, 7 };
        if (keepSeventh && (b7 || maj7)) out.push_back (b7 ? 10 : 11);
        std::sort (out.begin(), out.end());
        iv = out;
        return true;
    }

    inline juce::String chromaticRoman (int semi, bool minorRoot)
    {
        semi = wrap12 (semi);
        static const char* up[12] = { "I","bII","II","bIII","III","IV","bV","V","bVI","VI","bVII","VII" };
        static const char* lo[12] = { "i","bii","ii","biii","iii","iv","bv","v","bvi","vi","bvii","vii" };
        return juce::String (minorRoot ? lo[semi] : up[semi]);
    }

    inline juce::String romanFor (int tonicPc, const Chord& c)
    {
        const int semi = wrap12 (c.rootPc - tonicPc);
        const bool minorish = isMinorTriadBase (c.type);
        return chromaticRoman (semi, minorish);
    }

    // section/energy-weighted next-degree transition (0..6)
    inline int nextDegree (Section sec, int from, float energy, std::mt19937& rng)
    {
        // base "follow" weights (rows = from, cols = to) — trance-friendly motion
        static const float W[7][7] =
        {
            // to:  i    ii   III  iv   V    VI   VII
            /*i  */ { 0.f, 1.f, 3.f, 3.f, 2.f, 4.f, 3.f },
            /*ii */ { 2.f, 0.f, 1.f, 1.f, 4.f, 2.f, 1.f },
            /*III*/ { 2.f, 1.f, 0.f, 3.f, 2.f, 4.f, 3.f },
            /*iv */ { 2.f, 1.f, 1.f, 0.f, 4.f, 3.f, 3.f },
            /*V  */ { 5.f, 1.f, 1.f, 1.f, 0.f, 3.f, 1.f },
            /*VI */ { 3.f, 2.f, 3.f, 3.f, 2.f, 0.f, 4.f },
            /*VII*/ { 4.f, 1.f, 3.f, 2.f, 1.f, 3.f, 0.f },
        };
        std::array<float, 7> w {};
        for (int t = 0; t < 7; ++t) w[(size_t) t] = W[from][t];

        // section shaping
        if (sec == Section::Verse || sec == Section::Breakdown)
        {
            w[4] *= (sec == Section::Breakdown ? 0.3f : 0.5f);  // little/no dominant
            w[5] *= 1.3f; w[3] *= 1.25f;
            if (sec == Section::Breakdown) w[0] *= 1.4f;        // dwell on the tonic colour
        }
        else if (sec == Section::PreChorus || sec == Section::Buildup)
        {
            w[4] *= (1.6f + energy); w[1] *= 1.4f; w[6] *= 1.2f;  // push to the dominant
        }
        else if (sec == Section::Bridge)
        {
            w[2] *= 1.5f; w[5] *= 1.3f; w[3] *= 1.3f; w[1] *= 1.2f; w[0] *= 0.6f; // depart from tonic
        }
        else /* Chorus / Drop */
        {
            w[0] *= 1.4f; w[4] *= (1.2f + energy); w[5] *= 1.2f; w[3] *= 1.2f;
        }

        return weightedPick (w, rng);
    }

    //========================================================================
    // build a single chord for a degree, applying complexity / mood / toggles
    //========================================================================
    inline Chord buildRuleChord (const GenParams& gp, const std::array<int,7>& pcs,
                                 int degree, bool isCadence, bool preferFlats, std::mt19937& rng)
    {
        std::uniform_real_distribution<float> uni (0.0f, 1.0f);
        const int rootPc = pcs[(size_t) (degree % 7)];

        // --- borrowed chord (chromatic) only when scale-lock is OFF; triads only ---
        if (gp.allowBorrowed && ! gp.scaleLock && uni (rng) < 0.18f + 0.10f * gp.variation)
        {
            const bool minorMode = isMinorMode (gp.mode);
            struct B { int semi; ChordType t; };
            // bVII, bVI (major), iv (minor), bII (Neapolitan) — all whitelist triads
            static const B majBorrows[] = { {10, ChordType::Major}, {8, ChordType::Major}, {5, ChordType::Minor}, {1, ChordType::Major} };
            if (minorMode)
            {
                Chord c = makeChord (gp.tonicPc + 7, ChordType::Major, preferFlats); // V major (harmonic-minor colour)
                c.roman = chromaticRoman (7, false);
                return c;
            }
            const auto& b = majBorrows[rng() % 4];
            Chord c = makeChord (gp.tonicPc + b.semi, b.t, preferFlats);
            c.roman = chromaticRoman (b.semi, b.t == ChordType::Minor);
            return c;
        }

        // --- diatonic triad: tritone-free (perfect 5th or omitted) + fully in-scale ---
        const int thirdIv = wrap12 (pcs[(size_t) ((degree + 2) % 7)] - rootPc); // 3 or 4 (rarely 2/5)
        const bool hasThird = (thirdIv == 3 || thirdIv == 4);
        const bool isMinor3 = (thirdIv == 3);
        const bool perfectFifth = scaleContains (pcs, rootPc + 7); // omit dim/aug 5th -> no tritone

        std::vector<int> iv { 0 };
        if (hasThird)     iv.push_back (thirdIv);
        if (perfectFifth) iv.push_back (7);

        // --- colour by complexity (whitelist only: power-5 / add9 / rare min7) ---
        if (gp.complexity < 0.18f && perfectFifth && ! isCadence && uni (rng) < 0.5f)
        {
            iv = { 0, 7 };                                     // power-5 stab
        }
        else
        {
            const int ninthIv = wrap12 (pcs[(size_t) ((degree + 1) % 7)] - rootPc); // 2 = clean maj 9th
            if (hasThird && ninthIv == 2 && gp.complexity > 0.35f
                && uni (rng) < 0.20f + 0.35f * gp.complexity)
            {
                iv.push_back (14);                             // add9 — the uplifting colour
            }
            else if (isMinor3 && gp.complexity > 0.75f && ! isCadence)
            {
                const int b7 = wrap12 (pcs[(size_t) ((degree + 6) % 7)] - rootPc);
                if (b7 == 10 && uni (rng) < 0.35f) iv.push_back (10); // min7, sparingly
            }
        }
        if (gp.forbidTriads && (int) iv.size() < 4 && hasThird)   // "no triads" -> add9, never a jazz 7th
        {
            const int ninthIv = wrap12 (pcs[(size_t) ((degree + 1) % 7)] - rootPc);
            if (ninthIv == 2) iv.push_back (14);
        }

        // --- sus substitution (pure triadic sus2/sus4, in-scale) ---
        float susChance = 0.0f;
        if (gp.allowSus)
        {
            susChance = (gp.mood == Mood::Dreamy ? 0.20f : gp.mood == Mood::Euphoric ? 0.14f : 0.12f);
            if (isCadence) susChance = 0.35f;
        }
        if (susChance > 0.0f && uni (rng) < susChance)
        {
            const bool sus4 = isCadence ? true : (uni (rng) < (isMinor3 ? 0.40f : 0.60f));
            applySus (iv, pcs, degree, sus4, false);          // false = no 7th -> plain sus
        }

        std::sort (iv.begin(), iv.end());
        iv.erase (std::unique (iv.begin(), iv.end()), iv.end());

        Chord c;
        c.rootPc = rootPc;
        c.customIntervals = iv;
        c.type  = classifyChord (iv);
        c.label = pcName (rootPc, preferFlats) + juce::String (chordSuffix (c.type));
        c.romanDegree = degree;
        c.roman = romanFor (gp.tonicPc, c);
        return c;
    }

    //========================================================================
    // template path: pick a curated PDF progression and transpose it
    //========================================================================
    inline std::vector<Chord> buildTemplateCell (const GenParams& gp, bool preferFlats, std::mt19937& rng)
    {
        const auto& all = progressionTemplates();
        std::vector<int> matches, sectionOnly;
        for (int i = 0; i < (int) all.size(); ++i)
        {
            if (all[(size_t) i].section != gp.section) continue;
            sectionOnly.push_back (i);
            if (all[(size_t) i].mood == gp.mood) matches.push_back (i);
        }
        const std::vector<int>& pool = ! matches.empty() ? matches : sectionOnly;
        int idx = pool.front();
        if (gp.variation > 0.001f && ! pool.empty())
            idx = pool[(size_t) (rng() % pool.size())];

        const auto& tpl = all[(size_t) idx];
        const auto pcs = scalePitchClasses (gp.tonicPc, gp.mode);
        std::vector<Chord> cell;
        for (const auto& tc : tpl.chords)
        {
            Chord c = makeChord (gp.tonicPc + tc.semi, tc.type, preferFlats);
            // forbid-triads: upgrade bare triads to 7ths
            if (gp.forbidTriads && (c.type == ChordType::Major || c.type == ChordType::Minor))
                c = makeChord (gp.tonicPc + tc.semi, c.type == ChordType::Major ? ChordType::MajAdd9 : ChordType::MinAdd9, preferFlats);
            // degree / roman if the root is diatonic
            const int rp = wrap12 (gp.tonicPc + tc.semi);
            c.romanDegree = -1;
            for (int d = 0; d < 7; ++d) if (pcs[(size_t) d] == rp) { c.romanDegree = d; break; }
            c.roman = romanFor (gp.tonicPc, c);
            cell.push_back (c);
        }
        return cell;
    }

    inline bool isPopRotation (const std::vector<Chord>& cell)
    {
        if (cell.size() != 4) return false;
        // Only the literal I-V-vi-IV (starting on the tonic) is the pop cliche.
        // Trance-idiomatic rotations (vi-IV-I-V, IV-I-V-vi) are explicitly endorsed
        // by the source PDF for choruses, so we keep them.
        static const int pop[4] = { 0, 4, 5, 3 };
        for (int i = 0; i < 4; ++i) if (cell[(size_t) i].romanDegree != pop[i]) return false;
        return true;
    }

    // same chord roots + types (ignores locks/positions) — for non-repeat checks
    inline bool sameChords (const std::vector<Chord>& a, const std::vector<Chord>& b)
    {
        if (a.empty() || a.size() != b.size()) return false;
        for (size_t i = 0; i < a.size(); ++i)
            if (a[(size_t) i].rootPc != b[(size_t) i].rootPc || a[(size_t) i].type != b[(size_t) i].type) return false;
        return true;
    }

    inline void rotateDegrees (std::vector<int>& d, int by)
    {
        if (d.size() < 2) return;
        by = ((by % (int) d.size()) + (int) d.size()) % (int) d.size();
        std::rotate (d.begin(), d.begin() + by, d.end());
    }

    // realise scale degrees through the current mode (scale-accurate + decoration)
    inline std::vector<Chord> realizeDegrees (const GenParams& gp, const std::array<int,7>& pcs,
                                              std::vector<int> degrees, bool preferFlats, std::mt19937& rng)
    {
        std::uniform_real_distribution<float> u (0.0f, 1.0f);
        if (gp.section == Section::Verse && degrees.size() > 1 && u (rng) < 0.30f + 0.45f * gp.variation)
            rotateDegrees (degrees, 1 + (int) (rng() % (degrees.size() - 1)));

        std::vector<Chord> cell;
        for (int i = 0; i < (int) degrees.size(); ++i)
        {
            const bool cad = (i == (int) degrees.size() - 1);
            cell.push_back (buildRuleChord (gp, pcs, ((degrees[(size_t) i] % 7) + 7) % 7, cad, preferFlats, rng));
        }
        return cell;
    }

    // gather candidate degree-lists for {section, style}. Style::Any = everything
    // (generic + all styled); a specific style = that style's phrases (falls back
    // to the generic pool if a section happens to have none).
    inline std::vector<std::vector<int>> phrasePool (Section sec, Style style)
    {
        std::vector<std::vector<int>> pool;
        for (const auto& p : styledPhrases())
            if (p.section == sec && (style == Style::Any || p.style == style)) pool.push_back (p.degrees);
        if (style == Style::Any)
            for (const auto& p : degreePhrases()) if (p.section == sec) pool.push_back (p.degrees);
        if (pool.empty())
            for (const auto& p : degreePhrases()) if (p.section == sec) pool.push_back (p.degrees);
        return pool;
    }

    // pick a degree phrase for the section/style and realise it (huge mode-aware variety)
    inline std::vector<Chord> buildDegreeCell (const GenParams& gp, const std::array<int,7>& pcs,
                                               bool preferFlats, std::mt19937& rng)
    {
        const auto pool = phrasePool (gp.section, gp.style);
        if (pool.empty())
        {
            std::vector<Chord> cell; int cur = (rng() % 100 < 75) ? 0 : 5;
            for (int i = 0; i < 4; ++i) { cell.push_back (buildRuleChord (gp, pcs, cur, i == 3, preferFlats, rng)); cur = nextDegree (gp.section, cur, gp.energy, rng); }
            return cell;
        }
        const auto& degrees = pool[(size_t) (rng() % pool.size())];
        return realizeDegrees (gp, pcs, degrees, preferFlats, rng);
    }

    // free functional walk (highest Variation)
    inline std::vector<Chord> buildRuleCell (const GenParams& gp, const std::array<int,7>& pcs,
                                             bool preferFlats, std::mt19937& rng, int cellLen)
    {
        std::vector<Chord> cell; int cur = (rng() % 100 < 75) ? 0 : 5;
        for (int i = 0; i < cellLen; ++i)
        {
            const bool cad = (i == cellLen - 1);
            cell.push_back (buildRuleChord (gp, pcs, cur, cad, preferFlats, rng));
            cur = nextDegree (gp.section, cur, gp.energy, rng);
        }
        return cell;
    }

    // one cell; source chosen by Variation:
    //   < 0.20  signature PDF templates   |  0.20-0.82  mode-aware degree phrases  |  >= 0.82  free walk
    inline std::vector<Chord> buildCell (const GenParams& gp, const std::array<int,7>& pcs,
                                         bool preferFlats, std::mt19937& rng, int cellLen)
    {
        std::vector<Chord> cell;
        if (gp.variation < 0.20f)      cell = buildTemplateCell (gp, preferFlats, rng);
        else if (gp.variation < 0.82f) cell = buildDegreeCell (gp, pcs, preferFlats, rng);
        else                           cell = buildRuleCell (gp, pcs, preferFlats, rng, cellLen);

        if ((int) cell.size() > cellLen) cell.resize ((size_t) cellLen);
        while ((int) cell.size() < cellLen && ! cell.empty()) cell.push_back (cell.back());
        return cell;
    }

    // Secondary dominants intentionally REMOVED: dominant-7 colour chords are not
    // part of the vocal-/uplifting-trance vocabulary. Kept as a no-op so the
    // (now inert) sec_dominants parameter and existing call sites still link.
    inline void applySecondaryDominants (Progression&, const GenParams&, std::mt19937&) {}

    // Picardy third: brighten the final minor tonic of a chorus/drop to major
    inline void applyPicardy (Progression& prog, const GenParams& gp, std::mt19937& rng)
    {
        if (prog.empty() || gp.scaleLock || ! isMinorMode (gp.mode)) return; // chromatic: needs scale-lock off
        if (gp.section != Section::Chorus && gp.section != Section::Drop) return;
        Chord& last = prog.back();
        if (last.locked || wrap12 (last.rootPc - gp.tonicPc) != 0) return;
        std::uniform_real_distribution<float> u (0.0f, 1.0f);
        if (u (rng) >= (gp.mood == Mood::Euphoric ? 0.5f : 0.22f)) return;
        last = makeChord (last.rootPc, ChordType::Major, keyPrefersFlats (gp.tonicPc, gp.mode));
        last.roman = "I";
    }

    //========================================================================
    // main entry
    //========================================================================
    inline Progression generate (const GenParams& gp, uint32_t seed, const Progression& previous = {})
    {
        std::mt19937 rng (seed);
        const bool preferFlats = keyPrefersFlats (gp.tonicPc, gp.mode);
        const auto pcs = scalePitchClasses (gp.tonicPc, gp.mode);

        // section-aware harmonic rhythm: Breakdown = slow/long chords, Build-up = denser
        int    effDensity = gp.chordsPerBar;
        double lenScale   = 1.0;
        if (gp.section == Section::Breakdown)    { effDensity = 1; lenScale = 2.0; }
        else if (gp.section == Section::Buildup)   effDensity = juce::jmax (gp.chordsPerBar, 2);
        const double chordBeats = (4.0 / (double) juce::jmax (1, effDensity)) * lenScale;
        const int numChords = juce::jlimit (1, 64, (int) std::llround (gp.lengthBars * 4.0 / chordBeats));
        const int cellLen   = juce::jlimit (1, 4, numChords < 4 ? numChords : 4);

        // ---- build distinct cells, arranged A B (A C ...) for variety, and retry
        //      the whole build until it differs from the previous generation ----
        const int numCells = (numChords + cellLen - 1) / cellLen;
        const int distinct = (numChords <= cellLen)
                           ? 1
                           : juce::jlimit (2, numCells, 2 + (int) (gp.variation * 2.5f));

        Progression prog;
        for (int outer = 0; outer < 4; ++outer)
        {
            std::vector<std::vector<Chord>> cells;
            for (int c = 0; c < distinct; ++c)
            {
                std::vector<Chord> cell;
                for (int att = 0; att < 8; ++att)
                {
                    cell = buildCell (gp, pcs, preferFlats, rng, cellLen);
                    if (gp.noPop && isPopRotation (cell)) continue;           // reject I-V-vi-IV cliche
                    if (c > 0 && sameChords (cell, cells.back())) continue;   // make cells differ
                    break;
                }
                cells.push_back (cell);
            }

            prog.clear();
            for (int i = 0; i < numChords; ++i)
                prog.push_back (cells[(size_t) ((i / cellLen) % distinct)][(size_t) (i % cellLen)]);

            if (! sameChords (prog, previous)) break;   // each Generate must change the chords
        }

        // ---- section-specific ending ----
        if ((gp.section == Section::PreChorus || gp.section == Section::Buildup) && ! prog.empty())
        {
            // end on the V (scale-safe, whitelist; a Vsus resolution is idiomatic)
            Chord v = buildRuleChord (gp, pcs, 4, true, preferFlats, rng);
            v.romanDegree = 4; v.roman = "V";
            prog.back() = v;
        }
        if ((gp.section == Section::Chorus || gp.section == Section::Drop) && gp.modulation && numChords >= 4)
        {
            // lift the final cell up a tone (classic vocal-trance key change)
            const int lift = (rng() % 2 == 0) ? 1 : 2;
            for (int i = numChords - cellLen; i < numChords; ++i)
            {
                if (i < 0) continue;
                Chord& c = prog[(size_t) i];
                c.rootPc = wrap12 (c.rootPc + lift);
                if (c.bassPc >= 0) c.bassPc = wrap12 (c.bassPc + lift);
                c.label = pcName (c.rootPc, preferFlats) + juce::String (chordSuffix (c.type));
                c.roman += "^"; // marks the lifted segment
            }
        }

        applySecondaryDominants (prog, gp, rng);
        applyPicardy (prog, gp, rng);

        // ---- positions + locks ---- (chordBeats computed above, section-aware)
        for (int i = 0; i < numChords; ++i)
        {
            prog[(size_t) i].startBeat   = i * chordBeats;
            prog[(size_t) i].lengthBeats = chordBeats;
            // preserve locked chords from the previous progression (by index)
            if (i < (int) previous.size() && previous[(size_t) i].locked)
            {
                Chord keep = previous[(size_t) i];
                keep.startBeat   = prog[(size_t) i].startBeat;
                keep.lengthBeats = prog[(size_t) i].lengthBeats;
                prog[(size_t) i] = keep;
            }
        }
        return prog;
    }
}
