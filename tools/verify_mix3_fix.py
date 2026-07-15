"""Verify the mix3Src off-by-one fix AUDIBLY in the clone.

The user's VAZ-made "Osc 3" patch has mix3src=0 and lfo1rate=42.
  BEFORE the fix: the clone read 0 as "Noise"  -> ch3 = broadband noise (wrong).
  AFTER  the fix: the clone reads 0 as "Osc 3" -> ch3 = a pitched tone.
Ground truth from the REAL VAZ render of this same patch: f0 = 30 Hz
(= C3 * 2^((42-144)/48) = 130.81 * 0.2291 = 29.97 Hz).

So: isolate ch3 (mute osc1+osc2), render the clone, and the fundamental must be ~30 Hz and TONAL
(strong harmonic peaks), not flat broadband noise.
"""
import os, sys, math, struct, subprocess
import numpy as np
from scipy.io import wavfile
TOOLS = r'C:\APC\y\tools'
sys.path.insert(0, TOOLS)
from gen_trance_presets import trace, landmarks

FIX  = r'C:\APC\y\plugins\VAZClone\tests\fixtures'
REND = r'C:\APC\y\build\plugins\VAZClone\VazRender_artefacts\Release\VazRender.exe'
MIDI = os.path.join(TOOLS, 'abtest', 'midi', '01_sustain_C3.mid')
OUT  = os.path.join(TOOLS, 'abtest', 'wav')
C3   = 130.81

def patch(data, **fields):
    d = bytearray(data)
    prst, _, _ = landmarks(d)
    ver, off, val = trace(d, prst)
    for k, v in fields.items():
        struct.pack_into('<I', d, off[k], int(v) & 0xFFFFFFFF)
    return bytes(d)

def spectrum(path):
    sr, raw = wavfile.read(path)
    x = raw.astype(float)
    if x.ndim > 1: x = x.mean(axis=1)
    if np.issubdtype(raw.dtype, np.integer): x = x / float(np.iinfo(raw.dtype).max)
    n = len(x); pk = float(np.max(np.abs(x))) if n else 0.0
    if pk < 1e-6: return None, -999, []
    a = int(0.8 * sr); b = min(n, a + int(3.0 * sr))
    seg = x[a:b] - np.mean(x[a:b])
    w = np.hanning(len(seg)); S = np.abs(np.fft.rfft(seg * w)); fr = np.fft.rfftfreq(len(seg), 1.0 / sr)
    S[fr < 15] = 0
    mx = S.max() + 1e-12
    top = []
    for i in np.argsort(S)[::-1][:2000]:
        if S[i] / mx < 0.10: break
        if any(abs(fr[i] - f) < 4 for f, _ in top): continue
        top.append((float(fr[i]), float(S[i] / mx)))
        if len(top) >= 6: break
    top_sorted = sorted(top, key=lambda t: t[0])
    f0 = top_sorted[0][0] if top_sorted else None
    # tonality: energy in the top peaks vs total (noise would spread energy everywhere)
    tonal = float(np.sum(np.sort(S)[-40:] ** 2) / (np.sum(S ** 2) + 1e-12))
    return f0, 20 * math.log10(pk), top_sorted, tonal

def run(tag, data):
    p = os.path.join(OUT, f'{tag}.v2p'); open(p, 'wb').write(data)
    w = os.path.join(OUT, f'{tag}.wav')
    r = subprocess.run([REND, p, MIDI, w, '5', '48000'], capture_output=True, text=True, timeout=180)
    if not os.path.exists(w):
        print(f'  {tag}: RENDER FAILED\n{r.stdout[-400:]}{r.stderr[-400:]}'); return
    f0, pk, top, tonal = spectrum(w)
    print(f'  {tag}: peak={pk:.1f}dB  f0={None if f0 is None else round(f0,2)}Hz  tonality={tonal:.3f}')
    print(f'      peaks: {[f"{f:.1f}({m:.2f})" for f, m in top]}')
    return f0

osc3 = open(os.path.join(FIX, 'vaz_mix3_osc3.v2p'), 'rb').read()
ring = open(os.path.join(FIX, 'vaz_mix3_ringmod.v2p'), 'rb').read()
iso  = dict(o1level=0, o2level=0, cutoff=255, reso=0)

print('=== clone render of the USER\'s real VAZ presets (mix3 fix verification) ===')
print(f'expected Osc3 f0 = C3*2^((42-144)/48) = {C3 * 2 ** ((42 - 144) / 48.0):.2f} Hz  (real VAZ measured 30 Hz)\n')
print('-- ch3 ISOLATED (osc1+osc2 muted) --')
f_osc3 = run('MIX3FIX_osc3_isolated', patch(osc3, **iso))
f_ring = run('MIX3FIX_ringmod_isolated', patch(ring, **iso))
print('\n-- full patches, untouched --')
run('MIX3FIX_osc3_full', osc3)

pred = C3 * 2 ** ((42 - 144) / 48.0)
if f_osc3:
    err = abs(f_osc3 - pred) / pred * 100
    print(f'\nRESULT: clone Osc3 f0 = {f_osc3:.2f} Hz vs predicted/real-VAZ {pred:.2f} Hz  -> {err:.1f}% error')
    print('  ' + ('PASS — Osc3 sounds (pitched), matches the real VAZ' if err < 8 else 'FAIL — does not match'))
