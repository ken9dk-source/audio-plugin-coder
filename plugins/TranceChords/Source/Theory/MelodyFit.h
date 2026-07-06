#pragma once
#include "Generator.h"
#include "../Midi/MidiExport.h"

//==============================================================================
// Melody-aware chord refinement. Given a melody (note layer in beats), make each
// generated chord support the melody note(s) sounding under it:
//   1) if the dominant melody note is already a chord tone -> keep it;
//   2) else if it is a consonant in-scale extension (9/11/13/6/7/3) -> add it;
//   3) else substitute the diatonic chord (minimal root move) that contains it;
// Locked chords are never touched. Everything stays in the selected scale.
//==============================================================================
namespace tc
{
    struct MelodyHit { int targetPc = -1; std::vector<int> pcs; bool any = false; };

    // dominant (longest-overlap) melody pitch-class in [s,e), plus all active pcs
    inline MelodyHit melodyInSlot (const NoteLayer& mel, double s, double e)
    {
        MelodyHit h; double bestOverlap = 0.0;
        for (const auto& n : mel)
        {
            const double ns = n.startBeat, ne = n.startBeat + n.lengthBeats;
            const double ov = std::min (e, ne) - std::max (s, ns);
            if (ov > 1.0e-4)
            {
                h.any = true;
                const int pc = ((n.note % 12) + 12) % 12;
                if (std::find (h.pcs.begin(), h.pcs.end(), pc) == h.pcs.end()) h.pcs.push_back (pc);
                if (ov > bestOverlap) { bestOverlap = ov; h.targetPc = pc; }
            }
        }
        return h;
    }

    inline int melodyFitScore (const std::vector<int>& chordPcs, int rootPc, int targetPc)
    {
        if (targetPc < 0) return 50;
        const int rel = (((targetPc - rootPc) % 12) + 12) % 12;
        if (std::find (chordPcs.begin(), chordPcs.end(), targetPc) != chordPcs.end())
        {
            if (rel == 0 || rel == 7)                          return 100; // root / 5th
            if (rel == 3 || rel == 4 || rel == 10 || rel == 11) return 95;  // 3rd / 7th
            return 88;
        }
        if (rel == 2 || rel == 5 || rel == 9) return 55;   // addable 9 / 11 / 13
        return 8;                                          // clash
    }

    inline void applyMelodyFit (Progression& prog, const NoteLayer& mel,
                                int tonicPc, Mode mode, bool preferFlats)
    {
        if (mel.empty()) return;
        const auto pcs = scalePitchClasses (tonicPc, mode);

        for (auto& c : prog)
        {
            if (c.locked) continue;
            const auto hit = melodyInSlot (mel, c.startBeat, c.startBeat + c.lengthBeats);
            if (! hit.any || hit.targetPc < 0) continue;

            const auto cpcs = c.pitchClasses();
            if (std::find (cpcs.begin(), cpcs.end(), hit.targetPc) != cpcs.end()) continue; // already supported

            const int rel = (((hit.targetPc - c.rootPc) % 12) + 12) % 12;
            const bool inScale = scaleContains (pcs, hit.targetPc);

            bool hasMin3 = false;
            for (int x : c.intervals()) if (((x % 12) + 12) % 12 == 3) hasMin3 = true;

            // 1) add the melody note only as a whitelist colour: add9 (maj 9th) or,
            //    on a minor triad, min7. No 11/13/6/maj7 extensions.
            auto addColour = [&] (int interval)
            {
                std::vector<int> iv = c.intervals();
                if (std::find (iv.begin(), iv.end(), interval) == iv.end()) iv.push_back (interval);
                std::sort (iv.begin(), iv.end());
                c.customIntervals = iv;
                c.type  = classifyChord (iv);
                c.label = pcName (c.rootPc, preferFlats) + juce::String (chordSuffix (c.type));
            };
            if (inScale && rel == 2)                { addColour (14); continue; }   // add9
            if (inScale && rel == 10 && hasMin3)    { addColour (10); continue; }   // min7

            // 2) substitute the diatonic whitelist TRIAD (tritone-free, in-scale)
            //    that best contains the melody note
            auto triad = [&] (int d, std::vector<int>& iv)
            {
                const int root = pcs[(size_t) d];
                const int thirdIv = (((pcs[(size_t) ((d + 2) % 7)] - root) % 12) + 12) % 12;
                iv = { 0 };
                if (thirdIv == 3 || thirdIv == 4) iv.push_back (thirdIv);
                if (scaleContains (pcs, root + 7)) iv.push_back (7);
                std::vector<int> dp; for (int x : iv) dp.push_back ((((root + x) % 12) + 12) % 12);
                return dp;
            };
            int bestD = -1, bestScore = -1; std::vector<int> bestIv;
            for (int d = 0; d < 7; ++d)
            {
                std::vector<int> iv; const auto dp = triad (d, iv);
                if (std::find (dp.begin(), dp.end(), hit.targetPc) == dp.end()) continue;
                int sc = melodyFitScore (dp, pcs[(size_t) d], hit.targetPc);
                sc -= std::abs ((((pcs[(size_t) d] - c.rootPc) % 12 + 18) % 12) - 6) * 4; // root-move cost
                if (sc > bestScore) { bestScore = sc; bestD = d; bestIv = iv; }
            }
            if (bestD >= 0)
            {
                c.rootPc = pcs[(size_t) bestD];
                c.customIntervals = bestIv;
                c.type  = classifyChord (bestIv);
                c.label = pcName (c.rootPc, preferFlats) + juce::String (chordSuffix (c.type));
                c.romanDegree = bestD;
                c.roman = romanFor (tonicPc, c);
            }
            // else: leave the chord as generated (melody note becomes a passing tone)
        }
    }
}
