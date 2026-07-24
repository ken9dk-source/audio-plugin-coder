#include "PluginProcessor.h"
#include "PluginEditor.h"
#include <cmath>

constexpr float PeakLFOAudioProcessor::kSpeedSteps[];

const juce::StringArray& PeakLFOAudioProcessor::speedStepNames()
{
    static const juce::StringArray names {
        "1/2 step", "1 step", "2 steps", "3 steps", "4 steps",
        "8 steps", "16 steps", "32 steps", "64 steps", "128 steps"
    };
    return names;
}

const juce::StringArray& PeakLFOAudioProcessor::phaseNames()
{
    static const juce::StringArray names { "0%", "25%", "50%", "75%" };
    return names;
}

//==============================================================================
PeakLFOAudioProcessor::PeakLFOAudioProcessor()
    : AudioProcessor (BusesProperties()
        .withInput  ("Input",  juce::AudioChannelSet::stereo(), true)
        .withOutput ("Output", juce::AudioChannelSet::stereo(), true)),
      parameters (*this, nullptr, juce::Identifier ("PeakLFO"), createParameterLayout())
{
}

PeakLFOAudioProcessor::~PeakLFOAudioProcessor() = default;

//==============================================================================
juce::AudioProcessorValueTreeState::ParameterLayout PeakLFOAudioProcessor::createParameterLayout()
{
    using AF = juce::AudioParameterFloat;
    juce::AudioProcessorValueTreeState::ParameterLayout layout;
    auto bip = juce::NormalisableRange<float> (-1.0f, 1.0f, 0.001f);
    auto uni = juce::NormalisableRange<float> ( 0.0f, 1.0f, 0.001f);

    layout.add (std::make_unique<AF> (juce::ParameterID { "lfo_base", 1 },    "Volume",  uni, 0.75f));
    layout.add (std::make_unique<AF> (juce::ParameterID { "lfo_depth", 1 },   "Depth",   uni, 0.5f));
    layout.add (std::make_unique<AF> (juce::ParameterID { "lfo_tension", 1 }, "Tension", bip, 0.0f));
    layout.add (std::make_unique<juce::AudioParameterChoice> (juce::ParameterID { "lfo_speed", 1 },
                "Speed", speedStepNames(), 6 /* 16 steps = 1 bar */));
    layout.add (std::make_unique<juce::AudioParameterChoice> (juce::ParameterID { "lfo_shape", 1 },
                "Shape", juce::StringArray { "Sine", "Triangle", "Square", "Random" }, 0));
    layout.add (std::make_unique<juce::AudioParameterChoice> (juce::ParameterID { "lfo_phase", 1 },
                "Phase", phaseNames(), 0));
    return layout;
}

//==============================================================================
void PeakLFOAudioProcessor::prepareToPlay (double sampleRate, int)
{
    currentSampleRate = sampleRate;
    engine.prepare (sampleRate);
    smBase.reset  (sampleRate, 0.02);
    smDepth.reset (sampleRate, 0.02);
    smBase.setCurrentAndTargetValue  (parameters.getRawParameterValue ("lfo_base")->load());
    smDepth.setCurrentAndTargetValue (parameters.getRawParameterValue ("lfo_depth")->load());
}

bool PeakLFOAudioProcessor::isBusesLayoutSupported (const BusesLayout& layouts) const
{
    const auto& in  = layouts.getMainInputChannelSet();
    const auto& out = layouts.getMainOutputChannelSet();
    if (out != juce::AudioChannelSet::mono() && out != juce::AudioChannelSet::stereo())
        return false;
    return in == out;
}

//==============================================================================
void PeakLFOAudioProcessor::processBlock (juce::AudioBuffer<float>& buffer, juce::MidiBuffer&)
{
    juce::ScopedNoDenormals noDenormals;
    const int nIn  = getTotalNumInputChannels();
    const int nOut = getTotalNumOutputChannels();
    const int N    = buffer.getNumSamples();
    for (int ch = nIn; ch < nOut; ++ch) buffer.clear (ch, 0, N);
    if (N == 0) return;

    // ---- parameters ----
    const float baseTarget  = parameters.getRawParameterValue ("lfo_base")->load();
    const float depthTarget = parameters.getRawParameterValue ("lfo_depth")->load();
    const float tension     = parameters.getRawParameterValue ("lfo_tension")->load();
    const int   speedIdx    = (int) std::lround (parameters.getRawParameterValue ("lfo_speed")->load());
    const int   shapeIdx    = (int) std::lround (parameters.getRawParameterValue ("lfo_shape")->load());
    const int   phaseIdx    = (int) std::lround (parameters.getRawParameterValue ("lfo_phase")->load());

    engine.setLfoTension ((int) std::lround (tension * TENSION_FULL));
    engine.setShape (shapeIdx);
    smBase.setTargetValue (baseTarget);
    smDepth.setTargetValue (depthTarget);

    const float steps = kSpeedSteps[juce::jlimit (0, kNumSpeedSteps - 1, speedIdx)];
    const double beatsPerCycle = (double) steps / 4.0;          // 4 steps = 1 beat
    const double phaseOffset   = 0.25 * (double) juce::jlimit (0, 3, phaseIdx);

    // ---- transport ----
    double bpm = 120.0, ppq = 0.0; bool playing = false;
    if (auto* ph = getPlayHead())
        if (auto pos = ph->getPosition())
        {
            if (auto b = pos->getBpm())          bpm = *b;
            if (auto q = pos->getPpqPosition())  ppq = *q;
            playing = pos->getIsPlaying();
        }

    const double cyclesPerSample = (bpm / 60.0 / currentSampleRate) / juce::jmax (1.0e-9, beatsPerCycle);
    double phase = playing ? (ppq / beatsPerCycle) : freeRunPhase;

    auto* chans = buffer.getArrayOfWritePointers();
    float lastS = 0.5f, lastGain = 1.0f;

    for (int n = 0; n < N; ++n)
    {
        const float s     = engine.evalLfoUnipolar (phase + phaseOffset);   // 0..1
        const float base  = smBase.getNextValue();
        const float depth = smDepth.getNextValue();
        const float gain  = base * (1.0f - depth + depth * s);              // tremolo
        for (int ch = 0; ch < nOut; ++ch) chans[ch][n] *= gain;
        lastS = s; lastGain = gain;
        phase += cyclesPerSample;
    }

    if (!playing) freeRunPhase = phase - std::floor (phase);

    meterLfo.store (lastS);
    meterGain.store (lastGain);
}

//==============================================================================
juce::AudioProcessorEditor* PeakLFOAudioProcessor::createEditor()
{
    return new PeakLFOAudioProcessorEditor (*this);
}

void PeakLFOAudioProcessor::getStateInformation (juce::MemoryBlock& destData)
{
    if (auto xml = parameters.copyState().createXml())
        copyXmlToBinary (*xml, destData);
}

void PeakLFOAudioProcessor::setStateInformation (const void* data, int sizeInBytes)
{
    if (auto xml = getXmlFromBinary (data, sizeInBytes))
        if (xml->hasTagName (parameters.state.getType()))
            parameters.replaceState (juce::ValueTree::fromXml (*xml));
}

//==============================================================================
juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter()
{
    return new PeakLFOAudioProcessor();
}
