#pragma once
#include <juce_audio_processors/juce_audio_processors.h>
#include <juce_gui_extra/juce_gui_extra.h>
#include "PluginProcessor.h"
#include "ParameterIDs.hpp"

//==============================================================================
// VAZCompressor editor — WebView UI. Member order: Relays → WebView → Attachments.
// A "getMeter" native function feeds the gain-reduction meter + clip LED to the UI.
//==============================================================================
class VAZCompressorAudioProcessorEditor : public juce::AudioProcessorEditor
{
public:
    explicit VAZCompressorAudioProcessorEditor (VAZCompressorAudioProcessor&);
    ~VAZCompressorAudioProcessorEditor() override;

    void paint (juce::Graphics&) override;
    void resized() override;

private:
    VAZCompressorAudioProcessor& audioProcessor;

    juce::WebSliderRelay thresholdRelay { ParameterIDs::threshold };
    juce::WebSliderRelay ratioRelay     { ParameterIDs::ratio };
    juce::WebSliderRelay attackRelay    { ParameterIDs::attack };
    juce::WebSliderRelay releaseRelay   { ParameterIDs::release };
    juce::WebSliderRelay makeupRelay    { ParameterIDs::makeup };

    std::unique_ptr<juce::WebBrowserComponent> webView;

    std::unique_ptr<juce::WebSliderParameterAttachment>
        thresholdAtt, ratioAtt, attackAtt, releaseAtt, makeupAtt;

    std::optional<juce::WebBrowserComponent::Resource> getResource (const juce::String& url);
    static const char* getMimeForExtension (const juce::String& extension);

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (VAZCompressorAudioProcessorEditor)
};
