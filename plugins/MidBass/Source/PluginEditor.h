#pragma once
//==============================================================================
// MidBass editor — Phase 8a: layout skeleton. Every one of the 131 parameters
// gets a control on ONE fixed 1400x980 panel (spec: no tabs, no hidden menus).
// Sizing decision: readable at 100 % on 1080p (1920x1080 minus taskbar); no
// free resize. Phase 8b adds the rack LookAndFeel, sweet-spot arcs, the
// analyzer (placeholder box here) and value readouts.
//
// Automation correctness (condition f): every control is bound through an
// APVTS attachment (Slider/ComboBox/Button Attachment), which issue proper
// begin/endChangeGesture pairs. Preset quick buttons go through the JUCE
// programs API (setCurrentProgram) so hosts and attachments stay in sync.
//==============================================================================
#include <juce_audio_utils/juce_audio_utils.h>
#include "PluginProcessor.h"

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
    void paint (juce::Graphics& g) override
    {
        g.setColour (juce::Colour (0xff2a2d31));
        g.fillRoundedRectangle (getLocalBounds().toFloat(), 6.0f);
        g.setColour (juce::Colour (0xff45494f));
        g.drawRoundedRectangle (getLocalBounds().toFloat().reduced (0.5f), 6.0f, 1.0f);
        g.setColour (juce::Colour (0xffd08a2e));
        g.setFont (juce::FontOptions (11.0f, juce::Font::bold));
        g.drawText (title, 8, 2, getWidth() - 16, 14, juce::Justification::centredLeft);
    }
    void resized() override
    {
        auto b = getLocalBounds().reduced (6);
        b.removeFromTop (14);
        if (cells.empty()) return;
        const int rows = ((int) cells.size() + cols - 1) / cols;
        const int cw = b.getWidth() / cols, chh = b.getHeight() / rows;
        for (int i = 0; i < (int) cells.size(); ++i)
            cells[(size_t) i]->setBounds (b.getX() + (i % cols) * cw, b.getY() + (i / cols) * chh, cw, chh);
    }
    juce::String title;
    int cols;
    std::vector<juce::Component*> cells;
};

//==============================================================================
class MidBassAudioProcessorEditor : public juce::AudioProcessorEditor
{
public:
    explicit MidBassAudioProcessorEditor (MidBassAudioProcessor& p);
    ~MidBassAudioProcessorEditor() override;

    void paint (juce::Graphics& g) override;
    void resized() override;

    int attachedParameterCountForTest() const;   // condition d: all 131 reachable

private:
    MbLabeled* makeKnob (MbSection& sec, const char* pid, const juce::String& caption);
    MbLabeled* makeCombo (MbSection& sec, const char* pid, const juce::String& caption);
    juce::Component* makeToggle (MbSection& sec, const char* pid, const juce::String& caption);
    MbSection& section (const juce::String& title, int cols);

    MidBassAudioProcessor& proc;

    juce::Label logo;
    juce::OwnedArray<juce::TextButton> presetButtons;
    juce::OwnedArray<MbSection> sections;
    juce::OwnedArray<MbLabeled> labeled;
    juce::OwnedArray<juce::ToggleButton> toggles;

    struct AnalyzerPlaceholder : juce::Component
    {
        void paint (juce::Graphics& g) override
        {
            g.setColour (juce::Colour (0xff202226));
            g.fillRoundedRectangle (getLocalBounds().toFloat(), 6.0f);
            g.setColour (juce::Colour (0xff3ba8a0));
            g.drawRoundedRectangle (getLocalBounds().toFloat().reduced (0.5f), 6.0f, 1.0f);
            g.drawText ("SPECTRUM ANALYZER (Phase 8b): 20 Hz to 20 kHz, 100-900 Hz midbass highlight",
                        getLocalBounds(), juce::Justification::centred);
        }
    } analyzer;

    juce::MidiKeyboardComponent keyboard;

    // attachments last: destroyed first, before the controls they reference
    std::vector<std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment>>   sliderAtts;
    std::vector<std::unique_ptr<juce::AudioProcessorValueTreeState::ComboBoxAttachment>> comboAtts;
    std::vector<std::unique_ptr<juce::AudioProcessorValueTreeState::ButtonAttachment>>   buttonAtts;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (MidBassAudioProcessorEditor)
};
