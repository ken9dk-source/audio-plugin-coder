#pragma once
#include "../Midi/MidiExport.h"
#include "../Theory/MelodyFit.h"
#include "Arpeggiator.h"

//==============================================================================
// Counter-melody generator — a complementary melodic line built from chord tones
// in an upper register. Patterns: Top Voice, Outline (cycles chord tones), Pulse.
// When a main melody is loaded it avoids doubling the melody's current pitch class
// on each step (nudges to the next chord tone), so the two lines interlock.
//==============================================================================
namespace tc
{
    enum class CounterPattern { Outline = 0, TopVoice, Pulse, NumPatterns };

    inline const char* counterPatternName (CounterPattern p) noexcept
    {
        switch (p) { case CounterPattern::Outline: return "Outline";
                     case CounterPattern::TopVoice: return "Top Voice";
                     case CounterPattern::Pulse: return "Pulse"; default: return "?"; }
    }

    struct CounterParams
    {
        CounterPattern pattern = CounterPattern::Outline;
        ArpRate rate    = ArpRate::Eighth;
        int   octaveShift = 2;
        float gate        = 0.65f;
        float swing       = 0.0f;  // 0..0.66 groove on off-steps
        int   velocity    = 88;
    };

    inline NoteLayer generateCounter (const std::vector<VoicedChord>& voiced,
                                      const NoteLayer& melody, const CounterParams& cp)
    {
        NoteLayer layer;
        const double step = arpRateBeats (cp.rate);
        const float  gate = juce::jlimit (0.1f, 1.0f, cp.gate);
        const float  swing = juce::jlimit (0.0f, 0.66f, cp.swing);

        for (const auto& vc : voiced)
        {
            if (vc.notes.empty()) continue;

            std::vector<int> tones;
            for (int n : vc.notes) tones.push_back (juce::jlimit (0, 127, n + cp.octaveShift * 12));
            std::sort (tones.begin(), tones.end());
            tones.erase (std::unique (tones.begin(), tones.end()), tones.end());
            if (tones.empty()) continue;

            const int nsteps = (int) std::floor ((vc.lengthBeats + 1.0e-6) / step);
            int idx = 0;
            for (int s = 0; s < nsteps; ++s)
            {
                const double t = vc.startBeat + s * step + ((s % 2 == 1) ? step * swing : 0.0);
                int note;
                switch (cp.pattern)
                {
                    case CounterPattern::TopVoice: note = tones.back(); break;
                    case CounterPattern::Pulse:    note = tones[(size_t) ((s / 2) % (int) tones.size())]; break;
                    case CounterPattern::Outline:
                    default:                       note = tones[(size_t) (idx++ % (int) tones.size())]; break;
                }

                // avoid doubling the melody's pitch class on this step
                if (! melody.empty())
                {
                    const auto hit = melodyInSlot (melody, t, t + step);
                    if (hit.targetPc >= 0 && ((note % 12) + 12) % 12 == hit.targetPc && tones.size() > 1)
                        note = tones[(size_t) ((idx++) % (int) tones.size())];
                }

                layer.push_back ({ t, step * gate, note, cp.velocity });
            }
        }
        return layer;
    }
}
