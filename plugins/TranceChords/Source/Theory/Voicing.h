#pragma once
#include <vector>
#include <algorithm>
#include <numeric>
#include "Chord.h"

//==============================================================================
// Voicing: Chord -> absolute MIDI notes. Root/bass placed low, upper tones in a
// mid register as an ascending spread; with voice-leading strictness + the
// previous chord's notes it anchors the upper voices near the previous register
// to keep motion small (common-tone feel). Caps sounding voices (drops 5th, then
// extensions) to avoid mud, per the spec (<=4-5 voices).
//==============================================================================
namespace tc
{
    enum class VoicingStyle { Close = 0, Open, Drop2, Wide, NumStyles };

    struct VoicingConfig
    {
        int   bassCenter  = 40;    // ~E2: bass note placed near here
        int   upperAnchor = 59;    // ~B3: lowest upper voice starts near here
        float strictness  = 0.7f;  // 0 free .. 1 strict common-tone
        int   maxVoices   = 5;     // total sounding notes incl. bass
        int   octaveShift = 0;     // +/- semitones (12 * octave param)
        VoicingStyle style = VoicingStyle::Open;
    };

    // octave-only re-spacing of an ascending note set (keeps pitch classes -> scale safe)
    inline void applyVoicingStyle (std::vector<int>& notes, VoicingStyle style)
    {
        if (notes.size() < 3) return;
        switch (style)
        {
            case VoicingStyle::Close:
            {
                const int ref = notes[1]; // lowest upper voice
                for (size_t i = 2; i < notes.size(); ++i)
                    while (notes[i] - ref >= 12) notes[i] -= 12;
                break;
            }
            case VoicingStyle::Drop2: notes[notes.size() - 2] -= 12; break;
            case VoicingStyle::Wide:  notes[notes.size() - 1] += 12; break;
            case VoicingStyle::Open: default: break;
        }
        for (auto& n : notes) n = juce::jlimit (24, 96, n);
        std::sort (notes.begin(), notes.end());
        notes.erase (std::unique (notes.begin(), notes.end()), notes.end());
    }

    // nearest MIDI note of pitch-class pc to 'target'
    inline int placePcNear (int pc, int target) noexcept
    {
        pc = ((pc % 12) + 12) % 12;
        int base = target - ((target % 12 + 12) % 12); // octave floor
        int best = base + pc, bestDist = std::abs (best - target);
        for (int o : { -24, -12, 12, 24 })
        {
            int c = base + pc + o, d = std::abs (c - target);
            if (d < bestDist) { bestDist = d; best = c; }
        }
        return best;
    }

    // smallest MIDI note of pitch-class pc strictly greater than 'above'
    inline int placePcAbove (int pc, int above) noexcept
    {
        pc = ((pc % 12) + 12) % 12;
        int n = above + 1;
        n += ((pc - (n % 12)) % 12 + 12) % 12;
        return n;
    }

    // intervals (excluding root) chosen by musical priority: 3rd/sus, 7th/6th,
    // extensions, 5th last. Returns at most 'count' intervals (from the root).
    inline std::vector<int> selectUpperIntervals (const std::vector<int>& all, int count)
    {
        std::vector<int> thirds, sevenths, exts, fifths;
        for (int iv : all)
        {
            if (iv == 0) continue;
            if (iv >= 13) { exts.push_back (iv); continue; }
            int m = ((iv % 12) + 12) % 12;
            if      (m == 2 || m == 3 || m == 4 || m == 5) thirds.push_back (iv);
            else if (m == 9 || m == 10 || m == 11)         sevenths.push_back (iv);
            else if (m == 6 || m == 7 || m == 8)           fifths.push_back (iv);
            else                                           exts.push_back (iv);
        }
        std::vector<int> pri;
        auto add = [&pri] (const std::vector<int>& v) { for (int x : v) pri.push_back (x); };
        add (thirds); add (sevenths); add (exts); add (fifths);
        if ((int) pri.size() > count) pri.resize ((size_t) std::max (0, count));
        return pri;
    }

    inline std::vector<int> voiceChord (const Chord& c,
                                        const std::vector<int>& prevNotes,
                                        const VoicingConfig& cfg)
    {
        const int shift   = cfg.octaveShift;
        const int bassPc  = (c.bassPc >= 0 ? c.bassPc : c.rootPc);

        std::vector<int> notes;
        notes.push_back (placePcNear (bassPc, cfg.bassCenter + shift));

        // choose upper anchor: strict + previous chord => sit near previous upper centroid
        int anchor = cfg.upperAnchor + shift;
        if (cfg.strictness > 0.5f && prevNotes.size() > 1)
        {
            // average of previous upper voices (skip the bass = lowest)
            std::vector<int> up (prevNotes.begin() + 1, prevNotes.end());
            if (! up.empty())
            {
                int avg = std::accumulate (up.begin(), up.end(), 0) / (int) up.size();
                // blend toward the default anchor by (1 - strictness)
                anchor = (int) std::round (cfg.strictness * avg + (1.0f - cfg.strictness) * (cfg.upperAnchor + shift));
            }
        }
        anchor = juce::jlimit (50 + shift, 74 + shift, anchor);

        auto upper = selectUpperIntervals (c.intervals(), cfg.maxVoices - 1);
        std::sort (upper.begin(), upper.end()); // ascending stack

        int prevNote = anchor - 2;
        for (int iv : upper)
        {
            const int pc = ((c.rootPc + iv) % 12 + 12) % 12;
            int n = placePcAbove (pc, prevNote);
            // keep the very first upper voice close to the anchor (not far above)
            if (n > anchor + 13 && prevNote < anchor) n -= 12;
            n = juce::jlimit (36, 96, n);
            if (n <= prevNote) n = prevNote + 1;
            notes.push_back (n);
            prevNote = n;
        }

        std::sort (notes.begin(), notes.end());
        notes.erase (std::unique (notes.begin(), notes.end()), notes.end());
        applyVoicingStyle (notes, cfg.style);
        return notes;
    }
}
