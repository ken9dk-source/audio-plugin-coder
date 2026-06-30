#pragma once
#include <juce_audio_processors/juce_audio_processors.h>
#include "PluginProcessor.h"
#include "EQDisplay.h"

//==============================================================================
// Draws one frame from the knob filmstrip (square frames stacked top-to-bottom).
class FilmstripKnobLAF : public juce::LookAndFeel_V4
{
public:
    explicit FilmstripKnobLAF (juce::Image strip) : strip_ (std::move (strip)) {}

    void drawRotarySlider (juce::Graphics& g, int x, int y, int w, int h,
                           float sliderPos, float, float, juce::Slider&) override
    {
        if (! strip_.isValid()) return;
        const int numFrames = strip_.getHeight() / strip_.getWidth();
        const int frame     = juce::roundToInt (sliderPos * (numFrames - 1));
        const int fw        = strip_.getWidth();
        g.drawImage (strip_, x, y, w, h, 0, frame * fw, fw, fw);
    }
private:
    juce::Image strip_;
};

//==============================================================================
// Shape button. The skin (1000.png) already paints the 5 grey buttons, so we
// draw NOTHING when inactive (the skin shows through) and overlay only the
// yellow (active) half of the 35×32 filmstrip on the selected one.
class ShapeButton : public juce::Button
{
public:
    ShapeButton (juce::Image img, int) : juce::Button ("shape"), image_ (std::move (img))
    { setClickingTogglesState (false); }   // selection is driven explicitly on click

    void paintButton (juce::Graphics& g, bool, bool) override
    {
        // Draw the filmstrip over the skin button it is aligned with:
        //   inactive -> grey (top) half, active -> yellow (bottom) half.
        if (! image_.isValid()) return;
        const int stateH = image_.getHeight() / 2;          // 16 px per state
        const int srcY   = getToggleState() ? stateH : 0;   // 0 = grey, 16 = yellow
        g.drawImage (image_, 0, 0, getWidth(), getHeight(),
                     0, srcY, image_.getWidth(), stateH);
    }
private:
    juce::Image image_;
};

//==============================================================================
// Preset slider handle: a light rectangle with a blue centre line (the skin
// provides the track). Matches the original "Presets" slider.
class PresetSliderLAF : public juce::LookAndFeel_V4
{
public:
    void drawLinearSlider (juce::Graphics& g, int x, int y, int w, int h,
                           float sliderPos, float, float,
                           const juce::Slider::SliderStyle, juce::Slider&) override
    {
        juce::ignoreUnused (x, w);
        const float hw = 14.f;
        const float hx = juce::jlimit ((float) x, (float) (x + w) - hw, sliderPos - hw * 0.5f);
        juce::Rectangle<float> handle (hx, (float) y, hw, (float) h);
        g.setColour (juce::Colour (0xffaeb9c2));
        g.fillRect (handle);
        g.setColour (juce::Colour (0xff3a4a56));
        g.drawRect (handle, 1.f);
        g.setColour (juce::Colour (0xff4f86c6));
        g.fillRect (handle.getCentreX() - 0.5f, (float) y + 2.f, 1.5f, (float) h - 4.f);
    }
};

//==============================================================================
// "Default" preset box — plain navy box, centred text, NO dropdown arrow.
class PresetBoxLAF : public juce::LookAndFeel_V4
{
public:
    void drawComboBox (juce::Graphics& g, int w, int h, bool, int, int, int, int,
                       juce::ComboBox& box) override
    {
        g.setColour (box.findColour (juce::ComboBox::backgroundColourId));
        g.fillRect (0, 0, w, h);
        g.setColour (box.findColour (juce::ComboBox::outlineColourId));
        g.drawRect (0, 0, w, h, 1);
        // intentionally no arrow
    }
    juce::Font getComboBoxFont (juce::ComboBox&) override
    { return juce::Font (juce::FontOptions().withHeight (13.0f)); }
    void positionComboBoxText (juce::ComboBox& box, juce::Label& label) override
    {
        label.setBounds (0, 0, box.getWidth(), box.getHeight());
        label.setJustificationType (juce::Justification::centred);
        label.setFont (getComboBoxFont (box));
    }
};

//==============================================================================
class QuadraFuzzAudioProcessorEditor  : public juce::AudioProcessorEditor,
                                        public juce::AudioProcessorValueTreeState::Listener,
                                        public juce::Timer
{
public:
    explicit QuadraFuzzAudioProcessorEditor (QuadraFuzzAudioProcessor&);
    ~QuadraFuzzAudioProcessorEditor() override;

    void paint   (juce::Graphics&) override;
    void resized () override;
    void timerCallback () override;
    void parameterChanged (const juce::String& paramID, float newValue) override;

private:
    QuadraFuzzAudioProcessor& audioProcessor;

    juce::Image      bgImage;
    FilmstripKnobLAF knobLAF;

    // 6 knobs in original UI order: Band1..Band4, In (Gain), Out (Output)
    static constexpr int NUM_KNOBS = 6;
    juce::Slider knobs[NUM_KNOBS];
    juce::OwnedArray<juce::AudioProcessorValueTreeState::SliderAttachment> knobAtts;
    juce::Label  dbLabels[NUM_KNOBS];   // "0.0dB" box above each knob

    // 5 shape buttons -> "Shape" param (global)
    static constexpr int NUM_SHAPES = 5;
    ShapeButton* shapeButtons[NUM_SHAPES] = {};

    juce::ComboBox  presetBox;          // "Default" dropdown
    PresetBoxLAF    presetBoxLAF;
    juce::Slider    presetSlider;       // horizontal slider -> switches presets
    PresetSliderLAF presetLAF;
    juce::OwnedArray<juce::AudioProcessorValueTreeState::SliderAttachment> presetAtt;
    int  lastPresetIdx  = -1;
    bool presetChanging = false;
    int  overHold       = 0;   // OVER lamp hold counter (timer ticks)
    void selectPreset (int idx, bool updateParam);
    void refreshPresets();          // rebuild dropdown items from the bank
    void createPresetDialog();      // "Create" button — name + snapshot
    void deleteCurrentPreset();     // "Delete" button

    juce::ToggleButton btnDelete, btnCreate, btnSolo;

    EQDisplay eqDisplay;

    void updateShapeButtons (int selectedIndex);
    void updateDbLabels();

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (QuadraFuzzAudioProcessorEditor)
};
