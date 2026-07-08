#pragma once
//==============================================================================
// MidBass editor — Phase 8b. 2000s rack aesthetic: dark alu, orange value arcs
// and LEDs, teal sweet-spot arcs (data-tied to mb::kSweetSpots — the arc values
// are computed from the table at construction and counted against it, so GUI
// and data cannot drift). One fixed 1400x980 logical panel with STEPPED integer
// scaling (100/125/150 %, persisted in the APVTS state tree, no free resize).
// The analyzer pulls the processor's lock-free ring on a 30 Hz message-thread
// timer; the audio thread only ever does the one-store-per-sample tap.
//==============================================================================
#include <juce_audio_utils/juce_audio_utils.h>
#include <juce_dsp/juce_dsp.h>
#include "PluginProcessor.h"

//==============================================================================
class MbLookAndFeel : public juce::LookAndFeel_V4
{
public:
    static constexpr juce::uint32 kBg = 0xff17181b, kPanel = 0xff2a2d31, kRim = 0xff45494f,
                                  kOrange = 0xffd08a2e, kTeal = 0xff3ba8a0, kText = 0xffcfd2d6;
    MbLookAndFeel()
    {
        setColour (juce::ResizableWindow::backgroundColourId, juce::Colour (kBg));
        setColour (juce::Label::textColourId, juce::Colour (kText));
        setColour (juce::ComboBox::backgroundColourId, juce::Colour (0xff1e2023));
        setColour (juce::ComboBox::outlineColourId, juce::Colour (kRim));
        setColour (juce::ComboBox::textColourId, juce::Colour (kText));
        setColour (juce::ComboBox::arrowColourId, juce::Colour (kOrange));
        setColour (juce::PopupMenu::backgroundColourId, juce::Colour (0xff1e2023));
        setColour (juce::PopupMenu::textColourId, juce::Colour (kText));
        setColour (juce::PopupMenu::highlightedBackgroundColourId, juce::Colour (kOrange).withAlpha (0.35f));
        setColour (juce::TextButton::buttonColourId, juce::Colour (0xff26282c));
        setColour (juce::TextButton::buttonOnColourId, juce::Colour (kOrange).withAlpha (0.55f));
        setColour (juce::TextButton::textColourOffId, juce::Colour (kText));
        setColour (juce::ToggleButton::textColourId, juce::Colour (kText));
        setColour (juce::BubbleComponent::backgroundColourId, juce::Colour (0xff1e2023));
        setColour (juce::BubbleComponent::outlineColourId, juce::Colour (kRim));
    }

    void drawRotarySlider (juce::Graphics& g, int x, int y, int w, int h,
                           float pos, float a0, float a1, juce::Slider& s) override
    {
        auto bounds = juce::Rectangle<float> ((float) x, (float) y, (float) w, (float) h).reduced (2.0f);
        const float size = juce::jmin (bounds.getWidth(), bounds.getHeight());
        auto sq = bounds.withSizeKeepingCentre (size, size).reduced (size * 0.09f);
        const auto c = sq.getCentre();
        const float radius = sq.getWidth() * 0.5f;

        // teal sweet-spot arc, OUTSIDE the track — values come only from mb::kSweetSpots
        const auto& props = s.getProperties();
        if (props.contains ("ssLo"))
        {
            juce::Path ss;
            const float lo = (float) (double) props["ssLo"], hi = (float) (double) props["ssHi"];
            ss.addCentredArc (c.x, c.y, radius + 2.0f, radius + 2.0f, 0.0f,
                              a0 + (a1 - a0) * lo, a0 + (a1 - a0) * hi, true);
            g.setColour (juce::Colour (kTeal));
            g.strokePath (ss, juce::PathStrokeType (2.2f, juce::PathStrokeType::curved, juce::PathStrokeType::rounded));
        }

        // track + orange value arc
        juce::Path tr;
        tr.addCentredArc (c.x, c.y, radius - 2.0f, radius - 2.0f, 0.0f, a0, a1, true);
        g.setColour (juce::Colour (0xff1b1d20));
        g.strokePath (tr, juce::PathStrokeType (3.2f, juce::PathStrokeType::curved, juce::PathStrokeType::rounded));
        juce::Path vp;
        vp.addCentredArc (c.x, c.y, radius - 2.0f, radius - 2.0f, 0.0f, a0, a0 + (a1 - a0) * pos, true);
        g.setColour (juce::Colour (kOrange).withAlpha (s.isEnabled() ? 1.0f : 0.4f));
        g.strokePath (vp, juce::PathStrokeType (3.2f, juce::PathStrokeType::curved, juce::PathStrokeType::rounded));

        // alu body + pointer
        auto body = sq.reduced (5.0f);
        g.setGradientFill (juce::ColourGradient (juce::Colour (0xff54585e), body.getX(), body.getY(),
                                                 juce::Colour (0xff202226), body.getX(), body.getBottom(), false));
        g.fillEllipse (body);
        g.setColour (juce::Colour (0xff0d0e10));
        g.drawEllipse (body, 1.0f);
        const float ang = a0 + (a1 - a0) * pos;
        const auto p1 = c.getPointOnCircumference (radius * 0.30f, ang);
        const auto p2 = c.getPointOnCircumference (radius - 7.0f, ang);
        g.setColour (juce::Colour (kOrange));
        g.drawLine ({ p1, p2 }, 2.2f);
    }

    void drawToggleButton (juce::Graphics& g, juce::ToggleButton& b, bool over, bool down) override
    {
        juce::ignoreUnused (over, down);
        auto bounds = b.getLocalBounds().toFloat();
        const float d = juce::jmin (12.0f, bounds.getHeight() - 6.0f);
        auto led = juce::Rectangle<float> (d, d).withCentre ({ 4.0f + d * 0.5f, bounds.getCentreY() });
        const bool on = b.getToggleState();
        if (on)     // orange LED glow
        {
            g.setColour (juce::Colour (kOrange).withAlpha (0.35f));
            g.fillEllipse (led.expanded (2.5f));
        }
        g.setColour (on ? juce::Colour (kOrange) : juce::Colour (0xff35383d));
        g.fillEllipse (led);
        g.setColour (juce::Colour (0xff0d0e10));
        g.drawEllipse (led, 1.0f);
        g.setColour (juce::Colour (kText));
        g.setFont (juce::FontOptions (10.5f));
        g.drawText (b.getButtonText(), (int) (led.getRight() + 4), 0,
                    b.getWidth() - (int) led.getRight() - 6, b.getHeight(), juce::Justification::centredLeft);
    }
};

//==============================================================================
// A control with its caption underneath — the layout atom of the panel.
class MbLabeled : public juce::Component
{
public:
    MbLabeled (std::unique_ptr<juce::Component> c, const juce::String& caption) : ctrl (std::move (c))
    {
        addAndMakeVisible (*ctrl);
        label.setText (caption, juce::dontSendNotification);
        label.setJustificationType (juce::Justification::centred);
        label.setFont (juce::FontOptions (10.0f));
        addAndMakeVisible (label);
    }
    void resized() override
    {
        auto b = getLocalBounds();
        label.setBounds (b.removeFromBottom (12));
        ctrl->setBounds (b.reduced (1));
    }
    std::unique_ptr<juce::Component> ctrl;
    juce::Label label;
};

//==============================================================================
// A titled section frame that grid-lays its cells (fixed column count).
class MbSection : public juce::Component
{
public:
    explicit MbSection (const juce::String& t, int columns) : title (t), cols (columns) {}
    void add (juce::Component* c) { cells.push_back (c); addAndMakeVisible (c); }
    void setHero (juce::Component* c) { hero = c; addAndMakeVisible (c); }   // large left cell (FILTER cutoff)
    void paint (juce::Graphics& g) override
    {
        auto r = getLocalBounds().toFloat();
        g.setGradientFill (juce::ColourGradient (juce::Colour (0xff2e3136), r.getX(), r.getY(),
                                                 juce::Colour (0xff26282c), r.getX(), r.getBottom(), false));
        g.fillRoundedRectangle (r, 6.0f);
        g.setColour (juce::Colour (MbLookAndFeel::kRim));
        g.drawRoundedRectangle (r.reduced (0.5f), 6.0f, 1.0f);
        g.setColour (juce::Colour (MbLookAndFeel::kOrange));
        g.setFont (juce::FontOptions (11.0f, juce::Font::bold));
        g.drawText (title, 8, 2, getWidth() - 16, 14, juce::Justification::centredLeft);
    }
    void resized() override
    {
        auto b = getLocalBounds().reduced (6);
        b.removeFromTop (14);
        if (hero != nullptr)
        {
            auto hb = b.removeFromLeft (std::min (b.getHeight() + 14, b.getWidth() * 2 / 5));
            hero->setBounds (hb.reduced (2));
            b.removeFromLeft (4);
        }
        if (cells.empty()) return;
        const int rows = ((int) cells.size() + cols - 1) / cols;
        const int cw = b.getWidth() / cols, chh = b.getHeight() / rows;
        for (int i = 0; i < (int) cells.size(); ++i)
            cells[(size_t) i]->setBounds (b.getX() + (i % cols) * cw, b.getY() + (i / cols) * chh, cw, chh);
    }
    juce::String title;
    int cols;
    std::vector<juce::Component*> cells;
    juce::Component* hero = nullptr;
};

//==============================================================================
// Realtime spectrum: pulls the processor's lock-free ring at 30 Hz, windows +
// FFTs on the message thread. Log axis 20 Hz-20 kHz, 100-900 Hz highlight.
class MbAnalyzer : public juce::Component, private juce::Timer
{
public:
    explicit MbAnalyzer (MidBassAudioProcessor& p) : proc (p)
    {
        binsDb.fill (-90.0f);
        startTimerHz (30);
    }
    ~MbAnalyzer() override { stopTimer(); }

    void timerCallback() override
    {
        constexpr int N = MidBassAudioProcessor::kVizSize;
        const int wp = proc.vizWritePos.load (std::memory_order_acquire);
        std::array<float, (size_t) N * 2> buf {};
        for (int i = 0; i < N; ++i)
        {
            const float w = 0.5f - 0.5f * std::cos (2.0f * juce::MathConstants<float>::pi * (float) i / (float) N);
            buf[(size_t) i] = proc.vizRing[(wp - N + i) & (N - 1)] * w;
        }
        fft.performFrequencyOnlyForwardTransform (buf.data());

        const double sr = proc.getSampleRate() > 0.0 ? proc.getSampleRate() : 44100.0;
        for (int b = 0; b < kBins; ++b)
        {
            const double f0 = 20.0 * std::pow (1000.0, (double) b / kBins);          // 20..20k log
            const double f1 = 20.0 * std::pow (1000.0, (double) (b + 1) / kBins);
            int k0 = juce::jmax (1, (int) (f0 * N / sr)), k1 = juce::jmax (k0 + 1, (int) (f1 * N / sr));
            float m = 0.0f;
            for (int k = k0; k < juce::jmin (k1, N / 2); ++k) m = juce::jmax (m, buf[(size_t) k]);
            const float db = 20.0f * std::log10 (juce::jmax (m * (2.0f / N), 1.0e-6f));
            binsDb[(size_t) b] = juce::jmax (db, binsDb[(size_t) b] - 2.5f);          // ~75 dB/s fall
        }
        repaint();
    }

    void paint (juce::Graphics& g) override
    {
        auto r = getLocalBounds().toFloat();
        g.setColour (juce::Colour (0xff141518));
        g.fillRoundedRectangle (r, 6.0f);

        auto xOf = [&r] (double hz) { return r.getX() + (float) (std::log (hz / 20.0) / std::log (1000.0)) * r.getWidth(); };
        g.setColour (juce::Colour (MbLookAndFeel::kOrange).withAlpha (0.10f));       // mid-bass home
        g.fillRect (juce::Rectangle<float> (xOf (100.0), r.getY(), xOf (900.0) - xOf (100.0), r.getHeight()));
        g.setColour (juce::Colour (0xff26282c));
        for (double hz : { 100.0, 1000.0, 10000.0 })
            g.drawVerticalLine ((int) xOf (hz), r.getY() + 4, r.getBottom() - 4);

        juce::Path path;
        for (int b = 0; b < kBins; ++b)
        {
            const float px = r.getX() + (float) b / (kBins - 1) * r.getWidth();
            const float py = juce::jmap (juce::jlimit (-90.0f, 0.0f, binsDb[(size_t) b]), -90.0f, 0.0f,
                                         r.getBottom() - 2.0f, r.getY() + 4.0f);
            if (b == 0) path.startNewSubPath (px, py); else path.lineTo (px, py);
        }
        g.setColour (juce::Colour (MbLookAndFeel::kTeal).withAlpha (0.25f));
        auto fill = path;
        fill.lineTo (r.getRight(), r.getBottom());
        fill.lineTo (r.getX(), r.getBottom());
        fill.closeSubPath();
        g.fillPath (fill);
        g.setColour (juce::Colour (MbLookAndFeel::kTeal));
        g.strokePath (path, juce::PathStrokeType (1.6f));
        g.setColour (juce::Colour (MbLookAndFeel::kRim));
        g.drawRoundedRectangle (r.reduced (0.5f), 6.0f, 1.0f);
    }

private:
    static constexpr int kBins = 72;
    MidBassAudioProcessor& proc;
    juce::dsp::FFT fft { 11 };                     // 2048
    std::array<float, kBins> binsDb {};
};

//==============================================================================
class MidBassAudioProcessorEditor : public juce::AudioProcessorEditor
{
public:
    static constexpr int kW = 1400, kH = 980;      // logical panel size (fixed)

    explicit MidBassAudioProcessorEditor (MidBassAudioProcessor& p);
    ~MidBassAudioProcessorEditor() override;

    void paint (juce::Graphics& g) override;
    void resized() override;

    void applyScale (float s);                     // stepped: 1.0 / 1.25 / 1.5
    int attachedParameterCountForTest() const;     // condition d: all 131 reachable
    int sweetSpotArcCountForTest() const { return sweetSpotArcs; }   // condition e

private:
    MbLabeled* makeKnob (MbSection& sec, const char* pid, const juce::String& caption);
    MbLabeled* makeCombo (MbSection& sec, const char* pid, const juce::String& caption);
    juce::Component* makeToggle (MbSection& sec, const char* pid, const juce::String& caption);
    MbSection& section (const juce::String& title, int cols);
    void layoutContent();
    void wireSweetSpot (juce::Slider& s, const char* pid);

    MbLookAndFeel lnf;                             // declared first: outlives all children
    MidBassAudioProcessor& proc;
    juce::Component content;                       // fixed kW x kH, transformed for scaling
    float uiScale = 1.0f;
    int sweetSpotArcs = 0;

    juce::Label logo;
    juce::TextButton scaleButton { "100%" };
    juce::OwnedArray<juce::TextButton> presetButtons;
    juce::OwnedArray<MbSection> sections;
    juce::OwnedArray<MbLabeled> labeled;
    juce::OwnedArray<juce::ToggleButton> toggles;
    MbAnalyzer analyzer;
    juce::MidiKeyboardComponent keyboard;

    // attachments last: destroyed first, before the controls they reference
    std::vector<std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment>>   sliderAtts;
    std::vector<std::unique_ptr<juce::AudioProcessorValueTreeState::ComboBoxAttachment>> comboAtts;
    std::vector<std::unique_ptr<juce::AudioProcessorValueTreeState::ButtonAttachment>>   buttonAtts;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (MidBassAudioProcessorEditor)
};
