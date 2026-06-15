"""PresetGenerator — turns (category, subgenre) into a complete raw-VAZ parameter dict that obeys
the rule model. Values for cutoff / resonance / overdrive / amp+filter ADSR are SAMPLED from the
model's recommended ranges (so they are always in-range and realistic); on top of that every preset
gets the "pro grid" found on the bank's playable patches (filter env -> cutoff sweep, Osc1-Pitch ->
cutoff key-tracking, velocity -> cutoff & amp) plus per-category character (waves, voices, detune,
LFO -> waveshape animation, drive). Variation comes from a per-call RNG seed."""
from __future__ import annotations
import random
from ..model.rules import RuleModel
from ..model.params import WAVE, VMODE, SRC, NEUTRAL_TUNE, cents

# Always-on modulation grid (identical to the validated trance generator).
GRID = dict(
    fcut2s=SRC["keytrack"], fcut2d=70,     # Osc1 Pitch -> cutoff : filter follows the keyboard
    fcut3s=SRC["velocity"], fcut3d=30,     # Velocity   -> cutoff
    am2s=SRC["velocity"],   am2d=42,       # Velocity   -> amp
    fcut1s=SRC["env2"],                    # Env2 (filter env) -> cutoff (amount per category)
    o1pwms=SRC["lfo1"], o2pwms=SRC["lfo2"],  # LFO -> waveshape (amount per category)
)
# Fields zeroed so each preset is fully determined (neutralise template residue).
ZERO = dict(noise=0, o1level=255, bandwidth=0, hpCut=0, o1fm1d=0, o1fm2d=0, o2fm1d=0, o2fm2d=0,
            am1d=0, am3d=0, e2modamt=0, ma1amamt=0, fresS=0, fresD=0, fcut3s_unused=0,
            o1tune=NEUTRAL_TUNE, o2tune=NEUTRAL_TUNE)

# Per-category character. base = which rule type supplies the sampled ranges.
PROFILES = {
    "lead": dict(base="lead", waves=["saw", "saw", "pulse"], vmodes=["poly", "poly", "unison"],
                 o1shape=(20, 120), o2detune=(4, 16), uni=(30, 90), poly=(15, 45),
                 fenv=(45, 90), pwm=(55, 95), lfo1=(34, 45), lfo2=(24, 32),
                 filt=[19, 9, 15], porta=(0, 20)),
    "supersaw": dict(base="lead", waves=["multisaw", "saw", "multisaw"], vmodes=["unison", "poly", "unison"],
                     o1shape=(150, 230), o2detune=(2, 18), uni=(60, 120), poly=(20, 50),
                     fenv=(30, 60), pwm=(70, 110), lfo1=(30, 42), lfo2=(22, 30),
                     filt=[19, 9], porta=(0, 0), bright=True),
    "pad": dict(base="pad", waves=["saw", "saw", "pulse"], vmodes=["poly", "unison"],
                o1shape=(0, 130), o2detune=(8, 22), uni=(60, 120), poly=(30, 60),
                fenv=(25, 55), pwm=(70, 110), lfo1=(22, 32), lfo2=(18, 26),
                filt=[10, 19], porta=(0, 0)),
    "pluck": dict(base="pluck", waves=["pulse", "pulse", "saw"], vmodes=["poly"],
                  o1shape=(0, 70), o2detune=(4, 10), uni=(0, 0), poly=(0, 0),
                  fenv=(90, 150), pwm=(0, 25), lfo1=(0, 18), lfo2=(0, 0),
                  filt=[15, 19, 0], porta=(0, 0), sustain_floor=10),
    "bass": dict(base="bass", waves=["pulse", "pulse", "saw"], vmodes=["mono", "mono", "unison"],
                 o1shape=(0, 130), o2detune=(2, 6), uni=(0, 20), poly=(0, 0),
                 fenv=(100, 140), pwm=(0, 0), lfo1=(0, 0), lfo2=(0, 0),
                 filt=[19, 16, 15], porta=(0, 15), octave_down=True, sustain_floor=20),
    "fx": dict(base="pad", waves=["saw", "pulse"], vmodes=["poly", "unison"],
               o1shape=(0, 255), o2detune=(20, 120), uni=(80, 128), poly=(40, 80),
               fenv=(120, 200), pwm=(110, 200), lfo1=(60, 140), lfo2=(60, 140),
               filt=[19, 10, 21], porta=(0, 40), fx_lfo_cutoff=True),
}
# subgenre nudges (small, within range) — multiplicative on a few feels.
SUBGENRE = {
    "asot":        dict(cutoff=1.05, pwm=1.1, drive=0.8),
    "uplifting":   dict(cutoff=1.0,  pwm=1.0, drive=0.9),
    "psy":         dict(cutoff=1.08, pwm=1.0, drive=1.25, atk=0.5),
    "progressive": dict(cutoff=0.92, pwm=1.1, drive=0.7,  rel=1.2),
    "classic":     dict(cutoff=1.0,  pwm=0.85, drive=1.0),
}


def _ri(rng: random.Random, lo_hi) -> int:
    lo, hi = lo_hi
    return int(round(rng.uniform(lo, hi))) if hi > lo else int(lo)


def _clampi(v, lo, hi):
    return int(max(lo, min(hi, v)))


class PresetGenerator:
    def __init__(self, model: RuleModel | None = None):
        self.model = model or RuleModel()

    def generate(self, category: str, subgenre: str = "uplifting", seed: int | None = None) -> dict:
        rng = random.Random(seed)
        prof = PROFILES.get(category, PROFILES["lead"])
        rec = self.model.recommended(category)
        nudge = SUBGENRE.get(subgenre, SUBGENRE["uplifting"])

        wave = WAVE[rng.choice(prof["waves"])]
        vmode = VMODE[rng.choice(prof["vmodes"])]
        cut = _clampi(_ri(rng, rec["cutoff"]) * nudge["cutoff"], *self.model.allowed(category)["cutoff"])
        res = _ri(rng, rec["resonance"])
        od = _clampi(_ri(rng, rec["overdrive"]) * nudge["drive"], *self.model.allowed(category)["overdrive"])
        atk = _clampi(_ri(rng, rec["amp_attack"]) * nudge.get("atk", 1.0), 0, 425)
        dec = _ri(rng, rec["amp_decay"])
        sus = _ri(rng, rec["amp_sustain"])
        rel = _clampi(_ri(rng, rec["amp_release"]) * nudge.get("rel", 1.0), 0, 425)
        sus = max(sus, prof.get("sustain_floor", 0))     # keep held notes tonal (anti-click)

        p = dict(ZERO)
        p.update(GRID)
        p.update(
            o1wave=wave, o2wave=wave if category != "pluck" else WAVE["pulse"],
            o1shape=_ri(rng, prof["o1shape"]), o2shape=_ri(rng, prof["o1shape"]),
            o2level=_ri(rng, [120, 255]), o2tune=cents(fine=_ri(rng, prof["o2detune"])),
            filterMode=rng.choice(prof["filt"]), cutoff=cut, reso=res, overdrive=od,
            e1a=atk, e1d=dec, e1s=sus, e1r=rel,
            e2a=_ri(rng, [0, 6]), e2d=_ri(rng, rec.get("filter_env_decay", [90, 160])),
            e2s=_ri(rng, [120, 180]) if category not in ("pluck", "bass") else _ri(rng, [0, 0]) if category == "pluck" else _ri(rng, [120, 160]),
            e2r=_ri(rng, [40, 90]),
            fcut1d=int(_ri(rng, prof["fenv"])),
            o1pwmd=int(_ri(rng, prof["pwm"]) * nudge["pwm"]), o2pwmd=int(_ri(rng, prof["pwm"]) * nudge["pwm"]),
            lfo1rate=_ri(rng, prof["lfo1"]), lfo2rate=_ri(rng, prof["lfo2"]),
            voiceMode=vmode, uniDetune=_ri(rng, prof["uni"]), polyDetune=_ri(rng, prof["poly"]),
            portamento=_ri(rng, prof["porta"]),
        )
        if prof.get("octave_down") and rng.random() < 0.5:
            p["o1tune"] = cents(octaves=-1)
        if prof.get("fx_lfo_cutoff"):                    # FX: heavy LFO -> cutoff sweep movement
            p.update(fcut2s=SRC["lfo1"], fcut2d=_ri(rng, [80, 160]))
        # filter-env sustain handling for pluck (filter closes for the tick)
        if category == "pluck":
            p["e2s"] = 0
        p.pop("fcut3s_unused", None)
        return p

    def evolve(self, base_params: dict, category: str, amount: float = 0.3, seed: int | None = None) -> dict:
        """Return a variation of base_params: jitter the continuous sound params by +-amount of their
        allowed span, keep them in range, occasionally re-roll wave / voice mode. Structural grid kept."""
        rng = random.Random(seed)
        allowed = self.model.allowed(category)
        p = dict(base_params)
        span = {"cutoff": allowed["cutoff"], "resonance": allowed["resonance"], "overdrive": allowed["overdrive"],
                "e1a": allowed["amp_attack"], "e1d": allowed["amp_decay"], "e1s": allowed["amp_sustain"],
                "e1r": allowed["amp_release"]}
        key_map = {"cutoff": "cutoff", "reso": "resonance", "overdrive": "overdrive",
                   "e1a": "e1a", "e1d": "e1d", "e1s": "e1s", "e1r": "e1r"}
        for fld, rkey in key_map.items():
            lo, hi = span[rkey]
            jitter = (hi - lo) * amount * (rng.random() * 2 - 1)
            p[fld] = _clampi(p.get(fld, lo) + jitter, lo, hi)
        # occasionally vary timbre params
        prof = PROFILES.get(category, PROFILES["lead"])
        if rng.random() < 0.5:
            p["o1shape"] = _ri(rng, prof["o1shape"])
        if rng.random() < 0.35:
            p["o1wave"] = WAVE[rng.choice(prof["waves"])]; p["o2wave"] = p["o1wave"] if category != "pluck" else WAVE["pulse"]
        if rng.random() < 0.4:
            p["o1pwmd"] = _ri(rng, prof["pwm"]); p["o2pwmd"] = p["o1pwmd"]
        p["e1s"] = max(p["e1s"], prof.get("sustain_floor", 0))
        return p
