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
    - TODO (continuing P1): decode the remaining resonator coefs (a1/a2/b0) in this builder loop; map the
      22-mode jump table → each engine; then reimplement the R filter (default mode 19) to this exact 2D-table
      + biquad + cubic structure and **bit-null vs the A/B harness** (first closed reimpl loop). The other 5
      engines (A/B/C/D/K) + Comb follow. NOTE: the filter is intricate enough (4 tables, 2D reso, magic consts,
      Q-formats, 22 modes, oversampling) that exact reimpl is several more decode loops then a careful rewrite.
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
