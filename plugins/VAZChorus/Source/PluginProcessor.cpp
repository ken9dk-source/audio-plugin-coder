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
        juce::StringArray { "Sine", "Trapezoid", "Triangle" }, 0));   // order = VAZ modes 0/1/2 (FUN_00518ad8: 1=trap, 2=tri)
    layout.add (std::make_unique<AudioParameterBool> (ParameterID { ParameterIDs::mod_sync, 1 }, "Sync", false));
    const StringArray modPeriods { "1/32T","1/32","1/16T","1/16","1/8T","1/8","1/4T","1/4",
        "2b","3b","4b","5b","6b","8b","12b","16b","24b","32b","48b","64b","96b","128b","192b","256b" };
    layout.add (std::make_unique<AudioParameterChoice> (ParameterID { ParameterIDs::mod_period, 1 }, "Period", modPeriods, 10)); // default 4 beats
    return layout;
}

void VAZChorusAudioProcessor::prepareToPlay (double sampleRate, int)
{
    sr = sampleRate > 0.0 ? sampleRate : 44100.0;
    engine.clearBuffers();
    engine.buildSineLut();
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

    // Base delay (EXACT, FUN_00518fbc @0x518fbc): base = (sr·50/256000 int-div)·(delayParam+1) → (v+1)/5.12 ms.
    const int srI        = (int) std::llround (sr);
    const int delayParam = juce::jlimit (0, 255, (int) std::lround (fDelay * 255.0f));
    engine.base = ((srI * 50) / 256000) * (delayParam + 1);
    // Waveform mode: 0 sine · 1 trapezoid · 2 triangle (VAZ order) — the engine selects the shape directly.
    engine.mode1 = engine.mode2 = juce::jlimit (0, 2, waveform);
    // Modulation rate → 32-bit phase increments.  ⚠ APPROX (rate/depth/lr/gain scalings pending exact setters, step #3):
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
        rateHz = (double) fRate * (double) fRate * 6.0;       // free: 0..6 Hz (chorus is slow)
    engine.inc1  = (uint32_t) (int64_t) (rateHz / sr * 4294967296.0);
    engine.inc2  = (uint32_t) (int64_t) (rateHz * 1.27 / sr * 4294967296.0);   // 2nd LFO detuned (approx)
    engine.level = 0x8000;                                                     // common scale (approx)
    const int32_t modDepth = (int32_t) std::llround ((double) fDepth * 0.04 * sr);
    engine.depth = engine.level2 = modDepth;                                   // both LFO depths = fDepth (approx)
    engine.lrPhase = (int32_t) std::llround ((double) fLrPh * 1073741824.0);   // stereo spread (approx)
    engine.gain    = juce::jlimit (0, 255, (int) std::lround (fMix * 255.0f)); // wet amount (approx)
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
