#pragma once
//==============================================================================
// MidBass parameter IDs — the COMPLETE parameter surface, fixed in Phase 0.
// Ranges/defaults live in Params.h (createParameterLayout); this file is only
// the ID strings so DSP, GUI and tests all reference one spelling.
//==============================================================================
namespace mb
{
namespace pid
{
    // ---- global / voice ----
    inline constexpr auto output        = "output";         // dB
    inline constexpr auto voice_mode    = "voice_mode";     // choice: Retrig / Legato
    inline constexpr auto glide_time    = "glide_time";     // ms
    inline constexpr auto glide_legato  = "glide_legato";   // bool: glide only on legato notes
    inline constexpr auto voice_stack   = "voice_stack";    // choice: 1x / 2x / 4x
    inline constexpr auto bend_range    = "bend_range";     // semitones

    // ---- osc 1..3 (index with oscParam(n, suffix)) ----
    inline constexpr auto osc1_wave  = "osc1_wave";   // choice: Saw / Pulse / Triangle
    inline constexpr auto osc1_oct   = "osc1_oct";    // -2..+2
    inline constexpr auto osc1_semi  = "osc1_semi";   // -12..+12
    inline constexpr auto osc1_fine  = "osc1_fine";   // cents
    inline constexpr auto osc1_pw    = "osc1_pw";     // %
    inline constexpr auto osc1_level = "osc1_level";  // %
    inline constexpr auto osc2_wave  = "osc2_wave";
    inline constexpr auto osc2_oct   = "osc2_oct";
    inline constexpr auto osc2_semi  = "osc2_semi";
    inline constexpr auto osc2_fine  = "osc2_fine";
    inline constexpr auto osc2_pw    = "osc2_pw";
    inline constexpr auto osc2_level = "osc2_level";
    inline constexpr auto osc3_wave  = "osc3_wave";
    inline constexpr auto osc3_oct   = "osc3_oct";
    inline constexpr auto osc3_semi  = "osc3_semi";
    inline constexpr auto osc3_fine  = "osc3_fine";
    inline constexpr auto osc3_pw    = "osc3_pw";
    inline constexpr auto osc3_level = "osc3_level";

    // ---- osc interactions ----
    inline constexpr auto osc_sync  = "osc_sync";    // bool: hard sync osc2 -> osc1
    inline constexpr auto osc_fm    = "osc_fm";      // % (osc2 -> osc1, subtle range)
    inline constexpr auto osc_ring  = "osc_ring";    // % (osc1 x osc2 into mix)
    inline constexpr auto osc_drift = "osc_drift";   // % analog drift amount

    // ---- sub ----
    inline constexpr auto sub_on    = "sub_on";      // bool
    inline constexpr auto sub_wave  = "sub_wave";    // choice: Sine / Triangle / Square
    inline constexpr auto sub_oct   = "sub_oct";     // choice: -1 / -2
    inline constexpr auto sub_level = "sub_level";   // %

    // ---- unison ----
    inline constexpr auto uni_voices = "uni_voices"; // 1..8
    inline constexpr auto uni_detune = "uni_detune"; // %
    inline constexpr auto uni_spread = "uni_spread"; // % stereo
    inline constexpr auto uni_mono   = "uni_mono";   // bool: sum unison to mono

    // ---- filter ----
    inline constexpr auto flt_mode       = "flt_mode";       // choice: LP24 / LP12 / HP12 / BP12
    inline constexpr auto flt_cutoff     = "flt_cutoff";     // Hz (log)
    inline constexpr auto flt_reso       = "flt_reso";       // %
    inline constexpr auto flt_keytrack   = "flt_keytrack";   // %
    inline constexpr auto flt_env_amt    = "flt_env_amt";    // -100..+100 % (bipolar)
    inline constexpr auto flt_drive_pre  = "flt_drive_pre";  // %
    inline constexpr auto flt_drive_post = "flt_drive_post"; // %

    // ---- filter env (very fast capable) ----
    inline constexpr auto fenv_a = "fenv_a";  // ms, 0.05..2000
    inline constexpr auto fenv_d = "fenv_d";  // ms, 2..2000
    inline constexpr auto fenv_s = "fenv_s";  // %
    inline constexpr auto fenv_r = "fenv_r";  // ms

    // ---- amp env ----
    inline constexpr auto aenv_a = "aenv_a";
    inline constexpr auto aenv_d = "aenv_d";
    inline constexpr auto aenv_s = "aenv_s";
    inline constexpr auto aenv_r = "aenv_r";

    // ---- LFO 1..2 ----
    inline constexpr auto lfo1_wave    = "lfo1_wave";    // choice: Sine/Triangle/Saw/Square/S&H
    inline constexpr auto lfo1_sync    = "lfo1_sync";    // bool: tempo sync
    inline constexpr auto lfo1_rate_hz = "lfo1_rate_hz"; // Hz (free)
    inline constexpr auto lfo1_rate_div= "lfo1_rate_div";// choice (synced divisions)
    inline constexpr auto lfo1_amount  = "lfo1_amount";  // %
    inline constexpr auto lfo1_retrig  = "lfo1_retrig";  // bool: retrigger on note-on
    inline constexpr auto lfo1_dest    = "lfo1_dest";    // choice: Cutoff/Pitch/PWM/Volume
    inline constexpr auto lfo2_wave    = "lfo2_wave";
    inline constexpr auto lfo2_sync    = "lfo2_sync";
    inline constexpr auto lfo2_rate_hz = "lfo2_rate_hz";
    inline constexpr auto lfo2_rate_div= "lfo2_rate_div";
    inline constexpr auto lfo2_amount  = "lfo2_amount";
    inline constexpr auto lfo2_retrig  = "lfo2_retrig";
    inline constexpr auto lfo2_dest    = "lfo2_dest";

    // ---- mod matrix (6 slots) ----
    inline constexpr auto mod1_src = "mod1_src"; inline constexpr auto mod1_dst = "mod1_dst"; inline constexpr auto mod1_amt = "mod1_amt";
    inline constexpr auto mod2_src = "mod2_src"; inline constexpr auto mod2_dst = "mod2_dst"; inline constexpr auto mod2_amt = "mod2_amt";
    inline constexpr auto mod3_src = "mod3_src"; inline constexpr auto mod3_dst = "mod3_dst"; inline constexpr auto mod3_amt = "mod3_amt";
    inline constexpr auto mod4_src = "mod4_src"; inline constexpr auto mod4_dst = "mod4_dst"; inline constexpr auto mod4_amt = "mod4_amt";
    inline constexpr auto mod5_src = "mod5_src"; inline constexpr auto mod5_dst = "mod5_dst"; inline constexpr auto mod5_amt = "mod5_amt";
    inline constexpr auto mod6_src = "mod6_src"; inline constexpr auto mod6_dst = "mod6_dst"; inline constexpr auto mod6_amt = "mod6_amt";

    // ---- saturation (post-filter, 2x oversampled) ----
    inline constexpr auto sat_type  = "sat_type";   // choice: Tape/Tube/Diode/Soft Clip/Hard Clip
    inline constexpr auto sat_drive = "sat_drive";  // %
    inline constexpr auto sat_mix   = "sat_mix";    // % parallel

    // ---- EQ ----
    inline constexpr auto eq_ls_freq  = "eq_ls_freq";  // Hz
    inline constexpr auto eq_ls_gain  = "eq_ls_gain";  // dB
    inline constexpr auto eq_mid_freq = "eq_mid_freq"; // Hz
    inline constexpr auto eq_mid_gain = "eq_mid_gain"; // dB
    inline constexpr auto eq_mid_q    = "eq_mid_q";    // Q
    inline constexpr auto eq_hs_freq  = "eq_hs_freq";  // Hz
    inline constexpr auto eq_hs_gain  = "eq_hs_gain";  // dB

    // ---- transient shaper ----
    inline constexpr auto trans_attack  = "trans_attack";  // -100..+100
    inline constexpr auto trans_sustain = "trans_sustain"; // -100..+100

    // ---- macros ----
    inline constexpr auto macro_punch  = "macro_punch";
    inline constexpr auto macro_bite   = "macro_bite";
    inline constexpr auto macro_warmth = "macro_warmth";
    inline constexpr auto macro_snap   = "macro_snap";
    inline constexpr auto macro_body   = "macro_body";
    inline constexpr auto macro_width  = "macro_width";

    // ---- FX: chorus ----
    inline constexpr auto fx_cho_on    = "fx_cho_on";
    inline constexpr auto fx_cho_rate  = "fx_cho_rate";   // Hz
    inline constexpr auto fx_cho_depth = "fx_cho_depth";  // %
    inline constexpr auto fx_cho_mix   = "fx_cho_mix";    // %
    // ---- FX: phaser ----
    inline constexpr auto fx_pha_on    = "fx_pha_on";
    inline constexpr auto fx_pha_rate  = "fx_pha_rate";
    inline constexpr auto fx_pha_depth = "fx_pha_depth";
    inline constexpr auto fx_pha_fb    = "fx_pha_fb";
    inline constexpr auto fx_pha_mix   = "fx_pha_mix";
    // ---- FX: flanger ----
    inline constexpr auto fx_fla_on    = "fx_fla_on";
    inline constexpr auto fx_fla_rate  = "fx_fla_rate";
    inline constexpr auto fx_fla_depth = "fx_fla_depth";
    inline constexpr auto fx_fla_fb    = "fx_fla_fb";
    inline constexpr auto fx_fla_mix   = "fx_fla_mix";
    // ---- FX: delay (tempo-synced) ----
    inline constexpr auto fx_dly_on   = "fx_dly_on";
    inline constexpr auto fx_dly_div  = "fx_dly_div";   // choice: 1/4 .. 3/16
    inline constexpr auto fx_dly_fb   = "fx_dly_fb";    // %
    inline constexpr auto fx_dly_damp = "fx_dly_damp";  // %
    inline constexpr auto fx_dly_mix  = "fx_dly_mix";   // %
    inline constexpr auto fx_dly_ping = "fx_dly_ping";  // bool
    // ---- FX: reverb ----
    inline constexpr auto fx_rev_on   = "fx_rev_on";
    inline constexpr auto fx_rev_size = "fx_rev_size";
    inline constexpr auto fx_rev_damp = "fx_rev_damp";
    inline constexpr auto fx_rev_mix  = "fx_rev_mix";
    // ---- FX: compressor ----
    inline constexpr auto fx_cmp_on     = "fx_cmp_on";
    inline constexpr auto fx_cmp_thresh = "fx_cmp_thresh"; // dB
    inline constexpr auto fx_cmp_ratio  = "fx_cmp_ratio";
    inline constexpr auto fx_cmp_att    = "fx_cmp_att";    // ms
    inline constexpr auto fx_cmp_rel    = "fx_cmp_rel";    // ms
    inline constexpr auto fx_cmp_gain   = "fx_cmp_gain";   // dB makeup

    // Expected total — asserted by the Phase 0 test so a param can't silently vanish.
    inline constexpr int kExpectedParamCount = 131;
} // namespace pid
} // namespace mb
