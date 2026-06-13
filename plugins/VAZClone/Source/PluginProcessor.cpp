#include "PluginProcessor.h"
#ifndef VAZ_HEADLESS
 #include "PluginEditor.h"
#endif
#include "ParameterIDs.hpp"
#include "VAZInitTemplate.h"

//==============================================================================
VAZCloneAudioProcessor::VAZCloneAudioProcessor()
    : AudioProcessor (BusesProperties().withOutput ("Output", juce::AudioChannelSet::stereo(), true)),
      apvts (*this, nullptr, "PARAMETERS", createParameterLayout())
{
    synth.addSound (new VAZSound());
    // Detune positions spread WIDE across the first voices so Unison (which uses the first few
    // voices) always gets an obvious chorus, while Poly gets consistent per-voice analog drift.
    static const double spread[32] = { -1.0, 1.0, -0.5, 0.5, -0.75, 0.75, -0.25, 0.25,
                                        -0.9, 0.9, -0.6, 0.6, -0.35, 0.35, -0.12, 0.12,
                                        -0.82, 0.82, -0.45, 0.45, -0.68, 0.68, -0.20, 0.20,
                                        -0.95, 0.95, -0.55, 0.55, -0.30, 0.30, -0.07, 0.07 };
    for (int i = 0; i < kNumVoices; ++i)
        synth.addVoice (new VAZVoice (voiceParams, spread[i % 32]));
    sampleFormatMgr.registerBasicFormats();          // WAV/AIFF/FLAC for the sample oscillator
    voiceParams.osc1Sample = &osc1SampleData;        // stable ptrs (contents swapped under the callback lock)
    voiceParams.osc2Sample = &osc2SampleData;
}

VAZCloneAudioProcessor::~VAZCloneAudioProcessor() {}

//==============================================================================
juce::AudioProcessorValueTreeState::ParameterLayout VAZCloneAudioProcessor::createParameterLayout()
{
    using namespace juce;
    AudioProcessorValueTreeState::ParameterLayout layout;

    auto pct = [] (const char* id, const char* name, float def)
    {
        return std::make_unique<AudioParameterFloat>(
            ParameterID { id, 1 }, name, NormalisableRange<float>(0.0f, 1.0f), def);
    };

    // Oscillators / Mixer
    layout.add (std::make_unique<AudioParameterChoice>(ParameterID { ParameterIDs::o1_octave, 1 },
        "OSC1 Octave", StringArray { "32'", "16'", "8'", "4'", "2'" }, 2));
    // VAZ 5 waveform modes (OSC2 has Sync instead of Ext)
    const StringArray waves1 { "Sawtooth", "Pulse", "Multi-Saw", "Sample", "Ext" };
    const StringArray waves2 { "Sawtooth", "Pulse", "Multi-Saw", "Sample", "Sync" };
    layout.add (std::make_unique<AudioParameterChoice>(ParameterID { ParameterIDs::o1_wave, 1 },
        "OSC1 Waveform", waves1, 0));   // default Sawtooth
    layout.add (pct (ParameterIDs::o1_coarse, "OSC1 Coarse", 0.5f));   // center = no transpose
    layout.add (pct (ParameterIDs::o1_fine,   "OSC1 Fine",   0.5f));   // center = no detune
    layout.add (pct (ParameterIDs::o1_shape,  "OSC1 Waveshape", 0.0f));
    layout.add (pct (ParameterIDs::o1_level,  "OSC1 Level", 1.0f));
    layout.add (std::make_unique<AudioParameterChoice>(ParameterID { ParameterIDs::o2_octave, 1 },
        "OSC2 Octave", StringArray { "32'", "16'", "8'", "4'", "2'" }, 2));
    layout.add (std::make_unique<AudioParameterChoice>(ParameterID { ParameterIDs::o2_wave, 1 },
        "OSC2 Waveform", waves2, 0));
    layout.add (pct (ParameterIDs::o2_coarse, "OSC2 Coarse", 0.5f));
    layout.add (pct (ParameterIDs::o2_fine,   "OSC2 Fine",   0.5f));
    layout.add (pct (ParameterIDs::o2_shape,  "OSC2 Waveshape", 0.0f));
    layout.add (pct (ParameterIDs::o2_detune, "OSC2 Detune", 0.0f));
    layout.add (pct (ParameterIDs::o2_level,  "OSC2 Level", 0.0f));
    layout.add (pct (ParameterIDs::noise_level, "Noise Level", 0.0f));
    layout.add (std::make_unique<AudioParameterChoice>(ParameterID { ParameterIDs::mix1_src, 1 }, "Mix1 Source", StringArray { "Oscillator 1","Ring Modulator","Noise","External Input","Mod Amplifier 1","Mod Amplifier 2" }, 0));
    layout.add (std::make_unique<AudioParameterChoice>(ParameterID { ParameterIDs::mix2_src, 1 }, "Mix2 Source", StringArray { "Oscillator 2","Ring Modulator","Noise","External Input","Mod Amplifier 1","Mod Amplifier 2" }, 0));
    layout.add (std::make_unique<AudioParameterChoice>(ParameterID { ParameterIDs::mix3_src, 1 }, "Mix3 Source", StringArray { "Noise","Oscillator 3","Ring Modulator","External Input","Mod Amplifier 1","Mod Amplifier 2" }, 0));
    layout.add (std::make_unique<AudioParameterBool>(ParameterID { ParameterIDs::mix1_post, 1 }, "Mix1 Post", false));
    layout.add (std::make_unique<AudioParameterBool>(ParameterID { ParameterIDs::mix2_post, 1 }, "Mix2 Post", false));
    layout.add (std::make_unique<AudioParameterBool>(ParameterID { ParameterIDs::mix3_post, 1 }, "Mix3 Post", false));

    // Filter (22 modes ordered by .v2p byte value 0-21, default 19 = Type R 4-Pole LP Res)
    layout.add (std::make_unique<AudioParameterChoice>(ParameterID { ParameterIDs::filter_mode, 1 },
        "Filter Mode", StringArray {
            "A LP", "B LP", "C 2P LP RM", "C 4P LP RM", "A HP", "A BP", "B HP", "B BP",
            "C 4P LP SM", "C 4P LP HM", "D LP", "D BP", "D HP", "D HP+LP", "C 2P LP HM",
            "K LP", "K HP+LP", "R 2P LP RM", "R 2P LP HM", "R 4P LP RM", "R 4P LP HM", "Comb" }, 19));
    layout.add (pct (ParameterIDs::cutoff,    "Cutoff", 1.0f));   // open by default → raw bright osc
    layout.add (pct (ParameterIDs::resonance, "Resonance", 0.0f));
    layout.add (pct (ParameterIDs::hp_cutoff, "Highpass Cutoff", 0.0f));
    layout.add (pct (ParameterIDs::flt_aux,   "Bandwidth/Separation", 0.5f));

    // Amplifier
    layout.add (pct (ParameterIDs::overdrive, "Overdrive", 0.0f));

    // Envelope 1 (amp) — VAZ Init: A0 D0 S-full R0 (pure gate)
    layout.add (pct (ParameterIDs::e1_attack,  "Env1 Attack", 0.0f));
    layout.add (pct (ParameterIDs::e1_decay,   "Env1 Decay", 0.0f));
    layout.add (pct (ParameterIDs::e1_sustain, "Env1 Sustain", 1.0f));
    layout.add (pct (ParameterIDs::e1_release, "Env1 Release", 0.0f));

    // Envelope 2 (mod/filter) — VAZ Init: A0 D0 S-full R0
    layout.add (pct (ParameterIDs::e2_attack,  "Env2 Attack", 0.0f));
    layout.add (pct (ParameterIDs::e2_decay,   "Env2 Decay", 0.0f));
    layout.add (pct (ParameterIDs::e2_sustain, "Env2 Sustain", 1.0f));
    layout.add (pct (ParameterIDs::e2_release, "Env2 Release", 0.0f));
    layout.add (pct (ParameterIDs::filt_env_amt, "Filter Env Amount", 0.0f));

    // LFO 1 / 2 / 3 — VAZ Init: rate 0
    const StringArray lfoWaves { "Saw / Tri", "Tri + Delay", "Saw + Delay", "Sine + Delay", "Pulse", "Square + Delay", "S&H + Lag", "S&H + Delay" };
    layout.add (pct (ParameterIDs::lfo_rate, "LFO1 Rate", 0.0f));
    layout.add (std::make_unique<AudioParameterChoice>(ParameterID { ParameterIDs::lfo_wave, 1 },  "LFO1 Wave", lfoWaves, 0));
    layout.add (pct (ParameterIDs::lfo_amt, "Cutoff Mod 2 Depth", 0.0f));
    layout.add (pct (ParameterIDs::lfo2_rate, "LFO2 Rate", 0.0f));
    layout.add (std::make_unique<AudioParameterChoice>(ParameterID { ParameterIDs::lfo2_wave, 1 }, "LFO2 Wave", lfoWaves, 0));
    layout.add (pct (ParameterIDs::lfo3_rate, "LFO3 Rate", 0.0f));
    layout.add (std::make_unique<AudioParameterChoice>(ParameterID { ParameterIDs::lfo3_wave, 1 }, "LFO3 Wave", StringArray { "Tri", "Sine" }, 0));
    layout.add (pct (ParameterIDs::lfo_shape,  "LFO1 Shape", 0.5f));
    layout.add (pct (ParameterIDs::lfo2_shape, "LFO2 Shape", 0.5f));
    layout.add (pct (ParameterIDs::lfo2_delay, "LFO2 Delay", 0.0f));   // fade-in time for the +Delay waveforms
    layout.add (std::make_unique<AudioParameterBool>(ParameterID { ParameterIDs::lfo_trig, 1 },  "LFO1 Trig", false));
    layout.add (std::make_unique<AudioParameterBool>(ParameterID { ParameterIDs::lfo2_trig, 1 }, "LFO2 Trig", false));
    const StringArray lfoPeriods { "1/32T","1/32","1/16T","1/16","1/8T","1/8","1/4T","1/4",
        "2b","3b","4b","5b","6b","8b","12b","16b","24b","32b","48b","64b","96b","128b","192b","256b" };
    layout.add (std::make_unique<AudioParameterBool>(ParameterID { ParameterIDs::lfo_sync, 1 }, "LFO1 Sync", false));
    layout.add (std::make_unique<AudioParameterChoice>(ParameterID { ParameterIDs::lfo_period, 1 }, "LFO1 Period", lfoPeriods, 10));
    layout.add (std::make_unique<AudioParameterBool>(ParameterID { ParameterIDs::lfo2_sync, 1 }, "LFO2 Sync", false));
    layout.add (std::make_unique<AudioParameterChoice>(ParameterID { ParameterIDs::lfo2_period, 1 }, "LFO2 Period", lfoPeriods, 10));

    // Modulation matrix — selectable sources (KEYSTONE). Order MUST match the GUI modSources[]
    // array so the dropdown indices line up. (Implemented subset in srcVal(); rest read as 0.)
    const StringArray modSrcs {
        "None", "LFO 1", "LFO 2", "LFO 3", "Envelope 1", "Envelope 2", "Mod Amplifier 1",
        "Mod Amplifier 2", "Lag Processor", "Oscillator 1", "Oscillator 1 Pitch", "Oscillator 2",
        "Noise", "External Input", "Accent", "Sequencer A", "Sequencer B", "MIDI Velocity",
        "MIDI Pressure", "MIDI Control A", "MIDI Control B", "Voice Number" };
    layout.add (std::make_unique<AudioParameterChoice>(ParameterID { ParameterIDs::cut_mod1_src, 1 }, "Cutoff Mod 1 Src", modSrcs, 5)); // Envelope 2
    layout.add (std::make_unique<AudioParameterChoice>(ParameterID { ParameterIDs::cut_mod2_src, 1 }, "Cutoff Mod 2 Src", modSrcs, 1)); // LFO 1
    layout.add (std::make_unique<AudioParameterChoice>(ParameterID { ParameterIDs::res_mod_src, 1 },  "Res Mod Src",      modSrcs, 0)); // None
    layout.add (std::make_unique<AudioParameterChoice>(ParameterID { ParameterIDs::lfo2_rm_src, 1 },  "LFO2 Rate Mod Src", modSrcs, 0));
    layout.add (pct (ParameterIDs::lfo2_rm_amt, "LFO2 Rate Mod Depth", 0.0f));
    layout.add (pct (ParameterIDs::res_mod_amt, "Res Mod Depth", 0.0f));
    // Oscillator Frequency Modulation (1 input each, reads the same mod bus)
    layout.add (std::make_unique<AudioParameterChoice>(ParameterID { ParameterIDs::o1_fm_src, 1 }, "OSC1 FM Src", modSrcs, 0));
    layout.add (pct (ParameterIDs::o1_fm_amt, "OSC1 FM Depth", 0.0f));
    layout.add (std::make_unique<AudioParameterChoice>(ParameterID { ParameterIDs::o2_fm_src, 1 }, "OSC2 FM Src", modSrcs, 0));
    layout.add (pct (ParameterIDs::o2_fm_amt, "OSC2 FM Depth", 0.0f));
    layout.add (std::make_unique<AudioParameterChoice>(ParameterID { ParameterIDs::o1_ws_src, 1 }, "OSC1 WaveshapeMod Src", modSrcs, 0));
    layout.add (pct (ParameterIDs::o1_ws_amt, "OSC1 WaveshapeMod Depth", 0.0f));
    layout.add (std::make_unique<AudioParameterChoice>(ParameterID { ParameterIDs::o2_ws_src, 1 }, "OSC2 WaveshapeMod Src", modSrcs, 0));
    layout.add (pct (ParameterIDs::o2_ws_amt, "OSC2 WaveshapeMod Depth", 0.0f));
    layout.add (std::make_unique<AudioParameterChoice>(ParameterID { ParameterIDs::amp_mod_src, 1 }, "Amp Mod Src", modSrcs, 0));
    layout.add (pct (ParameterIDs::amp_mod_amt, "Amp Mod Depth", 0.0f));
    layout.add (pct (ParameterIDs::amp_level,   "Amp Level", 0.8f));   // Amplitude-Mod slot-1 depth = master volume
    layout.add (std::make_unique<AudioParameterChoice>(ParameterID { ParameterIDs::pan_mod_src, 1 }, "Pan Mod Src", modSrcs, 0));
    layout.add (pct (ParameterIDs::pan_mod_amt, "Pan Mod Depth", 0.0f));
    // Extra VAZ mod slots: 2nd FM input per osc, 3rd filter-cutoff mod, 2nd amp AM source.
    layout.add (std::make_unique<AudioParameterChoice>(ParameterID { ParameterIDs::o1_fm2_src, 1 }, "OSC1 FM2 Src", modSrcs, 0));
    layout.add (pct (ParameterIDs::o1_fm2_amt, "OSC1 FM2 Depth", 0.0f));
    layout.add (std::make_unique<AudioParameterChoice>(ParameterID { ParameterIDs::o2_fm2_src, 1 }, "OSC2 FM2 Src", modSrcs, 0));
    layout.add (pct (ParameterIDs::o2_fm2_amt, "OSC2 FM2 Depth", 0.0f));
    layout.add (std::make_unique<AudioParameterChoice>(ParameterID { ParameterIDs::cut_mod3_src, 1 }, "Cutoff Mod 3 Src", modSrcs, 0));
    layout.add (pct (ParameterIDs::cut_mod3_amt, "Cutoff Mod 3 Depth", 0.0f));
    layout.add (std::make_unique<AudioParameterChoice>(ParameterID { ParameterIDs::amp_mod2_src, 1 }, "Amp Mod 2 Src", modSrcs, 0));
    layout.add (pct (ParameterIDs::amp_mod2_amt, "Amp Mod 2 Depth", 0.0f));
    for (auto* invId : { ParameterIDs::filt_env_amt_inv, ParameterIDs::lfo_amt_inv, ParameterIDs::res_mod_amt_inv, ParameterIDs::amp_mod_amt_inv, ParameterIDs::pan_mod_amt_inv, ParameterIDs::o1_fm_amt_inv, ParameterIDs::o2_fm_amt_inv, ParameterIDs::o1_ws_amt_inv, ParameterIDs::o2_ws_amt_inv, ParameterIDs::e2_mod_amt_inv, ParameterIDs::ma1_am_amt_inv, ParameterIDs::ma2_am_amt_inv, ParameterIDs::o1_fm2_amt_inv, ParameterIDs::o2_fm2_amt_inv, ParameterIDs::cut_mod3_amt_inv, ParameterIDs::amp_mod2_amt_inv })
        layout.add (std::make_unique<AudioParameterBool>(ParameterID { invId, 1 }, juce::String (invId), false));
    layout.add (std::make_unique<AudioParameterBool>(ParameterID { ParameterIDs::osc_link, 1 }, "Osc Link", false));
    layout.add (std::make_unique<AudioParameterBool>(ParameterID { ParameterIDs::osc2_sync, 1 }, "Osc2 Sync", false));
    layout.add (std::make_unique<AudioParameterBool>(ParameterID { ParameterIDs::ma1_sq,   1 }, "Mod Amp 1 SQ", false));
    layout.add (std::make_unique<AudioParameterChoice>(ParameterID { ParameterIDs::e2_mod_src, 1 }, "Env2 Mod Src", modSrcs, 0));
    layout.add (pct (ParameterIDs::e2_mod_amt, "Env2 Mod Depth", 0.0f));
    layout.add (std::make_unique<AudioParameterChoice>(ParameterID { ParameterIDs::e2_dest, 1 }, "Env2 Dest", StringArray { "Attack","Decay","Sustain","Release","None" }, 4));
    layout.add (std::make_unique<AudioParameterChoice>(ParameterID { ParameterIDs::ma1_in_src, 1 }, "ModAmp1 In", modSrcs, 0));
    layout.add (std::make_unique<AudioParameterChoice>(ParameterID { ParameterIDs::ma1_am_src, 1 }, "ModAmp1 AM Src", modSrcs, 0));
    layout.add (pct (ParameterIDs::ma1_am_amt, "ModAmp1 AM Depth", 1.0f));
    layout.add (std::make_unique<AudioParameterChoice>(ParameterID { ParameterIDs::ma2_in_src, 1 }, "ModAmp2 In", modSrcs, 0));
    layout.add (std::make_unique<AudioParameterChoice>(ParameterID { ParameterIDs::ma2_am_src, 1 }, "ModAmp2 AM Src", modSrcs, 0));
    layout.add (pct (ParameterIDs::ma2_am_amt, "ModAmp2 AM Depth", 1.0f));
    layout.add (std::make_unique<AudioParameterChoice>(ParameterID { ParameterIDs::lag_in_src, 1 }, "Lag In", modSrcs, 0));
    layout.add (pct (ParameterIDs::lag_time, "Lag Time", 0.3f));
    // Envelope modes
    auto boolp = [] (const char* id, const char* name) { return std::make_unique<AudioParameterBool>(ParameterID { id, 1 }, name, false); };
    layout.add (boolp (ParameterIDs::e1_reset, "Env1 Reset")); layout.add (boolp (ParameterIDs::e1_cycle, "Env1 Cycle")); layout.add (boolp (ParameterIDs::e1_curve, "Env1 Curve"));
    layout.add (boolp (ParameterIDs::e2_reset, "Env2 Reset")); layout.add (boolp (ParameterIDs::e2_cycle, "Env2 Cycle")); layout.add (boolp (ParameterIDs::e2_curve, "Env2 Curve"));
    layout.add (boolp (ParameterIDs::e1_multi, "Env1 Multi")); layout.add (boolp (ParameterIDs::e2_multi, "Env2 Multi"));
    layout.add (std::make_unique<AudioParameterChoice>(ParameterID { ParameterIDs::note_priority, 1 }, "Note Priority", StringArray { "Last", "High", "Low", "Duo" }, 0));

    // Performance
    layout.add (std::make_unique<AudioParameterChoice>(ParameterID { ParameterIDs::voice_mode, 1 },
        "Voice Mode", StringArray { "Mono", "Poly", "Unison" }, 1));
    layout.add (pct (ParameterIDs::uni_detune, "Detune", 0.0f));
    layout.add (pct (ParameterIDs::portamento, "Portamento", 0.0f));
    layout.add (boolp (ParameterIDs::porta_exp, "Portamento Exp"));
    layout.add (std::make_unique<AudioParameterBool> (ParameterID { ParameterIDs::porta_auto, 1 }, "Portamento Auto", true));
    layout.add (pct (ParameterIDs::bend_range, "Bend Range", 1.0f / 23.0f));   // ≈ 2 semitones
    {   // Voices (polyphony): "Dynamic" (index 0 = full 32-voice pool) + a fixed 1..32 limit. VAZ's Voices control.
        StringArray vch; vch.add ("Dynamic");
        for (int n = 1; n <= 32; ++n) vch.add (String (n));
        layout.add (std::make_unique<AudioParameterChoice> (ParameterID { ParameterIDs::voices, 1 }, "Voices", vch, 0));
    }
    layout.add (pct (ParameterIDs::uni_voices, "Unison Voices", 3.0f / 31.0f)); // ≈ 4 voices (range 1..32)
    layout.add (std::make_unique<AudioParameterBool>  (ParameterID { ParameterIDs::arp_on,  1 }, "Arp On", false));
    layout.add (std::make_unique<AudioParameterChoice>(ParameterID { ParameterIDs::arp_mode,1 }, "Arp Mode", StringArray { "Up","Down","Up&Down","Random" }, 0));
    layout.add (std::make_unique<AudioParameterChoice>(ParameterID { ParameterIDs::arp_rate,1 }, "Arp Rate", StringArray { "1/4","1/8","1/8T","1/16","1/16T","1/32" }, 3));
    layout.add (std::make_unique<AudioParameterChoice>(ParameterID { ParameterIDs::arp_oct, 1 }, "Arp Octaves", StringArray { "1","2","3","4" }, 0));
    layout.add (std::make_unique<AudioParameterBool>  (ParameterID { ParameterIDs::arp_hold,1 }, "Arp Hold", false));

    return layout;
}

//==============================================================================
void VAZCloneAudioProcessor::prepareToPlay (double sampleRate, int samplesPerBlock)
{
    currentSampleRate = sampleRate;

    // A/B automation hook: load a .v2p named by env VAZCLONE_LOAD_PATCH (one-shot, after state restore).
    if (! envPatchTried)
    {
        envPatchTried = true;
        if (const char* pp = std::getenv ("VAZCLONE_LOAD_PATCH"))
        {
            juce::File f (juce::String::fromUTF8 (pp));
            juce::MemoryBlock mb;
            if (f.existsAsFile() && f.loadFileAsData (mb)) loadV2P (mb);
        }
    }

    synth.setCurrentPlaybackSampleRate (sampleRate);
    synth.setNoteStealingEnabled (true);

    ladder.prepare (sampleRate);
    smoothCutoff.reset (sampleRate, 0.02);
    smoothRes.reset (sampleRate, 0.02);
    filterEnv.setSampleRate (sampleRate);
    activeNotes = 0;
    lfo1Buf.assign ((size_t) samplesPerBlock, 0.0f);
    lfo2Buf.assign ((size_t) samplesPerBlock, 0.0f);
    lfo3Buf.assign ((size_t) samplesPerBlock, 0.0f);
    env2Buf.assign ((size_t) samplesPerBlock, 0.0f);
    ma1Buf .assign ((size_t) samplesPerBlock, 0.0f);
    ma2Buf .assign ((size_t) samplesPerBlock, 0.0f);
    lagBuf .assign ((size_t) samplesPerBlock, 0.0f);
    noiseModBuf.assign ((size_t) samplesPerBlock, 0.0f);
    lagState = 0.0;
}

// Filter mode index (0-21) is decoded directly by VAZMultiFilter::setMode (engine + tap).


void VAZCloneAudioProcessor::refreshVoiceParams()
{
    auto f = [this] (const char* id) { return apvts.getRawParameterValue (id)->load(); };
    auto sgn = [&] (const char* id) { return f (id) > 0.5f ? -1.0f : 1.0f; };   // − sign toggle = invert
    voiceParams.o1Wave   = (int) f (ParameterIDs::o1_wave);
    voiceParams.o2Wave   = (int) f (ParameterIDs::o2_wave);
    voiceParams.o1Octave = (int) f (ParameterIDs::o1_octave);
    voiceParams.o2Octave = (int) f (ParameterIDs::o2_octave);
    voiceParams.o1Coarse = f (ParameterIDs::o1_coarse);
    voiceParams.o1Fine   = f (ParameterIDs::o1_fine);
    voiceParams.o2Coarse = f (ParameterIDs::o2_coarse);
    voiceParams.o2Fine   = f (ParameterIDs::o2_fine);
    voiceParams.o1FmSrc  = (int) f (ParameterIDs::o1_fm_src);
    voiceParams.o1FmAmt  = f (ParameterIDs::o1_fm_amt) * sgn (ParameterIDs::o1_fm_amt_inv);
    voiceParams.o2FmSrc  = (int) f (ParameterIDs::o2_fm_src);
    voiceParams.o2FmAmt  = f (ParameterIDs::o2_fm_amt) * sgn (ParameterIDs::o2_fm_amt_inv);
    voiceParams.o1WsSrc  = (int) f (ParameterIDs::o1_ws_src);
    voiceParams.o1WsAmt  = f (ParameterIDs::o1_ws_amt) * sgn (ParameterIDs::o1_ws_amt_inv);
    voiceParams.o2WsSrc  = (int) f (ParameterIDs::o2_ws_src);
    voiceParams.o2WsAmt  = f (ParameterIDs::o2_ws_amt) * sgn (ParameterIDs::o2_ws_amt_inv);
    voiceParams.portamento = f (ParameterIDs::portamento);
    voiceParams.portaExp   = f (ParameterIDs::porta_exp) > 0.5f;
    voiceParams.portaAuto  = f (ParameterIDs::porta_auto) > 0.5f;
    voiceParams.bendRange  = f (ParameterIDs::bend_range);
    voiceParams.e1Reset = f (ParameterIDs::e1_reset) > 0.5f;
    voiceParams.e1Cycle = f (ParameterIDs::e1_cycle) > 0.5f;
    voiceParams.e1Curve = f (ParameterIDs::e1_curve) > 0.5f;
    voiceParams.o1Shape  = f (ParameterIDs::o1_shape);
    voiceParams.o2Shape  = f (ParameterIDs::o2_shape);
    voiceParams.o1Level  = f (ParameterIDs::o1_level);
    voiceParams.o2Detune = f (ParameterIDs::o2_detune);
    voiceParams.o2Level  = f (ParameterIDs::o2_level);
    voiceParams.noise    = f (ParameterIDs::noise_level);
    voiceParams.mix1Src  = (int) f (ParameterIDs::mix1_src);
    voiceParams.mix2Src  = (int) f (ParameterIDs::mix2_src);
    voiceParams.mix3Src  = (int) f (ParameterIDs::mix3_src);
    voiceParams.mix1Post = f (ParameterIDs::mix1_post) > 0.5f;
    voiceParams.mix2Post = f (ParameterIDs::mix2_post) > 0.5f;
    voiceParams.mix3Post = f (ParameterIDs::mix3_post) > 0.5f;
    voiceParams.atk      = f (ParameterIDs::e1_attack);
    voiceParams.dec      = f (ParameterIDs::e1_decay);
    voiceParams.sus      = f (ParameterIDs::e1_sustain);
    voiceParams.rel      = f (ParameterIDs::e1_release);
    // Per-voice filter params
    voiceParams.filterMode = (int) f (ParameterIDs::filter_mode);
    voiceParams.baseCut  = f (ParameterIDs::cutoff);
    voiceParams.baseRes  = f (ParameterIDs::resonance);
    voiceParams.fltAux   = f (ParameterIDs::flt_aux);
    voiceParams.hpNorm   = f (ParameterIDs::hp_cutoff);
    voiceParams.cutSrc1  = (int) f (ParameterIDs::cut_mod1_src); voiceParams.cutAmt1 = f (ParameterIDs::filt_env_amt) * sgn (ParameterIDs::filt_env_amt_inv);
    voiceParams.cutSrc2  = (int) f (ParameterIDs::cut_mod2_src); voiceParams.cutAmt2 = f (ParameterIDs::lfo_amt) * sgn (ParameterIDs::lfo_amt_inv);
    voiceParams.resSrc   = (int) f (ParameterIDs::res_mod_src);  voiceParams.resAmt  = f (ParameterIDs::res_mod_amt) * sgn (ParameterIDs::res_mod_amt_inv);
    voiceParams.ampSrc   = (int) f (ParameterIDs::amp_mod_src);  voiceParams.ampAmt  = f (ParameterIDs::amp_mod_amt) * sgn (ParameterIDs::amp_mod_amt_inv);
    voiceParams.panSrc   = (int) f (ParameterIDs::pan_mod_src);  voiceParams.panAmt  = f (ParameterIDs::pan_mod_amt) * sgn (ParameterIDs::pan_mod_amt_inv);
    voiceParams.o1Fm2Src = (int) f (ParameterIDs::o1_fm2_src);   voiceParams.o1Fm2Amt = f (ParameterIDs::o1_fm2_amt) * sgn (ParameterIDs::o1_fm2_amt_inv);
    voiceParams.o2Fm2Src = (int) f (ParameterIDs::o2_fm2_src);   voiceParams.o2Fm2Amt = f (ParameterIDs::o2_fm2_amt) * sgn (ParameterIDs::o2_fm2_amt_inv);
    voiceParams.cutSrc3  = (int) f (ParameterIDs::cut_mod3_src); voiceParams.cutAmt3 = f (ParameterIDs::cut_mod3_amt) * sgn (ParameterIDs::cut_mod3_amt_inv);
    voiceParams.amp2Src  = (int) f (ParameterIDs::amp_mod2_src); voiceParams.amp2Amt = f (ParameterIDs::amp_mod2_amt) * sgn (ParameterIDs::amp_mod2_amt_inv);
    voiceParams.ampLevel = f (ParameterIDs::amp_level);
    voiceParams.link     = f (ParameterIDs::osc_link) > 0.5f;
    voiceParams.osc2Sync = f (ParameterIDs::osc2_sync) > 0.5f;
    voiceParams.e2ModSrc = (int) f (ParameterIDs::e2_mod_src);
    voiceParams.e2ModAmt = f (ParameterIDs::e2_mod_amt) * sgn (ParameterIDs::e2_mod_amt_inv);
    voiceParams.e2Dest   = (int) f (ParameterIDs::e2_dest);
    voiceParams.e2Atk    = f (ParameterIDs::e2_attack);  voiceParams.e2Dec = f (ParameterIDs::e2_decay);
    voiceParams.e2Sus    = f (ParameterIDs::e2_sustain); voiceParams.e2Rel = f (ParameterIDs::e2_release);
    voiceParams.e2Reset  = f (ParameterIDs::e2_reset) > 0.5f;
    voiceParams.e2Cycle  = f (ParameterIDs::e2_cycle) > 0.5f;
    voiceParams.e2Curve  = f (ParameterIDs::e2_curve) > 0.5f;
    voiceParams.e1Multi  = f (ParameterIDs::e1_multi) > 0.5f;
    voiceParams.e2Multi  = f (ParameterIDs::e2_multi) > 0.5f;
    voiceParams.fltDrive = juce::jlimit (1.0f, 4.0f, 0.5f + voiceParams.o1Level + voiceParams.o2Level + voiceParams.noise);
    voiceParams.overdrive = f (ParameterIDs::overdrive);   // Overdrive knob → output cubic soft-clip (VAZ output stage, all modes)
    voiceParams.nyq      = (float) (currentSampleRate * 0.45);
}

void VAZCloneAudioProcessor::releaseResources() {}

bool VAZCloneAudioProcessor::isBusesLayoutSupported (const BusesLayout& layouts) const
{
    const auto& out = layouts.getMainOutputChannelSet();
    return out == juce::AudioChannelSet::mono() || out == juce::AudioChannelSet::stereo();
}

// ── .v2p patch I/O ──────────────────────────────────────────────────────────────────────────
// The .v2p is a nested chunk format [FourCC][u32 size][data]; inside PRST the parameters are a
// SEQUENTIAL, VERSION-GATED stream (NOT fixed offsets). Faithful port of VAZ's real reader
// FUN_004d6c3c @ 0x4d6c3c in Vaz2010Core.dll (verified field-by-field via the named/text reader
// FUN_004d891c and validated against the factory bank — see tools/vaz_v2p_format.md). Read widths:
// FUN_0049d5cc = u32, FUN_0049d620/5f4 = byte, FUN_0049d668 = u32, FUN_004d6c18 = u32 mod-source
// (+1 when version<200 && val>6). Version gates explain the ~90-byte layout delta that made the
// old fixed-offset loader misread 82% of the factory bank (the pre-2.0 v103-111 patches).
static int findTag (const juce::uint8* d, int n, const char* t, int from)
{
    for (int i = juce::jmax (0, from); i + 4 <= n; ++i)
        if (d[i]==t[0] && d[i+1]==t[1] && d[i+2]==t[2] && d[i+3]==t[3]) return i;
    return -1;
}

namespace {
struct V2PPatch
{
    int ver = 0;
    int filterMode = 0, cutoff = 255, reso = 0, bandwidth = 0, hpCut = 0;
    int fcut1s = 0, fcut1d = 0, fcut2s = 0, fcut2d = 0, fcut3s = 0, fcut3d = 0, fresS = 0, fresD = 0;
    int am1s = 3, am1d = 0, am2s = 0, am2d = 0, am3s = 0, am3d = 0, overdrive = 0;
    int e1a = 0, e1d = 0, e1s = 0, e1r = 0, e2a = 0, e2d = 0, e2s = 0, e2r = 0, e1mode = 0, e2mode = 0;
    int lfo1rate = 0, lfo2rate = 0, mono = 0;
    int lfo1wave = 0, lfo1shape = 127, lfo1trig = 0;   // LFO1 waveform / waveshape / retrigger
    int lfo2trig = 0, lfo2mode = 0, lfo2delay = 0;     // LFO2 retrigger + mode (normal/S&H) + Delay fade-in time (+0xd8)
    int lfo3sel = 0, lfo3wav = 0;                      // LFO3 rate-selector (+0x108, 0..174 → DAT_006dc4c0) + wave (+0x10c: 0=Tri 1=Sine)
    int o1wave = 0, o1shape = 0, o1tune = -2400, o2wave = 0, o2tune = -2400, o2shape = 0, o1sync = 0;
    int o1fm1s = 0, o1fm1d = 0, o1fm2s = 0, o1fm2d = 0, o1pwms = 0, o1pwmd = 0;
    int o2fm1s = 0, o2fm1d = 0, o2fm2s = 0, o2fm2d = 0, o2pwms = 0, o2pwmd = 0;
    // Mod Amplifiers (VAZ voice render @0x4de…: MA1 = in×AM×depth +SQ; MA2 = in×AM, full depth)
    int ma1in = 0, ma1sq = 0, ma1amsrc = 0, ma1amamt = 0, ma2in = 0, ma2amsrc = 0;
    // Env2 segment modulation (the +0x174 block, v2.0): source × amount → which env2 segments (dest bitfield)
    int e2modsrc = 0, e2modamt = 0, e2moddest = 0;
    int noise = 0, o1level = 255, o2level = 0, voiceMode = 0, portamento = 0, uniDetune = 0;
    int mix1src = 0, mix1post = 0, mix2src = 0, mix2post = 0, mix3src = 0, mix3post = 0;
    int bendRange = 2, uniVoices = 0;   // e0610 (1..24 st), e095c (2..16, v2.0 only; 0 = not present)
};

// Sequential cursor mirroring the VAZ stream primitives.
struct V2PCursor
{
    const juce::uint8* d; int n; int pos;
    int u32 ()  { int v = (pos + 4 <= n) ? (d[pos] | (d[pos+1] << 8) | (d[pos+2] << 16) | (d[pos+3] << 24)) : 0; pos += 4; return v; }
    int byte()  { int v = (pos >= 0 && pos < n) ? (int) d[pos] : 0; pos += 1; return v; }
    int modsrc(int ver) { int v = u32(); if (ver < 200 && v > 6) v += 1; return v; }
    void strsample() { byte(); byte(); int ln = u32(); pos += ln; }              // v<0x69/0x6a name path
    void skipMsmp()  { if (pos + 8 <= n && d[pos]=='M' && d[pos+1]=='S' && d[pos+2]=='m' && d[pos+3]=='p')
                           pos += 8 + (d[pos+4] | (d[pos+5] << 8) | (d[pos+6] << 16) | (d[pos+7] << 24)); }
};

static V2PPatch parseV2P (const juce::uint8* d, int n, int prst)
{
    V2PPatch p;
    p.ver = d[prst+8] | (d[prst+9] << 8) | (d[prst+10] << 16) | (d[prst+11] << 24);
    const int v = p.ver;
    V2PCursor c { d, n, prst + 12 };

    if (v >= 0x67) p.mono = c.byte();                 // preset_enable flag (reused slot)
    if (v >= 0x6d) c.u32();                            // voice_count
    if (v >= 0xc9) c.byte();                           // mono
    if (v >= 0xc9) c.u32();                            // +0x94
    p.lfo1rate = c.u32();
    { int w = c.u32(); if (v < 200) w = (w == 1) ? 4 : 0; p.lfo1wave = w; }   // lfo1 wave (ver<200: 1→Pulse else Saw)
    p.lfo1shape = c.u32(); p.lfo1trig = c.byte();      // lfo1 waveshape, retrigger
    if (v >= 0xc9) c.byte();                            // ded84
    if (v >= 0xc9) c.u32();                            // +0xe0
    p.lfo2rate = c.u32();                              // lfo2 rate
    if (v >= 200) { c.modsrc(v); c.u32(); }            // lfo2 trig src + depth
    p.lfo2trig = c.byte();                             // lfo2 retrigger
    if (v >= 200) p.lfo2mode = c.u32();                // v2.0: LFO2 waveform/mode (+0xd0, same LUT as LFO1)
    else          p.lfo2mode = (c.byte() != 0) ? 6 : 0;   // v1xx: S&H bool → 6 (S&H+Lag) / 0 (plain tri; VAZ's Delay is a separate param the clone bundles into +Delay waves)
    p.lfo2delay = c.u32(); p.lfo3sel = c.u32(); p.lfo3wav = c.byte();   // lfo2 delay (+0xd8); LFO3 rate-sel (+0x108) + wave (+0x10c)
    // env1
    if (v < 0x6b) { p.e1a = c.u32(); p.e1d = c.u32(); p.e1s = c.u32(); p.e1r = c.u32(); c.byte();
                    const int lin = c.byte();                         // linear/exp flag: ver<107 adds the rate-table offset at LOAD
                    if (lin == 0) { p.e1a += 0x3f; p.e1d += 0x94; p.e1r += 0x94; } else { p.e1d += 0x55; p.e1r += 0x55; } }
    else          { p.e1a = c.u32(); p.e1d = c.u32(); p.e1s = c.u32(); p.e1r = c.u32(); c.byte(); }
    p.e1mode = c.byte();                               // env1 def40 (PS+71 @ v201)
    if (v >= 0x6b) c.byte();
    if (v >= 0xca) c.byte();
    // env2
    if (v < 0x6c) { p.e2a = c.u32(); p.e2d = c.u32(); p.e2s = c.u32(); p.e2r = c.u32(); c.byte();
                    const int lin = c.byte();
                    if (lin == 0) { p.e2a += 0x3f; p.e2d += 0x94; p.e2r += 0x94; } else { p.e2d += 0x55; p.e2r += 0x55; } }
    else          { p.e2a = c.u32(); p.e2d = c.u32(); p.e2s = c.u32(); p.e2r = c.u32(); c.byte(); }
    c.byte();                                          // env2 df140
    if (v >= 0x6c) p.e2mode = c.byte(); else p.e2mode = 0;   // env2 def50 (PS+91 @ v201)
    if (v >= 0xca) c.byte();
    if (v >= 200) { p.e2modsrc = c.modsrc(v); p.e2modamt = c.u32(); p.e2moddest = c.u32(); }   // +0x174 block = Env2 segment modulation (src / amount / dest)
    p.ma1in = c.modsrc(v);                             // e0460 = Mod Amp 1 input source
    if (v >= 200) p.ma1sq = c.byte();                  // e0470 = Mod Amp 1 single-quadrant flag
    p.ma1amsrc = c.modsrc(v); p.ma1amamt = c.u32();    // e0480/e0494 = Mod Amp 1 AM source / depth
    if (v >= 200) p.ma2in    = c.modsrc(v);            // e04d4 = Mod Amp 2 input source
    if (v >= 200) p.ma2amsrc = c.modsrc(v);            // e04e4 = Mod Amp 2 AM source
    // osc1
    p.o1tune = c.u32(); p.o1wave = c.u32(); p.o1shape = c.u32();
    if (v >= 200) c.byte();                            // df430
    p.o1fm1s = c.modsrc(v); p.o1fm1d = c.u32();        // osc1 FM1 src/depth
    p.o1fm2s = c.modsrc(v); p.o1fm2d = c.u32();        // osc1 FM2 src/depth
    p.o1pwms = c.modsrc(v); p.o1pwmd = c.u32();        // osc1 PWM src/depth
    if (v < 0x69) c.strsample(); else { c.skipMsmp(); c.byte(); }   // osc1 sample (MSmp1)
    // osc2
    p.o2tune = c.u32(); p.o2wave = c.u32(); p.o1sync = c.byte(); p.o2shape = c.u32();   // osc2 tune, wave, osc-sync target, osc2 shape (df918/+0x1f0)
    p.o2fm1s = c.modsrc(v); p.o2fm1d = c.u32();        // osc2 FM1 src/depth
    p.o2fm2s = c.modsrc(v); p.o2fm2d = c.u32();        // osc2 FM2 src/depth
    p.o2pwms = c.modsrc(v); p.o2pwmd = c.u32();        // osc2 PWM src/depth
    if (v < 0x6a) c.strsample(); else { c.skipMsmp(); c.byte(); }   // osc2 sample (MSmp2)
    // filter / mixer / output (region C — anchored after MSmp2)
    if (v >= 200) p.mix1src = c.u32();                 // dfd2c = mixer ch1 source
    p.o1level = c.u32(); p.mix1post = c.byte();        // dfd5c = ch1 level (osc1), dfd4c = ch1 pre/post
    if (v >= 200) p.mix2src = c.u32();                 // dfde4 = mixer ch2 source
    p.o2level = c.u32(); p.mix2post = c.byte();        // dfe28 = ch2 level (osc2), dfe18 = ch2 pre/post
    p.mix3src = c.u32(); p.noise = c.u32(); p.mix3post = c.byte();  // dfeb0 = ch3 source, dfef4 = ch3 level (noise), dfee4 = ch3 pre/post
    p.filterMode = c.u32(); c.byte(); p.cutoff = c.u32(); p.reso = c.u32(); p.bandwidth = c.u32();
    if (v >= 200) p.hpCut = c.u32(); else p.hpCut = 0;
    p.fcut1s = c.modsrc(v); p.fcut1d = c.u32();
    p.fcut2s = c.modsrc(v); p.fcut2d = c.u32();
    p.fcut3s = c.modsrc(v); p.fcut3d = c.u32();        // cutoff mod 3
    p.fresS = c.modsrc(v); p.fresD = c.u32();
    p.am1s = c.modsrc(v); p.am1d = c.u32();            // amp AM1
    p.am2s = c.modsrc(v); p.am2d = c.u32();            // amp AM2
    if (v >= 200) { p.am3s = c.modsrc(v); p.am3d = c.u32(); }   // amp AM3 / pan mod
    p.overdrive = c.u32();
    if (v >= 0x65) c.modsrc(v);                        // e04f4
    if (v >= 0x65) c.u32();                            // e0504
    p.voiceMode = c.u32(); c.u32(); c.byte();          // e0530 (voice mode), e05f0, e0600
    p.bendRange = c.u32();                             // e0610 (pitch-bend range, semitones)
    if (v >= 200) p.uniVoices = c.u32();               // e095c (unison voice count, v2.0+)
    p.uniDetune = c.u32();                             // p2f4 (unison detune)
    if (v >= 200) c.u32();                             // p2f0
    p.portamento = c.u32();
    return p;
}
} // namespace

bool VAZCloneAudioProcessor::loadV2P (const juce::MemoryBlock& mb)
{
    const auto* d = (const juce::uint8*) mb.getData();
    const int n = (int) mb.getSize();
    const int prst = findTag (d, n, "PRST", 0);
    if (prst < 0 || prst + 12 > n) return false;

    const V2PPatch p = parseV2P (d, n, prst);
    auto S  = [&](const char* id, float v){ if (auto* q = apvts.getParameter (id)) q->setValueNotifyingHost (juce::jlimit (0.0f, 1.0f, v)); };
    auto SC = [&](const char* id, int idx){ S (id, juce::jlimit (0, 21, idx) / 21.0f); };       // mod-source index → choice
    // VAZ mod depths are SIGNED (FUN_004de75c stores |v| + a direction bit); map magnitude → depth, sign → invert toggle.
    auto SD = [&](const char* amtId, const char* invId, int v){ S (amtId, std::abs (v) / 255.0f);
                  if (auto* q = apvts.getParameter (invId)) q->setValueNotifyingHost (v < 0 ? 1.0f : 0.0f); };
    auto setTune = [&](int totalC, const char* oct, const char* coarse, const char* fine)
    {
        const int octSteps = juce::jlimit (-2, 2, (int) std::lround (totalC / 1200.0));
        const int rem      = totalC - octSteps * 1200;
        const int semis    = juce::jlimit (-12, 12, (int) std::lround (rem / 100.0));
        const int cents    = juce::jlimit (-100, 100, rem - semis * 100);
        S (oct, (2 + octSteps) / 4.0f);  S (coarse, 0.5f + semis / 24.0f);  S (fine, 0.5f + cents / 200.0f);
    };

    S (ParameterIDs::filter_mode, juce::jlimit (0, 21, p.filterMode) / 21.0f);
    S (ParameterIDs::cutoff,      p.cutoff / 255.0f);
    S (ParameterIDs::resonance,   p.reso / 255.0f);
    S (ParameterIDs::overdrive,   p.overdrive / 255.0f);
    S (ParameterIDs::noise_level, p.noise / 255.0f);
    S (ParameterIDs::o1_level,    p.o1level / 255.0f);   // osc1 level (mixer ch1) — was never loaded (stuck full)
    S (ParameterIDs::o2_level,    p.o2level / 255.0f);
    S (ParameterIDs::mix1_post, p.mix1post != 0 ? 1.0f : 0.0f);   // VAZ pre/post flag: !=0 → bypass filter (post)
    S (ParameterIDs::mix2_post, p.mix2post != 0 ? 1.0f : 0.0f);
    S (ParameterIDs::mix3_post, p.mix3post != 0 ? 1.0f : 0.0f);
    S (ParameterIDs::mix1_src,  juce::jlimit (0, 5, p.mix1src) / 5.0f);   // mixer source select (Osc/RingMod/Noise/…)
    S (ParameterIDs::mix2_src,  juce::jlimit (0, 5, p.mix2src) / 5.0f);
    S (ParameterIDs::mix3_src,  juce::jlimit (0, 5, p.mix3src) / 5.0f);   // 0=Noise 1=Osc3 2=RingMod (verified vs factory bank)
    S (ParameterIDs::voice_mode,  juce::jlimit (0, 2, p.voiceMode) / 2.0f);
    S (ParameterIDs::portamento,  p.portamento / 255.0f);
    S (ParameterIDs::uni_detune,  p.uniDetune / 255.0f);
    S (ParameterIDs::bend_range,  (juce::jlimit (1, 24, p.bendRange) - 1) / 23.0f);   // e0610: 1..24 st
    if (p.uniVoices > 0)                                                              // e095c only present in v2.0 patches
        S (ParameterIDs::uni_voices, (juce::jlimit (1, 32, p.uniVoices) - 1) / 31.0f);
    S (ParameterIDs::hp_cutoff,   p.hpCut / 255.0f);
    S (ParameterIDs::flt_aux,     p.bandwidth / 255.0f);
    S (ParameterIDs::e1_attack,   p.e1a / 425.0f);
    S (ParameterIDs::e1_decay,    p.e1d / 425.0f);
    S (ParameterIDs::e1_sustain,  p.e1s / 255.0f);
    S (ParameterIDs::e1_release,  p.e1r / 425.0f);
    S (ParameterIDs::e2_attack,   p.e2a / 425.0f);
    S (ParameterIDs::e2_decay,    p.e2d / 425.0f);
    S (ParameterIDs::e2_sustain,  p.e2s / 255.0f);
    S (ParameterIDs::e2_release,  p.e2r / 425.0f);
    S (ParameterIDs::lfo_rate,    p.lfo1rate / 255.0f);
    S (ParameterIDs::lfo2_rate,   p.lfo2rate / 255.0f);
    S (ParameterIDs::lfo_wave,    juce::jlimit (0, 7, p.lfo1wave) / 7.0f);   // LFO1 waveform (8 choices)
    S (ParameterIDs::lfo_shape,   p.lfo1shape / 255.0f);
    S (ParameterIDs::lfo_trig,    p.lfo1trig != 0 ? 1.0f : 0.0f);
    S (ParameterIDs::lfo2_wave,   juce::jlimit (0, 7, p.lfo2mode) / 7.0f);   // LFO2: normal/S&H (VAZ has no full LFO2 wave)
    S (ParameterIDs::lfo2_trig,   p.lfo2trig != 0 ? 1.0f : 0.0f);
    S (ParameterIDs::lfo2_delay,  p.lfo2delay / 255.0f);                     // LFO2 Delay fade-in time
    // LFO3: the 0..174 selector → exact rate (DAT_006dc4c0 is a clean exp 0.02·e^0.036·sel = 0.02..10.5 Hz),
    // inverse-mapped onto the clone's lfo3_rate law (0.05 + r²·20). Wave byte +0x10c: 0 = Triangle, else Sine.
    const double l3hz = 0.02 * std::exp (0.036 * (double) juce::jlimit (0, 255, p.lfo3sel));
    S (ParameterIDs::lfo3_rate,   (float) std::sqrt (juce::jmax (0.0, (l3hz - 0.05) / 20.0)));
    S (ParameterIDs::lfo3_wave,   p.lfo3wav != 0 ? 1.0f : 0.0f);             // 0=Tri, 1=Sine
    S (ParameterIDs::o1_wave,     juce::jlimit (0, 4, p.o1wave) / 4.0f);
    S (ParameterIDs::o1_shape,    juce::jlimit (0, 255, p.o1shape) / 255.0f);
    S (ParameterIDs::o2_wave,     juce::jlimit (0, 4, p.o2wave) / 4.0f);
    S (ParameterIDs::o2_shape,    juce::jlimit (0, 255, p.o2shape) / 255.0f);   // osc2 waveshape ("Modifier" df918/+0x1f0) — was never loaded
    S (ParameterIDs::osc2_sync,   p.o1sync != 0 ? 1.0f : 0.0f);                 // osc-sync target (df908) — independent of o2_wave
    setTune (p.o1tune + 2400, ParameterIDs::o1_octave, ParameterIDs::o1_coarse, ParameterIDs::o1_fine);
    setTune (p.o2tune + 2400, ParameterIDs::o2_octave, ParameterIDs::o2_coarse, ParameterIDs::o2_fine);
    SC (ParameterIDs::cut_mod1_src, p.fcut1s); SD (ParameterIDs::filt_env_amt, ParameterIDs::filt_env_amt_inv, p.fcut1d);
    SC (ParameterIDs::cut_mod2_src, p.fcut2s); SD (ParameterIDs::lfo_amt,      ParameterIDs::lfo_amt_inv,      p.fcut2d);
    SC (ParameterIDs::res_mod_src,  p.fresS);  SD (ParameterIDs::res_mod_amt,  ParameterIDs::res_mod_amt_inv,  p.fresD);
    SC (ParameterIDs::amp_mod_src,  p.am1s);   SD (ParameterIDs::amp_mod_amt,  ParameterIDs::amp_mod_amt_inv,  p.am1d);
    SC (ParameterIDs::pan_mod_src,  p.am3s);   SD (ParameterIDs::pan_mod_amt,  ParameterIDs::pan_mod_amt_inv,  p.am3d);
    SC (ParameterIDs::o1_fm_src,    p.o1fm1s); SD (ParameterIDs::o1_fm_amt,    ParameterIDs::o1_fm_amt_inv,    p.o1fm1d);
    SC (ParameterIDs::o1_ws_src,    p.o1pwms); SD (ParameterIDs::o1_ws_amt,    ParameterIDs::o1_ws_amt_inv,    p.o1pwmd);
    SC (ParameterIDs::o2_fm_src,    p.o2fm1s); SD (ParameterIDs::o2_fm_amt,    ParameterIDs::o2_fm_amt_inv,    p.o2fm1d);   // osc2 FM1 (was unmapped)
    SC (ParameterIDs::o2_ws_src,    p.o2pwms); SD (ParameterIDs::o2_ws_amt,    ParameterIDs::o2_ws_amt_inv,    p.o2pwmd);   // osc2 PWM (was unmapped)
    // Extra VAZ mod slots now loaded from the patch (osc FM2, filter cutoff mod 3, amp AM2).
    SC (ParameterIDs::o1_fm2_src,   p.o1fm2s); SD (ParameterIDs::o1_fm2_amt,   ParameterIDs::o1_fm2_amt_inv,   p.o1fm2d);
    SC (ParameterIDs::o2_fm2_src,   p.o2fm2s); SD (ParameterIDs::o2_fm2_amt,   ParameterIDs::o2_fm2_amt_inv,   p.o2fm2d);
    SC (ParameterIDs::cut_mod3_src, p.fcut3s); SD (ParameterIDs::cut_mod3_amt, ParameterIDs::cut_mod3_amt_inv, p.fcut3d);
    SC (ParameterIDs::amp_mod2_src, p.am2s);   SD (ParameterIDs::amp_mod2_amt, ParameterIDs::amp_mod2_amt_inv, p.am2d);
    // Mod Amplifiers 1 & 2 (VCAs that feed the mod bus) — now loaded from the patch.
    SC (ParameterIDs::ma1_in_src, p.ma1in);    SC (ParameterIDs::ma1_am_src, p.ma1amsrc);
    SD (ParameterIDs::ma1_am_amt, ParameterIDs::ma1_am_amt_inv, p.ma1amamt);
    S  (ParameterIDs::ma1_sq, p.ma1sq > 0 ? 1.0f : 0.0f);
    SC (ParameterIDs::ma2_in_src, p.ma2in);    SC (ParameterIDs::ma2_am_src, p.ma2amsrc);
    S  (ParameterIDs::ma2_am_amt, 1.0f);       // VAZ Mod Amp 2 = input × AM at full depth (no depth control)
    // Env2 segment modulation (VAZ +0x174 block): source × amount → env2 segment. The dest is a bitfield
    // (DAT_0052a8e4 maps the dropdown index → bit set: bit0=Attack bit1=Decay bit2=Release); the clone's
    // single e2_dest picks the first set segment. (v2.0 feature — earlier mislabeled as the "osc1 modifier".)
    static const int e2bf[8] = { 1, 2, 4, 3, 5, 6, 7, 9 };
    const int bf = e2bf[juce::jlimit (0, 7, p.e2moddest)];
    const int e2dest = (p.e2modsrc == 0 || p.e2modamt == 0) ? 4 : (bf & 1) ? 0 : (bf & 2) ? 1 : (bf & 4) ? 3 : 4;
    SC (ParameterIDs::e2_mod_src, p.e2modsrc);
    SD (ParameterIDs::e2_mod_amt, ParameterIDs::e2_mod_amt_inv, p.e2modamt);
    S  (ParameterIDs::e2_dest,    e2dest / 4.0f);
    // Env-mode bitfields (Multi=bit0, Reset=bit1, Cycle=bit2, Curve=bit3).
    const int em1 = p.e1mode, em2 = p.e2mode;
    S (ParameterIDs::e1_multi, (em1 & 1) ? 1.0f : 0.0f);  S (ParameterIDs::e1_reset, (em1 & 2) ? 1.0f : 0.0f);
    S (ParameterIDs::e1_cycle, (em1 & 4) ? 1.0f : 0.0f);  S (ParameterIDs::e1_curve, (em1 & 8) ? 1.0f : 0.0f);
    S (ParameterIDs::e2_multi, (em2 & 1) ? 1.0f : 0.0f);  S (ParameterIDs::e2_reset, (em2 & 2) ? 1.0f : 0.0f);
    S (ParameterIDs::e2_cycle, (em2 & 4) ? 1.0f : 0.0f);  S (ParameterIDs::e2_curve, (em2 & 8) ? 1.0f : 0.0f);
    S (ParameterIDs::lfo_sync,  p.mono > 0 ? 1.0f : 0.0f);

    lastPatchBytes = mb;     // keep the raw bytes as the Save template
    return true;
}

juce::MemoryBlock VAZCloneAudioProcessor::buildV2P()
{
    juce::MemoryBlock mb = lastPatchBytes.getSize() > 0
        ? lastPatchBytes : juce::MemoryBlock (kVAZInitTemplate, sizeof (kVAZInitTemplate));
    auto* d = (juce::uint8*) mb.getData();
    const int n = (int) mb.getSize();
    const int prst = findTag (d, n, "PRST", 0);
    const int ms1  = findTag (d, n, "MSmp", 0);
    const int ms2  = ms1 >= 0 ? findTag (d, n, "MSmp", ms1 + 4) : -1;
    if (prst < 0 || ms2 < 0) return mb;
    // These fixed write offsets are only correct for the v2.0 stream layout (ver 201/202). For the
    // older version-gated layouts the offsets shift, so writing them would corrupt the patch — return
    // the original bytes unchanged instead. (A full version-aware writer mirroring loadV2P is TODO.)
    const int ver = d[prst+8] | (d[prst+9] << 8) | (d[prst+10] << 16) | (d[prst+11] << 24);
    if (ver < 200) return mb;
    const int PS = prst + 12, sec3 = ms2 + 8 + 548;
    auto G   = [&](const char* id){ auto* p = apvts.getParameter (id); return p ? p->getValue() : 0.0f; };
    auto W   = [&](int o, int v){ if (o >= 0 && o < n) d[o] = (juce::uint8) juce::jlimit (0, 255, v); };
    auto W8  = [&](int o, float norm){ W (o, (int) std::lround (norm * 255.0f)); };
    auto W16 = [&](int o, float norm){ const int v = (int) std::lround (norm * 425.0f); W (o, v & 0xff); W (o + 1, (v >> 8) & 0xff); };
    auto GI  = [&](const char* id, int nch){ return (int) std::lround (G (id) * (nch - 1)); };

    W  (sec3+28, GI(ParameterIDs::filter_mode, 22));
    W8 (sec3+33, G(ParameterIDs::cutoff));
    W8 (sec3+37, G(ParameterIDs::resonance));
    W8 (sec3+105, G(ParameterIDs::overdrive));
    W8 (sec3+23, G(ParameterIDs::noise_level));
    W8 (sec3+14, G(ParameterIDs::o2_level));
    W  (sec3+117, GI(ParameterIDs::voice_mode, 3));
    W8 (sec3+142, G(ParameterIDs::portamento));
    W8 (sec3+134, G(ParameterIDs::uni_detune));
    W16(PS+54, G(ParameterIDs::e1_attack));  W16(PS+58, G(ParameterIDs::e1_decay));
    W8 (PS+62, G(ParameterIDs::e1_sustain)); W16(PS+66, G(ParameterIDs::e1_release));
    W16(PS+74, G(ParameterIDs::e2_attack));  W16(PS+78, G(ParameterIDs::e2_decay));
    W8 (PS+82, G(ParameterIDs::e2_sustain)); W16(PS+86, G(ParameterIDs::e2_release));
    W8 (PS+10, G(ParameterIDs::lfo_rate));
    W  (PS+131, GI(ParameterIDs::o1_wave, 5));
    W8 (PS+135, G(ParameterIDs::o1_shape));
    return mb;
}

void VAZCloneAudioProcessor::loadPatchDialog()
{
    patchChooser = std::make_unique<juce::FileChooser> ("Load VAZ Patch (.v2p)", juce::File(), "*.v2p");
    patchChooser->launchAsync (juce::FileBrowserComponent::openMode | juce::FileBrowserComponent::canSelectFiles,
        [this] (const juce::FileChooser& fc)
        {
            const auto f = fc.getResult();
            juce::MemoryBlock mb;
            if (f.existsAsFile() && f.loadFileAsData (mb)) loadV2P (mb);
        });
}

void VAZCloneAudioProcessor::savePatchDialog()
{
    patchChooser = std::make_unique<juce::FileChooser> ("Save VAZ Patch (.v2p)", juce::File(), "*.v2p");
    patchChooser->launchAsync (juce::FileBrowserComponent::saveMode | juce::FileBrowserComponent::canSelectFiles
                               | juce::FileBrowserComponent::warnAboutOverwriting,
        [this] (const juce::FileChooser& fc)
        {
            auto f = fc.getResult();
            if (f == juce::File()) return;
            if (! f.getFileExtension().equalsIgnoreCase (".v2p")) f = f.withFileExtension ("v2p");
            const auto mb = buildV2P();
            f.replaceWithData (mb.getData(), (size_t) mb.getSize());
        });
}

void VAZCloneAudioProcessor::loadSampleDialog (int osc, std::function<void (juce::String)> onDone)
{
    sampleChooser = std::make_unique<juce::FileChooser> ("Load Sample (WAV / AIFF / FLAC)", juce::File(), "*.wav;*.aif;*.aiff;*.flac");
    sampleChooser->launchAsync (juce::FileBrowserComponent::openMode | juce::FileBrowserComponent::canSelectFiles,
        [this, osc, onDone] (const juce::FileChooser& fc)
        {
            const auto f = fc.getResult();
            std::unique_ptr<juce::AudioFormatReader> reader (f != juce::File() ? sampleFormatMgr.createReaderFor (f) : nullptr);
            if (reader == nullptr) { if (onDone) onDone (juce::String()); return; }

            const int len = (int) reader->lengthInSamples, chans = (int) reader->numChannels;
            SampleData sd;
            if (len > 0 && chans > 0)
            {
                juce::AudioBuffer<float> buf (chans, len);
                reader->read (&buf, 0, len, 0, true, true);
                sd.data.resize ((size_t) len);
                for (int i = 0; i < len; ++i)
                {
                    float s = 0.0f;
                    for (int ch = 0; ch < chans; ++ch) s += buf.getSample (ch, i);
                    sd.data[(size_t) i] = s / (float) chans;            // mono mix
                }
                sd.sourceSR = reader->sampleRate > 0.0 ? reader->sampleRate : 44100.0;
                const int root = reader->metadataValues.getValue ("MidiUnityNote", "60").getIntValue();
                sd.rootHz = juce::MidiMessage::getMidiNoteInHertz (juce::jlimit (0, 127, root));
                if (reader->metadataValues.getValue ("NumSampleLoops", "0").getIntValue() > 0)   // WAV smpl chunk loop
                {
                    sd.loopStart = reader->metadataValues.getValue ("Loop0Start", "0").getIntValue();
                    sd.loopEnd   = reader->metadataValues.getValue ("Loop0End", juce::String (len)).getIntValue();
                    sd.hasLoop   = sd.loopEnd > sd.loopStart && sd.loopEnd <= len;
                }
                sd.name = f.getFileNameWithoutExtension();
            }
            { const juce::ScopedLock sl (getCallbackLock()); std::swap (osc == 1 ? osc2SampleData : osc1SampleData, sd); }
            if (onDone) onDone ((osc == 1 ? osc2SampleData : osc1SampleData).name);   // old data in sd freed here (outside lock)
        });
}

void VAZCloneAudioProcessor::resetSample (int osc)
{
    SampleData empty;
    const juce::ScopedLock sl (getCallbackLock());
    std::swap (osc == 1 ? osc2SampleData : osc1SampleData, empty);
}

// Arpeggiator: builds a clocked note sequence from held notes → MIDI for the synth.
void VAZCloneAudioProcessor::processArp (const juce::MidiBuffer& in, juce::MidiBuffer& out, int numSamples,
                                         double bpm, bool hold, int mode, int rate, int octs)
{
    const bool arpUni  = apvts.getRawParameterValue (ParameterIDs::voice_mode)->load() > 1.5f;   // Unison → each arp step plays N detuned voices
    const int  arpUniN = arpUni ? juce::jlimit (1, 32, (int) std::lround (1.0 + 31.0 * apvts.getRawParameterValue (ParameterIDs::uni_voices)->load())) : 1;
    for (const auto meta : in)                                  // 1) update held-note set
    {
        const auto m = meta.getMessage();
        if (m.isNoteOn())
        {
            const int n = m.getNoteNumber();
            if (std::find (arpHeld.begin(), arpHeld.end(), n) == arpHeld.end()) arpHeld.push_back (n);
            arpVel = (int) m.getVelocity();
        }
        else if (m.isNoteOff() && ! hold)
            arpHeld.erase (std::remove (arpHeld.begin(), arpHeld.end(), m.getNoteNumber()), arpHeld.end());
    }

    std::vector<int> seq;                                       // 2) build played order
    if (! arpHeld.empty())
    {
        std::vector<int> base (arpHeld.begin(), arpHeld.end());
        std::sort (base.begin(), base.end());
        for (int o = 0; o < juce::jlimit (1, 4, octs); ++o)
            for (int n : base) seq.push_back (n + 12 * o);
        if (mode == 1) std::reverse (seq.begin(), seq.end());                  // Down
        else if (mode == 2)                                                    // Up&Down (no repeated ends)
            for (int k = (int) seq.size() - 2; k >= 1; --k) seq.push_back (seq[(size_t) k]);
    }

    static const double stepBeatsTab[6] = { 1.0, 0.5, 1.0/3.0, 0.25, 1.0/6.0, 0.125 };
    const double stepSamples = bpm > 1.0
        ? (60.0 / bpm) * stepBeatsTab[juce::jlimit (0, 5, rate)] * getSampleRate() : 6000.0;

    for (int i = 0; i < numSamples; ++i)                        // 3) clock through the block
    {
        if (seq.empty())
        {
            if (arpSounding >= 0) { out.addEvent (juce::MidiMessage::noteOff (1, arpSounding), i); arpSounding = -1; }
            arpClock = 0.0; arpStep = 0;
            continue;
        }
        if (arpClock <= 0.0)
        {
            if (arpSounding >= 0) out.addEvent (juce::MidiMessage::noteOff (1, arpSounding), i);
            int idx;
            if (mode == 3) idx = modRng.nextInt ((int) seq.size());            // Random
            else { idx = arpStep % (int) seq.size(); ++arpStep; }
            const int note = juce::jlimit (0, 127, seq[(size_t) idx]);
            for (int u = 0; u < arpUniN; ++u)                                   // Unison: N voices per arp step
                out.addEvent (juce::MidiMessage::noteOn (1, note, (juce::uint8) juce::jlimit (1, 127, arpVel)), i);
            arpSounding = note;
            arpClock += stepSamples;
        }
        arpClock -= 1.0;
    }
}

void VAZCloneAudioProcessor::processBlock (juce::AudioBuffer<float>& buffer, juce::MidiBuffer& midi)
{
    juce::ScopedNoDenormals noDenormals;
    const juce::ScopedLock sl (getCallbackLock());   // serialise against the sample swap in loadSampleDialog/resetSample

    refreshVoiceParams();                       // snapshot params for the voices

    // ── Voice mode (Mono / Poly / Unison) + per-voice detune amount ──
    const int   vmode  = (int) apvts.getRawParameterValue (ParameterIDs::voice_mode)->load();
    const int   notePrio = (int) apvts.getRawParameterValue (ParameterIDs::note_priority)->load(); // 0=Last 1=High 2=Low
    const float detAmt = apvts.getRawParameterValue (ParameterIDs::uni_detune)->load();
    voiceParams.detuneCents = vmode == 2 ? (5.0f + detAmt * 45.0f) // Unison → chorus spread (min 5c so it's always audible)
                            : vmode == 1 ? detAmt * 12.0f       // Poly  → subtle analog drift
                            : 0.0f;                             // Mono  → none

    // Host tempo — used by the arpeggiator AND tempo-synced LFOs.
    double bpm = 120.0;
    if (auto* ph = getPlayHead())
        if (auto pos = ph->getPosition())
            if (auto b = pos->getBpm()) bpm = *b;
    const bool arpOn = apvts.getRawParameterValue (ParameterIDs::arp_on)->load() > 0.5f;
    synth.monoVoiceIdx = (! arpOn && vmode == 0) ? 0 : -1;   // MONO → dedicate voice 0 for legato glide; else normal voice allocation
    { const int vc = (int) apvts.getRawParameterValue (ParameterIDs::voices)->load();   // Voices: 0 = Dynamic (full pool), else fixed 1..32
      synth.voiceLimit = (vc <= 0) ? kNumVoices : juce::jmin (vc, kNumVoices); }

    juce::MidiBuffer voiced;
    if (arpOn)                                                  // ── ARPEGGIATOR ──
        processArp (midi, voiced, buffer.getNumSamples(), bpm,
                    apvts.getRawParameterValue (ParameterIDs::arp_hold)->load() > 0.5f,
                    (int) apvts.getRawParameterValue (ParameterIDs::arp_mode)->load(),
                    (int) apvts.getRawParameterValue (ParameterIDs::arp_rate)->load(),
                    (int) apvts.getRawParameterValue (ParameterIDs::arp_oct)->load() + 1);
    else {
    if (arpSounding >= 0) { synth.allNotesOff (1, true); arpSounding = -1; arpHeld.clear(); arpClock = 0.0; arpStep = 0; }
    if (vmode == 0)                                             // ── MONO: note priority (Last/High/Low) ──
    {
        auto pick = [&]() -> std::pair<int,int>                 // which held note should sound, or {-1,0}
        {
            if (heldNotes.empty()) return { -1, 0 };
            if (notePrio == 1) return *std::max_element (heldNotes.begin(), heldNotes.end(),
                                       [](auto& a, auto& b){ return a.first < b.first; });   // High
            if (notePrio == 2 || notePrio == 3) return *std::min_element (heldNotes.begin(), heldNotes.end(),
                                       [](auto& a, auto& b){ return a.first < b.first; });   // Low
            return heldNotes.back();                                                          // Last
        };
        for (const auto meta : midi)
        {
            const auto m = meta.getMessage(); const int sp = meta.samplePosition;
            if (m.isNoteOn())
                heldNotes.push_back ({ m.getNoteNumber(), (int) m.getVelocity() });
            else if (m.isNoteOff())
            {
                for (int k = (int) heldNotes.size() - 1; k >= 0; --k)
                    if (heldNotes[(size_t) k].first == m.getNoteNumber()) { heldNotes.erase (heldNotes.begin() + k); break; }
            }
            else { voiced.addEvent (m, sp); continue; }         // pass pitch-bend / CC straight through

            const auto s = pick();                              // recompute the sounding note per priority
            if (s.first != monoNote)
            {
                if (s.first >= 0)
                {
                    const int ch = (monoNote >= 0) ? 2 : 1;      // ch2 = legato (a note is already sounding); ch1 = fresh start
                    voiced.addEvent (juce::MidiMessage::noteOn (ch, s.first, (juce::uint8) s.second), sp);
                    if (monoNote < 0) monoStartNote = s.first;   // remember the note the voice was STARTED with (for the eventual note-off)
                }
                else if (monoStartNote >= 0)                      // all keys released → release the dedicated mono voice
                {
                    voiced.addEvent (juce::MidiMessage::noteOff (1, monoStartNote), sp);
                    monoStartNote = -1;
                }
                monoNote = s.first;
            }
        }
    }
    else if (vmode == 2)                                        // ── UNISON: N detuned voices/note ──
    {
        const int uniVoices = juce::jlimit (1, 32, (int) std::lround (1.0 + 31.0 * apvts.getRawParameterValue (ParameterIDs::uni_voices)->load()));
        for (const auto meta : midi)
        {
            const auto m = meta.getMessage(); const int sp = meta.samplePosition;
            if (m.isNoteOn()) { for (int u = 0; u < uniVoices; ++u) voiced.addEvent (m, sp); }
            else              voiced.addEvent (m, sp);
        }
    }
    }   // end !arpOn

    voiceParams.duoHighHz = (notePrio == 3 && vmode == 0 && ! heldNotes.empty())   // Duo: Osc2 = highest held note
        ? juce::MidiMessage::getMidiNoteInHertz (std::max_element (heldNotes.begin(), heldNotes.end(),
              [](auto& a, auto& b){ return a.first < b.first; })->first)
        : 0.0;
    const juce::MidiBuffer& useMidi = (! arpOn && vmode == 1) ? midi : voiced;   // Poly → pass-through

    modLfo .setTrig (apvts.getRawParameterValue (ParameterIDs::lfo_trig) ->load() > 0.5f);
    modLfo2.setTrig (apvts.getRawParameterValue (ParameterIDs::lfo2_trig)->load() > 0.5f);

    // ── MIDI tracking: filter-env gate + mod-source scalars (BEFORE render so the bus is ready) ──
    for (const auto meta : midi)
    {
        const auto m = meta.getMessage();
        if (m.isNoteOn())
        {
            if (activeNotes == 0) filterEnv.noteOn();
            ++activeNotes;
            randomVal    = modRng.nextFloat() * 2.0f - 1.0f;
            keyTrack     = juce::jlimit (0.0f, 1.0f, (m.getNoteNumber() - 24) / 72.0f);   // C1..C7
            lastVelocity = m.getFloatVelocity();
            modLfo.trigger(); modLfo2.trigger();                                          // LFO Trig (reset cycle/fade on note)
        }
        else if (m.isNoteOff()) { if (activeNotes > 0) --activeNotes; if (activeNotes == 0) filterEnv.noteOff(); }
        else if (m.isController() && m.getControllerNumber() == 1) modWheel = m.getControllerValue() / 127.0f;
        else if (m.isController() && m.getControllerNumber() == 11) midiCtrlB = m.getControllerValue() / 127.0f;
        else if (m.isChannelPressure()) aftertouch = m.getChannelPressureValue() / 127.0f;
    }

    // ── Filter / mod setup ──
    // Global filter envelope feeds ONLY the bus Env2 source — the actual filter is now per-voice (VAZVoice).
    const bool e2curve = apvts.getRawParameterValue (ParameterIDs::e2_curve)->load() > 0.5f;
    filterEnv.setADSR (apvts.getRawParameterValue (ParameterIDs::e2_attack)->load(),
                       apvts.getRawParameterValue (ParameterIDs::e2_decay)->load(),
                       apvts.getRawParameterValue (ParameterIDs::e2_sustain)->load(),
                       apvts.getRawParameterValue (ParameterIDs::e2_release)->load(), e2curve);
    filterEnv.setModes (apvts.getRawParameterValue (ParameterIDs::e2_reset)->load() > 0.5f,
                        apvts.getRawParameterValue (ParameterIDs::e2_cycle)->load() > 0.5f, e2curve);
    const int   w1 = (int) apvts.getRawParameterValue (ParameterIDs::lfo_wave)->load();
    const int   w2 = (int) apvts.getRawParameterValue (ParameterIDs::lfo2_wave)->load();
    const int   w3 = (int) apvts.getRawParameterValue (ParameterIDs::lfo3_wave)->load() == 0 ? 0 : 1; // LFO3 Tri/Sine
    const float shape1 = apvts.getRawParameterValue (ParameterIDs::lfo_shape) ->load();
    const float shape2 = apvts.getRawParameterValue (ParameterIDs::lfo2_shape)->load();
    // Host tempo for LFO sync (1 cycle = N beats; VAZ uses 1/24-quarter-note units).
    // (bpm already computed at the top of processBlock for the arpeggiator)
    static constexpr double periodBeats[24] = { 1.0/12, 1.0/8, 1.0/6, 1.0/4, 1.0/3, 1.0/2, 2.0/3, 1.0,
        2.0, 3.0, 4.0, 5.0, 6.0, 8.0, 12.0, 16.0, 24.0, 32.0, 48.0, 64.0, 96.0, 128.0, 192.0, 256.0 }; // VAZ list (beats/cycle)
    auto lfoRate = [&] (const char* rateId, const char* syncId, const char* periodId) -> double
    {
        if (apvts.getRawParameterValue (syncId)->load() > 0.5f)
        {
            int p = juce::jlimit (0, 23, (int) apvts.getRawParameterValue (periodId)->load());
            return (bpm / 60.0) / periodBeats[p];                       // tempo-synced Hz
        }
        return 0.05 + std::pow (apvts.getRawParameterValue (rateId)->load(), 2.0f) * 20.0; // free
    };
    modLfo .setRate (lfoRate (ParameterIDs::lfo_rate,  ParameterIDs::lfo_sync,  ParameterIDs::lfo_period),  currentSampleRate);
    const double lfo2Hz = lfoRate (ParameterIDs::lfo2_rate, ParameterIDs::lfo2_sync, ParameterIDs::lfo2_period);
    modLfo2.setRate (lfo2Hz, currentSampleRate);
    {   // LFO2 Delay (+0xd8): drives the fade-in time of the +Delay waveforms (was wrongly taken from WaveShape).
        const double dN = apvts.getRawParameterValue (ParameterIDs::lfo2_delay)->load();   // 0..1
        modLfo2.setDelay (dN * dN * 4.0);   // → 0..4 s (≈1 s at the bank's common value 127); time-curve approximate
    }
    const int   lfo2RmSrc = (int) apvts.getRawParameterValue (ParameterIDs::lfo2_rm_src)->load();
    const float lfo2RmAmt = apvts.getRawParameterValue (ParameterIDs::lfo2_rm_amt)->load();
    modLfo3.setRate (0.05 + std::pow (apvts.getRawParameterValue (ParameterIDs::lfo3_rate)->load(), 2.0f) * 20.0, currentSampleRate);

    const int   numSamples = buffer.getNumSamples();
    if ((size_t) numSamples > lfo1Buf.size())   // safety if host exceeds prepared block size
    {
        lfo1Buf.resize ((size_t) numSamples); lfo2Buf.resize ((size_t) numSamples);
        lfo3Buf.resize ((size_t) numSamples); env2Buf.resize ((size_t) numSamples);
        ma1Buf .resize ((size_t) numSamples); ma2Buf .resize ((size_t) numSamples);
        lagBuf .resize ((size_t) numSamples); noiseModBuf.resize ((size_t) numSamples);
    }

    // ── Pre-compute the mod-source bus for this block (LFO1/2/3 + Env2) — read by voices AND filter ──
    for (int i = 0; i < numSamples; ++i)
    {
        lfo1Buf[(size_t) i] = (float) modLfo .next (w1, (double) shape1);
        if (lfo2RmAmt > 0.0001f)   // LFO2 Rate Modulation — speeds up / slows down per sample (±2 oct)
            modLfo2.setRate (lfo2Hz * std::pow (2.0, (double) (lfo2RmAmt * 2.0f) * modBus.value (lfo2RmSrc, i)), currentSampleRate);
        lfo2Buf[(size_t) i] = (float) modLfo2.next (w2, (double) shape2);
        lfo3Buf[(size_t) i] = (float) modLfo3.nextSimple (w3);
        env2Buf[(size_t) i] = filterEnv.getNextSample();
    }
    modBus.lfo1 = lfo1Buf.data(); modBus.lfo2 = lfo2Buf.data(); modBus.lfo3 = lfo3Buf.data();
    modBus.env2 = env2Buf.data();
    modBus.modWheel = modWheel; modBus.velocity = lastVelocity; modBus.keyTrack = keyTrack;
    modBus.noise = noiseModBuf.data(); modBus.aftertouch = aftertouch; modBus.ctrlB = midiCtrlB;
    modBus.ma1 = nullptr; modBus.ma2 = nullptr;   // not computed yet → value(6/7)=0 while computing them

    // ── Mod Amplifiers 1/2 (VCAs: In × AM). ma1 reads primaries; ma2 may read ma1. ──
    const int   ma1In = (int) apvts.getRawParameterValue (ParameterIDs::ma1_in_src)->load();
    const int   ma1Am = (int) apvts.getRawParameterValue (ParameterIDs::ma1_am_src)->load();
    const float ma1Dp = apvts.getRawParameterValue (ParameterIDs::ma1_am_amt)->load() * (apvts.getRawParameterValue (ParameterIDs::ma1_am_amt_inv)->load() > 0.5f ? -1.0f : 1.0f);
    const bool  ma1Sq = apvts.getRawParameterValue (ParameterIDs::ma1_sq)->load() > 0.5f;
    const int   ma2In = (int) apvts.getRawParameterValue (ParameterIDs::ma2_in_src)->load();
    const int   ma2Am = (int) apvts.getRawParameterValue (ParameterIDs::ma2_am_src)->load();
    const float ma2Dp = apvts.getRawParameterValue (ParameterIDs::ma2_am_amt)->load() * (apvts.getRawParameterValue (ParameterIDs::ma2_am_amt_inv)->load() > 0.5f ? -1.0f : 1.0f);
    for (int i = 0; i < numSamples; ++i)
    {
        float in1 = modBus.value (ma1In, i); if (ma1Sq) in1 = (in1 + 1.0f) * 0.5f;   // SQ: single-quadrant (from bottom)
        ma1Buf[(size_t) i] = in1 * (ma1Am == 0 ? ma1Dp : ma1Dp * modBus.value (ma1Am, i));
    }
    modBus.ma1 = ma1Buf.data();
    for (int i = 0; i < numSamples; ++i)
        ma2Buf[(size_t) i] = modBus.value (ma2In, i) * (ma2Am == 0 ? ma2Dp : ma2Dp * modBus.value (ma2Am, i));
    modBus.ma2 = ma2Buf.data();

    // ── Lag Processor (slew limiter on a source → routable mod source) ──
    const int   lagIn   = (int) apvts.getRawParameterValue (ParameterIDs::lag_in_src)->load();
    const float lagT    = apvts.getRawParameterValue (ParameterIDs::lag_time)->load();
    const double lagCoef = 1.0 - std::exp (-1.0 / std::max (1.0, (double) lagT * lagT * 0.6 * currentSampleRate));
    for (int i = 0; i < numSamples; ++i)
    {
        lagState += (modBus.value (lagIn, i) - lagState) * lagCoef;
        lagBuf[(size_t) i] = (float) lagState;
    }
    modBus.lag = lagBuf.data();

    voiceParams.modBus = &modBus;

    // ── Render voices (they read the bus for FM) ──
    buffer.clear();
    synth.renderNextBlock (buffer, useMidi, 0, numSamples);

    // ── Filter + amp-mod are now applied PER VOICE (osc→filter→amp, inside VAZVoice). ──
    //    The voices already wrote the final filtered/amplified signal to all channels.
    //    Overdrive is the per-voice cubic soft-clip in VAZVoice (VAZ's output stage @0x4dbddc) — the old
    //    master-bus tanh stage was removed (it double-saturated and used the wrong shape).
}

//==============================================================================
juce::AudioProcessorEditor* VAZCloneAudioProcessor::createEditor()
{
   #ifdef VAZ_HEADLESS
    return nullptr;                                  // headless renderer: no WebView editor
   #else
    return new VAZCloneAudioProcessorEditor (*this);
   #endif
}

//==============================================================================
void VAZCloneAudioProcessor::getStateInformation (juce::MemoryBlock& destData)
{
    // Binary wrapper tree: APVTS params + the two loaded samples (so samples persist with the project).
    auto sampleTree = [] (const juce::Identifier& tag, const SampleData& sd)
    {
        juce::ValueTree t (tag);
        if (sd.loaded())
        {
            t.setProperty ("name", sd.name, nullptr);
            t.setProperty ("sr",   sd.sourceSR, nullptr);
            t.setProperty ("root", sd.rootHz, nullptr);
            t.setProperty ("ls",   sd.loopStart, nullptr);
            t.setProperty ("le",   sd.loopEnd, nullptr);
            t.setProperty ("hl",   sd.hasLoop, nullptr);
            t.setProperty ("data", juce::MemoryBlock (sd.data.data(), sd.data.size() * sizeof (float)), nullptr);
        }
        return t;
    };
    juce::ValueTree root ("VAZState");
    root.appendChild (apvts.copyState(), nullptr);
    root.appendChild (sampleTree ("Smp1", osc1SampleData), nullptr);
    root.appendChild (sampleTree ("Smp2", osc2SampleData), nullptr);
    juce::MemoryOutputStream mos (destData, false);
    root.writeToStream (mos);
}

void VAZCloneAudioProcessor::setStateInformation (const void* data, int sizeInBytes)
{
    auto restore = [this] (SampleData& dst, const juce::ValueTree& t)
    {
        SampleData sd;
        if (t.isValid() && t.hasProperty ("data"))
        {
            sd.name      = t.getProperty ("name").toString();
            sd.sourceSR  = (double) t.getProperty ("sr");
            sd.rootHz    = (double) t.getProperty ("root");
            sd.loopStart = (int)    t.getProperty ("ls");
            sd.loopEnd   = (int)    t.getProperty ("le");
            sd.hasLoop   = (bool)   t.getProperty ("hl");
            if (auto* mb = t.getProperty ("data").getBinaryData())
            {
                const int n = (int) (mb->getSize() / sizeof (float));
                sd.data.resize ((size_t) n);
                const float* src = static_cast<const float*> (mb->getData());
                for (int i = 0; i < n; ++i) sd.data[(size_t) i] = src[i];
            }
        }
        const juce::ScopedLock sl (getCallbackLock());
        std::swap (dst, sd);                          // old data freed in sd after the lock scope
    };

    auto tree = juce::ValueTree::readFromData (data, (size_t) sizeInBytes);
    if (tree.isValid() && tree.hasType ("VAZState"))                       // new format (params + samples)
    {
        auto params = tree.getChildWithName (apvts.state.getType());
        if (params.isValid()) apvts.replaceState (params);
        restore (osc1SampleData, tree.getChildWithName ("Smp1"));
        restore (osc2SampleData, tree.getChildWithName ("Smp2"));
        return;
    }
    if (auto xml = getXmlFromBinary (data, sizeInBytes))                   // old format (params only) — backward compatible
        if (xml->hasTagName (apvts.state.getType()))
            apvts.replaceState (juce::ValueTree::fromXml (*xml));
}

//==============================================================================
juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter()
{
    return new VAZCloneAudioProcessor();
}
