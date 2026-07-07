#include "PluginProcessor.h"
#if ! MIDBASS_HEADLESS
 #include "PluginEditor.h"
#endif

MidBassAudioProcessor::MidBassAudioProcessor()
    : AudioProcessor (BusesProperties().withOutput ("Output", juce::AudioChannelSet::stereo(), true)),
      apvts (*this, nullptr, "MidBass", mb::createParameterLayout())
{
}

void MidBassAudioProcessor::prepareToPlay (double sampleRate, int)
{
    voice.prepare (sampleRate);
    sat.prepare (sampleRate);
    eq.prepare (sampleRate);
    trans.prepare (sampleRate);
    heldNotes.clearQuick();
    updateVoiceParams();
    setLatencySamples (mb::MbSaturator::kLatency);   // constant 47-sample OS alignment delay
}

bool MidBassAudioProcessor::isBusesLayoutSupported (const BusesLayout& layouts) const
{
    return layouts.getMainOutputChannelSet() == juce::AudioChannelSet::stereo();
}

// Pull the APVTS values used by Phases 1-3 into the voice (once per block).
// LFOs/matrix arrive in Phase 4; saturation/EQ/transient in Phase 5; FX in 6.
void MidBassAudioProcessor::updateVoiceParams()
{
    namespace pid = mb::pid;
    mb::MbVoice::Params p;
    auto& c = p.oscCfg;

    const char* waveIds[3]  = { pid::osc1_wave,  pid::osc2_wave,  pid::osc3_wave };
    const char* octIds[3]   = { pid::osc1_oct,   pid::osc2_oct,   pid::osc3_oct };
    const char* semiIds[3]  = { pid::osc1_semi,  pid::osc2_semi,  pid::osc3_semi };
    const char* fineIds[3]  = { pid::osc1_fine,  pid::osc2_fine,  pid::osc3_fine };
    const char* pwIds[3]    = { pid::osc1_pw,    pid::osc2_pw,    pid::osc3_pw };
    const char* levelIds[3] = { pid::osc1_level, pid::osc2_level, pid::osc3_level };
    for (int i = 0; i < 3; ++i)
    {
        c.osc[i].wave      = (int) pRaw (waveIds[i])->load();
        c.osc[i].oct       = (int) pRaw (octIds[i])->load();
        c.osc[i].semi      = (int) pRaw (semiIds[i])->load();
        c.osc[i].fineCents = pRaw (fineIds[i])->load();
        c.osc[i].pw        = pRaw (pwIds[i])->load() * 0.01f;
        c.osc[i].level     = pRaw (levelIds[i])->load() * 0.01f;
    }
    c.sync       = pRaw (pid::osc_sync)->load() > 0.5f;
    c.fm         = pRaw (pid::osc_fm)->load() * 0.01f;
    c.ring       = pRaw (pid::osc_ring)->load() * 0.01f;
    c.drift      = pRaw (pid::osc_drift)->load() * 0.01f;
    c.subOn      = pRaw (pid::sub_on)->load() > 0.5f;
    c.subWave    = (int) pRaw (pid::sub_wave)->load();
    c.subOctDown = 1 + (int) pRaw (pid::sub_oct)->load();
    c.subLevel   = pRaw (pid::sub_level)->load() * 0.01f;
    c.uniVoices  = (int) pRaw (pid::uni_voices)->load();
    c.uniDetune  = pRaw (pid::uni_detune)->load() * 0.01f;
    c.uniSpread  = pRaw (pid::uni_spread)->load() * 0.01f;
    c.uniMono    = pRaw (pid::uni_mono)->load() > 0.5f;

    p.stack           = (int) pRaw (pid::voice_stack)->load();
    p.voiceMode       = (int) pRaw (pid::voice_mode)->load();
    p.glideMs         = pRaw (pid::glide_time)->load();
    p.glideLegatoOnly = pRaw (pid::glide_legato)->load() > 0.5f;
    p.bendRange       = (int) pRaw (pid::bend_range)->load();

    p.fltMode   = (int) pRaw (pid::flt_mode)->load();
    p.cutoffHz  = pRaw (pid::flt_cutoff)->load();
    p.reso      = pRaw (pid::flt_reso)->load() * 0.01f;
    p.keytrack  = pRaw (pid::flt_keytrack)->load() * 0.01f;
    p.envAmt    = pRaw (pid::flt_env_amt)->load() * 0.01f;
    p.drivePre  = pRaw (pid::flt_drive_pre)->load() * 0.01f;
    p.drivePost = pRaw (pid::flt_drive_post)->load() * 0.01f;

    p.fA = pRaw (pid::fenv_a)->load(); p.fD = pRaw (pid::fenv_d)->load();
    p.fS = pRaw (pid::fenv_s)->load(); p.fR = pRaw (pid::fenv_r)->load();
    p.aA = pRaw (pid::aenv_a)->load(); p.aD = pRaw (pid::aenv_d)->load();
    p.aS = pRaw (pid::aenv_s)->load(); p.aR = pRaw (pid::aenv_r)->load();

    p.outGain = juce::Decibels::decibelsToGain (pRaw (pid::output)->load());

    // ---- LFOs (Phase 4) ----
    struct LfoIds { const char* wave; const char* sync; const char* hz; const char* div; const char* amt; const char* ret; const char* dest; };
    const LfoIds lfoIds[2] = {
        { pid::lfo1_wave, pid::lfo1_sync, pid::lfo1_rate_hz, pid::lfo1_rate_div, pid::lfo1_amount, pid::lfo1_retrig, pid::lfo1_dest },
        { pid::lfo2_wave, pid::lfo2_sync, pid::lfo2_rate_hz, pid::lfo2_rate_div, pid::lfo2_amount, pid::lfo2_retrig, pid::lfo2_dest } };
    for (int i = 0; i < 2; ++i)
    {
        p.lfo[i].wave    = (int) pRaw (lfoIds[i].wave)->load();
        p.lfo[i].sync    = pRaw (lfoIds[i].sync)->load() > 0.5f;
        p.lfo[i].rateHz  = pRaw (lfoIds[i].hz)->load();
        p.lfo[i].rateDiv = (int) pRaw (lfoIds[i].div)->load();
        p.lfo[i].amount  = pRaw (lfoIds[i].amt)->load() * 0.01f;
        p.lfo[i].retrig  = pRaw (lfoIds[i].ret)->load() > 0.5f;
        p.lfo[i].dest    = (int) pRaw (lfoIds[i].dest)->load();
    }

    // ---- mod matrix ----
    const char* srcIds[6] = { pid::mod1_src, pid::mod2_src, pid::mod3_src, pid::mod4_src, pid::mod5_src, pid::mod6_src };
    const char* dstIds[6] = { pid::mod1_dst, pid::mod2_dst, pid::mod3_dst, pid::mod4_dst, pid::mod5_dst, pid::mod6_dst };
    const char* amtIds[6] = { pid::mod1_amt, pid::mod2_amt, pid::mod3_amt, pid::mod4_amt, pid::mod5_amt, pid::mod6_amt };
    for (int i = 0; i < 6; ++i)
    {
        p.matrix.slot[i].src = (int) pRaw (srcIds[i])->load();
        p.matrix.slot[i].dst = (int) pRaw (dstIds[i])->load();
        p.matrix.slot[i].amt = pRaw (amtIds[i])->load() * 0.01f;
    }

    p.bpm = curBpm;
    voice.setParams (p);

    // ---- tone chain (Phase 5) ----
    sat.setParams ((int) pRaw (pid::sat_type)->load(),
                   pRaw (pid::sat_drive)->load() * 0.01f,
                   pRaw (pid::sat_mix)->load() * 0.01f);
    eq.setParams (pRaw (pid::eq_ls_freq)->load(),  pRaw (pid::eq_ls_gain)->load(),
                  pRaw (pid::eq_mid_freq)->load(), pRaw (pid::eq_mid_gain)->load(),
                  pRaw (pid::eq_mid_q)->load(),
                  pRaw (pid::eq_hs_freq)->load(),  pRaw (pid::eq_hs_gain)->load());
    trans.setParams (pRaw (pid::trans_attack)->load() * 0.01f,
                     pRaw (pid::trans_sustain)->load() * 0.01f);
}

void MidBassAudioProcessor::handleMidiEvent (const juce::MidiMessage& m)
{
    if (m.isNoteOn())
    {
        const int note = m.getNoteNumber();
        const bool overlapping = ! heldNotes.isEmpty();
        heldNotes.removeAllInstancesOf (note);
        heldNotes.add (note);
        voice.noteOn (note, m.getFloatVelocity(), overlapping);
    }
    else if (m.isNoteOff())
    {
        const int note = m.getNoteNumber();
        heldNotes.removeAllInstancesOf (note);
        if (note == voice.curNote)
        {
            if (! heldNotes.isEmpty())
                voice.noteOn (heldNotes.getLast(), 1.0f, true);   // fall back to held note, legato
            else
                voice.noteOff();
        }
    }
    else if (m.isPitchWheel())
    {
        voice.setPitchBend ((float) (m.getPitchWheelValue() - 8192) / 8192.0f);
    }
    else if (m.isController() && m.getControllerNumber() == 1)
    {
        voice.modWheel = (float) m.getControllerValue() / 127.0f;
    }
    else if (m.isChannelPressure())
    {
        voice.aftertouch = (float) m.getChannelPressureValue() / 127.0f;
    }
    else if (m.isAftertouch())
    {
        voice.aftertouch = (float) m.getAfterTouchValue() / 127.0f;
    }
    else if (m.isAllNotesOff() || m.isAllSoundOff())
    {
        heldNotes.clearQuick();
        voice.noteOff();
    }
}

void MidBassAudioProcessor::processBlock (juce::AudioBuffer<float>& buffer, juce::MidiBuffer& midi)
{
    juce::ScopedNoDenormals noDenormals;
    keyboardState.processNextMidiBuffer (midi, 0, buffer.getNumSamples(), true);
    buffer.clear();

    // Host tempo, re-read every block so synced LFOs track BPM changes mid-note.
    if (auto* ph = getPlayHead())
        if (auto pos = ph->getPosition())
            if (pos->getBpm().hasValue() && *pos->getBpm() > 0.0)
                curBpm = *pos->getBpm();

    updateVoiceParams();

    const int n = buffer.getNumSamples();
    float* outL = buffer.getWritePointer (0);
    float* outR = buffer.getNumChannels() > 1 ? buffer.getWritePointer (1) : outL;

    auto it = midi.begin();
    for (int i = 0; i < n; ++i)
    {
        while (it != midi.end() && (*it).samplePosition <= i)
        {
            handleMidiEvent ((*it).getMessage());
            ++it;
        }
        float l = 0.0f, r = 0.0f;
        if (voice.isActive())
            voice.process (l, r);

        // tone chain runs on the full stream (delay lines/EQ tails need it)
        sat.processSample (l, r);
        eq.processSample (l, r);
        trans.processSample (l, r);
        outL[i] = l; outR[i] = r;
    }
}

bool MidBassAudioProcessor::hasEditor() const
{
   #if MIDBASS_HEADLESS
    return false;
   #else
    return true;
   #endif
}

juce::AudioProcessorEditor* MidBassAudioProcessor::createEditor()
{
   #if MIDBASS_HEADLESS
    return nullptr;
   #else
    return new MidBassAudioProcessorEditor (*this);
   #endif
}

void MidBassAudioProcessor::getStateInformation (juce::MemoryBlock& destData)
{
    if (auto xml = apvts.copyState().createXml())
        copyXmlToBinary (*xml, destData);
}

void MidBassAudioProcessor::setStateInformation (const void* data, int sizeInBytes)
{
    if (auto xml = getXmlFromBinary (data, sizeInBytes))
        if (xml->hasTagName (apvts.state.getType()))
            apvts.replaceState (juce::ValueTree::fromXml (*xml));
}

juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter()
{
    return new MidBassAudioProcessor();
}
