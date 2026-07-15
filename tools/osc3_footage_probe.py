"""osc3_footage_probe.py — AUDIO-RENDER extraction of the REAL VAZ's Osc3 footage->pitch law (#9).

Base = the USER's VAZ-native "Osc 3.v2p" (Osc3 actually selected in the mixer, so it does not crash).
DECODED from the user's Osc3-vs-RingMod pair: real mix3src 0 = Oscillator 3, 1 = Ring Modulator
(the clone's enum is off by one — it thinks 1 = Osc3).

Renders via VAZ File->Capture (the LOOPBACK path captures pure silence in this environment and must
NOT be used — the harness peak-normalises on write, which makes silence look like signal).

Question: WHICH field drives Osc3 pitch, and by what law?
  clone      : osc3FootMul = 2^((lfo1rate-144)/48)      (PluginProcessor.cpp:1056 — uses LFO1 RATE)
  real render: inc = 2^31*48*rateVal / (60*noteVal*FOOTAGE)   (FOOTAGE = the +0x94 field, a DIVISOR)
So we sweep +0x94 and lfo1rate INDEPENDENTLY and see which one moves the pitch, and how.
"""
import os, sys, time, math, struct, shutil
import numpy as np
from scipy.io import wavfile
TOOLS = r'C:\APC\y\tools'
sys.path.insert(0, TOOLS); sys.path.insert(0, os.path.join(TOOLS, 'vaz_auto'))
from gen_trance_presets import trace, landmarks
from vaz_auto import VazAuto

BASE  = r'C:\Users\ken98\Desktop\Osc 3.v2p'          # user-made, Osc3 selected in the mixer
DESK  = os.path.dirname(BASE)
WORK  = os.path.join(DESK, 'osc3probe'); os.makedirs(WORK, exist_ok=True)
MIDI  = os.path.join(TOOLS, 'abtest', 'midi', '01_sustain_C3.mid')

def patch(base_bytes, **fields):
    """Minimal patcher: set ONLY the named fields at their traced offsets. No base-grid, so the
    user's patch is otherwise untouched. 'foot' = the unnamed +0x94 u32 (lives at lfo1rate-4)."""
    d = bytearray(base_bytes)
    prst, _, _ = landmarks(d)
    ver, off, val = trace(d, prst)
    for k, v in fields.items():
        if k == 'foot':
            o = off['lfo1rate'] - 4
        else:
            if k not in off: raise KeyError(k)
            o = off[k]
        struct.pack_into('<I', d, o, int(v) & 0xFFFFFFFF)
    return bytes(d)

def analyse(path):
    if not os.path.exists(path): return None
    sr, raw = wavfile.read(path)
    x = raw.astype(float)
    if x.ndim > 1: x = x.mean(axis=1)
    if np.issubdtype(raw.dtype, np.integer): x = x / float(np.iinfo(raw.dtype).max)
    n = len(x)
    if n < sr // 2: return dict(peak=-999, f0=None, peaks=[], dur=n / max(sr,1))
    pk = float(np.max(np.abs(x))); pkdb = 20 * math.log10(max(pk, 1e-12))
    if pk < 1e-5: return dict(peak=pkdb, f0=None, peaks=[], dur=n / sr)
    a = int(0.6 * sr); bnd = min(n, a + int(2.0 * sr))
    seg = x[a:bnd] - np.mean(x[a:bnd])
    if len(seg) < 2048: return dict(peak=pkdb, f0=None, peaks=[], dur=n / sr)
    m = 1 << int(np.ceil(np.log2(len(seg) * 2)))
    S = np.fft.rfft(seg, m); c = np.fft.irfft(S * np.conj(S))[:len(seg)]; c /= (c[0] + 1e-12)
    lo = max(1, int(sr / 5000)); hi = min(len(c) - 1, int(sr / 20))
    f0 = None
    if hi > lo:
        k = lo + int(np.argmax(c[lo:hi]))
        if k > 0 and c[k] > 0.15: f0 = sr / k
    w = np.hanning(len(seg)); Sp = np.abs(np.fft.rfft(seg * w)); fr = np.fft.rfftfreq(len(seg), 1.0 / sr)
    top = []
    for i in np.argsort(Sp)[::-1][:800]:
        if fr[i] < 20: continue
        if any(abs(fr[i] - f) < 6 for f, _ in top): continue
        top.append((float(fr[i]), float(Sp[i] / (Sp.max() + 1e-12))))
        if len(top) >= 6: break
    return dict(peak=pkdb, f0=f0, peaks=top, dur=n / sr)

def main():
    base = open(BASE, 'rb').read()
    prst, _, _ = landmarks(bytearray(base))
    ver, off, val = trace(bytearray(base), prst)
    print(f'base "{os.path.basename(BASE)}": ver={ver} mix3src={val.get("mix3src")} '
          f'foot(+0x94)={struct.unpack_from("<I", base, off["lfo1rate"]-4)[0]} lfo1rate={val.get("lfo1rate")} '
          f'o1level={val.get("o1level")} o2level={val.get("o2level")}')

    jobs = []
    # STAGE 1 — baseline: the user's patch untouched (must render + not crash)
    jobs.append(('S1_base_untouched', base))
    # STAGE 2 — isolate Osc3: mute osc1+osc2, open the filter (one minimal change set)
    iso = dict(o1level=0, o2level=0, cutoff=255, reso=0)
    jobs.append(('S2_osc3_isolated', patch(base, **iso)))
    # STAGE 3 — sweep +0x94 FOOTAGE (the real divisor), lfo1rate held at the user's value
    for f in (48, 96, 144, 192, 240):
        jobs.append((f'S3_foot{f}', patch(base, foot=f, **iso)))
    # STAGE 4 — sweep lfo1rate (what the CLONE uses), footage held at 96
    for r in (48, 96, 144, 192, 240):
        jobs.append((f'S4_rate{r}', patch(base, foot=96, lfo1rate=r, **iso)))

    vaz = VazAuto(midi_hint='loop').launch()
    res = {}
    try:
        for tag, data in jobs:
            pth = os.path.join(WORK, f'{tag}.v2p'); open(pth, 'wb').write(data)
            out = os.path.join(WORK, f'{tag}.wav')
            print(f'--- {tag} ---')
            try:
                vaz.open_patch(pth); time.sleep(0.8)
                vaz.capture_render(MIDI, out); time.sleep(0.3)
                r = analyse(out)
            except Exception as e:
                r = None; print('   ERROR:', e)
            res[tag] = r
            if r: print(f"   peak={r['peak']:.1f}dB f0={None if r['f0'] is None else round(r['f0'],2)}Hz "
                        f"peaks={[f'{f:.0f}' for f,_ in r['peaks']]}")
    finally:
        vaz.close()

    b = res.get('S1_base_untouched')
    print('\n=== STAGE 1 baseline (must be real audio, else rig untrusted) ===\n   ', b)
    print('=== STAGE 2 Osc3 isolated ===\n   ', res.get('S2_osc3_isolated'))
    print('\n=== STAGE 3: sweep +0x94 FOOTAGE (real divisor), lfo1rate fixed ===')
    print('  foot     f0(Hz)     ratio vs foot96   1/f (divisor?)   2^((f-144)/48)')
    r96 = (res.get('S3_foot96') or {}).get('f0')
    for f in (48, 96, 144, 192, 240):
        r = res.get(f'S3_foot{f}') or {}; f0v = r.get('f0')
        rat = (f0v / r96) if (f0v and r96) else None
        inv = 96.0 / f
        cl = 2.0 ** ((f - 144) / 48.0)
        print(f'  {f:4d}  {("%9.2f"%f0v) if f0v else "     None"}   {("%9.4f"%rat) if rat else "       --"}       {inv:8.4f}       {cl:8.4f}')
    print('\n=== STAGE 4: sweep lfo1rate (what the CLONE uses), footage fixed=96 ===')
    print('  rate     f0(Hz)     ratio vs rate144   clone 2^((r-144)/48)   ratecurve e^(0.036(r-144))')
    r144 = (res.get('S4_rate144') or {}).get('f0')
    for r_ in (48, 96, 144, 192, 240):
        r = res.get(f'S4_rate{r_}') or {}; f0v = r.get('f0')
        rat = (f0v / r144) if (f0v and r144) else None
        cl = 2.0 ** ((r_ - 144) / 48.0); rc = math.exp(0.036 * (r_ - 144))
        print(f'  {r_:4d}  {("%9.2f"%f0v) if f0v else "     None"}   {("%9.4f"%rat) if rat else "       --"}        {cl:11.4f}        {rc:14.4f}')

if __name__ == '__main__':
    main()
