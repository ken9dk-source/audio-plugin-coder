// lfo_rate_isolation.test.js — reproduces the reported bug: "dragging LFO1 Rate flips the Pulsewidth
// Modulation dropdown (o1_ws_src) to Voice Number (index 21)". Drives the REAL bindSlider + bindCombo
// (extracted verbatim from index.html) through a faithful, name-routed relay backend, sweeps lfo_rate
// 0→1, and asserts o1_ws_src.selectedIndex is UNCHANGED the whole time.
//
// The backend model routes every event strictly by control name (a WebSliderRelay for "lfo_rate" and a
// WebComboBoxRelay for "o1_ws_src" are independent channels — exactly the JUCE contract). So if the test
// ever FAILS, it means the JS binding itself cross-wires the two. It PASSES → the binding is clean and the
// bug is not in this layer.  Run: node lfo_rate_isolation.test.js

const fs = require("fs");
const html = fs.readFileSync("C:/APC/y/plugins/VAZClone/Source/ui/public/index.html", "utf8");
const grab = (re, name) => { const m = html.match(re); if (!m) throw new Error("could not extract " + name); return m[0]; };

const bindSlider_src = grab(/function bindSlider\(paramId, el\)\s*\{[\s\S]*?\n\}/, "bindSlider");
const bindCombo_src  = grab(/function bindCombo\(paramId, el\)\s*\{[\s\S]*?\n\}/, "bindCombo");
// pin the choice-index formula so a drift here fails the test
if (!/Math\.round\(this\.value \* \(this\.properties\.choices\.length - 1\)\)/.test(grab(/getChoiceIndex\(\)\s*\{\s*return[^;]+;/, "getChoiceIndex")))
  throw new Error("getChoiceIndex formula drifted");

// Faithful per-name relay states (independent channels)
class Slider {
  constructor() { this._v = 0; this.properties = { parameterIndex: 0 }; this.valueChangedEvent = mkEv(); this.propertiesChangedEvent = mkEv(); }
  getNormalisedValue() { return this._v; }
  setNormalisedValue(v) { this._v = v; this.valueChangedEvent.fire(); }   // backend round-trip = same channel only
}
class Combo {
  constructor() { this.value = 0; this.properties = { choices: Array.from({ length: 22 }, (_, i) => "" + i), parameterIndex: 0 }; this.valueChangedEvent = mkEv(); this.propertiesChangedEvent = mkEv(); }
  getChoiceIndex() { return Math.round(this.value * (this.properties.choices.length - 1)); }
  setChoiceIndex(i) { this.value = i / 21; this.valueChangedEvent.fire(); }
  backendPush(v) { this.value = v; this.valueChangedEvent.fire(); }
}
function mkEv() { const ls = []; return { addListener: (f) => ls.push(f), fire: () => ls.forEach((f) => f()) }; }

const sliders = { lfo_rate: new Slider() };
const combos  = { o1_ws_src: new Combo() };
global.getSliderState   = (n) => sliders[n];
global.getComboBoxState = (n) => combos[n];

// Minimal el mocks: slider el (thumb + rect + listeners), combo el (options + selectedIndex)
const mkThumb = () => ({ style: {} });
function sliderEl() { const t = mkThumb(); return { _t: t, querySelector: () => t, getBoundingClientRect: () => ({ width: 100 }),
  setAttribute() {}, classList: { remove() {}, add() {} }, _h: {}, addEventListener(ev, fn) { this._h[ev] = fn; }, setPointerCapture() {}, releasePointerCapture() {} }; }
function comboEl() { return { options: Array.from({ length: 22 }, () => ({})), selectedIndex: 0, addEventListener() {} }; }

const lfoEl = sliderEl(), wsEl = comboEl();
const ctx = eval("(function(){ const Juce={getNativeFunction:()=>()=>{}}; " + bindSlider_src + "\n" + bindCombo_src + "\n return { bindSlider, bindCombo }; })")();
ctx.bindSlider("lfo_rate", lfoEl);
ctx.bindCombo("o1_ws_src", wsEl);

// Put the Pulsewidth-Mod source on "LFO 1" (index 1) and record it
combos.o1_ws_src.backendPush(1 / 21);
const before = wsEl.selectedIndex;

// Simulate a drag from low → high on the LFO1 Rate fader (bindSlider's pointer handlers → setNormalisedValue)
const fails = [];
lfoEl._h.pointerdown({ clientX: 0, preventDefault() {} });
for (let px = 0; px <= 100; px += 10) {
  lfoEl._h.pointermove({ clientX: px });
  if (wsEl.selectedIndex !== before)
    fails.push(`at rate≈${px}% : o1_ws_src.selectedIndex moved ${before} → ${wsEl.selectedIndex} (${wsEl.selectedIndex === 21 ? "Voice Number!" : ""})`);
}
lfoEl._h.pointerup({});

console.log(`lfo_rate swept 0→1 (thumb now ${lfoEl._t.style.left}); o1_ws_src.selectedIndex: before=${before} after=${wsEl.selectedIndex}`);
if (fails.length === 0) { console.log("PASS: LFO1 Rate drag left o1_ws_src (Pulsewidth Mod) unchanged the entire sweep."); process.exit(0); }
console.log("FAIL — LFO1 Rate drag cross-wired o1_ws_src:"); fails.forEach((f) => console.log("  - " + f)); process.exit(1);
