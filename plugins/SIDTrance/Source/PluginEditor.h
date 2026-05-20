#pragma once

#include <JuceHeader.h>
#include "PluginProcessor.h"
#include "VisageControls.h"
#include "VisageJuceHost.h"

//==============================================================================
class SIDTranceAudioProcessorEditor : public VisagePluginEditor
{
public:
    explicit SIDTranceAudioProcessorEditor(SIDTranceAudioProcessor&);
    ~SIDTranceAudioProcessorEditor() override;

    void onInit()   override;
    void onRender() override;
    void onDestroy() override;
    void onResize(int w, int h) override;

private:
    void addAllFrames();
    void bindAllParameters();

    SIDTranceAudioProcessor& audioProcessor;
    std::unique_ptr<SIDMainView> mainView;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(SIDTranceAudioProcessorEditor)
};
