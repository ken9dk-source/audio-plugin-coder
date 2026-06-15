#!/usr/bin/env python3
# Statistical analysis of a VAZ .v2p bank: classify by type (lead/pad/bass/pluck), derive per-type
# value ranges + modulation rules, and flag anti-patterns. Read-only — no presets are written.
import os, sys, statistics as st, collections
import gen_trance_presets as g
from analyze_vaz_bank import SRC, FILT, src, s8

def pct(xs, p):
    xs = sorted(xs);
    if not xs: return 0
    return xs[min(len(xs)-1, int(p*len(xs)))]
def rng(xs):
    xs = [x for x in xs if x is not None]
    if not xs: return "-"
    return f"{min(xs):>3}/{int(st.median(xs)):>3}/{max(xs):>3}"   # min / median / max

def classify(name, V):
    n = name.lower()
    kw = [('bass',('bass','sub ','subby','808','reese','donk','wobble')),
          ('pluck',('pluck','stab','clav','mallet','bell','plink','blip','harp')),
          ('pad',('pad','string','strings','atmos','choir','warm','drone','glacial','ice','wash','air','ambient','soft')),
          ('lead',('lead','saw','super','hoover','acid','solo','sync','trance','trancy'))]
    other = ('kick','hat','snare','clap','drum','perc','hit','tom','rim','fx','noise','sweep','riser',
             'organ','brass','guitar','vox','voice','formant','whistl','sfx','zap','laser','wind','rain')
    for t, ks in kw:
        if any(k in n for k in ks): return t
    if any(k in n for k in other): return 'other'
    # feature fallback
    e1a, e1s, e1d, vm = V.get('e1a',0), V.get('e1s',0), V.get('e1d',0), V.get('voiceMode',0)
    if e1a > 100 and e1s > 180: return 'pad'
    if e1s < 70 and e1d < 130:  return 'pluck'
    if vm == 0:                 return 'bass'
    return 'lead'

def cut_sources(V):
    out = []
    for s,a in [('fcut1s','fcut1d'),('fcut2s','fcut2d'),('fcut3s','fcut3d')]:
        if V.get(s) and V.get(a): out.append(V[s])
    return out

if __name__ == '__main__':
    root = sys.argv[1] if len(sys.argv) > 1 else r'C:\Users\ken98\Desktop\Vaz sound banks'
    items = []   # (type, name, ver, V)
    for r,_,fs in os.walk(root):
        for f in fs:
            if not f.lower().endswith('.v2p'): continue
            d = open(os.path.join(r,f),'rb').read(); p = d.find(b'PRST')
            if p < 0: continue
            try: ver, off, V = g.trace(d, p)
            except: continue
            t = classify(os.path.splitext(f)[0], V)
            items.append((t, f, ver, V))

    bytype = collections.defaultdict(list)
    for t,n,v,V in items: bytype[t].append(V)
    print(f"=== {len(items)} presets ===")
    print("Type counts:", {t: len(bytype[t]) for t in ['bass','lead','pad','pluck','other']})

    for T in ['bass','lead','pad','pluck']:
        Vs = bytype[T]
        if not Vs: continue
        n = len(Vs)
        def col(k): return [V.get(k,0) for V in Vs]
        # categorical distributions
        waves = collections.Counter(V.get('o1wave',0) for V in Vs)
        filts = collections.Counter(FILT.get(V.get('filterMode',0),'?') for V in Vs)
        vmode = collections.Counter(['Mono','Poly','Uni'][min(2,V.get('voiceMode',0))] for V in Vs)
        # modulation prevalence
        kt  = sum(1 for V in Vs if 10 in cut_sources(V))                      # keytrack on cutoff
        vel = sum(1 for V in Vs if 17 in cut_sources(V))                      # velocity on cutoff
        env = sum(1 for V in Vs if any(s in (4,5) for s in cut_sources(V)))   # env on cutoff
        lfoC= sum(1 for V in Vs if any(s in (1,2,3) for s in cut_sources(V))) # LFO on cutoff
        velA= sum(1 for V in Vs if V.get('am1s')==17 and V.get('am1d') or V.get('am2s')==17 and V.get('am2d'))
        pwm = sum(1 for V in Vs if (V.get('o1pwms') in (1,2,3) and V.get('o1pwmd')) or (V.get('o2pwms') in (1,2,3) and V.get('o2pwmd')))
        o2on= sum(1 for V in Vs if V.get('o2level',0) > 20)
        print(f"\n========== {T.upper()}  (n={n}) ==========")
        print(f"  osc1 wave:  {dict(waves.most_common())}   (0=Saw 1=Pulse 2=MultiSaw 3=Sample 4=Ext/Sync)")
        print(f"  filter:     {dict(filts.most_common(4))}")
        print(f"  voiceMode:  {dict(vmode)}     osc2 used: {o2on}/{n}")
        print(f"  cutoff   (min/med/max): {rng(col('cutoff'))}     reso: {rng(col('reso'))}     overdrive: {rng(col('overdrive'))}")
        print(f"  AMP  A/D/S/R:  A={rng(col('e1a'))}  D={rng(col('e1d'))}  S={rng(col('e1s'))}  R={rng(col('e1r'))}")
        print(f"  FILT A/D/S/R:  A={rng(col('e2a'))}  D={rng(col('e2d'))}  S={rng(col('e2s'))}  R={rng(col('e2r'))}")
        print(f"  modulation grid usage:")
        print(f"     Env->cutoff {env}/{n} ({100*env//n}%)   KeyTrack->cutoff {kt}/{n} ({100*kt//n}%)   Vel->cutoff {vel}/{n} ({100*vel//n}%)")
        print(f"     LFO->cutoff {lfoC}/{n} ({100*lfoC//n}%)  Vel->amp {velA}/{n} ({100*velA//n}%)   LFO->waveshape {pwm}/{n} ({100*pwm//n}%)")

    # ---- anti-patterns across the whole dataset ----
    print("\n========== ANTI-PATTERNS in the dataset ==========")
    click = [n for t,n,v,V in items if V.get('e1s',0)==0 and V.get('e1d',0) < 70 and V.get('e1a',0) < 4]
    darksus = [n for t,n,v,V in items if 5 in cut_sources(V) and V.get('e2s',0)==0 and V.get('cutoff',0) < 70]
    nokt = sum(1 for t,n,v,V in items if 10 not in cut_sources(V))
    hotres = [n for t,n,v,V in items if V.get('reso',0) > 230]
    noamp = [n for t,n,v,V in items if V.get('o1level',0) < 40 and V.get('o2level',0) < 40]
    print(f"  click risk (ampSus=0 + ampDecay<70 + attack~0): {len(click)}   e.g. {click[:4]}")
    print(f"  dark/silent sustain (filterEnv sweep + filtSus=0 + cutoff<70): {len(darksus)}   e.g. {darksus[:4]}")
    print(f"  NO filter key-tracking (cutoff fixed across keyboard): {nokt}/{len(items)}")
    print(f"  very hot resonance (>230, screech/self-osc risk): {len(hotres)}   e.g. {hotres[:4]}")
    print(f"  both osc levels near-zero (possible silent patch): {len(noamp)}   e.g. {noamp[:4]}")
