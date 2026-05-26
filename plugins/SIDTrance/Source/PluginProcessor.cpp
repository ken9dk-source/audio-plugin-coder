#include "PluginProcessor.h"
#include "PluginEditor.h"

#include <cstring>

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
    // Time-based parameters (attack / decay / release / glide) use a
    // logarithmic skew so that the trance "click" sweet spot (≈ 50-200 ms)
    // sits near the middle of the knob travel instead of in the bottom 5 %.
    //   skew = 0.3 → ¼ knob ≈ 4 % of range, ½ knob ≈ 10 % of range,
    //                ¾ knob ≈ 40 % of range.  Matches analog hardware curves.
    auto addFloatLog = [&](const char* id, const char* name,
                           float lo, float hi, float def, float skew = 0.3f) {
        params.push_back(std::make_unique<juce::AudioParameterFloat>(
            juce::ParameterID{id, 1}, name,
            juce::NormalisableRange<float>(lo, hi, 0.0f, skew), def));
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
    addInt  ("osc1_wave",    "OSC1 Wave",    0, 6,      0);  // 0-5 = original, 6 = Hypersaw
    addInt  ("osc1_semi",    "OSC1 Semi",  -24, 24,     0);
    addFloat("osc1_fine",    "OSC1 Fine", -100.0f, 100.0f, 0.0f);
    addFloat("osc1_pw",      "OSC1 PW",     0.05f, 0.95f, 0.5f);
    addFloatLog("osc1_attack",  "OSC1 A",   0.001f, 10.0f, 0.01f);
    addFloatLog("osc1_decay",   "OSC1 D",   0.001f, 10.0f, 0.3f);
    addFloat   ("osc1_sustain", "OSC1 S",   0.0f, 1.0f, 0.7f);
    addFloatLog("osc1_release", "OSC1 R",   0.001f, 20.0f, 0.5f);
    addFloat("osc1_volume",  "OSC1 Vol",    0.0f, 1.0f, 0.8f);

    // ── OSC 2 ───────────────────────────────────────────────
    addInt  ("osc2_wave",    "OSC2 Wave",   0, 6,       1);  // 6 = Hypersaw
    addInt  ("osc2_semi",    "OSC2 Semi", -24, 24,      0);
    addFloat("osc2_fine",    "OSC2 Fine", -100.0f, 100.0f, -7.0f);
    addFloat("osc2_pw",      "OSC2 PW",    0.05f, 0.95f, 0.5f);
    addFloatLog("osc2_attack",  "OSC2 A",  0.001f, 10.0f, 0.01f);
    addFloatLog("osc2_decay",   "OSC2 D",  0.001f, 10.0f, 0.3f);
    addFloat   ("osc2_sustain", "OSC2 S",  0.0f, 1.0f, 0.7f);
    addFloatLog("osc2_release", "OSC2 R",  0.001f, 20.0f, 0.5f);
    addFloat("osc2_volume",  "OSC2 Vol",   0.0f, 1.0f, 0.6f);

    // ── OSC 3 ───────────────────────────────────────────────
    addInt  ("osc3_wave",    "OSC3 Wave",   0, 6,       3); // NOISE; 6 = Hypersaw
    addInt  ("osc3_semi",    "OSC3 Semi", -24, 24,      0);
    addFloat("osc3_fine",    "OSC3 Fine", -100.0f, 100.0f, 0.0f);
    addFloat("osc3_pw",      "OSC3 PW",    0.05f, 0.95f, 0.5f);
    addFloatLog("osc3_attack",  "OSC3 A",  0.001f, 10.0f, 0.001f);
    addFloatLog("osc3_decay",   "OSC3 D",  0.001f, 10.0f, 0.1f);
    addFloat   ("osc3_sustain", "OSC3 S",  0.0f, 1.0f, 0.0f);
    addFloatLog("osc3_release", "OSC3 R",  0.001f, 20.0f, 0.1f);
    addFloat("osc3_volume",  "OSC3 Vol",   0.0f, 1.0f, 0.3f);

    // ── Filter ──────────────────────────────────────────────
    addFloat("filter_cutoff",    "Cutoff",     20.0f, 18000.0f, 1200.0f); // start lower for filter sweep room
    addFloat("filter_res",       "Resonance",  0.0f, 0.99f,    0.55f);   // more resonance for click character
    addInt  ("filter_type",      "Filter Type",0, 3,            0);
    addBool ("filter_slope",     "Slope 24dB",                  true);
    addBool ("filter_keytrack",  "Key Track",                   false);
    addBool ("filter_velocity",  "Vel Sens",                    false);
    addFloatLog("filter_env_attack",  "Filt Env A", 0.001f, 4.0f,  0.005f); // 5ms: faster click
    addFloatLog("filter_env_decay",   "Filt Env D", 0.001f, 4.0f,  0.15f);  // snappier default
    addFloat   ("filter_env_sustain", "Filt Env S", 0.0f,   1.0f,  0.1f);   // lower sustain
    addFloatLog("filter_env_release", "Filt Env R", 0.001f, 4.0f,  0.4f);
    addFloat   ("filter_env_amount",  "Filt Env Amt",-1.0f, 1.0f,  0.7f);   // stronger click out of the box

    // ── Amplifier ───────────────────────────────────────────
    addFloatLog("amp_attack",  "Amp A", 0.001f, 10.0f, 0.005f);
    addFloatLog("amp_decay",   "Amp D", 0.001f, 10.0f, 0.3f);
    addFloat   ("amp_sustain", "Amp S", 0.0f, 1.0f,    0.8f);
    addFloatLog("amp_release", "Amp R", 0.001f, 20.0f, 0.4f);
    addFloat("amp_volume",  "Amp Vol",0.0f, 1.0f,   0.85f);

    // ── LFO 1 ───────────────────────────────────────────────
    addBool ("lfo1_on",     "LFO1 On",    true);
    addInt  ("lfo1_shape",  "LFO1 Shape", 0, 6, 0);   // 0=Sine..5=S&H..6=Step
    addFloat("lfo1_rate",   "LFO1 Rate",  0.01f, 20.0f, 1.0f);
    addBool ("lfo1_sync",   "LFO1 Sync",  false);
    addInt  ("lfo1_div",    "LFO1 Div",   0, 12, 3); // 0-7=straight, 8-10=triplets, 11-12=dotted
    addBool ("lfo1_retrig", "LFO1 Retrig",true);
    // 16 step-curve bars for LFO1 (active when shape == STEP)
    for (int i = 1; i <= 16; ++i) {
        char id[24], nm[32];
        snprintf(id, sizeof(id), "lfo1_step_%02d", i);
        snprintf(nm, sizeof(nm), "LFO1 Step %d",   i);
        addFloat(id, nm, -1.0f, 1.0f, 0.0f);
    }

    // ── LFO 2 ───────────────────────────────────────────────
    addBool ("lfo2_on",     "LFO2 On",    true);
    addInt  ("lfo2_shape",  "LFO2 Shape", 0, 6, 1);   // 0=Sine..5=S&H..6=Step
    addFloat("lfo2_rate",   "LFO2 Rate",  0.01f, 20.0f, 0.25f);
    addBool ("lfo2_sync",   "LFO2 Sync",  false);
    addInt  ("lfo2_div",    "LFO2 Div",   0, 12, 3); // 0-7=straight, 8-10=triplets, 11-12=dotted
    addBool ("lfo2_retrig", "LFO2 Retrig",false);
    // 16 step-curve bars for LFO2 (active when shape == STEP)
    for (int i = 1; i <= 16; ++i) {
        char id[24], nm[32];
        snprintf(id, sizeof(id), "lfo2_step_%02d", i);
        snprintf(nm, sizeof(nm), "LFO2 Step %d",   i);
        addFloat(id, nm, -1.0f, 1.0f, 0.0f);
    }

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
    addBool ("arp_on",     "Arp On",     true);    // ON by default
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

    // Dual-engine chip selector (replaces the old sid_mode int 0..2 — value 1
    // was DSP-identical to value 2, so we collapse to a clean 2-way choice).
    // Default = 1 (8580) preserves the prior default character (non-aliased).
    params.push_back(std::make_unique<juce::AudioParameterChoice>(
        juce::ParameterID{"chip_model", 1}, "Chip Model",
        juce::StringArray{"6581", "8580"}, 1));

    // ── Unison / Detune ─────────────────────────────────────
    addInt  ("unison_voices",  "Unison Voices", 1, 16,     1);
    addFloat("unison_detune",  "Unison Detune", 0.0f, 100.0f, 0.0f);
    addFloat("unison_spread",  "Stereo Spread", 0.0f, 1.0f,   0.7f);

    // ── Glide / Portamento ──────────────────────────────────
    addInt     ("glide_mode",   "Glide Mode",  0, 2,    0);  // 0=Off, 1=Auto, 2=Always
    addFloatLog("glide_time",   "Glide Time",  0.001f, 4.0f, 0.15f);

    // ── OSC Sync & FM ───────────────────────────────────────
    addBool ("osc_sync",     "OSC Sync",   false);  // OSC3 hard-syncs OSC1+2
    addFloat("osc_fm_amt",   "FM Amount",  0.0f, 1.0f, 0.0f);

    // ── Drive / Saturation ──────────────────────────────────
    addInt  ("drive_type",   "Drive Type", 0, 3,    0);  // 0=Tube, 1=Digital, 2=Bitcrush, 3=SIDGrit
    addFloat("drive_amount", "Drive Amt",  0.0f, 1.0f, 0.0f);

    // ── Trance Gate ─────────────────────────────────────────
    addBool ("gate_on",    "Gate On",    false);
    addFloat("gate_swing", "Gate Swing", 0.0f, 0.5f, 0.0f);
    for (int i = 1; i <= 16; ++i) {
        const auto id   = juce::String::formatted("gate_step_%02d", i);
        const auto name = juce::String::formatted("GStep %d", i);
        params.push_back(std::make_unique<juce::AudioParameterFloat>(
            juce::ParameterID{id, 1}, name,
            juce::NormalisableRange<float>(0.0f, 1.0f), 1.0f));
    }

    // ── Macro Controls ──────────────────────────────────────
    addFloat("macro1", "Brightness", 0.0f, 1.0f, 0.5f);
    addFloat("macro2", "Motion",     0.0f, 1.0f, 0.5f);
    addFloat("macro3", "Space",      0.0f, 1.0f, 0.5f);
    addFloat("macro4", "Drive",      0.0f, 1.0f, 0.0f);

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
    reverb.prepare(sr);   // scales Freeverb comb/allpass lengths to actual SR

    // Limiter
    limEnvL = limEnvR = 0.0f;

    // Cache raw-parameter pointers so the audio thread avoids String/HashMap
    // overhead inside processBlock and processArpEvents.
    for (int i = 0; i < 16; ++i) {
        char id[20];
        snprintf(id, sizeof(id), "seq_step_%02d", i + 1);
        seqStepParams_[i] = apvts.getRawParameterValue(id);
    }
    for (int s = 0; s < 4; ++s) {
        char buf[24];
        snprintf(buf, sizeof(buf), "mod%d_src", s + 1); modSrcParams_[s] = apvts.getRawParameterValue(buf);
        snprintf(buf, sizeof(buf), "mod%d_amt", s + 1); modAmtParams_[s] = apvts.getRawParameterValue(buf);
        snprintf(buf, sizeof(buf), "mod%d_dst", s + 1); modDstParams_[s] = apvts.getRawParameterValue(buf);
    }
    for (int i = 0; i < 16; ++i) {
        char id[24];
        snprintf(id, sizeof(id), "gate_step_%02d", i + 1);
        gateStepParams_[i] = apvts.getRawParameterValue(id);
    }
    for (int i = 0; i < 16; ++i) {
        char id[24];
        snprintf(id, sizeof(id), "lfo1_step_%02d", i + 1);
        lfo1StepParams_[i] = apvts.getRawParameterValue(id);
        snprintf(id, sizeof(id), "lfo2_step_%02d", i + 1);
        lfo2StepParams_[i] = apvts.getRawParameterValue(id);
    }
    // Per-osc param cache: eliminates juce::String heap alloc on audio thread note-on path
    static const char* kOscNames[3] = { "osc1", "osc2", "osc3" };
    for (int n = 0; n < 3; ++n) {
        char buf[32];
        auto cache = [&](const char* suffix) -> std::atomic<float>* {
            snprintf(buf, sizeof(buf), "%s_%s", kOscNames[n], suffix);
            return apvts.getRawParameterValue(buf);
        };
        oscWaveParam_[n] = cache("wave");
        oscSemiParam_[n] = cache("semi");
        oscFineParam_[n] = cache("fine");
        oscPwParam_[n]   = cache("pw");
        oscVolParam_[n]  = cache("volume");
        oscAtkParam_[n]  = cache("attack");
        oscDecParam_[n]  = cache("decay");
        oscSusParam_[n]  = cache("sustain");
        oscRelParam_[n]  = cache("release");
    }

    masterVoicesParam_ = apvts.getRawParameterValue("master_voices");
    masterPolyParam_   = apvts.getRawParameterValue("master_poly");

    // Note-on / arp param cache — eliminates String alloc + HashMap lookup on audio thread
    digitalAgeParam_   = apvts.getRawParameterValue("digital_age");
    tranceDriftParam_  = apvts.getRawParameterValue("trance_drift");
    chipModelParam_    = apvts.getRawParameterValue("chip_model");
    uniVoicesParam_    = apvts.getRawParameterValue("unison_voices");
    uniDetuneParam_    = apvts.getRawParameterValue("unison_detune");
    uniSpreadParam_    = apvts.getRawParameterValue("unison_spread");
    glideModeParam_    = apvts.getRawParameterValue("glide_mode");
    ampAtkParam_       = apvts.getRawParameterValue("amp_attack");
    ampDecParam_       = apvts.getRawParameterValue("amp_decay");
    ampSusParam_       = apvts.getRawParameterValue("amp_sustain");
    ampRelParam_       = apvts.getRawParameterValue("amp_release");
    fenvAtkParam_      = apvts.getRawParameterValue("filter_env_attack");
    fenvDecParam_      = apvts.getRawParameterValue("filter_env_decay");
    fenvSusParam_      = apvts.getRawParameterValue("filter_env_sustain");
    fenvRelParam_      = apvts.getRawParameterValue("filter_env_release");
    arpTempoParam_     = apvts.getRawParameterValue("arp_tempo");
    arpSyncParam_      = apvts.getRawParameterValue("arp_sync");
    arpGateParam_      = apvts.getRawParameterValue("arp_gate");
    arpSwingParam_     = apvts.getRawParameterValue("arp_swing");
    arpOctaveParam_    = apvts.getRawParameterValue("arp_octave");
    arpModeParam_      = apvts.getRawParameterValue("arp_mode");

    // Reset gate, glide, and sustain state
    gate.reset();
    lastPlayedNote_   = -1;
    glideOffsetAccum_ = 0.0f;
    sustainHeld_      = false;
    sustainedCount_   = 0;
    pitchBendSemis_   = 0.0f;

    // Pre-size per-osc scope scratch buffers to host's block size so the
    // audio thread never reallocates.  Cleared each block in processBlock.
    for (int o = 0; o < 3; ++o) {
        oscScopeScratch_[o].assign(std::size_t(std::max(samplesPerBlock, 1)), 0.0f);
        oscScope[o].fifo.reset();
    }

    // Chip-model crossfade: 30 ms ramp absorbs the discrete parameter change
    // without any click/pop.  Snap to current param value on prepareToPlay so
    // there's no audible ramp on first audio block after a state load.
    chipModelSmoothed_.reset(sr, 0.030);
    if (chipModelParam_ != nullptr)
        chipModelSmoothed_.setCurrentAndTargetValue(
            juce::jlimit(0.0f, 1.0f, chipModelParam_->load()));
    else
        chipModelSmoothed_.setCurrentAndTargetValue(1.0f);   // default = 8580
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
int SIDTranceAudioProcessor::allocateVoice(int midiNote, const int* excluded, int nExcluded)
{
    // Use cached pointers — no HashMap lookup on audio thread
    const int  maxV = (masterVoicesParam_ != nullptr)
                    ? std::clamp(int(std::round(masterVoicesParam_->load())), 1, kMaxVoices)
                    : kMaxVoices;
    const bool poly = (masterPolyParam_ != nullptr)
                    ? masterPolyParam_->load() > 0.5f
                    : true;

    if (!poly) {
        // Mono: always use voice 0
        return 0;
    }

    // Helper: is this voice in the exclusion list?
    auto isExcluded = [&](int idx) {
        for (int j = 0; j < nExcluded; ++j)
            if (excluded[j] == idx) return true;
        return false;
    };

    // 1. Find a free voice that isn't already used by this unison note-on
    for (int i = 0; i < maxV; ++i)
        if (!voices[i].active && !isExcluded(i)) return i;

    // 2. Same-note steal — skip excluded voices so successive unison voices
    //    don't all land on the one that was just activated for this note.
    if (nExcluded == 0) {
        for (int i = 0; i < maxV; ++i)
            if (voices[i].note == midiNote) return i;
    }

    // 3. Oldest steal (prefer non-excluded; fall back to excluded if no choice)
    int oldest = -1, oldestAge = INT_MAX;
    for (int i = 0; i < maxV; ++i) {
        if (!isExcluded(i) && voices[i].age < oldestAge) {
            oldestAge = voices[i].age; oldest = i;
        }
    }
    if (oldest >= 0) return oldest;

    // Absolute fallback: every candidate was excluded — steal oldest regardless
    oldest = 0; oldestAge = INT_MAX;
    for (int i = 0; i < maxV; ++i)
        if (voices[i].age < oldestAge) { oldestAge = voices[i].age; oldest = i; }
    return oldest;
}

void SIDTranceAudioProcessor::handleNoteOn(int, int note, float vel)
{
    // ── Read shared params via pre-cached pointers (no String alloc, no HashMap lookup) ─
    if (digitalAgeParam_ == nullptr) return;  // guard: cache not populated before prepareToPlay
    const float digitalAge  = digitalAgeParam_->load();
    const float tranceDrift = tranceDriftParam_->load();
    // Chip blend at note-on time = current smoothed value (so a voice spawned
    // mid-crossfade picks up the current blend).  Per-sample updates in
    // processBlock keep it tracking the smoother for the rest of the note.
    const float initialChipBlend = chipModelSmoothed_.getCurrentValue();

    // Unison parameters
    const int   uniCount  = std::clamp(int(uniVoicesParam_->load()), 1, kMaxUnison);
    const float uniDetune = uniDetuneParam_->load();  // cents
    const float uniSpread = uniSpreadParam_->load();  // 0..1

    // Glide parameters
    const int   glideMode = int(glideModeParam_->load());  // 0=Off,1=Auto,2=Always
    const bool  doGlide   = (glideMode == 2) || (glideMode == 1 && lastPlayedNote_ >= 0);
    const float glideInterval = doGlide ? float(lastPlayedNote_ - note) : 0.0f;

    // Guard: parameter cache not populated until after first prepareToPlay.
    if (oscWaveParam_[0] == nullptr) return;  // (digitalAgeParam_ guard above fires first)

    // Helper: load osc parameters — uses pre-cached pointers (no String alloc, no HashMap lookup)
    auto loadOscParams = [&](SIDOscillator& osc, int n) {
        const int idx = n - 1;  // osc1→0, osc2→1, osc3→2
        osc.wave    = SIDOscillator::Wave(std::clamp(int(std::round(oscWaveParam_[idx]->load())), 0, 6));
        osc.semi    = int(std::round(oscSemiParam_[idx]->load()));
        osc.fine    = oscFineParam_[idx]->load();
        osc.pw      = oscPwParam_[idx]->load();
        osc.volume  = oscVolParam_[idx]->load();
        osc.updateAge(digitalAge);  // precomputes qCache for the bitcrusher (avoids pow() per sample)
        osc.drift     = tranceDrift;
        osc.chipBlend = initialChipBlend;
    };
    auto loadEnv = [&](ADSREnv& env, int n) {
        const int idx = n - 1;
        env.setParams(oscAtkParam_[idx]->load(),
                      oscDecParam_[idx]->load(),
                      oscSusParam_[idx]->load(),
                      oscRelParam_[idx]->load(), float(sr));
    };

    // ── Spawn one voice per unison layer ────────────────────────────────
    // Track which voices we've already allocated for this note-on so that
    // allocateVoice() won't steal the same voice twice when the pool is full.
    int justAllocated[kMaxUnison] = {};
    int nAllocated = 0;

    for (int u = 0; u < uniCount; ++u) {
        const int vi = allocateVoice(note, justAllocated, nAllocated);
        justAllocated[nAllocated++] = vi;
        auto& v = voices[vi];

        loadOscParams(v.osc1, 1);
        loadOscParams(v.osc2, 2);
        loadOscParams(v.osc3, 3);
        loadEnv(v.env1, 1); loadEnv(v.env2, 2); loadEnv(v.env3, 3);

        v.ampEnv.setParams(ampAtkParam_->load(),
                           ampDecParam_->load(),
                           ampSusParam_->load(),
                           ampRelParam_->load(), float(sr));
        v.filterEnv.setParams(fenvAtkParam_->load(),
                              fenvDecParam_->load(),
                              fenvSusParam_->load(),
                              fenvRelParam_->load(), float(sr));

        // Unison detune: spread evenly from -uniDetune/2 to +uniDetune/2 cents
        // Apply to OSC1 fine (OSC2 gets a complementary smaller offset for width)
        const float detuneF = (uniCount > 1)
            ? (float(u) / float(uniCount - 1) - 0.5f) * 2.0f * uniDetune
            : 0.0f;
        v.osc1.fine += detuneF;
        v.osc2.fine += detuneF * 0.7f;  // complementary detune on osc2 for richness

        // Unison panning: -spread … +spread
        v.uniPan = (uniCount > 1)
            ? (float(u) / float(uniCount - 1) - 0.5f) * 2.0f * uniSpread
            : 0.0f;

        // Glide offset: start at previous note offset, converge to 0
        v.glideOffsetSemis = glideInterval;

        v.noteOn(note, vel, float(sr));
        v.age = ++voiceCounter;
    }

    // Only track the last manually-played note for glide purposes.
    // When the arp is active it drives all note-ons — updating lastPlayedNote_ here
    // would make every arp step portamento-slide into the next, which is wrong.
    if (!arp.enabled)
        lastPlayedNote_ = note;

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
//  Mod matrix application
//  `mods` is filled once per block in processBlock (no APVTS reads here).
// ============================================================
void SIDTranceAudioProcessor::applyModMatrix(SIDVoice& v,
                                              float lfo1v, float lfo2v,
                                              float vel, float mw,
                                              const CachedMod* mods)
{
    // Reset mod destinations
    v.modCutoff = v.modRes = v.modPW1 = v.modPW2 = v.modPW3 = v.modAmp = v.modFine1 = v.modFine2 = 0.0f;

    for (int slot = 0; slot < 4; ++slot) {
        const int   src = mods[slot].src;
        const float amt = mods[slot].amt;
        const int   dst = mods[slot].dst;

        float srcVal = 0.0f;
        switch (src) {
        case 0: srcVal = lfo1v; break;
        case 1: srcVal = lfo2v; break;
        case 2: srcVal = vel; break;  // velocity is unipolar: 0 = no mod, 1 = full mod
        case 3: srcVal = mw * 2.0f - 1.0f;  break;
        case 4: srcVal = v.ampEnv.value * 2.0f - 1.0f; break;
        case 5: srcVal = (float(v.note) - 60.0f) / 60.0f; break;
        }

        const float mod = srcVal * amt;
        switch (dst) {
        case 0: v.modCutoff += mod; break;
        case 1: v.modPW1    += mod; break;
        case 2: v.modPW2    += mod; break;
        case 3: v.modPW3    += mod; break;  // OSC3 PW
        case 4: v.modAmp    += mod; break;
        case 5: v.modRes    += mod; break;
        case 6: v.modFine1  += mod * 100.0f; break;
        case 7: v.modFine2  += mod * 100.0f; break;
        }
    }
}

// ============================================================
//  ANALOG GLOW — multi-mode drive/saturation
//  type: 0=Tube(tanh), 1=Digital(hard clip), 2=Bitcrush, 3=SID Grit(fold)
// ============================================================
float SIDTranceAudioProcessor::analogGlow(float x, float amount, int type, float precomputedBitcrushQ)
{
    if (amount < 0.001f) return x;

    switch (type) {
    default:
    case 0: {   // Tube — smooth tanh saturation
        const float drive = 1.0f + amount * 4.0f;
        return std::tanh(x * drive) / std::tanh(drive);
    }
    case 1: {   // Digital — hard clip with output normalisation
        const float drive = 1.0f + amount * 6.0f;
        const float clip  = 1.0f / drive;
        return std::clamp(x * drive, -1.0f, 1.0f) * clip;
    }
    case 2: {   // Bitcrush — reduce bit depth dynamically with drive
        // Use pre-computed Q when available (avoids std::pow on audio thread per-sample)
        const float q = (precomputedBitcrushQ > 0.0f)
                      ? precomputedBitcrushQ
                      : std::pow(2.0f, 2.0f + (1.0f - amount) * 10.0f);
        return (q > 0.0f) ? std::round(x * q) / q : x;
    }
    case 3: {   // SID Grit — wave-fold (signature SID overdrive)
        const float drive = 1.0f + amount * 5.0f;
        float v = x * drive;
        if (!std::isfinite(v)) return 0.0f;
        // Arithmetic fold: no while-loop; O(1) regardless of fold depth.
        // Maps any v into [-1, 1] with the triangle-wave fold pattern.
        // Formula: fold(v) = |((v - 1) mod 4) - 2| - 1
        v = std::fabs(std::fmod(v - 1.0f, 4.0f) - 2.0f) - 1.0f;
        return v * (1.0f / drive);
    }
    }
}

// ============================================================
//  Arp processing (generates MIDI into midiBuffer)
//
//  Key changes vs original:
//  - No heap allocation (stack arrays replace std::vector)
//  - Pre-cached seqStepParams_ pointers avoid APVTS lookups in the hot path
//  - arp.gateOpen flag prevents duplicate note-offs and fixes gate edge cases
//  - Sends note-off immediately when the held-note list empties mid-block
// ============================================================
void SIDTranceAudioProcessor::processArpEvents(juce::MidiBuffer& midi,
                                                int numSamples, float bpm)
{
    // If all keys have been released, close any hanging note and bail.
    if (arp.heldCount == 0) {
        if (arp.gateOpen && arp.currentNote >= 0) {
            midi.addEvent(juce::MidiMessage::noteOff(1, arp.currentNote), 0);
            arp.gateOpen    = false;
            arp.currentNote = -1;
        }
        return;
    }

    // Guard: pointers may not be cached yet (before first prepareToPlay call).
    if (seqStepParams_[0] == nullptr) return;

    // All via pre-cached pointers — no String alloc on audio thread
    const float tempo    = arpTempoParam_->load();
    const bool  syncDaw  = arpSyncParam_->load() > 0.5f;
    const float useBpm   = syncDaw ? std::max(1.0f, bpm) : std::max(1.0f, tempo);
    // Clamp gate to ≥1% so the note-off never fires in the same sample as the note-on.
    const float arpGate  = std::max(arpGateParam_->load(), 0.01f);
    const float swing    = arpSwingParam_->load();
    const int   octRange = std::max(1, int(arpOctaveParam_->load()));
    const int   mode     = int(arpModeParam_->load());

    const float stepLen = float(sr) * 60.0f / useBpm / 4.0f;   // samples per 16th note

    // Collect active sequencer steps — stack array, no heap alloc.
    int activeSteps[16];
    int nSteps = 0;
    for (int i = 0; i < 16; ++i) {
        if (seqStepParams_[i]->load() > 0.5f)
            activeSteps[nSteps++] = i;
    }
    if (nSteps == 0) return;

    // Copy held notes into a stack array for sorting — no heap allocation.
    int sortedNotes[ArpSequencer::kMaxHeld];
    const int nNotes = arp.heldCount;
    if (nNotes == 0) return;
    for (int i = 0; i < nNotes; ++i) sortedNotes[i] = arp.heldNotes[i];
    std::sort(sortedNotes, sortedNotes + nNotes);

    for (int samp = 0; samp < numSamples; ++samp) {
        arp.beatPhase += 1.0f;

        // Swing: odd steps get a positive offset, even steps negative.
        const bool  isOdd      = (arp.stepIdx % 2 == 1);
        const float swingOff   = stepLen * swing * 0.5f;
        const float curStepLen = stepLen + (isOdd ? swingOff : -swingOff);

        if (arp.beatPhase >= curStepLen) {
            arp.beatPhase -= curStepLen;

            // Close previous note (only if gate is still open).
            if (arp.gateOpen && arp.currentNote >= 0) {
                midi.addEvent(juce::MidiMessage::noteOff(1, arp.currentNote), samp);
                arp.gateOpen = false;
            }
            arp.currentNote = -1;

            // Choose which note to play this step.
            // `patIdx`     = position within active sequencer steps (0 .. nSteps-1)
            // `notePatIdx` = position within held notes, wrapping so all notes cycle evenly.
            //
            // Bug-fix history: the old code used `patIdx` directly as `noteIdx`, then
            // clamped.  With 3 held notes and 16 active steps, steps 3-15 all clamped to
            // note[2], so only the highest note played.  Using `patIdx % nNotes` ensures
            // the held-note list is walked evenly across all active steps.
            const int patIdx     = arp.stepIdx % nSteps;
            const int notePatIdx = (nNotes > 0) ? (patIdx % nNotes) : 0;
            int noteIdx = 0;
            switch (mode) {
            case 0: noteIdx = notePatIdx; break;                              // Up
            case 1: noteIdx = nNotes - 1 - notePatIdx; break;                // Down
            case 2: // UpDown — bounce between note[0] and note[nNotes-1]
                noteIdx = (arp.direction > 0) ? notePatIdx : (nNotes - 1 - notePatIdx);
                // Flip direction at the ends of the NOTE range (not step range)
                if (notePatIdx == nNotes - 1 && arp.direction > 0)  arp.direction = -1;
                if (notePatIdx == 0           && arp.direction < 0)  arp.direction =  1;
                break;
            case 3: // Random — uniform among held notes (thread-safe LFSR)
                arp.randLfsr ^= (arp.randLfsr << 13);
                arp.randLfsr ^= (arp.randLfsr >> 17);
                arp.randLfsr ^= (arp.randLfsr << 5);
                noteIdx = int(arp.randLfsr % (unsigned)nNotes);
                break;
            case 4: noteIdx = notePatIdx; break;                              // Order (= Up)
            }
            noteIdx = std::clamp(noteIdx, 0, nNotes - 1);

            const int octShift   = (arp.stepIdx / nSteps) % octRange;
            const int noteToPlay = std::clamp(sortedNotes[noteIdx] + octShift * 12, 0, 127);

            midi.addEvent(juce::MidiMessage::noteOn(1, noteToPlay, uint8_t(100)), samp);
            arp.currentNote = noteToPlay;
            arp.gateOpen    = true;

            arp.stepIdx = (arp.stepIdx + 1) % (nSteps * octRange);
        }

        // Gate close: only fires once per note (gateOpen guards against double note-off).
        if (arp.gateOpen && arp.currentNote >= 0 && arp.beatPhase >= curStepLen * arpGate) {
            midi.addEvent(juce::MidiMessage::noteOff(1, arp.currentNote), samp);
            arp.gateOpen = false;
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
    const float filterEnvAmt = apvts.getRawParameterValue("filter_env_amount")->load();

    const float ampVol     = apvts.getRawParameterValue("amp_volume")->load();
    const float masterVol  = apvts.getRawParameterValue("master_volume")->load();
    const bool  limiterOn  = apvts.getRawParameterValue("master_limiter")->load() > 0.5f;

    const float sidWidth   = apvts.getRawParameterValue("sid_width")->load();
    const float glowAmt    = apvts.getRawParameterValue("analog_glow")->load();

    // ── New feature params (read once per block) ────────────
    const float glideTime  = apvts.getRawParameterValue("glide_time")->load();
    // glideRate: fraction to converge per sample (exponential approach)
    // At 44100 Hz, glideTime=0.1s → 4410 samples → rate ≈ 5/sr per sample converges well
    const float glideRate  = (glideTime > 0.001f)
                             ? (5.0f / (glideTime * float(sr))) : 1.0f;

    const bool  oscSync    = apvts.getRawParameterValue("osc_sync")->load() > 0.5f;
    const float fmAmt      = apvts.getRawParameterValue("osc_fm_amt")->load();
    const int   driveType  = int(apvts.getRawParameterValue("drive_type")->load());
    const float driveAmt   = apvts.getRawParameterValue("drive_amount")->load();

    // Gate config
    gate.enabled = apvts.getRawParameterValue("gate_on")->load() > 0.5f;
    gate.swing   = apvts.getRawParameterValue("gate_swing")->load();
    if (gateStepParams_[0] != nullptr) {
        for (int i = 0; i < 16; ++i)
            gate.levels[i] = gateStepParams_[i]->load();
    }

    // Macro → parameter mappings (additive offsets baked in per block)
    const float macro1 = apvts.getRawParameterValue("macro1")->load(); // Brightness → cutoff
    const float macro2 = apvts.getRawParameterValue("macro2")->load(); // Motion → LFO rate
    const float macro3 = apvts.getRawParameterValue("macro3")->load(); // Space → reverb
    const float macro4 = apvts.getRawParameterValue("macro4")->load(); // Drive → drive_amount

    // Apply macros
    const float effectiveCutoff = std::clamp(baseCutoff * (0.3f + macro1 * 1.4f), 20.0f, 18000.0f);
    smCutoff.setTargetValue(effectiveCutoff);
    // Macro2: scale LFO rates (LFO already configured above, override rate)
    // Macro3: extra reverb wet
    const float macro3ReverbBoost = macro3 * 0.4f;
    // Macro4: adds to drive amount
    const float effectiveDrive = std::clamp(driveAmt + macro4, 0.0f, 1.0f);

    smRes.setTargetValue(baseRes);
    smMasterVol.setTargetValue(masterVol);

    // Motion (macro2): logarithmic rate multiplier centred at 1× when knob is at 12-o'clock.
    // Formula: pow(16, macro2 - 0.5) → 0.25× at 0, 1× at 0.5, 4× at 1.
    const float motionMult = std::pow(16.0f, macro2 - 0.5f);

    // LFO config — macro2 scales free-run rate only (sync mode uses host BPM, unchanged)
    lfo1.enabled = apvts.getRawParameterValue("lfo1_on")->load()     > 0.5f;
    lfo1.shape   = LFOEngine::Shape(std::clamp(int(apvts.getRawParameterValue("lfo1_shape")->load()), 0, 6));
    lfo1.rate    = apvts.getRawParameterValue("lfo1_rate")->load() * motionMult;
    lfo1.syncOn  = apvts.getRawParameterValue("lfo1_sync")->load()   > 0.5f;
    lfo1.syncDiv = int(apvts.getRawParameterValue("lfo1_div")->load());
    lfo1.retrig  = apvts.getRawParameterValue("lfo1_retrig")->load() > 0.5f;
    lfo2.enabled = apvts.getRawParameterValue("lfo2_on")->load()     > 0.5f;
    lfo2.shape   = LFOEngine::Shape(std::clamp(int(apvts.getRawParameterValue("lfo2_shape")->load()), 0, 6));
    lfo2.rate    = apvts.getRawParameterValue("lfo2_rate")->load() * motionMult;
    lfo2.syncOn  = apvts.getRawParameterValue("lfo2_sync")->load()   > 0.5f;
    lfo2.syncDiv = int(apvts.getRawParameterValue("lfo2_div")->load());
    lfo2.retrig  = apvts.getRawParameterValue("lfo2_retrig")->load() > 0.5f;

    // Copy step-curve values to LFO engines (read once per block, not per-sample)
    if (lfo1StepParams_[0] != nullptr)
        for (int i = 0; i < 16; ++i) lfo1.stepValues[i] = lfo1StepParams_[i]->load();
    if (lfo2StepParams_[0] != nullptr)
        for (int i = 0; i < 16; ++i) lfo2.stepValues[i] = lfo2StepParams_[i]->load();

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

    // Reverb config (macro3 boosts reverb mix — Space macro)
    reverb.roomSize = apvts.getRawParameterValue("fx_reverb_size")->load();
    reverb.damp     = apvts.getRawParameterValue("fx_reverb_damp")->load();
    reverb.wet      = apvts.getRawParameterValue("fx_reverb_on")->load() > 0.5f
                    ? std::clamp(apvts.getRawParameterValue("fx_reverb_mix")->load() + macro3ReverbBoost, 0.0f, 1.0f)
                    : 0.0f;

    // Host BPM
    float hostBpm = 140.0f;
    if (auto* ph = getPlayHead()) {
        juce::AudioPlayHead::CurrentPositionInfo info;
        // Only use host BPM when it's a sensible value — some hosts report 0.0
        // when the transport is stopped, which would freeze the arp/LFO.
        if (ph->getCurrentPosition(info) && info.bpm > 10.0)
            hostBpm = float(info.bpm);
    }

    // Delay tempo sync
    if (delay.enabled) {
        const int divIdx = int(apvts.getRawParameterValue("fx_delay_time")->load());
        delay.setTempoDivision(divIdx, hostBpm, float(sr));
    }

    // ── Mod matrix: read APVTS once per block (not per sample per voice) ────
    CachedMod modCache[4];
    if (modSrcParams_[0] != nullptr) {   // guard: not yet cached before first prepareToPlay
        for (int s = 0; s < 4; ++s) {
            modCache[s].src = int(modSrcParams_[s]->load());
            modCache[s].amt = modAmtParams_[s]->load();
            modCache[s].dst = int(modDstParams_[s]->load());
        }
    }

    // ── Pre-block: push block-constant filter params to all voices ──────
    // These values (type, slope, sr) are fixed for the whole block.  Writing
    // them inside the per-sample × per-voice inner loop is pure waste.
    for (auto& v : voices) {
        v.filter.type    = filterType;
        v.filter.slope24 = slope24;
        v.filter.sr      = float(sr);
    }

    // ── Dual-engine target: read the discrete chip_model parameter and feed
    //    it into the smoother.  The crossfade is then driven per-sample from
    //    the smoothed value, so flipping the UI switch produces a click-free
    //    transition between the 6581 (aliased) and 8580 (bandlimited) sounds.
    if (chipModelParam_ != nullptr)
        setModelTarget(int(std::round(chipModelParam_->load())));

    // ── Arp: route MIDI ──────────────────────────────────────
    if (arp.enabled) {
        // Intercept user note-on/off → feed arp held list
        // Remove the raw notes so only arp-generated notes drive voices
        for (const auto& m : midiMessages) {
            const auto& msg = m.getMessage();
            if (msg.isNoteOn(true))   arp.addNote(msg.getNoteNumber());
            else if (msg.isNoteOff()) arp.removeNote(msg.getNoteNumber());
        }
        midiMessages.clear();
        processArpEvents(midiMessages, numSamples, hostBpm);
    } else {
        // Arp was just turned off: send note-off for any still-sounding arp note
        // before clearing the held list, otherwise that voice will sustain forever.
        if (arp.gateOpen && arp.currentNote >= 0) {
            handleNoteOff(1, arp.currentNote);
            arp.gateOpen    = false;
            arp.currentNote = -1;
        }
        arp.heldCount = 0;  // discard stale held list (fixed array — no heap free)
    }

    // ── Pre-compute drive constants (avoid pow() on audio thread per-sample) ──
    // Bitcrush quantisation step: only recomputed when drive type/amount changes,
    // not once per sample (std::pow is not real-time safe inside inner loops).
    float bitcrushQ = 0.0f;
    if (driveType == 2 && effectiveDrive > 0.001f) {
        const float bits = 2.0f + (1.0f - effectiveDrive) * 10.0f;
        bitcrushQ = std::pow(2.0f, bits);
    }

    // ── MIDI event processing ────────────────────────────────
    auto midiIt  = midiMessages.cbegin();
    const auto midiEnd = midiMessages.cend();

    // ── Per-sample synthesis loop ────────────────────────────
    for (int samp = 0; samp < numSamples; ++samp) {
        // Process MIDI events up to this sample
        while (midiIt != midiEnd && (*midiIt).samplePosition <= samp) {
            const auto& msg = (*midiIt).getMessage();
            if (msg.isNoteOn(true)) {
                handleNoteOn(msg.getChannel(), msg.getNoteNumber(),
                             msg.getVelocity() / 127.0f);
            } else if (msg.isNoteOff()) {
                // Sustain pedal: defer note-off rather than killing the voice immediately
                if (sustainHeld_) {
                    const int n = msg.getNoteNumber();
                    bool already = false;
                    for (int i = 0; i < sustainedCount_; ++i)
                        if (sustainedNotes_[i] == n) { already = true; break; }
                    if (!already && sustainedCount_ < 128)
                        sustainedNotes_[sustainedCount_++] = n;
                } else {
                    handleNoteOff(msg.getChannel(), msg.getNoteNumber());
                }
            } else if (msg.isController()) {
                const int cc  = msg.getControllerNumber();
                const int val = msg.getControllerValue();
                if (cc == 1) {
                    modWheelVal = val / 127.0f;
                } else if (cc == 64) {
                    // Sustain pedal (CC64)
                    if (val >= 64) {
                        sustainHeld_ = true;   // pedal pressed: defer subsequent note-offs
                    } else {
                        sustainHeld_ = false;  // pedal released: flush all deferred note-offs
                        for (int i = 0; i < sustainedCount_; ++i)
                            handleNoteOff(1, sustainedNotes_[i]);
                        sustainedCount_ = 0;
                    }
                }
            } else if (msg.isPitchWheel()) {
                // Pitch wheel: map 0..16383 → ±range semitones (centre = 8192)
                pitchBendSemis_ = ((msg.getPitchWheelValue() - 8192) / 8192.0f) * kPitchBendRange;
            }
            ++midiIt;
        }

        // LFO outputs (gated by on/off toggle)
        const float l1 = lfo1.enabled ? lfo1.process(hostBpm) : 0.0f;
        const float l2 = lfo2.enabled ? lfo2.process(hostBpm) : 0.0f;

        // Smooth filter params
        const float smCut = smCutoff.getNextValue();
        const float smR   = smRes.getNextValue();
        const float smVol = smMasterVol.getNextValue();

        // Dual-engine crossfade: one step of the chip-model smoother per
        // sample, then write the result into every voice's three oscillators
        // before they render.  Linear blend of the polyBLEP correction —
        // correct for two phase-coherent engines (see SIDOscillator::chipBlend).
        const float chipBlend = chipModelSmoothed_.getNextValue();
        for (auto& vv : voices) {
            if (!vv.active) continue;
            vv.osc1.chipBlend = chipBlend;
            vv.osc2.chipBlend = chipBlend;
            vv.osc3.chipBlend = chipBlend;
        }

        // Sum all active voices — with unison panning per voice.
        // Also accumulate per-osc sums across all voices so the scopes show
        // every playing note (chord, unison stack, etc.) — not just one voice.
        float sumL = 0.0f, sumR = 0.0f;
        float scopeSum1 = 0.0f, scopeSum2 = 0.0f, scopeSum3 = 0.0f;
        int   activeCount = 0;
        for (auto& v : voices) {
            if (!v.active) continue;

            // Apply mod matrix (uses block-cached values — no APVTS read here)
            applyModMatrix(v, l1, l2, v.velocity, modWheelVal, modCache);

            const float mono = v.processMono(smCut, smR,
                                             keyTrack ? 1.0f : 0.0f,
                                             velSens  ? 1.0f : 0.0f,
                                             filterEnvAmt,
                                             glideRate, oscSync, fmAmt,
                                             pitchBendSemis_);

            // processMono just wrote v.lastS1/2/3 — accumulate them across voices.
            scopeSum1 += v.lastS1;
            scopeSum2 += v.lastS2;
            scopeSum3 += v.lastS3;

            // Unison panning: equal-power constant-gain
            const float pan  = v.uniPan;   // -1..+1
            const float panL = std::sqrt(std::max(0.0f, 0.5f - pan * 0.5f));
            const float panR = std::sqrt(std::max(0.0f, 0.5f + pan * 0.5f));
            sumL += mono * panL;
            sumR += mono * panR;
            ++activeCount;
        }

        // Capture per-osc voice-sums for the scopes.  Match the audio path's
        // 1/sqrt(N) RMS normalisation so the scope amplitude is stable
        // regardless of how many notes are held.  When no voice plays, write
        // exact zeros so the UI's silence detector trips cleanly.
        // (Safety guard: skip if host gave us a larger block than promised.)
        if (samp < (int)oscScopeScratch_[0].size()) {
            const float scopeNorm = activeCount > 0
                                  ? (1.0f / std::sqrt(float(activeCount)))
                                  : 0.0f;
            oscScopeScratch_[0][samp] = scopeSum1 * scopeNorm;
            oscScopeScratch_[1][samp] = scopeSum2 * scopeNorm;
            oscScopeScratch_[2][samp] = scopeSum3 * scopeNorm;
        }

        // Normalise by voice count (prevents clipping with 7-voice unison)
        const float norm = activeCount > 0 ? (1.0f / std::sqrt(float(activeCount))) : 1.0f;
        sumL *= ampVol * norm;
        sumR *= ampVol * norm;

        // ── Drive / Saturation (multi-mode) ──────────────────
        // Bitcrush Q is pre-computed before the loop to avoid std::pow per sample.
        sumL = analogGlow(sumL, effectiveDrive, driveType, bitcrushQ);
        sumR = analogGlow(sumR, effectiveDrive, driveType, bitcrushQ);

        // ── Analog Glow (always Tube mode — merged with drive when same type) ──
        // When drive type is already Tube AND glow is active, combine into one pass
        // to avoid double-saturating through tanh twice (which creates unexpected distortion).
        if (glowAmt > 0.001f) {
            if (driveType == 0) {
                // Same mode — single combined pass using the higher of the two amounts
                const float combined = std::clamp(effectiveDrive + glowAmt, 0.0f, 1.0f);
                // Re-do the drive pass at the combined level; undo the previous pass first
                // by dividing out then re-applying — cleanest: just apply glow as a top-up
                // through the same tanh at the glow delta.
                const float delta = std::clamp(glowAmt - effectiveDrive * 0.5f, 0.0f, 1.0f);
                if (delta > 0.001f) {
                    sumL = analogGlow(sumL, delta, 0);
                    sumR = analogGlow(sumR, delta, 0);
                }
            } else {
                // Different type — Tube glow applied after the drive as a distinct stage.
                sumL = analogGlow(sumL, glowAmt, 0);
                sumR = analogGlow(sumR, glowAmt, 0);
            }
        }

        // ── Stereo spread via SID WIDTH (mid/side enhancement) ─
        const float mid  = (sumL + sumR) * 0.5f;
        const float side = (sumL - sumR) * 0.5f * (1.0f + sidWidth);
        float L = mid + side;
        float R = mid - side;

        // ── FX Chain ─────────────────────────────────────────
        chorus.process(L, R);
        delay.process(L, R);
        reverb.process(L, R);

        // ── Trance Gate ───────────────────────────────────────
        if (gate.enabled && gateStepParams_[0] != nullptr) {
            const float gateStepLen = float(sr) * 60.0f / hostBpm / 4.0f;
            // smoothing coefficient: 2ms at current sample rate
            const float gateSmooth  = 1.0f - std::exp(-1.0f / (float(sr) * 0.002f));
            // processStereo advances beatPhase exactly once per sample.
            // Calling process() twice (once per channel) ran the gate at 2× speed.
            gate.processStereo(L, R, gateStepLen, gateSmooth);
        }

        // ── Master volume ─────────────────────────────────────
        L *= smVol;
        R *= smVol;

        // ── Limiter — brick-wall: instantaneous attack, smooth release ─
        if (limiterOn) {
            const float peakL = std::abs(L);
            const float peakR = std::abs(R);
            static constexpr float relLim = 0.9998f;  // ~100 ms release @ 44100
            // Instantaneous attack: grab the peak immediately, then decay smoothly
            limEnvL = (peakL > limEnvL) ? peakL : limEnvL * relLim;
            limEnvR = (peakR > limEnvR) ? peakR : limEnvR * relLim;
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

    // ── Push per-osc samples to their FIFOs (UI reads at ~30 fps) ────
    for (int o = 0; o < 3; ++o) {
        auto& s = oscScope[o];
        const int toWrite = std::min(numSamples, s.fifo.getFreeSpace());
        if (toWrite > 0) {
            int s1, sz1, s2, sz2;
            s.fifo.prepareToWrite(toWrite, s1, sz1, s2, sz2);
            float*       dst = s.buf.getWritePointer(0);
            const float* src = oscScopeScratch_[o].data();
            if (sz1 > 0) std::memcpy(dst + s1, src,       size_t(sz1) * sizeof(float));
            if (sz2 > 0) std::memcpy(dst + s2, src + sz1, size_t(sz2) * sizeof(float));
            s.fifo.finishedWrite(toWrite);
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
        if (xml->hasTagName(apvts.state.getType())) {
            apvts.replaceState(juce::ValueTree::fromXml(*xml));
            // Signal the editor to rebind all widgets on the next render tick.
            // Visage controls are not APVTS listeners so they won't auto-update.
            stateJustLoaded = true;
        }
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
