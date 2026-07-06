#pragma once
#include <juce_audio_basics/juce_audio_basics.h>
#include "../Theory/Generator.h"
#include "../Theory/Voicing.h"

//==============================================================================
// MIDI layer model + export. A "NoteLayer" is the common currency for every
// performance layer (chords / bass / arp / melody…): a flat list of timed notes.
// One builder turns any layer into a .mid file (shared by drag-out, host MIDI
// output and offline render) so there is a single, tested code path.
//==============================================================================
namespace tc
{
    struct VoicedChord
    {
        double startBeat = 0.0;
        double lengthBeats = 4.0;
        std::vector<int> notes;   // absolute MIDI note numbers, ascending
    };

    struct TimedNote
    {
        double startBeat = 0.0;
        double lengthBeats = 1.0;
        int    note = 60;
        int    vel  = 96;
    };
    using NoteLayer = std::vector<TimedNote>;

    // voice-lead the whole progression in one pass (each chord anchored to the
    // previous chord's register when strictness is high).
    inline std::vector<VoicedChord> voiceProgression (const Progression& prog, const VoicingConfig& cfg)
    {
        std::vector<VoicedChord> out;
        std::vector<int> prevNotes;
        for (const auto& ch : prog)
        {
            VoicedChord vc;
            vc.startBeat   = ch.startBeat;
            vc.lengthBeats = ch.lengthBeats;
            vc.notes       = voiceChord (ch, prevNotes, cfg);
            prevNotes      = vc.notes;
            out.push_back (std::move (vc));
        }
        return out;
    }

    inline double progressionLengthBeats (const Progression& prog)
    {
        double end = 0.0;
        for (const auto& c : prog) end = std::max (end, c.startBeat + c.lengthBeats);
        return end > 0.0 ? end : 4.0;
    }

    // flatten voiced chords into a note layer (top voice a touch louder)
    inline NoteLayer chordsToLayer (const std::vector<VoicedChord>& voiced)
    {
        NoteLayer layer;
        for (const auto& vc : voiced)
            for (int i = 0; i < (int) vc.notes.size(); ++i)
                layer.push_back ({ vc.startBeat, vc.lengthBeats, vc.notes[(size_t) i],
                                   92 + (i == (int) vc.notes.size() - 1 ? 6 : 0) });
        return layer;
    }

    // Build a single-track MIDI file (960 PPQ) from any note layer.
    inline juce::MidiFile buildMidiFileFromLayer (const NoteLayer& layer, double bpm,
                                                  float humanize = 0.0f, int channel = 1)
    {
        const int ppq = 960;
        juce::MidiFile mf;
        mf.setTicksPerQuarterNote (ppq);

        juce::MidiMessageSequence track;
        track.addEvent (juce::MidiMessage::tempoMetaEvent ((int) std::round (60000000.0 / juce::jmax (1.0, bpm))));
        track.addEvent (juce::MidiMessage::textMetaEvent (3, "TranceChords"));

        juce::Random rnd (0x7C40D5);
        for (const auto& tn : layer)
        {
            const double onTick  = tn.startBeat * ppq;
            const double offTick = (tn.startBeat + tn.lengthBeats) * ppq - 2.0; // tiny gap
            int vel = tn.vel;
            if (humanize > 0.0f) vel += (int) std::round ((rnd.nextFloat() * 2.0f - 1.0f) * 12.0f * humanize);
            vel = juce::jlimit (1, 127, vel);
            const double jitter = humanize > 0.0f ? (rnd.nextFloat() * 2.0f - 1.0f) * 18.0f * humanize : 0.0;
            track.addEvent (juce::MidiMessage::noteOn  (channel, tn.note, (juce::uint8) vel), std::max (0.0, onTick + jitter));
            track.addEvent (juce::MidiMessage::noteOff (channel, tn.note),                    std::max (onTick + 1.0, offTick));
        }
        track.updateMatchedPairs();
        track.sort();
        mf.addTrack (track);
        return mf;
    }

    // chords convenience (kept for the render harness)
    inline juce::MidiFile buildMidiFile (const std::vector<VoicedChord>& voiced, double bpm, float humanize = 0.0f)
    {
        return buildMidiFileFromLayer (chordsToLayer (voiced), bpm, humanize);
    }

    // Write any layer to a uniquely-named temp .mid and return the file (for drag-out).
    inline juce::File writeTempMidiLayer (const NoteLayer& layer, double bpm, float humanize, const juce::String& tag)
    {
        const auto mf = buildMidiFileFromLayer (layer, bpm, humanize);
        juce::String name = "TranceChords_" + (tag.isNotEmpty() ? tag : juce::String ("layer"));
        name = juce::File::createLegalFileName (name) + "_" + juce::String (juce::Time::getCurrentTime().toMilliseconds()) + ".mid";

        auto file = juce::File::getSpecialLocation (juce::File::tempDirectory).getChildFile (name);
        file.deleteFile();
        if (auto os = std::unique_ptr<juce::FileOutputStream> (file.createOutputStream().release()))
        {
            mf.writeTo (*os);
            os->flush();
        }
        return file;
    }

    // chords convenience
    inline juce::File writeTempMidi (const Progression& prog, const VoicingConfig& cfg,
                                     double bpm, float humanize, const juce::String& tag)
    {
        return writeTempMidiLayer (chordsToLayer (voiceProgression (prog, cfg)), bpm, humanize, tag);
    }

    // Parse a dropped .mid file into a note layer (beats). Picks the track with the
    // most notes (the melody) and converts ticks -> beats via the file's PPQ.
    inline NoteLayer parseMelodyFile (const juce::File& f)
    {
        NoteLayer out;
        juce::FileInputStream in (f);
        if (! in.openedOk()) return out;

        juce::MidiFile mf;
        if (! mf.readFrom (in)) return out;

        const int tf = mf.getTimeFormat();
        const double ticksPerQuarter = tf > 0 ? (double) tf : 480.0; // PPQ; SMPTE approximated

        int bestTrack = -1, bestNotes = 0;
        for (int t = 0; t < mf.getNumTracks(); ++t)
        {
            int n = 0;
            const auto* seq = mf.getTrack (t);
            for (int i = 0; i < seq->getNumEvents(); ++i)
                if (seq->getEventPointer (i)->message.isNoteOn()) ++n;
            if (n > bestNotes) { bestNotes = n; bestTrack = t; }
        }
        if (bestTrack < 0) return out;

        juce::MidiMessageSequence s (*mf.getTrack (bestTrack));
        s.updateMatchedPairs();
        for (int i = 0; i < s.getNumEvents(); ++i)
        {
            const auto* e = s.getEventPointer (i);
            if (! e->message.isNoteOn()) continue;
            const double onTick  = e->message.getTimeStamp();
            const double offTick = e->noteOffObject != nullptr ? e->noteOffObject->message.getTimeStamp()
                                                               : onTick + ticksPerQuarter;
            const double startBeat = onTick / ticksPerQuarter;
            const double lenBeat   = juce::jmax (0.05, (offTick - onTick) / ticksPerQuarter);
            out.push_back ({ startBeat, lenBeat, e->message.getNoteNumber(), e->message.getVelocity() });
        }
        std::sort (out.begin(), out.end(), [] (const TimedNote& a, const TimedNote& b) { return a.startBeat < b.startBeat; });
        return out;
    }
}
