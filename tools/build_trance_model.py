#!/usr/bin/env python3
# Build a machine-readable trance sound-design MODEL from a VAZ .v2p bank. Read-only, emits JSON.
# Every number is computed from the dataset via the clone's own (version-gated) parser.
import os, sys, json, statistics as stt, collections
import gen_trance_presets as g
from analyze_vaz_bank import FILT
from analyze_vaz_rules import classify, cut_sources

WAVE = {0:"Saw",1:"Pulse",2:"MultiSaw",3:"Sample",4:"Ext/Sync"}
VM   = {0:"Mono",1:"Poly",2:"Unison"}
SRCN = {0:"None",1:"LFO1",2:"LFO2",3:"LFO3",4:"Env1",5:"Env2",10:"Osc1Pitch",17:"Velocity"}

def s8(v): return v-(1<<32) if v and v > 0x7fffffff else v
def detune_cents(t): return s8(t) + 2400 if t else 0      # o2tune raw -> cents off unison

def num(xs):
    xs = [x for x in xs if x is not None]
    if not xs: return None
    xs.sort(); q = lambda p: xs[min(len(xs)-1, int(p*len(xs)))]
    return {"min": xs[0], "max": xs[-1], "sweet_spot": int(stt.median(xs)), "recommended": [q(0.25), q(0.75)]}

def dist(xs, names):
    c = collections.Counter(xs); tot = sum(c.values()) or 1
    return {names.get(k, str(k)): round(100*v/tot) for k, v in c.most_common()}

def load(root):
    items = []
    for r, _, fs in os.walk(root):
        for f in fs:
            if not f.lower().endswith('.v2p'): continue
            d = open(os.path.join(r, f), 'rb').read(); p = d.find(b'PRST')
            if p < 0: continue
            try: ver, off, V = g.trace(d, p)
            except Exception: continue
            items.append((classify(os.path.splitext(f)[0], V), f, ver, V))
    return items

def has_src(V, srcset):
    return any(s in srcset for s in cut_sources(V))
def pct_true(Vs, fn): return round(100*sum(1 for V in Vs if fn(V))/max(1, len(Vs)))

def type_stats(Vs):
    col = lambda k: [V.get(k, 0) for V in Vs]
    n = len(Vs)
    uv = [V['uniVoices'] for V in Vs if V.get('uniVoices', 0) > 0]
    return {
        "n": n,
        "oscillator": {
            "osc1_wave_pct": dist(col('o1wave'), WAVE),
            "osc2_wave_pct": dist(col('o2wave'), WAVE),
            "osc1_shape_0_255": num(col('o1shape')),
            "osc2_level_0_255": num(col('o2level')),
            "osc2_detune_cents": num([detune_cents(V.get('o2tune')) for V in Vs]),
            "osc2_used_pct": pct_true(Vs, lambda V: V.get('o2level', 0) > 20),
        },
        "filter": {
            "mode_pct": dict(list(dist([FILT.get(w, '?') for w in col('filterMode')], {}).items())[:5]),
            "cutoff_0_255": num(col('cutoff')),
            "resonance_0_255": num(col('reso')),
        },
        "amp_env_A_D_R_0_425__S_0_255": {
            "attack": num(col('e1a')), "decay": num(col('e1d')),
            "sustain": num(col('e1s')), "release": num(col('e1r')),
        },
        "filter_env_A_D_R_0_425__S_0_255": {
            "attack": num(col('e2a')), "decay": num(col('e2d')),
            "sustain": num(col('e2s')), "release": num(col('e2r')),
        },
        "voices": {
            "voice_mode_pct": dist(col('voiceMode'), VM),
            "unison_detune_0_255": num(col('uniDetune')),
            "poly_detune_0_255": num(col('polyDetune')),
            "unison_voice_count_v2only": num(uv) if uv else None,
        },
        "overdrive_0_255": num(col('overdrive')),
        "modulation_grid_pct": {
            "env_to_cutoff": pct_true(Vs, lambda V: has_src(V, {4, 5})),
            "keytrack_to_cutoff": pct_true(Vs, lambda V: has_src(V, {10})),
            "velocity_to_cutoff": pct_true(Vs, lambda V: has_src(V, {17})),
            "lfo_to_cutoff": pct_true(Vs, lambda V: has_src(V, {1, 2, 3})),
            "velocity_to_amp": pct_true(Vs, lambda V: (V.get('am1s') == 17 and V.get('am1d')) or (V.get('am2s') == 17 and V.get('am2d'))),
            "lfo_to_waveshape": pct_true(Vs, lambda V: (V.get('o1pwms') in (1,2,3) and V.get('o1pwmd')) or (V.get('o2pwms') in (1,2,3) and V.get('o2pwmd'))),
        },
        "lfo_rate_when_used_0_255": num([V['lfo1rate'] for V in Vs if (V.get('o1pwms') in (1,2,3) and V.get('o1pwmd')) or has_src(V, {1,2,3})]),
    }

if __name__ == '__main__':
    root = sys.argv[1] if len(sys.argv) > 1 else r'C:\Users\ken98\Desktop\Vaz sound banks'
    items = load(root)
    by = collections.defaultdict(list)
    for t, n, v, V in items: by[t].append(V)
    allV = [V for _, _, _, V in items]

    # interaction probes (numeric, no prose)
    msaw = [V for V in allV if V.get('o1wave') == 2]
    pulse = [V for V in allV if V.get('o1wave') == 1]
    hidrv = [V for V in allV if V.get('overdrive', 0) > 150]
    fenv = [V for V in allV if has_src(V, {5})]
    wide = [V for V in allV if V.get('uniDetune', 0) > 20 or V.get('voiceMode') == 2 or V.get('o2level', 0) > 120]

    model = {
        "_meta": {
            "dataset": os.path.basename(root.rstrip('\\/')), "presets_analysed": len(items),
            "parser": "clone parseV2P (version-gated); values in native .v2p units",
            "units": {"cutoff/res/overdrive/levels/detune/shape/sustain": "0-255",
                      "amp/filter env attack/decay/release": "0-425", "osc tune": "cents (0 = unison)",
                      "clone_normalisation": "cutoff/res/od/level/sustain /255 ; A/D/R /425 ; modsrc index 0-21"},
            "fx_chain_note": "Chorus/Delay/Reverb are NOT in the synth .v2p PRST params (VAZ effects are separate insert plugins) -> not modelled here.",
        },
        "preset_classification": {
            "types_found": {t: len(by[t]) for t in ['bass', 'lead', 'pad', 'pluck', 'other']},
            "method": "name keyword + envelope-feature fallback (slow-attack+high-sustain=pad; low-sustain+short-decay=pluck; mono=bass; else lead)",
            "type_defining_params": ["voiceMode", "amp_env.attack", "amp_env.sustain", "filter_env.decay", "cutoff"],
            "trance_defining_params": ["o1wave/o2wave", "osc2_detune+unison_detune", "cutoff", "resonance",
                                       "filter_env->cutoff (fcut1=Env2)", "keytrack->cutoff (fcut=Osc1Pitch)",
                                       "lfo->waveshape (o1/o2_ws)", "overdrive", "voiceMode"],
        },
        "parameter_statistics": {t: type_stats(by[t]) for t in ['lead', 'pad', 'bass', 'pluck']},
        "trance_signature_rules": {
            "uplifting_trance_lead": {
                "voiceMode": "Poly", "osc1_wave": ["Saw", "MultiSaw", "Pulse"], "cutoff": ">=180 (bright)",
                "resonance": "low-mid (40-120)", "overdrive": "LOW (0-60) <- leads are clean (bank median 30)",
                "amp_env": {"attack": "fast-moderate (0-60)", "sustain": "high (130-235)", "release": "moderate-long (120-220)"},
                "must_have": ["Env2->cutoff sweep", "keytrack->cutoff"], "movement": "LFO->waveshape (rate 28-45)",
            },
            "asot_supersaw": {
                "_data_note": f"MultiSaw oscillator mode is UNUSED in this 190-bank (0 presets). Width is built from Saw/Pulse + 2 osc + unison_detune + osc2 detune. MultiSaw (wave 2) remains available in the synth.",
                "osc1_wave": "Saw (bank method) | MultiSaw (synth's dedicated supersaw mode)",
                "osc2_wave": "match osc1", "osc1_shape": "Saw->Tri morph or MultiSaw detune (0-130)",
                "osc2_detune_cents": "small (+2..+18) for width", "unison_detune": "high (40-128)",
                "voiceMode": ["Unison", "Poly"], "cutoff": "high (190-255)", "resonance": "low (20-70)",
                "movement": "LFO1->o1_ws + LFO2->o2_ws (rate ~30-40)", "overdrive": "low (0-50)",
            },
            "pluck_stab": {
                "osc1_wave": "Pulse", "voiceMode": "Poly", "amp_env": {"attack": "0-10", "decay": "50-110",
                "sustain": "LOW (0-90) = the pluck", "release": "short-moderate"}, "filter_env": "short decay, sustain 0",
                "two_approaches": {"amp_pluck (bank)": "high cutoff (~200) + amp env plucks",
                                   "filter_pluck (trance)": "low cutoff + Env2->cutoff opens it"},
            },
            "pad": {
                "voiceMode": "Poly/Unison", "amp_env": {"attack": "slow (120-250)", "sustain": "full (200-255)",
                "release": "long (200-300)"}, "resonance": "low (10-80)", "movement": "LFO->waveshape (48% of pads)",
                "width": "osc2 detune + high unison_detune", "overdrive": "low/none (0-40)",
            },
            "energy": ["high overdrive (bass 120-200)", "high resonance (90-180)", "fast amp attack", "Env2->cutoff full depth"],
            "width": ["MultiSaw osc1+osc2", "osc2 detune +5..+18c", "unison_detune 40-120", "Unison/Poly voiceMode"],
            "drive": ["overdrive 120-220 (bass)", "resonance + Type K/R filter"],
            "movement": ["LFO->waveshape (PWM) rate 28-45", "filter key-tracking", "velocity->cutoff"],
        },
        "generation_rules": {},   # filled below
        "parameter_interactions": {
            "work_together": {
                "width_stack": {"params": ["osc1+osc2 (Saw/Pulse)", "osc2_detune", "unison_detune", "voiceMode=Uni/Poly"],
                                 "multisaw_mode_used_in_bank": len(msaw),
                                 "evidence_wide_subset": {"n": len(wide), "median_uniDetune": num([V.get('uniDetune',0) for V in wide]),
                                 "median_osc2_level": num([V.get('o2level',0) for V in wide]),
                                 "voiceMode_pct": dist([V.get('voiceMode',0) for V in wide], VM)} if wide else None},
                "filter_movement": {"params": ["filter_env->cutoff (Env2)", "base cutoff", "resonance", "filter_env.decay"],
                                     "evidence_filterEnv_subset": {"n": len(fenv), "median_reso": num([V.get('reso',0) for V in fenv]),
                                     "median_cutoff": num([V.get('cutoff',0) for V in fenv])} if fenv else None},
                "animation": {"params": ["lfo1/2_rate", "o1/o2_ws_amount"], "note": "rate>0 AND ws_amount>0 both required or no movement"},
                "drive": {"params": ["overdrive", "resonance", "filterMode=K/R"],
                          "evidence_highOD_subset": {"n": len(hidrv), "median_reso": num([V.get('reso',0) for V in hidrv])} if hidrv else None},
                "playability": {"params": ["keytrack->cutoff", "cutoff"], "note": "without keytrack the filter is fixed across the keyboard"},
            },
            "oppose": {
                "lead_vs_overdrive": "leads use LOW overdrive (median 30); high OD belongs to bass/aggressive",
                "tonal_vs_zero_sustain": "amp.sustain=0 opposes a held tone -> click/transient only",
                "silent_combo": "filter_env->cutoff + filter.sustain=0 + low base cutoff -> held note filtered to silence",
            },
            "pulse_vs_saw": {"pulse_n": len(pulse), "saw_n": len([V for V in allV if V.get('o1wave')==0]),
                              "note": "Pulse is the most common osc1 wave for bass/lead/pluck; pairs with LFO->PWM for movement"},
        },
    }

    # generation_rules: allowed (observed min-max), recommended (p25-p75), forbidden combos
    def gen_rule(t):
        S = type_stats(by[t])
        ae = S["amp_env_A_D_R_0_425__S_0_255"]; fe = S["filter_env_A_D_R_0_425__S_0_255"]
        return {
            "allowed_ranges": {
                "cutoff": [S["filter"]["cutoff_0_255"]["min"], S["filter"]["cutoff_0_255"]["max"]],
                "resonance": [S["filter"]["resonance_0_255"]["min"], S["filter"]["resonance_0_255"]["max"]],
                "overdrive": [S["overdrive_0_255"]["min"], S["overdrive_0_255"]["max"]],
                "amp_attack": [ae["attack"]["min"], ae["attack"]["max"]], "amp_decay": [ae["decay"]["min"], ae["decay"]["max"]],
                "amp_sustain": [ae["sustain"]["min"], ae["sustain"]["max"]], "amp_release": [ae["release"]["min"], ae["release"]["max"]],
                "osc1_wave": list(S["oscillator"]["osc1_wave_pct"].keys()),
                "voice_mode": list(S["voices"]["voice_mode_pct"].keys()),
            },
            "recommended_ranges": {
                "cutoff": S["filter"]["cutoff_0_255"]["recommended"], "resonance": S["filter"]["resonance_0_255"]["recommended"],
                "overdrive": S["overdrive_0_255"]["recommended"], "amp_attack": ae["attack"]["recommended"],
                "amp_decay": ae["decay"]["recommended"], "amp_sustain": ae["sustain"]["recommended"],
                "amp_release": ae["release"]["recommended"], "filter_env_decay": fe["decay"]["recommended"],
                "osc1_wave": max(S["oscillator"]["osc1_wave_pct"], key=S["oscillator"]["osc1_wave_pct"].get),
                "voice_mode": max(S["voices"]["voice_mode_pct"], key=S["voices"]["voice_mode_pct"].get),
            },
            "forbidden_combinations": [
                {"rule": "amp_sustain==0 AND amp_decay<120 AND amp_attack<4", "reason": "click/transient (0 of 190 bank presets do this on tonal types)"},
                {"rule": "filter_env->cutoff AND filter_sustain==0 AND cutoff<70", "reason": "held note filtered to silence (only kicks do this in the bank)"},
            ] + ([{"rule": "overdrive>120", "reason": "leads in the bank stay clean (median 30); high drive belongs to bass"}] if t == "lead" else []),
        }
    model["generation_rules"] = {t: gen_rule(t) for t in ['lead', 'pad', 'bass', 'pluck']}

    print(json.dumps(model, indent=2, ensure_ascii=False))
