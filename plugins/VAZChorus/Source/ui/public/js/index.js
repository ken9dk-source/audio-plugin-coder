// VAZReverb WebView bridge — binds [data-param] sliders to JUCE parameters.
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

function init() {
  document.querySelectorAll("[data-param]").forEach((el) => {
    bindSlider(el.getAttribute("data-param"), el);
  });
}

if (document.readyState === "loading")
  document.addEventListener("DOMContentLoaded", init);
else
  init();
