#pragma once
//==============================================================================
// Phase 0 placeholder editor: generic parameter panel so the full APVTS surface
// can be exercised in a host today. Replaced by the custom rack GUI in Phase 8.
//==============================================================================
#include <juce_audio_utils/juce_audio_utils.h>
#include "PluginProcessor.h"

class MidBassAudioProcessorEditor : public juce::AudioProcessorEditor
{
public:
    explicit MidBassAudioProcessorEditor (MidBassAudioProcessor& p)
        : AudioProcessorEditor (p), generic (p)
    {
        addAndMakeVisible (generic);
        setSize (560, 720);
        setResizable (true, true);
    }

    void resized() override { generic.setBounds (getLocalBounds()); }

private:
    juce::GenericAudioProcessorEditor generic;
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (MidBassAudioProcessorEditor)
};
