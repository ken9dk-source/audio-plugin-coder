#include "PluginProcessor.h"
#include "PluginEditor.h"
#include "QuadraFuzzShapeTables.h"
#include <cmath>

using APVTS = juce::AudioProcessorValueTreeState;

constexpr float QuadraFuzzAudioProcessor::DEFAULT_CROSS[];

//==============================================================================
// 5 waveshaper types (Shape 0..4), matching the original's shape-button count.
static const juce::StringArray SHAPE_NAMES { "0", "1", "2", "3", "4" };

APVTS::ParameterLayout QuadraFuzzAudioProcessor::createParameterLayout()
{
    APVTS::ParameterLayout layout;

    auto dbStr = [] (float v, int) { return juce::String (v, 1); };
    // A fresh instance opens on the neutral "Default" (all 0 dB).

    // [0..3] Band1..Band4 — -12..+12 dB (the 4 fuzz-drive knobs)
    for (int b = 1; b <= 4; ++b)
        layout.add (std::make_unique<juce::AudioParameterFloat> (
            juce::ParameterID { "Band" + juce::String (b), 1 },
            "Band" + juce::String (b),
            juce::NormalisableRange<float> (-12.f, 12.f, 0.01f), 0.0f,
            juce::AudioParameterFloatAttributes{}.withLabel ("dB")
                                                 .withStringFromValueFunction (dbStr)));

    // [4] In, [5] Out — -20..+20 dB
    layout.add (std::make_unique<juce::AudioParameterFloat> (
        juce::ParameterID { "In", 1 }, "In",
        juce::NormalisableRange<float> (-20.f, 20.f, 0.01f), 0.0f,
        juce::AudioParameterFloatAttributes{}.withLabel ("dB").withStringFromValueFunction (dbStr)));

    layout.add (std::make_unique<juce::AudioParameterFloat> (
        juce::ParameterID { "Out", 1 }, "Out",
        juce::NormalisableRange<float> (-20.f, 20.f, 0.01f), 0.0f,
        juce::AudioParameterFloatAttributes{}.withLabel ("dB").withStringFromValueFunction (dbStr)));

    // [6] Shape — 5 choices
    layout.add (std::make_unique<juce::AudioParameterChoice> (
        juce::ParameterID { "Shape", 1 }, "Shape", SHAPE_NAMES, 0));

    // [7] Preset — continuous 0..1
    layout.add (std::make_unique<juce::AudioParameterFloat> (
        juce::ParameterID { "Preset", 1 }, "Preset",
        juce::NormalisableRange<float> (0.f, 1.f, 0.001f), 0.f));

    return layout;
}

//==============================================================================
QuadraFuzzAudioProcessor::QuadraFuzzAudioProcessor()
    : AudioProcessor (BusesProperties()
          .withInput  ("Input",  juce::AudioChannelSet::stereo(), true)
          .withOutput ("Output", juce::AudioChannelSet::stereo(), true)),
      apvts (*this, nullptr, "QuadraFuzz", createParameterLayout())
{
    // Load the on-disk preset bank (seeds from the factory snapshot on first run).
    // Live params/atomics already default to the neutral "Default" (see header).
    loadPresetBank();
}

//==============================================================================
bool QuadraFuzzAudioProcessor::isBusesLayoutSupported (const BusesLayout& layouts) const
{
    if (layouts.getMainOutputChannelSet() != juce::AudioChannelSet::stereo() &&
        layouts.getMainOutputChannelSet() != juce::AudioChannelSet::mono())
        return false;
    return layouts.getMainInputChannelSet() == layouts.getMainOutputChannelSet();
}

void QuadraFuzzAudioProcessor::prepareToPlay (double sampleRate, int samplesPerBlock)
{
    sr = sampleRate;
    const int ch = getTotalNumInputChannels();

    for (int b = 0; b < NUM_BANDS; ++b) bandBP[b].reset();
    for (int i = 0; i <= NUM_BANDS; ++i) lastEdge[i] = -1.f;

    dryBuf  .setSize (ch, samplesPerBlock);
    band0Buf.setSize (ch, samplesPerBlock);
    above0Buf.setSize(ch, samplesPerBlock);
    band1Buf.setSize (ch, samplesPerBlock);
    above1Buf.setSize(ch, samplesPerBlock);
    band2Buf.setSize (ch, samplesPerBlock);
    band3Buf.setSize (ch, samplesPerBlock);
}

//==============================================================================
// Re-design the four band-pass filters whenever any of the five edges moves.
// Band b spans [edge[b], edge[b+1]];  edges = {edgeLo, cross0, cross1, cross2, edgeHi}.
void QuadraFuzzAudioProcessor::updateFilters (float lo, float c0, float c1, float c2, float hi)
{
    const float e[NUM_BANDS + 1] = { lo, c0, c1, c2, hi };
    bool changed = false;
    for (int i = 0; i <= NUM_BANDS; ++i)
        if (std::abs (e[i] - lastEdge[i]) > 0.1f) changed = true;
    if (! changed) return;

    for (int b = 0; b < NUM_BANDS; ++b)
        bandBP[b].design (juce::jlimit (20.0, sr * 0.49, (double) e[b]),
                          juce::jlimit (20.0, sr * 0.49, (double) e[b + 1]), sr);
    for (int i = 0; i <= NUM_BANDS; ++i) lastEdge[i] = e[i];
}

void QuadraFuzzAudioProcessor::runCrossover (const juce::AudioBuffer<float>& src)
{
    const int ch = src.getNumChannels();
    const int n  = src.getNumSamples();

    // Independent per-band 4th-order Butterworth BANDPASS (true LP->BP transform,
    // not a HP.LP cascade) — the DLL's real topology. band b = bandBP[b](x); the
    // bands share no filter state, so they sum cleanly.
    juce::AudioBuffer<float>* bands[NUM_BANDS] = { &band0Buf, &band1Buf, &band2Buf, &band3Buf };
    for (int c = 0; c < ch; ++c)
    {
        const float* in = src.getReadPointer (c);
        for (int b = 0; b < NUM_BANDS; ++b)
        {
            float* d = bands[b]->getWritePointer (c);
            for (int s = 0; s < n; ++s)
                d[s] = bandBP[b].process (in[s], c);
        }
    }
}

// EXACT waveshaper from QuadraFuzz.dll (FUN_1000a320): the input x is already
// pre-gained; index the selected shape's 256-point table with round()+lerp, and
// hard-clip to ±1 outside [-1,1]. Bit-faithful to the original.
float QuadraFuzzAudioProcessor::waveshape (float x, int shape) noexcept
{
    const float* tbl = QFShapes::SHAPE_TABLE[juce::jlimit (0, 4, shape)];
    const float ax = std::abs (x);
    if (ax > 1.0f) return (x >= 0.0f) ? 1.0f : -1.0f;     // clip (sets Over in the original)
    const float fidx = ax * 254.0f;                       // _DAT_10021988 = 254
    int i = (int) std::lround (fidx);
    i = juce::jlimit (0, 255, i);
    const float frac = fidx - (float) i;                  // round-based, frac in [-0.5,0.5]
    const float out  = (tbl[i + 1] - tbl[i]) * frac + tbl[i];
    return (x >= 0.0f) ? out : -out;
}

// Per band (matches the original's order): pre-gain by the EQ-diamond LEVEL,
// waveshape via the table, then post-gain by the band-GAIN knob. (Global In is
// applied to the input before the crossover, Out to the sum afterwards — both
// linear, so equivalent to the original's per-sample In·…·Out.)
// Per-shape post-shaper normalisation (engine+0x3c). GROUND TRUTH from the Ghidra
// decompile (FUN_1000a540 @ 0x1000a540): shapeNorm = pow(10.0, -tbl[256]*0.05) =
// 10^(-headroom/20), where headroom = the shape table's [256] entry {2, 6.1, 7.4,
// 6.9, 10.2} (const_A=10 @0x10021470, const_B=0.05 @0x10021990). So it is NOT a
// free constant — it is derived from the tables, exactly as the DLL does it. The
// earlier 4-dp MEASURED values {0.7943,...} were right to ~1e-4, which left a fixed
// per-shape null residual of -80..-86 dB (constant across the whole In-drive sweep
// = a fixed post-shaper scalar error). Computing the literal removes it.
static float qfShapeNorm (int s) noexcept
{ return (float) std::pow (10.0, -(double) QFShapes::SHAPE_TABLE[s][256] / 20.0); }
static const float SHAPE_NORM[5] =
    { qfShapeNorm (0), qfShapeNorm (1), qfShapeNorm (2), qfShapeNorm (3), qfShapeNorm (4) };

void QuadraFuzzAudioProcessor::applyBandFuzz (juce::AudioBuffer<float>& buf,
                                              float gainDb, float levelDb, int shape)
{
    const float level = juce::Decibels::decibelsToGain (levelDb);   // EQ diamond, pre
    const float gain  = juce::Decibels::decibelsToGain (gainDb);    // band knob, post
    const float norm  = SHAPE_NORM[juce::jlimit (0, 4, shape)];     // shapeNorm
    bool over = false;
    for (int c = 0; c < buf.getNumChannels(); ++c)
    {
        float* d = buf.getWritePointer (c);
        for (int s = 0; s < buf.getNumSamples(); ++s)
        {
            const float wsIn = d[s] * level;                    // waveshaper input (= DLL's bandSig * inGain * drive)
            if (wsIn > 1.0f || wsIn < -1.0f) over = true;       // OVER: outside the +/-1.0 table range (clip point)
            d[s] = waveshape (wsIn, shape) * gain * norm;
        }
    }
    if (over) overFlag.store (1.0f, std::memory_order_relaxed); // latch; editor reads-and-clears
}

//==============================================================================
void QuadraFuzzAudioProcessor::processBlock (juce::AudioBuffer<float>& buffer, juce::MidiBuffer&)
{
    juce::ScopedNoDenormals noDenormals;
    const int ch = buffer.getNumChannels();
    const int n  = buffer.getNumSamples();

    const float inGain  = apvts.getRawParameterValue ("In")->load();
    const float outGain = apvts.getRawParameterValue ("Out")->load();
    const int   shape   = (int) apvts.getRawParameterValue ("Shape")->load();

    float drive[4];
    for (int b = 0; b < 4; ++b)
        drive[b] = apvts.getRawParameterValue ("Band" + juce::String (b + 1))->load();

    // Internal filterbank state (the EQ window) — independent of the knobs.
    updateFilters (edgeLoHz.load(), crossHz[0].load(), crossHz[1].load(),
                   crossHz[2].load(), edgeHiHz.load());
    float level[4];
    for (int b = 0; b < 4; ++b) level[b] = bandLevelDb[b].load();

    buffer.applyGain (juce::Decibels::decibelsToGain (inGain));

    runCrossover (buffer);
    juce::AudioBuffer<float>* bands[4] = { &band0Buf, &band1Buf, &band2Buf, &band3Buf };
    // NaN/Inf safety net: if a band filter ever goes unstable, flush its state and
    // silence just that band this block, so audio recovers next block instead of
    // being stuck silent forever (a stuck NaN in z1/z2 poisons every future sample).
    for (int b = 0; b < 4; ++b)
    {
        bool bad = false;
        for (int c = 0; c < ch && ! bad; ++c)
        {
            const float* p = bands[b]->getReadPointer (c);
            for (int s = 0; s < n; ++s) if (! std::isfinite (p[s])) { bad = true; break; }
        }
        if (bad) { bandBP[b].reset(); bands[b]->clear(); lastEdge[0] = -1.f; }  // force redesign
    }
    for (int b = 0; b < 4; ++b)
        applyBandFuzz (*bands[b], drive[b], level[b], shape);

    buffer.clear();
    for (int b = 0; b < 4; ++b)
        for (int c = 0; c < ch; ++c)
            buffer.addFrom (c, 0, *bands[b], c, 0, n);

    buffer.applyGain (juce::Decibels::decibelsToGain (outGain));
}

//==============================================================================
void QuadraFuzzAudioProcessor::getStateInformation (juce::MemoryBlock& dest)
{
    auto state = apvts.copyState();
    // Persist the internal filterbank (EQ) state + the selected preset index.
    state.setProperty ("currentPreset", currentPreset, nullptr);
    auto eq = state.getOrCreateChildWithName ("filterbank", nullptr);
    for (int b = 0; b < NUM_BANDS; ++b)
        eq.setProperty ("level" + juce::String (b), bandLevelDb[b].load(), nullptr);
    for (int i = 0; i < NUM_CROSS; ++i)
        eq.setProperty ("cross" + juce::String (i), crossHz[i].load(), nullptr);
    eq.setProperty ("edgeLo", edgeLoHz.load(), nullptr);
    eq.setProperty ("edgeHi", edgeHiHz.load(), nullptr);

    if (auto xml = state.createXml())
        copyXmlToBinary (*xml, dest);
}

void QuadraFuzzAudioProcessor::setStateInformation (const void* data, int size)
{
    if (auto xml = getXmlFromBinary (data, size))
        if (xml->hasTagName (apvts.state.getType()))
        {
            auto tree = juce::ValueTree::fromXml (*xml);
            apvts.replaceState (tree);
            currentPreset = juce::jlimit (0, juce::jmax (0, getNumPresets() - 1),
                                          (int) tree.getProperty ("currentPreset", 0));
            if (auto eq = tree.getChildWithName ("filterbank"); eq.isValid())
            {
                for (int b = 0; b < NUM_BANDS; ++b)
                    bandLevelDb[b].store ((float) eq.getProperty ("level" + juce::String (b), 0.f));
                for (int i = 0; i < NUM_CROSS; ++i)
                    crossHz[i].store ((float) eq.getProperty ("cross" + juce::String (i), DEFAULT_CROSS[i]));
                edgeLoHz.store ((float) eq.getProperty ("edgeLo", DEFAULT_EDGE_LO));
                edgeHiHz.store ((float) eq.getProperty ("edgeHi", DEFAULT_EDGE_HI));
            }
        }
}

//==============================================================================
// PRESET BANK (same model as the original: a disk-backed bank of up to 64).
//
// FACTORY SEED — only used to populate the bank on first run (when no bank file
// exists yet). Recovered from the original's chunk (decoded from the user's .flp,
// identical across two projects). "Default" is neutral. After first run the
// on-disk bank is authoritative and fully user-editable (Create / Delete).
namespace {
struct QFSeed { const char* name; float band[4]; float in, out; int shape; float level[4]; float cross[3]; float edgeLo, edgeHi; };
static constexpr int SEED_VERSION = 5;   // bump to force a re-seed of an older on-disk bank
// Verified 1:1 against the original (user supplied photos of every preset). The
// original chunk stores records as [values][name], so the correct pairing is
// "preset N gets the values that PRECEDE its name" — i.e. each named preset = the
// decoded values one slot earlier, "Default" = neutral, and a 17th preset ("60s
// Lead") appears at the end. Fields: name | Band1-4 | In | Out | Shape | levels | crossovers.
static const QFSeed FACTORY_SEED[] = {
    // Decoded straight from the DLL's factory BANK chunk (effGetChunk). Fields:
    // name | Band1-4 | In | Out | Shape | levels(f0-3) | crossovers(f5-7) | edgeLo(f4) | edgeHi(f8).
    { "Default",       {0.0000f,0.0000f,0.0000f,0.0000f},     0.0000f,0.0000f,   0, {0.0000f,0.0000f,0.0000f,0.0000f},     {136.2408f,742.4620f,4046.1445f}, 25.0000f,22049.9980f },
    { "DrumSmasher",   {12.0000f,12.0000f,-12.0000f,4.5000f}, 4.0000f,-18.0000f, 0, {17.6667f,12.0000f,0.0000f,20.0000f},  {95.9302f,4046.1448f,6459.1743f}, 25.0000f,13028.0928f },
    { "AnalogDrums",   {12.0000f,12.0000f,12.0000f,12.0000f}, -2.0000f,-18.0000f,1, {20.0000f,13.0000f,13.0000f,20.0000f}, {1849.0653f,2937.9780f,4929.5034f}, 187.6291f,8975.2246f },
    { "DrumsOfDoom",   {12.0000f,12.0000f,0.0000f,7.0000f},   20.0000f,-17.0000f,1, {20.0000f,10.6667f,0.3333f,5.3333f},   {233.3102f,4073.7915f,5648.7095f}, 25.0000f,13877.5439f },
    { "DrumSqueeze",   {-12.0000f,12.0000f,12.0000f,-12.0000f},16.0000f,-12.0000f,2,{20.0000f,10.6667f,0.3333f,20.0000f},  {233.3102f,2118.8408f,5648.7095f}, 25.0000f,13877.5439f },
    { "Tightener",     {6.0000f,8.5000f,12.0000f,12.0000f},   1.0000f,-11.5000f, 2, {20.0000f,7.6667f,14.0000f,6.3333f},   {94.9665f,752.6427f,4929.5034f}, 59.8160f,13877.5439f },
    { "GrungeKord",    {12.0000f,12.0000f,12.0000f,12.0000f}, 20.0000f,-16.0000f,3, {0.0000f,0.0000f,0.0000f,-0.3333f},    {500.2085f,1043.6127f,2006.5029f}, 55.0791f,22050.0059f },
    { "Resofuzz",      {8.5000f,-12.0000f,8.5000f,12.0000f},  6.5000f,-11.5000f, 2, {10.3333f,0.0000f,-0.3333f,18.0000f},  {306.3555f,752.6427f,3754.1467f}, 59.8160f,6835.2344f },
    { "BigNoiseKord",  {-0.5000f,12.0000f,3.0000f,-12.0000f}, 20.0000f,-12.0000f,3, {1.3333f,16.6667f,18.3333f,-10.3333f}, {227.0411f,1043.6127f,3964.3296f}, 25.0000f,8048.7559f },
    { "Acoustifuzz",   {-0.5000f,12.0000f,-12.0000f,4.5000f}, -13.5000f,-1.5000f,3, {1.3333f,-8.6667f,0.3333f,20.0000f},   {100.2833f,1043.6127f,4929.5034f}, 25.0000f,13877.5439f },
    { "KordBright",    {-2.5000f,-7.5000f,12.0000f,6.0000f},  8.0000f,-7.5000f,  4, {-6.0000f,0.0000f,0.0000f,11.6667f},   {693.5876f,2006.5029f,4668.1509f}, 97.5887f,12445.0195f },
    { "ChordRez",      {12.0000f,-12.0000f,12.0000f,-12.0000f},17.5000f,-13.0000f,1,{6.6667f,0.0000f,5.3333f,0.0000f},     {402.2700f,862.4520f,1447.0701f}, 233.3102f,22050.0059f },
    { "PowerChord",    {12.0000f,12.0000f,-4.5000f,-12.0000f},15.0000f,-13.5000f,1, {20.0000f,14.0000f,5.0000f,0.3333f},   {290.1130f,1043.6127f,2118.8408f}, 59.8160f,22050.0059f },
    { "KordKrunch+Hi", {1.5000f,12.0000f,-4.5000f,5.5000f},   15.0000f,-9.5000f, 4, {15.6667f,-5.3333f,-12.3333f,8.3333f}, {200.6124f,773.4249f,4301.8701f}, 59.8160f,8975.2246f },
    { "Basic Lead",    {12.0000f,12.0000f,-8.5000f,-12.0000f},20.0000f,-7.0000f, 2, {9.3333f,20.0000f,4.0000f,-13.3333f},  {163.7398f,460.9603f,1570.2800f}, 78.4813f,6651.5732f },
    { "Cutting Lead",  {12.0000f,12.0000f,12.0000f,-12.0000f},20.0000f,-15.0000f,0, {-14.0000f,16.6667f,-8.0000f,16.3333f},{215.0038f,712.7391f,1613.6390f}, 59.8160f,4073.7915f },
    { "60s Lead",      {2.0000f,12.0000f,7.5000f,-12.0000f},  20.0000f,-11.5000f,1, {18.3333f,9.6667f,-5.0000f,-20.0000f}, {215.0038f,621.9919f,1613.6390f}, 92.4147f,6129.6660f },
};
} // namespace

juce::String QuadraFuzzAudioProcessor::getPresetName (int idx) const
{
    return (idx >= 0 && idx < (int) presetBank.size()) ? presetBank[(size_t) idx].name : juce::String();
}

void QuadraFuzzAudioProcessor::applyPreset (int idx)
{
    if (idx < 0 || idx >= (int) presetBank.size()) return;
    currentPreset = idx;
    const QFPreset& p = presetBank[(size_t) idx];

    // VST parameters (the 4 drive knobs + In/Out + Shape).
    for (int b = 0; b < 4; ++b)
        if (auto* prm = apvts.getParameter ("Band" + juce::String (b + 1)))
            prm->setValueNotifyingHost (prm->convertTo0to1 (p.band[b]));
    if (auto* prm = apvts.getParameter ("In"))    prm->setValueNotifyingHost (prm->convertTo0to1 (p.in));
    if (auto* prm = apvts.getParameter ("Out"))   prm->setValueNotifyingHost (prm->convertTo0to1 (p.out));
    if (auto* prm = apvts.getParameter ("Shape")) prm->setValueNotifyingHost (prm->convertTo0to1 ((float) p.shape));

    // EQ window (independent of the knobs): the 4 band LEVELS + 3 crossover freqs.
    for (int b = 0; b < NUM_BANDS; ++b) bandLevelDb[b].store (p.level[b]);
    for (int i = 0; i < NUM_CROSS; ++i) crossHz[i].store (p.cross[i]);
    edgeLoHz.store (p.edgeLo);
    edgeHiHz.store (p.edgeHi);

    if (auto* ed = getActiveEditor()) ed->repaint();
}

void QuadraFuzzAudioProcessor::captureCurrent (QFPreset& p) const
{
    for (int b = 0; b < 4; ++b)
        p.band[b] = apvts.getRawParameterValue ("Band" + juce::String (b + 1))->load();
    p.in    = apvts.getRawParameterValue ("In") ->load();
    p.out   = apvts.getRawParameterValue ("Out")->load();
    p.shape = (int) apvts.getRawParameterValue ("Shape")->load();
    for (int b = 0; b < NUM_BANDS; ++b) p.level[b] = bandLevelDb[b].load();
    for (int i = 0; i < NUM_CROSS; ++i) p.cross[i] = crossHz[i].load();
    p.edgeLo = edgeLoHz.load();
    p.edgeHi = edgeHiHz.load();
}

int QuadraFuzzAudioProcessor::createPreset (const juce::String& name)
{
    if ((int) presetBank.size() >= MAX_PRESETS) return -1;   // bank full (64)
    QFPreset p;
    p.name = name.isNotEmpty() ? name : ("Preset " + juce::String ((int) presetBank.size()));
    captureCurrent (p);                 // snapshot the current knobs + EQ
    presetBank.push_back (p);
    currentPreset = (int) presetBank.size() - 1;
    savePresetBank();
    return currentPreset;
}

void QuadraFuzzAudioProcessor::deletePreset (int idx)
{
    if (idx < 0 || idx >= (int) presetBank.size()) return;
    if (presetBank.size() <= 1) return;                       // always keep at least one
    presetBank.erase (presetBank.begin() + idx);
    if (currentPreset >= (int) presetBank.size()) currentPreset = (int) presetBank.size() - 1;
    savePresetBank();
}

juce::File QuadraFuzzAudioProcessor::getBankFile() const
{
    return juce::File::getSpecialLocation (juce::File::userApplicationDataDirectory)
              .getChildFile ("QuadraFuzz").getChildFile ("QuadraFuzz.presets");
}

void QuadraFuzzAudioProcessor::savePresetBank() const
{
    juce::ValueTree vt ("QuadraFuzzBank");
    vt.setProperty ("seedVersion", SEED_VERSION, nullptr);
    for (const auto& p : presetBank)
    {
        juce::ValueTree n ("Preset");
        n.setProperty ("name", p.name, nullptr);
        for (int b = 0; b < 4; ++b) n.setProperty ("band" + juce::String (b), p.band[b], nullptr);
        n.setProperty ("in",  p.in,  nullptr);
        n.setProperty ("out", p.out, nullptr);
        n.setProperty ("shape", p.shape, nullptr);
        for (int b = 0; b < 4; ++b) n.setProperty ("level" + juce::String (b), p.level[b], nullptr);
        for (int i = 0; i < 3; ++i) n.setProperty ("cross" + juce::String (i), p.cross[i], nullptr);
        n.setProperty ("edgeLo", p.edgeLo, nullptr);
        n.setProperty ("edgeHi", p.edgeHi, nullptr);
        vt.appendChild (n, nullptr);
    }
    auto f = getBankFile();
    f.getParentDirectory().createDirectory();
    if (auto xml = vt.createXml()) xml->writeTo (f);
}

void QuadraFuzzAudioProcessor::loadPresetBank()
{
    presetBank.clear();
    auto f = getBankFile();
    if (f.existsAsFile())
        if (auto xml = juce::XmlDocument::parse (f))
        {
            auto vt = juce::ValueTree::fromXml (*xml);
            // Older banks (no seedVersion, or < current) are discarded so the
            // corrected factory snapshot below re-seeds them once.
            if (vt.isValid() && (int) vt.getProperty ("seedVersion", 1) >= SEED_VERSION)
                for (int c = 0; c < vt.getNumChildren(); ++c)
                {
                    auto n = vt.getChild (c);
                    QFPreset p;
                    p.name  = n.getProperty ("name", "Preset").toString();
                    for (int b = 0; b < 4; ++b) p.band[b] = (float) n.getProperty ("band" + juce::String (b), 0.0);
                    p.in    = (float) n.getProperty ("in",  0.0);
                    p.out   = (float) n.getProperty ("out", 0.0);
                    p.shape = (int)   n.getProperty ("shape", 0);
                    for (int b = 0; b < 4; ++b) p.level[b] = (float) n.getProperty ("level" + juce::String (b), 0.0);
                    for (int i = 0; i < 3; ++i) p.cross[i] = (float) n.getProperty ("cross" + juce::String (i), (double) DEFAULT_CROSS[i]);
                    p.edgeLo = (float) n.getProperty ("edgeLo", (double) DEFAULT_EDGE_LO);
                    p.edgeHi = (float) n.getProperty ("edgeHi", (double) DEFAULT_EDGE_HI);
                    if ((int) presetBank.size() < MAX_PRESETS) presetBank.push_back (p);
                }
        }

    // First run (or empty/corrupt file): seed from the factory snapshot, then save.
    if (presetBank.empty())
    {
        for (const auto& s : FACTORY_SEED)
        {
            QFPreset p;
            p.name = s.name;
            for (int b = 0; b < 4; ++b) p.band[b]  = s.band[b];
            p.in = s.in; p.out = s.out; p.shape = s.shape;
            for (int b = 0; b < 4; ++b) p.level[b] = s.level[b];
            for (int i = 0; i < 3; ++i) p.cross[i] = s.cross[i];
            p.edgeLo = s.edgeLo; p.edgeHi = s.edgeHi;
            presetBank.push_back (p);
        }
        savePresetBank();
    }
    if (currentPreset < 0 || currentPreset >= (int) presetBank.size()) currentPreset = 0;
}

//==============================================================================
juce::AudioProcessorEditor* QuadraFuzzAudioProcessor::createEditor()
{
    return new QuadraFuzzAudioProcessorEditor (*this);
}

juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter()
{
    return new QuadraFuzzAudioProcessor();
}
