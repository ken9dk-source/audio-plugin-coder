#include "PluginProcessor.h"
#include "PluginEditor.h"
#include "ParameterIDs.hpp"

VAZChorusAudioProcessor::VAZChorusAudioProcessor()
    : AudioProcessor (BusesProperties()
          .withInput  ("Input",  juce::AudioChannelSet::stereo(), true)
          .withOutput ("Output", juce::AudioChannelSet::stereo(), true)),
      apvts (*this, nullptr, "PARAMETERS", createParameterLayout())
{
}

VAZChorusAudioProcessor::~VAZChorusAudioProcessor() {}

juce::AudioProcessorValueTreeState::ParameterLayout VAZChorusAudioProcessor::createParameterLayout()
{
    using namespace juce;
    AudioProcessorValueTreeState::ParameterLayout layout;
    auto pct = [] (const char* id, const char* name, float def)
    {
        return std::make_unique<AudioParameterFloat>(
            ParameterID { id, 1 }, name, NormalisableRange<float>(0.0f, 1.0f), def);
    };
    layout.add (pct (ParameterIDs::delay,     "Delay",     0.4f));    // base delay 5..30 ms
    layout.add (pct (ParameterIDs::rate,      "Rate",      0.3f));
    layout.add (pct (ParameterIDs::depth,     "Depth",     0.5f));
    layout.add (pct (ParameterIDs::lr_phase,  "L/R Phase", 0.3f));
    layout.add (pct (ParameterIDs::mix,       "Mix",       0.5f));
    layout.add (pct (ParameterIDs::gain,      "Gain",      0.85f));   // ≈ −1.4 dB
    layout.add (std::make_unique<AudioParameterChoice> (ParameterID { ParameterIDs::waveform, 1 }, "Waveform",
        juce::StringArray { "Sine", "Triangle", "Trapezoid" }, 0));
    layout.add (std::make_unique<AudioParameterBool> (ParameterID { ParameterIDs::mod_sync, 1 }, "Sync", false));
    const StringArray modPeriods { "1/32T","1/32","1/16T","1/16","1/8T","1/8","1/4T","1/4",
        "2b","3b","4b","5b","6b","8b","12b","16b","24b","32b","48b","64b","96b","128b","192b","256b" };
    layout.add (std::make_unique<AudioParameterChoice> (ParameterID { ParameterIDs::mod_period, 1 }, "Period", modPeriods, 10)); // default 4 beats
    return layout;
}

void VAZChorusAudioProcessor::prepareToPlay (double sampleRate, int)
{
    sr = sampleRate;
    chL.prepare (sampleRate); chR.prepare (sampleRate);
    lfo1Phase = 0.0; lfo2Phase = 0.5;          // start the two LFOs half a cycle apart → immediate ensemble
}

bool VAZChorusAudioProcessor::isBusesLayoutSupported (const BusesLayout& layouts) const
{
    const auto& in  = layouts.getMainInputChannelSet();
    const auto& out = layouts.getMainOutputChannelSet();
    if (out != juce::AudioChannelSet::stereo() && out != juce::AudioChannelSet::mono())
        return false;
    return in == out;
}

void VAZChorusAudioProcessor::processBlock (juce::AudioBuffer<float>& buffer, juce::MidiBuffer&)
{
    juce::ScopedNoDenormals noDenormals;

    const float fDelay  = apvts.getRawParameterValue (ParameterIDs::delay)->load();
    const float fRate   = apvts.getRawParameterValue (ParameterIDs::rate)->load();
    const float fDepth  = apvts.getRawParameterValue (ParameterIDs::depth)->load();
    const float fLrPh   = apvts.getRawParameterValue (ParameterIDs::lr_phase)->load();
    const float fMix    = apvts.getRawParameterValue (ParameterIDs::mix)->load();
    const float fGain   = apvts.getRawParameterValue (ParameterIDs::gain)->load();
    const int   waveform = (int) apvts.getRawParameterValue (ParameterIDs::waveform)->load();

    // Real Chorus (RE'd from Core.dll TFXChorus @0x518AD8): dual-LFO 6-tap ensemble.
    // Delay = base centre delay; each LFO sweeps 'depth' samples; 3 taps at 0°/±120° per LFO.
    const double baseMs    = 5.0 + (double) fDelay * 25.0;          // centre delay 5..30 ms
    const double depthMs   = (double) fDepth * 8.0;                 // modulation depth 0..8 ms
    const double baseSamp  = baseMs  * 0.001 * sr;
    const double depthSamp = depthMs * 0.001 * sr;
    const double mix       = (double) fMix;
    const double gain      = (double) fGain;
    // Modulation rate: free (Rate²·6 → 0..6 Hz, chorus is slow) or tempo-synced (host BPM).
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
        rateHz = (double) fRate * (double) fRate * 6.0;       // free: 0..6 Hz
    const double lfo1Inc  = rateHz / sr;
    const double lfo2Inc  = lfo1Inc * 1.27;                // 2nd LFO detuned → ensemble beating (real has 2 LFOs)
    const double lrOffset = (double) fLrPh * 0.5;          // up to half-cycle between channels

    const int n = buffer.getNumSamples();
    const int numCh = buffer.getNumChannels();
    float* L = buffer.getWritePointer (0);
    float* R = numCh > 1 ? buffer.getWritePointer (1) : nullptr;

    for (int i = 0; i < n; ++i)
    {
        L[i] = (float) chL.process ((double) L[i], baseSamp, depthSamp, lfo1Phase, lfo2Phase, waveform, mix, gain);

        if (R != nullptr)
            R[i] = (float) chR.process ((double) R[i], baseSamp, depthSamp,
                                        lfo1Phase + lrOffset, lfo2Phase + lrOffset, waveform, mix, gain);

        lfo1Phase += lfo1Inc; if (lfo1Phase >= 1.0) lfo1Phase -= 1.0;
        lfo2Phase += lfo2Inc; if (lfo2Phase >= 1.0) lfo2Phase -= 1.0;
    }
}

void VAZChorusAudioProcessor::getStateInformation (juce::MemoryBlock& dest)
{
    if (auto state = apvts.copyState(); state.isValid())
        if (auto xml = state.createXml())
            copyXmlToBinary (*xml, dest);
}

void VAZChorusAudioProcessor::setStateInformation (const void* data, int size)
{
    if (auto xml = getXmlFromBinary (data, size))
        apvts.replaceState (juce::ValueTree::fromXml (*xml));
}

juce::AudioProcessorEditor* VAZChorusAudioProcessor::createEditor()
{
    return new VAZChorusAudioProcessorEditor (*this);
}

juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter()
{
    return new VAZChorusAudioProcessor();
}
