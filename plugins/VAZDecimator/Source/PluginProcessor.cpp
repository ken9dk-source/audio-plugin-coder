#include "PluginProcessor.h"
#include "PluginEditor.h"
#include "ParameterIDs.hpp"
#include <cmath>

VAZDecimatorAudioProcessor::VAZDecimatorAudioProcessor()
    : AudioProcessor (BusesProperties()
          .withInput  ("Input",  juce::AudioChannelSet::stereo(), true)
          .withOutput ("Output", juce::AudioChannelSet::stereo(), true)),
      apvts (*this, nullptr, "PARAMETERS", createParameterLayout())
{
}

VAZDecimatorAudioProcessor::~VAZDecimatorAudioProcessor() {}

juce::AudioProcessorValueTreeState::ParameterLayout VAZDecimatorAudioProcessor::createParameterLayout()
{
    using namespace juce;
    AudioProcessorValueTreeState::ParameterLayout layout;
    auto pct = [] (const char* id, const char* name, float def)
    {
        return std::make_unique<AudioParameterFloat>(
            ParameterID { id, 1 }, name, NormalisableRange<float>(0.0f, 1.0f), def);
    };
    layout.add (pct (ParameterIDs::sample_rate, "Sample Rate", 1.0f));   // 1 = full SR (transparent)
    layout.add (pct (ParameterIDs::bit_depth,   "Bit Depth",   1.0f));   // 1 = 16-bit (transparent)
    return layout;
}

void VAZDecimatorAudioProcessor::prepareToPlay (double sampleRate, int)
{
    curSR = sampleRate > 0.0 ? sampleRate : 44100.0;
    engine.reset();
}

bool VAZDecimatorAudioProcessor::isBusesLayoutSupported (const BusesLayout& layouts) const
{
    const auto& in  = layouts.getMainInputChannelSet();
    const auto& out = layouts.getMainOutputChannelSet();
    if (out != juce::AudioChannelSet::stereo() && out != juce::AudioChannelSet::mono())
        return false;
    return in == out;
}

void VAZDecimatorAudioProcessor::processBlock (juce::AudioBuffer<float>& buffer, juce::MidiBuffer&)
{
    juce::ScopedNoDenormals noDenormals;

    const float srP  = apvts.getRawParameterValue (ParameterIDs::sample_rate)->load();
    const float bitP = apvts.getRawParameterValue (ParameterIDs::bit_depth)->load();

    // Map the two knobs onto VAZ's .v2p decimator fields:
    const int srParam = juce::jlimit (0, 255, (int) std::lround (srP * 255.0f));   // SR-reduction (+0x260) → rate
    const int bits    = juce::jlimit (1, 16,  (int) std::lround (1.0f + bitP * 15.0f)); // bit depth (+0x264), 1..16 (VAZ dflt 16)
    engine.setParams (curSR, srParam, bits);

    constexpr double kFS = 8388608.0;   // Q23 full-scale (VAZ samples are ±2^23)
    const int nCh = juce::jmin (buffer.getNumChannels(), 2);
    const int n   = buffer.getNumSamples();
    float* L = buffer.getWritePointer (0);
    float* R = nCh > 1 ? buffer.getWritePointer (1) : nullptr;
    for (int i = 0; i < n; ++i)
    {
        int32_t li = (int32_t) std::llround ((double) L[i] * kFS);
        int32_t ri = R ? (int32_t) std::llround ((double) R[i] * kFS) : li;
        engine.processFrame (li, ri);
        L[i] = (float) ((double) li / kFS);
        if (R) R[i] = (float) ((double) ri / kFS);
    }
}

void VAZDecimatorAudioProcessor::getStateInformation (juce::MemoryBlock& dest)
{
    if (auto state = apvts.copyState(); state.isValid())
        if (auto xml = state.createXml())
            copyXmlToBinary (*xml, dest);
}

void VAZDecimatorAudioProcessor::setStateInformation (const void* data, int size)
{
    if (auto xml = getXmlFromBinary (data, size))
        apvts.replaceState (juce::ValueTree::fromXml (*xml));
}

juce::AudioProcessorEditor* VAZDecimatorAudioProcessor::createEditor()
{
    return new VAZDecimatorAudioProcessorEditor (*this);
}

//==============================================================================
juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter()
{
    return new VAZDecimatorAudioProcessor();
}
