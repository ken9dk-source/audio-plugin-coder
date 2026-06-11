"""Faithful simulator of VAZ's REAL .v2p reader FUN_004d6c3c @ 0x4d6c3c (Vaz2010Core.dll).
Version-gated sequential stream + MSmp chunk resync. Validates against known anchors.
Usage: python v2p_trace2.py <file.v2p>   |   python v2p_trace2.py validate
"""
import sys, struct, os, glob

def parse(data):
    prst = data.find(b'PRST')
    ver = struct.unpack_from('<I', data, prst+8)[0]
    pos = [prst + 12]
    out = {}

    def u32():
        v = struct.unpack_from('<i', data, pos[0])[0]; o = pos[0]; pos[0] += 4; return (o, v)
    def byte():
        v = data[pos[0]]; o = pos[0]; pos[0] += 1; return (o, v)
    def modsrc():  # FUN_004d6c18: u32 + (v<200 & val>6 -> +1)
        o, v = u32()
        if ver < 200 and v > 6: v += 1
        return (o, v)
    def strsample():  # v<0x69/0x6a path: d5f4,d5f4,d530(len+N)
        byte(); byte()
        lo, ln = u32(); pos[0] += ln
    def skip_msmp():
        if data[pos[0]:pos[0]+4] == b'MSmp':
            sz = struct.unpack_from('<I', data, pos[0]+4)[0]; pos[0] += 8 + sz
            return True
        return False
    def R(label, kv): out[label] = kv
    v = ver

    if v >= 0x67: R('preset_enable', byte())
    if v >= 0x6d: R('voice_count', u32())
    if v >= 0xc9: R('mono', byte())
    if v >= 0xc9: R('p94', u32())
    R('lfo1_rate', u32()); R('lfo1_wave', u32()); R('lfo1_waveshape', u32()); R('lfo1_retrig', byte())
    if v >= 0xc9: R('ded84', byte())
    if v >= 0xc9: R('pe0', u32())
    R('lfo2_rate', u32())
    if v >= 200:
        R('lfo2_trigsrc', modsrc()); R('lfo2_trigdep', u32())  # d6c18 + d668
    R('lfo2_retrig', byte())
    R('lfo2_sh', u32() if v >= 200 else byte())
    R('lfo2_delay', u32()); R('lfo3_wave', u32()); R('lfo3_x10c', byte())
    # env1
    if v < 0x6b:
        a = u32(); d = u32(); s = u32(); rel = u32(); R('env1_def30', byte()); byte()
        R('env1_attack', a); R('env1_decay', d); R('env1_sustain', s); R('env1_release', rel)
    else:
        R('env1_attack', u32()); R('env1_decay', u32()); R('env1_sustain', u32()); R('env1_release', u32()); R('env1_def30', byte())
    R('env1_def40', byte())
    if v >= 0x6b: R('env1_def50', byte())
    if v >= 0xca: R('env1_def60', byte())
    # env2
    if v < 0x6c:
        a = u32(); d = u32(); s = u32(); rel = u32(); R('env2_df130', byte()); byte()
        R('env2_attack', a); R('env2_decay', d); R('env2_sustain', s); R('env2_release', rel)
    else:
        R('env2_attack', u32()); R('env2_decay', u32()); R('env2_sustain', u32()); R('env2_release', u32()); R('env2_df130', byte())
    R('env2_df140', byte())
    if v >= 0x6c: R('env2_df150', byte())
    if v >= 0xca: R('env2_df160', byte())
    # line 307 block (+0x178 etc.)
    if v >= 200:
        R('df174src', modsrc()); R('df188dep', u32()); R('df1c8', u32())
    R('e0460src', modsrc())
    if v >= 200: R('e0470', byte())
    R('e0480src', modsrc()); R('e0494dep', u32())
    if v >= 200: R('e04d4src', modsrc())
    if v >= 200: R('e04e4src', modsrc())
    # osc1
    R('osc1_tuning', u32()); R('osc1_wave', u32()); R('osc1_x1b0', u32())
    if v >= 200: R('osc1_df430', byte())
    R('osc1_fm1src', modsrc()); R('osc1_fm1dep', u32())
    R('osc1_fm2src', modsrc()); R('osc1_fm2dep', u32())
    R('osc1_pwmsrc', modsrc()); R('osc1_pwmdep', u32())
    # osc1 sample
    if v < 0x69: strsample()
    else:
        R('_msmp1', (pos[0], 1 if skip_msmp() else 0)); R('osc1_sampleflag', byte())
    # osc2
    R('osc2_tuning', u32()); R('osc2_wave', u32()); R('osc1_sync', byte()); R('osc2_mod', u32())
    R('osc2_fm1src', modsrc()); R('osc2_fm1dep', u32())
    R('osc2_fm2src', modsrc()); R('osc2_fm2dep', u32())
    R('osc2_pwmsrc', modsrc()); R('osc2_pwmdep', u32())
    # osc2 sample
    if v < 0x6a: strsample()
    else:
        R('_msmp2', (pos[0], 1 if skip_msmp() else 0)); R('osc2_sampleflag', byte())
    # filter / mixer / output
    if v >= 200: R('dfd2c', u32())
    R('dfd5c', u32()); R('dfd4c', byte())
    if v >= 200: R('dfde4', u32())
    R('dfe28', u32()); R('dfe18', byte())
    R('dfeb0', u32()); R('dfef4', u32()); R('dfee4', byte())
    R('filter_mode', u32()); R('filter_slew', byte()); R('filter_cutoff', u32()); R('filter_reso', u32()); R('filter_bandwidth', u32())
    if v >= 200: R('e01d4', u32())
    R('fcut_m1src', modsrc()); R('fcut_m1dep', u32())
    R('fcut_m2src', modsrc()); R('fcut_m2dep', u32())
    R('fcut_m3src', modsrc()); R('fcut_m3dep', u32())
    R('freso_src', modsrc()); R('freso_dep', u32())
    R('amp_am1src', modsrc()); R('amp_am1dep', u32())
    R('amp_am2src', modsrc()); R('amp_am2dep', u32())
    if v >= 200: R('amp_am3src', modsrc()); R('amp_am3dep', u32())
    R('overdrive', u32())
    if v >= 0x65: R('e04f4src', modsrc())
    if v >= 0x65: R('e0504', u32())
    R('e0530', u32()); R('voice_mode', u32()); R('e0600', byte()); R('e0610', u32())
    if v >= 200: R('e095c', u32())
    R('p2f4', u32())
    if v >= 200: R('p2f0', u32())
    R('portamento', u32()); R('e09b8', byte()); R('e09c8', byte())
    return ver, out, pos[0], prst


def regionc_anchor(data):
    prst = data.find(b'PRST'); ms1 = data.find(b'MSmp'); ms2 = data.find(b'MSmp', ms1+4)
    s2 = struct.unpack_from('<I', data, ms2+4)[0]
    return ms2 + 8 + s2  # sec3


if __name__ == '__main__':
    if sys.argv[1] == 'checkall':
        root = r'C:\Program Files (x86)\Steinberg\Vstplugins\VAZ Synths\VAZ 2010\Patches'
        import collections
        bad = []; n_ok = 0; byver = collections.Counter()
        modehist = collections.Counter()
        for dp, dn, fn in os.walk(root):
            for name in fn:
                if not name.lower().endswith('.v2p'): continue
                p = os.path.join(dp, name)
                d = open(p, 'rb').read()
                try:
                    ver, out, endpos, prst = parse(d)
                    sec3 = regionc_anchor(d)
                    fm = out['filter_mode'][1]; cut = out['filter_cutoff'][1]; res = out['filter_reso'][1]
                    e1s = out['env1_sustain'][1]; ovr = out['overdrive'][1]
                    prend = prst + 12 + struct.unpack_from('<I', d, prst+4)[0]
                    issues = []
                    if not (0 <= fm <= 21): issues.append('mode=%d' % fm)
                    if not (0 <= cut <= 255): issues.append('cut=%d' % cut)
                    if not (0 <= res <= 255): issues.append('res=%d' % res)
                    if not (0 <= e1s <= 255): issues.append('e1s=%d' % e1s)
                    if not (0 <= ovr <= 255): issues.append('ovr=%d' % ovr)
                    # filter_mode read offset must equal where region C anchors expect it
                    if out['filter_cutoff'][0] >= prend: issues.append('overran')
                    byver[ver] += 1; modehist[fm] += 1
                    if issues: bad.append((os.path.relpath(p, root), ver, issues))
                    else: n_ok += 1
                except Exception as e:
                    bad.append((os.path.relpath(p, root), '?', [repr(e)]))
        print('OK: %d   BAD: %d' % (n_ok, len(bad)))
        print('by version:', dict(sorted(byver.items())))
        print('filter_mode histogram:', dict(sorted(modehist.items())))
        for relp, ver, iss in bad[:40]:
            print('  BAD ver=%s %s -> %s' % (ver, relp, iss))
    elif sys.argv[1] == 'validate':
        root = r'C:\Program Files (x86)\Steinberg\Vstplugins\VAZ Synths\VAZ 2010\Patches'
        samples = {201: '2.0 Features\\Filter K.v2p', 108: 'Bass\\AH Moog.v2p', 106: 'Bass\\bass 1.v2p',
                   202: '2.0 Demos\\101 Full On Bass.v2p'}
        for ver, rel in samples.items():
            p = os.path.join(root, rel)
            d = open(p, 'rb').read()
            realver, out, endpos, prst = parse(d)
            PS = prst + 12; sec3 = regionc_anchor(d)
            fm = out.get('filter_mode'); cut = out.get('filter_cutoff'); res = out.get('filter_reso')
            e1a = out.get('env1_attack'); e1s = out.get('env1_sustain')
            print('ver=%-3d %-28s  filter_mode@sec3+%-3d=%-2d cutoff@sec3+%-3d=%-3d reso@sec3+%-3d=%-3d  env1A=%s env1S=%s' % (
                realver, os.path.basename(p),
                fm[0]-sec3, fm[1], cut[0]-sec3, cut[1], res[0]-sec3, res[1],
                e1a[1] if e1a else '?', e1s[1] if e1s else '?'))
    else:
        d = open(sys.argv[1], 'rb').read()
        ver, out, endpos, prst = parse(d)
        PS = prst + 12; sec3 = regionc_anchor(d)
        print('ver=%d  PS=0x%x sec3=0x%x  endpos=0x%x' % (ver, PS, sec3, endpos))
        for k, (o, val) in out.items():
            anc = 'sec3+%d' % (o - sec3) if o >= sec3 else 'PS+%d' % (o - PS)
            print('  %-16s @0x%04x (%-9s) = %d' % (k, o, anc, val))
