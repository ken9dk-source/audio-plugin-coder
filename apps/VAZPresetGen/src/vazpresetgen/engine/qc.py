"""Quality control — validates a generated parameter dict against the model's allowed ranges and
forbidden combinations BEFORE it is saved. Invalid presets are rejected (the service regenerates)."""
from __future__ import annotations
from ..model.rules import RuleModel

# allowed-range key in the model -> raw param field
_RANGE = {"cutoff": "cutoff", "resonance": "reso", "overdrive": "overdrive",
          "amp_attack": "e1a", "amp_decay": "e1d", "amp_sustain": "e1s", "amp_release": "e1r"}


class QualityControl:
    def __init__(self, model: RuleModel | None = None):
        self.model = model or RuleModel()

    def check(self, category: str, p: dict) -> tuple[bool, list[str]]:
        allowed = self.model.allowed(category)
        out: list[str] = []
        for akey, fld in _RANGE.items():
            lo, hi = allowed[akey]
            val = p.get(fld, 0)
            if not (lo <= val <= hi):
                out.append(f"{fld}={val} outside allowed [{lo},{hi}]")
        # forbidden combinations (from generation_rules.*.forbidden_combinations)
        if p.get("e1s", 0) == 0 and p.get("e1d", 0) < 120 and p.get("e1a", 0) < 4:
            out.append("forbidden: amp_sustain=0 + amp_decay<120 + attack~0 (click)")
        if p.get("fcut1s") == 5 and p.get("e2s", 0) == 0 and p.get("cutoff", 255) < 70:
            out.append("forbidden: filter-env->cutoff + filter_sustain=0 + cutoff<70 (silent)")
        if self.model.base_type(category) == "lead" and p.get("overdrive", 0) > 120:
            out.append("forbidden(lead): overdrive>120 (leads stay clean)")
        return (len(out) == 0, out)
