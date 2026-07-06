#pragma once
#include <juce_audio_processors/juce_audio_processors.h>
#include <juce_gui_basics/juce_gui_basics.h>
#include <array>
#include "PluginProcessor.h"

//==============================================================================
// Native dark-trance UI. Editor is a DragAndDropContainer so the "DRAG MIDI"
// strip can start an OS-level drag of the exported .mid into the DAW.
//==============================================================================
class TranceChordsAudioProcessorEditor : public juce::AudioProcessorEditor,
                                         private juce::Timer,
                                         private juce::AudioProcessorValueTreeState::Listener,
                                         public  juce::DragAndDropContainer,
                                         public  juce::FileDragAndDropTarget
{
public:
    explicit TranceChordsAudioProcessorEditor (TranceChordsAudioProcessor&);
    ~TranceChordsAudioProcessorEditor() override;

    void paint (juce::Graphics&) override;
    void resized() override;
    void mouseDown (const juce::MouseEvent&) override;

    // accept a dropped .mid melody (melody-aware generation)
    bool isInterestedInFileDrag (const juce::StringArray& files) override;
    void filesDropped (const juce::StringArray& files, int x, int y) override;

private:
    void timerCallback() override;
    void parameterChanged (const juce::String& id, float newValue) override;

    using SAtt = juce::AudioProcessorValueTreeState::SliderAttachment;
    using CAtt = juce::AudioProcessorValueTreeState::ComboBoxAttachment;
    using BAtt = juce::AudioProcessorValueTreeState::ButtonAttachment;

    struct Knob
    {
        juce::Slider slider;
        juce::Label  label;
        std::unique_ptr<SAtt> att;
    };
    Knob& addKnob (const char* paramId, const juce::String& name);

    void setupCombo (juce::ComboBox& box, const char* paramId, std::unique_ptr<CAtt>& att);
    void addToggle (const char* paramId, const juce::String& name);

    // shared LookAndFeel — clean accent-arc rotary that scales with the knob size
    struct KnobLnf : public juce::LookAndFeel_V4
    {
        juce::Colour accent, panel, text, dim;
        void drawRotarySlider (juce::Graphics&, int x, int y, int w, int h,
                               float pos, float start, float end, juce::Slider&) override;
    };

    // chord-card layout / hit-testing
    juce::Rectangle<int> cardArea;
    std::vector<juce::Rectangle<int>> cardRects;
    void layoutCards();

    // new-layout regions (set in resized(), drawn in paint())
    std::array<juce::Rectangle<int>, 3> setupPills;
    std::array<juce::Rectangle<int>, 4> layerCards;
    juce::Rectangle<int> melodyStrip, tabBar, tabBody;

    // tabbed sound section + collapsible keyboard
    juce::TextButton genTabBtn { "GENERATOR" }, toneTabBtn { "TONE & FX" }, keyboardBtn;
    int  activeTab = 0;             // 0 = generator, 1 = tone & fx
    bool keyboardOpen = false;
    void updateTabVisibility();

    // visualization
    juce::Rectangle<int> keyboardArea, timelineArea, meterArea;
    void drawKeyboard (juce::Graphics&);
    void drawTimeline (juce::Graphics&);
    void drawMeters (juce::Graphics&);

    //== an OS-drag strip (one per exportable layer) ==
    struct DragStrip : public juce::Component
    {
        std::function<void (DragStrip*)> onDrag;
        juce::String text;
        int  layer = 0;          // 0=chords, 1=bass, 2=arp
        bool armed = false;
        void mouseDown (const juce::MouseEvent&) override { armed = true; }
        void mouseDrag (const juce::MouseEvent&) override { if (armed && onDrag) { armed = false; onDrag (this); } }
        void paint (juce::Graphics&) override;
    };

    TranceChordsAudioProcessor& proc;
    KnobLnf lnf;

    // header combos
    juce::ComboBox keyBox, modeBox, sectionBox, moodBox, styleBox, lengthBox, densityBox, presetBox;
    std::unique_ptr<CAtt> keyAtt, modeAtt, sectionAtt, moodAtt, styleAtt, lengthAtt, densityAtt;

    // knobs (generation + pad)
    std::vector<std::unique_ptr<Knob>> knobs;

    // toggles
    std::vector<std::unique_ptr<juce::ToggleButton>> toggles;
    std::vector<std::unique_ptr<BAtt>> toggleAtts;

    juce::TextButton generateBtn { "GENERATE" };
    juce::TextButton playBtn      { "PLAY" };
    juce::Label      titleLabel, bpmLabel, hintLabel;

    // history + favorites
    juce::TextButton undoBtn, redoBtn, saveFavBtn;
    juce::ComboBox   favoritesBox;
    int lastFavCount = -1;
    void refreshFavorites();

    // layer (bass/arp/counter) controls
    juce::ToggleButton bassEnableBtn { "Bass" }, arpEnableBtn { "Arp" }, counterEnableBtn { "Counter" };
    std::unique_ptr<BAtt> bassEnableAtt, arpEnableAtt, counterEnableAtt;
    juce::ComboBox bassPatternBox, arpPatternBox, arpRateBox, counterPatternBox, counterRateBox;
    std::unique_ptr<CAtt> bassPatternAtt, arpPatternAtt, arpRateAtt, counterPatternAtt, counterRateAtt;

    // song mode
    juce::ToggleButton songModeBtn { "Song" };
    std::unique_ptr<BAtt> songModeAtt;
    juce::ComboBox songFormBox;
    std::unique_ptr<CAtt> songFormAtt;

    // layer mixer (level faders live inside the layer cards)
    juce::Slider mixChordsSlider, mixBassSlider, mixArpSlider, mixCounterSlider;
    std::unique_ptr<SAtt> mixChordsAtt, mixBassAtt, mixArpAtt, mixCounterAtt;

    // voicing style
    juce::ComboBox voicingStyleBox;
    std::unique_ptr<CAtt> voicingStyleAtt;

    // melody-aware controls
    juce::ToggleButton melodyFitBtn { "Melody Fit" };
    std::unique_ptr<BAtt> melodyFitAtt;
    juce::Label      melodyLabel;
    juce::TextButton clearMelodyBtn { "Clear" };
    bool fileDragHover = false;

    DragStrip dragChords, dragBass, dragArp, dragCounter;

    std::atomic<bool> needsRegen { false }, needsRevoice { false };
    int lastVersion = -1;
    int lastEnableMask = -1;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (TranceChordsAudioProcessorEditor)
};
