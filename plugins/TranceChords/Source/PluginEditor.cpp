#include "PluginEditor.h"

namespace
{
    const juce::Colour kBg      { 0xff12161d };   // dark base
    const juce::Colour kPanel   { 0xff1a1f28 };   // panels one step lighter
    const juce::Colour kCard    { 0xff222834 };   // cards one more step
    const juce::Colour kCardLk  { 0xff2b3342 };   // locked card (neutral, not accent)
    const juce::Colour kBorder  { 0xff2c3340 };   // 1px borders
    const juce::Colour kAccent  { 0xff35d6c8 };   // the ONE accent (teal)
    const juce::Colour kText    { 0xffe8eef5 };
    const juce::Colour kDim     { 0xff6b7686 };   // low-contrast labels

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
void TranceChordsAudioProcessorEditor::KnobLnf::drawRotarySlider (juce::Graphics& g, int x, int y, int w, int h,
    float pos, float startAngle, float endAngle, juce::Slider& s)
{
    const auto b = juce::Rectangle<int> (x, y, w, h).toFloat().reduced (2.0f);
    const float radius = juce::jmin (b.getWidth(), b.getHeight()) * 0.5f;
    const float cx = b.getCentreX(), cy = b.getCentreY();
    const float thick = juce::jmax (2.5f, radius * 0.16f);
    const float pr = radius - thick;
    const float angle = startAngle + pos * (endAngle - startAngle);

    juce::Path track;   track.addCentredArc (cx, cy, pr, pr, 0.0f, startAngle, endAngle, true);
    g.setColour (panel.brighter (0.18f));
    g.strokePath (track, juce::PathStrokeType (thick, juce::PathStrokeType::curved, juce::PathStrokeType::rounded));

    juce::Path val;     val.addCentredArc (cx, cy, pr, pr, 0.0f, startAngle, angle, true);
    g.setColour (s.isEnabled() ? accent : dim);
    g.strokePath (val, juce::PathStrokeType (thick, juce::PathStrokeType::curved, juce::PathStrokeType::rounded));

    const juce::Point<float> tip  (cx + pr * std::sin (angle),         cy - pr * std::cos (angle));
    const juce::Point<float> root (cx + pr * 0.4f * std::sin (angle),  cy - pr * 0.4f * std::cos (angle));
    g.setColour (text);
    g.drawLine ({ root, tip }, juce::jmax (1.5f, thick * 0.45f));

    g.setFont (juce::Font (juce::FontOptions (juce::jmax (9.0f, radius * 0.44f), juce::Font::bold)));
    g.setColour (text);
    g.drawText (s.getTextFromValue (s.getValue()),
                juce::Rectangle<float> (cx - radius, cy - radius * 0.42f, radius * 2.0f, radius * 0.84f),
                juce::Justification::centred, false);
}

//==============================================================================
TranceChordsAudioProcessorEditor::TranceChordsAudioProcessorEditor (TranceChordsAudioProcessor& p)
    : juce::AudioProcessorEditor (&p), proc (p)
{
    lnf.accent = kAccent; lnf.panel = kPanel; lnf.text = kText; lnf.dim = kDim;
    lnf.setColourScheme (juce::LookAndFeel_V4::getDarkColourScheme());
    lnf.setColour (juce::ResizableWindow::backgroundColourId, kBg);
    lnf.setColour (juce::Slider::trackColourId,               kAccent);            // linear-fader fill
    lnf.setColour (juce::Slider::backgroundColourId,          kPanel.brighter (0.12f));
    lnf.setColour (juce::Slider::textBoxTextColourId,         kText);
    lnf.setColour (juce::Slider::textBoxOutlineColourId,      juce::Colours::transparentBlack);
    lnf.setColour (juce::ComboBox::backgroundColourId,        kPanel.brighter (0.06f));
    lnf.setColour (juce::ComboBox::textColourId,              kText);
    lnf.setColour (juce::ComboBox::outlineColourId,           kBorder);
    lnf.setColour (juce::ComboBox::arrowColourId,             kDim);
    lnf.setColour (juce::PopupMenu::backgroundColourId,       kPanel);
    lnf.setColour (juce::PopupMenu::highlightedBackgroundColourId, kAccent.withAlpha (0.25f));
    lnf.setColour (juce::TextButton::buttonColourId,          kPanel.brighter (0.06f));
    lnf.setColour (juce::TextButton::textColourOnId,          kBg);
    lnf.setColour (juce::TextButton::textColourOffId,         kText);
    lnf.setColour (juce::ToggleButton::textColourId,          kText);
    lnf.setColour (juce::ToggleButton::tickColourId,          kAccent);
    lnf.setColour (juce::ToggleButton::tickDisabledColourId,  kBorder);
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

    // layer (bass / arp / counter) controls — laid out inside layer cards
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

    // per-layer level faders (live inside the layer cards)
    auto setupMix = [this] (juce::Slider& s, const char* id, std::unique_ptr<SAtt>& att, const juce::String& nm)
    {
        s.setSliderStyle (juce::Slider::LinearHorizontal);
        s.setTextBoxStyle (juce::Slider::NoTextBox, false, 0, 0);
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

    // buttons — accent is reserved for GENERATE (the primary action)
    generateBtn.setColour (juce::TextButton::buttonColourId, kAccent);
    generateBtn.setColour (juce::TextButton::textColourOnId,  kBg);
    generateBtn.onClick = [this] { proc.generate(); };
    addAndMakeVisible (generateBtn);

    playBtn.setColour (juce::TextButton::buttonColourId, kPanel.brighter (0.12f));
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
    setupCombo (voicingStyleBox, ParameterIDs::voicing_style, voicingStyleAtt);   // lives on the GENERATOR tab

    // tabbed sound section
    auto setupTab = [this] (juce::TextButton& b, int idx)
    {
        b.onClick = [this, idx] { activeTab = idx; updateTabVisibility(); repaint(); };
        addAndMakeVisible (b);
    };
    setupTab (genTabBtn, 0);
    setupTab (toneTabBtn, 1);

    // collapsible keyboard disclosure (grows the window by 60px when open)
    keyboardBtn.setButtonText (juce::String::fromUTF8 ("\xE2\x96\xB8") + juce::String (" Keyboard"));
    keyboardBtn.onClick = [this]
    {
        keyboardOpen = ! keyboardOpen;
        keyboardBtn.setButtonText (juce::String::fromUTF8 (keyboardOpen ? "\xE2\x96\xBE" : "\xE2\x96\xB8") + juce::String (" Keyboard"));
        setSize (980, keyboardOpen ? 700 : 640);
    };
    addAndMakeVisible (keyboardBtn);

    titleLabel.setText ("TRANCECHORDS", juce::dontSendNotification);
    titleLabel.setFont (juce::Font (juce::FontOptions (20.0f, juce::Font::bold)));
    titleLabel.setColour (juce::Label::textColourId, kAccent);
    addAndMakeVisible (titleLabel);

    bpmLabel.setJustificationType (juce::Justification::centredRight);
    bpmLabel.setColour (juce::Label::textColourId, kDim);
    bpmLabel.setFont (juce::Font (juce::FontOptions (12.0f)));
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

    updateTabVisibility();
    setResizable (false, false);
    setSize (980, 640);
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
    k->slider.setTextBoxStyle (juce::Slider::NoTextBox, false, 0, 0);
    k->slider.setRotaryParameters (juce::MathConstants<float>::pi * 1.2f,
                                   juce::MathConstants<float>::pi * 2.8f, true);
    addAndMakeVisible (k->slider);

    k->label.setText (name, juce::dontSendNotification);
    k->label.setJustificationType (juce::Justification::centred);
    k->label.setColour (juce::Label::textColourId, kDim);
    k->label.setFont (juce::Font (juce::FontOptions (10.0f)));
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
    auto secLabel = [&g] (const juce::String& t, juce::Rectangle<int> r)
    {
        g.setColour (kDim);
        g.setFont (juce::Font (juce::FontOptions (10.0f, juce::Font::bold)));
        g.drawText (t.toUpperCase(), r, juce::Justification::centredLeft, false);
    };

    // A) setup pills
    for (auto& p : setupPills)
    {
        g.setColour (kPanel);  g.fillRoundedRectangle (p.toFloat(), 8.0f);
        g.setColour (kBorder); g.drawRoundedRectangle (p.toFloat().reduced (0.5f), 8.0f, 1.0f);
    }

    // C) chord cards
    secLabel ("Progression", { cardArea.getX(), cardArea.getY() - 14, 240, 12 });
    const auto& prog = proc.progression();
    const int playing = proc.playheadChordIndex();
    for (int i = 0; i < (int) cardRects.size() && i < (int) prog.size(); ++i)
    {
        const auto rr = cardRects[(size_t) i];
        const auto r  = rr.toFloat();
        const auto& c = prog[(size_t) i];

        g.setColour (c.locked ? kCardLk : kCard);
        g.fillRoundedRectangle (r, 6.0f);
        g.setColour (i == playing ? kAccent : (c.locked ? kBorder.brighter (0.35f) : kBorder));
        g.drawRoundedRectangle (r.reduced (0.6f), 6.0f, i == playing ? 2.0f : 1.0f);

        g.setColour (kText);
        g.setFont (juce::Font (juce::FontOptions (juce::jmin (24.0f, r.getWidth() * 0.32f), juce::Font::bold)));
        g.drawText (c.label, r.reduced (4.0f).withTrimmedBottom (r.getHeight() * 0.30f), juce::Justification::centred, false);

        g.setColour (kDim);
        g.setFont (juce::Font (juce::FontOptions (12.0f)));
        g.drawText (c.roman, r.withTrimmedTop (r.getHeight() * 0.72f), juce::Justification::centred, false);

        g.setColour (c.locked ? kAccent : kBorder.brighter (0.5f));
        g.drawText (c.locked ? juce::String::fromUTF8 ("\xE2\x97\x8F") : juce::String::fromUTF8 ("\xE2\x97\x8B"),
                    rr.getRight() - 16, rr.getY() + 3, 14, 14, juce::Justification::centred);
    }

    // C') melody drop-strip (dashed)
    {
        const auto mr = melodyStrip.toFloat();
        g.setColour (kPanel); g.fillRoundedRectangle (mr, 6.0f);
        juce::Path base; base.addRoundedRectangle (mr.reduced (0.6f), 6.0f);
        juce::Path dashed; const float dl[] = { 5.0f, 4.0f };
        juce::PathStrokeType (1.2f).createDashedStroke (dashed, base, dl, 2);
        g.setColour (proc.melodyNoteCount() > 0 ? kAccent.withAlpha (0.7f) : kBorder.brighter (0.2f));
        g.strokePath (dashed, juce::PathStrokeType (1.2f));
    }

    // D) layer cards
    secLabel ("Layers", { layerCards[0].getX(), layerCards[0].getY() - 14, 200, 12 });
    const bool en[4] = { true, bassEnableBtn.getToggleState(), arpEnableBtn.getToggleState(), counterEnableBtn.getToggleState() };
    for (int i = 0; i < 4; ++i)
    {
        const auto r = layerCards[(size_t) i].toFloat();
        g.setColour (kCard); g.fillRoundedRectangle (r, 6.0f);
        g.setColour (en[i] ? kAccent.withAlpha (0.55f) : kBorder);
        g.drawRoundedRectangle (r.reduced (0.6f), 6.0f, 1.0f);
        g.setColour (en[i] ? kAccent.withAlpha (0.85f) : kBorder);
        g.fillRoundedRectangle (r.getX() + 8.0f, r.getY() + 25.0f, r.getWidth() - 16.0f, 2.0f, 1.0f);
        if (i == 0)   // CHORDS card has no enable toggle — draw its name
        {
            g.setColour (kText);
            g.setFont (juce::Font (juce::FontOptions (12.0f, juce::Font::bold)));
            g.drawText ("CHORDS", layerCards[0].withTrimmedTop (5).removeFromTop (18).reduced (8, 0), juce::Justification::centredLeft);
        }
    }

    // E) tab body + active-tab underline
    g.setColour (kPanel);  g.fillRoundedRectangle (tabBody.toFloat(), 8.0f);
    g.setColour (kBorder); g.drawRoundedRectangle (tabBody.toFloat().reduced (0.5f), 8.0f, 1.0f);
    const auto ab = (activeTab == 0 ? genTabBtn : toneTabBtn).getBounds();
    g.setColour (kAccent); g.fillRect (ab.getX() + 8, ab.getBottom() - 2, ab.getWidth() - 16, 2);

    // F) footer: timeline (+ keyboard when open)
    secLabel ("Timeline", { timelineArea.getX(), timelineArea.getY() - 13, 160, 12 });
    drawTimeline (g);
    if (keyboardOpen)
    {
        secLabel ("Keyboard", { keyboardArea.getX(), keyboardArea.getY() - 13, 160, 12 });
        drawKeyboard (g);
    }
}

void TranceChordsAudioProcessorEditor::DragStrip::paint (juce::Graphics& g)
{
    auto r = getLocalBounds().toFloat().reduced (0.5f);
    g.setColour (kPanel.brighter (0.10f));
    g.fillRoundedRectangle (r, 4.0f);
    g.setColour (kBorder);
    g.drawRoundedRectangle (r, 4.0f, 1.0f);
    g.setColour (kDim.brighter (0.2f));
    g.setFont (juce::Font (juce::FontOptions (10.0f, juce::Font::bold)));
    g.drawText (juce::String::fromUTF8 ("\xE2\xA0\xBF DRAG MIDI"), getLocalBounds(), juce::Justification::centred);
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
        g.setColour (m == mel ? kText : (inChord (m) ? kAccent : juce::Colour (0xffcfd6df)));
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
        g.setColour (m == mel ? kText : (inChord (m) ? kAccent.darker (0.2f) : juce::Colour (0xff0e131b)));
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
    const int pad = 12, gap = 8;
    auto area = getLocalBounds();

    auto placeKnob = [this] (int idx, juce::Rectangle<int>& row, int sz, int cellW)
    {
        if (idx < 0 || idx >= (int) knobs.size()) return;
        auto cell = row.removeFromLeft (cellW);
        knobs[(size_t) idx]->slider.setBounds (cell.removeFromTop (sz).withSizeKeepingCentre (sz, sz));
        knobs[(size_t) idx]->label.setBounds (cell.removeFromTop (12));
    };

    // ---- A) SETUP BAR ----
    area.removeFromTop (8);
    auto setup = area.removeFromTop (40).reduced (pad, 0);
    titleLabel.setBounds (setup.removeFromLeft (150));
    setup.removeFromLeft (gap);
    bpmLabel.setBounds (setup.removeFromRight (86));
    setup.removeFromRight (gap);
    {
        const int avail = setup.getWidth();
        const int w1 = juce::roundToInt (avail * 0.27f), w3 = juce::roundToInt (avail * 0.24f);
        setupPills[0] = setup.removeFromLeft (w1); setup.removeFromLeft (gap);
        setupPills[1] = setup.removeFromLeft (avail - w1 - w3 - 2 * gap); setup.removeFromLeft (gap);
        setupPills[2] = setup;
    }
    { auto p = setupPills[0].reduced (7, 7); const int c = (p.getWidth() - 4) / 2;
      keyBox.setBounds (p.removeFromLeft (c)); p.removeFromLeft (4); modeBox.setBounds (p); }
    { auto p = setupPills[1].reduced (7, 7); const int c = (p.getWidth() - 8) / 3;
      sectionBox.setBounds (p.removeFromLeft (c)); p.removeFromLeft (4);
      moodBox.setBounds (p.removeFromLeft (c)); p.removeFromLeft (4); styleBox.setBounds (p); }
    { auto p = setupPills[2].reduced (7, 7); const int c = (p.getWidth() - 4) / 2;
      lengthBox.setBounds (p.removeFromLeft (c)); p.removeFromLeft (4); densityBox.setBounds (p); }

    // ---- B) ACTION ROW ----
    area.removeFromTop (gap);
    auto act = area.removeFromTop (40).reduced (pad, 2);
    generateBtn.setBounds (act.removeFromLeft (128)); act.removeFromLeft (gap);
    undoBtn.setBounds (act.removeFromLeft (30)); act.removeFromLeft (3);
    redoBtn.setBounds (act.removeFromLeft (30)); act.removeFromLeft (gap);
    playBtn.setBounds (act.removeFromLeft (74));
    favoritesBox.setBounds (act.removeFromRight (150)); act.removeFromRight (6);
    saveFavBtn.setBounds (act.removeFromRight (50));    act.removeFromRight (gap);
    songFormBox.setBounds (act.removeFromRight (126));  act.removeFromRight (6);
    songModeBtn.setBounds (act.removeFromRight (58));   act.removeFromRight (gap);
    presetBox.setBounds (act.removeFromRight (126));

    // ---- C) CHORD CARDS (hero) + melody drop-strip ----
    area.removeFromTop (gap + 6);
    cardArea = area.removeFromTop (112).reduced (pad, 0);
    layoutCards();
    area.removeFromTop (4);
    melodyStrip = area.removeFromTop (24).reduced (pad, 0);
    { auto m = melodyStrip.reduced (6, 3);
      melodyFitBtn.setBounds (m.removeFromLeft (92));
      clearMelodyBtn.setBounds (m.removeFromRight (56)); m.removeFromRight (6);
      melodyLabel.setBounds (m); }

    // ---- D) LAYER CARDS ----
    area.removeFromTop (gap + 6);
    auto lc = area.removeFromTop (122).reduced (pad, 0);
    { const int w = (lc.getWidth() - 3 * gap) / 4;
      for (int i = 0; i < 4; ++i) { layerCards[(size_t) i] = lc.removeFromLeft (i < 3 ? w : lc.getWidth()); if (i < 3) lc.removeFromLeft (gap); } }
    auto card = [] (juce::Rectangle<int> r) { return r.reduced (8, 6); };
    { auto in = card (layerCards[0]); in.removeFromTop (18 + 10);                              // CHORDS
      dragChords.setBounds (in.removeFromBottom (22)); in.removeFromBottom (8);
      mixChordsSlider.setBounds (in.removeFromBottom (22)); }
    { auto in = card (layerCards[1]); bassEnableBtn.setBounds (in.removeFromTop (18)); in.removeFromTop (10);  // BASS
      dragBass.setBounds (in.removeFromBottom (22)); in.removeFromBottom (8);
      mixBassSlider.setBounds (in.removeFromBottom (22)); in.removeFromBottom (8);
      bassPatternBox.setBounds (in.removeFromTop (22)); }
    { auto in = card (layerCards[2]); arpEnableBtn.setBounds (in.removeFromTop (18)); in.removeFromTop (10);   // ARP
      dragArp.setBounds (in.removeFromBottom (22)); in.removeFromBottom (8);
      mixArpSlider.setBounds (in.removeFromBottom (22)); in.removeFromBottom (8);
      arpPatternBox.setBounds (in.removeFromTop (22)); in.removeFromTop (4); arpRateBox.setBounds (in.removeFromTop (22)); }
    { auto in = card (layerCards[3]); counterEnableBtn.setBounds (in.removeFromTop (18)); in.removeFromTop (10); // COUNTER
      dragCounter.setBounds (in.removeFromBottom (22)); in.removeFromBottom (8);
      mixCounterSlider.setBounds (in.removeFromBottom (22)); in.removeFromBottom (8);
      counterPatternBox.setBounds (in.removeFromTop (22)); in.removeFromTop (4); counterRateBox.setBounds (in.removeFromTop (22)); }

    // ---- E) TABBED SOUND SECTION ----
    area.removeFromTop (gap);
    auto tabsRow = area.removeFromTop (26).reduced (pad, 0);
    genTabBtn.setBounds (tabsRow.removeFromLeft (110)); tabsRow.removeFromLeft (4);
    toneTabBtn.setBounds (tabsRow.removeFromLeft (110));
    keyboardBtn.setBounds (tabsRow.removeFromRight (110));
    auto body = area.removeFromTop (148).reduced (pad, 0);
    tabBody = body;
    {   // GENERATOR tab (knobs 0-6 + option toggles 0-5 + voicing)
        auto gen = body.reduced (10, 10);
        auto opts = gen.removeFromRight (210);
        auto r1 = gen.removeFromTop (66); { const int cw = r1.getWidth() / 3;
            placeKnob (0, r1, 54, cw); placeKnob (1, r1, 54, cw); placeKnob (3, r1, 54, cw); }   // Energy/Complexity/Variation
        gen.removeFromTop (2);
        auto r2 = gen.removeFromTop (52); { const int cw = r2.getWidth() / 4;
            placeKnob (2, r2, 40, cw); placeKnob (4, r2, 40, cw); placeKnob (5, r2, 40, cw); placeKnob (6, r2, 40, cw); }
        auto o = opts.reduced (4, 0);
        voicingStyleBox.setBounds (o.removeFromBottom (24));
        o.removeFromBottom (8);
        const int colW = o.getWidth() / 2;
        auto c1 = o.removeFromLeft (colW), c2 = o;
        int y1 = c1.getY(), y2 = c2.getY();
        for (int i = 0; i < 6 && i < (int) toggles.size(); ++i)
        { auto& col = (i < 3 ? c1 : c2); int& yy = (i < 3 ? y1 : y2);
          toggles[(size_t) i]->setBounds (col.getX(), yy, col.getWidth() - 4, 22); yy += 26; }
    }
    {   // TONE & FX tab (knobs 7-14 + Preview Snd toggle 6)
        auto tone = body.reduced (10, 10);
        auto tr = tone.removeFromRight (150);
        auto r1 = tone.removeFromTop (52); { const int cw = r1.getWidth() / 4;
            placeKnob (7, r1, 40, cw); placeKnob (8, r1, 40, cw); placeKnob (9, r1, 40, cw); placeKnob (10, r1, 40, cw); }
        tone.removeFromTop (2);
        auto r2 = tone.removeFromTop (66); { const int cw = r2.getWidth() / 4;
            placeKnob (11, r2, 40, cw); placeKnob (12, r2, 40, cw); placeKnob (13, r2, 54, cw); placeKnob (14, r2, 40, cw); }
        if (toggles.size() > 6) toggles[6]->setBounds (tr.withSizeKeepingCentre (140, 22));
    }

    // ---- F) FOOTER (anchored to the bottom) ----
    if (keyboardOpen) { keyboardArea = area.removeFromBottom (56).reduced (pad, 0); area.removeFromBottom (6); }
    timelineArea = area.removeFromBottom (24).reduced (pad, 0);
}

void TranceChordsAudioProcessorEditor::updateTabVisibility()
{
    // Both tabs' components stay alive with their attachments — only visibility
    // toggles, so host automation still moves the hidden tab's controls.
    const bool gen = (activeTab == 0);
    for (int i = 0; i < (int) knobs.size(); ++i)
    {
        const bool show = gen ? (i <= 6) : (i >= 7);
        knobs[(size_t) i]->slider.setVisible (show);
        knobs[(size_t) i]->label.setVisible (show);
    }
    for (int i = 0; i < (int) toggles.size(); ++i)
        toggles[(size_t) i]->setVisible (i < 6 ? gen : ! gen);   // 0-5 = generator, 6 (Preview Snd) = tone
    voicingStyleBox.setVisible (gen);

    genTabBtn.setColour (juce::TextButton::textColourOffId, gen ? kText : kDim);
    toneTabBtn.setColour (juce::TextButton::textColourOffId, gen ? kDim : kText);
    genTabBtn.setColour (juce::TextButton::buttonColourId, gen ? kPanel.brighter (0.14f) : kPanel);
    toneTabBtn.setColour (juce::TextButton::buttonColourId, gen ? kPanel : kPanel.brighter (0.14f));
    repaint();
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
        // moving playhead + live card highlight while playing
        repaint (cardArea);
        repaint (timelineArea);
        if (keyboardOpen) repaint (keyboardArea);
    }

    // recolour layer level-faders + card accents when enable states change
    const int em = (bassEnableBtn.getToggleState() ? 1 : 0)
                 | (arpEnableBtn.getToggleState() ? 2 : 0)
                 | (counterEnableBtn.getToggleState() ? 4 : 0);
    if (em != lastEnableMask)
    {
        lastEnableMask = em;
        mixChordsSlider.setColour  (juce::Slider::trackColourId, kAccent);
        mixBassSlider.setColour    (juce::Slider::trackColourId, (em & 1) ? kAccent : kDim);
        mixArpSlider.setColour     (juce::Slider::trackColourId, (em & 2) ? kAccent : kDim);
        mixCounterSlider.setColour (juce::Slider::trackColourId, (em & 4) ? kAccent : kDim);
        repaint();
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
