#pragma once
#include <juce_audio_processors/juce_audio_processors.h>
#include <juce_gui_extra/juce_gui_extra.h>
#include "PluginProcessor.h"
#include "ParameterIDs.hpp"

class VAZAutopanAudioProcessorEditor : public juce::AudioProcessorEditor
{
public:
    explicit VAZAutopanAudioProcessorEditor (VAZAutopanAudioProcessor&);
    ~VAZAutopanAudioProcessorEditor() override;
    void paint (juce::Graphics&) override;
    void resized() override;

private:
    VAZAutopanAudioProcessor& audioProcessor;

    juce::WebSliderRelay       leftRelay  { ParameterIDs::left_limit };
    juce::WebSliderRelay       rightRelay { ParameterIDs::right_limit };
    juce::WebSliderRelay       rateRelay  { ParameterIDs::rate };
    juce::WebToggleButtonRelay waveRelay  { ParameterIDs::waveform_sine };

    std::unique_ptr<juce::WebBrowserComponent> webView;

    std::unique_ptr<juce::WebSliderParameterAttachment> leftAtt, rightAtt, rateAtt;
    std::unique_ptr<juce::WebToggleButtonParameterAttachment> waveAtt;

    std::optional<juce::WebBrowserComponent::Resource> getResource (const juce::String& url);
    static const char* getMimeForExtension (const juce::String& extension);

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (VAZAutopanAudioProcessorEditor)
};
