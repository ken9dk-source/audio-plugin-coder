// detune_conditional.test.js — regression guard for the "permanent Osc2 Detune fader" bug.
// Osc2's Multi-Saw detune must be the o2_shape slider RELABELED conditionally (o1labels[2]='Detune'
// when o2_wave==Multi-Saw), NOT a separate always-visible fader. The clone-internal o2_detune param
// (absent from .v2p) must have NO GUI control. Run: node detune_conditional.test.js
const fs = require("fs");
const UI = "C:/APC/y/plugins/VAZClone/Source/ui/public/index.html";
const html = fs.readFileSync(UI, "utf8");
const fail = [];

// 1. No permanent fader bound to the artifact param o2_detune
if (/data-param="o2_detune"/.test(html))
  fail.push('a control is still bound to o2_detune (artifact param — must be unexposed)');

// 2. No static standalone "Detune" label (Poly/Unison Detune are different, allowed)
const staticDetune = [...html.matchAll(/<div class="lbl"[^>]*>Detune<\/div>/g)];
if (staticDetune.length)
  fail.push(`${staticDetune.length} static <div class="lbl">Detune</div> present — Detune must come only from the relabel`);

// 3. The REAL Osc2 detune control exists: o2_shape slider + its label defaults to "Waveshape"
if (!/data-param="o2_shape"/.test(html)) fail.push("o2_shape slider (the real conditional detune) is missing");
if (!/id="o2xlbl"[^>]*>Waveshape</.test(html)) fail.push('o2xlbl does not default to "Waveshape"');

// 4. The conditional relabel is wired: index 2 (Multi-Saw) => "Detune", and it fires for BOTH oscs
if (!/const o1labels=\[[^\]]*'Detune'[^\]]*\]/.test(html)) fail.push("o1labels missing a 'Detune' entry");
const li = html.match(/const o1labels=\[([^\]]*)\]/);
if (li) {
  const arr = li[1].split(",").map((s) => s.replace(/['\s]/g, ""));
  if (arr[2] !== "Detune") fail.push(`o1labels[2] is "${arr[2]}", expected "Detune" (Multi-Saw)`);
}
if (!/getElementById\('o'\+n\+'xlbl'\)/.test(html)) fail.push("relabel does not target both oscillators ('o'+n+'xlbl')");

if (fail.length) { console.log("FAIL:\n  - " + fail.join("\n  - ")); process.exit(1); }
console.log("PASS: Osc2 Detune is conditional-only (o2_shape relabeled on Multi-Saw); no permanent o2_detune fader.");
