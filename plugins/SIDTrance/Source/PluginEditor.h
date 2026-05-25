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

    // ── Preset system ──────────────────────────────────────────────────────
    struct PresetEntry {
        juce::String name;
        bool         isFactory = true;
        juce::File   userFile;   // valid for user presets only
    };

    void initPresets();
    void loadPreset        (int index);
    void applyFactoryPreset(int factoryIndex);
    void resetToInit();
    void prevPreset();
    void nextPreset();
    void savePreset();
    void savePresetAs();
    void savePresetWithName(const juce::String& name);
    void updatePresetBar();

    juce::Array<PresetEntry> presets_;
    int  currentPresetIdx_ = -1;   // -1 = Init / unsaved
    bool presetDirty_      = false;

    SIDTranceAudioProcessor& audioProcessor;
    std::unique_ptr<SIDMainView> mainView;

    // Per-instance render scratch buffers (were static locals — shared across instances, wrong).
    // kScopeMax (512) matches SIDOscilloscopeView::kMaxSamples and the .cpp local constant.
    static constexpr int kScopeMax  = 512;
    float adsrBuf_[512]          = {};
    float wvBuf_[512]            = {};
    float localBuf_[kScopeMax]   = {};

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(SIDTranceAudioProcessorEditor)
    JUCE_DECLARE_WEAK_REFERENCEABLE(SIDTranceAudioProcessorEditor)
};
