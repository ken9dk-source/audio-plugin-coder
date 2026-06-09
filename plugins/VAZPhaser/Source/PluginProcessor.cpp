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
    layout.add (std::make_unique<AudioParameterBool> (ParameterID { ParameterIDs::mod_sync, 1 }, "Sync", false));
    const StringArray modPeriods { "1/32T","1/32","1/16T","1/16","1/8T","1/8","1/4T","1/4",
        "2b","3b","4b","5b","6b","8b","12b","16b","24b","32b","48b","64b","96b","128b","192b","256b" };
    layout.add (std::make_unique<AudioParameterChoice> (ParameterID { ParameterIDs::mod_period, 1 }, "Period", modPeriods, 10)); // default 4 beats
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
    // RE'd from Core.dll TFXPhaser @0x5218D8: the LFO is a UNIPOLAR triangle (abs of the phase
    // accumulator) that sweeps the all-pass freq UP from a base (idx = base + |phase|·depth @0x52190A).
    const double fcBase   = 80.0 * std::pow (25.0, (double) fFreq);    // sweep FLOOR: 80 Hz..2 kHz
    const double depthOct = (double) fDepth * 2.5;                     // triangle sweep: 0..+2.5 octaves above the floor
    const double feedback = (double) fFb * 0.72;                       // resonance/intensity (kept musical → no screech)
    const double fbSign   = fbPhase > 0.5f ? -1.0 : 1.0;              // Feedback Phase: − = inverted polarity
    const double mix      = (double) fMix;
    const double gain     = (double) fGain;
    // Modulation rate: free (Rate² → 0..20 Hz, Rate fully DOWN = STATIC notch — the VAZ hardtrance signature)
    // or tempo-synced (Sync on → host-BPM division from the 24-entry note table).
    static constexpr double periodBeats[24] = { 1.0/12, 1.0/8, 1.0/6, 1.0/4, 1.0/3, 1.0/2, 2.0/3, 1.0,
        2.0, 3.0, 4.0, 5.0, 6.0, 8.0, 12.0, 16.0, 24.0, 32.0, 48.0, 64.0, 96.0, 128.0, 192.0, 256.0 };
    double rateHz;
    if (apvts.getRawParameterValue (ParameterIDs::mod_sync)->load() > 0.5f)
    {
        double bpm = 120.0;
        if (auto* ph = getPlayHead())
            if (auto pos = ph->getPosition())
                if (auto b = pos->getBpm()) bpm = *b;
        const int p = juce::jlimit (0, 23, (int) apvts.getRawParameterValue (ParameterIDs::mod_period)->load());
        rateHz = (bpm / 60.0) / periodBeats[p];               // tempo-synced Hz
    }
    else
        rateHz = (double) fRate * (double) fRate * 20.0;      // free: 0..20 Hz
    const double lfoInc   = rateHz / sr;
    if (lfoInc <= 0.0) lfoPhase = 0.0;            // static → freeze LFO at neutral so the Frequency knob alone sets the notch position
    const double nyq      = 0.45 * sr;
    const double lrOffset = (double) fLrPh * 0.5;          // up to half-cycle between channels

    const int n = buffer.getNumSamples();
    const int numCh = buffer.getNumChannels();
    float* L = buffer.getWritePointer (0);
    float* R = numCh > 1 ? buffer.getWritePointer (1) : nullptr;

    auto triangle = [] (double ph) noexcept                // VAZ LFO = abs(phase) = triangle (real @0x521902), 0→1→0
    { return ph < 0.5 ? 2.0 * ph : 2.0 - 2.0 * ph; };

    for (int i = 0; i < n; ++i)
    {
        const double lfoL = triangle (lfoPhase);           // unipolar 0..1 (was bipolar sine)
        const double fcL  = juce::jlimit (20.0, nyq, fcBase * std::pow (2.0, depthOct * lfoL));
        const double tL   = std::tan (juce::MathConstants<double>::pi * fcL / sr);
        const double aL   = juce::jlimit (-0.98, 0.98, (1.0 - tL) / (1.0 + tL));   // 1st-order all-pass coeff (positive → notch in the audible band)
        L[i] = (float) chL.process ((double) L[i], stages, aL, feedback, fbSign, mix, gain);

        if (R != nullptr)
        {
            double pr = lfoPhase + lrOffset; if (pr >= 1.0) pr -= 1.0;
            const double lfoR = triangle (pr);             // unipolar triangle (L/R phase offset)
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
