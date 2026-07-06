// TranceChords offline harness — proves the generator produces musical, in-scale
// progressions across modes (incl. Phrygian/Dorian/harmonic minor), that MIDI
// export works, and that the preview pad makes sound. Reuses engine headers only.
#include "Theory/Generator.h"
#include "Theory/MelodyFit.h"
#include "Midi/MidiExport.h"
#include "Layers/Bassline.h"
#include "Layers/Arpeggiator.h"
#include "Layers/CounterMelody.h"
#include "Synth/PadSynth.h"
#include "Synth/SimpleSynth.h"
#include "Synth/Pump.h"
#include <juce_audio_formats/juce_audio_formats.h>
#include <juce_core/juce_core.h>
#include <cstdio>
#include <cmath>

static const char* noteNames[12] = { "C","C#","D","D#","E","F","F#","G","G#","A","A#","B" };

int main()
{
    using namespace tc;

    // ---------------- 1. musicality + scale-lock across modes ----------------
    struct Case { int tonic; Mode mode; Section sec; Mood mood; Style style; };
    const Case cases[] = {
        { 0, Mode::Ionian,        Section::Verse,     Mood::Dreamy,   Style::Any           },
        { 9, Mode::Aeolian,       Section::Verse,     Mood::Romantic, Style::Any           },
        { 4, Mode::Phrygian,      Section::Verse,     Mood::Romantic, Style::Any           },
        { 2, Mode::Dorian,        Section::PreChorus, Mood::Euphoric, Style::Any           },
        { 7, Mode::HarmonicMinor, Section::Chorus,    Mood::Euphoric, Style::Any           },
        { 6, Mode::Lydian,        Section::Chorus,    Mood::Dreamy,   Style::Any           },
        { 9, Mode::Aeolian,       Section::Chorus,    Mood::Euphoric, Style::Uplifting     },
        { 9, Mode::Aeolian,       Section::Chorus,    Mood::Euphoric, Style::Oldschool2000 },
        { 9, Mode::Aeolian,       Section::Breakdown, Mood::Dreamy,   Style::Any           },
        { 9, Mode::Aeolian,       Section::Buildup,   Mood::Euphoric, Style::Any           },
        { 9, Mode::Aeolian,       Section::Drop,      Mood::Euphoric, Style::Uplifting     },
        { 0, Mode::Ionian,        Section::Bridge,    Mood::Romantic, Style::Any           },
        { 4, Mode::Phrygian,      Section::Drop,      Mood::Euphoric, Style::Psytrance     },
        { 9, Mode::Dorian,        Section::Verse,     Mood::Romantic, Style::Progressive   },
        { 11, Mode::Aeolian,      Section::Drop,      Mood::Euphoric, Style::Festival      },
        { 9, Mode::Aeolian,       Section::Chorus,    Mood::Euphoric, Style::ClassicASOT   },
    };

    bool scaleOk = true;
    bool varietyOk = false;
    bool layersOk = false;
    bool melodyOk = false;
    bool songOk = false;
    bool synthsOk = false;
    bool depthOk = false;
    bool vocabOk = false;
    bool polishOk = false;
    int  totalChords = 0;
    int  caseIdx = 0;
    for (const auto& c : cases)
    {
        GenParams gp;
        gp.tonicPc = c.tonic; gp.mode = c.mode; gp.section = c.sec; gp.mood = c.mood; gp.style = c.style;
        gp.lengthBars = 8; gp.chordsPerBar = 1; gp.complexity = 0.62f;
        gp.variation = 0.55f; gp.scaleLock = true; gp.allowBorrowed = false; // mode-aware degree phrases

        const uint32_t seed = 0xC0FFEEu + (uint32_t) (caseIdx++) * 2654435761u; // per-case seed
        const auto prog = generate (gp, seed);
        const auto pcs  = scalePitchClasses (c.tonic, c.mode);

        std::printf ("%-2s %-13s / %-10s / %-14s : ", noteNames[c.tonic], modeName (c.mode),
                     sectionName (c.sec), styleName (c.style));
        for (const auto& ch : prog)
        {
            std::printf ("%s ", ch.label.toRawUTF8());
            for (int iv : ch.intervals())
                if (! scaleContains (pcs, ch.rootPc + iv)) scaleOk = false;
            ++totalChords;
        }
        std::printf ("\n");
    }
    std::printf ("SCALE-LOCK: %s  (%d chords checked in-scale)\n", scaleOk ? "PASS" : "FAIL", totalChords);

    // ---- variety: same params, different seeds (+ non-repeat) must differ ----
    {
        GenParams g; g.tonicPc = 9; g.mode = Mode::Aeolian; g.section = Section::Verse;
        g.mood = Mood::Romantic; g.lengthBars = 8; g.variation = 0.55f;
        std::vector<Progression> gens; int distinct = 0; Progression prev;
        for (int s = 0; s < 8; ++s)
        {
            auto pr = generate (g, 100u + (uint32_t) s * 1234567u, prev);
            bool dup = false; for (auto& e : gens) if (sameChords (pr, e)) { dup = true; break; }
            if (! dup) ++distinct;
            gens.push_back (pr); prev = pr;
        }
        varietyOk = distinct >= 6;
        std::printf ("VARIETY: %s  %d/8 distinct progressions (same params, consecutive Generate clicks)\n",
                     varietyOk ? "PASS" : "FAIL", distinct);
    }

    // borrowed / modulation demo (scale-lock off) — printed, not scale-checked
    {
        GenParams gp; gp.tonicPc = 9; gp.mode = Mode::Aeolian; gp.section = Section::Chorus;
        gp.mood = Mood::Euphoric; gp.lengthBars = 8; gp.scaleLock = false; gp.allowBorrowed = true;
        gp.modulation = true; gp.variation = 0.9f; gp.energy = 0.8f;
        const auto prog = generate (gp, 0xBEEF);
        std::printf ("BORROWED+MOD (A Aeolian Chorus): ");
        for (const auto& ch : prog) std::printf ("%s ", ch.label.toRawUTF8());
        std::printf ("\n");
    }

    // ---------------- 2. MIDI export ----------------
    GenParams gp; gp.tonicPc = 0; gp.mode = Mode::Ionian; gp.section = Section::Chorus;
    gp.mood = Mood::Euphoric; gp.lengthBars = 8; gp.chordsPerBar = 1; gp.complexity = 0.6f;
    const auto prog   = generate (gp, 0x12345);
    VoicingConfig vc; vc.strictness = 0.7f;
    const auto voiced = voiceProgression (prog, vc);

    int expectedNotes = 0;
    for (const auto& v : voiced) expectedNotes += (int) v.notes.size();

    const auto midiFile = writeTempMidi (prog, vc, 124.0, 0.0f, "harness");
    const bool midiOk = midiFile.existsAsFile() && midiFile.getSize() > 0 && expectedNotes > 0;
    std::printf ("MIDI: %s  file=%s  size=%lld  voicedNotes=%d\n", midiOk ? "PASS" : "FAIL",
                 midiFile.getFileName().toRawUTF8(), (long long) midiFile.getSize(), expectedNotes);

    // ---- layers: bassline + arpeggiator ----
    {
        BassParams bp; bp.pattern = BassPattern::Offbeat;
        const auto bass = generateBass (prog, bp);
        ArpParams ap; ap.pattern = ArpPattern::Up; ap.rate = ArpRate::Sixteenth;
        const auto arp = generateArp (voiced, ap);

        int bmin = 200, bmax = 0;
        for (const auto& t : bass) { bmin = juce::jmin (bmin, t.note); bmax = juce::jmax (bmax, t.note); }
        const bool bassRange = bass.empty() || (bmin >= 24 && bmax <= 55);
        layersOk = ! bass.empty() && ! arp.empty() && bassRange;
        std::printf ("LAYERS: %s  bass=%d notes (MIDI %d..%d, Offbeat)  arp=%d notes (Up 1/16)\n",
                     layersOk ? "PASS" : "FAIL", (int) bass.size(), bass.empty() ? 0 : bmin,
                     bass.empty() ? 0 : bmax, (int) arp.size());
    }

    // ---- melody-aware fit + counter-melody ----
    {
        GenParams g; g.tonicPc = 9; g.mode = Mode::Aeolian; g.section = Section::Chorus;
        g.mood = Mood::Euphoric; g.lengthBars = 8; g.variation = 0.5f;
        auto pr = generate (g, 0x5151);
        const auto mpcs = scalePitchClasses (9, Mode::Aeolian);

        // a synthetic melody: one varied scale tone per chord slot
        NoteLayer mel;
        for (int i = 0; i < (int) pr.size(); ++i)
        {
            const int pc = mpcs[(size_t) ((i * 3) % 7)];
            mel.push_back ({ pr[(size_t) i].startBeat, pr[(size_t) i].lengthBeats, 60 + pc, 100 });
        }

        auto fitCount = [&] (const Progression& p)
        {
            int n = 0;
            for (int i = 0; i < (int) p.size(); ++i)
            {
                const auto hit = melodyInSlot (mel, p[(size_t) i].startBeat, p[(size_t) i].startBeat + p[(size_t) i].lengthBeats);
                const auto cp = p[(size_t) i].pitchClasses();
                if (hit.targetPc >= 0 && std::find (cp.begin(), cp.end(), hit.targetPc) != cp.end()) ++n;
            }
            return n;
        };

        const int before = fitCount (pr);
        applyMelodyFit (pr, mel, 9, Mode::Aeolian, false);
        const int after = fitCount (pr);

        VoicingConfig vc2; vc2.strictness = 0.7f;
        const auto voiced2 = voiceProgression (pr, vc2);
        CounterParams cpar; cpar.pattern = CounterPattern::Outline;
        const auto counter = generateCounter (voiced2, mel, cpar);

        const int total = (int) pr.size();
        melodyOk = after >= before && after >= (int) std::ceil (total * 0.8) && ! counter.empty();
        std::printf ("MELODY-FIT: %s  chords supporting melody %d->%d of %d   counter=%d notes\n",
                     melodyOk ? "PASS" : "FAIL", before, after, total, (int) counter.size());
    }

    // ---- song mode: chained sections ----
    {
        GenParams g; g.tonicPc = 9; g.mode = Mode::Aeolian; g.mood = Mood::Euphoric; g.variation = 0.6f;
        const auto form = songForm (2); // Full Track
        const auto spcs = scalePitchClasses (9, Mode::Aeolian);
        Progression song; double t = 0.0; int slot = 0; bool inScale = true;
        for (const auto& sec : form)
        {
            GenParams gp = g; gp.section = sec.type; gp.lengthBars = sec.bars; gp.modulation = false;
            auto p = generate (gp, 0x777u + (uint32_t) slot * 1234u, {});
            for (auto& c : p)
            {
                c.startBeat += t;
                for (int iv : c.intervals()) if (! scaleContains (spcs, c.rootPc + iv)) inScale = false;
                song.push_back (c);
            }
            t += sec.bars * 4.0; ++slot;
        }
        songOk = ! song.empty() && inScale && (int) form.size() >= 3;
        std::printf ("SONG: %s  %d sections, %d chords, %.0f beats (Full Track, in-scale=%s)\n",
                     songOk ? "PASS" : "FAIL", (int) form.size(), (int) song.size(),
                     progressionLengthBeats (song), inScale ? "YES" : "NO");
    }

    // ---- dedicated layer synths (bass + pluck) make sound ----
    {
        auto renderOne = [] (SimpleSynth& s, int note) -> float
        {
            const int N = 24000;
            juce::AudioBuffer<float> b (2, N); b.clear();
            juce::MidiBuffer m;
            m.addEvent (juce::MidiMessage::noteOn (1, note, (juce::uint8) 100), 0);
            m.addEvent (juce::MidiMessage::noteOff (1, note), 12000);
            for (int o = 0; o < N; o += 512)
            {
                const int bn = juce::jmin (512, N - o);
                juce::MidiBuffer sub;
                for (const auto meta : m) if (meta.samplePosition >= o && meta.samplePosition < o + bn) sub.addEvent (meta.getMessage(), meta.samplePosition - o);
                s.render (b, sub, o, bn);
            }
            float pk = 0.0f; for (int i = 0; i < N; ++i) pk = juce::jmax (pk, std::abs (b.getReadPointer (0)[i]));
            return pk;
        };
        SimpleSynth bs (4); bs.prepare (48000.0); bs.cfg.numSaws = 1; bs.cfg.subLevel = 0.9f; bs.cfg.cutoff = 700.0f; bs.cfg.filtEnv = 1400.0f;
        SimpleSynth ps (8); ps.prepare (48000.0); ps.cfg.cutoff = 900.0f; ps.cfg.filtEnv = 5500.0f;
        const float bp = renderOne (bs, 36); // bass C2
        const float pp = renderOne (ps, 72); // pluck C5
        synthsOk = bp > 0.01f && pp > 0.01f;
        std::printf ("SYNTHS: %s  bass peak=%.3f  pluck peak=%.3f\n", synthsOk ? "PASS" : "FAIL", bp, pp);
    }

    // ---- musical depth: swing, voicing styles, new bass patterns ----
    {
        GenParams g; g.tonicPc = 9; g.mode = Mode::Aeolian; g.section = Section::Chorus; g.lengthBars = 4; g.variation = 0.5f;
        const auto pr = generate (g, 0x1234);
        VoicingConfig vcd; const auto vd = voiceProgression (pr, vcd);

        ArpParams a0; a0.rate = ArpRate::Sixteenth; a0.swing = 0.0f;
        ArpParams a1 = a0; a1.swing = 0.5f;
        const auto arp0 = generateArp (vd, a0);
        const auto arp1 = generateArp (vd, a1);
        bool swingOk = false;
        if (arp0.size() == arp1.size())
            for (size_t i = 0; i < arp0.size(); ++i)
                if (arp1[i].startBeat > arp0[i].startBeat + 1.0e-4) { swingOk = true; break; }

        VoicingConfig close; close.style = VoicingStyle::Close;
        VoicingConfig wide;  wide.style  = VoicingStyle::Wide;
        const bool voicingOk = voiceChord (pr[0], {}, close) != voiceChord (pr[0], {}, wide);

        BassParams wb; wb.pattern = BassPattern::Walking;
        const auto walk = generateBass (pr, wb);
        bool bassPatOk = false;
        if (! walk.empty()) { int mn = 200, mx = 0; for (auto& t : walk) { mn = juce::jmin (mn, t.note); mx = juce::jmax (mx, t.note); } bassPatOk = (mx - mn) >= 5; }

        depthOk = swingOk && voicingOk && bassPatOk;
        std::printf ("DEPTH: %s  swing-shift=%s  voicing-diff=%s  walking-range=%s\n",
                     depthOk ? "PASS" : "FAIL", swingOk ? "Y" : "N", voicingOk ? "Y" : "N", bassPatOk ? "Y" : "N");
    }

    // ---- STEP 4: chord-vocabulary assertion (vocal-trance whitelist only) ----
    // every interval of every generated chord must be one of {0,2,3,4,5,7,10}
    // (root, sus2/9, min3, maj3, sus4, 5th, b7) and never exceed the 9th (<=14).
    // That excludes maj7 (11), tritone/dim (6), aug (8), 6th (9), b9 (1), 11/13/#11.
    {
        static const int allowed[] = { 0, 2, 3, 4, 5, 7, 10 };
        auto ok12 = [] (int iv) { if (iv > 14) return false; const int m = ((iv % 12) + 12) % 12;
                                  for (int a : allowed) if (a == m) return true; return false; };
        int checked = 0, violations = 0;
        for (int mode = 0; mode < (int) Mode::NumModes; ++mode)
          for (int sec = 0; sec < (int) Section::NumSections; ++sec)
            for (int st = 0; st < (int) Style::NumStyles; ++st)
              for (int lock = 0; lock < 2; ++lock)
              {
                  GenParams g; g.tonicPc = 9; g.mode = (Mode) mode; g.section = (Section) sec;
                  g.style = (Style) st; g.mood = Mood::Euphoric; g.lengthBars = 8;
                  g.complexity = 0.9f; g.variation = 0.7f; g.scaleLock = (lock != 0); g.allowBorrowed = true;
                  const auto p = generate (g, 0x1234u + (uint32_t) (mode * 97 + sec * 13 + st * 7 + lock));
                  for (const auto& c : p) for (int iv : c.intervals()) { ++checked; if (! ok12 (iv)) ++violations; }
              }
        vocabOk = violations == 0 && checked > 0;
        std::printf ("VOCAB: %s  %d chord-intervals checked across all modes/sections/styles, %d jazz/out-of-whitelist violations; library = %d generic + %d styled\n",
                     vocabOk ? "PASS" : "FAIL", checked, violations, (int) degreePhrases().size(), (int) styledPhrases().size());
    }

    // ---- sound polish: pump envelope + master soft-clip ----
    {
        const float g0 = pumpGain (0.0f, 0.8f), gm = pumpGain (0.5f, 0.8f), g1 = pumpGain (1.0f, 0.8f);
        const bool pumpShape = g0 < gm && gm < g1 && g0 < 0.4f && g1 > 0.95f;
        const bool clipOk = std::abs (softClip (0.5f) - 0.5f) < 1.0e-4f && softClip (2.0f) <= 1.0f && softClip (2.0f) > 0.9f;
        polishOk = pumpShape && clipOk;
        std::printf ("POLISH: %s  pump g(0/.5/1)=%.2f/%.2f/%.2f  softClip(2.0)=%.3f\n",
                     polishOk ? "PASS" : "FAIL", g0, gm, g1, softClip (2.0f));
    }

    // ---------------- 3. render the preview pad ----------------
    const double sr = 48000.0, bpm = 124.0;
    PadSynth synth; synth.prepare (sr);
    synth.setParams (8.0f, 500.0f, 4000.0f, 0.3f);

    const double total = progressionLengthBeats (prog);
    const int N = (int) (sr * (total * 60.0 / bpm)) + (int) (sr * 1.0);
    juce::AudioBuffer<float> buf (2, N); buf.clear();

    juce::MidiBuffer master;
    for (const auto& v : voiced)
    {
        const int onS  = (int) (v.startBeat * 60.0 / bpm * sr);
        const int offS = (int) ((v.startBeat + v.lengthBeats) * 60.0 / bpm * sr) - 200;
        for (int n : v.notes)
        {
            master.addEvent (juce::MidiMessage::noteOn  (1, n, (juce::uint8) 90), onS);
            master.addEvent (juce::MidiMessage::noteOff (1, n), juce::jmax (onS + 1, offS));
        }
    }

    const int blk = 512;
    for (int o = 0; o < N; o += blk)
    {
        const int bn = juce::jmin (blk, N - o);
        juce::MidiBuffer sub;
        for (const auto meta : master)
            if (meta.samplePosition >= o && meta.samplePosition < o + bn)
                sub.addEvent (meta.getMessage(), meta.samplePosition - o);
        synth.render (buf, sub, o, bn);
    }

    float peak = 0.0f; double rms = 0.0; bool finite = true;
    const float* L = buf.getReadPointer (0);
    for (int i = 0; i < N; ++i)
    {
        if (! std::isfinite (L[i])) finite = false;
        peak = juce::jmax (peak, std::abs (L[i]));
        rms += (double) L[i] * L[i];
    }
    rms = std::sqrt (rms / N);
    const bool soundOk = peak > 0.01f && finite;
    std::printf ("PAD:  %s  peak=%.4f  rms=%.4f  finite=%s\n", soundOk ? "PASS" : "FAIL",
                 peak, (float) rms, finite ? "YES" : "NO");

    auto wavOut = juce::File::getCurrentWorkingDirectory().getChildFile ("trancechords_test.wav");
    juce::WavAudioFormat wav;
    if (auto fos = std::unique_ptr<juce::FileOutputStream> (wavOut.createOutputStream().release()))
        if (auto* writer = wav.createWriterFor (fos.get(), sr, 2, 16, {}, 0))
        {
            fos.release();
            writer->writeFromAudioSampleBuffer (buf, 0, N);
            delete writer;
            std::printf ("WROTE: %s\n", wavOut.getFullPathName().toRawUTF8());
        }

    const bool ok = scaleOk && varietyOk && layersOk && melodyOk && songOk && synthsOk && depthOk && vocabOk && polishOk && midiOk && soundOk && totalChords > 0;
    std::printf ("RESULT: %s\n", ok ? "PASS" : "FAIL");
    return ok ? 0 : 3;
}
