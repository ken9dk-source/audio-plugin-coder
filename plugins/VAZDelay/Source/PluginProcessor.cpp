#include "PluginProcessor.h"
#include "PluginEditor.h"
#include "ParameterIDs.hpp"

VAZDelayAudioProcessor::VAZDelayAudioProcessor()
    : AudioProcessor (BusesProperties()
          .withInput  ("Input",  juce::AudioChannelSet::stereo(), true)
          .withOutput ("Output", juce::AudioChannelSet::stereo(), true)),
      apvts (*this, nullptr, "PARAMETERS", createParameterLayout())
{
}

VAZDelayAudioProcessor::~VAZDelayAudioProcessor() {}

juce::AudioProcessorValueTreeState::ParameterLayout VAZDelayAudioProcessor::createParameterLayout()
{
    using namespace juce;
    AudioProcessorValueTreeState::ParameterLayout layout;
    auto pct  = [] (const char* id, const char* name, float def)
    { return std::make_unique<AudioParameterFloat>(ParameterID { id, 1 }, name, NormalisableRange<float>(0.0f,1.0f), def); };
    auto boolp = [] (const char* id, const char* name, bool def)
    { return std::make_unique<AudioParameterBool>(ParameterID { id, 1 }, name, def); };

    layout.add (std::make_unique<AudioParameterChoice>(
        ParameterID { ParameterIDs::mode, 1 }, "Mode", StringArray { "Stereo", "Ping-Pong", "Double" }, 0));
    layout.add (boolp (ParameterIDs::link, "Link", false));
    layout.add (boolp (ParameterIDs::sync, "Sync", false));

    const StringArray noteVals { "32nd triplet", "32nd note", "16th triplet", "16th note", "8th triplet",
                                 "16th dotted", "8th note", "1/4 triplet", "8th dotted", "1/4 note",
                                 "1/4 dotted", "2 beats", "3 beats", "4 beats" };
    layout.add (std::make_unique<AudioParameterChoice>(ParameterID { ParameterIDs::note_l, 1 }, "Note L", noteVals, 9));  // 1/4 note
    layout.add (std::make_unique<AudioParameterChoice>(ParameterID { ParameterIDs::note_r, 1 }, "Note R", noteVals, 9));

    layout.add (pct (ParameterIDs::delay_l, "Delay L",    0.65f));   // ~500 ms
    layout.add (pct (ParameterIDs::fb_l,    "Feedback L", 0.35f));
    layout.add (pct (ParameterIDs::tone_l,  "Tone L",     0.6f));
    layout.add (pct (ParameterIDs::wet_l,   "Wet L",      0.5f));    // ~−6 dB
    layout.add (pct (ParameterIDs::dry_l,   "Dry L",      1.0f));    // 0 dB

    layout.add (pct (ParameterIDs::delay_r, "Delay R",    0.65f));
    layout.add (pct (ParameterIDs::fb_r,    "Feedback R", 0.35f));
    layout.add (pct (ParameterIDs::tone_r,  "Tone R",     0.6f));
    layout.add (pct (ParameterIDs::wet_r,   "Wet R",      0.5f));
    layout.add (pct (ParameterIDs::dry_r,   "Dry R",      1.0f));
    return layout;
}

void VAZDelayAudioProcessor::prepareToPlay (double sampleRate, int)
{
    sr = sampleRate > 0.0 ? sampleRate : 44100.0;
    bufLen = (int) (6.05 * sr) + 4;                 // 6 s max delay
    bufL.assign ((size_t) bufLen, 0.0f);
    bufR.assign ((size_t) bufLen, 0.0f);
    writeL = writeR = 0;
    curDelL = curDelR = 0.1 * sr;
    toneZL = toneZR = 0.0;
}

bool VAZDelayAudioProcessor::isBusesLayoutSupported (const BusesLayout& layouts) const
{
    const auto& in  = layouts.getMainInputChannelSet();
    const auto& out = layouts.getMainOutputChannelSet();
    if (out != juce::AudioChannelSet::stereo() && out != juce::AudioChannelSet::mono())
        return false;
    return in == out;
}

float VAZDelayAudioProcessor::readInterp (const std::vector<float>& buf, double readPos) const noexcept
{
    int i0 = (int) readPos;
    double f = readPos - (double) i0;
    if (i0 >= bufLen) i0 -= bufLen;
    int i1 = i0 + 1; if (i1 >= bufLen) i1 -= bufLen;
    return buf[(size_t) i0] + (buf[(size_t) i1] - buf[(size_t) i0]) * (float) f;
}

void VAZDelayAudioProcessor::processBlock (juce::AudioBuffer<float>& buffer, juce::MidiBuffer&)
{
    juce::ScopedNoDenormals noDenormals;

    const int   mode = (int) apvts.getRawParameterValue (ParameterIDs::mode)->load();
    const bool  link = apvts.getRawParameterValue (ParameterIDs::link)->load() > 0.5f;
    const bool  sync = apvts.getRawParameterValue (ParameterIDs::sync)->load() > 0.5f;

    float dL = apvts.getRawParameterValue (ParameterIDs::delay_l)->load();
    float fbL = apvts.getRawParameterValue (ParameterIDs::fb_l)->load();
    float tnL = apvts.getRawParameterValue (ParameterIDs::tone_l)->load();
    float wL  = apvts.getRawParameterValue (ParameterIDs::wet_l)->load();
    float dryL = apvts.getRawParameterValue (ParameterIDs::dry_l)->load();
    float dR = apvts.getRawParameterValue (ParameterIDs::delay_r)->load();
    float fbR = apvts.getRawParameterValue (ParameterIDs::fb_r)->load();
    float tnR = apvts.getRawParameterValue (ParameterIDs::tone_r)->load();
    float wR  = apvts.getRawParameterValue (ParameterIDs::wet_r)->load();
    float dryR = apvts.getRawParameterValue (ParameterIDs::dry_r)->load();
    int noteL = (int) apvts.getRawParameterValue (ParameterIDs::note_l)->load();
    int noteR = (int) apvts.getRawParameterValue (ParameterIDs::note_r)->load();
    if (link) { dR = dL; fbR = fbL; tnR = tnL; wR = wL; dryR = dryL; noteR = noteL; }

    double bpm = 120.0;
    if (auto* ph = getPlayHead())
        if (auto pos = ph->getPosition())
            if (auto b = pos->getBpm()) bpm = *b > 1.0 ? *b : 120.0;

    // VAZ note divisions (in beats), matching the Delay note popup order (32nd-trip .. 4 beats)
    static const double noteBeats[] = { 1.0/12.0, 0.125, 1.0/6.0, 0.25, 1.0/3.0, 0.375, 0.5,
                                        2.0/3.0, 0.75, 1.0, 1.5, 2.0, 3.0, 4.0 };
    auto delSamples = [&] (float p, int noteIdx) -> double
    {
        if (sync)   // note base × 50%..200% (100% at the slider centre)
        {
            const double beats = noteBeats[juce::jlimit (0, 13, noteIdx)];
            const double mult  = 0.5 * std::pow (4.0, (double) p);     // 50% .. 200%
            return beats * mult * (60.0 / bpm) * sr;
        }
        return 5.0 * std::pow (1200.0, (double) p) * 0.001 * sr;       // free: 5 ms .. 6 s (log)
    };
    const double tgtL = juce::jlimit (1.0, (double) (bufLen - 2), delSamples (dL, noteL));
    const double tgtR = juce::jlimit (1.0, (double) (bufLen - 2), delSamples (dR, noteR));

    auto toneCoef = [&] (float t) -> double
    {
        const double fc = 150.0 * std::pow (66.0, (double) t);     // Dark 150 Hz .. Bright ~10 kHz
        return 1.0 - std::exp (-juce::MathConstants<double>::twoPi * fc / sr);
    };
    const double tcL = toneCoef (tnL), tcR = toneCoef (tnR);
    const double smooth = 1.0 - std::exp (-1.0 / (0.02 * sr));     // ~20 ms delay-time glide → click-free

    const int n = buffer.getNumSamples();
    const int nCh = buffer.getNumChannels();
    float* L = buffer.getWritePointer (0);
    float* R = nCh > 1 ? buffer.getWritePointer (1) : nullptr;

    for (int i = 0; i < n; ++i)
    {
        curDelL += (tgtL - curDelL) * smooth;
        curDelR += (tgtR - curDelR) * smooth;

        const double inL = (double) L[i];
        const double inR = R != nullptr ? (double) R[i] : inL;

        double rdL = (double) writeL - curDelL; while (rdL < 0.0) rdL += bufLen;
        double rdR = (double) writeR - curDelR; while (rdR < 0.0) rdR += bufLen;
        const double dlyL = (double) readInterp (bufL, rdL);
        const double dlyR = (double) readInterp (bufR, rdR);

        toneZL += tcL * (dlyL - toneZL);          // 1-pole LP in the feedback path
        toneZR += tcR * (dlyR - toneZR);
        const double tL = toneZL, tR = toneZR;

        double wrL, wrR, outL, outR;
        if (mode == 1)            // Ping-Pong: feedback crosses to the opposite channel
        {
            wrL = inL + (double) fbL * tR;
            wrR = inR + (double) fbR * tL;
            outL = inL * dryL + dlyL * wL;
            outR = inR * dryR + dlyR * wR;
        }
        else if (mode == 2)       // Double: the two delays in series (mono pattern)
        {
            const double inM = 0.5 * (inL + inR);
            wrL = inM + (double) fbL * tR;        // delay 1 = input + feedback from end
            wrR = tL;                             // delay 1 output feeds delay 2
            outL = inL * dryL + dlyR * wL;        // output = end of the chain
            outR = inR * dryR + dlyR * wR;
        }
        else                      // Stereo: independent L / R
        {
            wrL = inL + (double) fbL * tL;
            wrR = inR + (double) fbR * tR;
            outL = inL * dryL + dlyL * wL;
            outR = inR * dryR + dlyR * wR;
        }

        bufL[(size_t) writeL] = (float) wrL;
        bufR[(size_t) writeR] = (float) wrR;
        if (++writeL >= bufLen) writeL = 0;
        if (++writeR >= bufLen) writeR = 0;

        L[i] = (float) outL;
        if (R != nullptr) R[i] = (float) outR;
    }
}

void VAZDelayAudioProcessor::getStateInformation (juce::MemoryBlock& dest)
{
    if (auto state = apvts.copyState(); state.isValid())
        if (auto xml = state.createXml())
            copyXmlToBinary (*xml, dest);
}

void VAZDelayAudioProcessor::setStateInformation (const void* data, int size)
{
    if (auto xml = getXmlFromBinary (data, size))
        apvts.replaceState (juce::ValueTree::fromXml (*xml));
}

juce::AudioProcessorEditor* VAZDelayAudioProcessor::createEditor()
{
    return new VAZDelayAudioProcessorEditor (*this);
}

//==============================================================================
juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter()
{
    return new VAZDelayAudioProcessor();
}
