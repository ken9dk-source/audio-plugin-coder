// VAZDelay WebView bridge — sliders / toggles / combos + a sync-aware Delay readout.
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
  el.addEventListener("pointerdown", (e) => {
    if (e.metaKey || e.ctrlKey) { try { Juce.getNativeFunction("resetParam")(paramId); } catch (err) {} e.preventDefault(); return; }
    dragging = true;
    try { el.setPointerCapture(e.pointerId); } catch (err) {}
    setFromX(e.clientX); e.preventDefault();
  });
  el.addEventListener("pointermove", (e) => { if (dragging) setFromX(e.clientX); });
  el.addEventListener("pointerup", (e) => { dragging = false; try { el.releasePointerCapture(e.pointerId); } catch (err) {} });
  el.addEventListener("pointercancel", () => { dragging = false; });
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
  let st; try { st = Juce.getComboBoxState(paramId); } catch (e) { st = null; }
  if (!st) return;
  const upd = () => { el.selectedIndex = st.getChoiceIndex(); };
  if (st.valueChangedEvent && st.valueChangedEvent.addListener) st.valueChangedEvent.addListener(upd);
  upd();
  el.addEventListener("change", () => st.setChoiceIndex(el.selectedIndex));
}

// Delay readout: Sync → musical multiplier as % (MUST mirror the processor's syncMult bands);
// Free → time (5 ms..6 s, log). Updates on drag + Sync toggle.
function syncMultJS(p) {
  return p < 0.15 ? 0.5 : p < 0.32 ? 2 / 3 : p < 0.45 ? 0.75 : p < 0.74 ? 1.0 : p < 0.84 ? 4 / 3 : p < 0.93 ? 1.5 : 2.0;
}
function fmtDelay(p, isSync) {
  if (isSync) return Math.round(syncMultJS(p) * 100) + "%";
  const ms = 5 * Math.pow(1200, p);
  return ms >= 1000 ? (ms / 1000).toFixed(2) + " s" : Math.round(ms) + " ms";
}
function bindDelayReadout(paramId) {
  const valEl = document.querySelector('[data-dval="' + paramId + '"]');
  if (!valEl) return;
  let sld; try { sld = Juce.getSliderState(paramId); } catch (e) { sld = null; }
  let syncSt; try { syncSt = Juce.getToggleState("sync"); } catch (e) { syncSt = null; }
  if (!sld) return;
  const render = () => { valEl.textContent = fmtDelay(sld.getNormalisedValue(), syncSt ? syncSt.getValue() : false); };
  if (sld.valueChangedEvent && sld.valueChangedEvent.addListener) sld.valueChangedEvent.addListener(render);
  if (syncSt && syncSt.valueChangedEvent && syncSt.valueChangedEvent.addListener) syncSt.valueChangedEvent.addListener(render);
  const slEl = document.querySelector('[data-param="' + paramId + '"]');
  if (slEl) slEl.addEventListener("pointermove", render);
  render();
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
  bindDelayReadout("delay_l");
  bindDelayReadout("delay_r");
}

if (document.readyState === "loading")
  document.addEventListener("DOMContentLoaded", init);
else
  init();
