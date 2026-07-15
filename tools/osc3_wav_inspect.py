"""Inspect what is ACTUALLY in the rendered WAVs before trusting any pitch number."""
import sys, math
import numpy as np
from scipy.io import wavfile

def report(path):
    sr, raw = wavfile.read(path)
    x = raw.astype(float)
    if x.ndim > 1: x = x.mean(axis=1)
    if np.issubdtype(raw.dtype, np.integer): x = x / float(np.iinfo(raw.dtype).max)
    n = len(x)
    pk = float(np.max(np.abs(x))) if n else 0.0
    rms = float(np.sqrt(np.mean(x ** 2))) if n else 0.0
    print(f'\n== {path.split(chr(92))[-1]}')
    print(f'   dtype={raw.dtype} sr={sr} dur={n/sr:.2f}s  peak={20*math.log10(max(pk,1e-12)):.1f}dBFS  rms={20*math.log10(max(rms,1e-12)):.1f}dBFS')
    if pk < 1e-5:
        print('   -> SILENT'); return
    # RMS envelope over time (is there a note at all?)
    w = int(0.25 * sr); env = [20*math.log10(max(float(np.sqrt(np.mean(x[i:i+w]**2))),1e-12)) for i in range(0, n-w, w)]
    print('   rms env (0.25s):', ' '.join(f'{e:.0f}' for e in env[:16]))
    # spectrum of the sustained part
    a = int(1.0 * sr); b = min(n, a + int(1.5 * sr))
    seg = x[a:b]
    if len(seg) < 1024: print('   too short'); return
    seg = seg - seg.mean()
    win = np.hanning(len(seg))
    S = np.abs(np.fft.rfft(seg * win))
    fr = np.fft.rfftfreq(len(seg), 1.0 / sr)
    # top spectral peaks
    idx = np.argsort(S)[::-1][:400]
    peaks = []
    for i in sorted(idx):
        if fr[i] < 15: continue
        if any(abs(fr[i] - p) < 8 for p, _ in peaks): continue
        peaks.append((fr[i], S[i]))
    peaks.sort(key=lambda t: -t[1])
    print('   top spectral peaks (Hz):', ', '.join(f'{f:.1f}({s/S.max():.2f})' for f, s in peaks[:8]))

for p in sys.argv[1:]:
    try: report(p)
    except Exception as e: print(p, 'ERR', e)
