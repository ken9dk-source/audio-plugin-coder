import sys, os

def finds(b, t):
    o = []; i = 0
    while True:
        i = b.find(t, i)
        if i < 0: break
        o.append(i); i += 1
    return o

def hexdump(d, start, length, base=None):
    base = start if base is None else base
    for i in range(start, min(start+length, len(d)), 16):
        row = d[i:i+16]
        print('%06x' % i, ' '.join('%02x' % x for x in row).ljust(48),
              ''.join(chr(x) if 32 <= x < 127 else '.' for x in row))

def inspect_patch(f):
    d = open(f, 'rb').read()
    print('FILE', os.path.basename(f), 'SIZE', len(d))
    tags = {}
    for t in (b'PRST', b'MSmp', b'VAZ', b'Pres', b'Samp'):
        tags[t] = finds(d, t)
        print('  tag', t, tags[t])
    prst = tags[b'PRST'][0] if tags[b'PRST'] else -1
    mss  = tags[b'MSmp']
    print('--- header (first 0x40) ---'); hexdump(d, 0, 0x40)
    if prst >= 0:
        print('--- around PRST @ 0x%x ---' % prst); hexdump(d, prst, 0x60)
    for k, ms in enumerate(mss[:2]):
        print('--- around MSmp[%d] @ 0x%x ---' % (k, ms)); hexdump(d, ms, 0x40)
    return d

def search_bins():
    for path in (r'tools\Vaz2010Core.dll',
                 r'C:\Program Files (x86)\Steinberg\Vstplugins\VAZ Synths\VAZ 2010\Vaz2010.exe',
                 r'tools\VAZ2010Effect.dll'):
        try:
            d = open(path, 'rb').read()
        except Exception as e:
            print('===', path, 'ERR', e); continue
        print('===', path, 'size', len(d), '===')
        for t in (b'PRST', b'MSmp', b'.v2p', b'v2p', b'Patch', b'PRESET', b'TPreset', b'TPatch'):
            hits = finds(d, t)
            extra = ('(+%d more)' % (len(hits)-8)) if len(hits) > 8 else ''
            print('  ', t, '->', [hex(h) for h in hits[:8]], extra)

def kw_strings(path, keywords, minlen=4):
    """Extract printable ASCII strings from a binary and show ones matching keywords."""
    import re
    d = open(path, 'rb').read()
    strs = re.findall(rb'[\x20-\x7e]{%d,}' % minlen, d)
    seen = set()
    hits = {k: [] for k in keywords}
    for s in strs:
        t = s.decode('latin1')
        for k in keywords:
            if k.lower() in t.lower() and t not in seen:
                hits[k].append(t)
    for k in keywords:
        uniq = []
        for t in hits[k]:
            if t not in uniq: uniq.append(t)
        print('### %s (%d)' % (k, len(uniq)))
        for t in uniq[:40]:
            print('    ', t)


def pe_map(path, file_offsets):
    """Map file offsets -> VA using the PE section table."""
    import struct as st
    d = open(path, 'rb').read()
    e_lfanew = st.unpack_from('<I', d, 0x3c)[0]
    assert d[e_lfanew:e_lfanew+4] == b'PE\x00\x00', 'not PE'
    coff = e_lfanew + 4
    nsec = st.unpack_from('<H', d, coff+2)[0]
    sizeopt = st.unpack_from('<H', d, coff+16)[0]
    opt = coff + 20
    magic = st.unpack_from('<H', d, opt)[0]
    imgbase = st.unpack_from('<I', d, opt+28)[0] if magic == 0x10b else st.unpack_from('<Q', d, opt+24)[0]
    print('ImageBase = 0x%x  nsec=%d' % (imgbase, nsec))
    secs = []
    sh = opt + sizeopt
    for i in range(nsec):
        base = sh + i*40
        name = d[base:base+8].rstrip(b'\x00').decode('latin1')
        vsize, va, rawsize, rawptr = st.unpack_from('<IIII', d, base+8)
        secs.append((name, va, vsize, rawptr, rawsize))
        print('  sec %-8s VA=0x%06x VSize=0x%06x RawPtr=0x%06x RawSize=0x%06x' % (name, va, vsize, rawptr, rawsize))
    out = {}
    for fo in file_offsets:
        for name, va, vsize, rawptr, rawsize in secs:
            if rawptr <= fo < rawptr + rawsize:
                rva = fo - rawptr + va
                out[fo] = imgbase + rva
                print('  fileoff 0x%06x -> VA 0x%08x  (sec %s)' % (fo, imgbase+rva, name))
                break
        else:
            print('  fileoff 0x%06x -> NOT IN ANY SECTION' % fo)
    return out

if __name__ == '__main__':
    if len(sys.argv) > 1 and sys.argv[1] == 'readva':
        import struct as st
        va = int(sys.argv[2], 0); n = int(sys.argv[3]) if len(sys.argv) > 3 else 8
        d = open(r'tools\Vaz2010Core.dll', 'rb').read()
        # PE map: DATA section VA 0x127000 RawPtr 0x125e00 (imagebase 0x400000)
        rva = va - 0x400000
        fo = rva - 0x127000 + 0x125e00
        print('VA 0x%x -> fileoff 0x%x' % (va, fo))
        for i in range(n):
            v = st.unpack_from('<i', d, fo + i*4)[0]
            print('  [%d] = %d (0x%x)' % (i, v, v & 0xffffffff))
    elif len(sys.argv) > 1 and sys.argv[1] == 'kw':
        kws = ['Arp', 'Sequenc', 'Step', 'Pattern', 'Swing', 'Unison', 'Glide', 'Portamento',
               'LFO', 'Chorus', 'Delay', 'Reverb', 'Distort', 'Overdrive', 'Sync', 'Ring',
               'Sample', 'Multisaw', 'Sub ', 'Noise', 'Velocity', 'Aftertouch', 'Wheel',
               'Bend', 'Tempo', 'Modifier', 'Slew', 'Bandwidth', 'Crossfade', 'One Shot',
               'No Trigger', 'Sample And Hold', 'Trig', 'Octave', 'Detune', 'Spread', 'Pan',
               'Comb', 'Formant', 'Drive', 'Resonance', 'Keyboard', 'Mono', 'Legato', 'Hold',
               'Mode', 'Source', 'Depth', 'Waveform', 'WaveShape']
        kw_strings(r'tools\Vaz2010Core.dll', kws)
    elif len(sys.argv) > 1 and sys.argv[1] == 'bins':
        search_bins()
    elif len(sys.argv) > 1 and sys.argv[1] == 'regions':
        import struct as st
        for f in sys.argv[2:]:
            d = open(f, 'rb').read()
            prst = d.find(b'PRST'); ver = st.unpack_from('<I', d, prst+8)[0]
            ms1 = d.find(b'MSmp'); s1 = st.unpack_from('<I', d, ms1+4)[0]
            ms2 = d.find(b'MSmp', ms1+4); s2 = st.unpack_from('<I', d, ms2+4)[0]
            PS = prst+12; A_end = ms1; B0 = ms1+8+s1; B_end = ms2; C0 = ms2+8+s2
            pend = prst+12+st.unpack_from('<I', d, prst+4)[0]
            print('=== %-26s ver=%-3d  A[0x%x..0x%x]=%dB  MSmp1 sz=%d  B[0x%x..0x%x]=%dB  MSmp2 sz=%d  C[0x%x..0x%x]'
                  % (os.path.basename(f)[:26], ver, PS, A_end, A_end-PS, s1, B0, B_end, B_end-B0, s2, C0, pend))
            print('  region C (filter/mix/out) first 64B from C0=0x%x:' % C0); hexdump(d, C0, 64)
    elif len(sys.argv) > 1 and sys.argv[1] == 'hex':
        f = sys.argv[2]; start = int(sys.argv[3], 0); length = int(sys.argv[4], 0) if len(sys.argv) > 4 else 0x80
        d = open(f, 'rb').read()
        prst = d.find(b'PRST')
        print(os.path.basename(f), 'PRST@0x%x  PS=0x%x  (showing 0x%x..)' % (prst, prst+12, start))
        hexdump(d, start, length)
    elif len(sys.argv) > 1 and sys.argv[1] == 'survey':
        import struct as st, collections
        root = sys.argv[2]
        vers = collections.Counter()
        bymss = collections.Counter()
        sampled = 0; total = 0
        examples = {}
        for dp, dn, fn in os.walk(root):
            for name in fn:
                if not name.lower().endswith('.v2p'): continue
                p = os.path.join(dp, name)
                try: d = open(p, 'rb').read()
                except: continue
                total += 1
                prst = d.find(b'PRST')
                if prst < 0: vers['NO_PRST'] += 1; continue
                ver = st.unpack_from('<I', d, prst+8)[0]
                vers[ver] += 1
                examples.setdefault(ver, p)
                # count MSmp + whether any has size != 548 (a real sample)
                i = 0; sizes = []
                while True:
                    i = d.find(b'MSmp', i)
                    if i < 0: break
                    sizes.append(st.unpack_from('<I', d, i+4)[0]); i += 1
                bymss[len(sizes)] += 1
                if any(s != 548 for s in sizes): sampled += 1
        print('TOTAL .v2p:', total)
        print('versions:', dict(sorted(vers.items(), key=lambda kv: str(kv[0]))))
        print('MSmp count distribution:', dict(bymss))
        print('patches with a non-548 MSmp (real sample):', sampled)
        print('example file per version:')
        for v, p in sorted(examples.items(), key=lambda kv: str(kv[0])):
            print('  ver %-5s %s' % (v, os.path.relpath(p, root)))
    elif len(sys.argv) > 1 and sys.argv[1] == 'hdr':
        import struct as st
        for f in sys.argv[2:]:
            d = open(f, 'rb').read()
            def u32(o): return st.unpack_from('<I', d, o)[0] if o+4 <= len(d) else -1
            prst = d.find(b'PRST')
            mss = []
            i = 0
            while True:
                i = d.find(b'MSmp', i)
                if i < 0: break
                mss.append((i, u32(i+4)))   # (pos, size)
                i += 1
            pv = u32(prst+8) if prst >= 0 else -1
            psz = u32(prst+4) if prst >= 0 else -1
            print('%-34s size=%6d  PRST@0x%-5x sz=%-6d ver=%-4d  MSmp=%s' %
                  (os.path.basename(f)[:34], len(d), prst, psz, pv,
                   ' '.join('@0x%x/sz%d' % (p, s) for p, s in mss)))
    elif len(sys.argv) > 1 and sys.argv[1] == 'map':
        f = sys.argv[2]
        d = open(f, 'rb').read()
        def find(t, frm=0):
            i = d.find(t, frm); return i
        prst = find(b'PRST'); ms1 = find(b'MSmp'); ms2 = find(b'MSmp', ms1+4)
        PS = prst + 12; sec2 = ms1 + 8 + 548; sec3 = ms2 + 8 + 548
        def B(o): return d[o] if 0 <= o < len(d) else 0
        def L16(o): return B(o) | (B(o+1) << 8)
        print('FILE', os.path.basename(f), 'prst=0x%x ms1=0x%x ms2=0x%x  PS=0x%x sec2=0x%x sec3=0x%x' % (prst,ms1,ms2,PS,sec2,sec3))
        rows = [
          ('filter_mode','B',sec3+28),('cutoff','B',sec3+33),('resonance','B',sec3+37),
          ('overdrive','B',sec3+105),('noise_level','B',sec3+23),('o2_level','B',sec3+14),
          ('voice_mode','B',sec3+117),('portamento','B',sec3+142),('uni_detune','B',sec3+134),
          ('hp_cutoff','B',sec3+45),('flt_aux','B',sec3+41),
          ('cut_mod1_src','B',sec3+49),('filt_env_amt','B',sec3+53),
          ('cut_mod2_src','B',sec3+57),('lfo_amt','B',sec3+61),
          ('res_mod_src','B',sec3+73),('res_mod_amt','B',sec3+77),
          ('pan_mod_src','B',sec3+97),('pan_mod_amt','B',sec3+101),
          ('e1_attack','L16',PS+54),('e1_decay','L16',PS+58),('e1_sustain','B',PS+62),('e1_release','L16',PS+66),
          ('e2_attack','L16',PS+74),('e2_decay','L16',PS+78),('e2_sustain','B',PS+82),('e2_release','L16',PS+86),
          ('lfo_rate','B',PS+10),('o1_wave','B',PS+131),('o1_shape','B',PS+135),
          ('o1_tune','L16',PS+127),('o2_tune','L16',sec2+1),('o2_wave','B',sec2+5),
          ('amp_mod_src','B',PS+111),('o1_fm_src','B',PS+140),('o1_fm_amt','B',PS+144),
          ('o1_ws_src','B',PS+156),('o1_ws_amt','B',PS+160),
          ('e1_mode','B',PS+71),('e2_mode','B',PS+91),('lfo_sync','B',PS+5),('lfo2_rate','B',PS+28),
        ]
        for name, kind, off in rows:
            v = L16(off) if kind == 'L16' else B(off)
            print('  %-14s %-4s @0x%04x (off+%d) = %5d  0x%04x' % (name, kind, off, off-(PS if off>=PS and off<sec2 else sec3 if off>=sec3 else sec2), v, v))
    elif len(sys.argv) > 1 and sys.argv[1] == 'va':
        path = r'tools\Vaz2010Core.dll'
        d = open(path, 'rb').read()
        offs = []
        for t in (b'PRST', b'MSmp', b'.v2p', b'v2p'):
            offs += finds(d, t)
        pe_map(path, sorted(set(offs)))
    else:
        f = sys.argv[1] if len(sys.argv) > 1 else \
            r'C:\Program Files (x86)\Steinberg\Vstplugins\VAZ Synths\VAZ 2010\Patches\2.0 Features\Filter K.v2p'
        inspect_patch(f)
