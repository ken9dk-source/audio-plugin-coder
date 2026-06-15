"""Map a free-text prompt ("uplifting trance lead", "ASOT style pluck", "rolling bass") to a
(category, subgenre) pair. Keyword based — deterministic, no AI service required at runtime."""
from __future__ import annotations
import re

_CATEGORY_KW = [
    ("supersaw", ("supersaw", "super saw", "hypersaw", "hyper saw", "saw stack", "trance gate saw")),
    ("pluck",    ("pluck", "stab", "gate", "gated", "arp")),
    ("pad",      ("pad", "atmos", "atmospheric", "string", "warm", "ambient", "drone", "wash")),
    ("bass",     ("bass", "sub", "rolling", "offbeat", "reese", "donk")),
    ("fx",       ("fx", "riser", "sweep", "uplifter", "downlifter", "impact", "noise", "effect", "atmosphere")),
    ("lead",     ("lead", "anthem", "melody", "solo", "hoover", "acid")),
]
_SUBGENRE_KW = [
    ("asot",        ("asot", "armin", "a state of trance")),
    ("uplifting",   ("uplifting", "uplift", "euphoric", "festival", "anthem")),
    ("psy",         ("psy", "psytrance", "goa", "full-on", "full on")),
    ("progressive", ("progressive", "prog", "deep", "melodic")),
    ("classic",     ("classic", "2004", "2005", "2006", "2008", "2010", "old school", "oldschool")),
]


def classify_prompt(prompt: str) -> tuple[str, str]:
    p = (prompt or "").lower()
    category = "lead"
    for cat, kws in _CATEGORY_KW:
        if any(k in p for k in kws):
            category = cat
            break
    subgenre = "uplifting"
    for sg, kws in _SUBGENRE_KW:
        if any(k in p for k in kws):
            subgenre = sg
            break
    return category, subgenre
