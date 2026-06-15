"""End-to-end engine test: prompt -> generate -> QC -> encode .v2p -> re-parse -> verify the
written sound params survive the round-trip and stay in the model's allowed ranges. No GUI."""
import sys, os
sys.path.insert(0, os.path.join(os.path.dirname(__file__), "..", "src"))

from vazpresetgen.engine import PresetService, parse
from vazpresetgen.config import CATEGORIES


def main():
    svc = PresetService()
    fails = 0
    total = 0

    # 1) prompt routing
    for prompt, exp in [("uplifting trance lead", "lead"), ("ASOT style pluck", "pluck"),
                        ("wide atmospheric pad", "pad"), ("rolling trance bass", "bass"),
                        ("hypersaw supersaw", "supersaw"), ("psy riser fx", "fx")]:
        ps = svc.from_prompt(prompt, count=1)
        assert ps and ps[0].category == exp, f"prompt '{prompt}' -> {ps[0].category if ps else None} != {exp}"
    print("[ok] prompt classification")

    # 2) generate a batch per category, QC-pass, encode + round-trip
    CHECK = {"cutoff": "cutoff", "reso": "resonance", "overdrive": "overdrive",
             "e1a": "amp_attack", "e1d": "amp_decay", "e1s": "amp_sustain", "e1r": "amp_release"}
    for cat in CATEGORIES:
        presets = svc.batch(cat, "uplifting", 8)
        assert len(presets) == 8, f"{cat}: only generated {len(presets)}/8 (QC too strict?)"
        allowed = svc.model.allowed(cat)
        for ps in presets:
            total += 1
            assert ps.qc_ok, f"{cat} '{ps.name}' failed QC: {ps.qc_notes}"
            data = svc.encode(ps)
            assert len(data) == 1565, f"{cat}: encoded size {len(data)} != 1565"
            back = parse(data)
            # written params must round-trip and stay in allowed ranges
            for fld, akey in CHECK.items():
                lo, hi = allowed[akey]
                got = back.get(fld, 0)
                if got > 0x7fffffff:
                    got -= 1 << 32
                assert ps.params[fld] == got, f"{cat}.{fld}: wrote {ps.params[fld]} read {got}"
                assert lo <= got <= hi, f"{cat}.{fld}={got} out of allowed [{lo},{hi}]"
            # the pro grid must be present: Env2->cutoff always; slot2 = keytrack (or LFO for FX)
            assert back.get("fcut1s") == 5, f"{cat}: missing Env2->cutoff"
            assert back.get("fcut2s") in (10, 1), f"{cat}: slot2 cutoff-mod is {back.get('fcut2s')} (want keytrack=10 or LFO=1)"
            assert back.get("fcut3s") == 17, f"{cat}: missing Velocity->cutoff"
        print(f"[ok] {cat:9s} 8 presets generated, QC-passed, round-trip exact, grid present")

    # 3) uniqueness — a batch should not be identical clones
    leads = svc.batch("lead", "asot", 10)
    sigs = {tuple(sorted(p.params.items())) for p in leads}
    assert len(sigs) >= 8, f"too many duplicate leads: {len(sigs)} unique / 10"
    print(f"[ok] variation: {len(sigs)}/10 unique leads")

    # 4) evolution
    var = svc.evolve(leads[0], count=5)
    assert len(var) == 5 and all(v.qc_ok for v in var)
    print("[ok] evolution: 5 valid variations")

    print(f"\nALL ENGINE TESTS PASSED ({total} presets round-tripped)")


if __name__ == "__main__":
    main()
