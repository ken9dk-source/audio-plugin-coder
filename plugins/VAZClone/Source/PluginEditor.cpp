#include "PluginProcessor.h"
#include "PluginEditor.h"
#include "BinaryData.h"

//==============================================================================
VAZCloneAudioProcessorEditor::VAZCloneAudioProcessorEditor (VAZCloneAudioProcessor& p)
    : AudioProcessorEditor (&p), audioProcessor (p)
{
    using Options = juce::WebBrowserComponent::Options;

    webView = std::make_unique<juce::WebBrowserComponent>(
        Options{}
           #if JUCE_WINDOWS
            .withBackend (Options::Backend::webview2)
            .withWinWebView2Options (Options::WinWebView2{}
                .withUserDataFolder (juce::File::getSpecialLocation (juce::File::SpecialLocationType::tempDirectory)))
           #endif
            .withNativeIntegrationEnabled()
            .withNativeFunction (juce::Identifier ("loadPatch"),
                [this] (const juce::Array<juce::var>&, juce::WebBrowserComponent::NativeFunctionCompletion complete)
                { audioProcessor.loadPatchDialog(); complete (juce::var()); })
            .withNativeFunction (juce::Identifier ("savePatch"),
                [this] (const juce::Array<juce::var>&, juce::WebBrowserComponent::NativeFunctionCompletion complete)
                { audioProcessor.savePatchDialog(); complete (juce::var()); })
            .withNativeFunction (juce::Identifier ("loadSample"),
                [this] (const juce::Array<juce::var>& args, juce::WebBrowserComponent::NativeFunctionCompletion complete)
                {
                    auto comp = std::make_shared<juce::WebBrowserComponent::NativeFunctionCompletion> (std::move (complete));
                    audioProcessor.loadSampleDialog (args.size() > 0 ? (int) args[0] : 0,
                        [comp] (juce::String n) { (*comp) (juce::var (n)); });   // resolves the JS promise with the sample name
                })
            .withNativeFunction (juce::Identifier ("resetSample"),
                [this] (const juce::Array<juce::var>& args, juce::WebBrowserComponent::NativeFunctionCompletion complete)
                { audioProcessor.resetSample (args.size() > 0 ? (int) args[0] : 0); complete (juce::var()); })
            .withNativeFunction (juce::Identifier ("getSampleName"),
                [this] (const juce::Array<juce::var>& args, juce::WebBrowserComponent::NativeFunctionCompletion complete)
                { const int o = args.size() > 0 ? (int) args[0] : 0;
                  complete (juce::var ((o == 1 ? audioProcessor.osc2SampleData : audioProcessor.osc1SampleData).name)); })
            .withResourceProvider ([this] (const auto& url) { return getResource (url); })
            .withOptionsFrom (o1OctaveRelay)
            .withOptionsFrom (o2OctaveRelay)
            .withOptionsFrom (o1WaveRelay)
            .withOptionsFrom (o1CoarseRelay)
            .withOptionsFrom (o1FineRelay)
            .withOptionsFrom (o1ShapeRelay)
            .withOptionsFrom (o1LevelRelay)
            .withOptionsFrom (o2WaveRelay)
            .withOptionsFrom (o2CoarseRelay)
            .withOptionsFrom (o2FineRelay)
            .withOptionsFrom (o2ShapeRelay)
            .withOptionsFrom (o2DetuneRelay)
            .withOptionsFrom (o2LevelRelay)
            .withOptionsFrom (noiseLevelRelay)
            .withOptionsFrom (mix1Relay)
            .withOptionsFrom (mix2Relay)
            .withOptionsFrom (mix3Relay)
            .withOptionsFrom (filterModeRelay)
            .withOptionsFrom (cutoffRelay)
            .withOptionsFrom (resonanceRelay)
            .withOptionsFrom (hpCutoffRelay)
            .withOptionsFrom (fltAuxRelay)
            .withOptionsFrom (overdriveRelay)
            .withOptionsFrom (e1AttackRelay)
            .withOptionsFrom (e1DecayRelay)
            .withOptionsFrom (e1SustainRelay)
            .withOptionsFrom (e1ReleaseRelay)
            .withOptionsFrom (e2AttackRelay)
            .withOptionsFrom (e2DecayRelay)
            .withOptionsFrom (e2SustainRelay)
            .withOptionsFrom (e2ReleaseRelay)
            .withOptionsFrom (filtEnvAmtRelay)
            .withOptionsFrom (lfoRateRelay)
            .withOptionsFrom (lfoAmtRelay)
            .withOptionsFrom (lfo2RateRelay)
            .withOptionsFrom (lfo3RateRelay)
            .withOptionsFrom (lfoShapeRelay)
            .withOptionsFrom (lfo2RmAmtRelay)
            .withOptionsFrom (resModAmtRelay)
            .withOptionsFrom (cutMod1Relay)
            .withOptionsFrom (cutMod2Relay)
            .withOptionsFrom (resModRelay)
            .withOptionsFrom (o1FmRelay)
            .withOptionsFrom (o2FmRelay)
            .withOptionsFrom (o1FmAmtRelay)
            .withOptionsFrom (o2FmAmtRelay)
            .withOptionsFrom (o1WsRelay)
            .withOptionsFrom (o2WsRelay)
            .withOptionsFrom (o1WsAmtRelay)
            .withOptionsFrom (o2WsAmtRelay)
            .withOptionsFrom (ampModRelay)
            .withOptionsFrom (ampModAmtRelay)
            .withOptionsFrom (ampLevelRelay)
            .withOptionsFrom (panModRelay)
            .withOptionsFrom (panModAmtRelay)
            .withOptionsFrom (cutInv1Relay)
            .withOptionsFrom (cutInv2Relay)
            .withOptionsFrom (resInvRelay)
            .withOptionsFrom (ampInvRelay)
            .withOptionsFrom (panInvRelay)
            .withOptionsFrom (o1FmInvRelay)
            .withOptionsFrom (o2FmInvRelay)
            .withOptionsFrom (o1WsInvRelay)
            .withOptionsFrom (o2WsInvRelay)
            .withOptionsFrom (e2ModInvRelay)
            .withOptionsFrom (ma1InvRelay)
            .withOptionsFrom (ma2InvRelay)
            .withOptionsFrom (portaExpRelay)
            .withOptionsFrom (portaAutoRelay)
            .withOptionsFrom (e1MultiRelay)
            .withOptionsFrom (e2MultiRelay)
            .withOptionsFrom (ma1InRelay)
            .withOptionsFrom (ma1AmRelay)
            .withOptionsFrom (ma1AmtRelay)
            .withOptionsFrom (ma2InRelay)
            .withOptionsFrom (ma2AmRelay)
            .withOptionsFrom (ma2AmtRelay)
            .withOptionsFrom (lagInRelay)
            .withOptionsFrom (lagTimeRelay)
            .withOptionsFrom (e1ResetRelay)
            .withOptionsFrom (e1CycleRelay)
            .withOptionsFrom (e1CurveRelay)
            .withOptionsFrom (e2ResetRelay)
            .withOptionsFrom (e2CycleRelay)
            .withOptionsFrom (e2CurveRelay)
            .withOptionsFrom (lfoSyncRelay)
            .withOptionsFrom (lfo2SyncRelay)
            .withOptionsFrom (lfoPeriodRelay)
            .withOptionsFrom (lfo2PeriodRelay)
            .withOptionsFrom (lfoWaveRelay)
            .withOptionsFrom (lfo2WaveRelay)
            .withOptionsFrom (lfo2RmRelay)
            .withOptionsFrom (lfoTrigRelay)
            .withOptionsFrom (lfo2TrigRelay)
            .withOptionsFrom (voiceModeRelay)
            .withOptionsFrom (notePrioRelay)
            .withOptionsFrom (uniDetuneRelay)
            .withOptionsFrom (portamentoRelay)
            .withOptionsFrom (bendRangeRelay)
            .withOptionsFrom (uniVoicesRelay)
            .withOptionsFrom (arpOnRelay)
            .withOptionsFrom (arpHoldRelay)
            .withOptionsFrom (arpModeRelay)
            .withOptionsFrom (arpRateRelay)
            .withOptionsFrom (arpOctRelay)
            .withOptionsFrom (oscLinkRelay)
            .withOptionsFrom (ma1SqRelay)
            .withOptionsFrom (e2ModRelay)
            .withOptionsFrom (e2ModAmtRelay)
            .withOptionsFrom (e2DestRelay)
            .withOptionsFrom (mix1PostRelay)
            .withOptionsFrom (mix2PostRelay)
            .withOptionsFrom (mix3PostRelay));

    addAndMakeVisible (*webView);

    // Attachments AFTER WebBrowserComponent
    auto attach = [this] (const char* id, juce::WebSliderRelay& relay)
    {
        return std::make_unique<juce::WebSliderParameterAttachment>(
            *audioProcessor.apvts.getParameter (id), relay, nullptr);
    };
    auto attachCombo = [this] (const char* id, juce::WebComboBoxRelay& relay)
    {
        return std::make_unique<juce::WebComboBoxParameterAttachment>(
            *audioProcessor.apvts.getParameter (id), relay, nullptr);
    };
    o1OctaveAtt   = attachCombo (ParameterIDs::o1_octave, o1OctaveRelay);
    o2OctaveAtt   = attachCombo (ParameterIDs::o2_octave, o2OctaveRelay);
    o1WaveAtt     = attachCombo (ParameterIDs::o1_wave,   o1WaveRelay);
    o1CoarseAtt   = attach (ParameterIDs::o1_coarse,   o1CoarseRelay);
    o1FineAtt     = attach (ParameterIDs::o1_fine,     o1FineRelay);
    o1ShapeAtt    = attach (ParameterIDs::o1_shape,    o1ShapeRelay);
    o1LevelAtt    = attach (ParameterIDs::o1_level,    o1LevelRelay);
    o2WaveAtt     = attachCombo (ParameterIDs::o2_wave,   o2WaveRelay);
    o2CoarseAtt   = attach (ParameterIDs::o2_coarse,   o2CoarseRelay);
    o2FineAtt     = attach (ParameterIDs::o2_fine,     o2FineRelay);
    o2ShapeAtt    = attach (ParameterIDs::o2_shape,    o2ShapeRelay);
    o2DetuneAtt   = attach (ParameterIDs::o2_detune,   o2DetuneRelay);
    o2LevelAtt    = attach (ParameterIDs::o2_level,    o2LevelRelay);
    noiseLevelAtt = attach (ParameterIDs::noise_level, noiseLevelRelay);
    mix1Att       = attachCombo (ParameterIDs::mix1_src, mix1Relay);
    mix2Att       = attachCombo (ParameterIDs::mix2_src, mix2Relay);
    mix3Att       = attachCombo (ParameterIDs::mix3_src, mix3Relay);
    filterModeAtt = attachCombo (ParameterIDs::filter_mode, filterModeRelay);
    cutoffAtt     = attach (ParameterIDs::cutoff,      cutoffRelay);
    resonanceAtt  = attach (ParameterIDs::resonance,   resonanceRelay);
    hpCutoffAtt   = attach (ParameterIDs::hp_cutoff,   hpCutoffRelay);
    fltAuxAtt     = attach (ParameterIDs::flt_aux,     fltAuxRelay);
    overdriveAtt  = attach (ParameterIDs::overdrive,   overdriveRelay);
    e1AttackAtt   = attach (ParameterIDs::e1_attack,   e1AttackRelay);
    e1DecayAtt    = attach (ParameterIDs::e1_decay,    e1DecayRelay);
    e1SustainAtt  = attach (ParameterIDs::e1_sustain,  e1SustainRelay);
    e1ReleaseAtt  = attach (ParameterIDs::e1_release,  e1ReleaseRelay);
    e2AttackAtt   = attach (ParameterIDs::e2_attack,   e2AttackRelay);
    e2DecayAtt    = attach (ParameterIDs::e2_decay,    e2DecayRelay);
    e2SustainAtt  = attach (ParameterIDs::e2_sustain,  e2SustainRelay);
    e2ReleaseAtt  = attach (ParameterIDs::e2_release,  e2ReleaseRelay);
    filtEnvAmtAtt = attach (ParameterIDs::filt_env_amt, filtEnvAmtRelay);
    lfoRateAtt    = attach (ParameterIDs::lfo_rate,    lfoRateRelay);
    lfoShapeAtt   = attach (ParameterIDs::lfo_shape,   lfoShapeRelay);
    lfo2RmAmtAtt  = attach (ParameterIDs::lfo2_rm_amt, lfo2RmAmtRelay);
    lfoAmtAtt     = attach (ParameterIDs::lfo_amt,     lfoAmtRelay);
    lfo2RateAtt   = attach (ParameterIDs::lfo2_rate,   lfo2RateRelay);
    lfo3RateAtt   = attach (ParameterIDs::lfo3_rate,   lfo3RateRelay);
    resModAmtAtt  = attach (ParameterIDs::res_mod_amt, resModAmtRelay);
    cutMod1Att    = attachCombo (ParameterIDs::cut_mod1_src, cutMod1Relay);
    cutMod2Att    = attachCombo (ParameterIDs::cut_mod2_src, cutMod2Relay);
    resModAtt     = attachCombo (ParameterIDs::res_mod_src,  resModRelay);
    o1FmAtt       = attachCombo (ParameterIDs::o1_fm_src,    o1FmRelay);
    o2FmAtt       = attachCombo (ParameterIDs::o2_fm_src,    o2FmRelay);
    o1FmAmtAtt    = attach (ParameterIDs::o1_fm_amt, o1FmAmtRelay);
    o2FmAmtAtt    = attach (ParameterIDs::o2_fm_amt, o2FmAmtRelay);
    o1WsAtt       = attachCombo (ParameterIDs::o1_ws_src,    o1WsRelay);
    o2WsAtt       = attachCombo (ParameterIDs::o2_ws_src,    o2WsRelay);
    o1WsAmtAtt    = attach (ParameterIDs::o1_ws_amt, o1WsAmtRelay);
    o2WsAmtAtt    = attach (ParameterIDs::o2_ws_amt, o2WsAmtRelay);
    ampModAtt     = attachCombo (ParameterIDs::amp_mod_src, ampModRelay);
    ampModAmtAtt  = attach (ParameterIDs::amp_mod_amt, ampModAmtRelay);
    ampLevelAtt   = attach (ParameterIDs::amp_level,   ampLevelRelay);
    panModAtt     = attachCombo (ParameterIDs::pan_mod_src, panModRelay);
    panModAmtAtt  = attach (ParameterIDs::pan_mod_amt, panModAmtRelay);
    ma1InAtt      = attachCombo (ParameterIDs::ma1_in_src, ma1InRelay);
    ma1AmAtt      = attachCombo (ParameterIDs::ma1_am_src, ma1AmRelay);
    ma1AmtAtt     = attach (ParameterIDs::ma1_am_amt, ma1AmtRelay);
    ma2InAtt      = attachCombo (ParameterIDs::ma2_in_src, ma2InRelay);
    ma2AmAtt      = attachCombo (ParameterIDs::ma2_am_src, ma2AmRelay);
    ma2AmtAtt     = attach (ParameterIDs::ma2_am_amt, ma2AmtRelay);
    lagInAtt      = attachCombo (ParameterIDs::lag_in_src, lagInRelay);
    lagTimeAtt    = attach (ParameterIDs::lag_time, lagTimeRelay);
    auto attachTgl = [this] (const char* id, juce::WebToggleButtonRelay& relay)
    { return std::make_unique<juce::WebToggleButtonParameterAttachment>(*audioProcessor.apvts.getParameter (id), relay, nullptr); };
    e1ResetAtt = attachTgl (ParameterIDs::e1_reset, e1ResetRelay);
    e1CycleAtt = attachTgl (ParameterIDs::e1_cycle, e1CycleRelay);
    e1CurveAtt = attachTgl (ParameterIDs::e1_curve, e1CurveRelay);
    e2ResetAtt = attachTgl (ParameterIDs::e2_reset, e2ResetRelay);
    e2CycleAtt = attachTgl (ParameterIDs::e2_cycle, e2CycleRelay);
    e2CurveAtt = attachTgl (ParameterIDs::e2_curve, e2CurveRelay);
    lfoSyncAtt  = attachTgl (ParameterIDs::lfo_sync,  lfoSyncRelay);
    lfo2SyncAtt = attachTgl (ParameterIDs::lfo2_sync, lfo2SyncRelay);
    lfoPeriodAtt  = attachCombo (ParameterIDs::lfo_period,  lfoPeriodRelay);
    lfo2PeriodAtt = attachCombo (ParameterIDs::lfo2_period, lfo2PeriodRelay);
    lfoWaveAtt    = attachCombo (ParameterIDs::lfo_wave,  lfoWaveRelay);
    lfo2WaveAtt   = attachCombo (ParameterIDs::lfo2_wave, lfo2WaveRelay);
    lfo2RmAtt     = attachCombo (ParameterIDs::lfo2_rm_src, lfo2RmRelay);
    lfoTrigAtt    = attachTgl   (ParameterIDs::lfo_trig,  lfoTrigRelay);
    lfo2TrigAtt   = attachTgl   (ParameterIDs::lfo2_trig, lfo2TrigRelay);
    voiceModeAtt  = attachCombo (ParameterIDs::voice_mode, voiceModeRelay);
    notePrioAtt   = attachCombo (ParameterIDs::note_priority, notePrioRelay);
    uniDetuneAtt  = attach (ParameterIDs::uni_detune,  uniDetuneRelay);
    portamentoAtt = attach (ParameterIDs::portamento,  portamentoRelay);
    bendRangeAtt  = attach (ParameterIDs::bend_range,  bendRangeRelay);
    uniVoicesAtt  = attach (ParameterIDs::uni_voices,  uniVoicesRelay);
    arpOnAtt   = attachTgl   (ParameterIDs::arp_on,   arpOnRelay);
    arpHoldAtt = attachTgl   (ParameterIDs::arp_hold, arpHoldRelay);
    cutInv1Att = attachTgl (ParameterIDs::filt_env_amt_inv, cutInv1Relay);
    cutInv2Att = attachTgl (ParameterIDs::lfo_amt_inv,      cutInv2Relay);
    resInvAtt  = attachTgl (ParameterIDs::res_mod_amt_inv,  resInvRelay);
    ampInvAtt  = attachTgl (ParameterIDs::amp_mod_amt_inv,  ampInvRelay);
    panInvAtt  = attachTgl (ParameterIDs::pan_mod_amt_inv,  panInvRelay);
    o1FmInvAtt = attachTgl (ParameterIDs::o1_fm_amt_inv, o1FmInvRelay);
    o2FmInvAtt = attachTgl (ParameterIDs::o2_fm_amt_inv, o2FmInvRelay);
    o1WsInvAtt = attachTgl (ParameterIDs::o1_ws_amt_inv, o1WsInvRelay);
    o2WsInvAtt = attachTgl (ParameterIDs::o2_ws_amt_inv, o2WsInvRelay);
    e2ModInvAtt= attachTgl (ParameterIDs::e2_mod_amt_inv, e2ModInvRelay);
    ma1InvAtt  = attachTgl (ParameterIDs::ma1_am_amt_inv, ma1InvRelay);
    ma2InvAtt  = attachTgl (ParameterIDs::ma2_am_amt_inv, ma2InvRelay);
    portaExpAtt= attachTgl (ParameterIDs::porta_exp, portaExpRelay);
    portaAutoAtt= attachTgl (ParameterIDs::porta_auto, portaAutoRelay);
    e1MultiAtt = attachTgl (ParameterIDs::e1_multi, e1MultiRelay);
    e2MultiAtt = attachTgl (ParameterIDs::e2_multi, e2MultiRelay);
    arpModeAtt = attachCombo (ParameterIDs::arp_mode, arpModeRelay);
    arpRateAtt = attachCombo (ParameterIDs::arp_rate, arpRateRelay);
    arpOctAtt  = attachCombo (ParameterIDs::arp_oct,  arpOctRelay);
    oscLinkAtt = attachTgl   (ParameterIDs::osc_link, oscLinkRelay);
    ma1SqAtt   = attachTgl   (ParameterIDs::ma1_sq,   ma1SqRelay);
    e2ModAtt    = attachCombo (ParameterIDs::e2_mod_src, e2ModRelay);
    e2ModAmtAtt = attach      (ParameterIDs::e2_mod_amt, e2ModAmtRelay);
    e2DestAtt   = attachCombo (ParameterIDs::e2_dest,    e2DestRelay);
    mix1PostAtt = attachTgl (ParameterIDs::mix1_post, mix1PostRelay);
    mix2PostAtt = attachTgl (ParameterIDs::mix2_post, mix2PostRelay);
    mix3PostAtt = attachTgl (ParameterIDs::mix3_post, mix3PostRelay);

    webView->goToURL (juce::WebBrowserComponent::getResourceProviderRoot());

    setSize (726, 572);   // 604x475 content × 1.2 zoom (+ small margin) → shows the full menu bar + Env Curve buttons
}

VAZCloneAudioProcessorEditor::~VAZCloneAudioProcessorEditor() {}

//==============================================================================
void VAZCloneAudioProcessorEditor::paint (juce::Graphics& g)
{
    g.fillAll (juce::Colour (0xff1c1c2c));
}

void VAZCloneAudioProcessorEditor::resized()
{
    if (webView != nullptr)
        webView->setBounds (getLocalBounds());
}

//==============================================================================
const char* VAZCloneAudioProcessorEditor::getMimeForExtension (const juce::String& extension)
{
    static const std::unordered_map<juce::String, const char*> mimeMap =
    {
        { "html", "text/html" }, { "htm", "text/html" }, { "js", "text/javascript" },
        { "css", "text/css" }, { "json", "application/json" }, { "svg", "image/svg+xml" },
        { "png", "image/png" }, { "jpg", "image/jpeg" }, { "woff2", "font/woff2" }
    };
    if (auto it = mimeMap.find (extension.toLowerCase()); it != mimeMap.end())
        return it->second;
    return "text/plain";
}

std::optional<juce::WebBrowserComponent::Resource>
VAZCloneAudioProcessorEditor::getResource (const juce::String& url)
{
    // Strip the resource-provider root, then a leading slash IF present. The previous
    // unconditional substring(1) ate the first char of sub-resource paths (js/index.js
    // -> s/index.js) when the root URL ended in '/', so the JS module never loaded.
    auto path = url.fromFirstOccurrenceOf (
        juce::WebBrowserComponent::getResourceProviderRoot(), false, false);
    if (path.startsWithChar ('/')) path = path.substring (1);
    if (path.isEmpty()) path = "index.html";

    const char* data = nullptr;
    int size = 0;
    juce::String mime;

    if (path == "index.html")
    {
        data = BinaryData::index_html; size = BinaryData::index_htmlSize; mime = "text/html";
    }
    else if (path == "js/index.js")
    {
        data = BinaryData::index_js; size = BinaryData::index_jsSize; mime = "text/javascript";
    }
    else if (path == "js/juce/index.js")
    {
        data = BinaryData::index_js2; size = BinaryData::index_js2Size; mime = "text/javascript"; // mangled dup
    }
    else if (path == "js/juce/check_native_interop.js")
    {
        data = BinaryData::check_native_interop_js; size = BinaryData::check_native_interop_jsSize; mime = "text/javascript";
    }

    if (data != nullptr && size > 0)
    {
        std::vector<std::byte> bytes ((size_t) size);
        std::memcpy (bytes.data(), data, (size_t) size);
        return juce::WebBrowserComponent::Resource { std::move (bytes), mime };
    }
    return std::nullopt;
}
