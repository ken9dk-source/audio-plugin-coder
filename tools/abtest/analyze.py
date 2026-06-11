#!/usr/bin/env python3
"""
analyze.py — A/B analyzer for VAZ 2010 (real) vs VAZClone (JUCE).

  py analyze.py real.wav clone.wav [--note 48] [--noteoff 2.8] [--json out.json]

Both WAVs must be rendered from the SAME MIDI + matched patch (see gen_midi.py).
Measures: noise floor, ADSR envelope, frequency response, harmonic content,
filter-sweep (spectral-centroid trajectory), resonance, modulation depth/rate,
stereo image — then ranks deviations by audible importance and names the likely module.

Deps: numpy, scipy.
"""
import sys, json, argparse
import numpy as np
from scipy.io import wavfile
from scipy.signal import resample_poly, hilbert

# ---------------------------------------------------------------- io / helpers
def read_wav(path):
    sr, x = wavfile.read(path)
    if x.dtype.kind in "iu":
        x = x.astype(np.float64) / np.iinfo(x.dtype).max
    else:
        x = x.astype(np.float64)
    if x.ndim == 1: x = x[:, None]
    return sr, x

def to_sr(x, sr, target=48000):
    if sr == target: return x
    g = np.gcd(sr, target)
    return resample_poly(x, target // g, sr // g, axis=0)

def mono(x): return x.mean(axis=1)

def db(v):
    v = np.asarray(v, float)
    return 20 * np.log10(np.maximum(np.abs(v), 1e-12))

def env_rms(x, sr, win_ms=15, hop_ms=5):
    w = max(1, int(sr * win_ms / 1000)); h = max(1, int(sr * hop_ms / 1000))
    n = 1 + (len(x) - w) // h if len(x) >= w else 1
    e = np.array([np.sqrt(np.mean(x[i*h:i*h+w] ** 2) + 1e-20) for i in range(max(n, 1))])
    t = np.arange(len(e)) * h / sr
    return t, e

def align(a, b, sr):
    """shift b so its onset matches a (compensate plugin/Capture latency); trim to common len.
    Onset-based (robust to large >0.5s Capture offsets), then a short cross-corr refine."""
    def onset(x):
        pk = np.max(np.abs(x))
        return 0 if pk <= 0 else int(np.argmax(np.abs(x) > 0.02 * pk))
    lag = onset(a) - onset(b)             # coarse: match onsets (fixes the ~0.56s Capture offset)
    if lag > 0:   b = np.r_[np.zeros(lag), b]
    elif lag < 0: b = b[-lag:]
    L = min(len(a), len(b))
    a, b = a[:L], b[:L]
    # fine refine: cross-corr on a 0.4s window just after the (now common) onset
    on = onset(a); w = min(L - on, int(0.4 * sr))
    if w > 64:
        aw = a[on:on+w] - a[on:on+w].mean(); bw = b[on:on+w] - b[on:on+w].mean()
        xc = np.correlate(aw, bw, "full"); fine = np.argmax(np.abs(xc)) - (w - 1)
        if abs(fine) < 0.02 * sr:         # only trust sub-20ms refinements
            lag += fine
            if fine > 0:   b = np.r_[np.zeros(fine), b][:L]
            elif fine < 0: b = np.r_[b[-fine:], np.zeros(-fine)][:L]
    return a, b, lag

# ---------------------------------------------------------------- measurements
def onset_idx(t, e):
    pk = e.max(); thr = max(pk * 0.05, e[:max(3, len(e)//50)].mean() * 4)
    above = np.where(e > thr)[0]
    return above[0] if len(above) else 0

def adsr(x, sr, note_off_s=None):
    t, e = env_rms(np.abs(x), sr, win_ms=4, hop_ms=1)      # fine resolution for attack/release
    on = onset_idx(t, e)
    pk_i = on + int(np.argmax(e[on:on + max(1, int(0.5/ (t[1]-t[0])))])) if len(e) > on+1 else on
    peak = e[pk_i]
    # attack 10->90 %
    seg = e[on:pk_i+1]
    if len(seg) > 1:
        a10 = on + np.argmax(seg >= 0.1*peak); a90 = on + np.argmax(seg >= 0.9*peak)
        attack = max(t[a90] - t[a10], 0.0)
    else: attack = 0.0
    off_i = np.searchsorted(t, note_off_s) if note_off_s else min(len(e)-1, pk_i + int(0.6*(len(e)-pk_i))+1)
    off_i = min(off_i, len(e)-1)
    sus_win = e[max(pk_i+1, off_i-int(0.4/(t[1]-t[0]))):off_i] if off_i > pk_i+1 else e[pk_i:pk_i+1]
    sustain = float(np.median(sus_win)) if len(sus_win) else peak
    # decay: peak -> within 5% of sustain
    dec = e[pk_i:off_i]
    decay = 0.0
    if len(dec) > 1 and peak > sustain*1.05:
        tgt = sustain + 0.05*(peak-sustain)
        below = np.where(dec <= tgt)[0]
        decay = (below[0])*(t[1]-t[0]) if len(below) else (t[off_i]-t[pk_i])
    # release: note_off -> 10 % of sustain (robust to noise floor; ~ -20 dB fall time)
    rel = e[off_i:]; release = 0.0
    if len(rel) > 2:
        floor = max(sustain, 1e-9) * 0.1
        below = np.where(rel <= floor)[0]
        release = (below[0])*(t[1]-t[0]) if len(below) else (t[-1]-t[off_i])
    return dict(attack=attack, peak=float(peak), decay=decay,
                sustain=sustain, sustain_db=float(db(sustain)), release=release,
                onset_s=float(t[on]), peak_s=float(t[pk_i]))

def sustain_slice(x, sr, note_off_s):
    t, e = env_rms(np.abs(x), sr); on = onset_idx(t, e); pk = on+int(np.argmax(e[on:]))
    a = int((t[pk]+0.2) * sr); b = int((note_off_s or t[-1]-0.2) * sr)
    a, b = max(0, a), min(len(x), b)
    if b - a < sr//4: a, b = len(x)//3, 2*len(x)//3
    return x[a:b]

def spectrum(x, sr):
    n = 1 << int(np.ceil(np.log2(len(x))))
    X = np.abs(np.fft.rfft(x * np.hanning(len(x)), n))
    f = np.fft.rfftfreq(n, 1/sr)
    return f, X / (X.max() + 1e-12)

def third_oct(f, X):
    centers = 1000 * 2.0 ** (np.arange(-19, 11) / 3)      # ~12.5 Hz .. ~16 kHz
    out = []
    for fc in centers:
        lo, hi = fc / 2**(1/6), fc * 2**(1/6)
        m = (f >= lo) & (f < hi)
        out.append(db(np.sqrt(np.mean(X[m]**2))) if m.any() else -120.0)
    return centers, np.array(out)

def harmonics(x, sr, f0, k=16):
    f, X = spectrum(x, sr)
    levels = []
    for h in range(1, k+1):
        fc = f0*h
        if fc > sr/2 - 50: levels.append(-120.0); continue
        m = (f >= fc-f0*0.4) & (f <= fc+f0*0.4)
        levels.append(db(X[m].max()) if m.any() else -120.0)
    levels = np.array(levels)
    lin = 10**(levels/20)
    thd = float(np.sqrt(np.sum(lin[1:]**2)) / (lin[0]+1e-12))
    return levels, thd

def centroid_traj(x, sr, win=4096, hop=2048):
    f = np.fft.rfftfreq(win, 1/sr); out=[]
    for i in range(0, len(x)-win, hop):
        X = np.abs(np.fft.rfft(x[i:i+win]*np.hanning(win)))
        out.append(np.sum(f*X)/(np.sum(X)+1e-12))
    return np.array(out)

def resonance(f, X):
    band = (f > 60) & (f < 12000)
    fb, Xb = f[band], db(X[band])
    if len(Xb) < 8: return dict(peak_db=0.0, peak_hz=0.0, q=0.0)
    sm = np.convolve(Xb, np.ones(9)/9, "same")
    i = int(np.argmax(Xb - sm))                  # sharpest local bump vs smoothed
    prom = float(Xb[i] - sm[i])
    return dict(peak_db=prom, peak_hz=float(fb[i]))

def modulation(x, sr, note_off_s):
    seg = sustain_slice(x, sr, note_off_s)
    _, e = env_rms(np.abs(seg), sr, 8, 2); e = e - e.mean()
    er = sr/ (2/1000*sr)                           # env hop = 2ms -> env sr = 500 Hz
    env_sr = 500.0
    n = 1 << int(np.ceil(np.log2(max(8,len(e)))))
    E = np.abs(np.fft.rfft(e, n)); ef = np.fft.rfftfreq(n, 1/env_sr)
    band = (ef > 0.2) & (ef < 30)
    if not band.any(): return dict(rate_hz=0.0, depth=0.0)
    i = np.argmax(E[band]); rate = float(ef[band][i])
    depth = float(2*E[band][i] / (len(e)+1e-9))    # ~amplitude of ripple
    return dict(rate_hz=rate, depth=depth)

def stereo(x, sr, note_off_s):
    if x.shape[1] < 2: return dict(is_stereo=False, correlation=1.0, width=0.0, side_db=-120.0)
    L = sustain_slice(x[:,0], sr, note_off_s); R = sustain_slice(x[:,1], sr, note_off_s)
    L,R = L[:min(len(L),len(R))], R[:min(len(L),len(R))]
    corr = float(np.corrcoef(L, R)[0,1]) if len(L) > 4 else 1.0
    mid = (L+R)/2; side=(L-R)/2
    side_db = float(db(np.sqrt(np.mean(side**2))) - db(np.sqrt(np.mean(mid**2))+1e-12))
    return dict(is_stereo=True, correlation=corr, width=float(1-corr), side_db=side_db)

def noise_floor(x, sr):
    """quietest 150 ms window anywhere (pre-attack OR post-release tail) = true noise floor."""
    w = int(0.15*sr); h = int(0.05*sr)
    if len(x) < w: return float(db(np.sqrt(np.mean(x**2)+1e-20)))
    rms = [np.sqrt(np.mean(x[i:i+w]**2)+1e-20) for i in range(0, len(x)-w, h)]
    return float(db(min(rms)))

# ---------------------------------------------------------------- compare + rank
def analyze(real_path, clone_path, note=None, note_off=None):
    sr_r, R = read_wav(real_path); sr_c, C = read_wav(clone_path)
    R = to_sr(R, sr_r); C = to_sr(C, sr_c); sr = 48000
    f0 = 440*2**((note-69)/12) if note else None
    # High-pass below the fundamental: removes loopback mains-hum/DC that would otherwise
    # dominate the spectral peak and make a faithful osc look wildly off (fair A/B).
    if f0:
        from scipy.signal import butter, sosfilt
        sos = butter(4, max(35.0, 0.6*f0)/(sr/2), "high", output="sos")
        R = sosfilt(sos, R, axis=0); C = sosfilt(sos, C, axis=0)
    mR, mC = mono(R), mono(C)
    mR, mC, lag = align(mR, mC, sr)

    rep = {}
    # noise floor
    rep["noise_floor"] = dict(real=noise_floor(mR,sr), clone=noise_floor(mC,sr), unit="dBFS",
                              module="Noise gen / denormals / dither", weight=2)
    # envelope
    aR, aC = adsr(mR, sr, note_off), adsr(mC, sr, note_off)
    for k,(mod,w) in {"attack":("Env1 / amp attack",7),"decay":("Env1 / amp decay",5),
                      "sustain_db":("Env1 sustain / osc level",6),"release":("Env1 / amp release",6)}.items():
        rep[f"env_{k}"]=dict(real=aR[k], clone=aC[k], unit="s" if k!="sustain_db" else "dB", module=mod, weight=w)
    # harmonics
    if f0:
        sR, sC = sustain_slice(mR,sr,note_off), sustain_slice(mC,sr,note_off)
        hR,tR = harmonics(sR,sr,f0); hC,tC = harmonics(sC,sr,f0)
        rep["thd"]=dict(real=tR, clone=tC, unit="ratio", module="Oscillators / waveshape / ring-mod / sync", weight=7)
        rep["_harm_real"]=hR.tolist(); rep["_harm_clone"]=hC.tolist()
        hd = float(np.mean(np.abs(hR[:10]-hC[:10])))
        rep["harmonic_profile"]=dict(real=0.0, clone=hd, unit="dB avg|d-harm|", module="Oscillators (waveform/level/FM)", weight=7)
    # frequency response (1/3-oct spectral tilt)
    sR, sC = sustain_slice(mR,sr,note_off), sustain_slice(mC,sr,note_off)
    fR,XR=spectrum(sR,sr); fC,XC=spectrum(sC,sr)
    cen,bR=third_oct(*(fR,XR)); _,bC=third_oct(fC,XC)
    valid=(bR>-110)|(bC>-110)
    lsd=float(np.sqrt(np.mean((bR[valid]-bC[valid])**2)))
    # brightness = energy centroid of the band curve
    briR=float(np.sum(cen*10**(bR/20))/ (np.sum(10**(bR/20))+1e-9))
    briC=float(np.sum(cen*10**(bC/20))/ (np.sum(10**(bC/20))+1e-9))
    rep["spectral_distance"]=dict(real=0.0, clone=lsd, unit="dB RMS / band", module="Filter cutoff / osc spectrum", weight=8)
    rep["brightness"]=dict(real=briR, clone=briC, unit="Hz centroid", module="Filter cutoff", weight=8)
    rep["_bands_real"]=bR.tolist(); rep["_bands_clone"]=bC.tolist(); rep["_band_hz"]=cen.tolist()
    # filter sweep (centroid trajectory shape)
    ctR, ctC = centroid_traj(mR,sr), centroid_traj(mC,sr)
    L=min(len(ctR),len(ctC))
    sweep=float(np.mean(np.abs(ctR[:L]-ctC[:L]))) if L>2 else 0.0
    rep["filter_sweep"]=dict(real=float(np.ptp(ctR)) if len(ctR) else 0.0,
                             clone=float(np.ptp(ctC)) if len(ctC) else 0.0,
                             unit="Hz centroid range", module="Filter env (Env2->cutoff) / cutoff mod", weight=7,
                             traj_diff=sweep)
    # resonance
    rR=resonance(fR,XR); rC=resonance(fC,XC)
    rep["resonance_peak"]=dict(real=rR["peak_db"], clone=rC["peak_db"], unit="dB prominence",
                               module="Filter resonance / ladder Q", weight=6)
    # modulation
    mR_=modulation(mR,sr,note_off); mC_=modulation(mC,sr,note_off)
    rep["mod_rate"]=dict(real=mR_["rate_hz"], clone=mC_["rate_hz"], unit="Hz", module="LFO rate / sync", weight=5)
    rep["mod_depth"]=dict(real=mR_["depth"], clone=mC_["depth"], unit="ripple", module="LFO depth / routing", weight=5)
    # stereo
    sR_=stereo(R,sr,note_off); sC_=stereo(C,sr,note_off)
    rep["stereo_width"]=dict(real=sR_["width"], clone=sC_["width"], unit="1-corr", module="Unison / pan / stereo engine", weight=4)
    rep["stereo_side"]=dict(real=sR_["side_db"], clone=sC_["side_db"], unit="dB S/M", module="Unison detune / pan spread", weight=4)
    rep["_meta"]=dict(align_lag=int(lag), f0=f0, sr=sr)
    return rep

def severity(k, r, c):
    """normalized 0..1 deviation per metric type"""
    if r is None or c is None: return 0.0
    try:
        if not (np.isfinite(float(r)) and np.isfinite(float(c))): return 0.0   # near-silent -> NaN guard
    except (TypeError, ValueError): return 0.0
    if k in ("env_attack","env_decay","env_release"):           # seconds
        return min(1.0, abs(r-c)/max(0.02, abs(r), abs(c)))
    if k in ("noise_floor","env_sustain_db","resonance_peak","stereo_side","brightness"):
        scale={"noise_floor":12,"env_sustain_db":6,"resonance_peak":6,"stereo_side":9,"brightness":1500}[k]
        return min(1.0, abs(r-c)/scale)
    if k=="spectral_distance": return min(1.0, c/8.0)            # c is the distance itself
    if k=="harmonic_profile":  return min(1.0, c/6.0)
    if k=="thd":               return min(1.0, abs(r-c)/max(0.05, r, c))
    if k=="mod_rate":          return min(1.0, abs(r-c)/max(0.5, r, c))
    if k in ("mod_depth","filter_sweep","stereo_width","brightness"):
        return min(1.0, abs(r-c)/max(1e-6, abs(r), abs(c)))
    return min(1.0, abs(r-c)/max(1e-6, abs(r), abs(c)))

def report(rep):
    rows=[]
    for k,v in rep.items():
        if k.startswith("_") or not isinstance(v,dict) or "real" not in v: continue
        s=severity(k, v["real"], v["clone"]); rows.append((s*v["weight"], s, k, v))
    rows.sort(reverse=True)
    print("\n"+"="*92)
    print("  VAZ  A/B  DEVIATION  REPORT   (ranked by audible importance = severity x weight)")
    print("="*92)
    print(f"  {'metric':18s}{'real':>12s}{'clone':>12s}{'sev':>6s}{'prio':>6s}  likely module")
    print("  "+"-"*88)
    for score,s,k,v in rows:
        r,c=v["real"],v["clone"]
        rs=f"{r:.3f}" if isinstance(r,float) else str(r)
        cs=f"{c:.3f}" if isinstance(c,float) else str(c)
        flag = "!!" if score>=4 else ("+ " if score>=2 else (". " if score>=0.8 else "  "))
        print(f"{flag} {k:18s}{rs:>12s}{cs:>12s}{s:>6.2f}{score:>6.1f}  {v['module']}  [{v['unit']}]")
    print("  "+"-"*88)
    print("  !! critical (very audible)   + notable   . minor   (blank) negligible")
    top=[k for _,s,k,_ in rows[:3] if s>0.15]
    if top:
        print("\n  >>> TOP SUSPECTS:")
        for _,s,k,v in rows[:3]:
            if s>0.15: print(f"      - {v['module']}  (via {k}, severity {s:.2f})")
    # harmonic detail
    if "_harm_real" in rep:
        print("\n  per-harmonic level (dB, h1..h10):")
        print("      real :", " ".join(f"{x:6.1f}" for x in rep['_harm_real'][:10]))
        print("      clone:", " ".join(f"{x:6.1f}" for x in rep['_harm_clone'][:10]))
    print()

def main():
    try: sys.stdout.reconfigure(encoding="utf-8", errors="replace")
    except Exception: pass
    ap=argparse.ArgumentParser()
    ap.add_argument("real"); ap.add_argument("clone")
    ap.add_argument("--note", type=int, default=48, help="MIDI note for harmonic analysis (default 48=C3)")
    ap.add_argument("--noteoff", type=float, default=2.8, help="note-off time in seconds")
    ap.add_argument("--json", default=None)
    a=ap.parse_args()
    rep=analyze(a.real, a.clone, a.note, a.noteoff)
    report(rep)
    if a.json:
        import os; os.makedirs(os.path.dirname(a.json) or ".", exist_ok=True)
        with open(a.json,"w") as f: json.dump({k:v for k,v in rep.items()}, f, indent=2)
        print(f"  JSON -> {a.json}")

if __name__=="__main__":
    main()
