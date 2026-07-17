"""osc_wave_probe — render the USER'S real-VAZ waveform presets via File->Capture and test the
disassembly-derived 2-segment model against actual VAZ audio.  (LOOPBACK must NOT be used: it captures
silence and peak-normalises on write, so silence looks like signal. capture_render = File->Capture.)

THE DECISIVE CHECK (cheapest, decides the rest):
  Does real VAZ's Saw at C3 ALIAS?  The model says freqIdx clamps to 0 below ~134 Hz, so the falling
  edge is 0.05% of a cycle (~0.16 samples) = effectively a hard discontinuity => it SHOULD alias.
    (a) aliases  -> the 2-segment model is complete
    (b) clean    -> something tames it ([+0x234] band-limit toggle / oversampling / filter): do NOT port
                    the raw ramp — that is the BLEP mistake again.

Other falsifiable predictions (model, at VAZ's own "50%" byte = 130):
    Pulse @shape 0   -> duty 0.500 (EXACT square)
    Pulse @shape 130 -> duty 0.754 (RISING, not falling; the clone's is the complement ~0.242)
    Saw   @shape 130 -> 25.4% falling edge
    MSaw  @shape 130 -> 4 saws at -/+18.75c and -/+6.25c
Run:  py tools/osc_wave_probe.py
"""
import os, sys, time, json
import numpy as np
from scipy.io import wavfile

TOOLS = os.path.dirname(os.path.abspath(__file__))
sys.path.insert(0, os.path.join(TOOLS, 'vaz_auto'))
from vaz_auto import VazAuto                                    # noqa: E402

DESK = r'C:\Users\ken98\Desktop'
WORK = os.path.join(DESK, 'oscprobe'); os.makedirs(WORK, exist_ok=True)
MIDI = os.path.join(TOOLS, 'abtest', 'midi', '01_sustain_C3.mid')
C3   = 130.8127827

PRESETS = [
    ('saw_s0',    'Vaz Saw.v2p'),
    ('saw_s130',  'Vaz saw 50%.v2p'),
    ('pulse_s0',  'Vaz pulse.v2p'),
    ('pulse_s130','Vaz Pulse 50%.v2p'),
    ('msaw_s0',   'Vaz Multisaw.v2p'),
    ('msaw_s130', 'Vaz Multisaw 50%.v2p'),
]


def spectrum(path):
    sr, raw = wavfile.read(path)
    x = raw.astype(np.float64)
    if x.ndim > 1: x = x.mean(axis=1)
    if np.max(np.abs(x)) < 1e-9: return None
    x /= np.max(np.abs(x))
    # take a steady segment well inside the note
    n = len(x); a = int(n * 0.35); b = min(a + 1 << 15, int(n * 0.85))
    seg = x[a:b]
    if len(seg) < 4096: return None
    seg = seg * np.hanning(len(seg))
    S = np.abs(np.fft.rfft(seg, 1 << 17))
    f = np.fft.rfftfreq(1 << 17, 1.0 / sr)
    S /= (S.max() + 1e-30)
    return dict(sr=sr, f=f, S=S, x=x)


def peaks(f, S, thresh_db=-60, fmax=20000):
    out = []
    for i in range(2, len(S) - 2):
        if f[i] > fmax: break
        if S[i] > S[i-1] and S[i] > S[i+1] and 20*np.log10(S[i]+1e-30) > thresh_db:
            out.append((f[i], 20*np.log10(S[i]+1e-30)))
    out.sort(key=lambda t: -t[1])
    return out[:60]


def alias_report(f, S, f0):
    """inharmonic energy = peaks NOT within 3% of a multiple of f0 -> aliasing fold-down"""
    pk = peaks(f, S, thresh_db=-55)
    harm, inharm = [], []
    for (fr, db) in pk:
        if fr < f0 * 0.5: continue
        k = round(fr / f0)
        if k >= 1 and abs(fr - k * f0) / (k * f0) < 0.03: harm.append((fr, db, k))
        else: inharm.append((fr, db))
    return harm, inharm


def duty_of(x, sr, f0):
    """measure pulse duty from the time domain: fraction of a cycle spent above the midpoint"""
    n = len(x); a = int(n*0.4); b = min(a + int(sr/f0)*20, int(n*0.85))
    seg = x[a:b]
    if len(seg) < 100: return None
    mid = 0.5*(seg.max()+seg.min())
    return float(np.mean(seg > mid))


def main():
    vaz = VazAuto(midi_hint='loop').launch()
    results = {}
    try:
        for tag, fname in PRESETS:
            src = os.path.join(DESK, fname)
            if not os.path.exists(src): print(f'  !! missing {fname}'); continue
            out = os.path.join(WORK, f'{tag}.wav')
            print(f'--- {tag}  ({fname}) ---')
            try:
                vaz.open_patch(src); time.sleep(0.8)
                vaz.capture_render(MIDI, out); time.sleep(0.3)
            except Exception as e:
                print('   CAPTURE ERROR:', e); continue
            sp = spectrum(out)
            if sp is None: print('   SILENT / unreadable capture'); continue
            harm, inharm = alias_report(sp['f'], sp['S'], C3)
            d = duty_of(sp['x'], sp['sr'], C3)
            ih_db = max([db for _, db in inharm], default=-99)
            print(f"   harmonics={len(harm)} (h1..h5 {[f'{k}:{db:.0f}dB' for _,db,k in sorted(harm,key=lambda t:t[2])[:5]]})")
            print(f"   INHARMONIC peaks={len(inharm)} strongest={ih_db:.1f}dB  -> {'ALIASES' if ih_db > -45 else 'clean'}")
            print(f"   duty(measured)={None if d is None else round(d,3)}")
            results[tag] = dict(nharm=len(harm), ninharm=len(inharm), inharm_db=ih_db, duty=d,
                                top_inharm=[(round(fr,1), round(db,1)) for fr, db in inharm[:6]])
    finally:
        try: vaz.close()
        except Exception: pass
    open(os.path.join(WORK, 'results.json'), 'w').write(json.dumps(results, indent=1))
    print('\n===== VERDICT =====')
    for k, v in results.items():
        print(f"  {k:11} inharm={v['inharm_db']:6.1f}dB  duty={v['duty']}  -> "
              f"{'ALIASES' if v['inharm_db'] > -45 else 'clean'}")


if __name__ == '__main__':
    main()
