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
    held[0] = held[1] = 0.0f;
    accum[0] = accum[1] = 1.0;
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

    // Sample-rate reduction (sample & hold): effective SR = 100 Hz .. fs (log), step = effSR/fs (<=1)
    const double effSR = 100.0 * std::pow (curSR / 100.0, (double) srP);
    const double step  = juce::jlimit (0.0001, 1.0, effSR / curSR);
    // Bit reduction: 1..16 bits → quantise step q over full-scale [-1,1]
    const double bits   = juce::jmap ((double) bitP, 1.0, 16.0);
    const double levels = std::pow (2.0, bits);
    const double q      = 2.0 / juce::jmax (1.0, levels - 1.0);

    const int nCh = juce::jmin (buffer.getNumChannels(), 2);
    const int n   = buffer.getNumSamples();
    for (int ch = 0; ch < nCh; ++ch)
    {
        float* d = buffer.getWritePointer (ch);
        for (int i = 0; i < n; ++i)
        {
            accum[ch] += step;
            if (accum[ch] >= 1.0) { accum[ch] -= 1.0; held[ch] = d[i]; }   // latch a new input sample
            double y = std::round ((double) held[ch] / q) * q;             // quantise the held value
            d[i] = (float) juce::jlimit (-1.0, 1.0, y);
        }
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
