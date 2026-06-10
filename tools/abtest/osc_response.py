"""osc_response.py — measure the OSCILLATOR harmonic spectrum (saw), real vs clone.

A steady saw (filter wide open, full sustain) → the output spectrum is the osc's harmonic
series at n·f0. Extract each harmonic's amplitude, normalise to the fundamental, and compare
the rolloff (dB vs harmonic n) — this reveals whether the clone's HF "tilt" matches the real's
actual harmonic rolloff (audit A1). f0 ≈ 130.8 Hz (note 48; vaz_auto plays the real +12 so both
land on the same pitch).
"""
import os, sys, subprocess, time
sys.path.insert(0, '.'); sys.path.insert(0, os.path.join('..', 'vaz_auto'))
import numpy as np
from scipy.io import wavfile
import make_test_patch as MK
from vaz_auto import VazAuto

VR  = r'C:\APC\y\build\plugins\VAZClone\VazRender_artefacts\Release\VazRender.exe'
MID = os.path.join('midi', 'low_C2.mid')
F0  = 130.81                                 # C3

def harmonics(path, nmax=40):
    sr, x = wavfile.read(path); x = x.astype(float); x = x.mean(1) if x.ndim > 1 else x
    seg = x[int(1.0*sr):int(2.5*sr)] * np.hanning(int(1.5*sr))
    X = np.abs(np.fft.rfft(seg)); fr = np.fft.rfftfreq(len(seg), 1/sr)
    amps = []
    for n in range(1, nmax+1):
        f = n*F0
        if f > sr/2 - 200: break
        k = np.argmin(np.abs(fr - f))
        amps.append(X[max(0,k-3):k+4].max())     # peak in a small window around n·f0
    a = np.array(amps); return a / (a[0] + 1e-30)  # normalise to the fundamental

def main():
    MK.make({**MK.BASE, 'o1_wave': 0, 'cutoff': 255, 'resonance': 0}, 'osc_saw.v2p')   # pure saw, filter open
    vaz = VazAuto().launch()
    try:
        vaz.open_patch(os.path.join('test_patches', 'osc_saw.v2p')); time.sleep(0.7)
        vaz.capture_render(MID, os.path.join('wav', 'OSC_saw_real.wav'))
    finally:
        vaz.close()
    subprocess.run([VR, os.path.join('test_patches', 'osc_saw.v2p'), MID,
                    os.path.join('wav', 'OSC_saw_clone.wav'), '3.0'], capture_output=True)

    hr = harmonics(os.path.join('wav', 'OSC_saw_real.wav'))
    hc = harmonics(os.path.join('wav', 'OSC_saw_clone.wav'))
    n = min(len(hr), len(hc))
    print('\n  harmonic   freq    REAL(dB)  CLONE(dB)   ideal saw 1/n   diff')
    print('  ' + '-'*60)
    for i in range(n):
        h = i+1
        rd = 20*np.log10(hr[i]+1e-9); cd = 20*np.log10(hc[i]+1e-9); idl = 20*np.log10(1.0/h)
        if h <= 16 or h % 4 == 0:
            print(f'   {h:3d}     {h*F0:5.0f}   {rd:6.1f}   {cd:6.1f}     {idl:6.1f}       {cd-rd:+5.1f}')

if __name__ == '__main__':
    main()
