"""Control: is the capture path working at all, and is the Osc3 preset genuinely silent?
Renders (a) the UNMODIFIED template (o1level=255 -> osc1 should be loud), (b) a mix3src=1 Osc3 preset,
(c) an osc1-audible + Osc3 preset. Compares true peak dBFS BEFORE any normalisation."""
import os, sys, time, math
import numpy as np
from scipy.io import wavfile
TOOLS = r'C:\APC\y\tools'
sys.path.insert(0, TOOLS); sys.path.insert(0, os.path.join(TOOLS, 'vaz_auto'))
from gen_trance_presets import build_patch
from vaz_auto import VazAuto

TEMPLATE = os.path.join(TOOLS, 'generated-presets', 'Acid_Square_Test.v2p')
PATCHDIR = os.path.join(TOOLS, 'abtest', 'test_patches')
WAVDIR   = os.path.join(TOOLS, 'abtest', 'wav')
MIDI     = os.path.join(TOOLS, 'abtest', 'midi', '01_sustain_C3.mid')
DUR      = 4.7

def truepeak(path):
    sr, x = wavfile.read(path)
    x = x.astype(float)
    if x.ndim > 1: x = x.mean(axis=1)
    if np.issubdtype(wavfile.read(path)[1].dtype, np.integer): x = x / 32768.0
    pk = float(np.max(np.abs(x))) if len(x) else 0.0
    return 20 * math.log10(max(pk, 1e-12)), len(x) / sr

def main():
    tmpl = open(TEMPLATE, 'rb').read()
    cases = {}
    # (a) unmodified template — control: osc1 at 255 should be clearly audible
    p = os.path.join(PATCHDIR, 'ctl_template.v2p'); open(p, 'wb').write(tmpl); cases['A_template_unmodified'] = p
    # (b) Osc3 only (what the probe used)
    d = build_patch(tmpl, dict(o1level=0, o2level=0, mix3src=1, noise=255, lfo1rate=144,
                               lfo1wave=0, lfo1shape=127, cutoff=255, reso=0,
                               fcut2d=0, fcut3d=0, am2d=0, o1pwmd=0, o2pwmd=0), name='CTLOSC3')
    p = os.path.join(PATCHDIR, 'ctl_osc3_only.v2p'); open(p, 'wb').write(d); cases['B_osc3_only'] = p
    # (c) osc1 audible AND mix3=Osc3 — isolates "is it the mute or the Osc3 routing?"
    d = build_patch(tmpl, dict(o1level=255, o2level=0, mix3src=1, noise=255, lfo1rate=144,
                               lfo1wave=0, lfo1shape=127, cutoff=255, reso=0,
                               fcut2d=0, fcut3d=0, am2d=0, o1pwmd=0, o2pwmd=0), name='CTLBOTH')
    p = os.path.join(PATCHDIR, 'ctl_osc1_plus_osc3.v2p'); open(p, 'wb').write(d); cases['C_osc1+osc3'] = p
    # (d) build_patch baseline with NO overrides — is build_patch itself producing silence?
    d = build_patch(tmpl, dict(), name='CTLBASE')
    p = os.path.join(PATCHDIR, 'ctl_buildpatch_base.v2p'); open(p, 'wb').write(d); cases['D_buildpatch_base'] = p

    vaz = VazAuto(midi_hint='loop').launch()
    try:
        for tag, path in cases.items():
            out = os.path.join(WAVDIR, f'{tag}.wav')
            print(f'--- {tag} ---')
            try:
                vaz.render(path, MIDI, out, DUR)
                time.sleep(0.4)
                pk, dur = truepeak(out)
                print(f'   TRUE peak = {pk:7.1f} dBFS   ({dur:.1f}s)')
            except Exception as e:
                print('   error:', e)
    finally:
        vaz.close()
    print('\nIf A (unmodified template) is also ~-177 dBFS -> the CAPTURE path is broken (not the preset).')
    print('If A is loud but B is silent  -> Osc3 routing/mute is wrong (preset issue).')

if __name__ == '__main__':
    main()
