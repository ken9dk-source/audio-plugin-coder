#pragma once
#include <juce_audio_processors/juce_audio_processors.h>
#include <juce_gui_extra/juce_gui_extra.h>
#include "PluginProcessor.h"
#include "ParameterIDs.hpp"

class VAZDelayAudioProcessorEditor : public juce::AudioProcessorEditor
{
public:
    explicit VAZDelayAudioProcessorEditor (VAZDelayAudioProcessor&);
    ~VAZDelayAudioProcessorEditor() override;
    void paint (juce::Graphics&) override;
    void resized() override;

private:
    VAZDelayAudioProcessor& audioProcessor;

    juce::WebComboBoxRelay     modeRelay { ParameterIDs::mode };
    juce::WebToggleButtonRelay linkRelay { ParameterIDs::link };
    juce::WebToggleButtonRelay syncRelay { ParameterIDs::sync };
    juce::WebSliderRelay dlLRelay { ParameterIDs::delay_l }, fbLRelay { ParameterIDs::fb_l },
                         tnLRelay { ParameterIDs::tone_l },  wLRelay  { ParameterIDs::wet_l },
                         dryLRelay { ParameterIDs::dry_l };
    juce::WebSliderRelay dlRRelay { ParameterIDs::delay_r }, fbRRelay { ParameterIDs::fb_r },
                         tnRRelay { ParameterIDs::tone_r },  wRRelay  { ParameterIDs::wet_r },
                         dryRRelay { ParameterIDs::dry_r };

    std::unique_ptr<juce::WebBrowserComponent> webView;

    std::unique_ptr<juce::WebComboBoxParameterAttachment>     modeAtt;
    std::unique_ptr<juce::WebToggleButtonParameterAttachment> linkAtt, syncAtt;
    std::unique_ptr<juce::WebSliderParameterAttachment>
        dlLAtt, fbLAtt, tnLAtt, wLAtt, dryLAtt,
        dlRAtt, fbRAtt, tnRAtt, wRAtt, dryRAtt;

    std::optional<juce::WebBrowserComponent::Resource> getResource (const juce::String& url);
    static const char* getMimeForExtension (const juce::String& extension);

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (VAZDelayAudioProcessorEditor)
};
