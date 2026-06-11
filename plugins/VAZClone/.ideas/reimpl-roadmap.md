# VAZ 2010 — Bit-Exact Reimplementation Program (roadmap)

Goal: replace the clone's float approximations with VAZ's **actual fixed-point DSP + extracted LUTs**,
toward functional 1:1 with `Vaz2010Core.dll`. This is a **multi-session RE program**, not a single task.

## Methodology (per subsystem)
1. **Locate** the function(s) in Core.dll — `tools/disasm-region.py` (capstone), VMT/RTTI via
   `tools/find-fx-classes.py` / `tools/find-callers.py` (note: code section = `CODE`, not `.text`).
2. **Decode** the fixed-point math (Q-format, per-sample loop) + **extract LUTs** (dump raw arrays).
3. **Reimplement** in the clone (exact float-equivalent of the fixed-point, or fixed-point where needed).
4. **Bit-compare** to null: render identical MIDI/preset through the **real VAZ** (File→Capture,
   `tools/vaz_auto`) and the **clone** (`VazRender`), diff with `tools/abtest/analyze.py`. Iterate.

   → The bit-comparison harness **already exists** (built during the A/B work): headless real render
   (clean Capture) + headless clone render + the analyzer. This is the key asset for this program.

## Confirmed anchors (ground truth so far)
| Address | Fact | Clone status |
|---|---|---|
| `0x4D4720` | Filter cutoff coef builder: `fc = exp(10.24·idx/1024)` (idx=cutoff param 0-1024) | ✅ applied (D1, ±4%) |
| `0x4D47A3` | Pole coef `a = (2−cosω) − √((2−cosω)²−1)`, stored **Q30** (×2³⁰) | ✅ exact (D2) |
| helpers | `0x402ba8`=exp · `0x402b98`=cos · `0x402bc0`=sin · `0x402bf4`=round→int · `0x42c220`=pow | — |
| effects | Flanger `0x5204F4` · Phaser `0x5218D8` · Chorus `0x518AD8` · EQ `0x51EB34` fully RE'd | ✅ matched |
| `.v2p` | param→byte map | partial (45/115) |

## Phases (audibility order — each = locate → decode → extract → reimpl → bit-null)
- **P1 Filter** (in progress): cutoff+pole ✅. **Per-sample PROCESS fn LOCATED @0x4DD870** (reads the
  coef tables; found via BSS table 0x6945e4 readers). Decoded so far:
    - **3 cutoff-indexed coef tables**: `0x6145e4`, `0x6545e4`, `0x6945e4` (0x4000 apart = 4096 int32 each,
      used 1024; idx = cutoff clamped 0..0x3ff + a mod offset [esi+0x164]). Built by the @0x4D4720-style builders.
    - **Fixed-point**: 64-bit accumulate (`ebx:ebp`), `acc = t3·in + t2·s2 + t1·s1`; state shift s2←s1.
    - **Saturation = cubic `y = x − x³/2`** (@0x4DD8E3-F1, no makeup) — **≠ clone's `1.5·(x−x³/3)`** (clone
      adds 1.5× linear makeup; cubic coef 0.5 matches). Real applies it to the feedback **state**, in Q-format.
    - **Topology = multi-pass (≈2× oversampled) 2nd-order section(s) with cubic state-feedback** — NOT the
      clone's 4-pole one-pole ladder. → For bit-exactness the filter needs **reimplementation** to this form.
    - The process is a **large dispatched function** (~0x4DD7xx–0x4DDExx) with a **jump table over the 22
      filter modes** (`cmp eax,0x28/0x2c…` @0x4DD83B+, `jmp 0x4de286`), per-mode input one-pole smoothers
      (0x4DD7ED-0x4DD811, states [esi+0x17c]/[esi+0x180]) feeding the shared biquad+cubic core above.
    - **RESONANCE found** (@0x4DD736-0x4DD7DC): a **4th coef table `0x5535e4`** (cutoff-indexed) = the
      integrator/resonance coef `ecx`. **Resonance amount = [esi+0x164]** (Q9, `<<23`), applied as a feedback
      gain `edi += 2·reso·(fb − edi)` (the `add ebp,ebp`=×2) → the resonant peak. **≠ clone's `reso·4.2·cubic(s3)`**
      — real scales `2·reso` and the cubic sits on the integrator state, not the ladder tap.
    - **Sub-mode = [ebx+0x258] & 3** = the modRoute (0 Resonance / 1 Highpass / 2 Separation) — matches the
      clone's `modRoute`. Submode 1 also applies **1.5× input gain** (`edi+edi/2` @0x4DD7A3).
    - So the real "R" filter = **state-variable / integrator-cascade with `2·reso·(fb−in)` feedback + cubic
      `x−x³/2` on the state**, 4 cutoff-indexed coef tables (0x5535e4 reso, 0x6145e4/6545e4/6945e4 biquad),
      ≈2× oversampled, fixed-point. Clone's ladder topology + `reso·4.2` + `1.5(x−x³/3)` all differ.
    - **COEF-TABLE BUILDER decoded further @0x4D4808-0x4D4988**: the coef tables are **2D = cutoff × 64
      resonance steps** (inner loop `ebp = 0..63` = reso index). Per (cutoff, reso):
        - bandwidth `resoVal = (64 − resoIdx)·4 + 1`  (5..257)
        - damping `E = exp(−π·resoVal/sr)`  ;  angle `C = cos(2π·freq2)`  (freq2 = clamped cutoff norm, ≤0.45)
        - `B1 = 4·E·C/(E+1)`
        - the resonator biquad has **3 coefs** (all Q28, ×2²⁸), stored as 3 separate **2D tables [reso 0..63][cutoff 0..1023]**
          (inner loop advances each pointer `+0x1000` = one 1024-int32 cutoff row per reso step):
            - **b0** = `sin(π·freq2)·√(((E+1)² − B1²)·(1−E)/(E+1))·22.7·(resoIdx+64)/64`   (@0x4D4979, gain)
            - **a1** = `B1 = 4·E·C/(E+1)`                                                  (@0x4D498A)
            - **a2** = `−E = −exp(−π·resoVal/sr)`                                           (@0x4D49A5, negated)
        - (so the resonance is a **64-step 2D biquad table**, not the clone's continuous `reso·4.2`.)
        - The builder has **multiple sections** (next @0x4D4A40 uses a 0.25 freq clamp instead of 0.45 → a
          different pole-config/engine) → it precomputes ALL engines' coef tables. Big precompute.
      Magic constants found: **22.7** (gain), Q30 (pole), Q28 (resonator coef), Q9 (reso feedback <<23).
    - **PER-SAMPLE ASSEMBLY decoded + VALIDATED (R engine fully spec'd):** the process @0x4DD897 is a
      direct-form **resonator biquad** `y[n] = b0·x[n] + a1·y[n-1] + a2·y[n-2]` (acc 64-bit; out = acc>>32<<2),
      with **cubic `x−x³/2` on the fed-back state** (`s1 = cubic(out)`, `s2 = s1_old`), **2× oversampled**,
      coefs from the 2D tables indexed `cutoffIdx + resoIdx·1024`. (t3=b0, t1=a1, t2=a2.)
    - **Python-validated** (tools): the decoded biquad is **stable** (pole radius 0.991→0.9998 as reso 0→63,
      i.e. → near self-oscillation at max), **peaks exactly at the cutoff** (499 Hz @fc=500, 1998 @fc=2000),
      and **resonance sharpens the peak** (8.5×→104× @fc=500). Exactly correct VAZ filter behaviour → **the
      whole R-engine decode is confirmed correct.** DC gain ≈2.1× (a scaling to calibrate).
    - **R-ENGINE STATUS: fully decoded + validated.** Only remaining detail = the exact fixed-point Q-scaling
      so the cubic operates in-range (the `<<2`/`>>32` shifts) — best nailed by **reimplementing + iterating
      against the bit-null harness** (the harness shows scaling as level/timbre error). That is the next loop:
      replace the clone's R ladder (VAZLadder) with this resonator-biquad+cubic, calibrate scaling, bit-null.
    - **BIT-NULL #1 (real VAZ Capture vs clone, 2026-06-09):** the reimplemented R filter's **resonant PEAK
      FREQUENCY matches the real** (129 Hz↔129 Hz lo-reso; 393↔428 Hz hi-reso ≈9%) → the decode + structure
      + cutoff map are **confirmed correct**. BUT the **resonance Q is ~17-20 dB too HIGH** (clone too resonant)
      at both lo + hi reso. The cubic-drive only reduces Q ~2 dB (it limits amplitude, not sharpness) → NOT the
      fix. **Diagnosis: the clone is missing the INPUT ONE-POLE LP stages** (real @0x4DD7ED: 2 one-poles with
      the cutoff coef BEFORE the resonator biquad) — they broaden the response + lower the effective Q. (Also
      possible: a different .v2p reso-byte→resoIdx map.) **NEXT loop:** decode the exact mode-19 per-sample
      path (submode [ebx+0x258]&3=0), add the input one-pole stage(s), re-bit-null to confirm the Q drops to
      match. Then the other engines (A/B/C/D/K via the 22-mode jump table) + the 0x5535e4 integrator table.
    - (The new biquad is kept deployed — its peak now matches the real, more faithful than the old ladder; the
      Q is bounded/stable, just hotter than the real until the input stages are added. Revertible via git.)
    - **CORRECTION + measurement finding (2026-06-09):** the biquad IS R (it has the cubic `x−x³/2` = the
      distorted resonance; the one-pole path @0x4DD7ED is CLEAN = the A/B engines). So the mapping was right —
      the Q gap is a **calibration**, not a mis-map. BUT: a resonance-damping test (`a1·=dmp, a2·=dmp²`, dmp=0.985)
      did **NOT move the bit-null Q metric** (peak/median stayed +17/+20 dB). **Root cause: the synth render can't
      cleanly isolate FILTER Q** — the oscillator harmonics + the note fundamental dominate the spectrum (at reso 50
      the "peak" is the 129 Hz note fundamental, not the resonance). So **filter-Q calibration via the synth
      render is not viable**; it needs a dedicated **filter-impulse/noise harness** (drive the filter with flat
      input, measure its frequency response directly) — a tooling task. A mild dmp=0.985 nudge is left in as a
      conservative anti-scream safety (not precisely calibrated). **R status: peak-matched + structurally correct
      (cubic resonator biquad); exact Q pending a filter-impulse harness.** This is the cleanest verifiable state.
    - **FILTER-IMPULSE HARNESS BUILT (tools/abtest/filter_response.py, 2026-06-09).** Trick: render the saw at a
      fixed cutoff but several reso settings incl. reso=0, then `spectrum(reso_X)/spectrum(reso_0)` cancels the
      osc + base-LP → **pure resonance shape**, osc-independent. Real-vs-clone resonance-peak (dB over reso=0):
      reso60: 21 vs 10 · reso120: 20 vs 21 · reso180: 28 vs **86** · reso240: 47 vs **93**.
      **FINDINGS:** (1) the REAL resonance is **controlled** — caps ~47 dB even at max (the cubic compresses HARD
      near self-osc); its curve is **flat ~20 dB at low-mid reso, steep only at the very top**. (2) The clone's
      curve is the **wrong shape** — too weak at bottom, **explodes to 86-93 dB at top** (clone-dB ≈ 2× real-dB →
      the 2×-pass-same-input over-builds the resonance, and the x·0.5 output keeps the cubic too small to compress).
      **NEXT (calibration, now measurable):** fix the 2-pass structure + scale the feedback state so the cubic
      actually compresses near self-osc → match the real's 21/20/28/47 dB curve via this harness.
    - **CROSS-CHECK vs EXACT dumped coef tables (2026-06-09, via tools/dump_vaz_tables.py ReadProcessMemory):**
      read the live filter coef tables at known (cutIdx,reso) indices and compared to my formula. **RESULT —
      the POLE decode is VALIDATED exactly:** table a1(0x6145e4)=+1.99 = my B1, a2(0x6545e4)=−0.99 = my −E
      (the Q-format is **Q30**, not the Q28 I'd assumed). **But b0(0x6945e4) was ~8× too large in my formula**
      (table ≈0.0009 vs my 0.0076) → almost certainly the cause of the resonance EXPLOSION (8× hotter input into
      a high-Q resonator). **So the biquad math was right except the b0 gain + the Q30 scaling.** → A correct
      filter redo is now de-risked: use the EXACT dumped 2D coef tables (b0/a1/a2 at 0x6945e4/0x6545e4/0x6145e4,
      reso·1024+cutIdx, Q30) directly, OR fix b0 (÷~8) + use Q30. Index by cutIdx (fc=exp(10.24·cutIdx/1024)) and
      resoIdx. The cubic + 2-pass structure then need re-checking against the harness with the correct b0.
    - **★ RESOLUTION — the filter has MULTIPLE engines; the MAIN one was decoded 2026-06-10 (disasm @0x4DD646):**
      the per-sample filter dispatches on mode `eax=[ebx+0x258]`: `cmp eax,0x2d(45) jg` / `cmp eax,0x12(18) jg`
      → **modes 0–18 = the MAIN engine** (the common LP/BP/HP), modes 19–45 = the cubic "R" engine decoded above
      (tables 0x61/0x65/0x69), modes >45 = a third. **All my earlier P1 work (cubic, 86/93 dB explosion) was the
      R engine — a DIFFERENT, less-common mode.** The MAIN engine is cleaner + correct:
        - **TWO cascaded identical biquad sections** (@0x4DD689 and @0x4DD6DE), output = **average** of the two
          (`ebp += ebx; sar ebp,1`). 4-pole total. **No cubic** in this path.
        - Coef tables (NOT the 0x6x ones): **b0=`0x5545e4`, a1=`0x5945e4`, a2=`0x5d45e4`**, plus reso/mix table
          **`0x5535e4`** (cutoff-indexed). Index = **`resoIdx·1024 + cutoffIdx`**, `cutoffIdx=cutoff>>19` (0..1023),
          `resoIdx=reso>>2` (0..63). cutoffIdx↔Hz via `fc=exp(10.24·cutoffIdx/1024)`.
        - Fixed-point per stage: 64-bit acc `= s2·a2 + s1·a1 + (in<<2)·b0`; `s1' = (acc>>32)<<4` (net **a1/a2 = Q28**,
          b0 effective Q30 w/ the in<<2). State [esi+0x184]=s1, [esi+0x188]=s2, [esi+0x194]=acc-lo.
        - Output mode mix = `[ebx+0x258]&3` (0/1/2/3 = LP/…); the cutoff has a **slew limiter** (max step 0x200000)
          and there are per-mode input one-pole smoothers before the cascade.
        - **VALIDATED (live coef dump + sim):** 2-cascade resonance peak rises **22.1 dB (reso0) → 49.7 dB (reso63)**,
          poles all stable (R 0.991→0.9998), b0 falls at high reso (constant-peak normalization) — **matches the real's
          ~47 dB character.** The earlier 86/93 dB was the wrong engine + wrong tables. → **This is the bit-exact core.**
      **BUILD PLAN (de-risked, next focused arc):** dump full 2D tables 0x5545e4/0x5945e4/0x5d45e4 (+0x5535e4) →
      embed in clone → implement 2-cascade biquad (exact int scaling) + cutoff/reso index maps + LP output mix →
      harness-validate vs real (filter_response.py) → swap VAZLadder only once validated (ladder stays until then).
    - **★★ COMPLETE clean-engine (modes 0–18) DECODED + EXACT-INTEGER-SIMULATED (2026-06-11):**
      Re-reading the disasm: the two "stages" are NOT cascaded biquads — they share the same input + coefs and the
      64-bit acc-low carries between them = a **2× oversampled single biquad** (2 sub-steps, ZOH input, output =
      avg of the two). THEN the output mix (`mode&3`, @0x4DD7ED for ==0/LP): **two one-pole LPs** on the input
      (state [esi+0x17c],[esi+0x180]; coef `rc`=0x5535e4[cutoffIdx], where rc·4/2³² = the pole, e.g. 0.944 @cutIdx600
      = a 403 Hz LP ✓), then a **resonance mix** `out = LP2(in) + (reso<<23)/2³²·(biquad_out − LP2(in))`
      (reso 0–255, so the mix gain ≤ ~0.49). State vars: s1/s2 (biquad), p1/p2 (one-poles), acc-lo. Submodes
      `&3`: 0=LP(@0x4DD7ED), 1=@0x4DD7A3 (1.5× in-gain), 2/3=@0x4DD75B.
      **EXACT integer sim of the complete LP** (live coefs, 32-bit-masked state, sine-sweep): resonance peak
      **−3.4 / −2.1 / +2.0 / +10.3 / +15.8 dB** at reso 60/120/180/240/252 — i.e. **caps ~16 dB**, gentle.
      **→ The earlier "real 21/20/28/47 dB" was CONFOUNDED** (the osc-cancel didn't remove the note fundamental
      sitting at the cutoff) — the clean engine is mild. **The deployed clone ladder (~47 dB) is far TOO resonant
      for this engine.** BUT: the cubic **R engine (modes 19–45)** self-oscillates (much hotter) and is likely what
      acid/resonant patches actually select → **which engine the default `.v2p` uses is the open question** (needs
      the filter-mode byte map). **The only reliable validation is the A/B bit-null** (render real Capture vs clone,
      same patch/MIDI, diff) — the spectrum-ratio harness is unreliable here. Until the default-engine + a bit-null
      confirm a target, the ladder stays deployed (a working approx).
    - **★★★ ALL FILTER TYPES: status + factory-patch priority (2026-06-11):** VAZ's 7 filter families are
      documented in the CHM (`Synth Windows/Filter Mode Dialog.htm`): **A** (variable-reso-bandwidth LP/BP/HP),
      **B** (fixed-bandwidth LP/BP/HP), **C** (2P/4P resonant LP + 1-pole HP, Separation), **D** (state-variable
      LP/BP/HP + HP→LP series), **K** (2-pole Sallen-Key, distorted self-osc, LP / HP+LP), **R** (4-pole
      integrator cascade self-osc + 1-pole HP, 2P-tap/4P), **Comb** (delay+feedback+damping). **The clone
      ALREADY implements all 7** (Synth.h `VAZMultiFilter`, engines 0–6, the 22-mode `setMode` table) and wires
      the `.v2p` mode byte (`sec3+28`, 0–21) → `setMode`. **VERIFIED working+distinct** (render a fixed note
      through each mode: A-LP centroid 175, A-BP 400, D-HP 2811, R-4P 38=darkest, Comb 3035=spread — all
      characteristic). **Factory-patch mode distribution (260 patches, none out-of-range):** mode **0 = 213
      (82%!) = Type A Lowpass**, 19 = 34 (13%) = R-4P, 16 = 5 (K), 17 = 3 (R-2P), 21 = 2 = Comb (CONFIRMED by
      the patch named "Comb Filter Feedback"). **→ STRATEGIC REDIRECT: the fidelity priority is engine 0 (Type
      A Lowpass) — 82% of all patches — NOT the resonant R engine** that all the earlier P1 work targeted (only
      13%). Type A = the gentle clean engine I decoded exactly (2× oversampled biquad + 2 one-pole LPs +
      reso-mix, caps ~16 dB). The clone's engine 0 is a TPT-SVF *approximation* (k=2−1.98·reso, can self-osc —
      likely too hot vs VAZ's ~16 dB cap). **Next fidelity arc: upgrade engine 0 to the exact decoded A-LP**
      (embed/fit the 0x5545e4/0x5945e4/0x5d45e4/0x5535e4 tables + the 2×-oversampled recurrence), bit-null vs
      real on a mode-0 patch. That single upgrade improves 82% of patches.
    - **A-LP exact-coef extraction (2026-06-11):** dumped a1/a2/b0 across reso×cutoff. **POLES clean + separable:**
      `R(reso) = sqrt(-a2/2^28) = 0.9909 + 0.0089*(resoIdx/63)` (identical @cutIdx 400 & 700), `a1 = 2R*cos(th)`
      (ok to 0.9%), `a2 = -R^2`, th=2*pi*exp(10.24*cutIdx/1024)/sr. **b0 (gain) NOT cleanly separable** —
      b0/(2^28*sin^2 th) varies with cutoff at low reso (1.87@400 vs 0.55@700) = the variable-bandwidth
      normalization (biquad peak rises ~11->25 dB over reso). rc=0x5535e4[cut] one-pole coef 0.25->0.04, 1D.
      -> engine-0 build: poles from the formula; b0 needs a small 2D table (~16 KB) or 2-term fit; then the
      2x-oversampled recurrence + 2 one-poles + reso-mix; validate vs the Python integer sim (-3->16 dB).
    - **★ KEY DISCOVERY — the clean-filter resonance is FIXED-POINT-QUANTIZATION-LIMITED (2026-06-11):** a
      float port of the exact recurrence (same a1f/a2f/B0 coefs) **over-resonates by 13-23 dB** vs the integer
      recurrence (reso 60-252: integer -4/-2/+2/+8/+6 dB, float +9/+17/+24/+31/+29 dB). Cause: VAZ stores the
      biquad state as `s1 = (acc>>32)<<4` = **quantized to multiples of 16**, which damps the resonance
      amplitude-dependently to the observed ~16 dB cap. There is NO explicit clamp — the fixed-point precision
      IS the resonance limiter. **⇒ This is why the filter has been hard all along.** A float/double clone
      engine (the current TPT-SVF, and engines C/D/K) has infinite precision ⇒ **over-resonates vs VAZ at high
      reso.** A bit-exact clone needs VAZ's *exact integer arithmetic* (int64 acc, int32 quantized state, the
      acc-low carry) AND a matched input signal scale (the Q is mildly amplitude-dependent). So engine-0 bit-exact
      = port the integer recurrence to C++ (not float) — doable but intricate. Cheap interim improvement: cap the
      clone's clean engines (A/B/C/D) resonance to ~16-18 dB (they currently can self-oscillate, which VAZ's
      Type A/B/C/D do NOT per the CHM — only K/R self-oscillate).
- **P2 Oscillator**: find the wavetable read (32-bit phase, top-bits index, interp) → **extract the wave
  LUTs** (saw/tri/sine/pulse, sizes 256/512, mip levels) → reimpl phase+interp fixed-point. (Highest raw-timbre value.)
- **P3 Envelope**: ADSR fixed-point — attack/decay/release curves + Multi/Reset/Cycle/Curve. (Resolves the parity-audit B1-B4.)
- **P4 LFO**: 8 waveforms + shape morph + rate/sync.
- **P5 Modulation matrix**: 22 sources, bipolar-depth math, FM (incl. the 2nd FM slot/osc), Mod Amps, Lag.
- **P6 Voice mgmt**: allocation/stealing, Unison, Portamento, pitch quantise.
- **P7 MIDI + presets + timing**: note/vel/bend, full .v2p decode, block/sample timing.

## Immediate next step
Find the filter **per-sample PROCESS** function (consumes the Q30 pole coef + applies resonance feedback
+ the engine topology). Search: callers/readers of the coef table built @0x4D4720. → RE the resonance
map (parity-audit D3) → P1 becomes the first fully bit-RE'd subsystem, validated by the A/B harness.

## Honest note
This program is large (the synth engine is far bigger than any single effect). Progress is incremental
and resumable via this doc + the parity-audit.md deviation queue. Each session should close one or more
locate→decode→reimpl→bit-null loops.
