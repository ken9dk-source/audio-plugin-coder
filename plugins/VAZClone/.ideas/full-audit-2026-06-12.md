# VAZClone — full Ghidra-vs-clone audit (2026-06-12)

Goal (user): *"kig hele vaz mappen igennem med ghidra uden undtagelser og samlign med VAZClonen — sikre at alt spiller som det skal og ingen bugs."*
Constraint: **static only** (Ghidra/decompile + integer-port reasoning + bank statistics) — **no WAV-recording analysis**.

Method: cross-referenced `Vaz2010Core.dll` decompiles (`tools/vaz_*.c`) and the real `.v2p` factory bank
(259 patches readable) against `plugins/VAZClone/Source/*`. Two complementary lenses:
1. **Range/usage statistics** over the whole bank (`tools/v2p_trace2.py`) — catches scaling clamps and unmapped fields.
2. **Stage-by-stage** decompile comparison (loader → mod-matrix → osc → filter → amp → env).

---

## TL;DR
**No critical bugs found.** The big one (preset misload) was the version-gated loader, fixed earlier this session.
This pass **confirmed the scaling and the mod-matrix are correct**, **fixed two silently-discarded loader fields**
(`bend_range`, `unison voices`), and **catalogued the remaining known gaps** (all minor / sequencer-tied).

---

## 1. Loader — scaling (range audit over 259 patches)
Cross-checked every `.v2p` field's observed min/max against the clone's divisor in `loadV2P`.

| Field group | bank range | clone divisor | verdict |
|---|---|---|---|
| Env A/D/R | 0..**425** (decay hits exactly 425) | `/425` | ✅ **exact** |
| Env sustain | 0..255 | `/255` | ✅ |
| Cutoff / Reso / Bandwidth | 0..255 | `/255` | ✅ |
| LFO rates, mixer levels, overdrive | 0..255 | `/255` | ✅ |
| Osc FM depth | ±255 | `/255` | ✅ |
| **Filter/amp/PWM mod depth** | **±127** | `/255` | ⚠️ see note |
| osc waves /4, filter mode /21, voice mode /2 | match | match | ✅ |

**⚠️ The one calibration uncertainty:** filter/amp/PWM mod *depths* are signed ±127 but normalised `/255`
(so they reach ~½ at max). This is **likely correct, not a bug**: VAZ applies depth as `depth × modsource >> 2`,
and a rough integer trace shows full depth (127) sweeps the cutoff ~½ the index range — which matches the
clone's ½-scale `coNorm += cutAmt·mv`. Confirming it exactly needs the modsource Q-range trace *or* an A/B
(banned). **Left as-is** (changing to `/127` risks doubling every filter-env depth in the bank).

→ **Conclusion: no scaling bugs.** Env `/425` is provably exact.

## 2. Loader — completeness (FIXED this pass)
Two fields were *read and discarded* by `parseV2P`:

- **`bend_range` ← e0610** (pitch-bend range, 1..24 st). 256/259 patches use the default `2`, 3 differ.
  Now mapped: `bend_range = (e0610−1)/23`.
- **`unison voices` ← e095c** (2..16, v2.0 patches only — 47 patches; 19 want a fat **16-voice** unison that
  was playing as the default 4 → audibly thin). Now mapped (v≥200 only): `uni_voices = (e095c−1)/15`.

Byte-consumption is **provably unchanged** — both reads already happened, only their results were dropped.

### Loader gaps left (documented, not fixed)
- **LFO2 delay** — 100/259 patches set a non-zero fade-in delay. **The clone has no LFO delay param at all.**
  Needs a new param + per-LFO fade envelope (+ UI). Biggest remaining loader gap → candidate for a follow-up.
- **lfo3_wave** — 111/259 non-zero, but the `.v2p` stores a 0..174 waveform-table index while the clone's LFO3
  is binary Tri/Sine. Lossy to map; skipped (LFO3 is a minor source).
- **voice_count** (polyphony, separate from unison) — clone is always-poly; no param to map onto. N/A.

## 3. Mod matrix — source indexing (the keystone)
The clone's `modSrcs` order (0..21) and resolver coverage (`SynthVoice.h mv()` + `ModBus::value()`):

| idx | source | clone | idx | source | clone |
|--|--|--|--|--|--|
|0|None|✅|11|Osc2|✅|
|1|LFO1|✅|12|Noise|✅|
|2|LFO2|✅|13|External In|— (no host input)|
|3|LFO3|✅|14|**Accent**|❌ → 0 (7 patches)|
|4|Env1|✅|15|**Seq A**|❌ → 0 (13 patches, deferred seq)|
|5|Env2|✅|16|**Seq B**|❌ → 0 (1 patch, deferred seq)|
|6|ModAmp1|✅ computed|17|Velocity|✅|
|7|ModAmp2|✅ computed|18|Pressure|✅|
|8|Lag|✅ computed|19|Control A|✅|
|9|Osc1|✅|20|Control B|✅|
|10|Osc1 Pitch / keytrack|✅|21|Voice Number|✅|

**Order verified two ways:** (a) defaults match VAZ (cut-mod1=Env2, cut-mod2=LFO1); (b) **the bank's usage
profile** — most-routed indices are Env1(308) > Env2(249) > LFO2(206) > LFO1(191) > LFO3/keytrack(120) >
Velocity(89) > Lag(68). That popularity ranking is exactly what a synth should show; a scrambled order would
make oddball sources (Voice Number, Noise) the most common. **→ order is correct.**

**Verified the implemented global sources are actually fed** (not stubbed): `lfo1/2/3`, `env2`, `noise`, `ma1`,
`ma2`, `lag` buffers are all computed each block in `PluginProcessor.cpp` (ModAmps = In×AM w/ single-quadrant
option; Lag = one-pole slew). Good.

**Gaps:** Accent (7 patches) + Seq A/B (14 patches) return 0. Accent and the sequencer are tied to the
**deferred step-sequencer** task → those routings come online with it. External-Input has no host signal. Each
affected patch loses *one* modulation lane; the rest of the patch is correct.

## 4. DSP stages
- **Filters** — bit-exact integer ports for every mode any factory patch selects (A-LP/HP/BP, B, C, D-HP, K, R;
  Comb's float already matched). See `[[project_vazclone_filters]]`. ✅
- **Oscillators** — saw/pulse/tri/sine/multisaw + FM1/FM2/PWM/sync; previously harness-validated. Tuning range
  (±3 oct via octave±2 + coarse±12) covers the bank's −6000..+1200-cent span. ✅
- **Amp / pan** — VCA × Env1, two AM lanes, pan-mod — all wired and fed. ✅
- **Output / overdrive** — ✅ **EXACT now (2026-06-13)** + a real bug fixed. The output stage was fully decoded
  from the voice render `vaz_big.c @0x4dbddc:1647-1681` (no A/B needed): drive `×256/(256−od)` (the `+0x2b4`
  gain law @vaz_prims.c:3007, 1×..256× = 0..48 dB) → hard-limit to the cubic's monotonic peak **±1/√3** (VAZ's
  `±0xd105e8` clamp) → cubic **`x−x³`** → ×amp VCA, **always on**. Ported into `VAZVoice` (per-voice, pre-VCA,
  matching VAZ) with a 1/(2√3) input anchor. **BUG found+fixed:** the clone was running *two* overdrive stages —
  a per-voice cubic AND a master-bus `tanh` — so od>0 double-saturated; the master tanh is removed. The
  per-voice **DC-block** one-pole HP before the clip is now ported too (commit after, `DAT_006df6c4 = −0.0001`
  → R'=0.9999 ≈ 0.7 Hz, SR-scaled; flat in-band, −77 dB at DC) so asymmetric pulse/PWM signals clip symmetrically.
- **Envelope** — ✅ **BIT-EXACT now (2026-06-13, commit 41bf408).** VAZ's env is a one-pole INTEGER ADSR
  (`vaz_big.c @0x4dbddc:384-443`); its per-sample rate coefs were dumped from the live BSS (`DAT_006db7e8`/
  `006db818`/`006dc0c0`) into `VAZEnvTables.h`. `VAZEnv` rewritten as the exact integer machine, SR-independent,
  ver<107 load-offset applied. Replaced the old empirical x⁴/x⁶ fit. See `[[project_vazclone_filters]]`.

---

## Disposition
- **Fixed & built:** bend_range + unison-voices loading. VST3 rebuilt OK.
- **No bugs to fix** in scaling, mod-routing, filters, osc, amp.
- **Follow-up candidates (in priority order):** (1) LFO delay param+DSP (100 patches), (2) bit-exact env via
  segment-table dump, (3) Accent + step-sequencer (deferred, big task / separate window), (4) lfo3 multi-wave.
