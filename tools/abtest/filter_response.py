"""filter_response.py — isolate the FILTER's resonance response, oscillator-cancelled.

The synth render confounds osc + filter. Trick: render a saw at a fixed cutoff but several
resonance settings (incl. reso=0). The osc spectrum + the base-LP rolloff are IDENTICAL across
reso (same osc, same note, same cutoff), so  spectrum(reso_X) / spectrum(reso_0)  CANCELS the
osc + base-LP and leaves the pure RESONANCE shape. Compare real vs clone osc-independently.

Outputs, per reso: the resonance-peak gain (dB over the reso=0 baseline) + its frequency,
for the real VAZ (File->Capture) and the clone (VazRender). That is the clean filter-Q metric.
"""
import os, sys, subprocess, time
sys.path.insert(0, '.'); sys.path.insert(0, os.path.join('..', 'vaz_auto'))
import numpy as np
from scipy.io import wavfile
from scipy.signal import welch
import make_test_patch as MK
from vaz_auto import VazAuto

VR  = r'C:\APC\y\build\plugins\VAZClone\VazRender_artefacts\Release\VazRender.exe'
MID = os.path.join('midi', 'low_C2.mid')
CUTOFF = 150
RESOS  = [0, 60, 120, 180, 240]

def spec(path):
    sr, x = wavfile.read(path); x = x.astype(float); x = x.mean(1) if x.ndim > 1 else x
    seg = x[int(1.0*sr):int(2.5*sr)]
    fr, P = welch(seg, sr, nperseg=8192)
    return fr, P / (P.max() + 1e-30)         # normalise (level-independent → renders are peak-normalised anyway)

def resonance(path0, pathX):
    fr, P0 = spec(path0); _, PX = spec(pathX)
    m = (fr > 90) & (fr < 8000)
    ratio = PX[m] / (P0[m] + 1e-12)          # cancel osc + base-LP → pure resonance
    frm = fr[m]
    i = np.argmax(ratio)
    return frm[i], 10*np.log10(ratio[i])     # resonant-peak freq, gain over the reso=0 baseline (dB)

def main():
    for r in RESOS:
        MK.make({**MK.BASE, 'cutoff': CUTOFF, 'resonance': r, 'filter_mode': 19}, f'fr_{r}.v2p')
    vaz = VazAuto().launch()
    try:
        for r in RESOS:
            vaz.open_patch(os.path.join('test_patches', f'fr_{r}.v2p')); time.sleep(0.7)
            vaz.capture_render(MID, os.path.join('wav', f'FR_{r}_real.wav'))
    finally:
        vaz.close()
    for r in RESOS:
        subprocess.run([VR, os.path.join('test_patches', f'fr_{r}.v2p'), MID,
                        os.path.join('wav', f'FR_{r}_clone.wav'), '3.0'], capture_output=True)

    print('\n  reso   REAL res-peak (dB@Hz)     CLONE res-peak (dB@Hz)    dB-diff')
    print('  ' + '-'*64)
    for r in RESOS[1:]:
        rf, rdb = resonance(os.path.join('wav', f'FR_0_real.wav'),  os.path.join('wav', f'FR_{r}_real.wav'))
        cf, cdb = resonance(os.path.join('wav', f'FR_0_clone.wav'), os.path.join('wav', f'FR_{r}_clone.wav'))
        print(f'  {r:3d}    {rdb:6.1f} dB @ {rf:5.0f} Hz      {cdb:6.1f} dB @ {cf:5.0f} Hz     {cdb-rdb:+5.1f}')

if __name__ == '__main__':
    main()
