// fx_gui_binding_audit.test.js — Track-B GUI binding integrity for all 8 VAZ effect plugins.
// For each effect: control->param table from index.html vs ParameterIDs.hpp. Flags the bug class the
// Rate-fader / Osc2-detune bugs belong to: dead bindings (control -> nonexistent param), cross-wiring
// (2+ controls on one param), uncovered params, and conditional-visibility (mode-dependent) controls.
// Run: node fx_gui_binding_audit.test.js
const fs = require("fs"), path = require("path");
const ROOT = "C:/APC/y/plugins";
const FX = ["VAZPhaser","VAZDelay","VAZReverb","VAZEqualizer","VAZDecimator","VAZChorus","VAZFlanger","VAZAutopan"];
let anyFail = false;

for (const fx of FX) {
  const uiPath = path.join(ROOT, fx, "Source/ui/public/index.html");
  const idsPath = path.join(ROOT, fx, "Source/ParameterIDs.hpp");
  if (!fs.existsSync(uiPath) || !fs.existsSync(idsPath)) { console.log(`${fx.padEnd(14)} MISSING ui or ParameterIDs`); anyFail = true; continue; }
  const html = fs.readFileSync(uiPath, "utf8"), ids = fs.readFileSync(idsPath, "utf8");
  const validParams = new Set([...ids.matchAll(/constexpr auto\s+[a-z0-9_]+\s*=\s*"([a-z0-9_]+)"/g)].map(m => m[1]));

  const bindings = [];
  for (const [attr, kind] of [["data-param","slider"],["data-combo","dropdown"],["data-toggle","toggle"]])
    for (const m of html.matchAll(new RegExp(attr + '="([a-z0-9_]+)"', "g"))) bindings.push({ kind, param: m[1] });
  for (const m of html.matchAll(/bind\w*\([^,]+,\s*"([a-z0-9_]+)"/g)) bindings.push({ kind: "init-call", param: m[1] });

  const unknown = bindings.filter(b => !validParams.has(b.param));                 // dead binding (typo/removed param)
  const byParam = {}; bindings.forEach(b => (byParam[b.param] ||= []).push(b.kind));
  const dupes = Object.entries(byParam).filter(([, ks]) => ks.length > 1);         // cross-wiring
  const boundSet = new Set(bindings.map(b => b.param));
  const uncovered = [...validParams].filter(p => !boundSet.has(p));
  const condHidden = (html.match(/display\s*:\s*none|data-show|class="[^"]*\bhidden\b/g) || []).length;

  const fails = [];
  if (unknown.length) fails.push("DEAD[" + unknown.map(b => b.param).join(",") + "]");
  if (dupes.length)   fails.push("XWIRE[" + dupes.map(([p, ks]) => `${p}:${ks.join("+")}`).join(",") + "]");
  if (fails.length) anyFail = true;
  console.log(`${fx.padEnd(14)} params:${String(validParams.size).padStart(2)} bind:${String(bindings.length).padStart(2)} uncov:${String(uncovered.length).padStart(2)} cond:${String(condHidden).padStart(2)}  [${fails.length ? "FAIL" : "ok"}] ${fails.join(" ")}`);
  if (uncovered.length) console.log(`               uncovered: ${uncovered.join(", ")}`);
}
console.log(anyFail ? "\nRESULT: FAIL — dead/cross-wired bindings above." : "\nRESULT: PASS — no dead or cross-wired bindings in any effect.");
process.exit(anyFail ? 1 : 0);
