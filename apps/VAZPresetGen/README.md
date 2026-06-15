# VAZ AI Preset Generator

A standalone Windows desktop app that generates **valid `.v2p` trance presets** for the VAZ 2010
clone synth from a text prompt — no Python, no terminal, and no AI service needed at runtime.

> Type `uplifting trance lead`, pick a batch size, **Generate** → a list of in-range, quality-checked
> presets. Tick the ones you like → **Save (.v2p)** or **Evolve** into variations.

The generator is **not** an LLM. It is a deterministic rule engine whose ranges, sweet spots and
forbidden combinations were extracted from an analysis of **190 real VAZ presets**
(`data/trance_model.json`). Every preset is encoded with the same offset-validated `.v2p` writer the
clone itself uses, so the synth reads back exactly what we write.

---

## 1. Technical architecture

```
            ┌──────────────────────────────────────────────┐
  prompt →  │  classifier   "uplifting trance lead"        │ → (category, subgenre)
            ├──────────────────────────────────────────────┤
            │  RuleModel    data/trance_model.json          │  allowed / recommended / forbidden
            ├──────────────────────────────────────────────┤
   gen   →  │  PresetGenerator  sample ranges + pro-grid    │ → raw VAZ params {cutoff, e1a, fcut1s…}
            ├──────────────────────────────────────────────┤
   QC    →  │  QualityControl   ranges + forbidden combos   │  reject invalid (regenerate)
            ├──────────────────────────────────────────────┤
  encode →  │  v2p.encode   template + offset map           │ → bytes (.v2p, 1565 B v2.0)
            └──────────────────────────────────────────────┘
                              ▲ GUI (CustomTkinter)  drives all of the above
```

* **Engine layer** (`src/vazpresetgen/engine/`) — pure logic, fully unit-tested, GUI-independent.
* **Model layer** (`src/vazpresetgen/model/`) — loads the rule JSON + the parameter vocabulary.
* **GUI layer** (`src/vazpresetgen/gui/`) — a thin CustomTkinter front end over `PresetService`.
* **Data** (`data/`, `assets/`) — the rule model + a v2.0 `.v2p` structural template, both bundled
  into the `.exe`.

`.v2p` encoding (`engine/v2p.py`) is a faithful port of the clone's `parseV2P` — the PRST block is a
**sequential, version-gated** stream, so we trace a real v2.0 template to get every field's byte
offset, then write generated values there. The offset map is validated against the clone's own
`buildV2P` (lfo1rate=PS+10, e1a=PS+54, filterMode=sec3+28, cutoff=sec3+33 … all match).

## 2. Folder structure

```
apps/VAZPresetGen/
├── run.py                     # launch from source:  py run.py
├── build.ps1                  # build the .exe (installs deps, self-tests, PyInstaller)
├── VAZPresetGen.spec          # PyInstaller spec (one-file, windowed)
├── requirements.txt
├── README.md
├── assets/
│   └── init_template.v2p      # v2.0 structural skeleton (all sound params overwritten by the engine)
├── data/
│   └── trance_model.json      # rule model from the 190-preset analysis
├── src/vazpresetgen/
│   ├── config.py              # paths (source + frozen), app constants
│   ├── naming.py              # auto preset names
│   ├── model/
│   │   ├── params.py          # waves / voice modes / mod-source indices / tuning
│   │   └── rules.py           # RuleModel — allowed/recommended/forbidden per type
│   ├── engine/
│   │   ├── v2p.py             # .v2p encoder/parser (offset-validated)
│   │   ├── classifier.py      # prompt -> (category, subgenre)
│   │   ├── generator.py       # PresetGenerator — sample rules + pro-grid + evolve
│   │   ├── qc.py              # QualityControl — validate before save
│   │   └── service.py         # PresetService — façade the GUI uses
│   └── gui/
│       └── app.py             # CustomTkinter desktop GUI
└── tests/
    └── test_engine.py         # end-to-end: generate -> QC -> encode -> re-parse -> verify
```

## 3. Data models

* **`Preset`** (`engine/service.py`): `name, category, subgenre, params: dict, qc_ok, qc_notes`.
* **`params`** — raw VAZ integer fields the encoder writes, e.g. `cutoff`/`reso`/`overdrive` (0-255),
  `e1a/e1d/e1r` (0-425), `e1s` (0-255), `o1wave` (0-4), `voiceMode` (0-2), the grid
  `fcut1s=Env2, fcut2s=Osc1Pitch, fcut3s=Velocity`, `o1pwms/o2pwms=LFO`, tuning in cents.
* **Rule model** (`data/trance_model.json`) — per type `allowed_ranges`, `recommended_ranges`,
  `forbidden_combinations`; plus `parameter_statistics`, `trance_signature_rules`,
  `parameter_interactions`.

## 4. Categories & trance focus

Lead · Supersaw · Pad · Pluck · Bass · FX. Styles: **uplifting**, **ASOT**, **classic (2000-2010)**,
**progressive**, **psy** (each nudges cutoff / animation / drive within the allowed ranges).

## 5. Quality control (before every save)

* every sampled parameter is inside the type's **allowed range**;
* **forbidden combinations** are rejected: `amp_sustain=0 + short decay + ~0 attack` (click),
  `filter-env→cutoff + filter_sustain=0 + cutoff<70` (silent), `lead overdrive>120`;
* invalid candidates are discarded and regenerated, so saved presets are always loadable.

## 6. Run from source (Windows / macOS / Linux)

The app is pure Python + CustomTkinter — it runs on all three from the same source.

```bash
cd apps/VAZPresetGen
python3 -m pip install customtkinter      # Windows: py -m pip install customtkinter
python3 run.py                            # Windows: py run.py
```
macOS only: use **python.org Python 3** or `brew install python-tk` (Apple's system Python ships
without a usable Tk, which CustomTkinter needs).

## 7. Build a standalone app

PyInstaller cannot cross-compile — build each OS's artifact **on that OS** (the source is shared, on
GitHub). The spec is OS-aware.

```powershell
# Windows  ->  dist\VAZ Preset Generator.exe
powershell -ExecutionPolicy Bypass -File .\build.ps1
```
```bash
# macOS   ->  dist/VAZ Preset Generator.app   (drag to /Applications; right-click > Open first time)
# Linux   ->  dist/VAZ Preset Generator
chmod +x build.sh && ./build.sh
```

## 8. Use the presets

Generated `.v2p` files land in `Desktop\VAZ Generated Presets\` (changeable). In the VAZClone:
**Menu ▸ → Load Patch…** → pick a `.v2p`.

## Run the tests

```powershell
py tests\test_engine.py
```
