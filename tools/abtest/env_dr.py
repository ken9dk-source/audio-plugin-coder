"""env_dr.py — measure the amp envelope DECAY + RELEASE (time + curve shape), real vs clone."""
import os, sys, subprocess, time
sys.path.insert(0, '.'); sys.path.insert(0, os.path.join('..', 'vaz_auto'))
import numpy as np
from scipy.io import wavfile
import make_test_patch as MK
from vaz_auto import VazAuto

VR  = r'C:\APC\y\build\plugins\VAZClone\VazRender_artefacts\Release\VazRender.exe'
MID = os.path.join('midi', 'longnote.mid')        # on@0, off@3.0s, 4s
DECS = [100, 250, 400]
RELS = [100, 250, 400]

def env(path, win=0.005):
    sr, x = wavfile.read(path); x = x.astype(float); x = x.mean(1) if x.ndim > 1 else x
    x = np.abs(x); n = int(win*sr)
    e = np.array([x[i:i+n].max() for i in range(0, len(x)-n, n)])
    return e/(e.max()+1e-30), n/sr

def decay_time(e, dt, sus):
    on = int(np.argmax(e > 0.5)); pk = on + int(np.argmax(e[on:] >= 0.97))
    tgt = sus + 0.05*(1.0 - sus)
    seg = e[pk:pk+int(3.0/dt)]
    i = int(np.argmax(seg <= tgt)) if np.any(seg <= tgt) else len(seg)-1
    t = i*dt
    s = seg[:max(3,i)]; cv = float(np.mean((s-s[0])/(s[-1]-s[0]+1e-9) - np.linspace(0,1,len(s)))) if i>=3 else 0
    return t, cv

def release_time(e, dt):
    # note-off ~3.0s; find the sustained level just before, then the fall to 5%
    off = int(3.0/dt); pre = np.median(e[max(0,off-30):off])
    seg = e[off:]; i = int(np.argmax(seg <= 0.05*pre)) if np.any(seg <= 0.05*pre) else len(seg)-1
    return i*dt

def main():
    for d in DECS: MK.make({**MK.BASE, 'e1_attack':0, 'e1_decay':d, 'e1_sustain':60, 'e1_release':30, 'cutoff':255}, f'dec_{d}.v2p')
    for r in RELS: MK.make({**MK.BASE, 'e1_attack':0, 'e1_decay':0, 'e1_sustain':255, 'e1_release':r, 'cutoff':255}, f'rel_{r}.v2p')
    vaz = VazAuto().launch()
    try:
        for d in DECS:
            vaz.open_patch(os.path.join('test_patches', f'dec_{d}.v2p')); time.sleep(0.7); vaz.capture_render(MID, os.path.join('wav', f'DEC_{d}_real.wav'))
        for r in RELS:
            vaz.open_patch(os.path.join('test_patches', f'rel_{r}.v2p')); time.sleep(0.7); vaz.capture_render(MID, os.path.join('wav', f'REL_{r}_real.wav'))
    finally:
        vaz.close()
    for d in DECS: subprocess.run([VR, os.path.join('test_patches', f'dec_{d}.v2p'), MID, os.path.join('wav', f'DEC_{d}_clone.wav'), '4.0'], capture_output=True)
    for r in RELS: subprocess.run([VR, os.path.join('test_patches', f'rel_{r}.v2p'), MID, os.path.join('wav', f'REL_{r}_clone.wav'), '4.0'], capture_output=True)

    print('\n  DECAY param  x       REAL-t   CLONE-t   REAL-cv  CLONE-cv   clone-map(0.001+x2*3*.35?)')
    for d in DECS:
        x = d/425.0
        er, dr = env(os.path.join('wav', f'DEC_{d}_real.wav')); tr, cr = decay_time(er, dr, 60/255.)
        ec, dc = env(os.path.join('wav', f'DEC_{d}_clone.wav')); tc, cc = decay_time(ec, dc, 60/255.)
        print(f'   {d:4d}       {x:.3f}   {tr:6.3f}s  {tc:6.3f}s   {cr:+.3f}   {cc:+.3f}')
    print('\n  REL param   x       REAL-t   CLONE-t')
    for r in RELS:
        x = r/425.0
        er, dr = env(os.path.join('wav', f'REL_{r}_real.wav')); tr = release_time(er, dr)
        ec, dc = env(os.path.join('wav', f'REL_{r}_clone.wav')); tc = release_time(ec, dc)
        print(f'   {r:4d}      {x:.3f}   {tr:6.3f}s  {tc:6.3f}s')

if __name__ == '__main__':
    main()
