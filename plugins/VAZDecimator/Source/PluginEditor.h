#pragma once
#include <juce_audio_processors/juce_audio_processors.h>
#include <juce_gui_extra/juce_gui_extra.h>
#include "PluginProcessor.h"
#include "ParameterIDs.hpp"

//==============================================================================
// VAZDecimator editor — WebView UI. Member order: Relays → WebView → Attachments.
//==============================================================================
class VAZDecimatorAudioProcessorEditor : public juce::AudioProcessorEditor
{
public:
    explicit VAZDecimatorAudioProcessorEditor (VAZDecimatorAudioProcessor&);
    ~VAZDecimatorAudioProcessorEditor() override;

    void paint (juce::Graphics&) override;
    void resized() override;

private:
    VAZDecimatorAudioProcessor& audioProcessor;

    juce::WebSliderRelay sampleRateRelay { ParameterIDs::sample_rate };
    juce::WebSliderRelay bitDepthRelay   { ParameterIDs::bit_depth };

    std::unique_ptr<juce::WebBrowserComponent> webView;

    std::unique_ptr<juce::WebSliderParameterAttachment> sampleRateAtt, bitDepthAtt;

    std::optional<juce::WebBrowserComponent::Resource> getResource (const juce::String& url);
    static const char* getMimeForExtension (const juce::String& extension);

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (VAZDecimatorAudioProcessorEditor)
};
