#include "PluginProcessor.h"
#include "PluginEditor.h"
#include "ParameterIDs.hpp"
#include <cmath>

VAZCompressorAudioProcessor::VAZCompressorAudioProcessor()
    : AudioProcessor (BusesProperties()
          .withInput  ("Input",  juce::AudioChannelSet::stereo(), true)
          .withOutput ("Output", juce::AudioChannelSet::stereo(), true)),
      apvts (*this, nullptr, "PARAMETERS", createParameterLayout())
{
}

VAZCompressorAudioProcessor::~VAZCompressorAudioProcessor() {}

juce::AudioProcessorValueTreeState::ParameterLayout VAZCompressorAudioProcessor::createParameterLayout()
{
    using namespace juce;
    AudioProcessorValueTreeState::ParameterLayout layout;
    auto p = [] (const char* id, const char* name, float def)
    {
        return std::make_unique<AudioParameterFloat>(
            ParameterID { id, 1 }, name, NormalisableRange<float>(0.0f, 1.0f), def);
    };
    // Defaults (normalised) match the VAZ Compressor GIF readouts.
    layout.add (p (ParameterIDs::threshold, "Threshold", 0.9242f));   // +1.0 dB (near top of -60..+6)
    layout.add (p (ParameterIDs::ratio,     "Ratio",     0.5f));      // slope 0.5 → 2:1
    layout.add (p (ParameterIDs::attack,    "Attack",    0.5518f));   // 5 ms  (0.1..120 ms log)
    layout.add (p (ParameterIDs::release,   "Release",   0.3248f));   // 50 ms (5 ms..6 s log)
    layout.add (p (ParameterIDs::makeup,    "Makeup",    0.0f));      // 0 dB  (0..+24 dB)
    return layout;
}

void VAZCompressorAudioProcessor::prepareToPlay (double sampleRate, int)
{
    curSR = sampleRate > 0.0 ? sampleRate : 44100.0;
    envGr = 0.0f;
    grMeter.store (0.0f);
    clip.store (false);
}

bool VAZCompressorAudioProcessor::isBusesLayoutSupported (const BusesLayout& layouts) const
{
    const auto& in  = layouts.getMainInputChannelSet();
    const auto& out = layouts.getMainOutputChannelSet();
    if (out != juce::AudioChannelSet::stereo() && out != juce::AudioChannelSet::mono())
        return false;
    return in == out;
}

void VAZCompressorAudioProcessor::processBlock (juce::AudioBuffer<float>& buffer, juce::MidiBuffer&)
{
    juce::ScopedNoDenormals noDenormals;
    auto get = [this] (const char* id) { return apvts.getRawParameterValue (id)->load(); };

    // ── map params to VAZ ranges ──
    const float thrDb    = -60.0f + get (ParameterIDs::threshold) * 66.0f;          // -60..+6 dB
    const float slope    = juce::jlimit (0.0f, 1.0f, get (ParameterIDs::ratio));    // 1-1/ratio (0=1:1 .. 1=inf)
    const float atkMs    = 0.1f * std::pow (1200.0f, get (ParameterIDs::attack));   // 0.1..120 ms
    const float relMs    = 5.0f * std::pow (1200.0f, get (ParameterIDs::release));  // 5..6000 ms
    const float makeupDb = get (ParameterIDs::makeup) * 24.0f;                      // 0..+24 dB

    const float atkCoef = std::exp (-1.0f / (atkMs * 0.001f * (float) curSR));      // one-pole coefs
    const float relCoef = std::exp (-1.0f / (relMs * 0.001f * (float) curSR));

    const int nCh = juce::jmin (buffer.getNumChannels(), 2);
    const int n   = buffer.getNumSamples();
    if (nCh <= 0) return;

    float* chans[2] = { buffer.getWritePointer (0), nCh > 1 ? buffer.getWritePointer (1) : nullptr };

    float blockPeakGr = 0.0f;
    bool  blockClip   = false;

    for (int i = 0; i < n; ++i)
    {
        // stereo-linked peak detector
        float detect = 0.0f;
        for (int ch = 0; ch < nCh; ++ch)
            detect = juce::jmax (detect, std::abs (chans[ch][i]));

        const float levelDb = 20.0f * std::log10 (juce::jmax (detect, 1.0e-7f));
        const float over    = levelDb - thrDb;
        const float targetGr = over > 0.0f ? over * slope : 0.0f;       // desired gain reduction (dB)

        // attack when pulling down harder, release when recovering
        const float coef = (targetGr > envGr) ? atkCoef : relCoef;
        envGr = coef * envGr + (1.0f - coef) * targetGr;
        if (envGr < 0.0f) envGr = 0.0f;

        const float g = std::pow (10.0f, (makeupDb - envGr) / 20.0f);   // gain = 10^((Makeup - GR)/20)
        for (int ch = 0; ch < nCh; ++ch)
        {
            const float o = chans[ch][i] * g;
            chans[ch][i] = o;
            if (std::abs (o) > 1.0f) blockClip = true;
        }
        if (envGr > blockPeakGr) blockPeakGr = envGr;
    }

    grMeter.store (blockPeakGr);
    if (blockClip) clip.store (true);
}

void VAZCompressorAudioProcessor::getStateInformation (juce::MemoryBlock& dest)
{
    if (auto state = apvts.copyState(); state.isValid())
        if (auto xml = state.createXml())
            copyXmlToBinary (*xml, dest);
}

void VAZCompressorAudioProcessor::setStateInformation (const void* data, int size)
{
    if (auto xml = getXmlFromBinary (data, size))
        apvts.replaceState (juce::ValueTree::fromXml (*xml));
}

juce::AudioProcessorEditor* VAZCompressorAudioProcessor::createEditor()
{
    return new VAZCompressorAudioProcessorEditor (*this);
}

//==============================================================================
juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter()
{
    return new VAZCompressorAudioProcessor();
}
