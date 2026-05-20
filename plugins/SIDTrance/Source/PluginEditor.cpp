#include "PluginProcessor.h"
#include "PluginEditor.h"

static constexpr int kWindowW = 1200;
static constexpr int kWindowH = 740;

SIDTranceAudioProcessorEditor::SIDTranceAudioProcessorEditor(SIDTranceAudioProcessor& p)
    : VisagePluginEditor(p), audioProcessor(p)
{
    setSize(kWindowW, kWindowH);
}

SIDTranceAudioProcessorEditor::~SIDTranceAudioProcessorEditor() = default;

void SIDTranceAudioProcessorEditor::onInit()
{
    mainView = std::make_unique<SIDMainView>();
    addFrameToCanvas(mainView.get());
    mainView->setBounds(0, 0, kWindowW, kWindowH);

    addAllFrames();
    bindAllParameters();
}

void SIDTranceAudioProcessorEditor::addAllFrames()
{
    auto& v = *mainView;

    // ── Header scopes ───────────────────────────────────────
    v.addChild(&v.scope1);
    v.addChild(&v.scope2);
    v.addChild(&v.scope3);
    v.addChild(&v.filterScope);
    v.scope1.setLabel("OSC 1: SAW");
    v.scope2.setLabel("OSC 2: TRI");
    v.scope3.setLabel("OSC 3: NOISE");
    v.filterScope.setLabel("FILTER: Low Pass 24dB");

    // ── Oscillator panels ───────────────────────────────────
    v.addChild(&v.osc1);
    v.addChild(&v.osc2);
    v.addChild(&v.osc3);
    // Each OSC panel self-adds its own children in init()

    // ── Filter / Amp / Master ───────────────────────────────
    v.addChild(&v.filterPanel);
    v.addChild(&v.ampPanel);
    v.addChild(&v.masterPanel);

    // ── Bottom panels ────────────────────────────────────────
    v.addChild(&v.modMatrix);
    v.addChild(&v.effectsPanel);
    v.addChild(&v.lfo1);
    v.addChild(&v.lfo2);
    v.addChild(&v.arpPanel);

    // Arp — default state matching reference (steps 1-6, 8-11, 13-14, 16 active)
    const std::array<bool, 16> defaultSteps = {
        true,true,true,true,true,true,false,true,
        true,true,true,false,true,true,false,true
    };
    for (int i = 0; i < 16; ++i)
        v.arpPanel.steps[i].setState(defaultSteps[i]);

    // FX defaults matching reference
    v.effectsPanel.chorusBtn.setState(true);
    v.effectsPanel.delayBtn.setState(true);
    v.effectsPanel.reverbBtn.setState(true);
    v.masterPanel.limiterBtn.setState(true);
    v.lfo1.syncBtn.setState(false);
    v.lfo2.syncBtn.setState(false);
    v.arpPanel.syncBtn.setState(true);
}

void SIDTranceAudioProcessorEditor::bindAllParameters()
{
    auto& apvts = audioProcessor.getAPVTS();
    auto& v     = *mainView;

    // ── Helper lambda: bind a knob to an APVTS parameter ────
    auto bindKnob = [&](SIDKnob& knob, const juce::String& paramId) {
        if (auto* param = apvts.getParameter(paramId)) {
            knob.setValue(param->getValue(), false);
            knob.onValueChanged = [&apvts, paramId](float val) {
                if (auto* p = apvts.getParameter(paramId))
                    p->setValueNotifyingHost(val);
            };
        }
    };

    auto bindFader = [&](SIDFader& fader, const juce::String& paramId) {
        if (auto* param = apvts.getParameter(paramId)) {
            fader.setValue(param->getValue(), false);
            fader.onValueChanged = [&apvts, paramId](float val) {
                if (auto* p = apvts.getParameter(paramId))
                    p->setValueNotifyingHost(val);
            };
        }
    };

    auto bindToggle = [&](SIDToggleButton& btn, const juce::String& paramId) {
        if (auto* param = apvts.getParameter(paramId)) {
            btn.setState(param->getValue() > 0.5f);
            btn.onToggled = [&apvts, paramId](bool on) {
                if (auto* p = apvts.getParameter(paramId))
                    p->setValueNotifyingHost(on ? 1.0f : 0.0f);
            };
        }
    };

    auto bindStep = [&](SIDStepButton& btn, const juce::String& paramId) {
        if (auto* param = apvts.getParameter(paramId)) {
            btn.setState(param->getValue() > 0.5f);
            btn.onToggled = [&apvts, paramId](bool on) {
                if (auto* p = apvts.getParameter(paramId))
                    p->setValueNotifyingHost(on ? 1.0f : 0.0f);
            };
        }
    };

    // ── OSC 1 ───────────────────────────────────────────────
    bindKnob(v.osc1.semiKnob,      "osc1_semi");
    bindKnob(v.osc1.fineKnob,      "osc1_fine");
    bindKnob(v.osc1.pwKnob,        "osc1_pw");
    bindKnob(v.osc1.volumeKnob,    "osc1_volume");
    bindFader(v.osc1.attackFader,  "osc1_attack");
    bindFader(v.osc1.decayFader,   "osc1_decay");
    bindFader(v.osc1.sustainFader, "osc1_sustain");
    bindFader(v.osc1.releaseFader, "osc1_release");

    // ── OSC 2 ───────────────────────────────────────────────
    bindKnob(v.osc2.semiKnob,      "osc2_semi");
    bindKnob(v.osc2.fineKnob,      "osc2_fine");
    bindKnob(v.osc2.pwKnob,        "osc2_pw");
    bindKnob(v.osc2.volumeKnob,    "osc2_volume");
    bindFader(v.osc2.attackFader,  "osc2_attack");
    bindFader(v.osc2.decayFader,   "osc2_decay");
    bindFader(v.osc2.sustainFader, "osc2_sustain");
    bindFader(v.osc2.releaseFader, "osc2_release");

    // ── OSC 3 ───────────────────────────────────────────────
    bindKnob(v.osc3.semiKnob,      "osc3_semi");
    bindKnob(v.osc3.fineKnob,      "osc3_fine");
    bindKnob(v.osc3.pwKnob,        "osc3_pw");
    bindKnob(v.osc3.volumeKnob,    "osc3_volume");
    bindFader(v.osc3.attackFader,  "osc3_attack");
    bindFader(v.osc3.decayFader,   "osc3_decay");
    bindFader(v.osc3.sustainFader, "osc3_sustain");
    bindFader(v.osc3.releaseFader, "osc3_release");

    // ── Filter ───────────────────────────────────────────────
    bindKnob(v.filterPanel.cutoffKnob,    "filter_cutoff");
    bindKnob(v.filterPanel.resonanceKnob, "filter_res");
    bindToggle(v.filterPanel.keyTrackBtn, "filter_keytrack");
    bindToggle(v.filterPanel.velocityBtn, "filter_velocity");

    // ── Amplifier ────────────────────────────────────────────
    bindKnob(v.ampPanel.attackKnob,  "amp_attack");
    bindKnob(v.ampPanel.decayKnob,   "amp_decay");
    bindKnob(v.ampPanel.sustainKnob, "amp_sustain");
    bindKnob(v.ampPanel.releaseKnob, "amp_release");
    bindKnob(v.ampPanel.volumeKnob,  "amp_volume");

    // ── Master ───────────────────────────────────────────────
    bindFader(v.masterPanel.outputFader, "master_volume");
    bindToggle(v.masterPanel.limiterBtn, "master_limiter");

    // ── LFOs ────────────────────────────────────────────────
    bindKnob(v.lfo1.rateKnob,    "lfo1_rate");
    bindToggle(v.lfo1.syncBtn,   "lfo1_sync");
    bindToggle(v.lfo1.retrigBtn, "lfo1_retrig");
    bindKnob(v.lfo2.rateKnob,    "lfo2_rate");
    bindToggle(v.lfo2.syncBtn,   "lfo2_sync");
    bindToggle(v.lfo2.retrigBtn, "lfo2_retrig");

    // ── Effects ─────────────────────────────────────────────
    bindToggle(v.effectsPanel.chorusBtn,    "fx_chorus_on");
    bindKnob(v.effectsPanel.chorusRate,     "fx_chorus_rate");
    bindKnob(v.effectsPanel.chorusDepth,    "fx_chorus_depth");
    bindKnob(v.effectsPanel.chorusMix,      "fx_chorus_mix");
    bindToggle(v.effectsPanel.delayBtn,     "fx_delay_on");
    bindKnob(v.effectsPanel.delayFeedback,  "fx_delay_feedback");
    bindKnob(v.effectsPanel.delayMix,       "fx_delay_mix");
    bindToggle(v.effectsPanel.reverbBtn,    "fx_reverb_on");
    bindKnob(v.effectsPanel.reverbSize,     "fx_reverb_size");
    bindKnob(v.effectsPanel.reverbDamping,  "fx_reverb_damp");
    bindKnob(v.effectsPanel.reverbMix,      "fx_reverb_mix");

    // ── Arp sequencer steps ─────────────────────────────────
    for (int i = 0; i < 16; ++i) {
        const juce::String id = juce::String::formatted("seq_step_%02d", i + 1);
        bindStep(v.arpPanel.steps[i], id);
    }
    bindToggle(v.arpPanel.syncBtn, "arp_sync");
}

void SIDTranceAudioProcessorEditor::onRender()
{
    // Push live waveform data to scopes (from audio thread FIFO — see PluginProcessor)
    // TODO in /impl: populate scope buffers via AbstractFifo
}

void SIDTranceAudioProcessorEditor::onDestroy()
{
    if (mainView) {
        removeFrameFromCanvas(mainView.get());
        mainView.reset();
    }
}

void SIDTranceAudioProcessorEditor::onResize(int w, int h)
{
    if (mainView) {
        mainView->setBounds(0, 0, w, h);
        mainView->redraw();
    }
}
