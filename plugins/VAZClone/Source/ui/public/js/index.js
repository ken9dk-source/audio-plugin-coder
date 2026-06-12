// VAZClone WebView bridge — connects the VAZ panel controls to JUCE parameters.
// Each control with [data-param="<id>"] binds to the matching APVTS parameter
// via the JUCE WebSliderRelay. IDs match Source/ParameterIDs.hpp.
import * as Juce from "./juce/index.js";

function bindSlider(paramId, el) {
  let state;
  try { state = Juce.getSliderState(paramId); } catch (e) { state = null; }
  if (!state) return; // running in a plain browser (no JUCE backend) — preview only

  const thumb = el.querySelector(".thumb");
  const updateThumb = () => {
    const v = state.getNormalisedValue();
    if (thumb) thumb.style.left = (v * 92) + "%";
  };
  if (state.valueChangedEvent && state.valueChangedEvent.addListener)
    state.valueChangedEvent.addListener(updateThumb);
  updateThumb();

  // tell the host (Ableton) which parameter this control maps to
  const setIdx = () => {
    const i = state.properties ? state.properties.parameterIndex : -1;
    if (i != null && i >= 0) el.setAttribute("controlparameterindex", i);
  };
  if (state.propertiesChangedEvent && state.propertiesChangedEvent.addListener)
    state.propertiesChangedEvent.addListener(setIdx);
  setIdx();

  // RELATIVE dragging: clicking GRABS the current value (no jump to the click position) and
  // the value changes by the drag DISTANCE. (Absolute "click-to-jump" made a click move the fader.)
  let dragging = false, startX = 0, startVal = 0;
  const applyDelta = (clientX) => {
    const r = el.getBoundingClientRect();
    let p = startVal + (clientX - startX) / r.width;   // delta, not absolute position
    p = Math.max(0, Math.min(1, p));
    state.setNormalisedValue(p);
    if (thumb) thumb.style.left = (p * 92) + "%";
  };
  el.addEventListener("pointerdown", (e) => {
    if (e.metaKey || e.ctrlKey) {                 // Cmd (Mac) / Ctrl (Win) → reset to default
      try { Juce.getNativeFunction("resetParam")(paramId); } catch (err) {}
      e.preventDefault();
      return;
    }
    dragging = true;
    el.classList.remove("center");
    try { el.setPointerCapture(e.pointerId); } catch (err) {}   // keep tracking even if the pointer leaves the slider (Mac fix)
    startX = e.clientX;
    startVal = state.getNormalisedValue();        // grab current value — NO jump on click
    e.preventDefault();
  });
  el.addEventListener("pointermove", (e) => { if (dragging) applyDelta(e.clientX); });
  el.addEventListener("pointerup",   (e) => { dragging = false; try { el.releasePointerCapture(e.pointerId); } catch (err) {} });
  el.addEventListener("pointercancel", () => { dragging = false; });
}

// <select> bound to a WebComboBoxRelay (correct API for choice params).
function bindCombo(paramId, el) {
  let state;
  try { state = Juce.getComboBoxState(paramId); } catch (e) { state = null; }
  if (!state) return;
  const update = () => { el.selectedIndex = state.getChoiceIndex(); };
  if (state.valueChangedEvent && state.valueChangedEvent.addListener)
    state.valueChangedEvent.addListener(update);
  update();
  el.addEventListener("change", () => state.setChoiceIndex(el.selectedIndex));
}

// Button group (waveform / octave) bound to a WebComboBoxRelay: button position = choice index.
function bindButtonGroup(selector, paramId) {
  let state;
  try { state = Juce.getComboBoxState(paramId); } catch (e) { state = null; }
  if (!state) return;
  const btns = Array.from(document.querySelectorAll(selector));
  const update = () => {
    const idx = state.getChoiceIndex();
    btns.forEach((b, i) => b.classList.toggle("on", i === idx));
  };
  if (state.valueChangedEvent && state.valueChangedEvent.addListener)
    state.valueChangedEvent.addListener(update);
  update();
  btns.forEach((b, i) => b.addEventListener("click", () => state.setChoiceIndex(i)));
}

// Radio-button group bound to a WebComboBoxRelay choice (e.g. Mono/Poly/Unison).
function bindRadioGroup(selector, paramId) {
  let state;
  try { state = Juce.getComboBoxState(paramId); } catch (e) { state = null; }
  if (!state) return;
  const radios = Array.from(document.querySelectorAll(selector));
  const update = () => {
    const idx = state.getChoiceIndex();
    radios.forEach((r, i) => { r.checked = (i === idx); });
  };
  if (state.valueChangedEvent && state.valueChangedEvent.addListener)
    state.valueChangedEvent.addListener(update);
  update();
  radios.forEach((r, i) => r.addEventListener("change", () => state.setChoiceIndex(i)));
}

// The 22 modulation sources — MUST match the order of the modSrcs[] array in the processor.
const MODSOURCES = ["None","LFO 1","LFO 2","LFO 3","Envelope 1","Envelope 2","Mod Amplifier 1",
  "Mod Amplifier 2","Lag Processor","Oscillator 1","Oscillator 1 Pitch","Oscillator 2","Noise",
  "External Input","Accent","Sequencer A","Sequencer B","MIDI Velocity","MIDI Pressure",
  "MIDI Control A","MIDI Control B","Voice Number"];

function populateModSources() {
  document.querySelectorAll("select[data-modsrc]").forEach((sel) => {
    if (sel.options.length < MODSOURCES.length)
      sel.innerHTML = MODSOURCES.map((s) => "<option>" + s + "</option>").join("");
  });
}

// Toggle button bound to a WebToggleButtonRelay bool param (ENV Reset/Cycle/Curve).
function bindToggle(paramId, el) {
  let state;
  try { state = Juce.getToggleState(paramId); } catch (e) { state = null; }
  if (!state) return;
  const update = () => el.classList.toggle("on", state.getValue());
  if (state.valueChangedEvent && state.valueChangedEvent.addListener)
    state.valueChangedEvent.addListener(update);
  update();
  el.addEventListener("click", () => state.setValue(!state.getValue()));
}

// +/- polarity buttons in each mod-row -> the matching *_amt_inv toggle (+ = positive, - = inverted).
// (The buttons were static HTML with no binding; the backend relays already exist.)
const SIGN_MAP = {
  o1_fm_src: "o1_fm_amt_inv", o1_ws_src: "o1_ws_amt_inv",
  o2_fm_src: "o2_fm_amt_inv", o2_ws_src: "o2_ws_amt_inv",
  e2_mod_src: "e2_mod_amt_inv",
  cut_mod1_src: "filt_env_amt_inv", cut_mod2_src: "lfo_amt_inv",
  res_mod_src: "res_mod_amt_inv", amp_mod_src: "amp_mod_amt_inv",
  pan_mod_src: "pan_mod_amt_inv", ma1_am_src: "ma1_am_amt_inv", ma2_am_src: "ma2_am_amt_inv",
};
function bindSigns() {
  document.querySelectorAll(".mod-row").forEach((row) => {
    const sel = row.querySelector("select[data-combo]");
    const inv = sel && SIGN_MAP[sel.getAttribute("data-combo")];
    if (!inv) return;
    let st; try { st = Juce.getToggleState(inv); } catch (e) { st = null; }
    if (!st) return;
    const plus  = row.querySelector(".pm-btn:not(.minus)");
    const minus = row.querySelector(".pm-btn.minus");
    const upd = () => { const on = st.getValue();      // true = inverted (-)
      if (plus)  plus.classList.toggle("on", !on);
      if (minus) minus.classList.toggle("on", on); };
    if (st.valueChangedEvent && st.valueChangedEvent.addListener) st.valueChangedEvent.addListener(upd);
    upd();
    if (plus)  plus.addEventListener("click", () => st.setValue(false));
    if (minus) minus.addEventListener("click", () => st.setValue(true));
  });
}

// Bottom-right resize grip. The WebView2 native window covers JUCE's corner resizer, so we draw
// our own grip and drive the editor size through native functions. screenX is screen-relative (so
// it's stable while the panel rescales mid-drag); editor px ≈ logical CSS px on the Chromium WebView.
function setupResize() {
  let getSize, setSize;
  try { getSize = Juce.getNativeFunction("getEditorSize"); setSize = Juce.getNativeFunction("setEditorSize"); }
  catch (e) { return; }                               // plain browser preview — no backend
  const grip = document.createElement("div");
  grip.title = "Drag to resize";
  grip.style.cssText = "position:fixed;right:1px;bottom:1px;width:16px;height:16px;cursor:nwse-resize;" +
    "z-index:99999;background:repeating-linear-gradient(135deg,transparent 0,transparent 2px," +
    "rgba(255,255,255,0.4) 2px,rgba(255,255,255,0.4) 3px);";
  document.body.appendChild(grip);
  let dragging = false, startX = 0, startW = 0;
  grip.addEventListener("pointerdown", async (e) => {
    dragging = true;
    try { grip.setPointerCapture(e.pointerId); } catch (err) {}
    startX = e.screenX;
    try { const s = await getSize(); startW = s[0]; } catch (err) { startW = window.innerWidth; }
    e.preventDefault();
  });
  grip.addEventListener("pointermove", (e) => {
    if (!dragging) return;
    try { setSize(Math.round(startW + (e.screenX - startX))); } catch (err) {}
  });
  const end = (e) => { dragging = false; try { grip.releasePointerCapture(e.pointerId); } catch (err) {} };
  grip.addEventListener("pointerup", end);
  grip.addEventListener("pointercancel", end);
}

function init() {
  populateModSources();                               // fill mod-source dropdowns before binding
  document.querySelectorAll("[data-toggle]").forEach((el) => {
    bindToggle(el.getAttribute("data-toggle"), el);
  });
  document.querySelectorAll("[data-param]").forEach((el) => {
    bindSlider(el.getAttribute("data-param"), el);
  });
  document.querySelectorAll("[data-combo]").forEach((el) => {
    bindCombo(el.getAttribute("data-combo"), el);
  });
  bindSigns();                                      // +/- modulation-polarity buttons
  bindButtonGroup('[data-wg="wf1"]', "o1_wave");    // waveform buttons OSC1
  bindButtonGroup('[data-wg="wf2"]', "o2_wave");    // waveform buttons OSC2
  bindButtonGroup('[data-og="o1t"]', "o1_octave");  // octave buttons OSC1
  bindButtonGroup('[data-og="o2t"]', "o2_octave");  // octave buttons OSC2
  bindRadioGroup('input[data-vm]', "voice_mode");   // Mono/Poly/Unison radios

  // host control→param mapping (Ableton "Configure" / automation): report the control under the mouse
  try {
    const idxUpdater = new Juce.ControlParameterIndexUpdater("controlparameterindex");
    window.addEventListener("mousemove", (e) => idxUpdater.handleMouseMove(e));
  } catch (e) {}

  // resizable GUI: scale the synth panel to fill the (resizable) window
  const fitWindow = () => {
    const win = document.querySelector(".vaz-win");
    if (win) win.style.zoom = Math.max(0.5, window.innerWidth / 604);
  };
  window.addEventListener("resize", fitWindow);
  fitWindow();

  setupResize();                                      // bottom-right drag-to-resize grip
}

// Modules load async via the resource provider — DOMContentLoaded may already have fired
// by the time we run, so init immediately if the DOM is already parsed.
if (document.readyState === "loading")
  document.addEventListener("DOMContentLoaded", init);
else
  init();
