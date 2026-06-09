#include "PluginProcessor.h"
#include "PluginEditor.h"
#include "ParameterIDs.hpp"

VAZFlangerAudioProcessor::VAZFlangerAudioProcessor()
    : AudioProcessor (BusesProperties()
          .withInput  ("Input",  juce::AudioChannelSet::stereo(), true)
          .withOutput ("Output", juce::AudioChannelSet::stereo(), true)),
      apvts (*this, nullptr, "PARAMETERS", createParameterLayout())
{
}

VAZFlangerAudioProcessor::~VAZFlangerAudioProcessor() {}

juce::AudioProcessorValueTreeState::ParameterLayout VAZFlangerAudioProcessor::createParameterLayout()
{
    using namespace juce;
    AudioProcessorValueTreeState::ParameterLayout layout;
    auto pct = [] (const char* id, const char* name, float def)
    {
        return std::make_unique<AudioParameterFloat>(
            ParameterID { id, 1 }, name, NormalisableRange<float>(0.0f, 1.0f), def);
    };
    layout.add (pct (ParameterIDs::delay_time, "Delay Time", 0.35f)); // base comb delay 0.1..20 ms
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

void VAZFlangerAudioProcessor::prepareToPlay (double sampleRate, int)
{
    sr = sampleRate;
    chL.prepare (sampleRate); chR.prepare (sampleRate);
    lfoPhase = 0.0;
}

bool VAZFlangerAudioProcessor::isBusesLayoutSupported (const BusesLayout& layouts) const
{
    const auto& in  = layouts.getMainInputChannelSet();
    const auto& out = layouts.getMainOutputChannelSet();
    if (out != juce::AudioChannelSet::stereo() && out != juce::AudioChannelSet::mono())
        return false;
    return in == out;
}

void VAZFlangerAudioProcessor::processBlock (juce::AudioBuffer<float>& buffer, juce::MidiBuffer&)
{
    juce::ScopedNoDenormals noDenormals;

    const float fDelay  = apvts.getRawParameterValue (ParameterIDs::delay_time)->load();
    const float fFb     = apvts.getRawParameterValue (ParameterIDs::feedback)->load();
    const float fRate   = apvts.getRawParameterValue (ParameterIDs::rate)->load();
    const float fDepth  = apvts.getRawParameterValue (ParameterIDs::depth)->load();
    const float fLrPh   = apvts.getRawParameterValue (ParameterIDs::lr_phase)->load();
    const float fMix    = apvts.getRawParameterValue (ParameterIDs::mix)->load();
    const float fGain   = apvts.getRawParameterValue (ParameterIDs::gain)->load();
    const float fbPhase = apvts.getRawParameterValue (ParameterIDs::feedback_phase)->load();

    // Real flanger (RE'd from Core.dll TFXFlanger @0x5204F4): a delay-line flanger.
    // Delay Time = minimum delay; the triangle LFO sweeps 'depth' samples on top of it.
    const double baseMs    = 0.5 + (double) fDelay * 5.0;             // Delay Time → 0.5 .. 5.5 ms minimum delay
    const double depthMs   = (double) fDepth * (baseMs * 1.6 + 1.0); // LFO sweep amount above the base
    const double baseSamp  = baseMs  * 0.001 * sr;
    const double depthSamp = depthMs * 0.001 * sr;
    const double fbSign    = fbPhase > 0.5f ? -1.0 : 1.0;            // Feedback Phase: − inverts the feedback
    const double feedback  = (double) fFb * 0.92 * fbSign;          // feedback comb gain (real [+0x264])
    const double mix       = (double) fMix;
    const double gain      = (double) fGain;
    const double inGain    = 1.0;                                    // real [+0x294] (input into the delay line)
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
    if (lfoInc <= 0.0) lfoPhase = 0.0;            // static → freeze LFO so Delay Time alone sets a fixed metallic comb
    const double lrOffset = (double) fLrPh * 0.5;          // up to half-cycle between channels

    const int n = buffer.getNumSamples();
    const int numCh = buffer.getNumChannels();
    float* L = buffer.getWritePointer (0);
    float* R = numCh > 1 ? buffer.getWritePointer (1) : nullptr;

    const bool stat = (lfoInc <= 0.0);                    // Rate fully down → frozen sweep (fixed comb)
    auto triangle = [] (double ph) noexcept               // VAZ LFO = abs(phase) = triangle (real @0x520531)
    { return ph < 0.5 ? 2.0 * ph : 2.0 - 2.0 * ph; };     // 0 → 1 → 0 over one cycle

    for (int i = 0; i < n; ++i)
    {
        const double dL = baseSamp + triangle (lfoPhase) * depthSamp;     // swept fractional delay
        L[i] = (float) (chL.process ((double) L[i], dL, feedback, mix, inGain) * gain);

        if (R != nullptr)
        {
            double pr = lfoPhase + lrOffset; if (pr >= 1.0) pr -= 1.0;     // L/R Phase offset
            const double dR = baseSamp + triangle (pr) * depthSamp;
            R[i] = (float) (chR.process ((double) R[i], dR, feedback, mix, inGain) * gain);
        }

        if (! stat) { lfoPhase += lfoInc; if (lfoPhase >= 1.0) lfoPhase -= 1.0; }
    }
}

void VAZFlangerAudioProcessor::getStateInformation (juce::MemoryBlock& dest)
{
    if (auto state = apvts.copyState(); state.isValid())
        if (auto xml = state.createXml())
            copyXmlToBinary (*xml, dest);
}

void VAZFlangerAudioProcessor::setStateInformation (const void* data, int size)
{
    if (auto xml = getXmlFromBinary (data, size))
        apvts.replaceState (juce::ValueTree::fromXml (*xml));
}

juce::AudioProcessorEditor* VAZFlangerAudioProcessor::createEditor()
{
    return new VAZFlangerAudioProcessorEditor (*this);
}

juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter()
{
    return new VAZFlangerAudioProcessor();
}
