#include "PluginEditor.h"

namespace
{
    const juce::Colour kBg      { 0xff0e1116 };
    const juce::Colour kPanel   { 0xff161c25 };
    const juce::Colour kCard    { 0xff1d2735 };
    const juce::Colour kCardLk  { 0xff2a3a52 };
    const juce::Colour kAccent  { 0xff35d6c8 };   // teal
    const juce::Colour kAccent2 { 0xff8b6cff };   // violet
    const juce::Colour kText    { 0xffe8eef5 };

    bool isWhiteKey (int m) noexcept
    { const int pc = ((m % 12) + 12) % 12; return pc != 1 && pc != 3 && pc != 6 && pc != 8 && pc != 10; }

    // 0..1 harmonic tension from the chord quality
    float chordTension (const tc::Chord& c) noexcept
    {
        using T = tc::ChordType;
        switch (c.type)
        {
            case T::Power5:                    return 0.06f;
            case T::Major: case T::Minor:      return 0.14f;
            case T::MajAdd9: case T::MinAdd9:  return 0.34f;
            case T::Sus2: case T::Sus4:        return 0.42f;
            case T::Min7:                      return 0.50f;
            default:                           return 0.25f;
        }
    }

    // 0..1 brightness (dark minor -> bright major)
    float chordBrightness (const tc::Chord& c) noexcept
    {
        bool maj = false, min = false, maj7 = false;
        for (int iv : c.intervals()) { const int m = ((iv % 12) + 12) % 12; if (m == 4) maj = true; if (m == 3) min = true; if (m == 11) maj7 = true; }
        float b = maj ? 0.78f : (min ? 0.32f : 0.55f);
        if (maj7) b = juce::jmin (1.0f, b + 0.12f);
        return b;
    }
}

//==============================================================================
TranceChordsAudioProcessorEditor::TranceChordsAudioProcessorEditor (TranceChordsAudioProcessor& p)
    : juce::AudioProcessorEditor (&p), proc (p)
{
    lnf.setColourScheme (juce::LookAndFeel_V4::getDarkColourScheme());
    lnf.setColour (juce::ResizableWindow::backgroundColourId, kBg);
    lnf.setColour (juce::Slider::rotarySliderFillColourId,    kAccent);
    lnf.setColour (juce::Slider::rotarySliderOutlineColourId, kPanel);
    lnf.setColour (juce::Slider::thumbColourId,               kAccent);
    lnf.setColour (juce::Slider::textBoxTextColourId,         kText);
    lnf.setColour (juce::Slider::textBoxOutlineColourId,      juce::Colours::transparentBlack);
    lnf.setColour (juce::ComboBox::backgroundColourId,        kPanel);
    lnf.setColour (juce::ComboBox::textColourId,              kText);
    lnf.setColour (juce::ComboBox::outlineColourId,           kCard);
    lnf.setColour (juce::TextButton::buttonColourId,          kPanel);
    lnf.setColour (juce::TextButton::textColourOnId,          kBg);
    lnf.setColour (juce::TextButton::textColourOffId,         kText);
    lnf.setColour (juce::ToggleButton::textColourId,          kText);
    lnf.setColour (juce::ToggleButton::tickColourId,          kAccent);
    lnf.setColour (juce::Label::textColourId,                 kText);
    setLookAndFeel (&lnf);

    // header combos
    setupCombo (keyBox,     ParameterIDs::key,     keyAtt);
    setupCombo (modeBox,    ParameterIDs::mode,    modeAtt);
    setupCombo (sectionBox, ParameterIDs::section, sectionAtt);
    setupCombo (moodBox,    ParameterIDs::mood,    moodAtt);
    setupCombo (styleBox,   ParameterIDs::style,   styleAtt);
    setupCombo (lengthBox,  ParameterIDs::length,  lengthAtt);
    setupCombo (densityBox, ParameterIDs::density, densityAtt);

    // generation knobs (indices 0..5)
    addKnob (ParameterIDs::energy,        "Energy");
    addKnob (ParameterIDs::complexity,    "Complexity");
    addKnob (ParameterIDs::voice_leading, "Voice Lead");
    addKnob (ParameterIDs::variation,     "Variation");
    addKnob (ParameterIDs::humanize,      "Humanize");
    addKnob (ParameterIDs::swing,         "Swing");
    addKnob (ParameterIDs::octave,        "Octave");
    // pad / fx knobs (indices 6..12)
    addKnob (ParameterIDs::prev_attack,   "Attack");
    addKnob (ParameterIDs::prev_release,  "Release");
    addKnob (ParameterIDs::prev_cutoff,   "Cutoff");
    addKnob (ParameterIDs::prev_detune,   "Detune");
    addKnob (ParameterIDs::prev_chorus,   "Chorus");
    addKnob (ParameterIDs::prev_reverb,   "Reverb");
    addKnob (ParameterIDs::pump,          "Pump");
    addKnob (ParameterIDs::output,        "Output");

    // toggles
    addToggle (ParameterIDs::allow_sus,      "Sus");
    addToggle (ParameterIDs::allow_borrowed, "Borrowed");
    addToggle (ParameterIDs::forbid_triads,  "No Triads");
    addToggle (ParameterIDs::no_pop,         "No Pop");
    addToggle (ParameterIDs::scale_lock,     "Scale Lock");
    addToggle (ParameterIDs::modulation,     "Modulation");
    addToggle (ParameterIDs::prev_enable,    "Preview Snd");

    // layer (bass / arp) controls
    layersLabel.setText ("LAYERS", juce::dontSendNotification);
    layersLabel.setFont (juce::Font (juce::FontOptions (12.0f, juce::Font::bold)));
    layersLabel.setColour (juce::Label::textColourId, juce::Colour (0xff7d8aaa));
    addAndMakeVisible (layersLabel);
    addAndMakeVisible (bassEnableBtn);
    bassEnableAtt = std::make_unique<BAtt> (proc.apvts, ParameterIDs::bass_enable, bassEnableBtn);
    addAndMakeVisible (arpEnableBtn);
    arpEnableAtt = std::make_unique<BAtt> (proc.apvts, ParameterIDs::arp_enable, arpEnableBtn);
    setupCombo (bassPatternBox, ParameterIDs::bass_pattern, bassPatternAtt);
    setupCombo (arpPatternBox,  ParameterIDs::arp_pattern,  arpPatternAtt);
    setupCombo (arpRateBox,     ParameterIDs::arp_rate,     arpRateAtt);
    addAndMakeVisible (counterEnableBtn);
    counterEnableAtt = std::make_unique<BAtt> (proc.apvts, ParameterIDs::counter_enable, counterEnableBtn);
    setupCombo (counterPatternBox, ParameterIDs::counter_pattern, counterPatternAtt);
    setupCombo (counterRateBox,    ParameterIDs::counter_rate,    counterRateAtt);

    // layer mixer
    mixLabel.setText ("MIX", juce::dontSendNotification);
    mixLabel.setFont (juce::Font (juce::FontOptions (12.0f, juce::Font::bold)));
    mixLabel.setColour (juce::Label::textColourId, juce::Colour (0xff7d8aaa));
    addAndMakeVisible (mixLabel);
    auto setupMix = [this] (juce::Slider& s, const char* id, std::unique_ptr<SAtt>& att, const juce::String& nm)
    {
        s.setSliderStyle (juce::Slider::LinearHorizontal);
        s.setTextBoxStyle (juce::Slider::TextBoxRight, false, 40, 16);
        s.setName (nm);
        addAndMakeVisible (s);
        att = std::make_unique<SAtt> (proc.apvts, id, s);
    };
    setupMix (mixChordsSlider,  ParameterIDs::mix_chords,  mixChordsAtt,  "Chords");
    setupMix (mixBassSlider,    ParameterIDs::mix_bass,    mixBassAtt,    "Bass");
    setupMix (mixArpSlider,     ParameterIDs::mix_arp,     mixArpAtt,     "Arp");
    setupMix (mixCounterSlider, ParameterIDs::mix_counter, mixCounterAtt, "Counter");

    // melody-aware controls
    addAndMakeVisible (melodyFitBtn);
    melodyFitAtt = std::make_unique<BAtt> (proc.apvts, ParameterIDs::melody_fit, melodyFitBtn);
    melodyLabel.setText ("Drop a .mid melody here to fit chords to it", juce::dontSendNotification);
    melodyLabel.setColour (juce::Label::textColourId, juce::Colour (0xff7d8aaa));
    melodyLabel.setFont (juce::Font (juce::FontOptions (12.0f)));
    addAndMakeVisible (melodyLabel);
    clearMelodyBtn.onClick = [this] { proc.clearMelody(); };
    addAndMakeVisible (clearMelodyBtn);

    // buttons
    generateBtn.setColour (juce::TextButton::buttonColourId, kAccent);
    generateBtn.onClick = [this] { proc.generate(); };
    addAndMakeVisible (generateBtn);

    playBtn.setColour (juce::TextButton::buttonColourId, kAccent2);
    playBtn.onClick = [this]
    {
        const bool now = ! proc.isPreviewPlaying();
        proc.setPreviewPlaying (now);
        playBtn.setButtonText (now ? "STOP" : "PLAY");
    };
    addAndMakeVisible (playBtn);

    presetBox.addItemList ({ "Dreamy Verse", "Romantic Verse", "Uplifting Anthem",
                             "Heavenly Chorus", "Pre-Chorus Build", "Dark Phrygian",
                             "Oldschool 2000", "Psy Roller", "Festival Anthem",
                             "Progressive Deep", "ASOT Uplifter", "Euphoric Drop",
                             "Vocal Verse", "Breakdown Pad", "Song: Uplifting", "Song: Festival" }, 1);
    presetBox.setTextWhenNothingSelected ("Presets...");
    presetBox.onChange = [this] { const int i = presetBox.getSelectedItemIndex(); if (i >= 0) proc.applyPreset (i); };
    addAndMakeVisible (presetBox);

    // history + favorites
    undoBtn.setButtonText (juce::String::fromUTF8 ("\xE2\x97\x80"));  // left triangle
    redoBtn.setButtonText (juce::String::fromUTF8 ("\xE2\x96\xB6")); // right triangle
    undoBtn.onClick = [this] { proc.undo(); };
    redoBtn.onClick = [this] { proc.redo(); };
    addAndMakeVisible (undoBtn);
    addAndMakeVisible (redoBtn);
    saveFavBtn.setButtonText (juce::String::fromUTF8 ("\xE2\x98\x85") + juce::String (" Fav")); // star
    saveFavBtn.onClick = [this] { proc.saveFavorite(); refreshFavorites(); };
    addAndMakeVisible (saveFavBtn);
    favoritesBox.setTextWhenNothingSelected ("Favorites...");
    favoritesBox.onChange = [this] { const int i = favoritesBox.getSelectedItemIndex(); if (i >= 0) proc.recallFavorite (i); };
    addAndMakeVisible (favoritesBox);

    addAndMakeVisible (songModeBtn);
    songModeAtt = std::make_unique<BAtt> (proc.apvts, ParameterIDs::song_mode, songModeBtn);
    setupCombo (songFormBox, ParameterIDs::song_form, songFormAtt);
    setupCombo (voicingStyleBox, ParameterIDs::voicing_style, voicingStyleAtt);

    titleLabel.setText ("TRANCECHORDS", juce::dontSendNotification);
    titleLabel.setFont (juce::Font (juce::FontOptions (20.0f, juce::Font::bold)));
    titleLabel.setColour (juce::Label::textColourId, kAccent);
    addAndMakeVisible (titleLabel);

    bpmLabel.setJustificationType (juce::Justification::centredRight);
    addAndMakeVisible (bpmLabel);

    auto setupDrag = [this] (DragStrip& d, const juce::String& txt, int layer)
    {
        d.text = txt; d.layer = layer;
        d.onDrag = [this] (DragStrip* s)
        {
            const auto file = proc.exportTempMidi (s->layer);
            if (file.existsAsFile())
            {
                juce::StringArray files; files.add (file.getFullPathName());
                performExternalDragDropOfFiles (files, false, s, nullptr);
            }
        };
        addAndMakeVisible (d);
    };
    setupDrag (dragChords,  "CHORDS",  0);
    setupDrag (dragBass,    "BASS",    1);
    setupDrag (dragArp,     "ARP",     2);
    setupDrag (dragCounter, "COUNTER", 3);

    // regenerate when structural params change; revoice when voicing params change
    for (auto* id : { ParameterIDs::key, ParameterIDs::mode, ParameterIDs::section,
                      ParameterIDs::mood, ParameterIDs::style, ParameterIDs::length, ParameterIDs::density,
                      ParameterIDs::allow_sus, ParameterIDs::allow_borrowed,
                      ParameterIDs::forbid_triads, ParameterIDs::no_pop,
                      ParameterIDs::scale_lock, ParameterIDs::modulation, ParameterIDs::sec_dominants,
                      ParameterIDs::melody_fit, ParameterIDs::song_mode, ParameterIDs::song_form })
        proc.apvts.addParameterListener (id, this);
    for (auto* id : { ParameterIDs::voice_leading, ParameterIDs::octave, ParameterIDs::swing, ParameterIDs::voicing_style,
                      ParameterIDs::bass_pattern, ParameterIDs::bass_octave, ParameterIDs::bass_gate,
                      ParameterIDs::arp_pattern, ParameterIDs::arp_rate, ParameterIDs::arp_octaves, ParameterIDs::arp_gate,
                      ParameterIDs::counter_pattern, ParameterIDs::counter_rate })
        proc.apvts.addParameterListener (id, this);

    setResizable (true, true);
    if (auto* c = getConstrainer()) c->setSizeLimits (940, 740, 1500, 1100);
    setSize (1000, 760);
    startTimerHz (30);
}

TranceChordsAudioProcessorEditor::~TranceChordsAudioProcessorEditor()
{
    for (auto* id : { ParameterIDs::key, ParameterIDs::mode, ParameterIDs::section,
                      ParameterIDs::mood, ParameterIDs::style, ParameterIDs::length, ParameterIDs::density,
                      ParameterIDs::allow_sus, ParameterIDs::allow_borrowed,
                      ParameterIDs::forbid_triads, ParameterIDs::no_pop,
                      ParameterIDs::scale_lock, ParameterIDs::modulation, ParameterIDs::sec_dominants,
                      ParameterIDs::melody_fit, ParameterIDs::song_mode, ParameterIDs::song_form,
                      ParameterIDs::voice_leading, ParameterIDs::octave, ParameterIDs::swing, ParameterIDs::voicing_style,
                      ParameterIDs::bass_pattern, ParameterIDs::bass_octave, ParameterIDs::bass_gate,
                      ParameterIDs::arp_pattern, ParameterIDs::arp_rate, ParameterIDs::arp_octaves, ParameterIDs::arp_gate,
                      ParameterIDs::counter_pattern, ParameterIDs::counter_rate })
        proc.apvts.removeParameterListener (id, this);

    setLookAndFeel (nullptr);
}

//==============================================================================
TranceChordsAudioProcessorEditor::Knob& TranceChordsAudioProcessorEditor::addKnob (const char* paramId, const juce::String& name)
{
    auto k = std::make_unique<Knob>();
    k->slider.setSliderStyle (juce::Slider::RotaryHorizontalVerticalDrag);
    k->slider.setTextBoxStyle (juce::Slider::TextBoxBelow, false, 64, 16);
    addAndMakeVisible (k->slider);

    k->label.setText (name, juce::dontSendNotification);
    k->label.setJustificationType (juce::Justification::centred);
    k->label.setFont (juce::Font (juce::FontOptions (12.0f)));
    addAndMakeVisible (k->label);

    k->att = std::make_unique<SAtt> (proc.apvts, paramId, k->slider);
    knobs.push_back (std::move (k));
    return *knobs.back();
}

void TranceChordsAudioProcessorEditor::setupCombo (juce::ComboBox& box, const char* paramId, std::unique_ptr<CAtt>& att)
{
    if (auto* cp = dynamic_cast<juce::AudioParameterChoice*> (proc.apvts.getParameter (paramId)))
        box.addItemList (cp->choices, 1);
    addAndMakeVisible (box);
    att = std::make_unique<CAtt> (proc.apvts, paramId, box);
}

void TranceChordsAudioProcessorEditor::addToggle (const char* paramId, const juce::String& name)
{
    auto t = std::make_unique<juce::ToggleButton> (name);
    addAndMakeVisible (*t);
    toggleAtts.push_back (std::make_unique<BAtt> (proc.apvts, paramId, *t));
    toggles.push_back (std::move (t));
}

//==============================================================================
void TranceChordsAudioProcessorEditor::paint (juce::Graphics& g)
{
    g.fillAll (kBg);

    // header + panel backgrounds
    g.setColour (kPanel);
    g.fillRect (0, 0, getWidth(), 84);

    // chord cards
    const auto& prog = proc.progression();
    const int playing = proc.playheadChordIndex();
    g.setColour (juce::Colour (0xff7d8aaa));
    g.setFont (juce::Font (juce::FontOptions (12.0f, juce::Font::bold)));
    g.drawText ("PROGRESSION", cardArea.getX(), cardArea.getY() - 18, 200, 16, juce::Justification::left);

    for (int i = 0; i < (int) cardRects.size() && i < (int) prog.size(); ++i)
    {
        const auto r = cardRects[(size_t) i].toFloat();
        const auto& c = prog[(size_t) i];

        g.setColour (c.locked ? kCardLk : kCard);
        g.fillRoundedRectangle (r, 6.0f);
        if (i == playing)
        {
            g.setColour (kAccent);
            g.drawRoundedRectangle (r.reduced (1.0f), 6.0f, 2.0f);
        }
        else if (c.locked)
        {
            g.setColour (kAccent2);
            g.drawRoundedRectangle (r.reduced (1.0f), 6.0f, 1.4f);
        }

        g.setColour (kText);
        g.setFont (juce::Font (juce::FontOptions (juce::jmin (22.0f, r.getWidth() * 0.30f), juce::Font::bold)));
        g.drawText (c.label, r.reduced (4.0f).withTrimmedBottom (r.getHeight() * 0.32f),
                    juce::Justification::centred, false);

        g.setColour (kAccent);
        g.setFont (juce::Font (juce::FontOptions (13.0f)));
        g.drawText (c.roman, r.withTrimmedTop (r.getHeight() * 0.70f), juce::Justification::centred, false);

        // lock glyph
        g.setColour (c.locked ? kAccent2 : juce::Colour (0xff44506a));
        g.drawText (c.locked ? juce::String::fromUTF8 ("\xE2\x97\x8F") : juce::String::fromUTF8 ("\xE2\x97\x8B"),
                    cardRects[(size_t) i].getRight() - 16, cardRects[(size_t) i].getY() + 3, 14, 14,
                    juce::Justification::centred);
    }

    // mixer slider names (drawn to the left of each fader)
    g.setColour (juce::Colour (0xff7d8aaa));
    g.setFont (juce::Font (juce::FontOptions (11.0f)));
    for (auto* s : { &mixChordsSlider, &mixBassSlider, &mixArpSlider, &mixCounterSlider })
        g.drawText (s->getName(), s->getX() - 46, s->getY() - 1, 42, s->getHeight(), juce::Justification::centredRight);

    // group labels for control sections
    g.setColour (juce::Colour (0xff7d8aaa));
    g.setFont (juce::Font (juce::FontOptions (12.0f, juce::Font::bold)));
    if (! knobs.empty())
    {
        g.drawText ("PROGRESSION ENGINE", 12, knobs[0]->slider.getY() - 18, 240, 16, juce::Justification::left);
        if (knobs.size() > 7)
            g.drawText ("PAD / FX", knobs[7]->slider.getX(), knobs[7]->slider.getY() - 18, 240, 16, juce::Justification::left);
    }

    g.setColour (juce::Colour (0xff7d8aaa));
    g.setFont (juce::Font (juce::FontOptions (12.0f, juce::Font::bold)));
    g.drawText ("TIMELINE", timelineArea.getX(), timelineArea.getY() - 16, 120, 14, juce::Justification::left);
    g.drawText ("KEYS",     keyboardArea.getX(), keyboardArea.getY() - 16, 120, 14, juce::Justification::left);
    drawTimeline (g);
    drawMeters (g);
    drawKeyboard (g);
}

void TranceChordsAudioProcessorEditor::DragStrip::paint (juce::Graphics& g)
{
    auto r = getLocalBounds().toFloat().reduced (1.0f);
    g.setColour (juce::Colour (0xff20304a));
    g.fillRoundedRectangle (r, 6.0f);
    g.setColour (juce::Colour (0xff35d6c8));
    g.drawRoundedRectangle (r, 6.0f, 1.4f);
    g.setColour (juce::Colour (0xffe8eef5));
    g.setFont (juce::Font (juce::FontOptions (13.0f, juce::Font::bold)));
    g.drawText (juce::String::fromUTF8 ("\xE2\xA0\xBF  DRAG ") + text, getLocalBounds(), juce::Justification::centred);
}

//==============================================================================
void TranceChordsAudioProcessorEditor::drawTimeline (juce::Graphics& g)
{
    g.setColour (kPanel);
    g.fillRoundedRectangle (timelineArea.toFloat(), 4.0f);

    const auto& prog = proc.progression();
    const double total = juce::jmax (1.0, proc.scheduleTotalBeats());
    const auto r = timelineArea.toFloat().reduced (2.0f);
    const int playing = proc.playheadChordIndex();

    for (int i = 0; i < (int) prog.size(); ++i)
    {
        const auto& c = prog[(size_t) i];
        const float x = r.getX() + (float) (c.startBeat / total) * r.getWidth();
        const float w = juce::jmax (1.0f, (float) (c.lengthBeats / total) * r.getWidth());
        const float bright = chordBrightness (c);
        g.setColour (kCard.interpolatedWith (kAccent, 0.10f + 0.5f * bright).withAlpha (i == playing ? 0.95f : 0.6f));
        g.fillRect (x + 0.5f, r.getY(), w - 1.0f, r.getHeight());
    }
    // smooth playhead line
    const double beat = proc.playheadBeat();
    if (beat >= 0.0)
    {
        const float px = r.getX() + (float) (beat / total) * r.getWidth();
        g.setColour (kText);
        g.fillRect (px, r.getY(), 1.5f, r.getHeight());
    }
    g.setColour (juce::Colour (0xff44506a));
    g.drawRoundedRectangle (timelineArea.toFloat(), 4.0f, 1.0f);
}

void TranceChordsAudioProcessorEditor::drawKeyboard (juce::Graphics& g)
{
    g.setColour (kBg);
    g.fillRoundedRectangle (keyboardArea.toFloat(), 4.0f);

    const int lo = 36, hi = 84;                       // C2..C6
    int idx = proc.playheadChordIndex(); if (idx < 0) idx = 0;
    const auto chord = proc.chordNotesAt (idx);
    const int mel = proc.activeMelodyNote();
    auto inChord = [&chord] (int m) { return std::find (chord.begin(), chord.end(), m) != chord.end(); };

    const auto area = keyboardArea.toFloat().reduced (2.0f);
    int nWhite = 0; for (int m = lo; m <= hi; ++m) if (isWhiteKey (m)) ++nWhite;
    const float w = area.getWidth() / (float) juce::jmax (1, nWhite);

    std::vector<float> xByMidi ((size_t) (hi + 1), -1.0f);
    int wi = 0;
    for (int m = lo; m <= hi; ++m)
    {
        if (! isWhiteKey (m)) continue;
        const float x = area.getX() + wi * w;
        xByMidi[(size_t) m] = x;
        g.setColour (m == mel ? kAccent2 : (inChord (m) ? kAccent : juce::Colour (0xffe9eef5)));
        g.fillRect (x + 0.5f, area.getY(), w - 1.0f, area.getHeight());
        g.setColour (juce::Colour (0xff223047));
        g.drawRect (x, area.getY(), w, area.getHeight(), 0.7f);
        ++wi;
    }
    // black keys over the gaps
    const float bw = w * 0.62f, bh = area.getHeight() * 0.62f;
    for (int m = lo; m <= hi; ++m)
    {
        if (isWhiteKey (m)) continue;
        const float below = xByMidi[(size_t) (m - 1)];
        if (below < 0.0f) continue;
        const float x = below + w - bw * 0.5f;
        g.setColour (m == mel ? kAccent2 : (inChord (m) ? kAccent.darker (0.2f) : juce::Colour (0xff10151c)));
        g.fillRect (x, area.getY(), bw, bh);
    }
}

void TranceChordsAudioProcessorEditor::drawMeters (juce::Graphics& g)
{
    const auto& prog = proc.progression();
    int idx = proc.playheadChordIndex(); if (idx < 0) idx = 0;
    float tension = 0.0f, bright = 0.0f;
    if (! prog.empty() && idx < (int) prog.size())
    {
        tension = chordTension (prog[(size_t) idx]);
        bright  = chordBrightness (prog[(size_t) idx]);
    }

    auto bar = [&g] (juce::Rectangle<int> r, const juce::String& nm, float v, juce::Colour col)
    {
        g.setColour (juce::Colour (0xff7d8aaa));
        g.setFont (juce::Font (juce::FontOptions (10.0f)));
        g.drawText (nm, r.removeFromLeft (52), juce::Justification::centredLeft);
        g.setColour (kPanel);  g.fillRoundedRectangle (r.toFloat(), 3.0f);
        auto fill = r.toFloat().reduced (1.0f); fill.setWidth (fill.getWidth() * juce::jlimit (0.0f, 1.0f, v));
        g.setColour (col);     g.fillRoundedRectangle (fill, 3.0f);
    };
    auto a = meterArea;
    bar (a.removeFromTop (a.getHeight() / 2).reduced (0, 1), "Tension", tension, juce::Colour (0xffff6b6b));
    bar (a.reduced (0, 1), "Bright", bright, kAccent);
}

//==============================================================================
void TranceChordsAudioProcessorEditor::layoutCards()
{
    cardRects.clear();
    const int n = juce::jmax (1, (int) proc.progression().size());
    const int gap = 6;
    const int perRow = (n <= 8) ? n : (n <= 16 ? 8 : 12);   // wrap long songs tighter
    const int rows = (n + perRow - 1) / perRow;
    const int cw = (cardArea.getWidth() - gap * (perRow - 1)) / perRow;
    const int ch = (cardArea.getHeight() - gap * (rows - 1)) / rows;

    for (int i = 0; i < n; ++i)
    {
        const int row = i / perRow, col = i % perRow;
        cardRects.push_back ({ cardArea.getX() + col * (cw + gap),
                               cardArea.getY() + row * (ch + gap), cw, ch });
    }
}

void TranceChordsAudioProcessorEditor::resized()
{
    auto area = getLocalBounds();

    // ---- header ----
    auto header = area.removeFromTop (84);
    titleLabel.setBounds (header.removeFromLeft (160).withTrimmedLeft (12));
    auto hrow = header.reduced (6, 14);
    auto cell = [&hrow] (int w) { auto r = hrow.removeFromLeft (w); hrow.removeFromLeft (6); return r; };
    keyBox.setBounds     (cell (58));
    modeBox.setBounds    (cell (116));
    sectionBox.setBounds (cell (98));
    moodBox.setBounds    (cell (90));
    styleBox.setBounds   (cell (120));
    lengthBox.setBounds  (cell (78));
    densityBox.setBounds (cell (78));
    bpmLabel.setBounds   (hrow.removeFromRight (84));

    area.removeFromTop (6);

    // ---- action row ----
    auto actions = area.removeFromTop (40).reduced (12, 4);
    generateBtn.setBounds (actions.removeFromLeft (112)); actions.removeFromLeft (6);
    undoBtn.setBounds (actions.removeFromLeft (28));      actions.removeFromLeft (3);
    redoBtn.setBounds (actions.removeFromLeft (28));      actions.removeFromLeft (8);
    playBtn.setBounds (actions.removeFromLeft (66));      actions.removeFromLeft (8);
    presetBox.setBounds (actions.removeFromLeft (120));   actions.removeFromLeft (8);
    songModeBtn.setBounds (actions.removeFromLeft (58));  actions.removeFromLeft (4);
    songFormBox.setBounds (actions.removeFromLeft (132)); actions.removeFromLeft (8);
    voicingStyleBox.setBounds (actions.removeFromLeft (92)); actions.removeFromLeft (10);
    saveFavBtn.setBounds (actions.removeFromLeft (54));   actions.removeFromLeft (6);
    favoritesBox.setBounds (actions);

    // ---- chord cards ----
    area.removeFromTop (20);
    cardArea = area.removeFromTop (116).reduced (12, 0);
    layoutCards();

    // ---- melody row (drop a .mid here) ----
    area.removeFromTop (8);
    {
        auto row = area.removeFromTop (24).reduced (12, 0);
        melodyFitBtn.setBounds (row.removeFromLeft (96));
        row.removeFromLeft (8);
        clearMelodyBtn.setBounds (row.removeFromRight (64));
        row.removeFromRight (8);
        melodyLabel.setBounds (row);
    }

    // ---- layers row (bass / arp / counter) ----
    area.removeFromTop (6);
    {
        auto row = area.removeFromTop (26).reduced (12, 0);
        layersLabel.setBounds (row.removeFromLeft (54));
        auto lcell = [&row] (int w) { auto r = row.removeFromLeft (w); row.removeFromLeft (6); return r; };
        bassEnableBtn.setBounds     (lcell (56));
        bassPatternBox.setBounds    (lcell (118));
        row.removeFromLeft (12);
        arpEnableBtn.setBounds      (lcell (50));
        arpPatternBox.setBounds     (lcell (104));
        arpRateBox.setBounds        (lcell (66));
        row.removeFromLeft (12);
        counterEnableBtn.setBounds  (lcell (74));
        counterPatternBox.setBounds (lcell (104));
        counterRateBox.setBounds    (lcell (66));
    }

    // ---- mixer row (per-layer preview levels) ----
    area.removeFromTop (6);
    {
        auto row = area.removeFromTop (22).reduced (12, 0);
        mixLabel.setBounds (row.removeFromLeft (34));
        const int seg = row.getWidth() / 4;
        auto mixCell = [&row, seg] (juce::Slider& s) { auto c = row.removeFromLeft (seg); c.removeFromLeft (46); s.setBounds (c.reduced (2, 1)); };
        mixCell (mixChordsSlider); mixCell (mixBassSlider); mixCell (mixArpSlider); mixCell (mixCounterSlider);
    }

    // ---- four drag strips: chords | bass | arp | counter ----
    area.removeFromTop (8);
    {
        auto row = area.removeFromTop (34).reduced (12, 0);
        const int w = (row.getWidth() - 24) / 4;
        dragChords.setBounds  (row.removeFromLeft (w)); row.removeFromLeft (8);
        dragBass.setBounds    (row.removeFromLeft (w)); row.removeFromLeft (8);
        dragArp.setBounds     (row.removeFromLeft (w)); row.removeFromLeft (8);
        dragCounter.setBounds (row);
    }

    // ---- bottom visualization: timeline + meters, then keyboard ----
    {
        auto viz = area.removeFromBottom (50 + 6 + 22);
        keyboardArea = viz.removeFromBottom (50).reduced (12, 0);
        viz.removeFromBottom (6);
        auto tm = viz.reduced (12, 0);
        meterArea = tm.removeFromRight (200);
        tm.removeFromRight (12);
        timelineArea = tm;
    }

    // ---- controls: knobs grid + toggles ----
    area.removeFromTop (16);
    auto controls = area.reduced (12, 0);

    // toggles on the right
    auto togCol = controls.removeFromRight (150);
    {
        int y = togCol.getY();
        for (auto& t : toggles) { t->setBounds (togCol.getX(), y, togCol.getWidth(), 24); y += 26; }
    }

    // knobs: 7 generation (top row) + 7 pad (bottom row)
    const int kw = 84, kh = 74;
    auto placeRow = [&] (int first, int count, juce::Rectangle<int> row)
    {
        int x = row.getX();
        for (int i = first; i < first + count && i < (int) knobs.size(); ++i)
        {
            knobs[(size_t) i]->slider.setBounds (x, row.getY(), kw, kh);
            knobs[(size_t) i]->label.setBounds (x, row.getY() + kh, kw, 16);
            x += kw + 6;
        }
    };
    auto top = controls.removeFromTop (kh + 22);
    placeRow (0, 7, top);
    controls.removeFromTop (10);
    placeRow (7, 8, controls.removeFromTop (kh + 22));
}

//==============================================================================
void TranceChordsAudioProcessorEditor::mouseDown (const juce::MouseEvent& e)
{
    for (int i = 0; i < (int) cardRects.size(); ++i)
    {
        if (cardRects[(size_t) i].contains (e.getPosition()))
        {
            const auto lockArea = juce::Rectangle<int> (cardRects[(size_t) i].getRight() - 18,
                                                        cardRects[(size_t) i].getY() + 1, 18, 18);
            if (lockArea.contains (e.getPosition())) proc.toggleLock (i);
            else                                     proc.auditionChord (i);
            repaint();
            return;
        }
    }
}

//==============================================================================
void TranceChordsAudioProcessorEditor::timerCallback()
{
    if (needsRegen.exchange (false))      proc.generate();
    else if (needsRevoice.exchange (false)) proc.revoice();

    const int v = proc.progressionVersion();
    if (v != lastVersion)
    {
        lastVersion = v;
        layoutCards();
        repaint();
    }
    else if (proc.playheadBeat() >= 0.0)
    {
        // moving playhead + live keyboard/meters while playing
        repaint (cardArea);
        repaint (timelineArea);
        repaint (keyboardArea);
        repaint (meterArea);
    }

    bpmLabel.setText (juce::String (proc.currentBpm(), 1) + " BPM", juce::dontSendNotification);
    if (! proc.isPreviewPlaying() && playBtn.getButtonText() == "STOP")
        playBtn.setButtonText ("PLAY");

    const int mn = proc.melodyNoteCount();
    melodyLabel.setText (mn > 0 ? "Melody: " + juce::String (mn) + " notes - chords fit to it"
                                : juce::String ("Drop a .mid melody here to fit chords to it"),
                         juce::dontSendNotification);
    clearMelodyBtn.setEnabled (mn > 0);

    undoBtn.setEnabled (proc.canUndo());
    redoBtn.setEnabled (proc.canRedo());
    if (proc.numFavorites() != lastFavCount) refreshFavorites();
}

void TranceChordsAudioProcessorEditor::refreshFavorites()
{
    favoritesBox.clear (juce::dontSendNotification);
    for (int i = 0; i < proc.numFavorites(); ++i)
        favoritesBox.addItem ("Fav " + juce::String (i + 1) + ": " + proc.favoriteLabel (i), i + 1);
    lastFavCount = proc.numFavorites();
}

bool TranceChordsAudioProcessorEditor::isInterestedInFileDrag (const juce::StringArray& files)
{
    for (const auto& f : files)
        if (f.endsWithIgnoreCase (".mid") || f.endsWithIgnoreCase (".midi")) return true;
    return false;
}

void TranceChordsAudioProcessorEditor::filesDropped (const juce::StringArray& files, int, int)
{
    for (const auto& f : files)
        if (f.endsWithIgnoreCase (".mid") || f.endsWithIgnoreCase (".midi"))
        {
            const auto m = tc::parseMelodyFile (juce::File (f));
            if (! m.empty()) { proc.setMelody (m); break; }
        }
    repaint();
}

void TranceChordsAudioProcessorEditor::parameterChanged (const juce::String& id, float)
{
    // structural params re-roll the chords; everything else we listen to re-voices
    // (rebuilds voicing + bass/arp layers, keeping the same chords).
    if (id == ParameterIDs::key || id == ParameterIDs::mode || id == ParameterIDs::section
        || id == ParameterIDs::mood || id == ParameterIDs::style || id == ParameterIDs::length
        || id == ParameterIDs::density || id == ParameterIDs::allow_sus || id == ParameterIDs::allow_borrowed
        || id == ParameterIDs::forbid_triads || id == ParameterIDs::no_pop
        || id == ParameterIDs::scale_lock || id == ParameterIDs::modulation || id == ParameterIDs::sec_dominants
        || id == ParameterIDs::melody_fit || id == ParameterIDs::song_mode || id == ParameterIDs::song_form)
        needsRegen.store (true);
    else
        needsRevoice.store (true);
}
