#pragma once
#include <juce_audio_processors/juce_audio_processors.h>
#include <juce_gui_extra/juce_gui_extra.h>
#include <optional>
#include "PluginProcessor.h"

//==============================================================================
// WebView editor. Relays/attachments for every APVTS parameter are built
// generically by walking the parameter list, so adding a band needs no edits
// here. A 30 Hz Timer pushes the FFT spectrum + detected pitch into the page.
//==============================================================================
class TranceEQAudioProcessorEditor : public juce::AudioProcessorEditor,
                                     private juce::Timer
{
public:
    explicit TranceEQAudioProcessorEditor (TranceEQAudioProcessor&);
    ~TranceEQAudioProcessorEditor() override;

    void paint (juce::Graphics&) override;
    void resized() override;
    int  getControlParameterIndex (juce::Component&) override;

private:
    void timerCallback() override;
    std::optional<juce::WebBrowserComponent::Resource> getResource (const juce::String& url);

    TranceEQAudioProcessor& audioProcessor;

    juce::OwnedArray<juce::WebSliderRelay>        sliderRelays;
    juce::OwnedArray<juce::WebComboBoxRelay>      comboRelays;
    juce::OwnedArray<juce::WebToggleButtonRelay>  toggleRelays;

    juce::WebControlParameterIndexReceiver controlParamReceiver;
    std::unique_ptr<juce::WebBrowserComponent> webView;

    juce::OwnedArray<juce::WebSliderParameterAttachment>       sliderAtts;
    juce::OwnedArray<juce::WebComboBoxParameterAttachment>     comboAtts;
    juce::OwnedArray<juce::WebToggleButtonParameterAttachment> toggleAtts;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (TranceEQAudioProcessorEditor)
};
