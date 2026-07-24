#include "PluginProcessor.h"
#include "PluginEditor.h"

using PID = VisageMainView::ParamId;

const char* PeakLFOAudioProcessorEditor::paramIdFor (PID id)
{
    switch (id)
    {
        case PID::Volume:  return "lfo_base";
        case PID::Depth:   return "lfo_depth";
        case PID::Tension: return "lfo_tension";
        case PID::Speed:   return "lfo_speed";
        case PID::Phase:   return "lfo_phase";
        default:           return nullptr;
    }
}

PeakLFOAudioProcessorEditor::PeakLFOAudioProcessorEditor (PeakLFOAudioProcessor& p)
    : VisagePluginEditor (p), audioProcessor (p)
{
    setSize (460, 300);
}

PeakLFOAudioProcessorEditor::~PeakLFOAudioProcessorEditor() = default;

void PeakLFOAudioProcessorEditor::onInit()
{
    mainView = std::make_unique<VisageMainView>();
    setEventRoot (mainView.get());

    mainView->setParamChangeCallback ([this] (PID id, float value01) {
        if (const char* pid = paramIdFor (id))
            if (auto* param = audioProcessor.parameters.getParameter (pid))
                param->setValueNotifyingHost (value01);   // choice params quantize automatically
    });
    mainView->setShapeChangeCallback ([this] (int shape) {
        if (auto* param = audioProcessor.parameters.getParameter ("lfo_shape"))
            param->setValueNotifyingHost (param->convertTo0to1 ((float) shape));
    });

    for (int i = 0; i < VisageMainView::kNumWheels; ++i)
    {
        const auto id = static_cast<PID> (i);
        if (auto* param = audioProcessor.parameters.getParameter (paramIdFor (id)))
            mainView->setWheel (id, param->getValue());
    }
    mainView->setShape ((int) std::lround (audioProcessor.parameters.getRawParameterValue ("lfo_shape")->load()));

    addFrameToCanvas (mainView.get());
    mainView->setBounds (0, 0, getWidth(), getHeight());
}

void PeakLFOAudioProcessorEditor::onRender()
{
    if (!mainView) return;
    for (int i = 0; i < VisageMainView::kNumWheels; ++i)
    {
        const auto id = static_cast<PID> (i);
        if (auto* param = audioProcessor.parameters.getParameter (paramIdFor (id)))
            mainView->setWheel (id, param->getValue());
    }
    mainView->setShape ((int) std::lround (audioProcessor.parameters.getRawParameterValue ("lfo_shape")->load()));
    mainView->setMeters (audioProcessor.meterLfo.load(), audioProcessor.meterGain.load());
}

void PeakLFOAudioProcessorEditor::onDestroy()
{
    if (mainView)
    {
        removeFrameFromCanvas (mainView.get());
        mainView.reset();
    }
}

void PeakLFOAudioProcessorEditor::onResize (int w, int h)
{
    if (mainView)
    {
        mainView->setBounds (0, 0, w, h);
        mainView->redraw();
    }
}
