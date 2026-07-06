#include "PluginProcessor.h"
#include "PluginEditor.h"

//==============================================================================
// Mode templates — straight from the PDF "Anbefalede EQ-bånd" table.
// Band roles are fixed across modes (b0 low-cut … b4 air, b5 spare) so the
// Focus-Mode macros always map to the same band index.
//==============================================================================
using FT = teq::FilterType;

static const std::array<BandPreset, ParameterIDs::kNumBands> kLead = {{
    { true,  FT::HPF,        120.0f,  0.0f, 0.70f, false },
    { true,  FT::LowShelf,   300.0f,  3.0f, 0.70f, false },
    { true,  FT::Peak,      2000.0f,  3.5f, 0.90f, true  },
    { true,  FT::Peak,      4500.0f,  3.0f, 0.70f, true  },
    { true,  FT::HighShelf, 12000.0f, 1.5f, 0.70f, false },
    { false, FT::Peak,       800.0f,  0.0f, 1.00f, false },
}};

static const std::array<BandPreset, ParameterIDs::kNumBands> kPluck = {{
    { true,  FT::HPF,        175.0f,  0.0f, 0.70f, false },
    { true,  FT::Peak,       450.0f,  0.0f, 0.70f, false },
    { true,  FT::Peak,      6000.0f,  3.5f, 0.80f, true  },
    { true,  FT::Peak,      2500.0f,  1.5f, 0.90f, true  },
    { true,  FT::HighShelf, 12000.0f, 1.5f, 0.70f, false },
    { false, FT::Peak,       900.0f,  0.0f, 1.00f, false },
}};

static const std::array<BandPreset, ParameterIDs::kNumBands> kMidbass = {{
    { true,  FT::HPF,         30.0f,  0.0f, 0.70f, false },
    { true,  FT::Peak,       150.0f,  4.0f, 0.80f, true  },
    { true,  FT::Peak,       320.0f, -1.5f, 0.70f, false },
    { true,  FT::HighShelf, 3500.0f,  1.0f, 0.70f, false },
    { false, FT::LPF,       8000.0f,  0.0f, 0.70f, false },
    { false, FT::Peak,       250.0f,  0.0f, 1.00f, false },
}};

const std::array<BandPreset, ParameterIDs::kNumBands>& TranceEQAudioProcessor::presetFor (int modeIndex)
{
    switch (modeIndex) { case 1: return kPluck; case 2: return kMidbass; default: return kLead; }
}

//==============================================================================
TranceEQAudioProcessor::TranceEQAudioProcessor()
    : AudioProcessor (BusesProperties()
          .withInput  ("Input",  juce::AudioChannelSet::stereo(), true)
          .withOutput ("Output", juce::AudioChannelSet::stereo(), true)),
      apvts (*this, nullptr, "PARAMETERS", createParameterLayout())
{
    heldNotes.reserve (128);
    for (int v = 0; v < kNumViz; ++v) vizBins[v].store (-120.0f);
    cacheParamPointers();
}

TranceEQAudioProcessor::~TranceEQAudioProcessor() {}

//==============================================================================
juce::AudioProcessorValueTreeState::ParameterLayout TranceEQAudioProcessor::createParameterLayout()
{
    using namespace juce;
    AudioProcessorValueTreeState::ParameterLayout layout;

    const StringArray typeChoices { "High-Pass", "Low Shelf", "Peak", "Notch", "High Shelf", "Low-Pass" };

    // ---- global ----
    layout.add (std::make_unique<AudioParameterChoice> (ParameterID { ParameterIDs::mode, 1 }, "Mode",
        StringArray { "Lead", "Pluck", "Midbass" }, 0));
    layout.add (std::make_unique<AudioParameterBool>   (ParameterID { ParameterIDs::bypass, 1 }, "Bypass", false));

    {
        NormalisableRange<float> r (-24.0f, 24.0f, 0.1f);
        layout.add (std::make_unique<AudioParameterFloat> (ParameterID { ParameterIDs::output, 1 }, "Output", r, 0.0f));
    }
    layout.add (std::make_unique<AudioParameterBool>   (ParameterID { ParameterIDs::kt_on, 1 }, "Key-Track", false));
    layout.add (std::make_unique<AudioParameterChoice> (ParameterID { ParameterIDs::kt_src, 1 }, "Track Source",
        StringArray { "Audio", "MIDI" }, 0));
    layout.add (std::make_unique<AudioParameterFloat>  (ParameterID { ParameterIDs::kt_amt, 1 }, "Track Amount",
        NormalisableRange<float> (0.0f, 1.0f), 1.0f));
    {
        NormalisableRange<float> r (27.5f, 880.0f); r.setSkewForCentre (110.0f);
        layout.add (std::make_unique<AudioParameterFloat> (ParameterID { ParameterIDs::kt_ref, 1 }, "Track Ref", r, 110.0f));
    }
    layout.add (std::make_unique<AudioParameterChoice> (ParameterID { ParameterIDs::os, 1 }, "Oversample",
        StringArray { "Off", "2x", "4x" }, 0));

    // ---- 6 bands, defaults from the Lead template ----
    const auto& def = presetFor (0);
    for (int b = 0; b < ParameterIDs::kNumBands; ++b)
    {
        layout.add (std::make_unique<AudioParameterBool> (ParameterID { ParameterIDs::bandOn (b), 1 },
            "Band " + String (b + 1) + " On", def[(size_t) b].on));
        layout.add (std::make_unique<AudioParameterChoice> (ParameterID { ParameterIDs::bandType (b), 1 },
            "Band " + String (b + 1) + " Type", typeChoices, (int) def[(size_t) b].type));

        NormalisableRange<float> fr (20.0f, 20000.0f); fr.setSkewForCentre (1000.0f);
        layout.add (std::make_unique<AudioParameterFloat> (ParameterID { ParameterIDs::bandFreq (b), 1 },
            "Band " + String (b + 1) + " Freq", fr, def[(size_t) b].freq));

        layout.add (std::make_unique<AudioParameterFloat> (ParameterID { ParameterIDs::bandGain (b), 1 },
            "Band " + String (b + 1) + " Gain", NormalisableRange<float> (-18.0f, 18.0f, 0.1f), def[(size_t) b].gain));

        NormalisableRange<float> qr (0.1f, 18.0f); qr.setSkewForCentre (0.9f);
        layout.add (std::make_unique<AudioParameterFloat> (ParameterID { ParameterIDs::bandQ (b), 1 },
            "Band " + String (b + 1) + " Q", qr, def[(size_t) b].q));

        layout.add (std::make_unique<AudioParameterBool> (ParameterID { ParameterIDs::bandTrack (b), 1 },
            "Band " + String (b + 1) + " Track", def[(size_t) b].track));
    }
    return layout;
}

void TranceEQAudioProcessor::cacheParamPointers()
{
    auto* raw = &apvts;
    pMode   = raw->getRawParameterValue (ParameterIDs::mode);
    pBypass = raw->getRawParameterValue (ParameterIDs::bypass);
    pOutput = raw->getRawParameterValue (ParameterIDs::output);
    pKtOn   = raw->getRawParameterValue (ParameterIDs::kt_on);
    pKtSrc  = raw->getRawParameterValue (ParameterIDs::kt_src);
    pKtAmt  = raw->getRawParameterValue (ParameterIDs::kt_amt);
    pKtRef  = raw->getRawParameterValue (ParameterIDs::kt_ref);
    pOS     = raw->getRawParameterValue (ParameterIDs::os);
    for (int b = 0; b < ParameterIDs::kNumBands; ++b)
    {
        bp[b].on    = raw->getRawParameterValue (ParameterIDs::bandOn (b));
        bp[b].type  = raw->getRawParameterValue (ParameterIDs::bandType (b));
        bp[b].freq  = raw->getRawParameterValue (ParameterIDs::bandFreq (b));
        bp[b].gain  = raw->getRawParameterValue (ParameterIDs::bandGain (b));
        bp[b].q     = raw->getRawParameterValue (ParameterIDs::bandQ (b));
        bp[b].track = raw->getRawParameterValue (ParameterIDs::bandTrack (b));
    }
}

void TranceEQAudioProcessor::applyModePreset (int modeIndex)
{
    const auto& preset = presetFor (modeIndex);
    auto setP = [this] (const juce::String& id, float v)
    {
        if (auto* p = apvts.getParameter (id))
        { p->beginChangeGesture(); p->setValueNotifyingHost (p->convertTo0to1 (v)); p->endChangeGesture(); }
    };

    if (auto* p = apvts.getParameter (ParameterIDs::mode))
    { p->beginChangeGesture(); p->setValueNotifyingHost (p->convertTo0to1 ((float) modeIndex)); p->endChangeGesture(); }

    for (int b = 0; b < ParameterIDs::kNumBands; ++b)
    {
        setP (ParameterIDs::bandOn (b),    preset[(size_t) b].on ? 1.0f : 0.0f);
        setP (ParameterIDs::bandType (b),  (float) (int) preset[(size_t) b].type);
        setP (ParameterIDs::bandFreq (b),  preset[(size_t) b].freq);
        setP (ParameterIDs::bandGain (b),  preset[(size_t) b].gain);
        setP (ParameterIDs::bandQ (b),     preset[(size_t) b].q);
        setP (ParameterIDs::bandTrack (b), preset[(size_t) b].track ? 1.0f : 0.0f);
    }
}

//==============================================================================
void TranceEQAudioProcessor::prepareToPlay (double sampleRate, int samplesPerBlock)
{
    sr = sampleRate;
    for (int ch = 0; ch < 2; ++ch)
        for (int b = 0; b < ParameterIDs::kNumBands; ++b)
            bands[ch][b].reset();

    pitch.prepare (sampleRate);
    ktRatioSmoothed = 1.0;
    for (int b = 0; b < ParameterIDs::kNumBands; ++b) bandActivePrev[b] = false;

    outGain.reset (sampleRate, 0.02);
    outGain.setCurrentAndTargetValue (juce::Decibels::decibelsToGain (pOutput->load()));

    // Oversampling: half-band polyphase IIR (low latency); biquads run inside the up/down.
    // Built for the *actual* negotiated channel count so a mono host can't make the oversampler
    // read past the block (it indexes channels [0, numChannels)).
    using OS = juce::dsp::Oversampling<float>;
    const size_t osCh = (size_t) juce::jmax (1, getTotalNumOutputChannels());
    osMaxBlock = juce::jmax (1, samplesPerBlock);
    os2x = std::make_unique<OS> (osCh, (size_t) 1, OS::filterHalfBandPolyphaseIIR, true, true);
    os4x = std::make_unique<OS> (osCh, (size_t) 2, OS::filterHalfBandPolyphaseIIR, true, true);
    os2x->initProcessing ((size_t) osMaxBlock);
    os4x->initProcessing ((size_t) osMaxBlock);

    // Report latency here (not on the first processBlock) so hosts that query right after
    // prepareToPlay get the correct value; processBlock only re-syncs when the factor changes.
    const int osSelInit = juce::jlimit (0, 2, (int) pOS->load());
    reportedLatency = osSelInit == 1 ? (int) std::round (os2x->getLatencyInSamples())
                    : osSelInit == 2 ? (int) std::round (os4x->getLatencyInSamples()) : 0;
    setLatencySamples (reportedLatency);
    currentOS = osSelInit;

    // Dry delay line for latency-compensated bypass (sized for the largest factor's latency).
    maxLatency  = (int) std::ceil (os4x->getLatencyInSamples());
    dryRingSize = maxLatency + osMaxBlock + 2;
    for (auto& r : dryRing) r.assign ((size_t) dryRingSize, 0.0f);
    dryW = 0;
    dryScratch.setSize (2, osMaxBlock, false, false, true);
    dryScratch.clear();
    bypassMix.reset (sampleRate, 0.015);                 // ~15 ms click-free crossfade
    bypassMix.setCurrentAndTargetValue (pBypass->load() > 0.5f ? 1.0f : 0.0f);

    monoBuf.assign ((size_t) osMaxBlock, 0.0f);

    fifo.fill (0.0f); fftData.fill (0.0f); fifoIndex = 0;
    for (int i = 0; i < kFftSize; ++i)
        hann[(size_t) i] = 0.5f - 0.5f * std::cos (2.0f * juce::MathConstants<float>::pi * (float) i / (float) (kFftSize - 1));
    for (int v = 0; v < kNumViz; ++v) vizBins[v].store (-120.0f);
}

bool TranceEQAudioProcessor::isBusesLayoutSupported (const BusesLayout& layouts) const
{
    const auto& in  = layouts.getMainInputChannelSet();
    const auto& out = layouts.getMainOutputChannelSet();
    if (out != juce::AudioChannelSet::stereo() && out != juce::AudioChannelSet::mono())
        return false;
    return in == out;
}

void TranceEQAudioProcessor::processBlock (juce::AudioBuffer<float>& buffer, juce::MidiBuffer& midi)
{
    juce::ScopedNoDenormals noDenormals;

    const int numCh = juce::jmin (2, buffer.getNumChannels());
    const int n     = buffer.getNumSamples();
    float* chans[2] { nullptr, nullptr };
    for (int ch = 0; ch < numCh; ++ch) chans[ch] = buffer.getWritePointer (ch);

    // ---- 1. track held MIDI notes (capacity-bounded, no realloc on the audio thread) ----
    for (const auto meta : midi)
    {
        const auto m = meta.getMessage();
        if (m.isNoteOn())
        {
            if ((int) heldNotes.size() >= 128) heldNotes.erase (heldNotes.begin());
            heldNotes.push_back (m.getNoteNumber());
        }
        else if (m.isNoteOff())
        {
            const int note = m.getNoteNumber();
            for (int i = (int) heldNotes.size() - 1; i >= 0; --i)
                if (heldNotes[(size_t) i] == note) { heldNotes.erase (heldNotes.begin() + i); break; }
        }
        else if (m.isAllNotesOff() || m.isAllSoundOff())
            heldNotes.clear();
    }

    // ---- 2. read params ----
    const bool  bypass = pBypass->load() > 0.5f;
    const bool  ktOn   = pKtOn->load() > 0.5f;
    const int   ktSrc  = (int) pKtSrc->load();        // 0 = Audio, 1 = MIDI
    const float ktAmt  = pKtAmt->load();
    const float ktRef  = pKtRef->load();

    // ---- 3. audio key-track: feed the detector with the *pre-EQ* mono mix ----
    if (ktOn && ktSrc == 0 && numCh > 0)
    {
        const int m = juce::jmin (n, (int) monoBuf.size());
        for (int i = 0; i < m; ++i)
            monoBuf[(size_t) i] = (numCh > 1) ? 0.5f * (chans[0][i] + chans[1][i]) : chans[0][i];
        pitch.pushBlock (monoBuf.data(), m);
    }

    // ---- 4. resolve the detected fundamental ----
    double f0 = 0.0;
    if (ktSrc == 1) { if (! heldNotes.empty()) f0 = 440.0 * std::pow (2.0, (heldNotes.back() - 69) / 12.0); }
    else            f0 = pitch.getF0();
    currentF0.store ((float) f0);

    double targetRatio = (ktOn && f0 > 0.0 && ktRef > 0.0) ? (f0 / (double) ktRef) : 1.0;
    targetRatio = juce::jlimit (0.03125, 32.0, targetRatio);          // ±5 octaves
    // Block-size-independent glide (~50 ms time constant) so tracking feels the same at any buffer size.
    const double glide = 1.0 - std::exp (-(double) juce::jmax (1, n) / (0.05 * sr));
    ktRatioSmoothed += (targetRatio - ktRatioSmoothed) * glide;

    // ---- 5. oversampling selection (latency + state sync only when the factor changes) ----
    const int osSel  = juce::jlimit (0, 2, (int) pOS->load());
    const int factor = (osSel == 1) ? 2 : (osSel == 2) ? 4 : 1;
    if (osSel != currentOS)
    {
        currentOS = osSel;
        double lat = 0.0;
        if      (osSel == 1 && os2x) lat = os2x->getLatencyInSamples();
        else if (osSel == 2 && os4x) lat = os4x->getLatencyInSamples();
        reportedLatency = (int) std::round (lat);
        setLatencySamples (reportedLatency);
        if (os2x) os2x->reset();
        if (os4x) os4x->reset();
        for (int ch = 0; ch < 2; ++ch)
            for (int b = 0; b < ParameterIDs::kNumBands; ++b) bands[ch][b].reset();
    }
    const bool   osRun   = (factor > 1) && (n <= osMaxBlock);
    const double procRate = osRun ? sr * (double) factor : sr;   // biquads run at this rate

    // ---- 6. (re)compute biquad coefficients for the active bands (at the processing rate) ----
    int active[ParameterIDs::kNumBands]; int numActive = 0;
    for (int b = 0; b < ParameterIDs::kNumBands; ++b)
    {
        const bool on = bp[b].on->load() > 0.5f;
        if (on && ! bandActivePrev[b]) { bands[0][b].reset(); bands[1][b].reset(); }   // clear stale state
        bandActivePrev[b] = on;
        if (! on) continue;

        const auto  type = (teq::FilterType) (int) bp[b].type->load();
        float       freq = bp[b].freq->load();
        const float gain = bp[b].gain->load();
        const float q    = bp[b].q->load();
        if (ktOn && bp[b].track->load() > 0.5f)
            freq = (float) juce::jlimit (20.0, 20000.0, (double) freq * std::pow (ktRatioSmoothed, (double) ktAmt));

        const auto c = teq::makeCoeffs (type, freq, gain, q, procRate);
        bands[0][b].setCoeffs (c);
        bands[1][b].setCoeffs (c);
        active[numActive++] = b;
    }

    // ---- 7a. capture the dry input (delayed by the reported latency) for the bypass crossfade ----
    // Done before the EQ touches the buffer; both wet and dry end up delayed by the same amount,
    // so crossfading them is sample-aligned and a bypassed signal stays time-aligned in the host.
    {
        int w = dryW;
        for (int i = 0; i < n; ++i)
        {
            const int rp = (w - reportedLatency + dryRingSize) % dryRingSize;
            for (int ch = 0; ch < numCh; ++ch)
            {
                dryRing[ch][(size_t) w] = chans[ch][i];
                dryScratch.setSample (ch, i, dryRing[ch][(size_t) rp]);
            }
            w = (w + 1) % dryRingSize;
        }
        dryW = w;
    }

    // ---- 7b. EQ — run the biquad chain (always, so the bypass crossfade has a wet path),
    //          oversampled when requested ----
    if (osRun)
    {
        juce::dsp::AudioBlock<float> block (buffer);
        auto& os = (factor == 2) ? *os2x : *os4x;
        auto osBlock = os.processSamplesUp (block);                 // up-sample (always, for stable latency)
        if (numActive > 0)
        {
            const int upN  = (int) osBlock.getNumSamples();
            const int upCh = juce::jmin (2, (int) osBlock.getNumChannels());
            for (int ch = 0; ch < upCh; ++ch)
            {
                auto* d = osBlock.getChannelPointer ((size_t) ch);
                for (int i = 0; i < upN; ++i)
                {
                    float x = d[i];
                    for (int a = 0; a < numActive; ++a) x = bands[ch][active[a]].process (x);
                    d[i] = x;
                }
            }
        }
        os.processSamplesDown (block);                              // down-sample back into buffer
    }
    else if (numActive > 0)
    {
        for (int ch = 0; ch < numCh; ++ch)
            for (int i = 0; i < n; ++i)
            {
                float x = chans[ch][i];
                for (int a = 0; a < numActive; ++a) x = bands[ch][active[a]].process (x);
                chans[ch][i] = x;
            }
    }

    // ---- 8. de-zippered output trim + click-free, latency-compensated bypass crossfade ----
    bypassMix.setTargetValue (bypass ? 1.0f : 0.0f);               // 0 = wet, 1 = dry
    outGain.setTargetValue (juce::Decibels::decibelsToGain (pOutput->load()));
    for (int i = 0; i < n; ++i)
    {
        const float mix = bypassMix.getNextValue();
        const float g   = outGain.getNextValue();                  // wet-only output trim
        float monoOut = 0.0f;
        for (int ch = 0; ch < numCh; ++ch)
        {
            const float wet = chans[ch][i] * g;
            const float dry = dryScratch.getSample (ch, i);
            const float out = wet + mix * (dry - wet);             // lerp wet → dry, no click
            chans[ch][i] = out;
            monoOut += out;
        }
        if (numCh > 0) monoOut /= (float) numCh;
        pushSampleToFifo (monoOut);
    }
}

//==============================================================================
void TranceEQAudioProcessor::pushSampleToFifo (float s) noexcept
{
    if (fifoIndex == kFftSize) { computeSpectrum(); fifoIndex = 0; }
    fifo[(size_t) fifoIndex++] = s;
}

void TranceEQAudioProcessor::computeSpectrum() noexcept
{
    for (int i = 0; i < kFftSize; ++i) fftData[(size_t) i] = fifo[(size_t) i] * hann[(size_t) i];
    std::fill (fftData.begin() + kFftSize, fftData.end(), 0.0f);
    fft.performFrequencyOnlyForwardTransform (fftData.data());

    const float  norm  = 4.0f / (float) kFftSize;     // ~0 dB for a full-scale sine
    const double binHz = sr / (double) kFftSize;
    for (int v = 0; v < kNumViz; ++v)
    {
        const double fa = 20.0 * std::pow (1000.0, (double)  v      / (double) (kNumViz - 1));
        const double fb = 20.0 * std::pow (1000.0, (double) (v + 1) / (double) (kNumViz - 1));
        int k0 = juce::jmax (1, (int) std::floor (fa / binHz));
        int k1 = juce::jmax (k0 + 1, (int) std::ceil (fb / binHz));
        k1 = juce::jmin (k1, kFftSize / 2);

        float mag = 0.0f;
        for (int k = k0; k < k1; ++k) mag = juce::jmax (mag, fftData[(size_t) k]);
        vizBins[v].store (juce::Decibels::gainToDecibels (mag * norm, -120.0f));
    }
}

//==============================================================================
void TranceEQAudioProcessor::getStateInformation (juce::MemoryBlock& dest)
{
    if (auto state = apvts.copyState(); state.isValid())
        if (auto xml = state.createXml())
            copyXmlToBinary (*xml, dest);
}

void TranceEQAudioProcessor::setStateInformation (const void* data, int size)
{
    if (auto xml = getXmlFromBinary (data, size))
        apvts.replaceState (juce::ValueTree::fromXml (*xml));
}

juce::AudioProcessorEditor* TranceEQAudioProcessor::createEditor()
{
    return new TranceEQAudioProcessorEditor (*this);
}

juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter()
{
    return new TranceEQAudioProcessor();
}
