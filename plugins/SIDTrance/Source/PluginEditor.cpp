#include "PluginProcessor.h"
#include "PluginEditor.h"

// Editor size matches the GUI design PNG aspect ratio (1672 × 941, 16:9).
// 1280×720 keeps the window at a sensible desktop size while preserving the
// design proportions so the background image scales without letterboxing.
static constexpr int kWindowW = 1280;
static constexpr int kWindowH = 720;

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

    // Add all children BEFORE setBounds so resized() positions them as real children
    // Note: addFrameToCanvas calls mainView->init() which sets initialized_=true (via Frame::init())
    // so addChild() calls below WILL trigger panel->init() which adds knobs/faders as children
    addAllFrames();
    bindAllParameters();

    mainView->setBounds(0, 0, kWindowW, kWindowH);

    // ── Preset system ──────────────────────────────────────────────────────
    initPresets();
    updatePresetBar();

    auto& bar = mainView->presetBar;
    bar.onPrev = [this]() { prevPreset(); };
    bar.onNext = [this]() { nextPreset(); };

    // Action buttons: toggle state resets immediately after the action fires
    bar.saveBtn.onToggled = [this](bool on) {
        if (!on) return;
        savePreset();
        mainView->presetBar.saveBtn.setState(false);
    };
    bar.saveAsBtn.onToggled = [this](bool on) {
        if (!on) return;
        savePresetAs();
        mainView->presetBar.saveAsBtn.setState(false);
    };
    bar.initBtn.onToggled = [this](bool on) {
        if (!on) return;
        resetToInit();
        mainView->presetBar.initBtn.setState(false);
    };

    // Register as the hit-test root so dispatchMouse() can find child frames
    setEventRoot(mainView.get());

    // Start the drawing cascade: mainView.drawing_=true cascades to all panels and their controls
    // Without this, redraw() silently skips frames (drawing_ is false by default)
    mainView->setDrawing(true);
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
    v.filterScope.setFilterState("LP", "24dB", 440.0f, 0.3f);

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
    v.addChild(&v.gatePanel);
    v.addChild(&v.macroPanel);
    v.addChild(&v.voiceModPanel);
    v.addChild(&v.noisePanel);   // bottom-right strip, 1/3 of kPngArpGate
    // Two invisible click regions overlaid on the chip badges painted in
    // the PNG header.  Replace the old standalone SIDChipSwitch widget.
    v.addChild(&v.chip6581Area);
    v.addChild(&v.chip8580Area);
    v.addChild(&v.masterVolKnob);   // top-header rotary, bound to master_volume
    v.addChild(&v.outputMeter);     // VU meter right of the master rotary
    v.addChild(&v.presetBar);

    // Shared popup overlay must be the LAST child added so it sits on top
    // of every other frame in the z-order — that way mouseDown anywhere
    // inside the editor goes to the overlay first while it's visible.
    v.addChild(&v.popupOverlay);

    // Hand the overlay pointer to every dropdown widget in the editor.
    // Each dropdown's mouseDown/mouseUp routes through the overlay instead
    // of visage::PopupMenu, so click-outside / click-again-to-close work
    // reliably across the whole UI.
    {
        auto* p = &v.popupOverlay;
        v.filterPanel.typeBtn.setPopupOverlay(p);
        v.effectsPanel.delayTimeBtn.setPopupOverlay(p);
        v.masterPanel.voiceCountBtn.setPopupOverlay(p);
        v.voiceModPanel.uniVoicesBtn.setPopupOverlay(p);
        v.voiceModPanel.glideModeBtn.setPopupOverlay(p);
        v.voiceModPanel.driveTypeBtn.setPopupOverlay(p);
        v.lfo1.shapeBtn.setPopupOverlay(p);
        v.lfo1.syncDivBtn.setPopupOverlay(p);
        v.lfo2.shapeBtn.setPopupOverlay(p);
        v.lfo2.syncDivBtn.setPopupOverlay(p);
        v.arpPanel.modeBtn.setPopupOverlay(p);
        v.arpPanel.octaveBtn.setPopupOverlay(p);
        v.arpPanel.gateBtn.setPopupOverlay(p);
        v.arpPanel.swingBtn.setPopupOverlay(p);
        v.arpPanel.tempoBtn.setPopupOverlay(p);
        v.gatePanel.gateSwingBtn.setPopupOverlay(p);
        // Per-OSC supersaw voice-count selectors
        v.osc1.superVoicesBtn.setPopupOverlay(p);
        v.osc2.superVoicesBtn.setPopupOverlay(p);
        v.osc3.superVoicesBtn.setPopupOverlay(p);
        // Noise type cycle (4 options)
        v.noisePanel.typeBtn.setPopupOverlay(p);
        for (int i = 0; i < 4; ++i) {
            v.modMatrix.srcBtn[i].setPopupOverlay(p);
            v.modMatrix.dstBtn[i].setPopupOverlay(p);
        }
    }

    // Debug overlay — translucent yellow rectangles around every field so
    // the layout-to-PNG mapping can be visually verified.  Set to false
    // (or remove the call) once the layout is confirmed correct.
    v.setDebugOverlay(true);

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

    // ── Waveform selectors (radio buttons, int 0-5) ────────
    auto bindWaveform = [&](SIDOscillatorPanel& osc, const juce::String& paramId) {
        int cur = int(std::round(*apvts.getRawParameterValue(paramId)));
        osc.setWaveform(cur);
        osc.onWaveformChanged = [&apvts, paramId](int w) {
            if (auto* p = apvts.getParameter(paramId))
                p->setValueNotifyingHost((float)w / 6.0f); // range 0-6 (7 waves) → normalized 0-1
        };
    };
    bindWaveform(v.osc1, "osc1_wave");
    bindWaveform(v.osc2, "osc2_wave");
    bindWaveform(v.osc3, "osc3_wave");

    // ── OSC 1 ───────────────────────────────────────────────
    bindKnob(v.osc1.semiKnob,      "osc1_semi");
    bindKnob(v.osc1.fineKnob,      "osc1_fine");
    bindKnob(v.osc1.pwKnob,        "osc1_pw");
    bindKnob(v.osc1.volumeKnob,    "osc1_volume");
    bindKnob(v.osc1.attackKnob,  "osc1_attack");
    bindKnob(v.osc1.decayKnob,   "osc1_decay");
    bindKnob(v.osc1.sustainKnob, "osc1_sustain");
    bindKnob(v.osc1.releaseKnob, "osc1_release");
    bindToggle(v.osc1.superBtn,        "osc1_super_on");
    bindKnob  (v.osc1.superDetuneKnob, "osc1_super_detune");
    bindKnob  (v.osc1.superMixKnob,    "osc1_super_mix");
    {
        int cur = int(std::round(*apvts.getRawParameterValue("osc1_super_voices")));
        v.osc1.superVoicesBtn.setIndex(std::clamp(cur, 0, 2));
        v.osc1.superVoicesBtn.onChanged = [&apvts](int i) {
            if (auto* p = apvts.getParameter("osc1_super_voices"))
                p->setValueNotifyingHost((float)i / 2.0f);   // 3 options
        };
    }

    // ── OSC 2 ───────────────────────────────────────────────
    bindKnob(v.osc2.semiKnob,      "osc2_semi");
    bindKnob(v.osc2.fineKnob,      "osc2_fine");
    bindKnob(v.osc2.pwKnob,        "osc2_pw");
    bindKnob(v.osc2.volumeKnob,    "osc2_volume");
    bindKnob(v.osc2.attackKnob,  "osc2_attack");
    bindKnob(v.osc2.decayKnob,   "osc2_decay");
    bindKnob(v.osc2.sustainKnob, "osc2_sustain");
    bindKnob(v.osc2.releaseKnob, "osc2_release");
    bindToggle(v.osc2.superBtn,        "osc2_super_on");
    bindKnob  (v.osc2.superDetuneKnob, "osc2_super_detune");
    bindKnob  (v.osc2.superMixKnob,    "osc2_super_mix");
    {
        int cur = int(std::round(*apvts.getRawParameterValue("osc2_super_voices")));
        v.osc2.superVoicesBtn.setIndex(std::clamp(cur, 0, 2));
        v.osc2.superVoicesBtn.onChanged = [&apvts](int i) {
            if (auto* p = apvts.getParameter("osc2_super_voices"))
                p->setValueNotifyingHost((float)i / 2.0f);
        };
    }

    // ── OSC 3 ───────────────────────────────────────────────
    bindKnob(v.osc3.semiKnob,      "osc3_semi");
    bindKnob(v.osc3.fineKnob,      "osc3_fine");
    bindKnob(v.osc3.pwKnob,        "osc3_pw");
    bindKnob(v.osc3.volumeKnob,    "osc3_volume");
    bindKnob(v.osc3.attackKnob,  "osc3_attack");
    bindKnob(v.osc3.decayKnob,   "osc3_decay");
    bindKnob(v.osc3.sustainKnob, "osc3_sustain");
    bindKnob(v.osc3.releaseKnob, "osc3_release");
    bindToggle(v.osc3.superBtn,        "osc3_super_on");
    bindKnob  (v.osc3.superDetuneKnob, "osc3_super_detune");
    bindKnob  (v.osc3.superMixKnob,    "osc3_super_mix");
    {
        int cur = int(std::round(*apvts.getRawParameterValue("osc3_super_voices")));
        v.osc3.superVoicesBtn.setIndex(std::clamp(cur, 0, 2));
        v.osc3.superVoicesBtn.onChanged = [&apvts](int i) {
            if (auto* p = apvts.getParameter("osc3_super_voices"))
                p->setValueNotifyingHost((float)i / 2.0f);
        };
    }

    // ── Filter ───────────────────────────────────────────────
    bindKnob(v.filterPanel.cutoffKnob,    "filter_cutoff");
    bindKnob(v.filterPanel.resonanceKnob, "filter_res");
    bindToggle(v.filterPanel.keyTrackBtn, "filter_keytrack");
    bindToggle(v.filterPanel.velocityBtn, "filter_velocity");
    bindToggle(v.filterPanel.slopeBtn,    "filter_slope");

    // Filter type cycle (0=LP, 1=HP, 2=BP, 3=Notch → normalized /3)
    {
        int cur = int(std::round(*apvts.getRawParameterValue("filter_type")));
        v.filterPanel.typeBtn.setIndex(cur);
        v.filterPanel.typeBtn.onChanged = [&apvts](int i) {
            if (auto* p = apvts.getParameter("filter_type"))
                p->setValueNotifyingHost((float)i / 3.0f);
        };
    }

    // Filter ENV
    bindKnob(v.filterPanel.envAmountKnob,  "filter_env_amount");
    bindKnob(v.filterPanel.envAttackKnob,  "filter_env_attack");
    bindKnob(v.filterPanel.envDecayKnob,   "filter_env_decay");
    bindKnob(v.filterPanel.envSustainKnob, "filter_env_sustain");
    bindKnob(v.filterPanel.envReleaseKnob, "filter_env_release");

    // ── Amplifier ────────────────────────────────────────────
    bindKnob(v.ampPanel.attackKnob,  "amp_attack");
    bindKnob(v.ampPanel.decayKnob,   "amp_decay");
    bindKnob(v.ampPanel.sustainKnob, "amp_sustain");
    bindKnob(v.ampPanel.releaseKnob, "amp_release");
    bindKnob(v.ampPanel.volumeKnob,  "amp_volume");

    // ── Master ───────────────────────────────────────────────
    // The previous output fader was removed; master_volume is now driven
    // exclusively by the top-header rotary next to the TranceSID logo.
    bindKnob (v.masterVolKnob,               "master_volume");

    // ── Noise generator ─────────────────────────────────────
    bindKnob  (v.noisePanel.attackKnob,  "noise_attack");
    bindKnob  (v.noisePanel.decayKnob,   "noise_decay");
    bindKnob  (v.noisePanel.sustainKnob, "noise_sustain");
    bindKnob  (v.noisePanel.releaseKnob, "noise_release");
    bindKnob  (v.noisePanel.levelKnob,   "noise_level");
    {
        int cur = int(std::round(*apvts.getRawParameterValue("noise_type")));
        v.noisePanel.typeBtn.setIndex(std::clamp(cur, 0, 3));
        v.noisePanel.typeBtn.onChanged = [&apvts](int i) {
            if (auto* p = apvts.getParameter("noise_type"))
                p->setValueNotifyingHost((float)i / 3.0f);   // 4 options
        };
    }
    bindToggle(v.masterPanel.limiterBtn,     "master_limiter");
    bindToggle(v.masterPanel.polyModeBtn,    "master_poly");

    // ── Chip selector — two clickable chip overlays ─────────
    // Each chip badge in the PNG header has an invisible SIDChipClickArea
    // on top.  Clicking either chip sets chip_model and updates the
    // highlight on both areas so the active model is visually obvious.
    // Same callback pattern as every other Visage widget (the
    // stateJustLoaded → bindAllParameters refresh covers preset-load and
    // host automation, so this is functionally equivalent to a JUCE
    // ButtonAttachment).
    {
        auto syncHighlight = [&v, &apvts]() {
            const int cur = int(std::round(apvts.getRawParameterValue("chip_model")->load()));
            v.chip6581Area.setActive(cur == 0);
            v.chip8580Area.setActive(cur == 1);
        };
        syncHighlight();
        auto setChip = [&apvts, syncHighlight](int idx) {
            if (auto* pp = apvts.getParameter("chip_model"))
                pp->setValueNotifyingHost(idx == 0 ? 0.0f : 1.0f);
            syncHighlight();
        };
        v.chip6581Area.onClicked = [setChip]() { setChip(0); };
        v.chip8580Area.onClicked = [setChip]() { setChip(1); };
    }

    // ── LFOs ────────────────────────────────────────────────
    // LFO 1
    bindToggle(v.lfo1.onBtn,     "lfo1_on");
    bindKnob  (v.lfo1.rateKnob,  "lfo1_rate");
    bindToggle(v.lfo1.syncBtn,   "lfo1_sync");
    bindToggle(v.lfo1.retrigBtn, "lfo1_retrig");
    {
        int cur = int(std::round(*apvts.getRawParameterValue("lfo1_shape")));
        v.lfo1.shapeBtn.setIndex(cur);
        v.lfo1.setStepEditorActive(cur == 6);
        v.lfo1.shapeBtn.onChanged = [&apvts, &lfo = v.lfo1](int i) {
            if (auto* p = apvts.getParameter("lfo1_shape"))
                p->setValueNotifyingHost((float)i / 6.0f);  // range 0..6 → 7 options
            lfo.setStepEditorActive(i == 6);
        };
    }
    // LFO 1 step curve (16 bars, -1..+1, active only in STEP shape)
    {
        for (int i = 0; i < 16; ++i) {
            char id[24]; snprintf(id, sizeof(id), "lfo1_step_%02d", i + 1);
            v.lfo1.stepEditor.setValue(i, *apvts.getRawParameterValue(id));
        }
        v.lfo1.stepEditor.onStepChanged = [&apvts](int step, float val) {
            char id[24]; snprintf(id, sizeof(id), "lfo1_step_%02d", step + 1);
            if (auto* p = apvts.getParameter(id))
                p->setValueNotifyingHost(p->convertTo0to1(val));
        };
    }
    // LFO 1 sync division (int 0-12: 0-7=straight, 8-10=triplets, 11-12=dotted)
    {
        int cur = int(std::round(*apvts.getRawParameterValue("lfo1_div")));
        v.lfo1.syncDivBtn.setIndex(std::clamp(cur, 0, 12));
        v.lfo1.syncDivBtn.onChanged = [&apvts](int i) {
            if (auto* p = apvts.getParameter("lfo1_div"))
                p->setValueNotifyingHost((float)i / 12.0f);
        };
    }
    // LFO 2
    bindToggle(v.lfo2.onBtn,     "lfo2_on");
    bindKnob  (v.lfo2.rateKnob,  "lfo2_rate");
    bindToggle(v.lfo2.syncBtn,   "lfo2_sync");
    bindToggle(v.lfo2.retrigBtn, "lfo2_retrig");
    {
        int cur = int(std::round(*apvts.getRawParameterValue("lfo2_shape")));
        v.lfo2.shapeBtn.setIndex(cur);
        v.lfo2.setStepEditorActive(cur == 6);
        v.lfo2.shapeBtn.onChanged = [&apvts, &lfo = v.lfo2](int i) {
            if (auto* p = apvts.getParameter("lfo2_shape"))
                p->setValueNotifyingHost((float)i / 6.0f);  // range 0..6 → 7 options
            lfo.setStepEditorActive(i == 6);
        };
    }
    // LFO 2 step curve (16 bars, -1..+1, active only in STEP shape)
    {
        for (int i = 0; i < 16; ++i) {
            char id[24]; snprintf(id, sizeof(id), "lfo2_step_%02d", i + 1);
            v.lfo2.stepEditor.setValue(i, *apvts.getRawParameterValue(id));
        }
        v.lfo2.stepEditor.onStepChanged = [&apvts](int step, float val) {
            char id[24]; snprintf(id, sizeof(id), "lfo2_step_%02d", step + 1);
            if (auto* p = apvts.getParameter(id))
                p->setValueNotifyingHost(p->convertTo0to1(val));
        };
    }
    // LFO 2 sync division (int 0-12: 0-7=straight, 8-10=triplets, 11-12=dotted)
    {
        int cur = int(std::round(*apvts.getRawParameterValue("lfo2_div")));
        v.lfo2.syncDivBtn.setIndex(std::clamp(cur, 0, 12));
        v.lfo2.syncDivBtn.onChanged = [&apvts](int i) {
            if (auto* p = apvts.getParameter("lfo2_div"))
                p->setValueNotifyingHost((float)i / 12.0f);
        };
    }

    // ── Mod Matrix (4 slots) ────────────────────────────────
    for (int i = 0; i < 4; ++i) {
        const juce::String pfx = "mod" + juce::String(i + 1) + "_";

        // Source cycle button (0–5 → normalize / 5)
        {
            int cur = int(std::round(*apvts.getRawParameterValue(pfx + "src")));
            v.modMatrix.srcBtn[i].setIndex(cur);
            v.modMatrix.srcBtn[i].onChanged = [&apvts, pfx](int idx) {
                if (auto* p = apvts.getParameter(pfx + "src"))
                    p->setValueNotifyingHost(float(idx) / 5.0f);
            };
        }

        // Amount knob — parameter range -1..+1, normalized by APVTS
        bindKnob(v.modMatrix.amtKnob[i], pfx + "amt");

        // Destination cycle button (0–7 → normalize / 7)
        {
            int cur = int(std::round(*apvts.getRawParameterValue(pfx + "dst")));
            v.modMatrix.dstBtn[i].setIndex(cur);
            v.modMatrix.dstBtn[i].onChanged = [&apvts, pfx](int idx) {
                if (auto* p = apvts.getParameter(pfx + "dst"))
                    p->setValueNotifyingHost(float(idx) / 7.0f);
            };
        }
    }

    // ── Effects ─────────────────────────────────────────────
    bindToggle(v.effectsPanel.chorusBtn,    "fx_chorus_on");
    bindKnob(v.effectsPanel.chorusRate,     "fx_chorus_rate");
    bindKnob(v.effectsPanel.chorusDepth,    "fx_chorus_depth");
    bindKnob(v.effectsPanel.chorusMix,      "fx_chorus_mix");
    bindToggle(v.effectsPanel.delayBtn,     "fx_delay_on");
    // Delay time division cycle (int 0-6, 7 options → normalize /6)
    {
        int cur = int(std::round(*apvts.getRawParameterValue("fx_delay_time")));
        v.effectsPanel.delayTimeBtn.setIndex(std::clamp(cur, 0, 6));
        v.effectsPanel.delayTimeBtn.onChanged = [&apvts](int i) {
            if (auto* p = apvts.getParameter("fx_delay_time"))
                p->setValueNotifyingHost((float)i / 6.0f);
        };
    }
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
    bindToggle(v.arpPanel.arpOnBtn, "arp_on");
    bindToggle(v.arpPanel.syncBtn,  "arp_sync");

    // ── Arp cycle controls ───────────────────────────────────
    // Helper: find closest index in a float array
    auto closestIdx = [](const float* vals, int n, float v) {
        int best = 0;
        float bestD = std::abs(vals[0] - v);
        for (int i = 1; i < n; ++i) {
            float d = std::abs(vals[i] - v);
            if (d < bestD) { bestD = d; best = i; }
        }
        return best;
    };

    // MODE  (int 0-4, 5 choices)
    if (auto* p = apvts.getParameter("arp_mode")) {
        int cur = int(std::round(*apvts.getRawParameterValue("arp_mode")));
        v.arpPanel.modeBtn.setIndex(cur);
        v.arpPanel.modeBtn.onChanged = [&apvts](int i) {
            if (auto* p2 = apvts.getParameter("arp_mode"))
                p2->setValueNotifyingHost((float)i / 4.0f);
        };
        (void)p;
    }

    // OCTAVE  (int 1-4, button index 0-3)
    if (auto* p = apvts.getParameter("arp_octave")) {
        int cur = int(std::round(*apvts.getRawParameterValue("arp_octave"))); // 1-4
        v.arpPanel.octaveBtn.setIndex(cur - 1);
        v.arpPanel.octaveBtn.onChanged = [&apvts](int i) {
            if (auto* p2 = apvts.getParameter("arp_octave"))
                p2->setValueNotifyingHost((float)i / 3.0f);
        };
        (void)p;
    }

    // GATE  (float 0-1, 6 presets)
    {
        static constexpr float gateVals[] = {0.1f, 0.25f, 0.5f, 0.75f, 0.9f, 1.0f};
        float cur = *apvts.getRawParameterValue("arp_gate");
        v.arpPanel.gateBtn.setIndex(closestIdx(gateVals, 6, cur));
        v.arpPanel.gateBtn.onChanged = [&apvts](int i) {
            static constexpr float vals[] = {0.1f, 0.25f, 0.5f, 0.75f, 0.9f, 1.0f};
            if (auto* p = apvts.getParameter("arp_gate"))
                p->setValueNotifyingHost(vals[i]); // range 0-1, normalized = raw
        };
    }

    // SWING  (float 0-0.5, 4 presets)
    {
        static constexpr float swingVals[] = {0.0f, 0.1f, 0.165f, 0.25f};
        float cur = *apvts.getRawParameterValue("arp_swing");
        v.arpPanel.swingBtn.setIndex(closestIdx(swingVals, 4, cur));
        v.arpPanel.swingBtn.onChanged = [&apvts](int i) {
            static constexpr float vals[] = {0.0f, 0.1f, 0.165f, 0.25f};
            if (auto* p = apvts.getParameter("arp_swing"))
                p->setValueNotifyingHost(vals[i] / 0.5f); // normalize 0-0.5 → 0-1
        };
    }

    // TEMPO  (float 60-200, 8 presets)
    {
        static constexpr float tempoVals[] = {60.f,80.f,100.f,120.f,140.f,160.f,180.f,200.f};
        float cur = *apvts.getRawParameterValue("arp_tempo");
        v.arpPanel.tempoBtn.setIndex(closestIdx(tempoVals, 8, cur));
        v.arpPanel.tempoBtn.onChanged = [&apvts](int i) {
            static constexpr float vals[] = {60.f,80.f,100.f,120.f,140.f,160.f,180.f,200.f};
            if (auto* p = apvts.getParameter("arp_tempo"))
                p->setValueNotifyingHost((vals[i] - 60.f) / 140.f); // normalize 60-200 → 0-1
        };
    }

    // ── Voice Mod Panel ─────────────────────────────────────
    // Unison voices (int 1–7, button index 0–6, normalize = index/6)
    {
        int cur = int(std::round(*apvts.getRawParameterValue("unison_voices")));
        v.voiceModPanel.uniVoicesBtn.setIndex(std::clamp(cur - 1, 0, 15));
        v.voiceModPanel.uniVoicesBtn.onChanged = [&apvts](int i) {
            if (auto* p = apvts.getParameter("unison_voices"))
                p->setValueNotifyingHost((float)i / 15.0f);
        };
    }
    bindKnob(v.voiceModPanel.uniDetuneKnob, "unison_detune");
    bindKnob(v.voiceModPanel.uniSpreadKnob, "unison_spread");

    // Glide mode (int 0–2, button index 0–2, normalize = index/2)
    {
        int cur = int(std::round(*apvts.getRawParameterValue("glide_mode")));
        v.voiceModPanel.glideModeBtn.setIndex(std::clamp(cur, 0, 2));
        v.voiceModPanel.glideModeBtn.onChanged = [&apvts](int i) {
            if (auto* p = apvts.getParameter("glide_mode"))
                p->setValueNotifyingHost((float)i / 2.0f);
        };
    }
    bindKnob(v.voiceModPanel.glideTimeKnob, "glide_time");

    // OSC Sync (toggle) + FM amount (knob)
    bindToggle(v.voiceModPanel.oscSyncBtn, "osc_sync");
    bindKnob  (v.voiceModPanel.fmAmtKnob,  "osc_fm_amt");

    // Drive type (int 0–3, button index 0–3, normalize = index/3)
    {
        int cur = int(std::round(*apvts.getRawParameterValue("drive_type")));
        v.voiceModPanel.driveTypeBtn.setIndex(std::clamp(cur, 0, 3));
        v.voiceModPanel.driveTypeBtn.onChanged = [&apvts](int i) {
            if (auto* p = apvts.getParameter("drive_type"))
                p->setValueNotifyingHost((float)i / 3.0f);
        };
    }
    bindKnob(v.voiceModPanel.driveAmtKnob, "drive_amount");

    // ── Stereo engine (voice mod's new STEREO column) ────────
    bindToggle(v.voiceModPanel.randPhaseBtn, "voice_rand_phase");
    bindKnob(v.voiceModPanel.oscPanKnob[0],  "osc1_pan");
    bindKnob(v.voiceModPanel.oscPanKnob[1],  "osc2_pan");
    bindKnob(v.voiceModPanel.oscPanKnob[2],  "osc3_pan");
    bindKnob(v.voiceModPanel.oscHaasKnob[0], "osc1_haas");
    bindKnob(v.voiceModPanel.oscHaasKnob[1], "osc2_haas");
    bindKnob(v.voiceModPanel.oscHaasKnob[2], "osc3_haas");

    // ── Macro Panel ─────────────────────────────────────────
    bindKnob(v.macroPanel.macro1, "macro1");
    bindKnob(v.macroPanel.macro2, "macro2");
    bindKnob(v.macroPanel.macro3, "macro3");
    bindKnob(v.macroPanel.macro4, "macro4");

    // ── Trance Gate Panel ───────────────────────────────────
    bindToggle(v.gatePanel.gateOnBtn, "gate_on");

    // Gate swing (float 0–0.5, 4 presets: 0, 0.1, 0.165, 0.25)
    {
        static constexpr float gateSwingVals[] = {0.0f, 0.1f, 0.165f, 0.25f};
        float cur = *apvts.getRawParameterValue("gate_swing");
        v.gatePanel.gateSwingBtn.setIndex(closestIdx(gateSwingVals, 4, cur));
        v.gatePanel.gateSwingBtn.onChanged = [&apvts](int i) {
            static constexpr float vals[] = {0.0f, 0.1f, 0.165f, 0.25f};
            if (auto* p = apvts.getParameter("gate_swing"))
                p->setValueNotifyingHost(vals[i] / 0.5f); // normalize 0-0.5 → 0-1
        };
    }

    // Gate sequencer steps (16 toggle buttons)
    for (int i = 0; i < 16; ++i) {
        const juce::String id = juce::String::formatted("gate_step_%02d", i + 1);
        bindStep(v.gatePanel.steps[i], id);
    }
}

void SIDTranceAudioProcessorEditor::onRender()
{
    if (!mainView) return;

    // ── Preset-load refresh ──────────────────────────────────
    // setStateInformation runs on the message thread and sets this flag.
    // We read it here (render thread = message thread for Visage) and rebind
    // all widgets, because Visage controls are not APVTS listeners.
    if (audioProcessor.stateJustLoaded.exchange(false))
        bindAllParameters();

    // ── LFO sync-mode visual toggle ──────────────────────────
    // When sync is on, the rate knob is irrelevant — show the division selector
    // instead; when off, show the rate knob. setSyncActive() is idempotent.
    {
        auto& apvts = audioProcessor.getAPVTS();
        const bool s1 = apvts.getRawParameterValue("lfo1_sync")->load() > 0.5f;
        const bool s2 = apvts.getRawParameterValue("lfo2_sync")->load() > 0.5f;
        mainView->lfo1.setSyncActive(s1);
        mainView->lfo2.setSyncActive(s2);
    }

    // ── Live scope labels — update every render frame ─────────
    {
        static const std::array<std::string, 7> waveNames = {
            "SAW", "TRI", "PLS", "NOI", "S+T", "RNG", "HSW"
        };
        static const std::array<std::string, 4> filterTypeNames = {
            "LP", "HP", "BP", "Notch"
        };

        auto& apvts = audioProcessor.getAPVTS();

        const int w1 = int(std::round(*apvts.getRawParameterValue("osc1_wave")));
        const int w2 = int(std::round(*apvts.getRawParameterValue("osc2_wave")));
        const int w3 = int(std::round(*apvts.getRawParameterValue("osc3_wave")));
        mainView->scope1.setLabel("OSC 1: " + waveNames[std::clamp(w1, 0, 6)]);
        mainView->scope2.setLabel("OSC 2: " + waveNames[std::clamp(w2, 0, 6)]);
        mainView->scope3.setLabel("OSC 3: " + waveNames[std::clamp(w3, 0, 6)]);

        const float cutoff = *apvts.getRawParameterValue("filter_cutoff");
        const float res    = *apvts.getRawParameterValue("filter_res");
        const int   ftype  = std::clamp(int(std::round(*apvts.getRawParameterValue("filter_type"))), 0, 3);
        const bool  slope  = *apvts.getRawParameterValue("filter_slope") > 0.5f;

        mainView->filterScope.setFilterState(
            filterTypeNames[ftype],
            slope ? "24dB" : "12dB",
            cutoff,
            res);
    }

    // ── ADSR envelope curve generation ───────────────────────
    // Generates a shape buffer from APVTS values and pushes to each OSC envelope display.
    // Called every render frame so the curve updates in real time as parameters change.
    {
        // Returns normalised value 0..1 for time position t (0..1)
        auto generateAdsr = [](float a, float d, float s, float r, float* buf, int n) {
            const float sustainHold = 0.3f;   // fraction of total to show sustain plateau
            const float totalT = a + d + sustainHold + r + 1e-6f;
            const float aEnd   = a / totalT;
            const float dEnd   = (a + d) / totalT;
            const float sEnd   = (a + d + sustainHold) / totalT;

            for (int i = 0; i < n; ++i) {
                const float t = float(i) / float(n - 1);
                float v;
                if      (t <= aEnd) v = (aEnd > 1e-6f) ? (t / aEnd) : 1.0f;
                else if (t <= dEnd) v = 1.0f - ((t - aEnd) / (dEnd - aEnd + 1e-6f)) * (1.0f - s);
                else if (t <= sEnd) v = s;
                else                v = s * (1.0f - (t - sEnd) / (1.0f - sEnd + 1e-6f));
                buf[i] = v * 2.0f - 1.0f;   // map 0..1 → -1..+1 for SIDOscilloscopeView
            }
        };

        // adsrBuf_ is a member variable — not static, so each instance has its own buffer.
        // (Static locals were shared across all instances, corrupting envelope displays
        //  when two SIDyssey instances were open in the same DAW process.)
        auto& apvts2 = audioProcessor.getAPVTS();

        // OSC 1 envelope display
        generateAdsr(
            *apvts2.getRawParameterValue("osc1_attack"),
            *apvts2.getRawParameterValue("osc1_decay"),
            *apvts2.getRawParameterValue("osc1_sustain"),
            *apvts2.getRawParameterValue("osc1_release"),
            adsrBuf_, 512);
        mainView->osc1.envelopeDisplay.updateSamples(adsrBuf_, 512);

        // OSC 2 envelope display
        generateAdsr(
            *apvts2.getRawParameterValue("osc2_attack"),
            *apvts2.getRawParameterValue("osc2_decay"),
            *apvts2.getRawParameterValue("osc2_sustain"),
            *apvts2.getRawParameterValue("osc2_release"),
            adsrBuf_, 512);
        mainView->osc2.envelopeDisplay.updateSamples(adsrBuf_, 512);

        // OSC 3 envelope display
        generateAdsr(
            *apvts2.getRawParameterValue("osc3_attack"),
            *apvts2.getRawParameterValue("osc3_decay"),
            *apvts2.getRawParameterValue("osc3_sustain"),
            *apvts2.getRawParameterValue("osc3_release"),
            adsrBuf_, 512);
        mainView->osc3.envelopeDisplay.updateSamples(adsrBuf_, 512);
    }

    // ── Per-oscillator scopes: drain each FIFO into the corresponding view ──
    // Lock-free, no allocations.  Each OSC has its own ring buffer fed by
    // processBlock; we read at the editor's render rate.  The scratch buffer
    // holds 2× the display width so we can run a rising-zero-cross trigger
    // search and align the displayed slice to a stable phase — without that,
    // the waveform appears to flicker/drift each frame.
    auto& proc = audioProcessor;
    static constexpr int kDisplaySamples = SIDTranceAudioProcessorEditor::kScopeMax; // 512
    static constexpr int kReadSamples    = kDisplaySamples * 2;                       // 1024
    float wideScratch[kReadSamples];

    SIDOscilloscopeView* const oscScopes[3] = {
        &mainView->scope1, &mainView->scope2, &mainView->scope3
    };

    for (int o = 0; o < 3; ++o) {
        auto& fifo = proc.oscScope[o].fifo;
        const int avail = fifo.getNumReady();
        if (avail <= 0) continue;   // hold previous frame

        // Read up to kReadSamples newest samples.  Discard older ones first.
        const int toShow    = std::min(avail, kReadSamples);
        const int toDiscard = avail - toShow;
        if (toDiscard > 0) {
            int ds1, dsz1, ds2, dsz2;
            fifo.prepareToRead(toDiscard, ds1, dsz1, ds2, dsz2);
            fifo.finishedRead(toDiscard);
        }

        int s1, sz1, s2, sz2;
        fifo.prepareToRead(toShow, s1, sz1, s2, sz2);
        const float* src = proc.oscScope[o].buf.getReadPointer(0);
        int written = 0;
        for (int i = 0; i < sz1 && written < kReadSamples; ++i, ++written)
            wideScratch[written] = src[s1 + i];
        for (int i = 0; i < sz2 && written < kReadSamples; ++i, ++written)
            wideScratch[written] = src[s2 + i];
        fifo.finishedRead(toShow);

        // Need at least one display window — otherwise hold previous frame.
        if (written < kDisplaySamples) continue;

        // Silence detection: if the entire buffer is essentially zero, draw
        // a flat line (just clear localBuf_ — the scope renders centre line).
        float peak = 0.0f;
        for (int i = 0; i < written; ++i)
            peak = std::max(peak, std::abs(wideScratch[i]));

        if (peak < 0.002f) {
            std::memset(localBuf_, 0, sizeof(localBuf_));
            oscScopes[o]->updateSamples(localBuf_, kDisplaySamples);
            continue;
        }

        // Trigger sync: find first rising zero-crossing in the search window
        // (the portion of the buffer before the display window).  Locks the
        // displayed slice to a consistent phase so the waveform stands still.
        const int searchEnd = written - kDisplaySamples;   // last index that still leaves room
        int triggerIdx = 0;
        for (int i = 1; i < searchEnd; ++i) {
            if (wideScratch[i - 1] < 0.0f && wideScratch[i] >= 0.0f) {
                triggerIdx = i;
                break;
            }
        }

        // Copy the display window starting at the trigger point.
        std::memcpy(localBuf_, wideScratch + triggerIdx,
                    sizeof(float) * kDisplaySamples);
        oscScopes[o]->updateSamples(localBuf_, kDisplaySamples);
    }

    // filterScope is now SIDFilterInfoView — driven by setFilterState() above, not audio data

    // Output VU meter — pull the processor's atomic peak values (updated
    // each block in processBlock) and push them into the widget.  Cheap
    // and lock-free; the meter only redraws when the values actually change.
    mainView->outputMeter.setLevels(
        audioProcessor.outPeakL.load(std::memory_order_relaxed),
        audioProcessor.outPeakR.load(std::memory_order_relaxed));
}

void SIDTranceAudioProcessorEditor::onDestroy()
{
    if (mainView) {
        removeFrameFromCanvas(mainView.get());
        mainView.reset();
    }
}

// ============================================================
//  Factory Preset Data
//  30 trance-focused presets covering every major sound class
//  (plucks, supersaws, basses, gated pads, arps, stabs, FX pads).
//  Each preset is a list of (paramId, denormalized value) pairs
//  applied on top of the Init (all-defaults) state via
//  applyFactoryPreset().  All IDs are validated against
//  createParameterLayout(); the apvts will silently ignore an
//  unknown ID, so every line below has been cross-checked.
// ============================================================
namespace {
    struct FP { const char* id; float v; };

    // ═════════════════════════════ PLUCKS (6) ═════════════════════════════
    static const FP kFP_PluckGlassSting[] = {
        {"osc1_wave",0.f},{"osc1_volume",0.85f},
        {"osc2_wave",1.f},{"osc2_fine",7.f},{"osc2_volume",0.40f},
        {"osc3_volume",0.f},
        {"filter_cutoff",4500.f},{"filter_res",0.70f},
        {"filter_env_amount",0.90f},{"filter_env_attack",0.001f},
        {"filter_env_decay",0.10f},{"filter_env_sustain",0.0f},
        {"amp_attack",0.001f},{"amp_decay",0.15f},{"amp_sustain",0.0f},{"amp_release",0.30f},
        {"chip_model",1.f},{"analog_glow",0.20f},
        {"fx_delay_mix",0.18f},{"fx_reverb_mix",0.30f},
    };
    static const FP kFP_PluckCrystalBell[] = {
        {"osc1_wave",1.f},{"osc1_volume",0.85f},
        {"osc2_wave",0.f},{"osc2_fine",12.f},{"osc2_volume",0.45f},
        {"osc3_volume",0.f},
        {"filter_cutoff",3500.f},{"filter_res",0.55f},
        {"filter_env_amount",0.75f},{"filter_env_decay",0.18f},{"filter_env_sustain",0.05f},
        {"amp_attack",0.001f},{"amp_decay",0.20f},{"amp_sustain",0.0f},{"amp_release",0.40f},
        {"chip_model",1.f},
        {"fx_chorus_mix",0.30f},{"fx_reverb_mix",0.40f},
    };
    static const FP kFP_PluckHammer[] = {
        {"osc1_wave",2.f},{"osc1_pw",0.25f},{"osc1_volume",0.90f},
        {"osc2_wave",0.f},{"osc2_fine",-5.f},{"osc2_volume",0.55f},
        {"osc3_volume",0.f},
        {"filter_cutoff",2200.f},{"filter_res",0.75f},
        {"filter_env_amount",0.85f},{"filter_env_decay",0.12f},{"filter_env_sustain",0.0f},
        {"amp_attack",0.001f},{"amp_decay",0.18f},{"amp_sustain",0.0f},{"amp_release",0.25f},
        {"drive_type",0.f},{"drive_amount",0.20f},
        {"chip_model",1.f},
        {"fx_reverb_mix",0.25f},
    };
    static const FP kFP_PluckIceBell[] = {
        {"osc1_wave",5.f},{"osc1_volume",0.85f},                    // ring mod
        {"osc2_wave",1.f},{"osc2_semi",12.f},{"osc2_volume",0.55f},
        {"osc3_wave",0.f},{"osc3_semi",-12.f},{"osc3_volume",0.50f},
        {"filter_cutoff",5000.f},{"filter_res",0.45f},
        {"filter_env_amount",0.70f},{"filter_env_decay",0.20f},{"filter_env_sustain",0.0f},
        {"amp_attack",0.001f},{"amp_decay",0.30f},{"amp_sustain",0.0f},{"amp_release",0.60f},
        {"chip_model",1.f},
        {"fx_reverb_size",0.70f},{"fx_reverb_mix",0.50f},
    };
    static const FP kFP_PluckSID6581[] = {
        {"osc1_wave",2.f},{"osc1_pw",0.40f},{"osc1_volume",0.90f},
        {"osc2_wave",0.f},{"osc2_fine",4.f},{"osc2_volume",0.55f},
        {"osc3_wave",3.f},{"osc3_volume",0.18f},                    // noise sprinkle
        {"filter_cutoff",1800.f},{"filter_res",0.78f},
        {"filter_env_amount",0.95f},{"filter_env_decay",0.10f},{"filter_env_sustain",0.0f},
        {"amp_attack",0.001f},{"amp_decay",0.18f},{"amp_sustain",0.0f},{"amp_release",0.20f},
        {"chip_model",0.f},                                          // 6581 — aliased grit
        {"digital_age",0.25f},{"analog_glow",0.30f},
        {"fx_reverb_mix",0.20f},
    };
    static const FP kFP_PluckSubKnock[] = {
        {"osc1_wave",1.f},{"osc1_semi",-12.f},{"osc1_volume",0.95f},
        {"osc2_wave",0.f},{"osc2_semi",-24.f},{"osc2_volume",0.45f},
        {"osc3_volume",0.f},
        {"filter_cutoff",700.f},{"filter_res",0.60f},
        {"filter_env_amount",0.85f},{"filter_env_decay",0.10f},{"filter_env_sustain",0.0f},
        {"amp_attack",0.001f},{"amp_decay",0.12f},{"amp_sustain",0.0f},{"amp_release",0.18f},
        {"drive_type",0.f},{"drive_amount",0.30f},
        {"chip_model",1.f},
    };

    // ══════════════════════ SUPERSAW / DETUNED LEADS (5) ═══════════════════
    static const FP kFP_LeadHypersaw7[] = {
        {"osc1_wave",6.f},{"osc1_pw",0.85f},{"osc1_volume",0.95f},
        {"osc2_volume",0.f},{"osc3_volume",0.f},
        {"filter_cutoff",4500.f},{"filter_res",0.40f},
        {"filter_env_amount",0.50f},{"filter_env_decay",0.40f},{"filter_env_sustain",0.15f},
        {"amp_attack",0.008f},{"amp_sustain",0.85f},{"amp_release",0.70f},
        {"trance_drift",0.25f},{"analog_glow",0.30f},
        {"unison_voices",5.f},{"unison_detune",28.f},{"unison_spread",0.85f},
        {"chip_model",1.f},
        {"fx_chorus_mix",0.40f},{"fx_reverb_mix",0.30f},{"fx_delay_mix",0.15f},
    };
    static const FP kFP_LeadStackedSaws[] = {
        {"osc1_wave",0.f},{"osc1_volume",0.85f},
        {"osc2_wave",0.f},{"osc2_fine",-9.f},{"osc2_volume",0.80f},
        {"osc3_wave",0.f},{"osc3_fine",11.f},{"osc3_volume",0.55f},
        {"filter_cutoff",3000.f},{"filter_res",0.45f},
        {"filter_env_amount",0.55f},{"filter_env_decay",0.30f},
        {"amp_attack",0.005f},{"amp_sustain",0.88f},{"amp_release",0.60f},
        {"unison_voices",3.f},{"unison_detune",18.f},{"unison_spread",0.70f},
        {"chip_model",1.f},{"trance_drift",0.18f},
        {"fx_chorus_mix",0.35f},{"fx_reverb_mix",0.25f},
    };
    static const FP kFP_LeadWideDetune[] = {
        {"osc1_wave",0.f},{"osc1_volume",0.90f},
        {"osc2_wave",2.f},{"osc2_pw",0.45f},{"osc2_fine",6.f},{"osc2_volume",0.60f},
        {"osc3_volume",0.f},
        {"filter_cutoff",3800.f},{"filter_res",0.50f},
        {"filter_env_amount",0.55f},{"filter_env_decay",0.28f},
        {"amp_attack",0.004f},{"amp_sustain",0.85f},{"amp_release",0.65f},
        {"unison_voices",8.f},{"unison_detune",40.f},{"unison_spread",1.0f},
        {"sid_width",0.85f},{"chip_model",1.f},
        {"fx_chorus_mix",0.45f},{"fx_reverb_mix",0.30f},
    };
    static const FP kFP_LeadAnthem[] = {
        {"osc1_wave",6.f},{"osc1_pw",0.75f},{"osc1_volume",0.85f},
        {"osc2_wave",0.f},{"osc2_semi",-12.f},{"osc2_volume",0.55f},
        {"osc3_volume",0.f},
        {"filter_cutoff",2800.f},{"filter_res",0.55f},
        {"filter_env_amount",0.65f},{"filter_env_decay",0.45f},{"filter_env_sustain",0.20f},
        {"amp_attack",0.010f},{"amp_sustain",0.90f},{"amp_release",1.20f},
        {"unison_voices",4.f},{"unison_detune",22.f},{"unison_spread",0.75f},
        {"trance_drift",0.20f},{"chip_model",1.f},
        {"fx_delay_time",3.f},{"fx_delay_feedback",0.40f},{"fx_delay_mix",0.30f},
        {"fx_reverb_size",0.70f},{"fx_reverb_mix",0.40f},
    };
    static const FP kFP_LeadHoover[] = {
        {"osc1_wave",0.f},{"osc1_semi",12.f},{"osc1_volume",0.85f},
        {"osc2_wave",0.f},{"osc2_fine",-5.f},{"osc2_volume",0.80f},
        {"osc3_wave",2.f},{"osc3_pw",0.30f},{"osc3_volume",0.50f},
        {"osc_fm_amt",0.35f},
        {"filter_cutoff",2400.f},{"filter_res",0.55f},
        {"filter_env_amount",-0.40f},{"filter_env_decay",0.40f},
        {"amp_attack",0.020f},{"amp_sustain",0.85f},{"amp_release",0.60f},
        {"trance_drift",0.30f},{"chip_model",0.f},                  // 6581 grit
        {"fx_chorus_depth",0.65f},{"fx_chorus_mix",0.40f},
        {"fx_reverb_mix",0.30f},
    };

    // ════════════════════ ROLLING / OFFBEAT BASSES (5) ═════════════════════
    static const FP kFP_BassRollingOffbeat[] = {
        {"osc1_wave",0.f},{"osc1_volume",0.95f},
        {"osc2_wave",2.f},{"osc2_pw",0.30f},{"osc2_semi",-12.f},{"osc2_volume",0.55f},
        {"osc3_volume",0.f},
        {"filter_cutoff",450.f},{"filter_res",0.65f},
        {"filter_env_amount",0.75f},{"filter_env_attack",0.001f},
        {"filter_env_decay",0.14f},{"filter_env_sustain",0.10f},
        {"amp_attack",0.001f},{"amp_decay",0.18f},{"amp_sustain",0.0f},{"amp_release",0.20f},
        {"arp_on",1.f},{"arp_mode",4.f},{"arp_octave",1.f},{"arp_gate",0.40f},
        {"analog_glow",0.35f},{"chip_model",0.f},
    };
    static const FP kFP_BassReese[] = {
        {"osc1_wave",0.f},{"osc1_semi",-12.f},{"osc1_volume",0.85f},
        {"osc2_wave",0.f},{"osc2_semi",-12.f},{"osc2_fine",-12.f},{"osc2_volume",0.80f},
        {"osc3_wave",0.f},{"osc3_semi",-12.f},{"osc3_fine",10.f},{"osc3_volume",0.55f},
        {"filter_cutoff",380.f},{"filter_res",0.50f},
        {"filter_env_amount",0.55f},{"filter_env_decay",0.30f},{"filter_env_sustain",0.25f},
        {"amp_attack",0.005f},{"amp_sustain",0.80f},{"amp_release",0.30f},
        {"drive_type",0.f},{"drive_amount",0.35f},
        {"sid_width",0.40f},{"chip_model",1.f},
        {"fx_chorus_mix",0.20f},
    };
    static const FP kFP_BassAcidSquelch[] = {
        {"osc1_wave",0.f},{"osc1_volume",0.95f},
        {"osc2_volume",0.f},{"osc3_volume",0.f},
        {"filter_cutoff",220.f},{"filter_res",0.95f},
        {"filter_env_amount",0.95f},{"filter_env_attack",0.001f},
        {"filter_env_decay",0.18f},{"filter_env_sustain",0.05f},
        {"amp_attack",0.001f},{"amp_decay",0.30f},{"amp_sustain",0.55f},{"amp_release",0.25f},
        {"arp_on",1.f},{"arp_mode",0.f},{"arp_gate",0.55f},
        {"analog_glow",0.50f},{"chip_model",0.f},
        {"fx_delay_mix",0.20f},
    };
    static const FP kFP_BassSubDrone[] = {
        {"osc1_wave",1.f},{"osc1_semi",-12.f},{"osc1_volume",0.95f},
        {"osc2_wave",0.f},{"osc2_semi",-24.f},{"osc2_volume",0.55f},
        {"osc3_volume",0.f},
        {"filter_cutoff",600.f},{"filter_res",0.35f},
        {"filter_env_amount",0.20f},{"filter_env_decay",0.50f},
        {"amp_attack",0.005f},{"amp_sustain",0.90f},{"amp_release",0.60f},
        {"chip_model",1.f},
        {"fx_reverb_mix",0.15f},
    };
    static const FP kFP_BassTechPulse[] = {
        {"osc1_wave",2.f},{"osc1_pw",0.35f},{"osc1_volume",0.90f},
        {"osc2_wave",0.f},{"osc2_semi",-12.f},{"osc2_volume",0.55f},
        {"osc3_wave",1.f},{"osc3_volume",0.30f},
        {"osc_fm_amt",0.20f},
        {"filter_cutoff",550.f},{"filter_res",0.70f},
        {"filter_env_amount",0.80f},{"filter_env_decay",0.16f},{"filter_env_sustain",0.05f},
        {"amp_attack",0.002f},{"amp_decay",0.20f},{"amp_sustain",0.40f},{"amp_release",0.25f},
        {"lfo1_on",1.f},{"lfo1_shape",4.f},{"lfo1_sync",1.f},{"lfo1_div",1.f},
        {"mod1_src",0.f},{"mod1_amt",0.30f},{"mod1_dst",1.f},        // LFO1 → PW1
        {"chip_model",1.f},
    };

    // ══════════════════════════ GATED PADS (4) ═════════════════════════════
    static const FP kFP_GatedPad8thPump[] = {
        {"osc1_wave",0.f},{"osc1_volume",0.85f},
        {"osc2_wave",0.f},{"osc2_fine",7.f},{"osc2_volume",0.70f},
        {"osc3_wave",4.f},{"osc3_fine",-5.f},{"osc3_volume",0.45f},
        {"filter_cutoff",2200.f},{"filter_res",0.45f},
        {"filter_env_amount",0.40f},{"filter_env_decay",0.45f},
        {"amp_attack",0.020f},{"amp_sustain",0.90f},{"amp_release",0.50f},
        {"gate_on",1.f},{"gate_swing",0.05f},
        {"chip_model",1.f},
        {"fx_delay_mix",0.20f},{"fx_reverb_mix",0.30f},
    };
    static const FP kFP_GatedPad16thStutter[] = {
        {"osc1_wave",0.f},{"osc1_volume",0.85f},
        {"osc2_wave",2.f},{"osc2_pw",0.40f},{"osc2_fine",12.f},{"osc2_volume",0.65f},
        {"osc3_volume",0.f},
        {"filter_cutoff",2800.f},{"filter_res",0.55f},
        {"filter_env_amount",0.50f},{"filter_env_decay",0.35f},
        {"amp_attack",0.010f},{"amp_sustain",0.85f},{"amp_release",0.40f},
        {"gate_on",1.f},
        {"gate_step_01",1.f},{"gate_step_02",0.f},{"gate_step_03",1.f},{"gate_step_04",0.f},
        {"gate_step_05",1.f},{"gate_step_06",0.f},{"gate_step_07",1.f},{"gate_step_08",0.f},
        {"gate_step_09",1.f},{"gate_step_10",0.f},{"gate_step_11",1.f},{"gate_step_12",0.f},
        {"gate_step_13",1.f},{"gate_step_14",0.f},{"gate_step_15",1.f},{"gate_step_16",0.f},
        {"chip_model",1.f},
        {"fx_reverb_mix",0.35f},
    };
    static const FP kFP_GatedPadTranceChoir[] = {
        {"osc1_wave",1.f},{"osc1_volume",0.80f},
        {"osc2_wave",4.f},{"osc2_fine",7.f},{"osc2_volume",0.70f},
        {"osc3_wave",0.f},{"osc3_fine",-7.f},{"osc3_volume",0.55f},
        {"filter_cutoff",1800.f},{"filter_res",0.30f},
        {"filter_env_amount",0.25f},{"filter_env_attack",0.50f},
        {"amp_attack",0.50f},{"amp_sustain",0.90f},{"amp_release",1.20f},
        {"unison_voices",3.f},{"unison_detune",18.f},{"unison_spread",0.70f},
        {"gate_on",1.f},
        {"chip_model",1.f},{"trance_drift",0.25f},
        {"fx_chorus_mix",0.45f},{"fx_reverb_size",0.75f},{"fx_reverb_mix",0.45f},
    };
    static const FP kFP_GatedPadSoftSweep[] = {
        {"osc1_wave",4.f},{"osc1_volume",0.80f},
        {"osc2_wave",0.f},{"osc2_fine",11.f},{"osc2_volume",0.55f},
        {"osc3_volume",0.f},
        {"filter_cutoff",1500.f},{"filter_res",0.40f},
        {"filter_env_amount",0.30f},{"filter_env_attack",1.50f},
        {"amp_attack",0.80f},{"amp_sustain",0.90f},{"amp_release",1.50f},
        {"lfo1_on",1.f},{"lfo1_shape",0.f},{"lfo1_rate",0.30f},
        {"mod1_src",0.f},{"mod1_amt",0.35f},{"mod1_dst",0.f},        // LFO1 → cutoff
        {"gate_on",1.f},
        {"chip_model",1.f},
        {"fx_reverb_size",0.80f},{"fx_reverb_mix",0.55f},
    };

    // ═══════════════════════ ARP / SEQUENCER (4) ═══════════════════════════
    static const FP kFP_ArpClassicTrance[] = {
        {"osc1_wave",0.f},{"osc1_volume",0.90f},
        {"osc2_wave",1.f},{"osc2_fine",5.f},{"osc2_volume",0.55f},
        {"osc3_volume",0.f},
        {"filter_cutoff",1400.f},{"filter_res",0.65f},
        {"filter_env_amount",0.65f},{"filter_env_decay",0.18f},{"filter_env_sustain",0.10f},
        {"amp_attack",0.002f},{"amp_decay",0.20f},{"amp_sustain",0.55f},{"amp_release",0.25f},
        {"arp_on",1.f},{"arp_mode",0.f},{"arp_octave",2.f},{"arp_gate",0.65f},
        {"chip_model",1.f},
        {"fx_delay_time",3.f},{"fx_delay_feedback",0.40f},{"fx_delay_mix",0.30f},
        {"fx_reverb_mix",0.25f},
    };
    static const FP kFP_ArpPluckyPattern[] = {
        {"osc1_wave",2.f},{"osc1_pw",0.30f},{"osc1_volume",0.90f},
        {"osc2_wave",0.f},{"osc2_fine",-7.f},{"osc2_volume",0.50f},
        {"osc3_volume",0.f},
        {"filter_cutoff",2400.f},{"filter_res",0.70f},
        {"filter_env_amount",0.80f},{"filter_env_decay",0.12f},{"filter_env_sustain",0.0f},
        {"amp_attack",0.001f},{"amp_decay",0.18f},{"amp_sustain",0.0f},{"amp_release",0.20f},
        {"arp_on",1.f},{"arp_mode",2.f},{"arp_octave",2.f},{"arp_gate",0.55f},
        // Off some seq steps so the pattern isn't perfectly regular
        {"seq_step_03",0.f},{"seq_step_07",0.f},{"seq_step_11",0.f},{"seq_step_15",0.f},
        {"chip_model",1.f},
        {"fx_delay_time",2.f},{"fx_delay_mix",0.25f},
        {"fx_reverb_mix",0.30f},
    };
    static const FP kFP_ArpOctaveClimber[] = {
        {"osc1_wave",0.f},{"osc1_volume",0.90f},
        {"osc2_wave",0.f},{"osc2_fine",12.f},{"osc2_volume",0.50f},
        {"osc3_volume",0.f},
        {"filter_cutoff",2000.f},{"filter_res",0.55f},
        {"filter_env_amount",0.55f},{"filter_env_decay",0.20f},{"filter_env_sustain",0.10f},
        {"amp_attack",0.002f},{"amp_decay",0.20f},{"amp_sustain",0.40f},{"amp_release",0.30f},
        {"arp_on",1.f},{"arp_mode",0.f},{"arp_octave",4.f},{"arp_gate",0.60f},
        {"chip_model",1.f},
        {"fx_delay_time",2.f},{"fx_delay_feedback",0.45f},{"fx_delay_mix",0.35f},
        {"fx_reverb_mix",0.30f},
    };
    static const FP kFP_ArpRandomGlitch[] = {
        {"osc1_wave",2.f},{"osc1_pw",0.50f},{"osc1_volume",0.85f},
        {"osc2_wave",3.f},{"osc2_volume",0.30f},                   // noise sprinkle
        {"osc3_volume",0.f},
        {"filter_cutoff",3000.f},{"filter_res",0.65f},
        {"filter_env_amount",0.75f},{"filter_env_decay",0.12f},{"filter_env_sustain",0.0f},
        {"amp_attack",0.001f},{"amp_decay",0.15f},{"amp_sustain",0.0f},{"amp_release",0.20f},
        {"arp_on",1.f},{"arp_mode",3.f},{"arp_octave",3.f},{"arp_gate",0.50f},
        {"digital_age",0.30f},{"chip_model",0.f},                  // 6581 + crunch
        {"drive_type",2.f},{"drive_amount",0.40f},
        {"fx_delay_mix",0.20f},{"fx_reverb_mix",0.25f},
    };

    // ════════════════════════ STABS / CHORDS (3) ═══════════════════════════
    static const FP kFP_StabBigRoom[] = {
        {"osc1_wave",0.f},{"osc1_volume",0.90f},
        {"osc2_wave",0.f},{"osc2_fine",7.f},{"osc2_volume",0.75f},
        {"osc3_wave",0.f},{"osc3_fine",-5.f},{"osc3_volume",0.60f},
        {"filter_cutoff",2000.f},{"filter_res",0.65f},
        {"filter_env_amount",0.85f},{"filter_env_attack",0.001f},
        {"filter_env_decay",0.10f},{"filter_env_sustain",0.0f},
        {"amp_attack",0.001f},{"amp_decay",0.20f},{"amp_sustain",0.0f},{"amp_release",0.30f},
        {"unison_voices",2.f},{"unison_detune",10.f},{"unison_spread",0.50f},
        {"chip_model",1.f},
        {"fx_delay_time",3.f},{"fx_delay_feedback",0.50f},{"fx_delay_mix",0.35f},
        {"fx_reverb_size",0.80f},{"fx_reverb_mix",0.50f},
    };
    static const FP kFP_StabFiltered[] = {
        {"osc1_wave",2.f},{"osc1_pw",0.50f},{"osc1_volume",0.90f},
        {"osc2_wave",0.f},{"osc2_fine",-7.f},{"osc2_volume",0.70f},
        {"osc3_wave",1.f},{"osc3_fine",12.f},{"osc3_volume",0.55f},
        {"filter_cutoff",1200.f},{"filter_res",0.80f},
        {"filter_env_amount",0.95f},{"filter_env_attack",0.001f},
        {"filter_env_decay",0.12f},{"filter_env_sustain",0.0f},
        {"amp_attack",0.001f},{"amp_decay",0.20f},{"amp_sustain",0.0f},{"amp_release",0.25f},
        {"chip_model",0.f},                                         // 6581 stab character
        {"fx_delay_mix",0.25f},{"fx_reverb_mix",0.30f},
    };
    static const FP kFP_StabDetunedHit[] = {
        {"osc1_wave",0.f},{"osc1_volume",0.85f},
        {"osc2_wave",6.f},{"osc2_pw",0.60f},{"osc2_volume",0.70f},
        {"osc3_volume",0.f},
        {"filter_cutoff",2500.f},{"filter_res",0.50f},
        {"filter_env_amount",0.70f},{"filter_env_decay",0.15f},{"filter_env_sustain",0.0f},
        {"amp_attack",0.001f},{"amp_decay",0.20f},{"amp_sustain",0.0f},{"amp_release",0.30f},
        {"unison_voices",4.f},{"unison_detune",20.f},{"unison_spread",0.85f},
        {"chip_model",1.f},
        {"fx_reverb_size",0.65f},{"fx_reverb_mix",0.40f},
    };

    // ════════════════════ ATMOSPHERIC PADS / FX (3) ════════════════════════
    static const FP kFP_PadCosmic[] = {
        {"osc1_wave",4.f},{"osc1_volume",0.75f},
        {"osc2_wave",1.f},{"osc2_fine",12.f},{"osc2_volume",0.60f},
        {"osc3_wave",0.f},{"osc3_fine",-12.f},{"osc3_volume",0.50f},
        {"filter_cutoff",1300.f},{"filter_res",0.25f},
        {"filter_env_amount",0.30f},{"filter_env_attack",2.5f},
        {"amp_attack",1.50f},{"amp_sustain",0.90f},{"amp_release",3.5f},
        {"lfo1_on",1.f},{"lfo1_shape",0.f},{"lfo1_rate",0.15f},
        {"mod1_src",0.f},{"mod1_amt",0.20f},{"mod1_dst",0.f},        // LFO1 → cutoff
        {"unison_voices",3.f},{"unison_detune",16.f},{"unison_spread",0.80f},
        {"trance_drift",0.40f},{"chip_model",1.f},
        {"fx_chorus_mix",0.55f},
        {"fx_delay_time",4.f},{"fx_delay_feedback",0.55f},{"fx_delay_mix",0.20f},
        {"fx_reverb_size",0.90f},{"fx_reverb_mix",0.70f},
    };
    static const FP kFP_PadAtmosphere[] = {
        {"osc1_wave",1.f},{"osc1_volume",0.70f},
        {"osc2_wave",3.f},{"osc2_volume",0.18f},                    // noise wash
        {"osc3_wave",0.f},{"osc3_fine",-5.f},{"osc3_volume",0.55f},
        {"filter_cutoff",950.f},{"filter_res",0.40f},
        {"filter_env_amount",0.25f},{"filter_env_attack",2.0f},
        {"amp_attack",2.0f},{"amp_sustain",0.95f},{"amp_release",3.0f},
        {"lfo1_on",1.f},{"lfo1_shape",0.f},{"lfo1_rate",0.10f},
        {"mod1_src",0.f},{"mod1_amt",0.30f},{"mod1_dst",0.f},
        {"trance_drift",0.45f},{"chip_model",1.f},
        {"fx_chorus_mix",0.50f},
        {"fx_reverb_size",0.95f},{"fx_reverb_damp",0.30f},{"fx_reverb_mix",0.70f},
    };
    static const FP kFP_FxRiser[] = {
        {"osc1_wave",3.f},{"osc1_volume",0.60f},                    // noise base
        {"osc2_wave",0.f},{"osc2_semi",-12.f},{"osc2_volume",0.55f},
        {"osc3_volume",0.f},
        {"filter_cutoff",300.f},{"filter_res",0.65f},
        {"filter_env_amount",0.95f},{"filter_env_attack",4.0f},     // slow open = riser
        {"filter_env_decay",4.0f},
        {"amp_attack",2.0f},{"amp_sustain",0.95f},{"amp_release",0.50f},
        {"lfo1_on",1.f},{"lfo1_shape",2.f},{"lfo1_rate",4.0f},      // saw LFO
        {"mod1_src",0.f},{"mod1_amt",0.50f},{"mod1_dst",0.f},
        {"trance_drift",0.50f},{"chip_model",1.f},
        {"fx_chorus_mix",0.30f},
        {"fx_reverb_size",0.85f},{"fx_reverb_mix",0.60f},
    };

    struct FactoryPresetEntry { const char* name; const FP* data; int count; };
    static const FactoryPresetEntry kFactoryPresets[] = {
        // ── Plucks ──
        { "Pluck — Glass Sting",       kFP_PluckGlassSting,    (int)std::size(kFP_PluckGlassSting)    },
        { "Pluck — Crystal Bell",      kFP_PluckCrystalBell,   (int)std::size(kFP_PluckCrystalBell)   },
        { "Pluck — Hammer",            kFP_PluckHammer,        (int)std::size(kFP_PluckHammer)        },
        { "Pluck — Ice Bell",          kFP_PluckIceBell,       (int)std::size(kFP_PluckIceBell)       },
        { "Pluck — SID 6581",          kFP_PluckSID6581,       (int)std::size(kFP_PluckSID6581)       },
        { "Pluck — Sub Knock",         kFP_PluckSubKnock,      (int)std::size(kFP_PluckSubKnock)      },
        // ── Supersaw / detuned leads ──
        { "Lead — Hypersaw 7",         kFP_LeadHypersaw7,      (int)std::size(kFP_LeadHypersaw7)      },
        { "Lead — Stacked Saws",       kFP_LeadStackedSaws,    (int)std::size(kFP_LeadStackedSaws)    },
        { "Lead — Wide Detune",        kFP_LeadWideDetune,     (int)std::size(kFP_LeadWideDetune)     },
        { "Lead — Anthem",             kFP_LeadAnthem,         (int)std::size(kFP_LeadAnthem)         },
        { "Lead — Hoover",             kFP_LeadHoover,         (int)std::size(kFP_LeadHoover)         },
        // ── Rolling / offbeat basses ──
        { "Bass — Rolling Offbeat",    kFP_BassRollingOffbeat, (int)std::size(kFP_BassRollingOffbeat) },
        { "Bass — Reese",              kFP_BassReese,          (int)std::size(kFP_BassReese)          },
        { "Bass — Acid Squelch",       kFP_BassAcidSquelch,    (int)std::size(kFP_BassAcidSquelch)    },
        { "Bass — Sub Drone",          kFP_BassSubDrone,       (int)std::size(kFP_BassSubDrone)       },
        { "Bass — Tech Pulse",         kFP_BassTechPulse,      (int)std::size(kFP_BassTechPulse)      },
        // ── Gated pads ──
        { "Gated Pad — 1/8 Pump",      kFP_GatedPad8thPump,    (int)std::size(kFP_GatedPad8thPump)    },
        { "Gated Pad — 16th Stutter",  kFP_GatedPad16thStutter,(int)std::size(kFP_GatedPad16thStutter)},
        { "Gated Pad — Trance Choir",  kFP_GatedPadTranceChoir,(int)std::size(kFP_GatedPadTranceChoir)},
        { "Gated Pad — Soft Sweep",    kFP_GatedPadSoftSweep,  (int)std::size(kFP_GatedPadSoftSweep)  },
        // ── Arp / sequencer ──
        { "Arp — Classic Trance",      kFP_ArpClassicTrance,   (int)std::size(kFP_ArpClassicTrance)   },
        { "Arp — Plucky Pattern",      kFP_ArpPluckyPattern,   (int)std::size(kFP_ArpPluckyPattern)   },
        { "Arp — Octave Climber",      kFP_ArpOctaveClimber,   (int)std::size(kFP_ArpOctaveClimber)   },
        { "Arp — Random Glitch",       kFP_ArpRandomGlitch,    (int)std::size(kFP_ArpRandomGlitch)    },
        // ── Stabs / chords ──
        { "Stab — Big Room",           kFP_StabBigRoom,        (int)std::size(kFP_StabBigRoom)        },
        { "Stab — Filtered",           kFP_StabFiltered,       (int)std::size(kFP_StabFiltered)       },
        { "Stab — Detuned Hit",        kFP_StabDetunedHit,     (int)std::size(kFP_StabDetunedHit)     },
        // ── Atmospheric pads / FX ──
        { "Pad — Cosmic",              kFP_PadCosmic,          (int)std::size(kFP_PadCosmic)          },
        { "Pad — Atmosphere",          kFP_PadAtmosphere,      (int)std::size(kFP_PadAtmosphere)      },
        { "FX — Riser",                kFP_FxRiser,            (int)std::size(kFP_FxRiser)            },
    };
    static constexpr int kNumFactoryPresets = (int)std::size(kFactoryPresets);
    static_assert(kNumFactoryPresets == 30, "Expected exactly 30 factory presets");
} // namespace

// ============================================================
//  Preset system implementation
// ============================================================

void SIDTranceAudioProcessorEditor::initPresets()
{
    presets_.clear();

    // Add factory presets first
    for (int i = 0; i < kNumFactoryPresets; ++i) {
        PresetEntry e;
        e.name      = kFactoryPresets[i].name;
        e.isFactory = true;
        presets_.add(e);
    }

    // Scan user presets folder
    auto userDir = juce::File::getSpecialLocation(juce::File::userApplicationDataDirectory)
                       .getChildFile("SIDyssey").getChildFile("Presets");
    if (userDir.exists()) {
        juce::Array<juce::File> files;
        userDir.findChildFiles(files, juce::File::findFiles, false, "*.xml");
        files.sort();
        for (auto& f : files) {
            PresetEntry e;
            e.name      = f.getFileNameWithoutExtension();
            e.isFactory = false;
            e.userFile  = f;
            presets_.add(e);
        }
    }

    currentPresetIdx_ = -1;  // Start as Init
    presetDirty_      = false;
}

void SIDTranceAudioProcessorEditor::updatePresetBar()
{
    if (!mainView) return;

    juce::String name;
    if (currentPresetIdx_ >= 0 && currentPresetIdx_ < presets_.size())
        name = presets_[currentPresetIdx_].name;

    mainView->presetBar.setPresetName(name.toStdString());  // empty = "-- Init Patch --"
    mainView->presetBar.setDirty(presetDirty_);
}

void SIDTranceAudioProcessorEditor::applyFactoryPreset(int factoryIndex)
{
    if (factoryIndex < 0 || factoryIndex >= kNumFactoryPresets) return;

    // 1. Reset all parameters to defaults
    for (auto* param : audioProcessor.getParameters())
        param->setValueNotifyingHost(param->getDefaultValue());

    // 2. Apply this preset's overrides (denormalized → normalize via convertTo0to1)
    auto& apvts = audioProcessor.getAPVTS();
    const auto& fp = kFactoryPresets[factoryIndex];
    for (int i = 0; i < fp.count; ++i)
        if (auto* p = apvts.getParameter(fp.data[i].id))
            p->setValueNotifyingHost(p->convertTo0to1(fp.data[i].v));

    // 3. Signal UI thread to rebind widgets
    audioProcessor.stateJustLoaded = true;
}

void SIDTranceAudioProcessorEditor::loadPreset(int index)
{
    if (index < 0 || index >= presets_.size()) return;
    const auto& entry = presets_.getReference(index);

    if (entry.isFactory) {
        applyFactoryPreset(index);
    } else {
        // Load user preset from XML file
        if (auto xml = juce::XmlDocument::parse(entry.userFile)) {
            if (xml->hasTagName(audioProcessor.getAPVTS().state.getType())) {
                audioProcessor.getAPVTS().replaceState(juce::ValueTree::fromXml(*xml));
                audioProcessor.stateJustLoaded = true;
            }
        }
    }

    currentPresetIdx_ = index;
    presetDirty_      = false;
    updatePresetBar();
}

void SIDTranceAudioProcessorEditor::resetToInit()
{
    auto& apvts = audioProcessor.getAPVTS();

    // First: reset everything to its declared default
    for (auto* param : audioProcessor.getParameters())
        param->setValueNotifyingHost(param->getDefaultValue());

    // Then override the "on" toggles and modulation amounts so INIT is a
    // truly blank slate: nothing is modulating, nothing is making noise
    // except a single bare oscillator.
    auto setNorm = [&](const char* id, float denorm) {
        if (auto* p = apvts.getParameter(id))
            p->setValueNotifyingHost(p->convertTo0to1(denorm));
    };

    // Mod matrix — zero out all four amounts so the slots have no audible effect
    setNorm("mod1_amt", 0.0f);
    setNorm("mod2_amt", 0.0f);
    setNorm("mod3_amt", 0.0f);
    setNorm("mod4_amt", 0.0f);

    // Arp off (defaults to ON in createParameterLayout)
    setNorm("arp_on", 0.0f);

    // Both LFOs off
    setNorm("lfo1_on", 0.0f);
    setNorm("lfo2_on", 0.0f);

    // FX off
    setNorm("fx_chorus_on", 0.0f);
    setNorm("fx_delay_on",  0.0f);
    setNorm("fx_reverb_on", 0.0f);

    // Trance gate off (already default false, but make explicit)
    setNorm("gate_on", 0.0f);

    currentPresetIdx_ = -1;
    presetDirty_      = false;
    audioProcessor.stateJustLoaded = true;
    updatePresetBar();
}

void SIDTranceAudioProcessorEditor::prevPreset()
{
    if (presets_.isEmpty()) return;
    const int next = (currentPresetIdx_ <= 0) ? presets_.size() - 1
                                               : currentPresetIdx_ - 1;
    loadPreset(next);
}

void SIDTranceAudioProcessorEditor::nextPreset()
{
    if (presets_.isEmpty()) return;
    const int next = (currentPresetIdx_ < 0) ? 0
                                              : (currentPresetIdx_ + 1) % presets_.size();
    loadPreset(next);
}

void SIDTranceAudioProcessorEditor::savePreset()
{
    // Factory preset → must Save As (can't overwrite factory)
    if (currentPresetIdx_ < 0 || currentPresetIdx_ >= presets_.size() ||
        presets_[currentPresetIdx_].isFactory) {
        savePresetAs();
        return;
    }

    const auto& entry = presets_.getReference(currentPresetIdx_);
    auto state = audioProcessor.getAPVTS().copyState();
    if (auto xml = state.createXml()) {
        xml->setAttribute("presetName", entry.name);
        entry.userFile.replaceWithText(xml->toString());
    }
    presetDirty_ = false;
    updatePresetBar();
}

void SIDTranceAudioProcessorEditor::savePresetAs()
{
    const juce::String defaultName =
        (currentPresetIdx_ >= 0 && currentPresetIdx_ < presets_.size())
            ? presets_[currentPresetIdx_].name
            : "New Preset";

    auto* dialog = new juce::AlertWindow("Save Preset As", "", juce::MessageBoxIconType::NoIcon);
    dialog->addTextEditor("name", defaultName, "Preset name:");
    dialog->addButton("Save",   1, juce::KeyPress(juce::KeyPress::returnKey));
    dialog->addButton("Cancel", 0, juce::KeyPress(juce::KeyPress::escapeKey));

    // SafePointer becomes null if the editor is deleted while the dialog is open
    // (e.g., host closes the plugin window during the modal). Raw `this` would be
    // a use-after-free if savePresetWithName() fires after the editor is gone.
    juce::Component::SafePointer<SIDTranceAudioProcessorEditor> safeThis(this);
    dialog->enterModalState(true,
        juce::ModalCallbackFunction::create([safeThis, dialog](int result) {
            if (safeThis == nullptr) return;   // editor was destroyed — abort
            if (result == 1) {
                auto name = dialog->getTextEditorContents("name").trim();
                if (name.isEmpty()) name = "New Preset";
                safeThis->savePresetWithName(name);
            }
        }), true /* deleteWhenDismissed */);
}

void SIDTranceAudioProcessorEditor::savePresetWithName(const juce::String& name)
{
    // Ensure the user presets directory exists
    auto userDir = juce::File::getSpecialLocation(juce::File::userApplicationDataDirectory)
                       .getChildFile("SIDyssey").getChildFile("Presets");
    userDir.createDirectory();

    // Sanitize filename
    const juce::String safeName = name.replaceCharacters("\\/:*?\"<>|", "_________");
    auto file = userDir.getChildFile(safeName + ".xml");

    // Write the current APVTS state to disk
    auto state = audioProcessor.getAPVTS().copyState();
    if (auto xml = state.createXml()) {
        xml->setAttribute("presetName", name);
        file.replaceWithText(xml->toString());
    }

    // Update or append in our preset list (user section only)
    int existingIdx = -1;
    for (int i = kNumFactoryPresets; i < presets_.size(); ++i)
        if (presets_[i].name == name) { existingIdx = i; break; }

    if (existingIdx >= 0) {
        presets_.getReference(existingIdx).userFile = file;
        currentPresetIdx_ = existingIdx;
    } else {
        PresetEntry e;
        e.name      = name;
        e.isFactory = false;
        e.userFile  = file;
        presets_.add(e);
        currentPresetIdx_ = presets_.size() - 1;
    }

    presetDirty_ = false;
    updatePresetBar();
}

void SIDTranceAudioProcessorEditor::onResize(int w, int h)
{
    if (mainView) {
        mainView->setBounds(0, 0, w, h);
        mainView->redraw();
    }
}
