#pragma once
// VAZClone parameter IDs — shared between processor, editor relays and the WebView UI.
// IDs match the control IDs in Source/ui/public/index.html and .ideas/parameter-spec.md.

namespace ParameterIDs
{
    // Oscillator 1
    constexpr auto o1_octave  = "o1_octave";   // choice: 32'/16'/8'/4'/2'
    constexpr auto o1_wave    = "o1_wave";     // choice: Sawtooth/Pulse/Multi-Saw/Sample/Ext
    constexpr auto o1_coarse  = "o1_coarse";   // float 0..1 (center 0.5) → ±12 semitones
    constexpr auto o1_fine    = "o1_fine";     // float 0..1 (center 0.5) → ±1 semitone (±100 cents)
    constexpr auto o1_fm_src  = "o1_fm_src";   // choice: FM source (mod-bus index)
    constexpr auto o1_fm_amt  = "o1_fm_amt";   // float 0..1 FM depth
    constexpr auto o1_ws_src  = "o1_ws_src";   // choice: Waveshape Mod source
    constexpr auto o1_ws_amt  = "o1_ws_amt";   // float 0..1 Waveshape mod depth
    constexpr auto o1_shape   = "o1_shape";    // float 0..1 Waveshape (morph/pw/detune)
    constexpr auto o1_level   = "o1_level";    // float 0..1
    // Oscillator 2
    constexpr auto o2_octave  = "o2_octave";   // choice: 32'/16'/8'/4'/2'
    constexpr auto o2_wave    = "o2_wave";     // choice (Ext→Sync)
    constexpr auto o2_coarse  = "o2_coarse";   // float 0..1 (center 0.5) → ±12 semitones
    constexpr auto o2_fine    = "o2_fine";     // float 0..1 (center 0.5) → ±1 semitone (±100 cents)
    constexpr auto o2_fm_src  = "o2_fm_src";   // choice: FM source (mod-bus index)
    constexpr auto o2_fm_amt  = "o2_fm_amt";   // float 0..1 FM depth
    constexpr auto o2_ws_src  = "o2_ws_src";   // choice: Waveshape Mod source
    constexpr auto o2_ws_amt  = "o2_ws_amt";   // float 0..1 Waveshape mod depth
    constexpr auto o2_shape   = "o2_shape";    // float 0..1 Waveshape
    constexpr auto o2_detune  = "o2_detune";   // float 0..1
    constexpr auto o2_level   = "o2_level";    // float 0..1
    // Noise
    constexpr auto noise_level = "noise_level"; // float 0..1
    // Mixer (per-channel source select)
    constexpr auto mix1_src = "mix1_src";  // ch1: Osc1/RingMod/Noise/Ext/MA1/MA2
    constexpr auto mix2_src = "mix2_src";  // ch2: Osc2/RingMod/Noise/Ext/MA1/MA2
    constexpr auto mix3_src = "mix3_src";  // ch3: Noise/Osc3/RingMod/Ext/MA1/MA2
    constexpr auto mix1_post = "mix1_post"; constexpr auto mix2_post = "mix2_post"; constexpr auto mix3_post = "mix3_post"; // bool: post-filter
    // Filter
    constexpr auto filter_mode = "filter_mode"; // choice 0..21
    constexpr auto cutoff      = "cutoff";      // float 0..1
    constexpr auto resonance   = "resonance";   // float 0..1
    constexpr auto hp_cutoff   = "hp_cutoff";   // float 0..1
    constexpr auto flt_aux     = "flt_aux";     // float 0..1  slot-3 fader: Bandwidth (B) / Separation (C,D)
    // Amplifier
    constexpr auto overdrive   = "overdrive";   // float 0..1
    constexpr auto amp_mod_src = "amp_mod_src"; // choice: amplitude mod source (tremolo)
    constexpr auto amp_mod_amt = "amp_mod_amt"; // float 0..1 amplitude mod depth
    constexpr auto amp_level   = "amp_level";   // float 0..1 Amplitude-Mod slot-1 depth = master volume
    constexpr auto pan_mod_src = "pan_mod_src"; // choice: pan mod source
    constexpr auto pan_mod_amt = "pan_mod_amt"; // float 0..1 pan mod depth
    // Modulation sign toggles (− = inverted) for the bipolar mod slots
    constexpr auto filt_env_amt_inv = "filt_env_amt_inv";
    constexpr auto lfo_amt_inv      = "lfo_amt_inv";
    constexpr auto res_mod_amt_inv  = "res_mod_amt_inv";
    constexpr auto amp_mod_amt_inv  = "amp_mod_amt_inv";
    constexpr auto pan_mod_amt_inv  = "pan_mod_amt_inv";
    constexpr auto o1_fm_amt_inv    = "o1_fm_amt_inv";
    constexpr auto o2_fm_amt_inv    = "o2_fm_amt_inv";
    constexpr auto o1_ws_amt_inv    = "o1_ws_amt_inv";
    constexpr auto o2_ws_amt_inv    = "o2_ws_amt_inv";
    constexpr auto e2_mod_amt_inv   = "e2_mod_amt_inv";
    constexpr auto ma1_am_amt_inv   = "ma1_am_amt_inv";
    constexpr auto ma2_am_amt_inv   = "ma2_am_amt_inv";
    constexpr auto osc_link    = "osc_link";    // bool: Osc1 FM input 1 → also Osc2
    constexpr auto osc2_sync   = "osc2_sync";   // bool: hard-sync Osc2 to Osc1 (VAZ "Sync Target", independent of o2_wave)
    constexpr auto ma1_sq      = "ma1_sq";      // bool: Mod Amp 1 single-quadrant
    constexpr auto e2_mod_src  = "e2_mod_src";  // Env2 envelope-mod source
    constexpr auto e2_mod_amt  = "e2_mod_amt";  // Env2 envelope-mod depth
    constexpr auto e2_dest     = "e2_dest";     // Env2 mod dest: Attack/Decay/Sustain/Release/None
    // Envelope 1 (amp)
    constexpr auto e1_attack   = "e1_attack";
    constexpr auto e1_decay    = "e1_decay";
    constexpr auto e1_sustain  = "e1_sustain";
    constexpr auto e1_release  = "e1_release";
    // Envelope 2 (mod/filter)
    constexpr auto e2_attack   = "e2_attack";
    constexpr auto e2_decay    = "e2_decay";
    constexpr auto e2_sustain  = "e2_sustain";
    constexpr auto e2_release  = "e2_release";
    constexpr auto filt_env_amt = "filt_env_amt";  // cutoff mod 1 DEPTH 0..1
    // LFO 1 / 2 / 3
    constexpr auto lfo_rate    = "lfo_rate";        // LFO1 rate 0..1
    constexpr auto lfo_wave    = "lfo_wave";        // LFO1 wave choice
    constexpr auto lfo_amt     = "lfo_amt";         // cutoff mod 2 DEPTH 0..1
    constexpr auto lfo2_rate   = "lfo2_rate";       // LFO2 rate 0..1
    constexpr auto lfo2_wave   = "lfo2_wave";       // LFO2 wave choice
    constexpr auto lfo3_rate   = "lfo3_rate";       // LFO3 rate 0..1
    constexpr auto lfo3_wave   = "lfo3_wave";       // LFO3 wave: Tri/Sine
    constexpr auto lfo_shape   = "lfo_shape";       // LFO1 variation (morph/fade/lag) 0..1
    constexpr auto lfo2_shape  = "lfo2_shape";      // LFO2 variation 0..1
    constexpr auto lfo2_delay  = "lfo2_delay";      // LFO2 Delay = fade-in time for the +Delay waves (.v2p +0xd8) 0..1
    constexpr auto lfo_trig    = "lfo_trig";        // LFO1 retrigger on note
    constexpr auto lfo2_trig   = "lfo2_trig";       // LFO2 retrigger on note
    constexpr auto lfo_sync    = "lfo_sync";        // LFO1 tempo-sync on/off
    constexpr auto lfo_period  = "lfo_period";      // LFO1 sync period (note length)
    constexpr auto lfo2_sync   = "lfo2_sync";       // LFO2 tempo-sync on/off
    constexpr auto lfo2_period = "lfo2_period";     // LFO2 sync period
    constexpr auto lfo2_rm_src = "lfo2_rm_src";     // LFO2 rate-mod source
    constexpr auto lfo2_rm_amt = "lfo2_rm_amt";     // LFO2 rate-mod depth
    // Modulation matrix — selectable sources for the filter (the keystone of the mod system)
    constexpr auto cut_mod1_src = "cut_mod1_src";   // choice: None/LFO1/2/3/Env2/ModWheel/Random/KeyTrack
    constexpr auto cut_mod2_src = "cut_mod2_src";   // choice (same enum)
    constexpr auto res_mod_src  = "res_mod_src";    // choice (same enum)
    constexpr auto res_mod_amt  = "res_mod_amt";    // resonance mod DEPTH 0..1
    // Extra VAZ modulation slots (VAZ: 2 FM/osc, 3 cutoff mods, 2 amp AM sources — used by 20-30% of factory patches)
    constexpr auto o1_fm2_src   = "o1_fm2_src";   constexpr auto o1_fm2_amt   = "o1_fm2_amt";   constexpr auto o1_fm2_amt_inv   = "o1_fm2_amt_inv";   // Osc1 FM source 2
    constexpr auto o2_fm2_src   = "o2_fm2_src";   constexpr auto o2_fm2_amt   = "o2_fm2_amt";   constexpr auto o2_fm2_amt_inv   = "o2_fm2_amt_inv";   // Osc2 FM source 2
    constexpr auto cut_mod3_src = "cut_mod3_src"; constexpr auto cut_mod3_amt = "cut_mod3_amt"; constexpr auto cut_mod3_amt_inv = "cut_mod3_amt_inv"; // Filter cutoff mod 3
    constexpr auto amp_mod2_src = "amp_mod2_src"; constexpr auto amp_mod2_amt = "amp_mod2_amt"; constexpr auto amp_mod2_amt_inv = "amp_mod2_amt_inv"; // Amp AM source 2
    // Mod Amplifiers 1 & 2 (VCAs: In × AM → routable mod source)
    constexpr auto ma1_in_src  = "ma1_in_src";  // choice: input source
    constexpr auto ma1_am_src  = "ma1_am_src";  // choice: amplitude-mod source
    constexpr auto ma1_am_amt  = "ma1_am_amt";  // float 0..1 AM depth
    constexpr auto ma2_in_src  = "ma2_in_src";
    constexpr auto ma2_am_src  = "ma2_am_src";
    constexpr auto ma2_am_amt  = "ma2_am_amt";
    // Lag Processor (slew on a source → routable mod source)
    constexpr auto lag_in_src  = "lag_in_src"; // choice
    constexpr auto lag_time    = "lag_time";   // float 0..1 slew time
    // Envelope modes (Multi/Reset/Curve/Cycle) per envelope
    constexpr auto e1_reset = "e1_reset"; constexpr auto e1_cycle = "e1_cycle"; constexpr auto e1_curve = "e1_curve";
    constexpr auto e2_reset = "e2_reset"; constexpr auto e2_cycle = "e2_cycle"; constexpr auto e2_curve = "e2_curve";
    constexpr auto e1_multi = "e1_multi"; constexpr auto e2_multi = "e2_multi";   // Multi-trigger (legato re-attack)
    // Note priority (mono/unison)
    constexpr auto note_priority = "note_priority"; // choice: Last/High/Low
    // Performance
    constexpr auto voice_mode  = "voice_mode";  // choice: Mono/Poly/Unison/Duo
    constexpr auto uni_detune  = "uni_detune";  // float 0..1
    constexpr auto portamento  = "portamento";  // float 0..1
    constexpr auto bend_range  = "bend_range";  // float 0..1 → 1..24 semitones
    constexpr auto uni_voices  = "uni_voices";  // float 0..1 → 1..16 unison voices
    constexpr auto porta_exp   = "porta_exp";   // bool: exponential glide (off = linear constant-rate)
    constexpr auto porta_auto  = "porta_auto";  // bool: Portamento Auto (glide only on overlapping notes)
    // Arpeggiator
    constexpr auto arp_on   = "arp_on";    // bool
    constexpr auto arp_mode = "arp_mode";  // choice: Up/Down/Up&Down/Random
    constexpr auto arp_rate = "arp_rate";  // choice: 1/4 .. 1/32
    constexpr auto arp_oct  = "arp_oct";   // choice: 1..4 octaves
    constexpr auto arp_hold = "arp_hold";  // bool
}
