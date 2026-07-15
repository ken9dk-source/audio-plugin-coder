"""Final disambiguation for #9: are the NON-octave points on the smooth exponential?
The octave anchors (48/96/144/192/240) are satisfied by BOTH a continuous 2^((b-144)/48) AND a
stepped/octave-quantised mapping. b=120 and b=168 separate them:
    continuous -> 130.81*2^-0.5 = 92.5 Hz  and  130.81*2^+0.5 = 185.0 Hz
    stepped    -> would snap to 65.4/130.8 or 130.8/261.6
Fundamental is read from the SPECTRUM (lowest strong peak) — the autocorr detector was unreliable.
"""
import os, sys, time, math, struct
import numpy as np
from scipy.io import wavfile
TOOLS = r'C:\APC\y\tools'
sys.path.insert(0, TOOLS); sys.path.insert(0, os.path.join(TOOLS, 'vaz_auto'))
from gen_trance_presets import trace, landmarks
from vaz_auto import VazAuto

BASE = r'C:\Users\ken98\Desktop\Osc 3.v2p'
WORK = os.path.join(os.path.dirname(BASE), 'osc3probe'); os.makedirs(WORK, exist_ok=True)
MIDI = os.path.join(TOOLS, 'abtest', 'midi', '01_sustain_C3.mid')
C3   = 130.81

def patch(base_bytes, **fields):
    d = bytearray(base_bytes)
    prst, _, _ = landmarks(d)
    ver, off, val = trace(d, prst)
    for k, v in fields.items():
        o = off['lfo1rate'] - 4 if k == 'foot' else off[k]
        struct.pack_into('<I', d, o, int(v) & 0xFFFFFFFF)
    return bytes(d)

def fundamental(path):
    """Lowest strong spectral peak = f0 (robust; the signal is a strong odd-harmonic tone)."""
    sr, raw = wavfile.read(path)
    x = raw.astype(float)
    if x.ndim > 1: x = x.mean(axis=1)
    if np.issubdtype(raw.dtype, np.integer): x = x / float(np.iinfo(raw.dtype).max)
    n = len(x)
    a = int(0.6 * sr); b = min(n, a + int(2.5 * sr))
    seg = x[a:b] - np.mean(x[a:b])
    if len(seg) < 4096: return None, -999
    pk = 20 * math.log10(max(float(np.max(np.abs(x))), 1e-12))
    w = np.hanning(len(seg)); S = np.abs(np.fft.rfft(seg * w)); fr = np.fft.rfftfreq(len(seg), 1.0 / sr)
    S[fr < 18] = 0.0
    mx = S.max() + 1e-12
    cand = [(float(fr[i]), float(S[i] / mx)) for i in np.argsort(S)[::-1][:1500] if S[i] / mx > 0.12]
    if not cand: return None, pk
    # lowest frequency among the strong peaks = the fundamental
    cand.sort(key=lambda t: t[0])
    return cand[0][0], pk

def main():
    base = open(BASE, 'rb').read()
    iso = dict(o1level=0, o2level=0, cutoff=255, reso=0, foot=96)
    tests = [120, 144, 168, 216]      # 144 = reference re-check; 120/168/216 = NON-octave points
    vaz = VazAuto(midi_hint='loop').launch()
    out = {}
    try:
        for r in tests:
            p = os.path.join(WORK, f'F_rate{r}.v2p'); open(p, 'wb').write(patch(base, lfo1rate=r, **iso))
            w = os.path.join(WORK, f'F_rate{r}.wav')
            print(f'--- lfo1rate={r} ---')
            vaz.open_patch(p); time.sleep(0.8)
            vaz.capture_render(MIDI, w); time.sleep(0.3)
            f, pk = fundamental(w)
            out[r] = f
            print(f'   f0={None if f is None else round(f,2)} Hz  peak={pk:.1f}dB')
    finally:
        vaz.close()
    print('\n  rate   measured f0    continuous C3*2^((b-144)/48)   stepped(octave-quantised)   err%')
    for r in tests:
        f = out.get(r)
        cont = C3 * 2.0 ** ((r - 144) / 48.0)
        step = C3 * 2.0 ** round((r - 144) / 48.0)
        e = (abs(f - cont) / cont * 100.0) if f else None
        print(f'  {r:4d}   {("%9.2f"%f) if f else "     None"}      {cont:11.2f}               {step:11.2f}      {("%5.1f"%e) if e is not None else "  --"}')
    print('\nIf measured tracks the CONTINUOUS column at 120/168/216 -> the exponential is smooth ->')
    print('the clone formula 2^((b-144)/48) is CONFIRMED as a continuous law (not just at octaves).')

if __name__ == '__main__':
    main()
