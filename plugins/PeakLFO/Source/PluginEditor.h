#pragma once

#include <memory>
#include "PluginProcessor.h"
#include "VisageControls.h"
#include "VisageJuceHost.h"

//==============================================================================
class PeakLFOAudioProcessorEditor : public VisagePluginEditor
{
public:
    explicit PeakLFOAudioProcessorEditor (PeakLFOAudioProcessor&);
    ~PeakLFOAudioProcessorEditor() override;

    void onInit() override;
    void onRender() override;
    void onDestroy() override;
    void onResize (int w, int h) override;

private:
    static const char* paramIdFor (VisageMainView::ParamId id);
    const char* speedParamId() const;
    void pushStateToView();

    PeakLFOAudioProcessor& audioProcessor;
    std::unique_ptr<VisageMainView> mainView;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (PeakLFOAudioProcessorEditor)
};
