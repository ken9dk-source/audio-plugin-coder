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
    reverb.setSampleRate (sampleRate);
    reverb.reset();
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

    juce::Reverb::Parameters p;
    p.roomSize   = 0.40f + rt * 0.58f;     // Reverb Time → decay (~0.5s..2.5s)
    p.damping    = 1.0f - tone;            // Tone: 0 Dark (max damping) … 1 Bright (none)
    p.wetLevel   = mix;                    // Mix: Dry … Wet
    p.dryLevel   = 1.0f - mix;
    p.width      = 1.0f;
    p.freezeMode = 0.0f;
    reverb.setParameters (p);

    const int n = buffer.getNumSamples();
    if (buffer.getNumChannels() >= 2)
        reverb.processStereo (buffer.getWritePointer (0), buffer.getWritePointer (1), n);
    else if (buffer.getNumChannels() == 1)
        reverb.processMono (buffer.getWritePointer (0), n);
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
