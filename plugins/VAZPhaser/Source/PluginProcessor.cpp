#include "PluginProcessor.h"
#include "PluginEditor.h"
#include "ParameterIDs.hpp"

VAZPhaserAudioProcessor::VAZPhaserAudioProcessor()
    : AudioProcessor (BusesProperties()
          .withInput  ("Input",  juce::AudioChannelSet::stereo(), true)
          .withOutput ("Output", juce::AudioChannelSet::stereo(), true)),
      apvts (*this, nullptr, "PARAMETERS", createParameterLayout())
{
}

VAZPhaserAudioProcessor::~VAZPhaserAudioProcessor() {}

juce::AudioProcessorValueTreeState::ParameterLayout VAZPhaserAudioProcessor::createParameterLayout()
{
    using namespace juce;
    AudioProcessorValueTreeState::ParameterLayout layout;
    auto pct = [] (const char* id, const char* name, float def)
    {
        return std::make_unique<AudioParameterFloat>(
            ParameterID { id, 1 }, name, NormalisableRange<float>(0.0f, 1.0f), def);
    };
    layout.add (pct (ParameterIDs::stages,    "Stages",    0.2f));   // → 4 stages
    layout.add (pct (ParameterIDs::frequency, "Frequency", 0.5f));
    layout.add (pct (ParameterIDs::feedback,  "Feedback",  0.5f));
    layout.add (pct (ParameterIDs::rate,      "Rate",      0.3f));
    layout.add (pct (ParameterIDs::depth,     "Depth",     0.6f));
    layout.add (pct (ParameterIDs::lr_phase,  "L/R Phase", 0.25f));
    layout.add (pct (ParameterIDs::mix,       "Mix",       0.5f));
    layout.add (pct (ParameterIDs::gain,      "Gain",      0.70f));   // ≈ −3 dB (VAZ default)
    layout.add (std::make_unique<AudioParameterBool> (ParameterID { ParameterIDs::feedback_phase, 1 }, "Feedback Phase", false));
    return layout;
}

void VAZPhaserAudioProcessor::prepareToPlay (double sampleRate, int)
{
    sr = sampleRate;
    chL.reset(); chR.reset();
    lfoPhase = 0.0;
}

bool VAZPhaserAudioProcessor::isBusesLayoutSupported (const BusesLayout& layouts) const
{
    const auto& in  = layouts.getMainInputChannelSet();
    const auto& out = layouts.getMainOutputChannelSet();
    if (out != juce::AudioChannelSet::stereo() && out != juce::AudioChannelSet::mono())
        return false;
    return in == out;
}

void VAZPhaserAudioProcessor::processBlock (juce::AudioBuffer<float>& buffer, juce::MidiBuffer&)
{
    juce::ScopedNoDenormals noDenormals;

    const float fStages = apvts.getRawParameterValue (ParameterIDs::stages)->load();
    const float fFreq   = apvts.getRawParameterValue (ParameterIDs::frequency)->load();
    const float fFb     = apvts.getRawParameterValue (ParameterIDs::feedback)->load();
    const float fRate   = apvts.getRawParameterValue (ParameterIDs::rate)->load();
    const float fDepth  = apvts.getRawParameterValue (ParameterIDs::depth)->load();
    const float fLrPh   = apvts.getRawParameterValue (ParameterIDs::lr_phase)->load();
    const float fMix    = apvts.getRawParameterValue (ParameterIDs::mix)->load();
    const float fGain   = apvts.getRawParameterValue (ParameterIDs::gain)->load();
    const float fbPhase = apvts.getRawParameterValue (ParameterIDs::feedback_phase)->load();

    const int    stages   = juce::jlimit (2, 12, 2 + 2 * (int) std::lround (fStages * 5.0f));
    const double fcBase   = 200.0 * std::pow (10.0, (double) fFreq);   // all-pass break freq 200 Hz..2 kHz (log) — classic phaser band
    const double depthOct = (double) fDepth * 1.5;                     // LFO sweep ±1.5 octaves
    const double feedback = (double) fFb * 0.72;                       // resonance/intensity (kept musical → no screech)
    const double fbSign   = fbPhase > 0.5f ? -1.0 : 1.0;              // Feedback Phase: − = inverted polarity
    const double mix      = (double) fMix;
    const double gain     = (double) fGain;
    const double lfoInc   = (double) fRate * (double) fRate * 8.0 / sr;   // Rate fully DOWN → 0 Hz = STATIC notch (the VAZ signature, e.g. hardtrance); up to ~8 Hz
    if (lfoInc <= 0.0) lfoPhase = 0.0;            // static → freeze LFO at neutral so the Frequency knob alone sets the notch position
    const double nyq      = 0.45 * sr;
    const double lrOffset = (double) fLrPh * 0.5;          // up to half-cycle between channels

    const int n = buffer.getNumSamples();
    const int numCh = buffer.getNumChannels();
    float* L = buffer.getWritePointer (0);
    float* R = numCh > 1 ? buffer.getWritePointer (1) : nullptr;

    for (int i = 0; i < n; ++i)
    {
        const double lfoL = std::sin (2.0 * juce::MathConstants<double>::pi * lfoPhase);
        const double fcL  = juce::jlimit (20.0, nyq, fcBase * std::pow (2.0, depthOct * lfoL));
        const double tL   = std::tan (juce::MathConstants<double>::pi * fcL / sr);
        const double aL   = juce::jlimit (-0.98, 0.98, (1.0 - tL) / (1.0 + tL));   // 1st-order all-pass coeff (positive → notch in the audible band)
        L[i] = (float) chL.process ((double) L[i], stages, aL, feedback, fbSign, mix, gain);

        if (R != nullptr)
        {
            double pr = lfoPhase + lrOffset; if (pr >= 1.0) pr -= 1.0;
            const double lfoR = std::sin (2.0 * juce::MathConstants<double>::pi * pr);
            const double fcR  = juce::jlimit (20.0, nyq, fcBase * std::pow (2.0, depthOct * lfoR));
            const double tR   = std::tan (juce::MathConstants<double>::pi * fcR / sr);
            const double aR   = juce::jlimit (-0.98, 0.98, (1.0 - tR) / (1.0 + tR));
            R[i] = (float) chR.process ((double) R[i], stages, aR, feedback, fbSign, mix, gain);
        }

        lfoPhase += lfoInc; if (lfoPhase >= 1.0) lfoPhase -= 1.0;
    }
}

void VAZPhaserAudioProcessor::getStateInformation (juce::MemoryBlock& dest)
{
    if (auto state = apvts.copyState(); state.isValid())
        if (auto xml = state.createXml())
            copyXmlToBinary (*xml, dest);
}

void VAZPhaserAudioProcessor::setStateInformation (const void* data, int size)
{
    if (auto xml = getXmlFromBinary (data, size))
        apvts.replaceState (juce::ValueTree::fromXml (*xml));
}

juce::AudioProcessorEditor* VAZPhaserAudioProcessor::createEditor()
{
    return new VAZPhaserAudioProcessorEditor (*this);
}

juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter()
{
    return new VAZPhaserAudioProcessor();
}
