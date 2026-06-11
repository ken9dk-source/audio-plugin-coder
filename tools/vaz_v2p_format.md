# VAZ 2010 `.v2p` binary format — complete field map (Ghidra, static)

Two loaders in `Vaz2010Core.dll`:
- **`FUN_004da270`** — the **binary `.v2p` reader** (sequential stream). Reads chunk headers
  via `FUN_004d9f20` ("Patch", needs ver≥200) → `FUN_004da0d8` ("PRST", needs ver≥100, returns the
  file version in `local_18`), then reads params **in a fixed order**.
- **`FUN_004d891c`** — the **named/text reader** (legacy import). Calls the SAME patch-object setters
  but with **property names** → this is the Rosetta stone that names every field below.

## Read primitives (byte widths)
| fn | width | meaning |
|----|-------|---------|
| `FUN_0049d5cc` | 4 | read u32 LE |
| `FUN_0049d620` | 1 | read byte |
| `FUN_0049d5f4` | 1 | read bool/byte |
| `FUN_0049d668(s,&out)` | 4 | read u32 into *out, return sign flag (mod-routing) |
| `FUN_0049d530(s,&str)` | 4+N | read u32 length then N bytes (string; only ver<106 sample path) |

## Patch-object struct offsets (also read by the DSP voice render FUN_004dbddc)
| setter | struct off | field (from named reader) |
|--------|-----------|---------------------------|
| de804 | 0x80 | LFO1 Rate |
| de8a0 | 0x84 | LFO1 Waveform (ver<200: (v==1)?4:0) |
| de930 | 0x8c | LFO1 WaveShape |
| de8f8 | 0x90 | LFO1 Retrigger |
| dead8 | 0xc4 | LFO2 Rate |
| debf8 | 0xdc | LFO2 Retrigger/TrigTarget |
| deba0 | 0xd0 | LFO2 S&H (c?6:1) |
| dec30 | 0xd8 | LFO2 Delay |
| dee54 | 0x118 | **Env1 Attack** (val+0x3f) |
| dee74 | 0x11c | **Env1 Decay** (val+0x94) |
| deeac | 0x120 | **Env1 Sustain** |
| deef8 | 0x124 | **Env1 Release** (val+0x94) |
| df054 | 0x150 | **Env2 Attack** (val+0x3f) |
| df074 | 0x154 | **Env2 Decay** |
| df0ac | 0x158 | **Env2 Sustain** |
| df0f8 | 0x15c | **Env2 Release** |
| df300 | 0x1a8 | **Osc1 Tuning** (default 0xfffff6a0 = centre) |
| df3c0 | 0x1ac | **Osc1 Waveform** (table DAT_0052a8b8) |
| df440/df454 | 0x1b8/0x1bc | Osc1 FM1 src/depth |
| df4e8/df4fc | 0x1c8/0x1cc | Osc1 PWM src/depth |
| df838 | 0x1d8 | **Osc2 Tuning** |
| df8f8 | 0x1e8 | **Osc2 Waveform** (table DAT_0052a8b8) |
| df908 | — | Osc1 Sync Target |
| df918 | — | Osc2 Modifier |
| df990/df9a4 | 0x7e | Osc2 FM1 src/depth |
| df9e4/df9f8 | 0x80 | Osc2 FM2 src/depth |
| dfa38/dfa4c | 0x82 | Osc2 PWM src/depth |
| (sample block) | 0x20c | **Osc2 Sample** (MSmp chunk; ONLY osc2 has a sample) |
| dfd5c/dfe28 | 0x220/0x230 | Filter source / mixer balance |
| **dffb0** | **0x254→0x258** | **Filter Type/Mode** (dropdown 0-21 → internal, see map) |
| e018c | 0x25c/0x260 | Filter Slew Limit |
| **e01a4** | **0x264** | **Filter Cutoff** (default 0xff) |
| **e01b4** | **0x26c** | **Filter Resonance** (default 0) |
| e01c4 | 0x270 | Filter Bandwidth (default 0x50; only if mode<3) |
| e01e4/e01f8 | 0x9f | Filter cutoff-mod1 src/depth |
| e0238/e024c | 0xa1 | Filter cutoff-mod2 src/depth |
| e028c/e02a0 | 0xa3 | Filter cutoff-mod3 src/depth |
| e02e0/e02f4 | 0xa5 | Filter reso-mod src/depth |
| e0334/e0348 | 0xa7 | Amp AM1 src/depth |
| e0388/e039c | 0xa9 | Amp AM2 src/depth |
| e0430 | — | **Amp Overdrive** |
| e09d8 | — | **Osc1 Portamento** |

## Filter dropdown (0-21) → internal mode @0x258 (from FUN_004dffb0)
`0→0x00 1→0x10 2→0x20 3→0x28 4→0x02 5→0x01 6→0x12 7→0x11 8→0x2c 9→0x2d 10→0x30 11→0x31`
`12→0x32 13→0x34 14→0x24 15→0x40 16→0x44(K) 17→0x50 18→0x54 19→0x58(R) 20→0x5d 21→0x60`

## Mod-source remap tables (stored index → internal source id)
`DAT_0052b0ac` (most mod sources), `DAT_0052a8b8` (osc waveform index). Need to dump these.

## Version families (260 factory patches)
- 201/202 = 47 patches (the "2.0" bank) — clone's current offsets fit these.
- 103/106/108/109/111 = 213 patches (82%) — older format, clone MISREADS → wrong sound.
- Width-affecting version gates inside FUN_004da270: only the ver<106 sample path. The ~90-byte
  1xx↔2xx size delta lives in the **oscillator/sample** sub-readers (FUN_004bad88 callers @0x4d738f,
  0x4d7567 + the vtable osc reader), so anchoring purely to PS/MSmp is insufficient — a faithful
  version-aware sequential parser is required.
