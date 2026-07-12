// control_binding_audit.test.js — Phase-3 audit. Builds the full control→parameter table for the ENTIRE
// index.html (every slider/dropdown/toggle/button-group/id-bound control → its target APVTS param) and
// cross-references ParameterIDs.hpp. Flags the cross-connection / dead-binding bug CLASS (the Rate-fader
// suspicion): a control bound to an UNKNOWN param, TWO controls sharing one param, or a param with NO control.
// Run: node control_binding_audit.test.js

const fs = require("fs");
const UI  = "C:/APC/y/plugins/VAZClone/Source/ui/public/index.html";
const IDS = "C:/APC/y/plugins/VAZClone/Source/ParameterIDs.hpp";
const html = fs.readFileSync(UI, "utf8");
const ids  = fs.readFileSync(IDS, "utf8");

// 1. Valid param IDs (the C++ source of truth)
const validParams = new Set([...ids.matchAll(/constexpr auto\s+[a-z0-9_]+\s*=\s*"([a-z0-9_]+)"/g)].map((m) => m[1]));

// 2. Every control → param binding
//    attribute bindings: data-param (slider), data-combo (dropdown), data-toggle (toggle)
const bindings = [];  // {kind, param, ctx}
for (const [attr, kind] of [["data-param", "slider"], ["data-combo", "dropdown"], ["data-toggle", "toggle"]])
  for (const m of html.matchAll(new RegExp(attr + '="([a-z0-9_]+)"', "g")))
    bindings.push({ kind, param: m[1] });
//    explicit init-call bindings: bindX(<selector-or-getElementById>, "paramId", ...)
for (const m of html.matchAll(/bind(?:ButtonGroup|RadioGroup|FloatPicker|ChoicePicker|Number|PitchWheel)\([^,]+,\s*"([a-z0-9_]+)"/g))
  bindings.push({ kind: "init-call", param: m[1] });
// bindDetuneRows() drives two detune params by mode
for (const p of ["poly_detune", "uni_detune"]) if (html.includes("bindDetuneRows")) bindings.push({ kind: "detune-row", param: p });
// bindFilterKnob2 re-targets resonance<->flt_aux (a KNOWN dual-target)
if (html.includes("bindFilterKnob2")) { bindings.push({ kind: "knob2", param: "resonance" }); bindings.push({ kind: "knob2", param: "flt_aux" }); }
// bindSigns binds each mod-row's +/- polarity buttons to the SIGN_MAP *_amt_inv param
for (const m of html.matchAll(/([a-z0-9_]+_src):\s*"([a-z0-9_]+_amt_inv)"/g))
  bindings.push({ kind: "sign-btn", param: m[2] });

// 3. Cross-checks
const unknown = bindings.filter((b) => !validParams.has(b.param));
const byParam = {};
bindings.forEach((b) => (byParam[b.param] ||= []).push(b.kind));
const KNOWN_DUAL = new Set(["resonance", "flt_aux", "poly_detune", "uni_detune"]);  // knob-2 re-target + detune-row swap are intentional
const dupes = Object.entries(byParam).filter(([p, ks]) => ks.length > 1 && !KNOWN_DUAL.has(p));
const boundSet = new Set(bindings.map((b) => b.param));
const uncovered = [...validParams].filter((p) => !boundSet.has(p));

// 4. Report
console.log(`params in ParameterIDs.hpp: ${validParams.size} | control bindings found: ${bindings.length} | distinct params bound: ${boundSet.size}`);
const fail = [];
if (unknown.length) { console.log("\n✗ UNKNOWN-param bindings (control targets a param that does not exist → dead/typo):");
  unknown.forEach((b) => console.log(`   ${b.kind}  "${b.param}"`)); fail.push("unknown"); }
if (dupes.length) { console.log("\n✗ SHARED-param bindings (2+ controls on one param — the Rate-fader cross-wiring class):");
  dupes.forEach(([p, ks]) => console.log(`   "${p}"  ← ${ks.join(" + ")}`)); fail.push("dupes"); }
console.log(`\nℹ params with NO GUI control (${uncovered.length}) — expected for hidden/internal params:`);
console.log("   " + (uncovered.join(", ") || "(none)"));

if (fail.length === 0) { console.log("\nPASS: every control targets a real, unique param; no cross-wiring / dead bindings."); process.exit(0); }
console.log("\nFAIL: " + fail.join(" + ")); process.exit(1);
