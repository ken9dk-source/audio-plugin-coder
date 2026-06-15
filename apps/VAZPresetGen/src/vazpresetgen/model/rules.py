"""Loads the machine-readable rule model (data/trance_model.json — derived from a 190-preset
analysis) and exposes the per-type generation rules: allowed ranges, recommended ranges and
forbidden combinations. The model is the ONLY source of truth for value ranges."""
from __future__ import annotations
import json
from pathlib import Path
from ..config import MODEL_PATH

# Categories the GUI offers -> the rule type they draw their ranges from.
CATEGORY_BASE = {"lead": "lead", "supersaw": "lead", "pad": "pad",
                 "pluck": "pluck", "bass": "bass", "fx": "pad"}


class RuleModel:
    def __init__(self, path: str | Path = MODEL_PATH):
        self.path = Path(path)
        self.data = json.loads(self.path.read_text(encoding="utf-8"))
        self.gen = self.data["generation_rules"]
        self.stats = self.data.get("parameter_statistics", {})
        self.signatures = self.data.get("trance_signature_rules", {})

    def base_type(self, category: str) -> str:
        return CATEGORY_BASE.get(category, "lead")

    def rules(self, category: str) -> dict:
        return self.gen[self.base_type(category)]

    def allowed(self, category: str) -> dict:
        return self.rules(category)["allowed_ranges"]

    def recommended(self, category: str) -> dict:
        return self.rules(category)["recommended_ranges"]

    def forbidden(self, category: str) -> list:
        return self.rules(category)["forbidden_combinations"]
