"""PresetService — the façade the GUI talks to. Ties prompt-classification, generation, quality
control, naming, .v2p encoding and batch saving together. No GUI / no AI service dependency."""
from __future__ import annotations
import re, random
from pathlib import Path
from dataclasses import dataclass, field

from .v2p import encode
from .generator import PresetGenerator
from .qc import QualityControl
from .classifier import classify_prompt
from ..model.rules import RuleModel
from ..model.params import WAVE_NAME, VMODE_NAME
from ..naming import make_name
from ..config import TEMPLATE_PATH


@dataclass
class Preset:
    name: str
    category: str
    subgenre: str
    params: dict
    qc_ok: bool
    qc_notes: list = field(default_factory=list)

    def summary(self) -> str:
        p = self.params
        return (f"{WAVE_NAME.get(p.get('o1wave', 0), '?')}  "
                f"cut {round(p.get('cutoff', 0) / 255 * 100)}%  res {round(p.get('reso', 0) / 255 * 100)}%  "
                f"OD {round(p.get('overdrive', 0) / 255 * 100)}%  "
                f"A/D/S/R {p.get('e1a', 0)}/{p.get('e1d', 0)}/{p.get('e1s', 0)}/{p.get('e1r', 0)}  "
                f"{VMODE_NAME.get(p.get('voiceMode', 0), '?')}")


class PresetService:
    def __init__(self):
        self.model = RuleModel()
        self.gen = PresetGenerator(self.model)
        self.qc = QualityControl(self.model)
        self._template = Path(TEMPLATE_PATH).read_bytes()

    # ---- generation ----
    def from_prompt(self, prompt: str, count: int = 1, seed: int | None = None) -> list[Preset]:
        cat, sub = classify_prompt(prompt)
        return self.batch(cat, sub, count, seed)

    def batch(self, category: str, subgenre: str, count: int, seed: int | None = None) -> list[Preset]:
        base = seed if seed is not None else random.randrange(1 << 30)
        out: list[Preset] = []
        attempts = 0
        while len(out) < count and attempts < count * 25 + 25:
            attempts += 1
            s = base + attempts * 101
            params = self.gen.generate(category, subgenre, seed=s)
            ok, notes = self.qc.check(category, params)
            if not ok:
                continue
            name = make_name(category, subgenre, len(out) + 1, random.Random(s))
            out.append(Preset(name, category, subgenre, params, ok, notes))
        return out

    def evolve(self, base: Preset, count: int = 4, amount: float = 0.3, seed: int | None = None) -> list[Preset]:
        base_seed = seed if seed is not None else random.randrange(1 << 30)
        out: list[Preset] = []
        attempts = 0
        while len(out) < count and attempts < count * 25 + 25:
            attempts += 1
            s = base_seed + attempts * 131
            params = self.gen.evolve(base.params, base.category, amount, seed=s)
            ok, notes = self.qc.check(base.category, params)
            if not ok:
                continue
            out.append(Preset(f"{base.name} var{len(out) + 1:02d}", base.category, base.subgenre, params, ok, notes))
        return out

    # ---- export ----
    def encode(self, preset: Preset) -> bytes:
        return encode(self._template, preset.params, preset.name)

    def save(self, presets: list[Preset], outdir: str | Path) -> list[Path]:
        outdir = Path(outdir)
        outdir.mkdir(parents=True, exist_ok=True)
        paths: list[Path] = []
        for ps in presets:
            fn = outdir / (self._safe(ps.name) + ".v2p")
            n = 1
            while fn.exists():
                fn = outdir / f"{self._safe(ps.name)} ({n}).v2p"
                n += 1
            fn.write_bytes(self.encode(ps))
            paths.append(fn)
        return paths

    @staticmethod
    def _safe(name: str) -> str:
        return re.sub(r'[<>:"/\\|?*]', "_", name).strip() or "preset"
