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
    sr = sampleRate > 0.0 ? sampleRate : 44100.0;
    engine.clearBuffers();
    engine.setSampleRate (sr);   // build the SR-adjusted coef LUT
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
    const bool  fbPhase = apvts.getRawParameterValue (ParameterIDs::feedback_phase)->load() > 0.5f;

    // Map the knobs onto VAZ's TFXPhaser fields (setters FUN_00521b68/bf4/d14/b80/d24; render @0x5218d8):
    const int stagesParam = juce::jlimit (0, 5,   (int) std::lround (fStages * 5.0f));  // N=(p+1)·2 → 2..12 (FUN_00521b68)
    const int fbParam     = juce::jlimit (0, 100, (int) std::lround (fFb   * 100.0f));  // feedback 0..100 → 0..0.78125 (FUN_00521bf4)
    const int depthP      = juce::jlimit (0, 255, (int) std::lround (fDepth * 255.0f)); // LUT-index sweep (+0x280)
    const int centerP     = juce::jlimit (0, 255, (int) std::lround (fFreq  * 255.0f)); // base LUT index / notch freq (+0x264)
    const int lrP         = juce::jlimit (0, 255, (int) std::lround (fLrPh  * 255.0f)); // L/R phase offset (+0x284)
    const int mixP        = juce::jlimit (0, 255, (int) std::lround (fMix   * 255.0f)); // Dry..Wet (+0x288)

    // LFO rate → 32-bit phase increment. ⚠ APPROX (rate/inc is VAZ 80-bit x87, not isolable-dumpable — accepted).
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
        rateHz = (bpm / 60.0) / periodBeats[p];
    }
    else
        rateHz = (double) fRate * (double) fRate * 20.0;      // free: 0..20 Hz
    const uint32_t inc = (uint32_t) (int64_t) (rateHz / sr * 4294967296.0);
    engine.setParams (stagesParam, fbParam, fbPhase, depthP, centerP, lrP, mixP, inc);
    const double outGain = (double) fGain;

    constexpr double kFS = 8388608.0;   // Q23 full-scale
    const int n = buffer.getNumSamples();
    const int numCh = buffer.getNumChannels();
    float* L = buffer.getWritePointer (0);
    float* R = numCh > 1 ? buffer.getWritePointer (1) : nullptr;
    for (int i = 0; i < n; ++i)
    {
        int32_t li = (int32_t) std::llround ((double) L[i] * kFS);
        int32_t ri = R ? (int32_t) std::llround ((double) R[i] * kFS) : li;
        engine.processFrame (li, ri);
        L[i] = (float) ((double) li / kFS * outGain);
        if (R) R[i] = (float) ((double) ri / kFS * outGain);
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
