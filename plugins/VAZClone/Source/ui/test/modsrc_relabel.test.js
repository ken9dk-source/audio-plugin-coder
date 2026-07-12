// modsrc_relabel.test.js — regression guard for the LFO1↔Osc3 relabel (85abb27) and the per-mode filter
// labels (b159088/22b315c). Asserts that toggling mix3_src (Osc3 on/off) relabels ONLY the LFO1 panel header
// (lfo1Lbl) and leaves EVERY modulation-source dropdown (Pulsewidth/Waveshape/Frequency/Cutoff/… Modulation)
// showing exactly the same source in both states. Reproduces the reported bug: "Pulsewidth Modulation source
// flips LFO 1 → Voice Number when mix3Src==1".
//
// Faithfulness: extracts the REAL MODSOURCES / populateModSources / bindCombo / bindLfo1Identity verbatim from
// index.html and runs them through the same runtime path a backend value-push takes. No re-typed logic.
// Run: node modsrc_relabel.test.js

const fs = require("fs");
const HTML = "C:/APC/y/plugins/VAZClone/Source/ui/public/index.html";
const html = fs.readFileSync(HTML, "utf8");
const grab = (re, name) => { const m = html.match(re); if (!m) throw new Error("could not extract " + name); return m[0]; };

// 1. Real code, verbatim
const MODSOURCES_src   = grab(/const MODSOURCES = \[[\s\S]*?\];/, "MODSOURCES");
const populate_src     = grab(/function populateModSources\(\)\s*\{[\s\S]*?\n\}/, "populateModSources");
const bindCombo_src    = grab(/function bindCombo\(paramId, el\)\s*\{[\s\S]*?\n\}/, "bindCombo");
const bindLfo1_src     = grab(/function bindLfo1Identity\(\)\s*\{[\s\S]*?\n\}/, "bindLfo1Identity");

// 2. Pin the ComboBoxState formulas (fail if they drift)
const getExpr = grab(/getChoiceIndex\(\)\s*\{\s*return\s+[^;]+;/, "getChoiceIndex");
if (!/Math\.round\(this\.value \* \(this\.properties\.choices\.length - 1\)\)/.test(getExpr))
  throw new Error("getChoiceIndex formula drifted: " + getExpr);

// 3. Enumerate the ACTUAL mod-source dropdowns declared in the HTML (data-combo + data-modsrc)
const modDropdowns = [...html.matchAll(/data-combo="([a-z0-9_]+)"\s+data-modsrc/g)].map((m) => m[1]);
if (modDropdowns.length === 0) throw new Error("found no data-modsrc dropdowns");

// 4. Faithful ComboBoxState (per-param choice count)
const CHOICES = { mix3_src: 6 };            // Noise/Osc3/Ring/Ext/MA1/MA2 ; every mod dropdown = 22
class Combo {
  constructor(len) { this.value = 0; this.properties = { choices: Array.from({ length: len }, (_, i) => "" + i) }; this._ls = []; this.valueChangedEvent = { addListener: (f) => this._ls.push(f) }; }
  getChoiceIndex() { return Math.round(this.value * (this.properties.choices.length - 1)); }
  setChoiceIndex(i) { this.value = this.properties.choices.length > 1 ? i / (this.properties.choices.length - 1) : 0; this._ls.forEach((f) => f()); }
  backendPush(v) { this.value = v; this._ls.forEach((f) => f()); }
}

// 5. Mock <select> + label + document
class Sel {
  constructor(name) { this._name = name; this._opts = []; this.selectedIndex = 0; this._at = { "data-combo": name, "data-modsrc": "" }; }
  get options() { return this._opts; }
  set innerHTML(h) { this._opts = [...h.matchAll(/<option>([^<]*)<\/option>/g)].map((m) => ({ text: m[1] })); }
  getAttribute(a) { return a in this._at ? this._at[a] : null; }
  addEventListener() {}
  get shown() { return this._opts[this.selectedIndex] ? this._opts[this.selectedIndex].text : "(none)"; }
}
const selects = modDropdowns.map((n) => new Sel(n));
const lfo1Lbl = { textContent: "LFO 1" };
const combos = {};
const getCombo = (n) => (combos[n] || (combos[n] = new Combo(CHOICES[n] || 22)));

global.document = {
  querySelectorAll: (q) => q.includes("data-modsrc") ? selects : q.includes("data-combo") ? selects : [],
  getElementById: (id) => (id === "lfo1Lbl" ? lfo1Lbl : null),
};
global.getComboBoxState = (n) => getCombo(n);

// 6. Instantiate real funcs, populate + bind
const ctx = eval("(function(){ " + MODSOURCES_src + "\n" + populate_src + "\n" + bindCombo_src + "\n" + bindLfo1_src +
  "\n return { MODSOURCES, populateModSources, bindCombo, bindLfo1Identity }; })")();
ctx.populateModSources();
selects.forEach((s) => ctx.bindCombo(s._name, s));
ctx.bindLfo1Identity();

// 7. Put a KNOWN source on the pulsewidth-mod dropdown: o1_ws_src = "LFO 1" (index 1)
getCombo("o1_ws_src").backendPush(1 / 21);

const snapshot = () => ({ lfo1: lfo1Lbl.textContent, drops: selects.map((s) => s._name + "=" + s.shown) });

// 8. State A: mix3 = 0 (Noise). State B: mix3 = 1 (Oscillator 3).
getCombo("mix3_src").backendPush(0 / 5);
const A = snapshot();
getCombo("mix3_src").backendPush(1 / 5);
const B = snapshot();

// 9. Assert
const fails = [];
if (!(A.lfo1 === "LFO 1" && B.lfo1 === "Oscillator 3"))
  fails.push(`lfo1Lbl should flip LFO 1→Oscillator 3, got A="${A.lfo1}" B="${B.lfo1}"`);
A.drops.forEach((d, i) => { if (d !== B.drops[i]) fails.push(`mod-source dropdown changed with mix3: A[${d}] B[${B.drops[i]}]`); });
const ws = selects.find((s) => s._name === "o1_ws_src");
if (ws && ws.shown !== "LFO 1") fails.push(`o1_ws_src (Pulsewidth Mod) shows "${ws.shown}", expected "LFO 1"`);
if (ctx.MODSOURCES.length !== 22) fails.push("MODSOURCES length " + ctx.MODSOURCES.length + " ≠ 22");
selects.forEach((s) => { if (s.options.length !== 22) fails.push(s._name + " has " + s.options.length + " options ≠ 22"); });

console.log(`mod-source dropdowns tested: ${modDropdowns.length}  | MODSOURCES: ${ctx.MODSOURCES.length}`);
console.log(`mix3=0: lfo1Lbl="${A.lfo1}"  o1_ws_src="${ws ? ws.shown : "?"}"`);
console.log(`mix3=1: lfo1Lbl="${B.lfo1}"  o1_ws_src="${selects.find((s)=>s._name==="o1_ws_src").shown}"`);
if (fails.length === 0) { console.log("PASS: only lfo1Lbl relabels on mix3; all mod-source dropdowns unchanged."); process.exit(0); }
console.log("FAIL:"); fails.forEach((f) => console.log("  - " + f)); process.exit(1);
