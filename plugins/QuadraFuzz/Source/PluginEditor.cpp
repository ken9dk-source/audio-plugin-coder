#include "PluginProcessor.h"
#include "PluginEditor.h"
#include "BinaryData.h"

// ---- Pixel-measured layout (from the original DLL editor capture) -------
// Per band column: waveform box (skin) -> "0.0dB" box -> knob -> label (skin).
static const juce::Rectangle<int> KNOB_BOUNDS[6] = {
    {  38, 61, 48, 48 },   // Band1 – Low
    { 118, 61, 48, 48 },   // Band2 – Low Mid
    { 198, 61, 48, 48 },   // Band3 – High Mid
    { 278, 61, 48, 48 },   // Band4 – High
    {  38,143, 48, 48 },   // In  – Gain  (bottom-left)
    { 278,143, 48, 48 },   // Out – Output (bottom-right)
};

static const juce::Rectangle<int> DB_LABEL_BOUNDS[6] = {
    {  38, 46, 48, 14 }, { 118, 46, 48, 14 }, { 198, 46, 48, 14 }, { 278, 46, 48, 14 },
    {  38,128, 48, 14 }, { 278,128, 48, 14 },
};

// Filmstrip buttons aligned 1:1 over the skin's 5 buttons. Pixel-measured from
// the original: each skin button frame is x683..715 / y49..61, and the 35x16
// filmstrip has 1px top + 2px right/bottom panel-coloured padding with its frame
// at img(x0,y1). Placing the native 35x16 strip at {683,48} maps img(x0,y1) ->
// skin(683,49) exactly, so the strip overlays the skin button seamlessly.
static const juce::Rectangle<int> SHAPE_BOUNDS[5] = {
    { 683,  48, 35, 16 }, { 683, 78, 35, 16 }, { 683, 108, 35, 16 },
    { 683, 138, 35, 16 }, { 683, 168, 35, 16 },
};

static const char* const KNOB_IDS[6] = { "Band1", "Band2", "Band3", "Band4", "In", "Out" };

static const char* const SHAPE_DATA[5] = {
    BinaryData::_1006_png, BinaryData::_1007_png, BinaryData::_1008_png,
    BinaryData::_1009_png, BinaryData::_1010_png
};
static const int SHAPE_DATA_SIZE[5] = {
    BinaryData::_1006_pngSize, BinaryData::_1007_pngSize, BinaryData::_1008_pngSize,
    BinaryData::_1009_pngSize, BinaryData::_1010_pngSize
};

//==============================================================================
QuadraFuzzAudioProcessorEditor::QuadraFuzzAudioProcessorEditor (QuadraFuzzAudioProcessor& p)
    : AudioProcessorEditor (&p),
      audioProcessor (p),
      bgImage (juce::ImageCache::getFromMemory (BinaryData::_1000_png, BinaryData::_1000_pngSize)),
      knobLAF (juce::ImageCache::getFromMemory (BinaryData::_1012_png, BinaryData::_1012_pngSize)),
      eqDisplay (p)
{
    for (int i = 0; i < NUM_KNOBS; ++i)
    {
        auto& s = knobs[i];
        s.setLookAndFeel (&knobLAF);
        s.setSliderStyle (juce::Slider::RotaryVerticalDrag);
        s.setTextBoxStyle (juce::Slider::NoTextBox, true, 0, 0);
        s.setScrollWheelEnabled (true);
        addAndMakeVisible (s);
        knobAtts.add (new juce::AudioProcessorValueTreeState::SliderAttachment (
            audioProcessor.apvts, KNOB_IDS[i], s));

        auto& lbl = dbLabels[i];
        lbl.setJustificationType (juce::Justification::centred);
        lbl.setFont (juce::Font (juce::FontOptions().withHeight (11.f).withStyle ("Bold")));
        lbl.setColour (juce::Label::textColourId,       juce::Colour (0xff58c0e0));
        lbl.setColour (juce::Label::backgroundColourId, juce::Colour (0xff041420));
        lbl.setInterceptsMouseClicks (false, false);
        addAndMakeVisible (lbl);
    }

    // Shape buttons -> single global "Shape" param
    for (int i = 0; i < NUM_SHAPES; ++i)
    {
        auto img = juce::ImageCache::getFromMemory (SHAPE_DATA[i], SHAPE_DATA_SIZE[i]);
        shapeButtons[i] = new ShapeButton (std::move (img), i);
        addAndMakeVisible (shapeButtons[i]);
        const int idx = i;
        shapeButtons[i]->onClick = [this, idx]()
        {
            updateShapeButtons (idx);   // single click -> instantly gold
            if (auto* prm = audioProcessor.apvts.getParameter ("Shape"))
                prm->setValueNotifyingHost (prm->convertTo0to1 ((float) idx));
        };
    }
    updateShapeButtons ((int) audioProcessor.apvts.getRawParameterValue ("Shape")->load());
    audioProcessor.apvts.addParameterListener ("Shape", this);

    addAndMakeVisible (eqDisplay);

    // Preset dropdown — populated from the disk-backed bank (plain navy box, no arrow)
    presetBox.setLookAndFeel (&presetBoxLAF);
    presetBox.setJustificationType (juce::Justification::centred);
    presetBox.setColour (juce::ComboBox::backgroundColourId, juce::Colour (0xff041420));
    presetBox.setColour (juce::ComboBox::textColourId,       juce::Colour (0xff9fc4d4));
    presetBox.setColour (juce::ComboBox::outlineColourId,    juce::Colour (0xff223844));
    presetBox.onChange = [this]()
    {
        // Apply the SELECTED preset directly (and sync the Preset param/slider).
        selectPreset (presetBox.getSelectedId() - 1, true);
    };
    addAndMakeVisible (presetBox);
    refreshPresets();   // fill items from the bank + select the current preset

    // Horizontal Presets slider — switches through the factory presets
    presetSlider.setSliderStyle (juce::Slider::LinearHorizontal);
    presetSlider.setTextBoxStyle (juce::Slider::NoTextBox, true, 0, 0);
    presetSlider.setLookAndFeel (&presetLAF);
    addAndMakeVisible (presetSlider);
    presetAtt.add (new juce::AudioProcessorValueTreeState::SliderAttachment (
        audioProcessor.apvts, "Preset", presetSlider));
    audioProcessor.apvts.addParameterListener ("Preset", this);

    for (auto* btn : { &btnDelete, &btnCreate, &btnSolo })
    {
        btn->setColour (juce::ToggleButton::textColourId,         juce::Colours::transparentBlack);
        btn->setColour (juce::ToggleButton::tickColourId,         juce::Colour (0xff40b0d0));
        btn->setColour (juce::ToggleButton::tickDisabledColourId, juce::Colours::transparentBlack);
        addAndMakeVisible (btn);
    }
    // Create = capture current knobs/EQ as a new named preset; Delete = remove current.
    btnCreate.onClick = [this]() { createPresetDialog(); };
    btnDelete.onClick = [this]() { deleteCurrentPreset(); };

    setSize (730, 213);
    startTimerHz (20);
}

QuadraFuzzAudioProcessorEditor::~QuadraFuzzAudioProcessorEditor()
{
    stopTimer();
    audioProcessor.apvts.removeParameterListener ("Shape", this);
    audioProcessor.apvts.removeParameterListener ("Preset", this);
    presetSlider.setLookAndFeel (nullptr);
    presetBox.setLookAndFeel (nullptr);
    for (auto& s : knobs) s.setLookAndFeel (nullptr);
    for (int i = 0; i < NUM_SHAPES; ++i) delete shapeButtons[i];
}

//==============================================================================
void QuadraFuzzAudioProcessorEditor::paint (juce::Graphics& g)
{
    if (bgImage.isValid()) g.drawImage (bgImage, getLocalBounds().toFloat());
    else                   g.fillAll (juce::Colour (0xffa2adaf));

    // OVER lamp — bright red over the skin's "Over" dot when the processor flagged
    // a waveshaper clip (|input|>1.0). Position pixel-measured from the skin (104,138).
    if (overHold > 0)
    {
        g.setColour (juce::Colour (0xffff2a18));
        g.fillEllipse (100.5f, 134.0f, 8.0f, 8.0f);
    }
}

void QuadraFuzzAudioProcessorEditor::resized()
{
    for (int i = 0; i < NUM_KNOBS; ++i)
    {
        knobs[i].setBounds (KNOB_BOUNDS[i]);
        dbLabels[i].setBounds (DB_LABEL_BOUNDS[i]);
    }
    for (int i = 0; i < NUM_SHAPES; ++i)
        shapeButtons[i]->setBounds (SHAPE_BOUNDS[i]);

    eqDisplay.setBounds (377, 30, 290, 156);

    presetBox.setBounds (121, 127, 122, 16);
    presetSlider.setBounds (110, 148, 136, 15);   // over the skin's preset track

    btnDelete.setBounds (400, 9, 13, 13);
    btnCreate.setBounds (465, 9, 13, 13);
    btnSolo  .setBounds (525, 9, 13, 13);
}

//==============================================================================
void QuadraFuzzAudioProcessorEditor::timerCallback()
{
    updateDbLabels();

    // Poll the OVER flag (read-and-clear, like the DLL's FUN_1000a110). Hold the
    // lamp ~300 ms for visibility (the original's hold = its editIdle interval).
    const bool wasLit = (overHold > 0);
    if (audioProcessor.readAndClearOver()) overHold = 6;     // 6 ticks @ 20 Hz
    else if (overHold > 0)                 --overHold;
    if ((overHold > 0) != wasLit) repaint (100, 133, 10, 10);
}

void QuadraFuzzAudioProcessorEditor::updateDbLabels()
{
    for (int i = 0; i < NUM_KNOBS; ++i)
    {
        const float v = audioProcessor.apvts.getRawParameterValue (KNOB_IDS[i])->load();
        dbLabels[i].setText (juce::String (v, 1) + "dB", juce::dontSendNotification);
    }
}

void QuadraFuzzAudioProcessorEditor::parameterChanged (const juce::String& paramID, float newValue)
{
    if (paramID == "Shape")
    {
        // APVTS delivers the DENORMALISED value here; for an AudioParameterChoice
        // that is already the 0..4 index — do NOT rescale it (the old *(N-1) turned
        // idx 1 into 4 and 2..4 out of range). Read it straight, like the ctor does.
        const int idx = juce::jlimit (0, NUM_SHAPES - 1,
                                      (int) audioProcessor.apvts.getRawParameterValue ("Shape")->load());
        juce::MessageManager::callAsync ([this, idx]() { updateShapeButtons (idx); });
    }
    else if (paramID == "Preset")
    {
        const int N   = juce::jmax (1, audioProcessor.getNumPresets());
        const int idx = juce::jlimit (0, N - 1, juce::roundToInt (newValue * (N - 1)));
        juce::MessageManager::callAsync ([this, idx]() { selectPreset (idx, false); });
    }
}

// Single entry point for preset changes (dropdown AND slider/param). Always
// applies the SELECTED index — no early-return that could leave the name
// updated while the values stay on the previous preset.
void QuadraFuzzAudioProcessorEditor::selectPreset (int idx, bool updateParam)
{
    const int N = juce::jmax (1, audioProcessor.getNumPresets());
    idx = juce::jlimit (0, N - 1, idx);

    if (presetChanging) return;          // re-entrancy guard only (not a "same idx" skip)
    presetChanging = true;

    lastPresetIdx = idx;
    audioProcessor.applyPreset (idx);
    presetBox.setSelectedId (idx + 1, juce::dontSendNotification);
    if (updateParam)
        if (auto* p = audioProcessor.apvts.getParameter ("Preset"))
            p->setValueNotifyingHost ((N > 1) ? (float) idx / (float) (N - 1) : 0.f);

    presetChanging = false;
}

//==============================================================================
// Rebuild the dropdown items from the processor's bank and select the current one.
void QuadraFuzzAudioProcessorEditor::refreshPresets()
{
    const bool wasChanging = presetChanging;
    presetChanging = true;               // suppress onChange while we repopulate
    presetBox.clear (juce::dontSendNotification);
    const int n = audioProcessor.getNumPresets();
    for (int i = 0; i < n; ++i)
    {
        auto nm = audioProcessor.getPresetName (i);
        presetBox.addItem (nm.isNotEmpty() ? nm : ("Preset " + juce::String (i)), i + 1);
    }
    const int cur = juce::jlimit (0, juce::jmax (0, n - 1), audioProcessor.getCurrentPreset());
    presetBox.setSelectedId (cur + 1, juce::dontSendNotification);
    lastPresetIdx = cur;
    presetChanging = wasChanging;
}

// "Create" — ask for a name, snapshot the current knobs/EQ into a new preset.
void QuadraFuzzAudioProcessorEditor::createPresetDialog()
{
    auto* aw = new juce::AlertWindow ("Create preset",
                                      "Name of new preset:",
                                      juce::MessageBoxIconType::NoIcon);
    aw->addTextEditor ("name", juce::String(), juce::String());
    aw->addButton ("OK",     1, juce::KeyPress (juce::KeyPress::returnKey));
    aw->addButton ("Cancel", 0, juce::KeyPress (juce::KeyPress::escapeKey));
    aw->enterModalState (true, juce::ModalCallbackFunction::create ([this, aw] (int result)
    {
        if (result == 1)
        {
            const auto name = aw->getTextEditorContents ("name").trim();
            const int idx = audioProcessor.createPreset (name);
            if (idx >= 0) { refreshPresets(); selectPreset (idx, true); }
        }
        delete aw;
    }), false);
}

// "Delete" — remove the current preset (the bank always keeps at least one).
void QuadraFuzzAudioProcessorEditor::deleteCurrentPreset()
{
    audioProcessor.deletePreset (audioProcessor.getCurrentPreset());
    refreshPresets();
    selectPreset (audioProcessor.getCurrentPreset(), true);
}

void QuadraFuzzAudioProcessorEditor::updateShapeButtons (int selectedIndex)
{
    for (int i = 0; i < NUM_SHAPES; ++i)
        shapeButtons[i]->setToggleState (i == selectedIndex, juce::dontSendNotification);
}
