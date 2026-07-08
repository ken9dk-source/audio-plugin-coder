#include "PluginEditor.h"

using APVTS = juce::AudioProcessorValueTreeState;
namespace pid = mb::pid;

//==============================================================================
MbSection& MidBassAudioProcessorEditor::section (const juce::String& title, int cols)
{
    auto* s = sections.add (new MbSection (title, cols));
    content.addAndMakeVisible (s);
    return *s;
}

// Condition e (data tie): arc extents are COMPUTED from mb::kSweetSpots and
// counted; construction asserts every table row landed on exactly one knob.
void MidBassAudioProcessorEditor::wireSweetSpot (juce::Slider& s, const char* pidStr)
{
    for (int i = 0; i < mb::kNumSweetSpots; ++i)
    {
        const auto& ss = mb::kSweetSpots[i];
        if (std::strcmp (ss.paramID, pidStr) != 0) continue;
        if (auto* rp = proc.apvts.getParameter (pidStr))
        {
            s.getProperties().set ("ssLo", (double) rp->convertTo0to1 (ss.lo));
            s.getProperties().set ("ssHi", (double) rp->convertTo0to1 (ss.hi));
            ++sweetSpotArcs;
        }
    }
}

MbLabeled* MidBassAudioProcessorEditor::makeKnob (MbSection& sec, const char* pidStr, const juce::String& caption)
{
    auto sl = std::make_unique<juce::Slider> (juce::Slider::RotaryHorizontalVerticalDrag, juce::Slider::NoTextBox);
    sl->setPopupDisplayEnabled (true, true, this);          // value readout on touch
    wireSweetSpot (*sl, pidStr);
    sliderAtts.push_back (std::make_unique<APVTS::SliderAttachment> (proc.apvts, pidStr, *sl));
    auto* lc = labeled.add (new MbLabeled (std::move (sl), caption));
    sec.add (lc);
    return lc;
}

MbLabeled* MidBassAudioProcessorEditor::makeCombo (MbSection& sec, const char* pidStr, const juce::String& caption)
{
    auto cb = std::make_unique<juce::ComboBox>();
    if (auto* pc = dynamic_cast<juce::AudioParameterChoice*> (proc.apvts.getParameter (pidStr)))
    {
        int id = 1;
        for (const auto& ch : pc->choices) cb->addItem (ch, id++);
    }
    comboAtts.push_back (std::make_unique<APVTS::ComboBoxAttachment> (proc.apvts, pidStr, *cb));
    auto* lc = labeled.add (new MbLabeled (std::move (cb), caption));
    sec.add (lc);
    return lc;
}

juce::Component* MidBassAudioProcessorEditor::makeToggle (MbSection& sec, const char* pidStr, const juce::String& caption)
{
    auto* tb = toggles.add (new juce::ToggleButton (caption));
    buttonAtts.push_back (std::make_unique<APVTS::ButtonAttachment> (proc.apvts, pidStr, *tb));
    sec.add (tb);
    return tb;
}

int MidBassAudioProcessorEditor::attachedParameterCountForTest() const
{
    return (int) (sliderAtts.size() + comboAtts.size() + buttonAtts.size());
}

//==============================================================================
MidBassAudioProcessorEditor::MidBassAudioProcessorEditor (MidBassAudioProcessor& p)
    : AudioProcessorEditor (p), proc (p), analyzer (p),
      keyboard (p.keyboardState, juce::MidiKeyboardComponent::horizontalKeyboard)
{
    setLookAndFeel (&lnf);
    addAndMakeVisible (content);

    logo.setText ("MIDBASS", juce::dontSendNotification);
    logo.setFont (juce::FontOptions (20.0f, juce::Font::bold));
    logo.setColour (juce::Label::textColourId, juce::Colour (MbLookAndFeel::kOrange));
    content.addAndMakeVisible (logo);

    scaleButton.onClick = [this]
    {
        applyScale (uiScale >= 1.45f ? 1.0f : uiScale >= 1.2f ? 1.5f : 1.25f);
    };
    content.addAndMakeVisible (scaleButton);

    for (int i = 0; i < mb::kNumFactoryPresets; ++i)
    {
        auto* b = presetButtons.add (new juce::TextButton (mb::kFactoryPresets[i].name));
        b->onClick = [this, i] { proc.setCurrentProgram (i); proc.updateHostDisplay(); };
        content.addAndMakeVisible (b);
    }

    // ---- row 1: sound generation ----
    {
        const char* names[3] = { "OSC 1", "OSC 2", "OSC 3" };
        const char* waves[3] = { pid::osc1_wave, pid::osc2_wave, pid::osc3_wave };
        const char* octs[3]  = { pid::osc1_oct, pid::osc2_oct, pid::osc3_oct };
        const char* semis[3] = { pid::osc1_semi, pid::osc2_semi, pid::osc3_semi };
        const char* fines[3] = { pid::osc1_fine, pid::osc2_fine, pid::osc3_fine };
        const char* pws[3]   = { pid::osc1_pw, pid::osc2_pw, pid::osc3_pw };
        const char* lvls[3]  = { pid::osc1_level, pid::osc2_level, pid::osc3_level };
        for (int i = 0; i < 3; ++i)
        {
            auto& s = section (names[i], 3);
            makeCombo (s, waves[i], "WAVE");
            makeKnob (s, octs[i], "OCT");
            makeKnob (s, semis[i], "SEMI");
            makeKnob (s, fines[i], "FINE");
            makeKnob (s, pws[i], "PW");
            makeKnob (s, lvls[i], "LEVEL");
        }
        auto& sub = section ("SUB", 2);
        makeToggle (sub, pid::sub_on, "ON");
        makeCombo (sub, pid::sub_wave, "WAVE");
        makeCombo (sub, pid::sub_oct, "OCT");
        makeKnob (sub, pid::sub_level, "LEVEL");

        auto& om = section ("OSC MOD", 2);
        makeToggle (om, pid::osc_sync, "SYNC 2>1");
        makeKnob (om, pid::osc_fm, "FM");
        makeKnob (om, pid::osc_ring, "RING");
        makeKnob (om, pid::osc_drift, "DRIFT");

        auto& uni = section ("UNISON", 2);
        makeKnob (uni, pid::uni_voices, "VOICES");
        makeKnob (uni, pid::uni_detune, "DETUNE");
        makeKnob (uni, pid::uni_spread, "SPREAD");
        makeToggle (uni, pid::uni_mono, "MONO");
    }

    // ---- row 2: hero — filter + envelopes ----
    {
        auto& f = section ("FILTER", 3);
        {   // HERO: the large cutoff knob owns the left of the section
            auto sl = std::make_unique<juce::Slider> (juce::Slider::RotaryHorizontalVerticalDrag, juce::Slider::NoTextBox);
            sl->setPopupDisplayEnabled (true, true, this);
            wireSweetSpot (*sl, pid::flt_cutoff);
            sliderAtts.push_back (std::make_unique<APVTS::SliderAttachment> (proc.apvts, pid::flt_cutoff, *sl));
            auto* lc = labeled.add (new MbLabeled (std::move (sl), "CUTOFF"));
            f.setHero (lc);
        }
        makeCombo (f, pid::flt_mode, "MODE");
        makeKnob (f, pid::flt_reso, "RESO");
        makeKnob (f, pid::flt_keytrack, "KEYTRK");
        makeKnob (f, pid::flt_env_amt, "ENV AMT");
        makeKnob (f, pid::flt_drive_pre, "DRIVE PRE");
        makeKnob (f, pid::flt_drive_post, "DRIVE POST");

        auto& fe = section ("FILTER ENV", 4);
        makeKnob (fe, pid::fenv_a, "A");
        makeKnob (fe, pid::fenv_d, "D");
        makeKnob (fe, pid::fenv_s, "S");
        makeKnob (fe, pid::fenv_r, "R");

        auto& ae = section ("AMP ENV", 4);
        makeKnob (ae, pid::aenv_a, "A");
        makeKnob (ae, pid::aenv_d, "D");
        makeKnob (ae, pid::aenv_s, "S");
        makeKnob (ae, pid::aenv_r, "R");
    }

    // ---- row 3: modulation ----
    {
        const char* names[2] = { "LFO 1", "LFO 2" };
        struct L { const char* w; const char* sy; const char* hz; const char* dv; const char* am; const char* rt; const char* ds; };
        const L ls[2] = {
            { pid::lfo1_wave, pid::lfo1_sync, pid::lfo1_rate_hz, pid::lfo1_rate_div, pid::lfo1_amount, pid::lfo1_retrig, pid::lfo1_dest },
            { pid::lfo2_wave, pid::lfo2_sync, pid::lfo2_rate_hz, pid::lfo2_rate_div, pid::lfo2_amount, pid::lfo2_retrig, pid::lfo2_dest } };
        for (int i = 0; i < 2; ++i)
        {
            auto& s = section (names[i], 4);
            makeCombo (s, ls[i].w, "WAVE");
            makeKnob (s, ls[i].hz, "RATE");
            makeCombo (s, ls[i].dv, "DIV");
            makeKnob (s, ls[i].am, "AMOUNT");
            makeCombo (s, ls[i].ds, "DEST");
            makeToggle (s, ls[i].sy, "SYNC");
            makeToggle (s, ls[i].rt, "RETRIG");
        }
        auto& mm = section ("MOD MATRIX", 6);
        const char* srcs[6] = { pid::mod1_src, pid::mod2_src, pid::mod3_src, pid::mod4_src, pid::mod5_src, pid::mod6_src };
        const char* dsts[6] = { pid::mod1_dst, pid::mod2_dst, pid::mod3_dst, pid::mod4_dst, pid::mod5_dst, pid::mod6_dst };
        const char* amts[6] = { pid::mod1_amt, pid::mod2_amt, pid::mod3_amt, pid::mod4_amt, pid::mod5_amt, pid::mod6_amt };
        for (int i = 0; i < 6; ++i)
            makeCombo (mm, srcs[i], "SRC " + juce::String (i + 1));
        for (int i = 0; i < 6; ++i)
            makeCombo (mm, dsts[i], "DST " + juce::String (i + 1));
        for (int i = 0; i < 6; ++i)
            makeKnob (mm, amts[i], "AMT " + juce::String (i + 1));
    }

    // ---- row 4: tone ----
    {
        auto& sa = section ("SATURATION", 3);
        makeCombo (sa, pid::sat_type, "TYPE");
        makeKnob (sa, pid::sat_drive, "DRIVE");
        makeKnob (sa, pid::sat_mix, "MIX");

        auto& eq = section ("EQ", 4);
        makeKnob (eq, pid::eq_ls_freq, "LO FREQ");
        makeKnob (eq, pid::eq_ls_gain, "LO GAIN");
        makeKnob (eq, pid::eq_mid_freq, "MID FREQ");
        makeKnob (eq, pid::eq_mid_gain, "MID GAIN");
        makeKnob (eq, pid::eq_mid_q, "MID Q");
        makeKnob (eq, pid::eq_hs_freq, "HI FREQ");
        makeKnob (eq, pid::eq_hs_gain, "HI GAIN");

        auto& tr = section ("TRANSIENT", 2);
        makeKnob (tr, pid::trans_attack, "ATTACK");
        makeKnob (tr, pid::trans_sustain, "SUSTAIN");

        auto& vo = section ("VOICE / OUT", 3);
        makeCombo (vo, pid::voice_mode, "MODE");
        makeCombo (vo, pid::voice_stack, "STACK");
        makeKnob (vo, pid::glide_time, "GLIDE");
        makeToggle (vo, pid::glide_legato, "GLD LEG");
        makeKnob (vo, pid::bend_range, "BEND");
        makeKnob (vo, pid::output, "OUTPUT");
    }

    // ---- row 5: macros ----
    {
        auto& ma = section ("MACROS", 6);
        makeKnob (ma, pid::macro_punch, "PUNCH");
        makeKnob (ma, pid::macro_bite, "BITE");
        makeKnob (ma, pid::macro_warmth, "WARMTH");
        makeKnob (ma, pid::macro_snap, "SNAP");
        makeKnob (ma, pid::macro_body, "BODY");
        makeKnob (ma, pid::macro_width, "WIDTH");
    }

    // ---- row 6: analyzer + FX strip ----
    content.addAndMakeVisible (analyzer);
    {
        auto& ch = section ("CHORUS", 2);
        makeToggle (ch, pid::fx_cho_on, "ON");
        makeKnob (ch, pid::fx_cho_rate, "RATE");
        makeKnob (ch, pid::fx_cho_depth, "DEPTH");
        makeKnob (ch, pid::fx_cho_mix, "MIX");

        auto& ph = section ("PHASER", 2);
        makeToggle (ph, pid::fx_pha_on, "ON");
        makeKnob (ph, pid::fx_pha_rate, "RATE");
        makeKnob (ph, pid::fx_pha_depth, "DEPTH");
        makeKnob (ph, pid::fx_pha_fb, "FB");
        makeKnob (ph, pid::fx_pha_mix, "MIX");

        auto& fl = section ("FLANGER", 2);
        makeToggle (fl, pid::fx_fla_on, "ON");
        makeKnob (fl, pid::fx_fla_rate, "RATE");
        makeKnob (fl, pid::fx_fla_depth, "DEPTH");
        makeKnob (fl, pid::fx_fla_fb, "FB");
        makeKnob (fl, pid::fx_fla_mix, "MIX");

        auto& dl = section ("DELAY", 2);
        makeToggle (dl, pid::fx_dly_on, "ON");
        makeCombo (dl, pid::fx_dly_div, "TIME");
        makeKnob (dl, pid::fx_dly_fb, "FB");
        makeKnob (dl, pid::fx_dly_damp, "DAMP");
        makeKnob (dl, pid::fx_dly_mix, "MIX");
        makeToggle (dl, pid::fx_dly_ping, "PING");

        auto& rv = section ("REVERB", 2);
        makeToggle (rv, pid::fx_rev_on, "ON");
        makeKnob (rv, pid::fx_rev_size, "SIZE");
        makeKnob (rv, pid::fx_rev_damp, "DAMP");
        makeKnob (rv, pid::fx_rev_mix, "MIX");

        auto& cp = section ("COMP", 2);
        makeToggle (cp, pid::fx_cmp_on, "ON");
        makeKnob (cp, pid::fx_cmp_thresh, "THRESH");
        makeKnob (cp, pid::fx_cmp_ratio, "RATIO");
        makeKnob (cp, pid::fx_cmp_att, "ATT");
        makeKnob (cp, pid::fx_cmp_rel, "REL");
        makeKnob (cp, pid::fx_cmp_gain, "MAKEUP");
    }

    content.addAndMakeVisible (keyboard);
    keyboard.setAvailableRange (24, 72);        // C1..C5 — the mid-bass playground
    keyboard.setColour (juce::MidiKeyboardComponent::keyDownOverlayColourId,
                        juce::Colour (MbLookAndFeel::kTeal).withAlpha (0.8f));
    keyboard.setColour (juce::MidiKeyboardComponent::mouseOverKeyOverlayColourId,
                        juce::Colour (MbLookAndFeel::kTeal).withAlpha (0.35f));
    keyboard.setColour (juce::MidiKeyboardComponent::shadowColourId, juce::Colours::black);

    // conditions d + e, hard-asserted at construction
    jassert (attachedParameterCountForTest() == mb::pid::kExpectedParamCount);
    jassert (sweetSpotArcs == mb::kNumSweetSpots);

    applyScale ((float) (double) proc.apvts.state.getProperty ("guiScale", 1.0));
}

MidBassAudioProcessorEditor::~MidBassAudioProcessorEditor()
{
    setLookAndFeel (nullptr);
}

void MidBassAudioProcessorEditor::applyScale (float s)
{
    uiScale = (s >= 1.45f) ? 1.5f : (s >= 1.2f) ? 1.25f : 1.0f;      // stepped, never free
    proc.apvts.state.setProperty ("guiScale", (double) uiScale, nullptr);
    scaleButton.setButtonText (juce::String ((int) std::lround (uiScale * 100.0f)) + "%");
    setSize ((int) std::lround (kW * uiScale), (int) std::lround (kH * uiScale));
}

void MidBassAudioProcessorEditor::paint (juce::Graphics& g)
{
    g.fillAll (juce::Colour (MbLookAndFeel::kBg));
}

void MidBassAudioProcessorEditor::resized()
{
    content.setTransform (juce::AffineTransform::scale (uiScale));
    content.setBounds (0, 0, kW, kH);
    layoutContent();
}

void MidBassAudioProcessorEditor::layoutContent()
{
    auto b = juce::Rectangle<int> (0, 0, kW, kH).reduced (8);

    // header
    auto header = b.removeFromTop (30);
    logo.setBounds (header.removeFromLeft (120));
    scaleButton.setBounds (header.removeFromRight (54).reduced (2));
    header.removeFromLeft (6);
    const int bw = header.getWidth() / presetButtons.size();
    for (auto* pb : presetButtons)
        pb->setBounds (header.removeFromLeft (bw).reduced (2, 2));

    auto layoutRow = [&b] (std::initializer_list<std::pair<juce::Component*, int>> items, int height)
    {
        b.removeFromTop (6);
        const auto row = b.removeFromTop (height);
        int totalW = 0;
        for (auto& it : items) totalW += it.second;
        int x = row.getX(), wsum = 0;
        for (auto& it : items)
        {
            wsum += it.second;
            const int xEnd = row.getX() + row.getWidth() * wsum / totalW;
            it.first->setBounds (juce::Rectangle<int> (x, row.getY(), xEnd - x, height).reduced (3, 0));
            x = xEnd;
        }
    };

    int i = 0;
    auto* osc1 = sections[i++]; auto* osc2 = sections[i++]; auto* osc3 = sections[i++];
    auto* sub  = sections[i++]; auto* omod = sections[i++]; auto* uni  = sections[i++];
    auto* flt  = sections[i++]; auto* fenv = sections[i++]; auto* aenv = sections[i++];
    auto* lfo1 = sections[i++]; auto* lfo2 = sections[i++]; auto* mtx  = sections[i++];
    auto* sat  = sections[i++]; auto* eqs  = sections[i++]; auto* trs  = sections[i++]; auto* vout = sections[i++];
    auto* mac  = sections[i++];
    auto* cho  = sections[i++]; auto* pha  = sections[i++]; auto* fla  = sections[i++];
    auto* dly  = sections[i++]; auto* rev  = sections[i++]; auto* cmp  = sections[i++];

    // 8a markup incorporated: FILTER hero widened (52), matrix +60 px from the
    // LFOs (25/25/50), analyzer wider vs narrower FX (46/9x6), taller macro row.
    layoutRow ({ { osc1, 23 }, { osc2, 23 }, { osc3, 23 }, { sub, 15 }, { omod, 15 }, { uni, 15 } }, 148);
    layoutRow ({ { flt, 52 }, { fenv, 24 }, { aenv, 24 } }, 134);
    layoutRow ({ { lfo1, 25 }, { lfo2, 25 }, { mtx, 50 } }, 148);
    layoutRow ({ { sat, 20 }, { eqs, 40 }, { trs, 12 }, { vout, 28 } }, 120);
    layoutRow ({ { mac, 100 } }, 124);
    layoutRow ({ { &analyzer, 46 }, { cho, 9 }, { pha, 9 }, { fla, 9 }, { dly, 9 }, { rev, 9 }, { cmp, 9 } }, 150);

    b.removeFromTop (6);
    auto kb = b.removeFromTop (64);
    keyboard.setKeyWidth ((float) kb.getWidth() / 29.0f);    // 29 white keys C1..C5 fill the strip
    keyboard.setBounds (kb);
}
