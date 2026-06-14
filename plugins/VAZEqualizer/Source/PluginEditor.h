#pragma once
#include <juce_audio_processors/juce_audio_processors.h>
#include <juce_gui_extra/juce_gui_extra.h>
#include "PluginProcessor.h"
#include "ParameterIDs.hpp"

//==============================================================================
// VAZEqualizer editor — WebView UI. Member order: Relays → WebView → Attachments.
//==============================================================================
class VAZEqualizerAudioProcessorEditor : public juce::AudioProcessorEditor
{
public:
    explicit VAZEqualizerAudioProcessorEditor (VAZEqualizerAudioProcessor&);
    ~VAZEqualizerAudioProcessorEditor() override;

    void paint (juce::Graphics&) override;
    void resized() override;

private:
    VAZEqualizerAudioProcessor& audioProcessor;

    juce::WebSliderRelay lowGainRelay   { ParameterIDs::low_gain };
    juce::WebSliderRelay lowFreqRelay   { ParameterIDs::low_freq };
    juce::WebSliderRelay lowQRelay      { ParameterIDs::low_q };
    juce::WebSliderRelay loMidGainRelay { ParameterIDs::lomid_gain };
    juce::WebSliderRelay loMidFreqRelay { ParameterIDs::lomid_freq };
    juce::WebSliderRelay loMidQRelay    { ParameterIDs::lomid_q };
    juce::WebSliderRelay hiMidGainRelay { ParameterIDs::himid_gain };
    juce::WebSliderRelay hiMidFreqRelay { ParameterIDs::himid_freq };
    juce::WebSliderRelay hiMidQRelay    { ParameterIDs::himid_q };
    juce::WebSliderRelay highGainRelay  { ParameterIDs::high_gain };
    juce::WebSliderRelay highFreqRelay  { ParameterIDs::high_freq };
    juce::WebSliderRelay highQRelay     { ParameterIDs::high_q };

    std::unique_ptr<juce::WebBrowserComponent> webView;

    std::unique_ptr<juce::WebSliderParameterAttachment>
        lowGainAtt, lowFreqAtt, lowQAtt, loMidGainAtt, loMidFreqAtt, loMidQAtt,
        hiMidGainAtt, hiMidFreqAtt, hiMidQAtt, highGainAtt, highFreqAtt, highQAtt;

    std::optional<juce::WebBrowserComponent::Resource> getResource (const juce::String& url);
    static const char* getMimeForExtension (const juce::String& extension);

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (VAZEqualizerAudioProcessorEditor)
};
