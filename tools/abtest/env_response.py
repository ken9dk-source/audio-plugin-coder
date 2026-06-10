"""env_response.py — measure the AMP ENVELOPE (attack time + curve shape + decay), real vs clone.

The output amplitude = osc × amp-env, and the osc is steady → the amplitude envelope IS the amp
env, cleanly (no osc confound, unlike filter Q). Renders a held note (longnote.mid) with a sweep
of attack values + one slow attack/decay patch, then extracts the envelope (peak per 5 ms window)
and reports: actual attack time (param→seconds map) and the attack curvature (linear vs exp).
"""
import os, sys, subprocess, time
sys.path.insert(0, '.'); sys.path.insert(0, os.path.join('..', 'vaz_auto'))
import numpy as np
from scipy.io import wavfile
import make_test_patch as MK
from vaz_auto import VazAuto

VR  = r'C:\APC\y\build\plugins\VAZClone\VazRender_artefacts\Release\VazRender.exe'
MID = os.path.join('midi', 'longnote.mid')
ATKS = [40, 120, 240, 400]                   # amp-attack 16-bit values; clone maps x = v/425 (loadV2P @PS+54)

def envelope(path, win=0.005):
    sr, x = wavfile.read(path); x = x.astype(float); x = x.mean(1) if x.ndim > 1 else x
    x = np.abs(x); n = int(win*sr)
    env = np.array([x[i:i+n].max() for i in range(0, len(x)-n, n)])
    env /= (env.max() + 1e-30); dt = n/sr
    return env, dt

def attack_time(env, dt):
    pk = np.argmax(env >= 0.98)                 # first sample reaching ~peak
    # note-on = first rise above noise
    on = np.argmax(env > 0.03)
    return (pk - on)*dt, on

def curvature(env, dt, on, atk_t):
    # sample the rising portion, compare to a linear ramp → +convex(exp-up) / 0 linear / −concave
    npts = max(3, int(atk_t/dt)); seg = env[on:on+npts]
    if len(seg) < 3: return 0.0
    seg = seg/seg[-1]; lin = np.linspace(0, 1, len(seg))
    return float(np.mean(seg - lin))            # >0 = above the line (concave/fast-start), <0 = below (convex/slow-start)

def main():
    for v in ATKS:
        MK.make({**MK.BASE, 'e1_attack': v, 'e1_decay': 0, 'e1_sustain': 255, 'cutoff': 255}, f'atk_{v}.v2p')
    vaz = VazAuto().launch()
    try:
        for v in ATKS:
            vaz.open_patch(os.path.join('test_patches', f'atk_{v}.v2p')); time.sleep(0.7)
            vaz.capture_render(MID, os.path.join('wav', f'ATK_{v}_real.wav'))
    finally:
        vaz.close()
    for v in ATKS:
        subprocess.run([VR, os.path.join('test_patches', f'atk_{v}.v2p'), MID,
                        os.path.join('wav', f'ATK_{v}_clone.wav'), '4.0'], capture_output=True)

    print('\n  attack-param   x=v/65535   REAL atk(s)   CLONE atk(s)   REAL curve   CLONE curve')
    print('  ' + '-'*78)
    for v in ATKS:
        x = v/425.0
        er, dr = envelope(os.path.join('wav', f'ATK_{v}_real.wav'));  tr, onr = attack_time(er, dr)
        ec, dc = envelope(os.path.join('wav', f'ATK_{v}_clone.wav')); tc, onc = attack_time(ec, dc)
        cr = curvature(er, dr, onr, tr); cc = curvature(ec, dc, onc, tc)
        print(f'  {v:6d}        {x:.3f}       {tr:6.3f}        {tc:6.3f}       {cr:+.3f}       {cc:+.3f}')
    print('  (curve >0 = fast-start/concave, ~0 = linear, <0 = slow-start/convex)')
    print('  clone map: t = 0.001 + x^2 * 3.0 :', [f'{0.001+(v/425.)**2*3.0:.3f}' for v in ATKS])

if __name__ == '__main__':
    main()
