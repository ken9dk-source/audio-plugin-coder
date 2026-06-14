// VAZEqualizer WebView bridge — binds [data-param] sliders to JUCE params, with value readouts.
// NOTE: the SHIPPED UI is the self-contained index.html (this JUCE interop + these bindings are
// inlined there). This module mirrors the inlined bindings as the readable source of truth.
// Mappings MUST mirror the processor: gain ±18 dB, freq 20 Hz..20 kHz (log), mid Q 0.3..10 (log),
// Low/High Q morph shelf (≤0.8) → high-pass / low-pass.
import * as Juce from "./juce/index.js";

function fmtVal(fmt, n) {
  if (fmt === "gain") { const d = (n - 0.5) * 36; return (d >= 0 ? "+" : "") + d.toFixed(1) + " dB"; }
  if (fmt === "freq") { const hz = 20 * Math.pow(1000, n); return hz >= 1000 ? (hz / 1000).toFixed(2) + " kHz" : Math.round(hz) + " Hz"; }
  if (fmt === "qmid") { return "Q " + (0.3 * Math.pow(33.333, n)).toFixed(2); }
  if (fmt === "qmorphLo") { return n <= 0.8 ? "Shelf" : "HiPass"; }
  if (fmt === "qmorphHi") { return n <= 0.8 ? "Shelf" : "LoPass"; }
  return n.toFixed(2);
}

function bindSlider(paramId, el) {
  let state;
  try { state = Juce.getSliderState(paramId); } catch (e) { state = null; }
  if (!state) return;

  const thumb = el.querySelector(".thumb");
  const fmt = el.getAttribute("data-fmt");
  const valEl = document.querySelector('[data-val="' + paramId + '"]');
  const update = () => {
    const v = state.getNormalisedValue();
    if (thumb) thumb.style.left = (v * 92) + "%";
    if (valEl) valEl.textContent = fmtVal(fmt, v);
  };
  if (state.valueChangedEvent && state.valueChangedEvent.addListener)
    state.valueChangedEvent.addListener(update);
  update();

  let dragging = false;
  const setFromX = (clientX) => {
    const r = el.getBoundingClientRect();
    let p = (clientX - r.left) / r.width;
    p = Math.max(0, Math.min(1, p));
    state.setNormalisedValue(p);
    if (thumb) thumb.style.left = (p * 92) + "%";
    if (valEl) valEl.textContent = fmtVal(fmt, p);
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
