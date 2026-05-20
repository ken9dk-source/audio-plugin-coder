#include "PluginProcessor.h"
#include "PluginEditor.h"

using APVTS = juce::AudioProcessorValueTreeState;

// ============================================================
//  Parameter Layout
// ============================================================
APVTS::ParameterLayout SIDTranceAudioProcessor::createParameterLayout()
{
    std::vector<std::unique_ptr<juce::RangedAudioParameter>> params;

    // Helper lambdas
    auto addFloat = [&](const char* id, const char* name,
                        float lo, float hi, float def) {
        params.push_back(std::make_unique<juce::AudioParameterFloat>(
            juce::ParameterID{id, 1}, name,
            juce::NormalisableRange<float>(lo, hi), def));
    };
    auto addInt = [&](const char* id, const char* name, int lo, int hi, int def) {
        params.push_back(std::make_unique<juce::AudioParameterInt>(
            juce::ParameterID{id, 1}, name, lo, hi, def));
    };
    auto addBool = [&](const char* id, const char* name, bool def) {
        params.push_back(std::make_unique<juce::AudioParameterBool>(
            juce::ParameterID{id, 1}, name, def));
    };

    // ── OSC 1 ───────────────────────────────────────────────
    addInt  ("osc1_wave",    "OSC1 Wave",    0, 5,      0);
    addInt  ("osc1_semi",    "OSC1 Semi",  -24, 24,     0);
    addFloat("osc1_fine",    "OSC1 Fine", -100.0f, 100.0f, 0.0f);
    addFloat("osc1_pw",      "OSC1 PW",     0.05f, 0.95f, 0.5f);
    addFloat("osc1_attack",  "OSC1 A",      0.001f, 10.0f, 0.01f);
    addFloat("osc1_decay",   "OSC1 D",      0.001f, 10.0f, 0.3f);
    addFloat("osc1_sustain", "OSC1 S",      0.0f, 1.0f, 0.7f);
    addFloat("osc1_release", "OSC1 R",      0.001f, 20.0f, 0.5f);
    addFloat("osc1_volume",  "OSC1 Vol",    0.0f, 1.0f, 0.8f);

    // ── OSC 2 ───────────────────────────────────────────────
    addInt  ("osc2_wave",    "OSC2 Wave",   0, 5,       1);
    addInt  ("osc2_semi",    "OSC2 Semi", -24, 24,      0);
    addFloat("osc2_fine",    "OSC2 Fine", -100.0f, 100.0f, -7.0f);
    addFloat("osc2_pw",      "OSC2 PW",    0.05f, 0.95f, 0.5f);
    addFloat("osc2_attack",  "OSC2 A",     0.001f, 10.0f, 0.01f);
    addFloat("osc2_decay",   "OSC2 D",     0.001f, 10.0f, 0.3f);
    addFloat("osc2_sustain", "OSC2 S",     0.0f, 1.0f, 0.7f);
    addFloat("osc2_release", "OSC2 R",     0.001f, 20.0f, 0.5f);
    addFloat("osc2_volume",  "OSC2 Vol",   0.0f, 1.0f, 0.6f);

    // ── OSC 3 ───────────────────────────────────────────────
    addInt  ("osc3_wave",    "OSC3 Wave",   0, 5,       3); // NOISE
    addInt  ("osc3_semi",    "OSC3 Semi", -24, 24,      0);
    addFloat("osc3_fine",    "OSC3 Fine", -100.0f, 100.0f, 0.0f);
    addFloat("osc3_pw",      "OSC3 PW",    0.05f, 0.95f, 0.5f);
    addFloat("osc3_attack",  "OSC3 A",     0.001f, 10.0f, 0.001f);
    addFloat("osc3_decay",   "OSC3 D",     0.001f, 10.0f, 0.1f);
    addFloat("osc3_sustain", "OSC3 S",     0.0f, 1.0f, 0.0f);
    addFloat("osc3_release", "OSC3 R",     0.001f, 20.0f, 0.1f);
    addFloat("osc3_volume",  "OSC3 Vol",   0.0f, 1.0f, 0.3f);

    // ── Filter ──────────────────────────────────────────────
    addFloat("filter_cutoff",    "Cutoff",     20.0f, 18000.0f, 2480.0f);
    addFloat("filter_res",       "Resonance",  0.0f, 0.99f,    0.35f);
    addInt  ("filter_type",      "Filter Type",0, 3,            0);
    addBool ("filter_slope",     "Slope 24dB",                  true);
    addBool ("filter_keytrack",  "Key Track",                   false);
    addBool ("filter_velocity",  "Vel Sens",                    false);

    // ── Amplifier ───────────────────────────────────────────
    addFloat("amp_attack",  "Amp A",  0.001f, 10.0f, 0.005f);
    addFloat("amp_decay",   "Amp D",  0.001f, 10.0f, 0.3f);
    addFloat("amp_sustain", "Amp S",  0.0f, 1.0f,   0.8f);
    addFloat("amp_release", "Amp R",  0.001f, 20.0f, 0.4f);
    addFloat("amp_volume",  "Amp Vol",0.0f, 1.0f,   0.85f);

    // ── LFO 1 ───────────────────────────────────────────────
    addInt  ("lfo1_shape",  "LFO1 Shape", 0, 5, 0);
    addFloat("lfo1_rate",   "LFO1 Rate",  0.01f, 20.0f, 1.0f);
    addBool ("lfo1_sync",   "LFO1 Sync",  false);
    addBool ("lfo1_retrig", "LFO1 Retrig",true);

    // ── LFO 2 ───────────────────────────────────────────────
    addInt  ("lfo2_shape",  "LFO2 Shape", 0, 5, 1);
    addFloat("lfo2_rate",   "LFO2 Rate",  0.01f, 20.0f, 0.25f);
    addBool ("lfo2_sync",   "LFO2 Sync",  false);
    addBool ("lfo2_retrig", "LFO2 Retrig",false);

    // ── Mod Matrix (4 slots) ────────────────────────────────
    addInt  ("mod1_src", "Mod1 Src", 0, 5, 0); // 0=LFO1,1=LFO2,2=Vel,3=MW,4=AmpEnv,5=Key
    addFloat("mod1_amt", "Mod1 Amt",-1.0f, 1.0f, 0.375f);
    addInt  ("mod1_dst", "Mod1 Dst", 0, 7, 0); // 0=Cutoff,1=PW1,2=PW2,3=PW3,4=Amp,5=Res,6=Fine1,7=Fine2
    addInt  ("mod2_src", "Mod2 Src", 0, 5, 1);
    addFloat("mod2_amt", "Mod2 Amt",-1.0f, 1.0f, 0.25f);
    addInt  ("mod2_dst", "Mod2 Dst", 0, 7, 1);
    addInt  ("mod3_src", "Mod3 Src", 0, 5, 2);
    addFloat("mod3_amt", "Mod3 Amt",-1.0f, 1.0f, 0.5f);
    addInt  ("mod3_dst", "Mod3 Dst", 0, 7, 4);
    addInt  ("mod4_src", "Mod4 Src", 0, 5, 3);
    addFloat("mod4_amt", "Mod4 Amt",-1.0f, 1.0f, 0.4f);
    addInt  ("mod4_dst", "Mod4 Dst", 0, 7, 5);

    // ── Effects ─────────────────────────────────────────────
    addBool ("fx_chorus_on",    "Chorus On",   true);
    addFloat("fx_chorus_rate",  "Chorus Rate", 0.1f, 10.0f, 1.2f);
    addFloat("fx_chorus_depth", "Chorus Depth",0.0f, 1.0f, 0.4f);
    addFloat("fx_chorus_mix",   "Chorus Mix",  0.0f, 1.0f, 0.3f);
    addBool ("fx_delay_on",     "Delay On",    true);
    addInt  ("fx_delay_time",   "Delay Time",  0, 6,  2);
    addFloat("fx_delay_feedback","Delay FB",   0.0f, 0.95f, 0.35f);
    addFloat("fx_delay_mix",    "Delay Mix",   0.0f, 1.0f, 0.25f);
    addBool ("fx_reverb_on",    "Reverb On",   true);
    addFloat("fx_reverb_size",  "Reverb Size", 0.0f, 1.0f, 0.4f);
    addFloat("fx_reverb_damp",  "Reverb Damp", 0.0f, 1.0f, 0.6f);
    addFloat("fx_reverb_mix",   "Reverb Mix",  0.0f, 1.0f, 0.3f);

    // ── Master ──────────────────────────────────────────────
    addFloat("master_volume",  "Master Vol",  0.0f, 1.0f, 0.8f);
    addBool ("master_limiter", "Limiter",     true);
    addBool ("master_poly",    "Poly Mode",   true);
    addInt  ("master_voices",  "Voice Count", 1, 16, 16);

    // ── Arp / Sequencer ─────────────────────────────────────
    addBool ("arp_on",     "Arp On",     false);
    addInt  ("arp_mode",   "Arp Mode",   0, 4, 0);
    addInt  ("arp_octave", "Arp Octave", 1, 4, 1);
    addFloat("arp_gate",   "Arp Gate",   0.0f, 1.0f, 0.75f);
    addFloat("arp_swing",  "Arp Swing",  0.0f, 0.5f, 0.0f);
    addFloat("arp_tempo",  "Arp Tempo",  60.0f, 200.0f, 140.0f);
    addBool ("arp_sync",   "Arp Sync",   true);

    for (int i = 1; i <= 16; ++i) {
        const auto id   = juce::String::formatted("seq_step_%02d", i);
        const auto name = juce::String::formatted("Step %d", i);
        const bool def  = (i != 7 && i != 12 && i != 15);
        params.push_back(std::make_unique<juce::AudioParameterBool>(
            juce::ParameterID{id, 1}, name, def));
    }

    // ── SID Specials ────────────────────────────────────────
    addFloat("trance_drift",  "Trance Drift",  0.0f, 1.0f, 0.2f);
    addFloat("digital_age",   "Digital Age",   0.0f, 1.0f, 0.0f);
    addFloat("sid_width",     "SID Width",     0.0f, 1.0f, 0.5f);
    addFloat("analog_glow",   "Analog Glow",   0.0f, 1.0f, 0.25f);
    addInt  ("sid_mode",      "SID Mode",      0, 2,      2);

    return { params.begin(), params.end() };
}

// ============================================================
//  Constructor / Destructor
// ============================================================
SIDTranceAudioProcessor::SIDTranceAudioProcessor()
    : AudioProcessor(BusesProperties()
                        .withOutput("Output", juce::AudioChannelSet::stereo(), true)),
      apvts(*this, nullptr, "SIDTrance", createParameterLayout())
{
}

SIDTranceAudioProcessor::~SIDTranceAudioProcessor() = default;

// ============================================================
//  prepareToPlay
// ============================================================
void SIDTranceAudioProcessor::prepareToPlay(double sampleRate_, int samplesPerBlock)
{
    sr = sampleRate_;
    const float fsr = float(sr);

    // Reset all voices
    for (auto& v : voices) {
        v = SIDVoice{};
        v.filter.sr = fsr;
        v.sr = fsr;
    }

    // Smoothed values
    smCutoff.reset(sr, 0.02);
    smCutoff.setCurrentAndTargetValue(2480.0f);
    smRes.reset(sr, 0.01);
    smRes.setCurrentAndTargetValue(0.35f);
    smMasterVol.reset(sr, 0.05);
    smMasterVol.setCurrentAndTargetValue(0.8f);

    // LFOs
    lfo1.sr = fsr;
    lfo2.sr = fsr;

    // FX
    chorus.sr = fsr;
    chorus.reset();
    delay.reset();
    reverb.reset();

    // Limiter
    limEnvL = limEnvR = 0.0f;

    juce::ignoreUnused(samplesPerBlock);
}

// ============================================================
//  Bus layout
// ============================================================
bool SIDTranceAudioProcessor::isBusesLayoutSupported(const BusesLayout& layouts) const
{
    return layouts.getMainOutputChannelSet() == juce::AudioChannelSet::stereo();
}

// ============================================================
//  Voice allocation helpers
// ============================================================
int SIDTranceAudioProcessor::allocateVoice(int midiNote)
{
    const int maxV = std::clamp(
        int(apvts.getRawParameterValue("master_voices")->load()), 1, kMaxVoices);
    const bool poly = apvts.getRawParameterValue("master_poly")->load() > 0.5f;

    if (!poly) {
        // Mono: always use voice 0
        return 0;
    }

    // Find free voice
    for (int i = 0; i < maxV; ++i)
        if (!voices[i].active) return i;

    // Same note steal
    for (int i = 0; i < maxV; ++i)
        if (voices[i].note == midiNote) return i;

    // Oldest steal
    int oldest = 0, oldestAge = INT_MAX;
    for (int i = 0; i < maxV; ++i) {
        if (voices[i].age < oldestAge) { oldestAge = voices[i].age; oldest = i; }
    }
    return oldest;
}

void SIDTranceAudioProcessor::handleNoteOn(int, int note, float vel)
{
    const int vi = allocateVoice(note);
    auto& v = voices[vi];

    // Pull current osc settings from APVTS into each voice oscillator
    auto loadOscParams = [&](SIDOscillator& osc, int n) {
        const auto pfx = juce::String("osc") + juce::String(n) + "_";
        osc.wave   = SIDOscillator::Wave(int(apvts.getRawParameterValue(pfx + "wave")->load()));
        osc.semi   = int(apvts.getRawParameterValue(pfx + "semi")->load());
        osc.fine   = apvts.getRawParameterValue(pfx + "fine")->load();
        osc.pw     = apvts.getRawParameterValue(pfx + "pw")->load();
        osc.volume = apvts.getRawParameterValue(pfx + "volume")->load();
        osc.age    = apvts.getRawParameterValue("digital_age")->load();
        osc.drift  = apvts.getRawParameterValue("trance_drift")->load();
        const int mode = int(apvts.getRawParameterValue("sid_mode")->load());
        osc.aliased = (mode == 0); // Classic SID = aliased
    };
    loadOscParams(v.osc1, 1);
    loadOscParams(v.osc2, 2);
    loadOscParams(v.osc3, 3);

    // Per-osc envelopes
    auto loadEnv = [&](ADSREnv& env, int n) {
        const auto pfx = juce::String("osc") + juce::String(n) + "_";
        env.setParams(apvts.getRawParameterValue(pfx + "attack")->load(),
                      apvts.getRawParameterValue(pfx + "decay")->load(),
                      apvts.getRawParameterValue(pfx + "sustain")->load(),
                      apvts.getRawParameterValue(pfx + "release")->load(),
                      float(sr));
    };
    loadEnv(v.env1, 1); loadEnv(v.env2, 2); loadEnv(v.env3, 3);

    // Amp envelope
    v.ampEnv.setParams(apvts.getRawParameterValue("amp_attack")->load(),
                       apvts.getRawParameterValue("amp_decay")->load(),
                       apvts.getRawParameterValue("amp_sustain")->load(),
                       apvts.getRawParameterValue("amp_release")->load(),
                       float(sr));

    v.noteOn(note, vel, float(sr));
    v.age = ++voiceCounter;

    // LFO retrigger
    lfo1.retrigger();
    lfo2.retrigger();
}

void SIDTranceAudioProcessor::handleNoteOff(int, int note)
{
    for (auto& v : voices)
        if (v.active && v.note == note)
            v.noteOff();
}

// ============================================================
//  Mod matrix application (called each sample or each block)
// ============================================================
void SIDTranceAudioProcessor::applyModMatrix(SIDVoice& v,
                                              float lfo1v, float lfo2v,
                                              float vel, float mw)
{
    // Reset mod destinations
    v.modCutoff = v.modRes = v.modPW1 = v.modPW2 = v.modAmp = v.modFine1 = v.modFine2 = 0.0f;

    for (int slot = 1; slot <= 4; ++slot) {
        const auto pfx = juce::String("mod") + juce::String(slot) + "_";
        const int  src = int(apvts.getRawParameterValue(pfx + "src")->load());
        const float amt = apvts.getRawParameterValue(pfx + "amt")->load();
        const int  dst = int(apvts.getRawParameterValue(pfx + "dst")->load());

        float srcVal = 0.0f;
        switch (src) {
        case 0: srcVal = lfo1v; break;
        case 1: srcVal = lfo2v; break;
        case 2: srcVal = vel * 2.0f - 1.0f; break;
        case 3: srcVal = mw * 2.0f - 1.0f;  break;
        case 4: srcVal = v.ampEnv.value * 2.0f - 1.0f; break;
        case 5: srcVal = (float(v.note) - 60.0f) / 60.0f; break;
        }

        const float mod = srcVal * amt;
        switch (dst) {
        case 0: v.modCutoff += mod; break;
        case 1: v.modPW1    += mod; break;
        case 2: v.modPW2    += mod; break;
        case 3: /* OSC3 PW */      break;
        case 4: v.modAmp    += mod; break;
        case 5: v.modRes    += mod; break;
        case 6: v.modFine1  += mod * 100.0f; break;
        case 7: v.modFine2  += mod * 100.0f; break;
        }
    }
}

// ============================================================
//  ANALOG GLOW — soft saturation (tanh approx)
// ============================================================
float SIDTranceAudioProcessor::analogGlow(float x, float amount)
{
    if (amount < 0.001f) return x;
    const float drive = 1.0f + amount * 4.0f;
    return std::tanh(x * drive) / std::tanh(drive);
}

// ============================================================
//  Arp processing (generates MIDI into midiBuffer)
// ============================================================
void SIDTranceAudioProcessor::processArpEvents(juce::MidiBuffer& midi,
                                                int numSamples, float bpm)
{
    if (!arp.enabled || arp.held.empty()) return;

    const float tempo    = apvts.getRawParameterValue("arp_tempo")->load();
    const bool  syncDaw  = apvts.getRawParameterValue("arp_sync")->load() > 0.5f;
    const float useBpm   = syncDaw ? bpm : tempo;
    const float gate     = apvts.getRawParameterValue("arp_gate")->load();
    const float swing    = apvts.getRawParameterValue("arp_swing")->load();
    const int   octRange = int(apvts.getRawParameterValue("arp_octave")->load());
    const int   mode     = int(apvts.getRawParameterValue("arp_mode")->load());

    const float stepsPerBeat = 4.0f; // 16th notes per beat
    const float beatLen      = float(sr) * 60.0f / useBpm;
    const float stepLen      = beatLen / stepsPerBeat;

    // Find active steps
    std::vector<int> activeSteps;
    for (int i = 0; i < 16; ++i) {
        const auto id = juce::String::formatted("seq_step_%02d", i + 1);
        if (apvts.getRawParameterValue(id)->load() > 0.5f)
            activeSteps.push_back(i);
    }
    if (activeSteps.empty()) return;

    // Sort held notes
    auto sortedNotes = arp.held;
    std::sort(sortedNotes.begin(), sortedNotes.end());

    for (int samp = 0; samp < numSamples; ++samp) {
        arp.beatPhase += 1.0f;

        // Swing: odd steps are slightly longer
        const bool isOdd = (arp.stepIdx % 2 == 1);
        const float swingOff = isOdd ? stepLen * swing * 0.5f : 0.0f;
        const float curStepLen = stepLen + (isOdd ? swingOff : -swingOff);

        if (arp.beatPhase >= curStepLen) {
            arp.beatPhase -= curStepLen;

            // Note off for previous
            if (arp.currentNote >= 0) {
                midi.addEvent(juce::MidiMessage::noteOff(1, arp.currentNote), samp);
                arp.currentNote = -1;
            }

            // Advance step index into activeSteps
            const int nSteps = int(activeSteps.size());
            if (nSteps == 0) continue;

            int patIdx = arp.stepIdx % nSteps;
            // Choose note
            int noteIdx = 0;
            switch (mode) {
            case 0: noteIdx = patIdx; break;                      // Up
            case 1: noteIdx = nSteps - 1 - patIdx; break;         // Down
            case 2: // UpDown
                noteIdx = (arp.direction > 0) ? patIdx : (nSteps - 1 - patIdx);
                if (patIdx == nSteps - 1) arp.direction = -1;
                if (patIdx == 0)          arp.direction =  1;
                break;
            case 3: noteIdx = int(float(rand()) / RAND_MAX * nSteps); break; // Random
            case 4: noteIdx = patIdx; break; // Order (same as Up for now)
            }
            noteIdx = std::clamp(noteIdx, 0, int(sortedNotes.size()) - 1);

            const int octShift = (arp.stepIdx / nSteps) % octRange;
            const int noteToPlay = std::clamp(sortedNotes[noteIdx] + octShift * 12, 0, 127);

            midi.addEvent(juce::MidiMessage::noteOn(1, noteToPlay, uint8_t(100)), samp);
            arp.currentNote = noteToPlay;

            arp.stepIdx = (arp.stepIdx + 1) % (nSteps * octRange);
        }

        // Gate close
        if (arp.currentNote >= 0 && arp.beatPhase >= curStepLen * gate) {
            midi.addEvent(juce::MidiMessage::noteOff(1, arp.currentNote), samp);
            arp.currentNote = -1;
        }
    }
}

// ============================================================
//  processBlock — Main audio engine
// ============================================================
void SIDTranceAudioProcessor::processBlock(juce::AudioBuffer<float>& buffer,
                                            juce::MidiBuffer& midiMessages)
{
    juce::ScopedNoDenormals noDenormals;

    // Clear output
    buffer.clear();

    const int numSamples  = buffer.getNumSamples();
    float* outL = buffer.getWritePointer(0);
    float* outR = buffer.getWritePointer(1);

    // ── Read parameters ──────────────────────────────────────
    const float baseCutoff = apvts.getRawParameterValue("filter_cutoff")->load();
    const float baseRes    = apvts.getRawParameterValue("filter_res")->load();
    const int   filterType = int(apvts.getRawParameterValue("filter_type")->load());
    const bool  slope24    = apvts.getRawParameterValue("filter_slope")->load() > 0.5f;
    const bool  keyTrack   = apvts.getRawParameterValue("filter_keytrack")->load() > 0.5f;
    const bool  velSens    = apvts.getRawParameterValue("filter_velocity")->load() > 0.5f;

    const float ampVol     = apvts.getRawParameterValue("amp_volume")->load();
    const float masterVol  = apvts.getRawParameterValue("master_volume")->load();
    const bool  limiterOn  = apvts.getRawParameterValue("master_limiter")->load() > 0.5f;

    const float sidWidth   = apvts.getRawParameterValue("sid_width")->load();
    const float glowAmt    = apvts.getRawParameterValue("analog_glow")->load();

    smCutoff.setTargetValue(baseCutoff);
    smRes.setTargetValue(baseRes);
    smMasterVol.setTargetValue(masterVol);

    // LFO config
    lfo1.shape  = LFOEngine::Shape(int(apvts.getRawParameterValue("lfo1_shape")->load()));
    lfo1.rate   = apvts.getRawParameterValue("lfo1_rate")->load();
    lfo1.syncOn = apvts.getRawParameterValue("lfo1_sync")->load() > 0.5f;
    lfo1.retrig = apvts.getRawParameterValue("lfo1_retrig")->load() > 0.5f;
    lfo2.shape  = LFOEngine::Shape(int(apvts.getRawParameterValue("lfo2_shape")->load()));
    lfo2.rate   = apvts.getRawParameterValue("lfo2_rate")->load();
    lfo2.syncOn = apvts.getRawParameterValue("lfo2_sync")->load() > 0.5f;
    lfo2.retrig = apvts.getRawParameterValue("lfo2_retrig")->load() > 0.5f;

    // Arp config
    arp.enabled = apvts.getRawParameterValue("arp_on")->load() > 0.5f;

    // Chorus config
    chorus.enabled = apvts.getRawParameterValue("fx_chorus_on")->load() > 0.5f;
    chorus.rate    = apvts.getRawParameterValue("fx_chorus_rate")->load();
    chorus.depth   = apvts.getRawParameterValue("fx_chorus_depth")->load();
    chorus.mix     = apvts.getRawParameterValue("fx_chorus_mix")->load();

    // Delay config
    delay.enabled  = apvts.getRawParameterValue("fx_delay_on")->load() > 0.5f;
    delay.feedback = apvts.getRawParameterValue("fx_delay_feedback")->load();
    delay.mix      = apvts.getRawParameterValue("fx_delay_mix")->load();

    // Reverb config
    reverb.roomSize = apvts.getRawParameterValue("fx_reverb_size")->load();
    reverb.damp     = apvts.getRawParameterValue("fx_reverb_damp")->load();
    reverb.wet      = apvts.getRawParameterValue("fx_reverb_on")->load() > 0.5f
                    ? apvts.getRawParameterValue("fx_reverb_mix")->load() : 0.0f;

    // Host BPM
    float hostBpm = 140.0f;
    if (auto* ph = getPlayHead()) {
        juce::AudioPlayHead::CurrentPositionInfo info;
        if (ph->getCurrentPosition(info))
            hostBpm = float(info.bpm);
    }

    // Delay tempo sync
    if (delay.enabled) {
        const int divIdx = int(apvts.getRawParameterValue("fx_delay_time")->load());
        delay.setTempoDivision(divIdx, hostBpm, float(sr));
    }

    // ── Arp: inject MIDI events ──────────────────────────────
    if (arp.enabled) processArpEvents(midiMessages, numSamples, hostBpm);

    // ── MIDI event processing ────────────────────────────────
    int midiEventPos = 0;
    auto midiIt = midiMessages.cbegin();
    const auto midiEnd = midiMessages.cend();

    // ── Per-sample synthesis loop ────────────────────────────
    for (int samp = 0; samp < numSamples; ++samp) {
        // Process MIDI events up to this sample
        while (midiIt != midiEnd && (*midiIt).samplePosition <= samp) {
            const auto& msg = (*midiIt).getMessage();
            if (msg.isNoteOn(true))
                handleNoteOn(msg.getChannel(), msg.getNoteNumber(),
                             msg.getVelocity() / 127.0f);
            else if (msg.isNoteOff())
                handleNoteOff(msg.getChannel(), msg.getNoteNumber());
            else if (msg.isController() && msg.getControllerNumber() == 1)
                modWheelVal = msg.getControllerValue() / 127.0f;
            ++midiIt;
        }

        // LFO outputs
        const float l1 = lfo1.process(hostBpm);
        const float l2 = lfo2.process(hostBpm);

        // Smooth filter params
        const float smCut = smCutoff.getNextValue();
        const float smR   = smRes.getNextValue();
        const float smVol = smMasterVol.getNextValue();

        // Sum all active voices
        float monoSum = 0.0f;
        for (auto& v : voices) {
            if (!v.active) continue;

            // Apply mod matrix
            applyModMatrix(v, l1, l2, v.velocity, modWheelVal);

            // Set per-voice filter params
            v.filter.type   = filterType;
            v.filter.slope24= slope24;
            v.filter.sr     = float(sr);

            monoSum += v.processMono(smCut, smR,
                                     keyTrack ? 1.0f : 0.0f,
                                     velSens  ? 1.0f : 0.0f);
        }

        monoSum *= ampVol;

        // ── ANALOG GLOW (saturation) ─────────────────────────
        monoSum = analogGlow(monoSum, glowAmt);

        // ── Stereo spread via SID WIDTH ──────────────────────
        // Simple pseudo-stereo: L/R slightly offset (Haas-effect)
        // The real unison spread is achieved via detuned voice copies
        // For now: mid/side spread proportional to sid_width
        const float spreadL = monoSum * (1.0f + sidWidth * 0.2f);
        const float spreadR = monoSum * (1.0f - sidWidth * 0.2f);

        float L = spreadL;
        float R = spreadR;

        // ── FX Chain ─────────────────────────────────────────
        chorus.process(L, R);
        delay.process(L, R);
        reverb.process(L, R);

        // ── Master volume ─────────────────────────────────────
        L *= smVol;
        R *= smVol;

        // ── Limiter (peak envelope follower + gain reduction) ─
        if (limiterOn) {
            const float peakL = std::abs(L);
            const float peakR = std::abs(R);
            const float attLim = 0.99f;
            const float relLim = 0.999f;
            limEnvL = (peakL > limEnvL) ? peakL * attLim + limEnvL * (1.0f - attLim) : limEnvL * relLim;
            limEnvR = (peakR > limEnvR) ? peakR * attLim + limEnvR * (1.0f - attLim) : limEnvR * relLim;
            if (limEnvL > 1.0f) L /= limEnvL;
            if (limEnvR > 1.0f) R /= limEnvR;
        }

        outL[samp] = L;
        outR[samp] = R;
    }

    // ── Push to scope FIFO (for oscilloscope display) ────────
    {
        const int toWrite = std::min(numSamples, scopeFifo.getFreeSpace());
        if (toWrite > 0) {
            int start1, size1, start2, size2;
            scopeFifo.prepareToWrite(toWrite, start1, size1, start2, size2);
            if (size1 > 0) {
                scopeBuf.copyFrom(0, start1, buffer, 0, 0, size1);
                scopeBuf.copyFrom(1, start1, buffer, 1, 0, size1);
            }
            if (size2 > 0) {
                scopeBuf.copyFrom(0, start2, buffer, 0, size1, size2);
                scopeBuf.copyFrom(1, start2, buffer, 1, size1, size2);
            }
            scopeFifo.finishedWrite(toWrite);
        }
    }

    // Clear MIDI messages (arp output already injected)
    midiMessages.clear();
}

// ============================================================
//  State: save / load
// ============================================================
void SIDTranceAudioProcessor::getStateInformation(juce::MemoryBlock& destData)
{
    auto state = apvts.copyState();
    if (auto xml = state.createXml())
        copyXmlToBinary(*xml, destData);
}

void SIDTranceAudioProcessor::setStateInformation(const void* data, int sizeInBytes)
{
    if (auto xml = getXmlFromBinary(data, sizeInBytes))
        if (xml->hasTagName(apvts.state.getType()))
            apvts.replaceState(juce::ValueTree::fromXml(*xml));
}

// ============================================================
//  Editor
// ============================================================
juce::AudioProcessorEditor* SIDTranceAudioProcessor::createEditor()
{
    return new SIDTranceAudioProcessorEditor(*this);
}

// ============================================================
//  Plugin entry point
// ============================================================
juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter()
{
    return new SIDTranceAudioProcessor();
}
