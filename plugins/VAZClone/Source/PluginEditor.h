#pragma once

#include <juce_audio_processors/juce_audio_processors.h>
#include <juce_gui_extra/juce_gui_extra.h>
#include "PluginProcessor.h"
#include "ParameterIDs.hpp"

//==============================================================================
// VAZClone editor — WebView UI.
// CRITICAL member order: Relays → WebBrowserComponent → Attachments
// (C++ destroys in reverse; WebView must die before the relays it references).
//==============================================================================
class VAZCloneAudioProcessorEditor : public juce::AudioProcessorEditor
{
public:
    explicit VAZCloneAudioProcessorEditor (VAZCloneAudioProcessor&);
    ~VAZCloneAudioProcessorEditor() override;

    void paint (juce::Graphics&) override;
    void resized() override;

private:
    VAZCloneAudioProcessor& audioProcessor;

    // 1. RELAYS (destroyed last)
    juce::WebComboBoxRelay o1OctaveRelay  { ParameterIDs::o1_octave };
    juce::WebComboBoxRelay o2OctaveRelay  { ParameterIDs::o2_octave };
    juce::WebComboBoxRelay o1WaveRelay    { ParameterIDs::o1_wave };
    juce::WebSliderRelay o1CoarseRelay    { ParameterIDs::o1_coarse };
    juce::WebSliderRelay o1FineRelay      { ParameterIDs::o1_fine };
    juce::WebSliderRelay o1ShapeRelay     { ParameterIDs::o1_shape };
    juce::WebSliderRelay o1LevelRelay     { ParameterIDs::o1_level };
    juce::WebComboBoxRelay o2WaveRelay    { ParameterIDs::o2_wave };
    juce::WebSliderRelay o2CoarseRelay    { ParameterIDs::o2_coarse };
    juce::WebSliderRelay o2FineRelay      { ParameterIDs::o2_fine };
    juce::WebSliderRelay o2ShapeRelay     { ParameterIDs::o2_shape };
    juce::WebSliderRelay o2DetuneRelay    { ParameterIDs::o2_detune };
    juce::WebSliderRelay o2LevelRelay     { ParameterIDs::o2_level };
    juce::WebSliderRelay noiseLevelRelay  { ParameterIDs::noise_level };
    juce::WebComboBoxRelay mix1Relay      { ParameterIDs::mix1_src };
    juce::WebComboBoxRelay mix2Relay      { ParameterIDs::mix2_src };
    juce::WebComboBoxRelay mix3Relay      { ParameterIDs::mix3_src };
    juce::WebComboBoxRelay filterModeRelay { ParameterIDs::filter_mode };
    juce::WebSliderRelay cutoffRelay      { ParameterIDs::cutoff };
    juce::WebSliderRelay resonanceRelay   { ParameterIDs::resonance };
    juce::WebSliderRelay hpCutoffRelay    { ParameterIDs::hp_cutoff };
    juce::WebSliderRelay fltAuxRelay      { ParameterIDs::flt_aux };
    juce::WebSliderRelay overdriveRelay   { ParameterIDs::overdrive };
    juce::WebSliderRelay e1AttackRelay    { ParameterIDs::e1_attack };
    juce::WebSliderRelay e1DecayRelay     { ParameterIDs::e1_decay };
    juce::WebSliderRelay e1SustainRelay   { ParameterIDs::e1_sustain };
    juce::WebSliderRelay e1ReleaseRelay   { ParameterIDs::e1_release };
    juce::WebSliderRelay e2AttackRelay    { ParameterIDs::e2_attack };
    juce::WebSliderRelay e2DecayRelay     { ParameterIDs::e2_decay };
    juce::WebSliderRelay e2SustainRelay   { ParameterIDs::e2_sustain };
    juce::WebSliderRelay e2ReleaseRelay   { ParameterIDs::e2_release };
    juce::WebSliderRelay filtEnvAmtRelay  { ParameterIDs::filt_env_amt };
    juce::WebSliderRelay lfoRateRelay     { ParameterIDs::lfo_rate };
    juce::WebSliderRelay lfoAmtRelay      { ParameterIDs::lfo_amt };
    juce::WebSliderRelay lfo2RateRelay    { ParameterIDs::lfo2_rate };
    juce::WebSliderRelay lfo3RateRelay    { ParameterIDs::lfo3_rate };
    juce::WebSliderRelay lfoShapeRelay    { ParameterIDs::lfo_shape };
    juce::WebSliderRelay lfo2RmAmtRelay   { ParameterIDs::lfo2_rm_amt };
    juce::WebSliderRelay resModAmtRelay   { ParameterIDs::res_mod_amt };
    juce::WebComboBoxRelay cutMod1Relay   { ParameterIDs::cut_mod1_src };
    juce::WebComboBoxRelay cutMod2Relay   { ParameterIDs::cut_mod2_src };
    juce::WebComboBoxRelay resModRelay    { ParameterIDs::res_mod_src };
    juce::WebComboBoxRelay o1FmRelay      { ParameterIDs::o1_fm_src };
    juce::WebComboBoxRelay o2FmRelay      { ParameterIDs::o2_fm_src };
    juce::WebSliderRelay o1FmAmtRelay     { ParameterIDs::o1_fm_amt };
    juce::WebSliderRelay o2FmAmtRelay     { ParameterIDs::o2_fm_amt };
    juce::WebComboBoxRelay o1WsRelay      { ParameterIDs::o1_ws_src };
    juce::WebComboBoxRelay o2WsRelay      { ParameterIDs::o2_ws_src };
    juce::WebSliderRelay o1WsAmtRelay     { ParameterIDs::o1_ws_amt };
    juce::WebSliderRelay o2WsAmtRelay     { ParameterIDs::o2_ws_amt };
    juce::WebComboBoxRelay ampModRelay    { ParameterIDs::amp_mod_src };
    juce::WebSliderRelay ampModAmtRelay   { ParameterIDs::amp_mod_amt };
    juce::WebSliderRelay ampLevelRelay    { ParameterIDs::amp_level };
    juce::WebComboBoxRelay panModRelay    { ParameterIDs::pan_mod_src };
    juce::WebSliderRelay panModAmtRelay   { ParameterIDs::pan_mod_amt };
    juce::WebToggleButtonRelay cutInv1Relay { ParameterIDs::filt_env_amt_inv };
    juce::WebToggleButtonRelay cutInv2Relay { ParameterIDs::lfo_amt_inv };
    juce::WebToggleButtonRelay resInvRelay  { ParameterIDs::res_mod_amt_inv };
    juce::WebToggleButtonRelay ampInvRelay  { ParameterIDs::amp_mod_amt_inv };
    juce::WebToggleButtonRelay panInvRelay  { ParameterIDs::pan_mod_amt_inv };
    juce::WebToggleButtonRelay o1FmInvRelay { ParameterIDs::o1_fm_amt_inv };
    juce::WebToggleButtonRelay o2FmInvRelay { ParameterIDs::o2_fm_amt_inv };
    juce::WebToggleButtonRelay o1WsInvRelay { ParameterIDs::o1_ws_amt_inv };
    juce::WebToggleButtonRelay o2WsInvRelay { ParameterIDs::o2_ws_amt_inv };
    juce::WebToggleButtonRelay e2ModInvRelay{ ParameterIDs::e2_mod_amt_inv };
    juce::WebToggleButtonRelay ma1InvRelay  { ParameterIDs::ma1_am_amt_inv };
    juce::WebToggleButtonRelay ma2InvRelay  { ParameterIDs::ma2_am_amt_inv };
    juce::WebToggleButtonRelay portaExpRelay{ ParameterIDs::porta_exp };
    juce::WebToggleButtonRelay portaAutoRelay{ ParameterIDs::porta_auto };
    juce::WebToggleButtonRelay e1MultiRelay { ParameterIDs::e1_multi };
    juce::WebToggleButtonRelay e2MultiRelay { ParameterIDs::e2_multi };
    juce::WebComboBoxRelay ma1InRelay     { ParameterIDs::ma1_in_src };
    juce::WebComboBoxRelay ma1AmRelay     { ParameterIDs::ma1_am_src };
    juce::WebSliderRelay ma1AmtRelay      { ParameterIDs::ma1_am_amt };
    juce::WebComboBoxRelay ma2InRelay     { ParameterIDs::ma2_in_src };
    juce::WebComboBoxRelay ma2AmRelay     { ParameterIDs::ma2_am_src };
    juce::WebSliderRelay ma2AmtRelay      { ParameterIDs::ma2_am_amt };
    juce::WebComboBoxRelay lagInRelay     { ParameterIDs::lag_in_src };
    juce::WebSliderRelay lagTimeRelay     { ParameterIDs::lag_time };
    juce::WebToggleButtonRelay e1ResetRelay { ParameterIDs::e1_reset };
    juce::WebToggleButtonRelay e1CycleRelay { ParameterIDs::e1_cycle };
    juce::WebToggleButtonRelay e1CurveRelay { ParameterIDs::e1_curve };
    juce::WebToggleButtonRelay e2ResetRelay { ParameterIDs::e2_reset };
    juce::WebToggleButtonRelay e2CycleRelay { ParameterIDs::e2_cycle };
    juce::WebToggleButtonRelay e2CurveRelay { ParameterIDs::e2_curve };
    juce::WebToggleButtonRelay lfoSyncRelay  { ParameterIDs::lfo_sync };
    juce::WebToggleButtonRelay lfo2SyncRelay { ParameterIDs::lfo2_sync };
    juce::WebToggleButtonRelay lfoTrigRelay  { ParameterIDs::lfo_trig };
    juce::WebToggleButtonRelay lfo2TrigRelay { ParameterIDs::lfo2_trig };
    juce::WebToggleButtonRelay arpOnRelay    { ParameterIDs::arp_on };
    juce::WebToggleButtonRelay arpHoldRelay  { ParameterIDs::arp_hold };
    juce::WebToggleButtonRelay oscLinkRelay  { ParameterIDs::osc_link };
    juce::WebToggleButtonRelay ma1SqRelay    { ParameterIDs::ma1_sq };
    juce::WebToggleButtonRelay mix1PostRelay { ParameterIDs::mix1_post };
    juce::WebToggleButtonRelay mix2PostRelay { ParameterIDs::mix2_post };
    juce::WebToggleButtonRelay mix3PostRelay { ParameterIDs::mix3_post };
    juce::WebComboBoxRelay e2ModRelay        { ParameterIDs::e2_mod_src };
    juce::WebSliderRelay   e2ModAmtRelay     { ParameterIDs::e2_mod_amt };
    juce::WebComboBoxRelay e2DestRelay       { ParameterIDs::e2_dest };
    juce::WebComboBoxRelay lfoPeriodRelay  { ParameterIDs::lfo_period };
    juce::WebComboBoxRelay lfo2PeriodRelay { ParameterIDs::lfo2_period };
    juce::WebComboBoxRelay lfoWaveRelay    { ParameterIDs::lfo_wave };
    juce::WebComboBoxRelay lfo2WaveRelay   { ParameterIDs::lfo2_wave };
    juce::WebComboBoxRelay lfo2RmRelay     { ParameterIDs::lfo2_rm_src };
    juce::WebComboBoxRelay arpModeRelay    { ParameterIDs::arp_mode };
    juce::WebComboBoxRelay arpRateRelay    { ParameterIDs::arp_rate };
    juce::WebComboBoxRelay arpOctRelay     { ParameterIDs::arp_oct };
    juce::WebComboBoxRelay voiceModeRelay { ParameterIDs::voice_mode };
    juce::WebComboBoxRelay notePrioRelay  { ParameterIDs::note_priority };
    juce::WebSliderRelay uniDetuneRelay   { ParameterIDs::uni_detune };
    juce::WebSliderRelay portamentoRelay  { ParameterIDs::portamento };
    juce::WebSliderRelay bendRangeRelay   { ParameterIDs::bend_range };
    juce::WebSliderRelay uniVoicesRelay   { ParameterIDs::uni_voices };

    // 2. WEBVIEW (destroyed middle)
    std::unique_ptr<juce::WebBrowserComponent> webView;

    // 3. ATTACHMENTS (destroyed first)
    std::unique_ptr<juce::WebSliderParameterAttachment>
        o1CoarseAtt, o1FineAtt, o2CoarseAtt, o2FineAtt,
        o1ShapeAtt, o1LevelAtt, o2ShapeAtt, o2DetuneAtt, o2LevelAtt,
        noiseLevelAtt, cutoffAtt, resonanceAtt, hpCutoffAtt, fltAuxAtt,
        overdriveAtt, e1AttackAtt, e1DecayAtt, e1SustainAtt, e1ReleaseAtt,
        e2AttackAtt, e2DecayAtt, e2SustainAtt, e2ReleaseAtt, filtEnvAmtAtt,
        lfoRateAtt, lfoAmtAtt, lfo2RateAtt, lfo3RateAtt, lfoShapeAtt, lfo2RmAmtAtt, resModAmtAtt,
        o1FmAmtAtt, o2FmAmtAtt, o1WsAmtAtt, o2WsAmtAtt, ampModAmtAtt, ampLevelAtt, panModAmtAtt, e2ModAmtAtt,
        ma1AmtAtt, ma2AmtAtt, lagTimeAtt, uniDetuneAtt, portamentoAtt, bendRangeAtt, uniVoicesAtt;
    std::unique_ptr<juce::WebComboBoxParameterAttachment>
        o1OctaveAtt, o2OctaveAtt, o1WaveAtt, o2WaveAtt, filterModeAtt, voiceModeAtt, notePrioAtt,
        cutMod1Att, cutMod2Att, resModAtt, o1FmAtt, o2FmAtt, o1WsAtt, o2WsAtt, ampModAtt, panModAtt,
        ma1InAtt, ma1AmAtt, ma2InAtt, ma2AmAtt, lagInAtt, mix1Att, mix2Att, mix3Att;
    std::unique_ptr<juce::WebToggleButtonParameterAttachment>
        e1ResetAtt, e1CycleAtt, e1CurveAtt, e2ResetAtt, e2CycleAtt, e2CurveAtt, lfoSyncAtt, lfo2SyncAtt, lfoTrigAtt, lfo2TrigAtt, arpOnAtt, arpHoldAtt, oscLinkAtt, ma1SqAtt, mix1PostAtt, mix2PostAtt, mix3PostAtt, cutInv1Att, cutInv2Att, resInvAtt, ampInvAtt, panInvAtt, o1FmInvAtt, o2FmInvAtt, o1WsInvAtt, o2WsInvAtt, e2ModInvAtt, ma1InvAtt, ma2InvAtt, portaExpAtt, portaAutoAtt, e1MultiAtt, e2MultiAtt;
    std::unique_ptr<juce::WebComboBoxParameterAttachment> lfoPeriodAtt, lfo2PeriodAtt, lfoWaveAtt, lfo2WaveAtt, lfo2RmAtt, arpModeAtt, arpRateAtt, arpOctAtt, e2ModAtt, e2DestAtt;

    // Resource provider (serves embedded web UI from BinaryData)
    std::optional<juce::WebBrowserComponent::Resource> getResource (const juce::String& url);
    static const char* getMimeForExtension (const juce::String& extension);

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (VAZCloneAudioProcessorEditor)
};
