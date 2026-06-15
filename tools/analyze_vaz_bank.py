#!/usr/bin/env python3
# Analyse a folder of VAZ .v2p presets via the clone's own (version-gated) reader, focusing on the
# MODULATION routing — what sources drive cutoff/res/pitch/PWM/amp, LFO rates/waves, filter modes —
# so we can copy the techniques into the trance presets.
import os, sys, struct, collections
import gen_trance_presets as g

SRC = {0:'None',1:'LFO1',2:'LFO2',3:'LFO3',4:'Env1',5:'Env2',6:'MA1',7:'MA2',8:'Lag',9:'Osc1',
       10:'Osc1Pit',11:'Osc2',12:'Noise',13:'Ext',14:'Accent',15:'SeqA',16:'SeqB',17:'Vel',
       18:'Press',19:'CtrlA',20:'CtrlB',21:'VoiceN'}
FILT = {0:'A-LP',1:'B-LP',2:'C2P+HP-RM',3:'C4P+HP-RM',4:'A-HP',5:'A-BP',6:'B-HP',7:'B-BP',8:'C4P+HP-SM',
        9:'C4P+HP-HM',10:'D-LP',11:'D-BP',12:'D-HP',13:'D-HP+LP',14:'C2P+HP-HM',15:'K-LP',16:'K-HP+LP',
        17:'R2P+HP-RM',18:'R2P+HP-HM',19:'R4P+HP-RM',20:'R4P+HP-HM',21:'Comb'}
def src(i): return SRC.get(i, f'?{i}')
def s8(v):  # signed interpretation of a ±255 mod amount (stored as small u32 / two's-comp)
    return v-(1<<32) if v > 0x7fffffff else v

def analyze(path):
    d = open(path, 'rb').read()
    prst = g.find_tag(d, 'PRST')
    if prst < 0: return None
    try: ver, off, V = g.trace(d, prst)
    except Exception: return None
    return ver, V

def fmt_mods(V):
    """Compact list of active cutoff/res/amp/fm/pwm modulations."""
    out = []
    for label, ssrc, samt in [('cut1','fcut1s','fcut1d'),('cut2','fcut2s','fcut2d'),('cut3','fcut3s','fcut3d'),
                              ('res','fresS','fresD'),('amp','am1s','am1d'),('amp2','am2s','am2d'),
                              ('pan','am3s','am3d'),('o1fm','o1fm1s','o1fm1d'),('o1pwm','o1pwms','o1pwmd'),
                              ('o2fm','o2fm1s','o2fm1d'),('o2pwm','o2pwms','o2pwmd'),
                              ('e2seg','e2modsrc','e2modamt')]:
        s = V.get(ssrc, 0); a = s8(V.get(samt, 0))
        if s and a: out.append(f"{label}={src(s)}:{a:+d}")
    return " ".join(out) if out else "(no mod)"

if __name__ == '__main__':
    root = sys.argv[1] if len(sys.argv) > 1 else r'C:\Users\ken98\Desktop\Vaz sound banks'
    files = []
    for r, _, fs in os.walk(root):
        for f in fs:
            if f.lower().endswith('.v2p'): files.append(os.path.join(r, f))
    files.sort()

    # ---- aggregate stats ----
    cutmod = collections.Counter()      # source used on ANY cutoff slot
    lfo_on_cut = 0; env_on_cut = 0; n = 0
    filt = collections.Counter()
    lfo1rates = []; lfo2rates = []
    pwm_lfo = 0; pitch_lfo = 0
    per = []
    for f in files:
        r = analyze(f)
        if not r: continue
        ver, V = r; n += 1
        cuts = [V.get('fcut1s',0), V.get('fcut2s',0), V.get('fcut3s',0)]
        camt = [V.get('fcut1d',0), V.get('fcut2d',0), V.get('fcut3d',0)]
        active = [s for s, a in zip(cuts, camt) if s and a]
        for s in active: cutmod[src(s)] += 1
        if any(s in (1,2,3) for s in active): lfo_on_cut += 1
        if any(s in (4,5) for s in active):   env_on_cut += 1
        filt[FILT.get(V.get('filterMode',0),'?')] += 1
        if V.get('fcut1d',0) or V.get('fcut2d',0):  # only count LFO-driven rates as "used"
            if any(s in (1,2,3) for s in active): lfo1rates.append(V.get('lfo1rate',0))
        if V.get('o1pwms') in (1,2,3) and V.get('o1pwmd'): pwm_lfo += 1
        if V.get('o1fm1s') in (1,2,3) and V.get('o1fm1d'): pitch_lfo += 1
        per.append((os.path.relpath(f, root), ver, V))

    print(f"=== {n} presets analysed in {root} ===\n")
    print("CUTOFF-mod source usage (how the filter is moved):")
    for s, c in cutmod.most_common(): print(f"   {s:8s} {c}")
    print(f"   -> {lfo_on_cut} presets move cutoff with an LFO, {env_on_cut} with an envelope")
    print(f"   -> {pwm_lfo} use LFO->PWM, {pitch_lfo} use LFO->pitch/FM")
    print("\nFilter modes used:")
    for s, c in filt.most_common(8): print(f"   {s:12s} {c}")
    if lfo1rates:
        lfo1rates.sort()
        print(f"\nLFO1 rate (0-255) on cutoff-LFO patches: min={lfo1rates[0]} med={lfo1rates[len(lfo1rates)//2]} max={lfo1rates[-1]}")

    # ---- detailed dump of trance-relevant presets ----
    KW = ('bass','lead','pluck','trance','trancy','stab','saw','super','sub','acid','gate','arp','hoover','rave')
    print("\n=== trance-relevant presets (mod routing) ===")
    shown = 0
    for rel, ver, V in per:
        nm = os.path.basename(rel).lower()
        if not any(k in nm for k in KW): continue
        shown += 1
        if shown > 40: break
        o1w = V.get('o1wave',0); fm = FILT.get(V.get('filterMode',0),'?')
        print(f"\n* {rel}  (v{ver})")
        print(f"    osc1 wave={o1w} shape={V.get('o1shape',0)}  osc2 wave={V.get('o2wave',0)} lvl={V.get('o2level',0)}  vmode={V.get('voiceMode',0)} od={V.get('overdrive',0)}")
        print(f"    filter={fm} cut={V.get('cutoff',0)} res={V.get('reso',0)}  LFO1 rate={V.get('lfo1rate',0)} wave={V.get('lfo1wave',0)}  LFO2 rate={V.get('lfo2rate',0)}")
        print(f"    mods: {fmt_mods(V)}")
    print(f"\n(showed {shown} trance-relevant of {n})")
