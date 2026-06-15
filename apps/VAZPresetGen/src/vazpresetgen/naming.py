"""Automatic, human-readable, collision-free preset names: '<Subgenre> <Flavour> <Category> NN'."""
from __future__ import annotations
import random

_SUB = {"asot": "ASOT", "uplifting": "Uplifting", "psy": "Psy",
        "progressive": "Prog", "classic": "Classic"}
_FLAVOUR = {
    "lead":     ["Anthem", "Sky", "Hands Up", "Euphoria", "Drive", "Solar", "Horizon", "Pulse"],
    "supersaw": ["Hypersaw", "Wide Stack", "Saw Wall", "Detune", "Mega Saw", "Unison"],
    "pad":      ["Atmos", "Glacier", "Aurora", "Warm", "Deep Space", "Choir", "Mist"],
    "pluck":    ["Gate", "Bounce", "Crystal", "Melodic", "Tick", "Glass", "Spark"],
    "bass":     ["Rolling", "Offbeat", "Sub Drive", "Reese", "Dark", "Engine"],
    "fx":       ["Riser", "Uplifter", "Sweep", "Noise Wash", "Impact", "Downfall"],
}
_CAT = {"lead": "Lead", "supersaw": "Supersaw", "pad": "Pad", "pluck": "Pluck", "bass": "Bass", "fx": "FX"}


def make_name(category: str, subgenre: str, index: int, rng: random.Random | None = None) -> str:
    rng = rng or random.Random(index)
    flavour = rng.choice(_FLAVOUR.get(category, ["Trance"]))
    return f"{_SUB.get(subgenre, 'Trance')} {flavour} {_CAT.get(category, 'Lead')} {index:02d}"
