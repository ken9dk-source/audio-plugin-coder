#pragma once
#include <juce_audio_processors/juce_audio_processors.h>
#include <juce_gui_extra/juce_gui_extra.h>
#include "PluginProcessor.h"
#include "ParameterIDs.hpp"

class VAZPhaserAudioProcessorEditor : public juce::AudioProcessorEditor
{
public:
    explicit VAZPhaserAudioProcessorEditor (VAZPhaserAudioProcessor&);
    ~VAZPhaserAudioProcessorEditor() override;
    void paint (juce::Graphics&) override;
    void resized() override;

private:
    VAZPhaserAudioProcessor& audioProcessor;

    juce::WebSliderRelay stagesRelay   { ParameterIDs::stages };
    juce::WebSliderRelay frequencyRelay{ ParameterIDs::frequency };
    juce::WebSliderRelay feedbackRelay { ParameterIDs::feedback };
    juce::WebSliderRelay rateRelay     { ParameterIDs::rate };
    juce::WebSliderRelay depthRelay    { ParameterIDs::depth };
    juce::WebSliderRelay lrPhaseRelay  { ParameterIDs::lr_phase };
    juce::WebSliderRelay mixRelay      { ParameterIDs::mix };
    juce::WebSliderRelay gainRelay     { ParameterIDs::gain };
    juce::WebToggleButtonRelay fbPhaseRelay { ParameterIDs::feedback_phase };

    std::unique_ptr<juce::WebBrowserComponent> webView;

    std::unique_ptr<juce::WebSliderParameterAttachment>
        stagesAtt, frequencyAtt, feedbackAtt, rateAtt, depthAtt, lrPhaseAtt, mixAtt, gainAtt;
    std::unique_ptr<juce::WebToggleButtonParameterAttachment> fbPhaseAtt;

    std::optional<juce::WebBrowserComponent::Resource> getResource (const juce::String& url);
    static const char* getMimeForExtension (const juce::String& extension);

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (VAZPhaserAudioProcessorEditor)
};
