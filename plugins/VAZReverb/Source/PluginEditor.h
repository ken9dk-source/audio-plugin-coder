#pragma once
#include <juce_audio_processors/juce_audio_processors.h>
#include <juce_gui_extra/juce_gui_extra.h>
#include "PluginProcessor.h"
#include "ParameterIDs.hpp"

//==============================================================================
// VAZReverb editor — WebView UI. Member order: Relays → WebView → Attachments.
//==============================================================================
class VAZReverbAudioProcessorEditor : public juce::AudioProcessorEditor
{
public:
    explicit VAZReverbAudioProcessorEditor (VAZReverbAudioProcessor&);
    ~VAZReverbAudioProcessorEditor() override;

    void paint (juce::Graphics&) override;
    void resized() override;

private:
    VAZReverbAudioProcessor& audioProcessor;

    juce::WebSliderRelay reverbTimeRelay { ParameterIDs::reverb_time };
    juce::WebSliderRelay toneRelay       { ParameterIDs::tone };
    juce::WebSliderRelay mixRelay        { ParameterIDs::mix };

    std::unique_ptr<juce::WebBrowserComponent> webView;

    std::unique_ptr<juce::WebSliderParameterAttachment> reverbTimeAtt, toneAtt, mixAtt;

    std::optional<juce::WebBrowserComponent::Resource> getResource (const juce::String& url);
    static const char* getMimeForExtension (const juce::String& extension);

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (VAZReverbAudioProcessorEditor)
};
