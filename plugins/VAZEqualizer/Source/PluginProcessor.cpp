#include "PluginProcessor.h"
#include "PluginEditor.h"
#include "ParameterIDs.hpp"
#include <cmath>

VAZEqualizerAudioProcessor::VAZEqualizerAudioProcessor()
    : AudioProcessor (BusesProperties()
          .withInput  ("Input",  juce::AudioChannelSet::stereo(), true)
          .withOutput ("Output", juce::AudioChannelSet::stereo(), true)),
      apvts (*this, nullptr, "PARAMETERS", createParameterLayout())
{
}

VAZEqualizerAudioProcessor::~VAZEqualizerAudioProcessor() {}

juce::AudioProcessorValueTreeState::ParameterLayout VAZEqualizerAudioProcessor::createParameterLayout()
{
    using namespace juce;
    AudioProcessorValueTreeState::ParameterLayout layout;
    auto p = [] (const char* id, const char* name, float def)
    {
        return std::make_unique<AudioParameterFloat>(
            ParameterID { id, 1 }, name, NormalisableRange<float>(0.0f, 1.0f), def);
    };
    // Defaults (normalised) match VAZ's GIF: Low 125 Hz shelf, Lo-Mid 1 kHz Q0.70,
    // Hi-Mid 3.97 kHz Q0.70, High 10.2 kHz shelf — all 0 dB. freqN = log10(Hz/20)/3.
    layout.add (p (ParameterIDs::low_gain,   "Low Gain",    0.5f));
    layout.add (p (ParameterIDs::low_freq,   "Low Freq",    0.2653f));   // 125 Hz
    layout.add (p (ParameterIDs::low_q,      "Low Q",       0.0f));      // shelf
    layout.add (p (ParameterIDs::lomid_gain, "Lo-Mid Gain", 0.5f));
    layout.add (p (ParameterIDs::lomid_freq, "Lo-Mid Freq", 0.5663f));   // 1 kHz
    layout.add (p (ParameterIDs::lomid_q,    "Lo-Mid Q",    0.2417f));   // Q ≈ 0.70
    layout.add (p (ParameterIDs::himid_gain, "Hi-Mid Gain", 0.5f));
    layout.add (p (ParameterIDs::himid_freq, "Hi-Mid Freq", 0.7659f));   // 3.97 kHz
    layout.add (p (ParameterIDs::himid_q,    "Hi-Mid Q",    0.2417f));   // Q ≈ 0.70
    layout.add (p (ParameterIDs::high_gain,  "High Gain",   0.5f));
    layout.add (p (ParameterIDs::high_freq,  "High Freq",   0.9025f));   // 10.2 kHz
    layout.add (p (ParameterIDs::high_q,     "High Q",      0.0f));      // shelf
    return layout;
}

VAZEqualizerAudioProcessor::Coefs::Ptr
VAZEqualizerAudioProcessor::makeBand (int band, float gainN, float freqN, float qN) const
{
    const double fs   = curSR;
    const float  freq = (float) juce::jlimit (20.0, fs * 0.45, 20.0 * std::pow (1000.0, (double) freqN));
    const float  gDb  = (gainN - 0.5f) * 36.0f;                 // ±18 dB
    const float  gLin = std::pow (10.0f, gDb / 20.0f);          // linear gain for RBJ peak/shelf

    if (band == LoMid || band == HiMid)
    {
        const float Q = 0.3f * std::pow (10.0f / 0.3f, qN);     // 0.3 .. 10 (log)
        return Coefs::makePeakFilter (fs, freq, Q, gLin);
    }
    if (band == Low)
    {
        if (qN <= 0.8f)
        {
            const float Q = juce::jmap (qN, 0.0f, 0.8f, 0.4f, 2.0f);   // low-shelf slope
            return Coefs::makeLowShelf (fs, freq, Q, gLin);
        }
        const float Q = juce::jmap (qN, 0.8f, 1.0f, 0.7f, 8.0f);       // morph → high-pass
        return Coefs::makeHighPass (fs, freq, Q);
    }
    // High band
    if (qN <= 0.8f)
    {
        const float Q = juce::jmap (qN, 0.0f, 0.8f, 0.4f, 2.0f);       // high-shelf slope
        return Coefs::makeHighShelf (fs, freq, Q, gLin);
    }
    const float Q = juce::jmap (qN, 0.8f, 1.0f, 0.7f, 8.0f);           // morph → low-pass
    return Coefs::makeLowPass (fs, freq, Q);
}

void VAZEqualizerAudioProcessor::prepareToPlay (double sampleRate, int samplesPerBlock)
{
    curSR = sampleRate > 0.0 ? sampleRate : 44100.0;
    juce::dsp::ProcessSpec spec { curSR, (juce::uint32) juce::jmax (1, samplesPerBlock), 1 };
    for (int ch = 0; ch < 2; ++ch)
        for (int b = 0; b < NumBands; ++b)
        {
            bands[ch][b].prepare (spec);
            bands[ch][b].coefficients = Coefs::makePeakFilter (curSR, 1000.0f, 0.7f, 1.0f);   // flat init
            bands[ch][b].reset();
        }
}

bool VAZEqualizerAudioProcessor::isBusesLayoutSupported (const BusesLayout& layouts) const
{
    const auto& in  = layouts.getMainInputChannelSet();
    const auto& out = layouts.getMainOutputChannelSet();
    if (out != juce::AudioChannelSet::stereo() && out != juce::AudioChannelSet::mono())
        return false;
    return in == out;
}

void VAZEqualizerAudioProcessor::processBlock (juce::AudioBuffer<float>& buffer, juce::MidiBuffer&)
{
    juce::ScopedNoDenormals noDenormals;
    auto get = [this] (const char* id) { return apvts.getRawParameterValue (id)->load(); };

    static const struct { const char* g; const char* f; const char* q; } ids[NumBands] = {
        { ParameterIDs::low_gain,   ParameterIDs::low_freq,   ParameterIDs::low_q   },
        { ParameterIDs::lomid_gain, ParameterIDs::lomid_freq, ParameterIDs::lomid_q },
        { ParameterIDs::himid_gain, ParameterIDs::himid_freq, ParameterIDs::himid_q },
        { ParameterIDs::high_gain,  ParameterIDs::high_freq,  ParameterIDs::high_q  },
    };
    // recompute coefficients once per block (control rate); both channels share the band's coefs
    for (int b = 0; b < NumBands; ++b)
    {
        auto c = makeBand (b, get (ids[b].g), get (ids[b].f), get (ids[b].q));
        bands[0][b].coefficients = c;
        bands[1][b].coefficients = c;
    }

    const int nCh = juce::jmin (buffer.getNumChannels(), 2);
    const int n   = buffer.getNumSamples();
    for (int ch = 0; ch < nCh; ++ch)
    {
        float* d = buffer.getWritePointer (ch);
        for (int i = 0; i < n; ++i)
        {
            float x = d[i];
            for (int b = 0; b < NumBands; ++b)
                x = bands[ch][b].processSample (x);
            d[i] = x;
        }
    }
}

void VAZEqualizerAudioProcessor::getStateInformation (juce::MemoryBlock& dest)
{
    if (auto state = apvts.copyState(); state.isValid())
        if (auto xml = state.createXml())
            copyXmlToBinary (*xml, dest);
}

void VAZEqualizerAudioProcessor::setStateInformation (const void* data, int size)
{
    if (auto xml = getXmlFromBinary (data, size))
        apvts.replaceState (juce::ValueTree::fromXml (*xml));
}

juce::AudioProcessorEditor* VAZEqualizerAudioProcessor::createEditor()
{
    return new VAZEqualizerAudioProcessorEditor (*this);
}

//==============================================================================
juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter()
{
    return new VAZEqualizerAudioProcessor();
}
