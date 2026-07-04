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
    engine.prepare (sr);                 // buffer = next pow2(sr·2.55) (FUN_0051bf78)
    curDelL = curDelR = 0.1 * sr;
}

bool VAZDelayAudioProcessor::isBusesLayoutSupported (const BusesLayout& layouts) const
{
    const auto& in  = layouts.getMainInputChannelSet();
    const auto& out = layouts.getMainOutputChannelSet();
    if (out != juce::AudioChannelSet::stereo() && out != juce::AudioChannelSet::mono())
        return false;
    return in == out;
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
    // Sync: note value × a MUSICAL multiplier, snapped so the delay always locks to the host grid.
    // (Was a continuous 0.5·4^p = 50..200%, only exactly 100% at p=0.5 — but the slider defaults to 0.65,
    // so a "1/4 note" actually ran at ~123% = off-grid. Now the default slider sits in the 100% band.)
    auto syncMult = [] (double p) -> double
    {
        return p < 0.15 ? 0.5            // ½  (note ÷ 2)
             : p < 0.32 ? 2.0 / 3.0      // triplet down
             : p < 0.45 ? 0.75           // dotted down
             : p < 0.74 ? 1.0            // unity — the default slider position (0.65) lands here → grid-locked
             : p < 0.84 ? 4.0 / 3.0      // triplet up
             : p < 0.93 ? 1.5            // dotted up
             :            2.0;           // ×2  (note × 2)
    };
    auto delSamples = [&] (float p, int noteIdx) -> double
    {
        if (sync)   // tempo-synced: exact note value × musical multiplier (grid-locked)
        {
            const double beats = noteBeats[juce::jlimit (0, 13, noteIdx)];
            return beats * syncMult ((double) p) * (60.0 / bpm) * sr;
        }
        return 5.0 * std::pow (1200.0, (double) p) * 0.001 * sr;       // free: 5 ms .. 6 s (log)
    };
    const int    maxTap = engine.mask - 2;
    const double tgtL = juce::jlimit (1.0, (double) maxTap, delSamples (dL, noteL));
    const double tgtR = juce::jlimit (1.0, (double) maxTap, delSamples (dR, noteR));

    // VAZ damping one-pole (render @0x51bba8:54-56): w = x + (state−x)·k, k = damp/2^28 (feedback retention).
    // Match the clone's tone (fc = 150·66^t): k = 1 − tc = exp(−2π·fc/sr). ⚠ exact VAZ tone→damp is 80-bit (approx).
    auto dampCoef = [&] (float t) -> int32_t
    {
        const double fc = 150.0 * std::pow (66.0, (double) t);
        const double k  = std::exp (-juce::MathConstants<double>::twoPi * fc / sr);
        return (int32_t) juce::jlimit ((long long) 0, (long long) 0x0FFFFFFF, std::llround (k * (double) 0x10000000));
    };

    engine.mode = juce::jlimit (0, 2, mode);
    engine.fbL  = juce::jlimit (0, 255, (int) std::lround (fbL * 255.0f));   // feedback (fb/256)
    engine.fbR  = juce::jlimit (0, 255, (int) std::lround (fbR * 255.0f));
    engine.dampL = dampCoef (tnL);  engine.dampR = dampCoef (tnR);
    engine.dryL = (int32_t) std::llround ((double) dryL * (double) 0x40000000);   // Q30
    engine.dryR = (int32_t) std::llround ((double) dryR * (double) 0x40000000);
    engine.wetL = (int32_t) std::llround ((double) wL   * (double) 0x40000000);   // Q30
    engine.wetR = (int32_t) std::llround ((double) wR   * (double) 0x40000000);

    const double smooth = 1.0 - std::exp (-1.0 / (0.02 * sr));     // ~20 ms delay-time glide → quantised int tap
    constexpr double kFS = 8388608.0;                             // Q23 full-scale
    const int n = buffer.getNumSamples();
    const int nCh = buffer.getNumChannels();
    float* L = buffer.getWritePointer (0);
    float* R = nCh > 1 ? buffer.getWritePointer (1) : nullptr;
    for (int i = 0; i < n; ++i)
    {
        curDelL += (tgtL - curDelL) * smooth;
        curDelR += (tgtR - curDelR) * smooth;
        engine.delayL = juce::jlimit (1, maxTap, (int) std::lround (curDelL));
        engine.delayR = juce::jlimit (1, maxTap, (int) std::lround (curDelR));
        int32_t li = (int32_t) std::llround ((double) L[i] * kFS);
        int32_t ri = R ? (int32_t) std::llround ((double) R[i] * kFS) : li;
        engine.processFrame (li, ri);
        L[i] = (float) ((double) li / kFS);
        if (R) R[i] = (float) ((double) ri / kFS);
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
