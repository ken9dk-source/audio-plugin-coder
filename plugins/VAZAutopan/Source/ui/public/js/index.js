// VAZAutopan WebView bridge — binds sliders / toggles / combos to JUCE parameters.
// NOTE: the SHIPPED UI is the self-contained index.html (this JUCE interop + these bindings are
// inlined there). This module mirrors the inlined bindings as the readable source of truth.
import * as Juce from "./juce/index.js";

function bindSlider(paramId, el) {
  let state;
  try { state = Juce.getSliderState(paramId); } catch (e) { state = null; }
  if (!state) return;

  const thumb = el.querySelector(".thumb");
  const updateThumb = () => {
    const v = state.getNormalisedValue();
    if (thumb) thumb.style.left = (v * 92) + "%";
  };
  if (state.valueChangedEvent && state.valueChangedEvent.addListener)
    state.valueChangedEvent.addListener(updateThumb);
  updateThumb();

  let dragging = false;
  const setFromX = (clientX) => {
    const r = el.getBoundingClientRect();
    let p = (clientX - r.left) / r.width;
    p = Math.max(0, Math.min(1, p));
    state.setNormalisedValue(p);
    if (thumb) thumb.style.left = (p * 92) + "%";
  };
  el.addEventListener("mousedown", (e) => { dragging = true; setFromX(e.clientX); });
  window.addEventListener("mousemove", (e) => { if (dragging) setFromX(e.clientX); });
  window.addEventListener("mouseup", () => { dragging = false; });
}

function bindToggle(paramId, el) {
  let st; try { st = Juce.getToggleState(paramId); } catch (e) { st = null; }
  if (!st) return;
  const upd = () => { el.classList.toggle("on", st.getValue()); };
  if (st.valueChangedEvent && st.valueChangedEvent.addListener) st.valueChangedEvent.addListener(upd);
  upd();
  el.addEventListener("click", () => st.setValue(!st.getValue()));
}

function bindCombo(paramId, el) {
  let state; try { state = Juce.getComboBoxState(paramId); } catch (e) { state = null; }
  if (!state) return;
  const update = () => { el.selectedIndex = state.getChoiceIndex(); };
  if (state.valueChangedEvent && state.valueChangedEvent.addListener) state.valueChangedEvent.addListener(update);
  update();
  el.addEventListener("change", () => state.setChoiceIndex(el.selectedIndex));
}

function init() {
  document.querySelectorAll("[data-param]").forEach((el) => {
    bindSlider(el.getAttribute("data-param"), el);
  });
  document.querySelectorAll("[data-toggle]").forEach((el) => {
    bindToggle(el.getAttribute("data-toggle"), el);
  });
  document.querySelectorAll("[data-combo]").forEach((el) => {
    bindCombo(el.getAttribute("data-combo"), el);
  });
}

if (document.readyState === "loading")
  document.addEventListener("DOMContentLoaded", init);
else
  init();
