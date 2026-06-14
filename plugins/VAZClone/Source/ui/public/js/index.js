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
  o1_fm2_src: "o1_fm2_amt_inv", o2_fm2_src: "o2_fm2_amt_inv",
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

// Number-input box (UniVce / Bend) bound to a continuous param: shown value = round(lo + span·norm).
// Type a value + Enter, or scroll the wheel over the box to nudge it (like VAZ's value picker).
function bindNumber(el, paramId, lo, hi) {
  if (!el) return;
  let state; try { state = Juce.getSliderState(paramId); } catch (e) { state = null; }
  if (!state) return;
  const span = hi - lo;
  const fromParam = () => { el.value = Math.round(lo + span * state.getNormalisedValue()); };
  if (state.valueChangedEvent && state.valueChangedEvent.addListener) state.valueChangedEvent.addListener(fromParam);
  fromParam();
  const setN = (n) => { n = Math.max(lo, Math.min(hi, n)); state.setNormalisedValue((n - lo) / span); el.value = n; };
  el.addEventListener("change", () => { const n = parseInt(el.value, 10); if (isNaN(n)) fromParam(); else setN(n); });
  el.addEventListener("wheel", (e) => { e.preventDefault(); setN((parseInt(el.value, 10) || lo) + (e.deltaY < 0 ? 1 : -1)); }, { passive: false });
}

// On-screen pitch-bend wheel: absolute drag (thumb tracks the mouse) that SPRINGS BACK to centre (0.5)
// the moment you let go — like a real wheel. The DSP turns 0.5±x into ±Bend-Range semitones.
function bindPitchWheel(el, paramId) {
  if (!el) return;
  let state; try { state = Juce.getSliderState(paramId); } catch (e) { state = null; }
  if (!state) return;
  const thumb = el.querySelector(".thumb");
  const place = (v) => { if (thumb) thumb.style.left = (v * 92) + "%"; };
  const fromParam = () => place(state.getNormalisedValue());
  if (state.valueChangedEvent && state.valueChangedEvent.addListener) state.valueChangedEvent.addListener(fromParam);
  fromParam();
  const setIdx = () => { const i = state.properties ? state.properties.parameterIndex : -1; if (i != null && i >= 0) el.setAttribute("controlparameterindex", i); };
  if (state.propertiesChangedEvent && state.propertiesChangedEvent.addListener) state.propertiesChangedEvent.addListener(setIdx);
  setIdx();
  let dragging = false;
  const apply = (clientX) => {
    const r = el.getBoundingClientRect();
    const v = Math.max(0, Math.min(1, (clientX - r.left) / r.width));   // absolute: thumb = mouse position
    state.setNormalisedValue(v); place(v);
  };
  el.addEventListener("pointerdown", (e) => { dragging = true; el.classList.remove("center"); try { el.setPointerCapture(e.pointerId); } catch (err) {} apply(e.clientX); e.preventDefault(); });
  el.addEventListener("pointermove", (e) => { if (dragging) apply(e.clientX); });
  const release = (e) => { if (!dragging) return; dragging = false; try { el.releasePointerCapture(e.pointerId); } catch (err) {} state.setNormalisedValue(0.5); place(0.5); };   // spring back to centre
  el.addEventListener("pointerup", release);
  el.addEventListener("pointercancel", release);
}

// VAZ-style value picker: click a value → a grid popup (numbers in a 4-col grid + a wide "Dynamic" row).
function showPicker(anchor, labels, curIdx, onPick) {
  document.querySelectorAll(".vpick-pop").forEach((e) => e.remove());
  const pop = document.createElement("div"); pop.className = "vpick-pop";
  const grid = document.createElement("div"); grid.className = "vpick-grid";
  labels.forEach((lab, i) => {
    if (lab === "Dynamic") return;
    const c = document.createElement("div");
    c.className = "vpick-cell" + (i === curIdx ? " on" : "");
    c.textContent = lab;
    c.addEventListener("click", (e) => { e.stopPropagation(); onPick(i); pop.remove(); });
    grid.appendChild(c);
  });
  pop.appendChild(grid);
  const di = labels.indexOf("Dynamic");
  if (di >= 0) {
    const d = document.createElement("div");
    d.className = "vpick-dyn" + (di === curIdx ? " on" : "");
    d.textContent = "Dynamic";
    d.addEventListener("click", (e) => { e.stopPropagation(); onPick(di); pop.remove(); });
    pop.appendChild(d);
  }
  document.body.appendChild(pop);
  const r = anchor.getBoundingClientRect();
  pop.style.left = Math.max(2, Math.min(r.left, window.innerWidth - pop.offsetWidth - 4)) + "px";
  pop.style.top = Math.min(r.bottom + 2, window.innerHeight - pop.offsetHeight - 4) + "px";
  setTimeout(() => document.addEventListener("pointerdown", function cl(ev) {
    if (!pop.contains(ev.target)) { pop.remove(); document.removeEventListener("pointerdown", cl); }
  }), 0);
}
// Choice param (e.g. Voices: "Dynamic"+1..32) shown as a click-to-pick value.
function bindChoicePicker(el, paramId, labels) {
  if (!el) return;
  let st; try { st = Juce.getComboBoxState(paramId); } catch (e) { st = null; }
  if (!st) return;
  const disp = () => { el.textContent = (labels[st.getChoiceIndex()] === "Dynamic") ? "Dyn" : labels[st.getChoiceIndex()]; };
  if (st.valueChangedEvent && st.valueChangedEvent.addListener) st.valueChangedEvent.addListener(disp);
  disp();
  el.addEventListener("click", () => showPicker(el, labels, st.getChoiceIndex(), (i) => { st.setChoiceIndex(i); disp(); }));
}
// Continuous param (e.g. UniVce: uni_voices 0..1) shown as a click-to-pick integer lo..hi.
function bindFloatPicker(el, paramId, lo, hi) {
  if (!el) return;
  let st; try { st = Juce.getSliderState(paramId); } catch (e) { st = null; }
  if (!st) return;
  const span = hi - lo;
  const labels = Array.from({ length: span + 1 }, (_, i) => String(lo + i));
  const cur = () => Math.round(lo + span * st.getNormalisedValue());
  const disp = () => { el.textContent = String(cur()); };
  if (st.valueChangedEvent && st.valueChangedEvent.addListener) st.valueChangedEvent.addListener(disp);
  disp();
  el.addEventListener("click", () => showPicker(el, labels, cur() - lo, (i) => { st.setNormalisedValue(i / span); disp(); }));
}

// One detune fader: show "Poly Detune" in Mono/Poly mode, "Unison Detune" in Unison mode (two params, one visible).
function bindDetuneRows() {
  const pr = document.getElementById("polyDetRow"), ur = document.getElementById("uniDetRow");
  if (!pr || !ur) return;
  let vm; try { vm = Juce.getComboBoxState("voice_mode"); } catch (e) { vm = null; }
  const upd = () => {
    const r = document.querySelectorAll('input[data-vm]');
    let i = Array.from(r).findIndex((x) => x.checked);   // radios are current (user clicks + bindRadioGroup on preset load)
    if (i < 0 && vm) i = vm.getChoiceIndex();
    const uni = i === 2;
    pr.style.display = uni ? "none" : "";
    ur.style.display = uni ? "" : "none";
  };
  if (vm && vm.valueChangedEvent && vm.valueChangedEvent.addListener) vm.valueChangedEvent.addListener(upd);
  document.querySelectorAll('input[data-vm]').forEach((r) => r.addEventListener("change", upd));
  upd();
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
  bindFloatPicker(document.getElementById("univPick"), "uni_voices", 1, 32);   // UniVce 1..32 → grid picker
  bindChoicePicker(document.getElementById("voicesPick"), "voices",
    ["Dynamic"].concat(Array.from({ length: 32 }, (_, i) => String(i + 1))));  // Voices: Dynamic + 1..32
  bindNumber(document.getElementById("bendInput"), "bend_range", 1, 24);  // Pitch-bend range 1..24 st
  bindPitchWheel(document.getElementById("pitchWheel"), "pitch_bend");    // on-screen pitch-bend wheel (springs to centre)
  bindDetuneRows();                                  // one detune fader, swapped Poly/Unison by mode

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
