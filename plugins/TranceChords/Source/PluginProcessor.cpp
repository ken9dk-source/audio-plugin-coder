#include "PluginProcessor.h"
#include "PluginEditor.h"
#include <cmath>

using APF = juce::AudioParameterFloat;
using API = juce::AudioParameterInt;
using APC = juce::AudioParameterChoice;
using APB = juce::AudioParameterBool;
namespace P = ParameterIDs;

//==============================================================================
// parameter layout
//==============================================================================
juce::AudioProcessorValueTreeState::ParameterLayout TranceChordsAudioProcessor::createParameterLayout()
{
    std::vector<std::unique_ptr<juce::RangedAudioParameter>> p;

    const juce::StringArray keys   { "C","C#","D","D#","E","F","F#","G","G#","A","A#","B" };
    const juce::StringArray modes  { "Ionian","Dorian","Phrygian","Lydian","Mixolydian",
                                     "Aeolian","Locrian","Harmonic Minor","Melodic Minor" };
    const juce::StringArray sects  { "Verse","Pre-Chorus","Chorus","Breakdown","Build-up","Drop","Bridge" };
    const juce::StringArray moods  { "Dreamy","Romantic","Euphoric" };
    const juce::StringArray styles { "Any","Uplifting","Oldschool 2000","Psytrance","Festival","Progressive","Classic ASOT" };
    const juce::StringArray lens   { "4 bars","8 bars","16 bars" };
    const juce::StringArray dens   { "1 / bar","2 / bar" };

    p.push_back (std::make_unique<APC> (P::key,     "Key",     keys,  0));
    p.push_back (std::make_unique<APC> (P::mode,    "Mode",    modes, 5));   // Aeolian
    p.push_back (std::make_unique<APC> (P::section, "Section", sects, 0));
    p.push_back (std::make_unique<APC> (P::mood,    "Mood",    moods,  0));
    p.push_back (std::make_unique<APC> (P::style,   "Style",   styles, 0));  // Any
    p.push_back (std::make_unique<APC> (P::length,  "Length",  lens,   1));  // 8 bars
    p.push_back (std::make_unique<APC> (P::density, "Density", dens,  0));

    auto pct = [] (const char* id, const char* nm, float def)
    { return std::make_unique<APF> (id, nm, juce::NormalisableRange<float> (0.0f, 100.0f, 0.1f), def); };

    p.push_back (pct (P::energy,        "Energy",        40.0f));
    p.push_back (pct (P::complexity,    "Complexity",    45.0f));
    p.push_back (pct (P::voice_leading, "Voice Leading", 70.0f));
    p.push_back (pct (P::variation,     "Variation",     50.0f));
    p.push_back (pct (P::humanize,      "Humanize",      20.0f));
    p.push_back (pct (P::swing,         "Swing",         0.0f));
    p.push_back (std::make_unique<APC> (P::voicing_style, "Voicing",
                 juce::StringArray { "Close", "Open", "Drop-2", "Wide" }, 1)); // Open
    p.push_back (std::make_unique<API> (P::octave, "Octave", -2, 2, 0));

    p.push_back (std::make_unique<APB> (P::allow_sus,      "Allow Sus",      true));
    p.push_back (std::make_unique<APB> (P::allow_borrowed, "Allow Borrowed", true));
    p.push_back (std::make_unique<APB> (P::forbid_triads,  "Forbid Triads",  false));
    p.push_back (std::make_unique<APB> (P::no_pop,         "No Pop",         true));
    p.push_back (std::make_unique<APB> (P::scale_lock,     "Scale Lock",     true));
    p.push_back (std::make_unique<APB> (P::modulation,     "Modulation",     false));
    p.push_back (std::make_unique<APB> (P::sec_dominants,  "Sec. Dominants", false));

    // ---- layers: bass + arp ----
    p.push_back (std::make_unique<APB> (P::bass_enable,  "Bass", false));
    p.push_back (std::make_unique<APC> (P::bass_pattern, "Bass Pattern",
                 juce::StringArray { "Sustained", "Root 8ths", "Offbeat", "Rolling 16ths", "Octaves", "Walking" }, 2));
    p.push_back (std::make_unique<API> (P::bass_octave,  "Bass Octave", 1, 3, 2));
    p.push_back (pct (P::bass_gate, "Bass Gate", 72.0f));

    p.push_back (std::make_unique<APB> (P::arp_enable,   "Arp", false));
    p.push_back (std::make_unique<APC> (P::arp_pattern,  "Arp Pattern",
                 juce::StringArray { "Up", "Down", "Up/Down", "Converge", "Random" }, 0));
    p.push_back (std::make_unique<APC> (P::arp_rate,     "Arp Rate",
                 juce::StringArray { "1/8", "1/16", "1/8T" }, 1));
    p.push_back (std::make_unique<API> (P::arp_octaves,  "Arp Octaves", 1, 2, 1));
    p.push_back (pct (P::arp_gate, "Arp Gate", 70.0f));

    p.push_back (std::make_unique<APB> (P::counter_enable,  "Counter", false));
    p.push_back (std::make_unique<APC> (P::counter_pattern, "Counter Pattern",
                 juce::StringArray { "Outline", "Top Voice", "Pulse" }, 0));
    p.push_back (std::make_unique<APC> (P::counter_rate,    "Counter Rate",
                 juce::StringArray { "1/8", "1/16", "1/8T" }, 0));

    p.push_back (std::make_unique<APB> (P::melody_fit, "Melody Fit", true));

    p.push_back (std::make_unique<APB> (P::song_mode, "Song Mode", false));
    {
        juce::StringArray forms;
        for (int i = 0; i < tc::kNumSongForms; ++i) forms.add (tc::songFormName (i));
        p.push_back (std::make_unique<APC> (P::song_form, "Song Form", forms, 0));
    }

    auto pctTo = [] (const char* id, const char* nm, float def, float hi)
    { return std::make_unique<APF> (id, nm, juce::NormalisableRange<float> (0.0f, hi, 0.1f), def); };
    p.push_back (pctTo (P::mix_chords,  "Chords Level",  100.0f, 150.0f));
    p.push_back (pctTo (P::mix_bass,    "Bass Level",    100.0f, 150.0f));
    p.push_back (pctTo (P::mix_arp,     "Arp Level",     100.0f, 150.0f));
    p.push_back (pctTo (P::mix_counter, "Counter Level", 100.0f, 150.0f));

    p.push_back (std::make_unique<APB> (P::prev_enable, "Preview", true));
    p.push_back (std::make_unique<APF> (P::prev_attack, "Pad Attack",
                 juce::NormalisableRange<float> (1.0f, 2000.0f, 1.0f, 0.4f), 12.0f));
    p.push_back (std::make_unique<APF> (P::prev_release, "Pad Release",
                 juce::NormalisableRange<float> (20.0f, 4000.0f, 1.0f, 0.4f), 600.0f));
    p.push_back (std::make_unique<APF> (P::prev_cutoff, "Pad Cutoff",
                 juce::NormalisableRange<float> (200.0f, 16000.0f, 1.0f, 0.30f), 3500.0f));
    p.push_back (pct (P::prev_detune, "Pad Detune", 30.0f));
    p.push_back (pct (P::prev_reverb, "Reverb",     35.0f));
    p.push_back (pct (P::prev_chorus, "Chorus",     40.0f));
    p.push_back (pct (P::pump,        "Pump",       0.0f));
    p.push_back (std::make_unique<APF> (P::output, "Output",
                 juce::NormalisableRange<float> (-24.0f, 6.0f, 0.1f), -3.0f));

    return { p.begin(), p.end() };
}

//==============================================================================
TranceChordsAudioProcessor::TranceChordsAudioProcessor()
    : AudioProcessor (BusesProperties().withOutput ("Output", juce::AudioChannelSet::stereo(), true)),
      apvts (*this, nullptr, "PARAMS", createParameterLayout())
{
    generate();
}

void TranceChordsAudioProcessor::prepareToPlay (double sampleRate, int samplesPerBlock)
{
    sr = sampleRate;
    padSynth.prepare (sampleRate);
    bassSynth.prepare (sampleRate);
    pluckSynth.prepare (sampleRate);
    counterSynth.prepare (sampleRate);

    // BASS voice: deep saw + strong sub, punchy, lowish filter with snap
    bassSynth.cfg.numSaws = 1;  bassSynth.cfg.subLevel = 0.9f; bassSynth.cfg.detune = 0.0f;
    bassSynth.cfg.ampA = 2.0f;  bassSynth.cfg.ampD = 140.0f;   bassSynth.cfg.ampS = 0.55f; bassSynth.cfg.ampR = 90.0f;
    bassSynth.cfg.cutoff = 700.0f; bassSynth.cfg.filtEnv = 1400.0f; bassSynth.cfg.filtD = 130.0f; bassSynth.cfg.filtS = 0.25f;
    bassSynth.cfg.reso = 0.25f;  bassSynth.cfg.drive = 2.0f; bassSynth.cfg.level = 0.9f;

    // PLUCK voice: bright 2-saw, fast filter-env pluck, short tail
    pluckSynth.cfg.numSaws = 2;  pluckSynth.cfg.subLevel = 0.15f; pluckSynth.cfg.detune = 10.0f;
    pluckSynth.cfg.ampA = 1.0f;  pluckSynth.cfg.ampD = 220.0f;  pluckSynth.cfg.ampS = 0.15f; pluckSynth.cfg.ampR = 160.0f;
    pluckSynth.cfg.cutoff = 900.0f; pluckSynth.cfg.filtEnv = 5500.0f; pluckSynth.cfg.filtD = 130.0f; pluckSynth.cfg.filtS = 0.1f;
    pluckSynth.cfg.reso = 0.4f;  pluckSynth.cfg.drive = 1.4f; pluckSynth.cfg.level = 0.7f;

    // COUNTER voice: softer, slightly darker pluck so it sits under the lead
    counterSynth.cfg.numSaws = 2; counterSynth.cfg.subLevel = 0.1f; counterSynth.cfg.detune = 7.0f;
    counterSynth.cfg.ampA = 2.0f; counterSynth.cfg.ampD = 260.0f; counterSynth.cfg.ampS = 0.18f; counterSynth.cfg.ampR = 200.0f;
    counterSynth.cfg.cutoff = 700.0f; counterSynth.cfg.filtEnv = 3800.0f; counterSynth.cfg.filtD = 160.0f; counterSynth.cfg.filtS = 0.12f;
    counterSynth.cfg.reso = 0.3f; counterSynth.cfg.drive = 1.3f; counterSynth.cfg.level = 0.6f;

    scratchPad.setSize (2, samplesPerBlock, false, false, true);
    scratchPluck.setSize (2, samplesPerBlock, false, false, true);
    scratchBass.setSize (2, samplesPerBlock, false, false, true);
    scratchCounter.setSize (2, samplesPerBlock, false, false, true);

    juce::dsp::ProcessSpec spec { sampleRate, (juce::uint32) juce::jmax (1, samplesPerBlock), 2 };
    chorus.prepare (spec);
    chorus.setRate (0.6f);
    chorus.setDepth (0.25f);
    chorus.setCentreDelay (7.0f);
    chorus.setFeedback (0.15f);

    reverb.setSampleRate (sampleRate);

    if (! schedule.load()) generate();
}

bool TranceChordsAudioProcessor::isBusesLayoutSupported (const BusesLayout& layouts) const
{
    const auto out = layouts.getMainOutputChannelSet();
    return out == juce::AudioChannelSet::stereo() || out == juce::AudioChannelSet::mono();
}

//==============================================================================
// param collection
//==============================================================================
tc::GenParams TranceChordsAudioProcessor::collectGenParams() const
{
    auto idx = [this] (const char* id) { return (int) apvts.getRawParameterValue (id)->load(); };
    auto f   = [this] (const char* id) { return apvts.getRawParameterValue (id)->load(); };

    tc::GenParams gp;
    gp.tonicPc      = idx (P::key);
    gp.mode         = (tc::Mode)    juce::jlimit (0, (int) tc::Mode::NumModes - 1, idx (P::mode));
    gp.section      = (tc::Section) juce::jlimit (0, (int) tc::Section::NumSections - 1, idx (P::section));
    gp.mood         = (tc::Mood)    juce::jlimit (0, 2, idx (P::mood));
    gp.style        = (tc::Style)   juce::jlimit (0, (int) tc::Style::NumStyles - 1, idx (P::style));
    static const int barsByIdx[3] { 4, 8, 16 };
    gp.lengthBars   = barsByIdx[juce::jlimit (0, 2, idx (P::length))];
    gp.chordsPerBar = idx (P::density) == 0 ? 1 : 2;
    gp.energy       = f (P::energy)        * 0.01f;
    gp.complexity   = f (P::complexity)    * 0.01f;
    gp.voiceLeading = f (P::voice_leading) * 0.01f;
    gp.variation    = f (P::variation)     * 0.01f;
    gp.humanize     = f (P::humanize)      * 0.01f;
    gp.octave       = idx (P::octave);
    gp.allowSus      = f (P::allow_sus)      > 0.5f;
    gp.allowBorrowed = f (P::allow_borrowed) > 0.5f;
    gp.forbidTriads  = f (P::forbid_triads)  > 0.5f;
    gp.noPop         = f (P::no_pop)         > 0.5f;
    gp.scaleLock     = f (P::scale_lock)     > 0.5f;
    gp.modulation    = f (P::modulation)     > 0.5f;
    gp.secDominants  = f (P::sec_dominants)  > 0.5f;
    return gp;
}

tc::VoicingConfig TranceChordsAudioProcessor::collectVoicing() const
{
    tc::VoicingConfig vc;
    vc.strictness  = apvts.getRawParameterValue (P::voice_leading)->load() * 0.01f;
    vc.octaveShift = ((int) apvts.getRawParameterValue (P::octave)->load()) * 12;
    vc.maxVoices   = 5;
    vc.style       = (tc::VoicingStyle) juce::jlimit (0, (int) tc::VoicingStyle::NumStyles - 1,
                                                      (int) apvts.getRawParameterValue (P::voicing_style)->load());
    return vc;
}

tc::BassParams TranceChordsAudioProcessor::collectBassParams() const
{
    auto idx = [this] (const char* id) { return (int) apvts.getRawParameterValue (id)->load(); };
    tc::BassParams bp;
    bp.pattern = (tc::BassPattern) juce::jlimit (0, (int) tc::BassPattern::NumPatterns - 1, idx (P::bass_pattern));
    bp.octave  = juce::jlimit (1, 3, idx (P::bass_octave));
    bp.gate    = apvts.getRawParameterValue (P::bass_gate)->load() * 0.01f;
    bp.swing   = apvts.getRawParameterValue (P::swing)->load() * 0.01f;
    return bp;
}

tc::ArpParams TranceChordsAudioProcessor::collectArpParams() const
{
    auto idx = [this] (const char* id) { return (int) apvts.getRawParameterValue (id)->load(); };
    tc::ArpParams ap;
    ap.pattern = (tc::ArpPattern) juce::jlimit (0, (int) tc::ArpPattern::NumPatterns - 1, idx (P::arp_pattern));
    ap.rate    = (tc::ArpRate)    juce::jlimit (0, (int) tc::ArpRate::NumRates - 1,    idx (P::arp_rate));
    ap.octaves = juce::jlimit (1, 2, idx (P::arp_octaves));
    ap.gate    = apvts.getRawParameterValue (P::arp_gate)->load() * 0.01f;
    ap.swing   = apvts.getRawParameterValue (P::swing)->load() * 0.01f;
    return ap;
}

tc::CounterParams TranceChordsAudioProcessor::collectCounterParams() const
{
    auto idx = [this] (const char* id) { return (int) apvts.getRawParameterValue (id)->load(); };
    tc::CounterParams cp;
    cp.pattern = (tc::CounterPattern) juce::jlimit (0, (int) tc::CounterPattern::NumPatterns - 1, idx (P::counter_pattern));
    cp.rate    = (tc::ArpRate)        juce::jlimit (0, (int) tc::ArpRate::NumRates - 1,           idx (P::counter_rate));
    cp.swing   = apvts.getRawParameterValue (P::swing)->load() * 0.01f;
    return cp;
}

// flat note layer -> sorted note-on/off events for the audio thread
static std::vector<tc::ScheduledNote> layerToEvents (const tc::NoteLayer& layer)
{
    std::vector<tc::ScheduledNote> ev;
    ev.reserve (layer.size() * 2);
    for (const auto& tn : layer)
    {
        const double off = tn.startBeat + std::max (0.02, tn.lengthBeats) - 0.01;
        ev.push_back ({ tn.startBeat, true, tn.note, tn.vel });
        ev.push_back ({ std::max (tn.startBeat + 0.001, off), false, tn.note, 0 });
    }
    std::sort (ev.begin(), ev.end(), [] (const tc::ScheduledNote& a, const tc::ScheduledNote& b)
               { return a.beat != b.beat ? a.beat < b.beat : (a.on ? 1 : 0) < (b.on ? 1 : 0); });
    return ev;
}

std::shared_ptr<const tc::Schedule> TranceChordsAudioProcessor::buildSchedule (const tc::Progression& prog) const
{
    auto sch = std::make_shared<tc::Schedule>();
    sch->chords      = tc::voiceProgression (prog, collectVoicing());
    sch->totalBeats  = tc::progressionLengthBeats (prog);
    sch->chordEvents   = layerToEvents (tc::chordsToLayer (sch->chords));
    sch->bassEvents    = layerToEvents (tc::generateBass (prog, collectBassParams()));
    sch->arpEvents     = layerToEvents (tc::generateArp  (sch->chords, collectArpParams()));
    sch->counterEvents = layerToEvents (tc::generateCounter (sch->chords, melody, collectCounterParams()));
    return sch;
}

//==============================================================================
// generation / UI API
//==============================================================================
tc::Progression TranceChordsAudioProcessor::buildSong (const tc::GenParams& base, uint32_t seed) const
{
    const int formIdx = (int) apvts.getRawParameterValue (P::song_form)->load();
    const auto form = tc::songForm (formIdx);

    tc::Progression song;
    std::vector<int> sectionStart;
    double t = 0.0;
    int slot = 0;
    for (const auto& sec : form)
    {
        sectionStart.push_back ((int) song.size());
        tc::GenParams gp = base;
        gp.section    = sec.type;
        gp.lengthBars = sec.bars;
        gp.modulation = false;            // keep the whole song in one key
        auto p = tc::generate (gp, seed + (uint32_t) slot * 0x9E37u, {});
        for (auto& c : p) { c.startBeat += t; song.push_back (c); }
        t += sec.bars * 4.0;
        ++slot;
    }
    juce::ignoreUnused (sectionStart);
    return song;
}

void TranceChordsAudioProcessor::generate()
{
    const auto gp = collectGenParams();
    const uint32_t seed = (gp.variation < 0.001f) ? 0x1234u : (seedCounter += 0x9E3779B1u);
    if (apvts.getRawParameterValue (P::song_mode)->load() > 0.5f)
        currentProg = buildSong (gp, seed);
    else
        currentProg = tc::generate (gp, seed, currentProg);
    if (apvts.getRawParameterValue (P::melody_fit)->load() > 0.5f && ! melody.empty())
        tc::applyMelodyFit (currentProg, melody, gp.tonicPc, gp.mode, tc::keyPrefersFlats (gp.tonicPc, gp.mode));
    pushHistory();
    schedule.store (buildSchedule (currentProg));
    progVersion.fetch_add (1);
}

void TranceChordsAudioProcessor::pushHistory()
{
    if (historyPos >= 0 && historyPos < (int) history.size() - 1)
        history.resize ((size_t) historyPos + 1);          // drop the redo tail
    history.push_back (currentProg);
    while ((int) history.size() > kMaxHistory) history.erase (history.begin());
    historyPos = (int) history.size() - 1;
}

void TranceChordsAudioProcessor::undo()
{
    if (! canUndo()) return;
    --historyPos;
    currentProg = history[(size_t) historyPos];
    schedule.store (buildSchedule (currentProg));
    progVersion.fetch_add (1);
}

void TranceChordsAudioProcessor::redo()
{
    if (! canRedo()) return;
    ++historyPos;
    currentProg = history[(size_t) historyPos];
    schedule.store (buildSchedule (currentProg));
    progVersion.fetch_add (1);
}

void TranceChordsAudioProcessor::saveFavorite()
{
    if (! currentProg.empty()) favorites.push_back (currentProg);
}

void TranceChordsAudioProcessor::recallFavorite (int index)
{
    if (index < 0 || index >= (int) favorites.size()) return;
    currentProg = favorites[(size_t) index];
    pushHistory();
    schedule.store (buildSchedule (currentProg));
    progVersion.fetch_add (1);
}

juce::String TranceChordsAudioProcessor::favoriteLabel (int index) const
{
    if (index < 0 || index >= (int) favorites.size()) return {};
    const auto& f = favorites[(size_t) index];
    juce::String s;
    for (int i = 0; i < juce::jmin (4, (int) f.size()); ++i) s += (i ? " " : "") + f[(size_t) i].label;
    if ((int) f.size() > 4) s += " ...";
    return s;
}

void TranceChordsAudioProcessor::revoice()
{
    if (currentProg.empty()) { generate(); return; }
    schedule.store (buildSchedule (currentProg));
    progVersion.fetch_add (1);
}

void TranceChordsAudioProcessor::setMelody (const tc::NoteLayer& m)
{
    melody = m;
    melodyCount.store ((int) m.size());
    generate();   // re-fit chords to the new melody
}

void TranceChordsAudioProcessor::clearMelody()
{
    melody.clear();
    melodyCount.store (0);
    generate();
}

void TranceChordsAudioProcessor::toggleLock (int index)
{
    if (index >= 0 && index < (int) currentProg.size())
    {
        currentProg[(size_t) index].locked = ! currentProg[(size_t) index].locked;
        progVersion.fetch_add (1);
    }
}

void TranceChordsAudioProcessor::auditionChord (int index)   { auditionReq.store (index); }

void TranceChordsAudioProcessor::setPreviewPlaying (bool shouldPlay)
{
    if (shouldPlay) previewResetPending.store (true);
    previewPlaying.store (shouldPlay);
}

int TranceChordsAudioProcessor::playheadChordIndex() const { return curChordForUi.load(); }

double TranceChordsAudioProcessor::scheduleTotalBeats() const
{
    if (auto s = schedule.load()) return s->totalBeats;
    return 0.0;
}

std::vector<int> TranceChordsAudioProcessor::chordNotesAt (int index) const
{
    if (auto s = schedule.load())
        if (index >= 0 && index < (int) s->chords.size())
            return s->chords[(size_t) index].notes;
    return {};
}

int TranceChordsAudioProcessor::activeMelodyNote() const
{
    const double beat = playBeatForUi.load();
    if (melody.empty() || beat < 0.0) return -1;
    for (const auto& n : melody)
        if (beat >= n.startBeat - 1.0e-4 && beat < n.startBeat + n.lengthBeats) return n.note;
    return -1;
}

double TranceChordsAudioProcessor::hostOrDefaultBpm() const { return bpmForUi.load(); }

juce::File TranceChordsAudioProcessor::exportTempMidi (int layer)
{
    const double bpm = hostOrDefaultBpm();
    const float  hum = apvts.getRawParameterValue (P::humanize)->load() * 0.01f;
    const bool songMode = apvts.getRawParameterValue (P::song_mode)->load() > 0.5f;
    const juce::String base = currentProg.empty()
        ? juce::String ("chords")
        : (songMode ? juce::String ("Song_") : juce::String (tc::sectionName (collectGenParams().section)) + "_")
          + currentProg.front().label;
    const auto voiced = tc::voiceProgression (currentProg, collectVoicing());

    switch (layer)
    {
        case 1:  return tc::writeTempMidiLayer (tc::generateBass (currentProg, collectBassParams()), bpm, hum, "Bass_"    + base);
        case 2:  return tc::writeTempMidiLayer (tc::generateArp  (voiced,      collectArpParams()),  bpm, hum, "Arp_"     + base);
        case 3:  return tc::writeTempMidiLayer (tc::generateCounter (voiced, melody, collectCounterParams()), bpm, hum, "Counter_" + base);
        default: return tc::writeTempMidiLayer (tc::chordsToLayer (voiced),                          bpm, hum, "Chords_"  + base);
    }
}

void TranceChordsAudioProcessor::applyPreset (int index)
{
    auto setP = [this] (const char* id, float raw)
    { if (auto* prm = apvts.getParameter (id)) prm->setValueNotifyingHost (prm->convertTo0to1 (raw)); };

    // reset everything presets touch to sensible defaults first
    setP (P::allow_sus, 1.0f); setP (P::allow_borrowed, 1.0f);
    setP (P::forbid_triads, 0.0f); setP (P::no_pop, 1.0f);
    setP (P::scale_lock, 1.0f); setP (P::modulation, 0.0f); setP (P::sec_dominants, 0.0f);
    setP (P::style, 0.0f); setP (P::density, 0.0f);
    setP (P::song_mode, 0.0f);
    setP (P::bass_enable, 0.0f); setP (P::arp_enable, 0.0f); setP (P::counter_enable, 0.0f);

    auto bass  = [&] (float pat) { setP (P::bass_enable, 1.0f); setP (P::bass_pattern, pat); };
    auto arp   = [&] (float pat, float rate) { setP (P::arp_enable, 1.0f); setP (P::arp_pattern, pat); setP (P::arp_rate, rate); };
    auto basic = [&] (float key, float mode, float sec, float mood, float style, float cx, float vr, float en, float vl, float len)
    { setP (P::key, key); setP (P::mode, mode); setP (P::section, sec); setP (P::mood, mood); setP (P::style, style);
      setP (P::complexity, cx); setP (P::variation, vr); setP (P::energy, en); setP (P::voice_leading, vl); setP (P::length, len); };

    switch (index)
    {
        case 0:  basic (0, 0, 0, 0, 0, 50, 50, 30, 80, 1); break;                              // Dreamy Verse (C Ionian)
        case 1:  basic (9, 5, 0, 1, 0, 55, 55, 35, 75, 1); break;                              // Romantic Verse (A Aeolian)
        case 2:  basic (9, 5, 2, 2, 1, 62, 55, 85, 70, 2); setP (P::modulation, 1.0f); bass (2); break; // Uplifting Anthem
        case 3:  basic (6, 3, 2, 0, 0, 60, 50, 55, 80, 1); break;                              // Heavenly Chorus (F# Lydian)
        case 4:  basic (9, 5, 1, 2, 0, 55, 55, 85, 60, 0); break;                              // Pre-Chorus Build
        case 5:  basic (4, 2, 0, 1, 0, 55, 60, 40, 70, 1); setP (P::allow_borrowed, 0.0f); break; // Dark Phrygian
        case 6:  basic (9, 5, 2, 2, 2, 42, 60, 75, 65, 1); bass (2); break;                    // Oldschool 2000
        case 7:  basic (4, 2, 5, 2, 3, 35, 60, 80, 60, 1); bass (3); arp (0, 1); break;        // Psy Roller (E Phrygian Drop)
        case 8:  basic (11,5, 5, 2, 4, 55, 55, 90, 70, 1); setP (P::modulation, 1.0f); bass (2); arp (0, 1); break; // Festival Anthem (B Aeolian Drop)
        case 9:  basic (9, 1, 0, 1, 5, 65, 50, 45, 85, 2); arp (0, 1); break;                  // Progressive Deep (A Dorian)
        case 10: basic (9, 5, 2, 2, 6, 60, 55, 85, 75, 2); setP (P::modulation, 1.0f); bass (2); break; // ASOT Uplifter
        case 11: basic (2, 0, 5, 2, 1, 65, 55, 90, 65, 1); setP (P::modulation, 1.0f); bass (2); arp (0, 1); break; // Euphoric Drop (D Ionian)
        case 12: basic (7, 0, 0, 0, 0, 45, 45, 35, 80, 1); break;                              // Vocal Verse (G Ionian)
        case 13: basic (6, 5, 3, 0, 5, 60, 45, 30, 85, 1); break;                              // Breakdown Pad (F# Aeolian)
        case 14: basic (9, 5, 2, 2, 1, 60, 55, 80, 70, 1); setP (P::song_mode, 1.0f); setP (P::song_form, 2); bass (2); arp (0, 1); break; // Song: Uplifting
        case 15: basic (11,5, 5, 2, 4, 55, 55, 90, 70, 1); setP (P::song_mode, 1.0f); setP (P::song_form, 1); bass (2); arp (0, 1); setP (P::counter_enable, 1.0f); break; // Song: Festival
        default: break;
    }
    generate();
}

//==============================================================================
// playback helpers
//==============================================================================
static int chordIndexAtBeat (const tc::Schedule& s, double beat)
{
    for (int i = 0; i < (int) s.chords.size(); ++i)
    {
        const auto& c = s.chords[(size_t) i];
        if (beat >= c.startBeat && beat < c.startBeat + c.lengthBeats) return i;
    }
    return -1;
}

void TranceChordsAudioProcessor::emitRange (const std::vector<tc::ScheduledNote>& events, double totalBeats,
                                            double beatStart, double blockBeats, int numSamples,
                                            juce::MidiBuffer& padMidi, juce::MidiBuffer* hostOut)
{
    if (blockBeats <= 0.0 || totalBeats <= 0.0) return;
    const double winEnd = beatStart + blockBeats;

    for (const auto& ev : events)
    {
        for (double occ : { ev.beat, ev.beat + totalBeats })
        {
            if (occ >= beatStart && occ < winEnd)
            {
                const int off = juce::jlimit (0, juce::jmax (0, numSamples - 1),
                                              (int) ((occ - beatStart) / blockBeats * numSamples));
                const auto msg = ev.on ? juce::MidiMessage::noteOn  (1, ev.note, (juce::uint8) ev.vel)
                                       : juce::MidiMessage::noteOff (1, ev.note);
                padMidi.addEvent (msg, off);
                if (hostOut != nullptr) hostOut->addEvent (msg, off);
            }
        }
    }
}

void TranceChordsAudioProcessor::processBlock (juce::AudioBuffer<float>& buffer, juce::MidiBuffer& midi)
{
    juce::ScopedNoDenormals noDenormals;
    const int numSamples = buffer.getNumSamples();
    buffer.clear();

    auto raw = [this] (const char* id) { return apvts.getRawParameterValue (id)->load(); };
    const bool  prevEnable = raw (P::prev_enable) > 0.5f;
    padSynth.setParams (raw (P::prev_attack), raw (P::prev_release), raw (P::prev_cutoff), raw (P::prev_detune) * 0.01f);
    const float chorusMix = raw (P::prev_chorus) * 0.01f;
    const float reverbMix = raw (P::prev_reverb) * 0.01f;
    const float outGain   = juce::Decibels::decibelsToGain (raw (P::output));
    const float pumpAmt   = raw (P::pump) * 0.01f;
    const bool  bassOn    = raw (P::bass_enable)    > 0.5f;
    const bool  arpOn     = raw (P::arp_enable)     > 0.5f;
    const bool  counterOn = raw (P::counter_enable) > 0.5f;

    // transport
    double bpm = 124.0, ppq = 0.0; bool hostPlaying = false;
    if (auto* ph = getPlayHead())
        if (const auto pos = ph->getPosition())
        {
            if (pos->getBpm())         bpm = *pos->getBpm();
            if (pos->getPpqPosition()) ppq = *pos->getPpqPosition();
            hostPlaying = pos->getIsPlaying();
        }
    bpmForUi.store (bpm);

    const float gChords  = raw (P::mix_chords)  * 0.01f;
    const float gBass    = raw (P::mix_bass)    * 0.01f;
    const float gArp     = raw (P::mix_arp)     * 0.01f;
    const float gCounter = raw (P::mix_counter) * 0.01f;

    // per-layer MIDI: chords+keyboard -> pad, bass -> bass synth, arp -> pluck, counter -> counter.
    // host MIDI out (midi) is rebuilt from all layers merged.
    juce::MidiBuffer padMidi, bassMidi, pluckMidi, counterMidi;
    for (const auto meta : midi) padMidi.addEvent (meta.getMessage(), meta.samplePosition);
    midi.clear();

    const double blockBeats = ((double) numSamples / sr) * (bpm / 60.0);
    if (previewResetPending.exchange (false)) previewBeat = 0.0;

    auto sched = schedule.load();
    if (sched && sched->totalBeats > 0.0)
    {
        auto emitAll = [&] (double beatStart, juce::MidiBuffer* hostOut)
        {
            emitRange (sched->chordEvents, sched->totalBeats, beatStart, blockBeats, numSamples, padMidi, hostOut);
            if (bassOn)    emitRange (sched->bassEvents,    sched->totalBeats, beatStart, blockBeats, numSamples, bassMidi,  hostOut);
            if (arpOn)     emitRange (sched->arpEvents,     sched->totalBeats, beatStart, blockBeats, numSamples, pluckMidi,   hostOut);
            if (counterOn) emitRange (sched->counterEvents, sched->totalBeats, beatStart, blockBeats, numSamples, counterMidi, hostOut);
        };

        if (hostPlaying)
        {
            double bStart = std::fmod (ppq, sched->totalBeats);
            if (bStart < 0.0) bStart += sched->totalBeats;
            emitAll (bStart, &midi);
            curChordForUi.store (chordIndexAtBeat (*sched, bStart));
            playBeatForUi.store (bStart);
        }
        else if (previewPlaying.load())
        {
            emitAll (previewBeat, nullptr);
            curChordForUi.store (chordIndexAtBeat (*sched, previewBeat));
            playBeatForUi.store (previewBeat);
            previewBeat = std::fmod (previewBeat + blockBeats, sched->totalBeats);
        }
        else
        {
            curChordForUi.store (-1);
            playBeatForUi.store (-1.0);
        }

        // one-shot click-to-audition
        const int idx = auditionReq.exchange (-1);
        if (idx >= 0 && idx < (int) sched->chords.size())
        {
            for (int n : auditionNotes) padMidi.addEvent (juce::MidiMessage::noteOff (1, n), 0);
            auditionNotes.clear();
            for (int n : sched->chords[(size_t) idx].notes)
            {
                padMidi.addEvent (juce::MidiMessage::noteOn (1, n, (juce::uint8) 100), 0);
                auditionNotes.push_back (n);
            }
            auditionSamplesLeft = (int) (sr * 1.3);
        }
        if (auditionSamplesLeft > 0)
        {
            auditionSamplesLeft -= numSamples;
            if (auditionSamplesLeft <= 0)
            {
                for (int n : auditionNotes) padMidi.addEvent (juce::MidiMessage::noteOff (1, n), 0);
                auditionNotes.clear();
            }
        }
    }

    if (prevEnable)
    {
        const int chans = buffer.getNumChannels();
        scratchPad.clear(); scratchPluck.clear(); scratchBass.clear(); scratchCounter.clear();
        padSynth.render     (scratchPad,     padMidi,     0, numSamples);
        pluckSynth.render   (scratchPluck,   pluckMidi,   0, numSamples);
        counterSynth.render (scratchCounter, counterMidi, 0, numSamples);
        bassSynth.render    (scratchBass,    bassMidi,    0, numSamples);

        // wet bus = chords (pad) + arp (pluck) + counter -> chorus + reverb
        for (int ch = 0; ch < chans; ++ch)
        {
            const int sc = juce::jmin (ch, 1);
            buffer.addFrom (ch, 0, scratchPad,     sc, 0, numSamples, gChords);
            buffer.addFrom (ch, 0, scratchPluck,   sc, 0, numSamples, gArp);
            buffer.addFrom (ch, 0, scratchCounter, sc, 0, numSamples, gCounter);
        }

        juce::dsp::AudioBlock<float> block (buffer);
        juce::dsp::ProcessContextReplacing<float> ctx (block);
        chorus.setMix (juce::jlimit (0.0f, 1.0f, chorusMix * 0.6f));
        chorus.process (ctx);

        juce::Reverb::Parameters rp;
        rp.roomSize = 0.80f; rp.damping = 0.32f; rp.width = 1.0f;            // lusher tail
        rp.wetLevel = juce::jlimit (0.0f, 0.9f, reverbMix * 0.65f); rp.dryLevel = 1.0f; rp.freezeMode = 0.0f;
        reverb.setParameters (rp);
        if (chans >= 2) reverb.processStereo (buffer.getWritePointer (0), buffer.getWritePointer (1), numSamples);
        else            reverb.processMono   (buffer.getWritePointer (0), numSamples);

        // bass added dry (kept out of the reverb to stay tight)
        for (int ch = 0; ch < chans; ++ch)
            buffer.addFrom (ch, 0, scratchBass, juce::jmin (ch, 1), 0, numSamples, gBass);

        // sidechain pump (beat-synced) + output gain + master soft-clip, per sample
        if (hostPlaying) pumpPhase = ppq - std::floor (ppq);   // align the pump to the host grid
        const double beatsPerSample = (bpm / 60.0) / sr;
        for (int i = 0; i < numSamples; ++i)
        {
            const float g = (pumpAmt > 0.0f ? tc::pumpGain ((float) pumpPhase, pumpAmt) : 1.0f) * outGain;
            for (int ch = 0; ch < chans; ++ch)
            {
                float* d = buffer.getWritePointer (ch);
                d[i] = tc::softClip (d[i] * g);
            }
            pumpPhase += beatsPerSample;
            if (pumpPhase >= 1.0) pumpPhase -= 1.0;
        }
    }
}

//==============================================================================
// state
//==============================================================================
static juce::ValueTree progToTree (const tc::Progression& p)
{
    juce::ValueTree t ("PROG");
    for (const auto& c : p)
    {
        juce::ValueTree ct ("C");
        ct.setProperty ("r",   c.rootPc,            nullptr);
        ct.setProperty ("t",   (int) c.type,        nullptr);
        ct.setProperty ("b",   c.bassPc,            nullptr);
        ct.setProperty ("d",   c.romanDegree,       nullptr);
        ct.setProperty ("lk",  c.locked,            nullptr);
        ct.setProperty ("sb",  c.startBeat,         nullptr);
        ct.setProperty ("lb",  c.lengthBeats,       nullptr);
        ct.setProperty ("lab", c.label,             nullptr);
        ct.setProperty ("rom", c.roman,             nullptr);
        juce::StringArray iv; for (int x : c.customIntervals) iv.add (juce::String (x));
        ct.setProperty ("iv", iv.joinIntoString (","), nullptr);
        t.appendChild (ct, nullptr);
    }
    return t;
}

static tc::Progression treeToProg (const juce::ValueTree& t)
{
    tc::Progression p;
    for (int i = 0; i < t.getNumChildren(); ++i)
    {
        const auto ct = t.getChild (i);
        tc::Chord c;
        c.rootPc      = (int)  ct.getProperty ("r",  0);
        c.type        = (tc::ChordType) (int) ct.getProperty ("t", 1);
        c.bassPc      = (int)  ct.getProperty ("b",  -1);
        c.romanDegree = (int)  ct.getProperty ("d",  -1);
        c.locked      = (bool) ct.getProperty ("lk", false);
        c.startBeat   = (double) ct.getProperty ("sb", 0.0);
        c.lengthBeats = (double) ct.getProperty ("lb", 4.0);
        c.label       = ct.getProperty ("lab", "").toString();
        c.roman       = ct.getProperty ("rom", "").toString();
        const juce::String ivs = ct.getProperty ("iv", "").toString();
        if (ivs.isNotEmpty())
        {
            juce::StringArray a; a.addTokens (ivs, ",", "");
            for (auto& s : a) if (s.isNotEmpty()) c.customIntervals.push_back (s.getIntValue());
        }
        p.push_back (c);
    }
    return p;
}

static juce::ValueTree melodyToTree (const tc::NoteLayer& m)
{
    juce::ValueTree t ("MELODY");
    for (const auto& n : m)
    {
        juce::ValueTree e ("N");
        e.setProperty ("s", n.startBeat, nullptr); e.setProperty ("l", n.lengthBeats, nullptr);
        e.setProperty ("p", n.note, nullptr);      e.setProperty ("v", n.vel, nullptr);
        t.appendChild (e, nullptr);
    }
    return t;
}

static tc::NoteLayer treeToMelody (const juce::ValueTree& t)
{
    tc::NoteLayer m;
    for (int i = 0; i < t.getNumChildren(); ++i)
    {
        const auto e = t.getChild (i);
        m.push_back ({ (double) e.getProperty ("s", 0.0), (double) e.getProperty ("l", 1.0),
                       (int) e.getProperty ("p", 60), (int) e.getProperty ("v", 96) });
    }
    return m;
}

void TranceChordsAudioProcessor::getStateInformation (juce::MemoryBlock& destData)
{
    juce::ValueTree root ("TC_STATE");
    root.appendChild (apvts.copyState(), nullptr);
    root.appendChild (progToTree (currentProg), nullptr);

    juce::ValueTree favs ("FAVS");
    for (const auto& f : favorites) favs.appendChild (progToTree (f), nullptr);
    root.appendChild (favs, nullptr);

    root.appendChild (melodyToTree (melody), nullptr);

    if (auto xml = root.createXml()) copyXmlToBinary (*xml, destData);
}

void TranceChordsAudioProcessor::setStateInformation (const void* data, int sizeInBytes)
{
    if (auto xml = getXmlFromBinary (data, sizeInBytes))
    {
        const auto root = juce::ValueTree::fromXml (*xml);
        if (root.isValid())
        {
            const auto params = root.getChildWithName (apvts.state.getType());
            if (params.isValid()) apvts.replaceState (params);
            const auto prog = root.getChildWithName ("PROG");
            if (prog.isValid()) currentProg = treeToProg (prog);

            favorites.clear();
            const auto favs = root.getChildWithName ("FAVS");
            if (favs.isValid())
                for (int i = 0; i < favs.getNumChildren(); ++i) favorites.push_back (treeToProg (favs.getChild (i)));

            const auto mel = root.getChildWithName ("MELODY");
            if (mel.isValid()) { melody = treeToMelody (mel); melodyCount.store ((int) melody.size()); }
        }
    }

    if (currentProg.empty())
    {
        generate();
    }
    else
    {
        history.clear(); historyPos = -1; pushHistory();   // restored progression = history baseline
        schedule.store (buildSchedule (currentProg));
        progVersion.fetch_add (1);
    }
}

juce::AudioProcessorEditor* TranceChordsAudioProcessor::createEditor()
{
    return new TranceChordsAudioProcessorEditor (*this);
}

//==============================================================================
juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter()
{
    return new TranceChordsAudioProcessor();
}
