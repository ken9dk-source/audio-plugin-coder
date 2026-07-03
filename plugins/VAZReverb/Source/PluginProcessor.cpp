#include "PluginProcessor.h"
#include "PluginEditor.h"
#include "ParameterIDs.hpp"

VAZReverbAudioProcessor::VAZReverbAudioProcessor()
    : AudioProcessor (BusesProperties()
          .withInput  ("Input",  juce::AudioChannelSet::stereo(), true)
          .withOutput ("Output", juce::AudioChannelSet::stereo(), true)),
      apvts (*this, nullptr, "PARAMETERS", createParameterLayout())
{
}

VAZReverbAudioProcessor::~VAZReverbAudioProcessor() {}

juce::AudioProcessorValueTreeState::ParameterLayout VAZReverbAudioProcessor::createParameterLayout()
{
    using namespace juce;
    AudioProcessorValueTreeState::ParameterLayout layout;
    auto pct = [] (const char* id, const char* name, float def)
    {
        return std::make_unique<AudioParameterFloat>(
            ParameterID { id, 1 }, name, NormalisableRange<float>(0.0f, 1.0f), def);
    };
    layout.add (pct (ParameterIDs::reverb_time, "Reverb Time", 0.6f));
    layout.add (pct (ParameterIDs::tone,        "Tone",        0.6f));
    layout.add (pct (ParameterIDs::mix,         "Mix",         0.3f));
    return layout;
}

void VAZReverbAudioProcessor::prepareToPlay (double sampleRate, int)
{
    sr = sampleRate > 0.0 ? sampleRate : 44100.0;
    engine.clearBuffers();
    engine.setLengths (sr);
}

bool VAZReverbAudioProcessor::isBusesLayoutSupported (const BusesLayout& layouts) const
{
    const auto& in  = layouts.getMainInputChannelSet();
    const auto& out = layouts.getMainOutputChannelSet();
    if (out != juce::AudioChannelSet::stereo() && out != juce::AudioChannelSet::mono())
        return false;
    return in == out;
}

void VAZReverbAudioProcessor::processBlock (juce::AudioBuffer<float>& buffer, juce::MidiBuffer&)
{
    juce::ScopedNoDenormals noDenormals;

    const float rt   = apvts.getRawParameterValue (ParameterIDs::reverb_time)->load();
    const float tone = apvts.getRawParameterValue (ParameterIDs::tone)->load();
    const float mix  = apvts.getRawParameterValue (ParameterIDs::mix)->load();

    // Map the 3 clone knobs onto VAZ's .v2p reverb fields (size/damp/mix, 0..255):
    const int size = juce::jlimit (0, 255, (int) std::lround (rt * 255.0f));          // Reverb Time → size (+0x260)
    const int damp = juce::jlimit (0, 255, (int) std::lround ((1.0f - tone) * 255.0f)); // Tone: bright → less damping (+0x264)
    const int mixP = juce::jlimit (0, 255, (int) std::lround (mix * 255.0f));          // Mix (+0x268)
    engine.setParams (sr, size, damp, mixP);

    constexpr double kFS = 8388608.0;   // Q23 full-scale (VAZ clips samples at ±2^23, render @0x5228a4)
    const int n = buffer.getNumSamples();
    float* L = buffer.getWritePointer (0);
    float* R = buffer.getNumChannels() > 1 ? buffer.getWritePointer (1) : nullptr;
    for (int i = 0; i < n; ++i)
    {
        int32_t li = (int32_t) std::llround ((double) L[i] * kFS);
        int32_t ri = R ? (int32_t) std::llround ((double) R[i] * kFS) : li;
        engine.processFrame (li, ri);
        L[i] = (float) ((double) li / kFS);
        if (R) R[i] = (float) ((double) ri / kFS);
    }
}

void VAZReverbAudioProcessor::getStateInformation (juce::MemoryBlock& dest)
{
    if (auto state = apvts.copyState(); state.isValid())
        if (auto xml = state.createXml())
            copyXmlToBinary (*xml, dest);
}

void VAZReverbAudioProcessor::setStateInformation (const void* data, int size)
{
    if (auto xml = getXmlFromBinary (data, size))
        apvts.replaceState (juce::ValueTree::fromXml (*xml));
}

juce::AudioProcessorEditor* VAZReverbAudioProcessor::createEditor()
{
    return new VAZReverbAudioProcessorEditor (*this);
}

//==============================================================================
juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter()
{
    return new VAZReverbAudioProcessor();
}
