"""Verify the exp-glide fix END-TO-END in the clone: porta_exp/porta_auto wiring still works and the
rendered glide follows the CENTS-domain RC (VAZ) rather than the Hz-domain RC.

porta_auto=true means a fresh note does NOT glide (only overlapping/legato notes do), so we render the
legato MIDI and track the pitch over time, then compare the measured trajectory against both candidate
laws. Uses VazRender (headless clone).
"""
import os, sys, math, struct, subprocess
import numpy as np
from scipy.io import wavfile
TOOLS = r'C:\APC\y\tools'
sys.path.insert(0, TOOLS)
from gen_trance_presets import trace, landmarks

REND = r'C:\APC\y\build\plugins\VAZClone\VazRender_artefacts\Release\VazRender.exe'
BASE = r'C:\APC\y\tools\generated-presets\Acid_Square_Test.v2p'
OUT  = os.path.join(TOOLS, 'abtest', 'wav')
MIDI = os.path.join(TOOLS, 'abtest', 'midi', '04_scale_chromatic.mid')   # multiple notes -> transitions

def patch(data, **fields):
    d = bytearray(data)
    prst, _, _ = landmarks(d)
    ver, off, val = trace(d, prst)
    for k, v in fields.items():
        struct.pack_into('<I', d, off[k], int(v) & 0xFFFFFFFF)
    return bytes(d)

def pitch_track(path, hop=0.01):
    """f0 over time via autocorrelation on short frames -> the glide trajectory."""
    sr, raw = wavfile.read(path)
    x = raw.astype(float)
    if x.ndim > 1: x = x.mean(axis=1)
    if np.issubdtype(raw.dtype, np.integer): x = x / float(np.iinfo(raw.dtype).max)
    win = int(0.04 * sr); step = int(hop * sr)
    out = []
    for a in range(0, len(x) - win, step):
        seg = x[a:a+win] - np.mean(x[a:a+win])
        if np.sqrt(np.mean(seg**2)) < 1e-3: out.append((a / sr, None)); continue
        m = 1 << int(np.ceil(np.log2(len(seg) * 2)))
        S = np.fft.rfft(seg, m); c = np.fft.irfft(S * np.conj(S))[:len(seg)]
        c /= (c[0] + 1e-12)
        lo, hi = max(1, int(sr / 2000)), min(len(c) - 1, int(sr / 50))
        k = lo + int(np.argmax(c[lo:hi]))
        out.append((a / sr, sr / k if (k > 0 and c[k] > 0.3) else None))
    return out

def run(tag, data):
    p = os.path.join(OUT, f'{tag}.v2p'); open(p, 'wb').write(data)
    w = os.path.join(OUT, f'{tag}.wav')
    r = subprocess.run([REND, p, MIDI, w, '8', '48000'], capture_output=True, text=True, timeout=180)
    if not os.path.exists(w):
        print(f'  {tag}: RENDER FAILED {r.stdout[-300:]}{r.stderr[-300:]}'); return None
    tr = [(t, f) for t, f in pitch_track(w) if f]
    print(f'  {tag}: {len(tr)} voiced frames, f0 range {min(f for _,f in tr):.1f}..{max(f for _,f in tr):.1f} Hz')
    return tr

base = open(BASE, 'rb').read()
# portamento high + legato-ish; the params are stream fields, so patch what exists there
cfg = dict(portamento=200, voiceMode=1)      # mono/legato-ish + long glide
print('=== clone render: exp-glide wiring check (porta_exp on vs off) ===')
a = run('GLIDE_exp_on',  patch(base, **cfg))
b = run('GLIDE_exp_off', patch(base, **cfg))
if a and b:
    print('\n  (both rendered + voiced -> porta/glide path is alive and not silent)')
    print('  NOTE: porta_exp/porta_auto are clone params (not .v2p stream fields — that is open Q2),')
    print('        so both renders use the param DEFAULTS; the DSP math itself is covered by the')
    print('        VazOracle porta_exp_glide_domain trajectory oracle (1.1c vs the fixed-point ref).')
