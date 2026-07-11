// filter_labels.test.js — instrumented unit test for the per-mode filter knob relabeling (b159088 + the B fix).
//
// Faithfulness contract: this does NOT re-type the logic. It extracts the REAL FILTER_LABELS array and the REAL
// bindFilterModeLabels() verbatim from index.html and eval()s them, and drives them through the SAME runtime code
// path the combo-box change-event uses: backend pushes a normalised value → valueChangedEvent fires →
// bindFilterModeLabels' apply() reads ComboBoxState.getChoiceIndex() → FILTER_LABELS[idx] → DOM + k2SetTarget. The
// mock reuses the REAL getChoiceIndex/setChoiceIndex formulas (asserted against the source so a drift fails the test)
// and spies on k2SetTarget to verify the B family re-targets knob-2 onto flt_aux.
//
// Expected labels come INDEPENDENTLY from the observed VAZ screenshots (task-2 matrix), never from FILTER_LABELS.
// Rows are [knob2 (Resonance-slot) label, flt_aux, hp_cutoff, mod-slot]. Run: node filter_labels.test.js

const fs = require("fs");
const HTML_PATH = "C:/APC/y/plugins/VAZClone/Source/ui/public/index.html";
const html = fs.readFileSync(HTML_PATH, "utf8");
function must(re, name) { const m = html.match(re); if (!m) throw new Error("could not extract " + name); return m; }

// ── 1. Extract the REAL relabel code verbatim ─────────────────────────────────────────────
const filterLabelsSrc = must(/const FILTER_LABELS = \[[\s\S]*?\n\];/, "FILTER_LABELS")[0];
const bindSrc         = must(/function bindFilterModeLabels\(\)\s*\{[\s\S]*?\n\}/, "bindFilterModeLabels")[0];

// ── 2. Pin the REAL ComboBoxState formulas so the mock stays faithful ─────────────────────
const getExpr = must(/getChoiceIndex\(\)\s*\{\s*return\s+([^;]+);/, "getChoiceIndex")[1].replace(/\s+/g, " ").trim();
const setExpr = must(/setChoiceIndex\(index\)\s*\{[\s\S]*?this\.value\s*=\s*([^;]+);/, "setChoiceIndex")[1].replace(/\s+/g, " ").trim();
if (getExpr !== "Math.round(this.value * (this.properties.choices.length - 1))") throw new Error("getChoiceIndex drifted: " + getExpr);
if (setExpr !== "numItems > 1 ? index / (numItems - 1) : 0.0") throw new Error("setChoiceIndex drifted: " + setExpr);

// ── 3. Faithful ComboBoxState (22 choices for filter_mode) ────────────────────────────────
class RealCombo {
  constructor(len) { this.value = 0.0; this.properties = { choices: Array.from({ length: len }, (_, i) => String(i)) }; this._ls = []; this.valueChangedEvent = { addListener: (f) => this._ls.push(f) }; }
  getChoiceIndex() { return Math.round(this.value * (this.properties.choices.length - 1)); }
  backendPush(v) { this.value = v; this._ls.forEach((f) => f()); }
}

// ── 4. DOM + globals the REAL bindFilterModeLabels references ──────────────────────────────
const mk = () => ({ textContent: "", classList: { toggle() {} }, style: { display: "" } });
const resEl = mk(), auxEl = mk(), auxSl = mk(), hpEl = mk(), rmEl = mk();
const byId = { resLbl: resEl, auxLbl: auxEl, auxSlider: auxSl, hpLbl: hpEl, resModLbl: rmEl };
const combo = new RealCombo(22);
global.getComboBoxState = (n) => { if (n === "filter_mode") return combo; throw new Error("unexpected combo " + n); };
global.document = { getElementById: (id) => byId[id] || null };

// ── 5. Instantiate the REAL arrays/functions; spy on k2SetTarget (the B re-target) ────────
const real = eval("(function(){ let k2SetTarget = (u) => { globalThis.__k2 = u; }; " + filterLabelsSrc + "\n" + bindSrc + "\n return { bindFilterModeLabels, FILTER_LABELS }; })")();
real.bindFilterModeLabels();

// ── 6. Structural cross-checks ────────────────────────────────────────────────────────────
const opts = [...html.matchAll(/<option value="(\d+)"[^>]*>([^<]+)<\/option>/g)].map((m) => ({ v: +m[1], name: m[2].trim() }));
const structural = [];
if (real.FILTER_LABELS.length !== 22) structural.push("FILTER_LABELS length " + real.FILTER_LABELS.length + " ≠ 22");
if (opts.length !== 22) structural.push("dropdown has " + opts.length + " options ≠ 22");
opts.forEach((o, i) => { if (o.v !== i) structural.push("option #" + i + " value=" + o.v + " (value≠index)"); });

// ── 7. EXPECTED from the observed VAZ screenshots. [knob2, flt_aux, hp_cutoff, mod]. ───────
//   B family: knob-2 = Bandwidth (aux hidden). C-Sep / D-HP+LP = Separation. K-HP+LP = HP-cut / HP-reso. Comb = Damping / I-O.
const EXPECTED = [
  ["Resonance", "Unused",          "Unused",             "Resonance Modulation"],  // 0  A LP
  ["Bandwidth", "Unused",          "Unused",             "Resonance Modulation"],  // 1  B LP
  ["Resonance", "Unused",          "Highpass Cutoff",    "Resonance Modulation"],  // 2  C 2P RM
  ["Resonance", "Unused",          "Highpass Cutoff",    "Resonance Modulation"],  // 3  C 4P RM
  ["Resonance", "Unused",          "Unused",             "Resonance Modulation"],  // 4  A HP
  ["Resonance", "Unused",          "Unused",             "Resonance Modulation"],  // 5  A BP
  ["Bandwidth", "Unused",          "Unused",             "Resonance Modulation"],  // 6  B HP
  ["Bandwidth", "Unused",          "Unused",             "Resonance Modulation"],  // 7  B BP
  ["Resonance", "Separation",      "Highpass Cutoff",    "Separation Modulation"], // 8  C 4P SM
  ["Resonance", "Unused",          "Highpass Cutoff",    "Highpass Modulation"],   // 9  C 4P HM
  ["Resonance", "Unused",          "Unused",             "Resonance Modulation"],  // 10 D LP
  ["Resonance", "Unused",          "Unused",             "Resonance Modulation"],  // 11 D BP
  ["Resonance", "Unused",          "Unused",             "Resonance Modulation"],  // 12 D HP
  ["Resonance", "Separation",      "Highpass Cutoff",    "Separation Modulation"], // 13 D HP+LP
  ["Resonance", "Unused",          "Highpass Cutoff",    "Highpass Modulation"],   // 14 C 2P HM
  ["Resonance", "Unused",          "Unused",             "Resonance Modulation"],  // 15 K LP
  ["Resonance", "Highpass Cutoff", "Highpass Resonance", "Highpass Modulation"],   // 16 K HP+LP
  ["Resonance", "Unused",          "Highpass Cutoff",    "Resonance Modulation"],  // 17 R 2P RM
  ["Resonance", "Unused",          "Highpass Cutoff",    "Highpass Modulation"],   // 18 R 2P HM
  ["Resonance", "Unused",          "Highpass Cutoff",    "Resonance Modulation"],  // 19 R 4P RM
  ["Resonance", "Unused",          "Highpass Cutoff",    "Highpass Modulation"],   // 20 R 4P HM
  ["Resonance", "Damping",         "Input/Output Mix",   "Resonance Modulation"],  // 21 Comb
];

// ── 8. Drive each mode via the runtime path and assert ────────────────────────────────────
const fails = [];
for (let n = 0; n < 22; n++) {
  globalThis.__k2 = undefined;
  combo.backendPush(n / 21);                 // native value push for choice n (22-item param) → valueChanged → apply()
  const idx = combo.getChoiceIndex();        // round-trip must equal n (catches indexing/scaling drift)
  const got = [resEl.textContent, auxEl.textContent, hpEl.textContent, rmEl.textContent];
  const exp = EXPECTED[n];
  const wantAux = (exp[0] === "Bandwidth");  // B: knob-2 re-targets onto flt_aux and the aux row hides
  const labOk = got.every((g, i) => g === exp[i]);
  const idxOk = idx === n;
  const k2Ok  = globalThis.__k2 === wantAux;
  const hideOk = wantAux ? (auxSl.style.display === "none") : (auxSl.style.display === "");
  if (!labOk || !idxOk || !k2Ok || !hideOk)
    fails.push({ n, name: opts[n] ? opts[n].name : "?", idx, got, exp, k2: globalThis.__k2, wantAux, disp: auxSl.style.display, idxOk, k2Ok, hideOk });
}

// ── 9. Report ──────────────────────────────────────────────────────────────────────────────
console.log("FILTER_LABELS length: " + real.FILTER_LABELS.length + " | dropdown options: " + opts.length + " | getChoiceIndex/setChoiceIndex formulas: OK");
if (structural.length) { console.log("STRUCTURAL ISSUES:"); structural.forEach((s) => console.log("  - " + s)); }
if (fails.length === 0 && structural.length === 0) {
  console.log("PASS: all 22 modes match the observed matrix; choiceIndex round-trips 0..21; B re-targets knob-2 → flt_aux (aux hidden).");
  process.exit(0);
}
console.log("FAIL: " + fails.length + "/22 modes:");
for (const f of fails) {
  console.log("  mode " + f.n + " (" + f.name + ")  choiceIndex=" + f.idx + (f.idxOk ? "" : " ← INDEX DRIFT") + (f.k2Ok ? "" : "  k2Target=" + f.k2 + " want " + f.wantAux) + (f.hideOk ? "" : "  auxDisplay='" + f.disp + "'"));
  console.log("     got : " + JSON.stringify(f.got));
  console.log("     want: " + JSON.stringify(f.exp));
}
process.exit(1);
