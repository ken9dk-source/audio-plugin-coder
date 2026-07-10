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
    layout.add (std::make_unique<AudioParameterBool>(
        ParameterID { ParameterIDs::mod_sync, 1 }, "Sync", false));
    const StringArray modPeriods { "1/32T","1/32","1/16T","1/16","1/8T","1/8","1/4T","1/4",
        "2b","3b","4b","5b","6b","8b","12b","16b","24b","32b","48b","64b","96b","128b","192b","256b" };
    layout.add (std::make_unique<AudioParameterChoice> (
        ParameterID { ParameterIDs::mod_period, 1 }, "Period", modPeriods, 10));   // default 4 beats
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

    // Rate: free 0.1..20 Hz, or tempo-synced (host BPM ÷ note division) — one full pan cycle per Period.
    static constexpr double periodBeats[24] = { 1.0/12, 1.0/8, 1.0/6, 1.0/4, 1.0/3, 1.0/2, 2.0/3, 1.0,
        2.0, 3.0, 4.0, 5.0, 6.0, 8.0, 12.0, 16.0, 24.0, 32.0, 48.0, 64.0, 96.0, 128.0, 192.0, 256.0 };
    double lfoInc;
    if (apvts.getRawParameterValue (ParameterIDs::mod_sync)->load() > 0.5f)
    {
        double bpm = 120.0;
        if (auto* ph = getPlayHead())
            if (auto pos = ph->getPosition())
                if (auto b = pos->getBpm()) bpm = *b > 1.0 ? *b : 120.0;
        const int p = juce::jlimit (0, 23, (int) apvts.getRawParameterValue (ParameterIDs::mod_period)->load());
        lfoInc = ((bpm / 60.0) / periodBeats[p]) / sr;
    }
    else
        lfoInc = (0.1 + (double) fRate * (double) fRate * 19.9) / sr;   // free: 0.1 .. 20 Hz
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
        // VAZ pan-law is LINEAR, not equal-power: pan-table[i]=(i/255)·2^30 (autopan ctor FUN_00517ae4 @0x517b1c,
        // t·0.5·(2^31-1), NO cos/sin), render gainR=table[idx], gainL=table[255−idx] → gainR=pan, gainL=1−pan.
        const double gL = 1.0 - pan01, gR = pan01;

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
