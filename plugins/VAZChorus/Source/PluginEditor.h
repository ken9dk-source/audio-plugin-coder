#pragma once
#include <juce_audio_processors/juce_audio_processors.h>
#include <juce_gui_extra/juce_gui_extra.h>
#include "PluginProcessor.h"
#include "ParameterIDs.hpp"

class VAZChorusAudioProcessorEditor : public juce::AudioProcessorEditor
{
public:
    explicit VAZChorusAudioProcessorEditor (VAZChorusAudioProcessor&);
    ~VAZChorusAudioProcessorEditor() override;
    void paint (juce::Graphics&) override;
    void resized() override;
    int  getControlParameterIndex (juce::Component&) override;

private:
    VAZChorusAudioProcessor& audioProcessor;

    juce::WebSliderRelay delayRelay    { ParameterIDs::delay };
    juce::WebSliderRelay rateRelay     { ParameterIDs::rate };
    juce::WebSliderRelay depthRelay    { ParameterIDs::depth };
    juce::WebSliderRelay lrPhaseRelay  { ParameterIDs::lr_phase };
    juce::WebSliderRelay mixRelay      { ParameterIDs::mix };
    juce::WebSliderRelay gainRelay     { ParameterIDs::gain };
    juce::WebComboBoxRelay      waveformRelay  { ParameterIDs::waveform };
    juce::WebToggleButtonRelay modSyncRelay { ParameterIDs::mod_sync };
    juce::WebComboBoxRelay      modPeriodRelay { ParameterIDs::mod_period };

    juce::WebControlParameterIndexReceiver controlParamReceiver;
    std::unique_ptr<juce::WebBrowserComponent> webView;

    std::unique_ptr<juce::WebSliderParameterAttachment>
        delayAtt, rateAtt, depthAtt, lrPhaseAtt, mixAtt, gainAtt;
    std::unique_ptr<juce::WebToggleButtonParameterAttachment> modSyncAtt;
    std::unique_ptr<juce::WebComboBoxParameterAttachment>      waveformAtt, modPeriodAtt;

    std::optional<juce::WebBrowserComponent::Resource> getResource (const juce::String& url);
    static const char* getMimeForExtension (const juce::String& extension);

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (VAZChorusAudioProcessorEditor)
};
