// VAZCompressor WebView bridge — sliders + value readouts + a polled gain-reduction meter / clip LED.
// NOTE: the SHIPPED UI is the self-contained index.html (this JUCE interop + these bindings are
// inlined there). This module mirrors the inlined bindings as the readable source of truth.
// Mappings MUST mirror the processor: thr -60..+6 dB, ratio = 1/(1-slope), attack 0.1..120 ms (log),
// release 5 ms..6 s (log), makeup 0..+24 dB.
import * as Juce from "./juce/index.js";

function fmtVal(fmt, n) {
  if (fmt === "thr")   { const d = -60 + n * 66; return (d >= 0 ? "+" : "") + d.toFixed(1) + " dB"; }
  if (fmt === "ratio") { if (n >= 0.995) return "∞:1"; const r = 1 / (1 - n); return r.toFixed(r < 10 ? 1 : 0) + ":1"; }
  if (fmt === "atk")   { const ms = 0.1 * Math.pow(1200, n); return (ms < 10 ? ms.toFixed(2) : ms.toFixed(0)) + " ms"; }
  if (fmt === "rel")   { const ms = 5 * Math.pow(1200, n); return ms >= 1000 ? (ms / 1000).toFixed(2) + " s" : ms.toFixed(0) + " ms"; }
  if (fmt === "gain")  { const d = n * 24; return "+" + d.toFixed(1) + " dB"; }
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

// Poll the gain-reduction meter + clip flag (~16 fps). Backend getMeter() → [grDb, clipped].
function startMeter() {
  const fill = document.getElementById("grfill");
  const led = document.getElementById("clipLed");
  let getMeter = null;
  try { getMeter = Juce.getNativeFunction("getMeter"); } catch (e) { return; }
  if (!getMeter) return;
  let shown = 0;
  let clipUntil = 0;
  const MAXGR = 24;
  const tick = () => {
    getMeter().then((m) => {
      const gr = Array.isArray(m) ? (+m[0] || 0) : 0;
      const clipped = Array.isArray(m) ? !!m[1] : false;
      shown = Math.max(gr, shown - 1.5);
      if (fill) fill.style.width = Math.min(100, (shown / MAXGR) * 100) + "%";
      if (clipped) clipUntil = Date.now() + 400;
      if (led) led.classList.toggle("on", Date.now() < clipUntil);
    }).catch(() => {});
  };
  setInterval(tick, 60);
}

function init() {
  document.querySelectorAll("[data-param]").forEach((el) => {
    bindSlider(el.getAttribute("data-param"), el);
  });
  startMeter();
}

if (document.readyState === "loading")
  document.addEventListener("DOMContentLoaded", init);
else
  init();
