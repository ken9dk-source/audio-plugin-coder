#include "PluginProcessor.h"
#include "PluginEditor.h"
#include "ParameterIDs.hpp"

VAZAutopanAudioProcessor::VAZAutopanAudioProcessor()
    : AudioProcessor (BusesProperties()
          .withInput  ("Input",  juce::AudioChannelSet::stereo(), true)
          .withOutput ("Output", juce::AudioChannelSet::stereo(), true)),
      apvts (*this, nullptr, "PARAMETERS", createParameterLayout())
{
}

VAZAutopanAudioProcessor::~VAZAutopanAudioProcessor() {}

juce::AudioProcessorValueTreeState::ParameterLayout VAZAutopanAudioProcessor::createParameterLayout()
{
    using namespace juce;
    AudioProcessorValueTreeState::ParameterLayout layout;
    auto pct = [] (const char* id, const char* name, float def)
    {
        return std::make_unique<AudioParameterFloat>(
            ParameterID { id, 1 }, name, NormalisableRange<float>(0.0f, 1.0f), def);
    };
    layout.add (pct (ParameterIDs::left_limit,  "Left Limit",  0.0f));   // hard left
    layout.add (pct (ParameterIDs::right_limit, "Right Limit", 1.0f));   // hard right
    layout.add (pct (ParameterIDs::rate,        "Rate",        0.3f));   // ~moderate
    layout.add (std::make_unique<AudioParameterBool>(
        ParameterID { ParameterIDs::waveform_sine, 1 }, "Waveform Sine", false));   // Triangle by default
    return layout;
}

void VAZAutopanAudioProcessor::prepareToPlay (double sampleRate, int)
{
    sr = sampleRate > 0.0 ? sampleRate : 44100.0;
    lfoPhase = 0.0;
}

bool VAZAutopanAudioProcessor::isBusesLayoutSupported (const BusesLayout& layouts) const
{
    const auto& in  = layouts.getMainInputChannelSet();
    const auto& out = layouts.getMainOutputChannelSet();
    if (out != juce::AudioChannelSet::stereo() && out != juce::AudioChannelSet::mono())
        return false;
    return in == out;
}

void VAZAutopanAudioProcessor::processBlock (juce::AudioBuffer<float>& buffer, juce::MidiBuffer&)
{
    juce::ScopedNoDenormals noDenormals;

    const double leftP  = (double) apvts.getRawParameterValue (ParameterIDs::left_limit)->load();
    const double rightP = (double) apvts.getRawParameterValue (ParameterIDs::right_limit)->load();
    const float  fRate  = apvts.getRawParameterValue (ParameterIDs::rate)->load();
    const bool   sine   = apvts.getRawParameterValue (ParameterIDs::waveform_sine)->load() > 0.5f;

    const double lfoInc = (0.1 + (double) fRate * (double) fRate * 19.9) / sr;   // 0.1 .. 20 Hz
    const double halfPi = juce::MathConstants<double>::halfPi;
    const double twoPi  = juce::MathConstants<double>::twoPi;

    const int   n     = buffer.getNumSamples();
    const int   numCh = buffer.getNumChannels();
    float* L = buffer.getWritePointer (0);
    float* R = numCh > 1 ? buffer.getWritePointer (1) : nullptr;

    for (int i = 0; i < n; ++i)
    {
        // LFO → 0..1 sweep: Triangle (constant speed) or Sine (slow at the limits)
        const double lfo01 = sine ? (0.5 - 0.5 * std::cos (twoPi * lfoPhase))
                                  : (1.0 - std::abs (2.0 * lfoPhase - 1.0));
        const double pan01 = juce::jlimit (0.0, 1.0, leftP + lfo01 * (rightP - leftP));
        const double ang   = pan01 * halfPi;                 // equal-power pan
        const double gL = std::cos (ang), gR = std::sin (ang);

        const double inL = (double) L[i];
        const double inR = R != nullptr ? (double) R[i] : inL;
        L[i] = (float) (inL * gL);
        if (R != nullptr) R[i] = (float) (inR * gR);

        lfoPhase += lfoInc; if (lfoPhase >= 1.0) lfoPhase -= 1.0;
    }
}

void VAZAutopanAudioProcessor::getStateInformation (juce::MemoryBlock& dest)
{
    if (auto state = apvts.copyState(); state.isValid())
        if (auto xml = state.createXml())
            copyXmlToBinary (*xml, dest);
}

void VAZAutopanAudioProcessor::setStateInformation (const void* data, int size)
{
    if (auto xml = getXmlFromBinary (data, size))
        apvts.replaceState (juce::ValueTree::fromXml (*xml));
}

juce::AudioProcessorEditor* VAZAutopanAudioProcessor::createEditor()
{
    return new VAZAutopanAudioProcessorEditor (*this);
}

//==============================================================================
juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter()
{
    return new VAZAutopanAudioProcessor();
}
