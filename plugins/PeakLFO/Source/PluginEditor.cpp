#include "PluginProcessor.h"
#include "PluginEditor.h"

using PID = VisageMainView::ParamId;

const char* PeakLFOAudioProcessorEditor::paramIdFor (PID id)
{
    switch (id)
    {
        case PID::Volume:  return "lfo_volume";
        case PID::Base:    return "lfo_base";
        case PID::Tension: return "lfo_tension";
        case PID::Phase:   return "lfo_phase";
        case PID::Speed:   return nullptr;   // routed dynamically (sync vs free)
        default:           return nullptr;
    }
}

const char* PeakLFOAudioProcessorEditor::speedParamId() const
{
    const bool free = audioProcessor.parameters.getRawParameterValue ("lfo_sync")->load() > 0.5f;
    return free ? "lfo_rate" : "lfo_speed";
}

PeakLFOAudioProcessorEditor::PeakLFOAudioProcessorEditor (PeakLFOAudioProcessor& p)
    : VisagePluginEditor (p), audioProcessor (p)
{
    setSize (480, 320);
}

PeakLFOAudioProcessorEditor::~PeakLFOAudioProcessorEditor() = default;

void PeakLFOAudioProcessorEditor::onInit()
{
    mainView = std::make_unique<VisageMainView>();
    setEventRoot (mainView.get());

    mainView->setParamChangeCallback ([this] (PID id, float value01) {
        const char* pid = (id == PID::Speed) ? speedParamId() : paramIdFor (id);
        if (pid != nullptr)
            if (auto* param = audioProcessor.parameters.getParameter (pid))
                param->setValueNotifyingHost (value01);
    });
    mainView->setShapeChangeCallback ([this] (int shape) {
        if (auto* param = audioProcessor.parameters.getParameter ("lfo_shape"))
            param->setValueNotifyingHost (param->convertTo0to1 ((float) shape));
    });
    mainView->setSyncChangeCallback ([this] (bool free) {
        if (auto* param = audioProcessor.parameters.getParameter ("lfo_sync"))
            param->setValueNotifyingHost (free ? 1.0f : 0.0f);
    });

    pushStateToView();
    addFrameToCanvas (mainView.get());
    mainView->setBounds (0, 0, getWidth(), getHeight());
}

void PeakLFOAudioProcessorEditor::pushStateToView()
{
    if (!mainView) return;
    auto& p = audioProcessor.parameters;
    for (int i = 0; i < VisageMainView::kNumWheels; ++i)
    {
        const auto id = static_cast<PID> (i);
        const char* pid = (id == PID::Speed) ? speedParamId() : paramIdFor (id);
        if (pid != nullptr)
            if (auto* param = p.getParameter (pid))
                mainView->setWheel (id, param->getValue());
    }
    const bool free = p.getRawParameterValue ("lfo_sync")->load() > 0.5f;
    mainView->setFree (free);
    mainView->setShape ((int) std::lround (p.getRawParameterValue ("lfo_shape")->load()));

    // speed read-out: steps (sync) or Hz (free)
    if (free)
    {
        const float hz = p.getRawParameterValue ("lfo_rate")->load();
        char b[24]; std::snprintf (b, sizeof (b), "%.2f Hz", hz);
        mainView->setSpeedLabel (b);
    }
    else
    {
        const int idx = (int) std::lround (p.getRawParameterValue ("lfo_speed")->load());
        mainView->setSpeedLabel (PeakLFOAudioProcessor::speedStepNames()[juce::jlimit (0, 9, idx)].toStdString());
    }
}

void PeakLFOAudioProcessorEditor::onRender()
{
    if (!mainView) return;
    pushStateToView();
    mainView->setMeters (audioProcessor.meterLfo.load(), audioProcessor.meterGain.load());
}

void PeakLFOAudioProcessorEditor::onDestroy()
{
    if (mainView) { removeFrameFromCanvas (mainView.get()); mainView.reset(); }
}

void PeakLFOAudioProcessorEditor::onResize (int w, int h)
{
    if (mainView) { mainView->setBounds (0, 0, w, h); mainView->redraw(); }
}
