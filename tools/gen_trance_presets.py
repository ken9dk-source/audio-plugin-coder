#!/usr/bin/env python3
# Generate VAZClone trance presets as v2.0 .v2p files.
# Strategy: take a real v2.0 (1565-byte) preset as a template and write parameter values at the
# EXACT byte offsets the clone's parseV2P reads them from. The offset map is produced by porting
# parseV2P (PluginProcessor.cpp) 1:1 in Python, instrumented to record each field's cursor offset.
# This guarantees the clone reads back exactly what we write (it's the same reader logic).
import struct, os, sys

def find_tag(d, t, frm=0):
    t = t.encode() if isinstance(t, str) else t
    i = d.find(t, max(0, frm))
    return i

class Cursor:
    def __init__(self, d, pos):
        self.d = d; self.n = len(d); self.pos = pos
        self.off = {}      # field name -> byte offset where its read started
    def u32(self, name=None):
        if name: self.off[name] = self.pos
        p = self.pos
        v = int.from_bytes(self.d[p:p+4], 'little') if p+4 <= self.n else 0
        self.pos += 4; return v
    def byte(self, name=None):
        if name: self.off[name] = self.pos
        v = self.d[self.pos] if 0 <= self.pos < self.n else 0
        self.pos += 1; return v
    def modsrc(self, ver, name=None):
        if name: self.off[name] = self.pos
        p = self.pos
        v = int.from_bytes(self.d[p:p+4], 'little') if p+4 <= self.n else 0
        self.pos += 4
        if ver < 200 and v > 6: v += 1
        return v
    def strsample(self):
        self.byte(); self.byte(); ln = self.u32(); self.pos += ln
    def skipMsmp(self):
        p = self.pos
        if p+8 <= self.n and self.d[p:p+4] == b'MSmp':
            self.pos += 8 + int.from_bytes(self.d[p+4:p+8], 'little')

# Port of parseV2P — returns (offsets dict, parsed-values dict). Mirrors the C++ exactly.
def trace(d, prst):
    ver = int.from_bytes(d[prst+8:prst+12], 'little')
    v = ver
    c = Cursor(d, prst+12)
    P = {}
    if v >= 0x67: P['mono'] = c.byte('mono')
    if v >= 0x6d: c.u32()
    if v >= 0xc9: c.byte()
    if v >= 0xc9: c.u32()
    P['lfo1rate'] = c.u32('lfo1rate')
    c.u32('lfo1wave')
    P['lfo1shape'] = c.u32('lfo1shape'); c.byte('lfo1trig')
    if v >= 0xc9: c.byte()
    if v >= 0xc9: c.u32()
    P['lfo2rate'] = c.u32('lfo2rate')
    if v >= 200: c.modsrc(v); c.u32()
    c.byte('lfo2trig')
    if v >= 200: c.u32('lfo2mode')
    else: c.byte()
    c.u32('lfo2delay'); c.u32('lfo3sel'); c.byte('lfo3wav')
    # env1
    if v < 0x6b:
        P['e1a']=c.u32('e1a'); P['e1d']=c.u32('e1d'); P['e1s']=c.u32('e1s'); P['e1r']=c.u32('e1r'); c.byte(); c.byte()
    else:
        P['e1a']=c.u32('e1a'); P['e1d']=c.u32('e1d'); P['e1s']=c.u32('e1s'); P['e1r']=c.u32('e1r'); c.byte()
    c.byte('e1mode')
    if v >= 0x6b: c.byte()
    if v >= 0xca: c.byte()
    # env2
    if v < 0x6c:
        P['e2a']=c.u32('e2a'); P['e2d']=c.u32('e2d'); P['e2s']=c.u32('e2s'); P['e2r']=c.u32('e2r'); c.byte(); c.byte()
    else:
        P['e2a']=c.u32('e2a'); P['e2d']=c.u32('e2d'); P['e2s']=c.u32('e2s'); P['e2r']=c.u32('e2r'); c.byte()
    c.byte()
    if v >= 0x6c: c.byte('e2mode')
    if v >= 0xca: c.byte()
    if v >= 200:
        c.modsrc(v, 'e2modsrc'); c.u32('e2modamt'); c.u32('e2moddest')
    c.modsrc(v, 'ma1in')
    if v >= 200: c.byte('ma1sq')
    c.modsrc(v, 'ma1amsrc'); c.u32('ma1amamt')
    if v >= 200: c.modsrc(v, 'ma2in')
    if v >= 200: c.modsrc(v, 'ma2amsrc')
    # osc1
    P['o1tune']=c.u32('o1tune'); P['o1wave']=c.u32('o1wave'); P['o1shape']=c.u32('o1shape')
    if v >= 200: c.byte()
    c.modsrc(v,'o1fm1s'); c.u32('o1fm1d')
    c.modsrc(v,'o1fm2s'); c.u32('o1fm2d')
    c.modsrc(v,'o1pwms'); c.u32('o1pwmd')
    if v < 0x69: c.strsample()
    else: c.skipMsmp(); c.byte()
    # osc2
    P['o2tune']=c.u32('o2tune'); P['o2wave']=c.u32('o2wave'); c.byte('o1sync'); P['o2shape']=c.u32('o2shape')
    c.modsrc(v,'o2fm1s'); c.u32('o2fm1d')
    c.modsrc(v,'o2fm2s'); c.u32('o2fm2d')
    c.modsrc(v,'o2pwms'); c.u32('o2pwmd')
    if v < 0x6a: c.strsample()
    else: c.skipMsmp(); c.byte()
    # filter / mixer / output
    if v >= 200: c.u32('mix1src')
    P['o1level']=c.u32('o1level'); c.byte('mix1post')
    if v >= 200: c.u32('mix2src')
    P['o2level']=c.u32('o2level'); c.byte('mix2post')
    c.u32('mix3src'); P['noise']=c.u32('noise'); c.byte('mix3post')
    P['filterMode']=c.u32('filterMode'); c.byte(); P['cutoff']=c.u32('cutoff'); P['reso']=c.u32('reso'); P['bandwidth']=c.u32('bandwidth')
    if v >= 200: c.u32('hpCut')
    P['fcut1s']=c.modsrc(v,'fcut1s'); P['fcut1d']=c.u32('fcut1d')
    P['fcut2s']=c.modsrc(v,'fcut2s'); P['fcut2d']=c.u32('fcut2d')
    c.modsrc(v,'fcut3s'); c.u32('fcut3d')
    c.modsrc(v,'fresS'); c.u32('fresD')
    c.modsrc(v,'am1s'); c.u32('am1d')
    c.modsrc(v,'am2s'); c.u32('am2d')
    if v >= 200: c.modsrc(v,'am3s'); c.u32('am3d')
    P['overdrive']=c.u32('overdrive')
    if v >= 0x65: c.modsrc(v)
    if v >= 0x65: c.u32()
    P['voiceMode']=c.u32('voiceMode'); c.u32(); c.byte()
    c.u32('bendRange')
    if v >= 200: c.u32('uniVoices')
    P['uniDetune']=c.u32('uniDetune')
    if v >= 200: P['polyDetune']=c.u32('polyDetune')
    P['portamento']=c.u32('portamento')
    return ver, c.off, P

def landmarks(d):
    prst = find_tag(d, 'PRST')
    ms1  = find_tag(d, 'MSmp')
    ms2  = find_tag(d, 'MSmp', ms1+4) if ms1 >= 0 else -1
    return prst, ms1, ms2

# ── generation ──────────────────────────────────────────────────────────────────────────────
# mod-source indices (clone choice order): 0 None 1 LFO1 2 LFO2 3 LFO3 4 Env1 5 Env2(filter env)
SRC_NONE, SRC_LFO1, SRC_ENV2 = 0, 1, 5
NEUTRAL_TUNE = -2400          # parseV2P o1tune/o2tune base → setTune(tune+2400)=0 (neutral)

def cents(semitones=0, octaves=0, fine=0):
    return NEUTRAL_TUNE + octaves*1200 + semitones*100 + fine

def build_patch(template, params, name=None):
    d = bytearray(template)
    prst, ms1, ms2 = landmarks(d)
    _, off, _ = trace(d, prst)
    def setv(field, val):
        if field not in off: raise KeyError(field)
        o = off[field]
        struct.pack_into('<I', d, o, int(val) & 0xFFFFFFFF)   # 4-byte LE; small vals leave high bytes 0
    # baseline — neutralise template quirks so each patch is fully determined by `params`
    base = dict(noise=0, o1level=255, bandwidth=0, hpCut=0,
                o1fm1d=0, o1fm2d=0, o1pwmd=0, o2fm1d=0, o2fm2d=0, o2pwmd=0,
                am1d=0, am2d=0, am3d=0, e2modamt=0, ma1amamt=0,
                fcut1s=SRC_NONE, fcut1d=0, fcut2s=SRC_NONE, fcut2d=0, fcut3s=SRC_NONE, fcut3d=0,
                fresS=SRC_NONE, fresD=0, o1tune=NEUTRAL_TUNE, o2tune=NEUTRAL_TUNE)
    base.update(params)
    for k, v in base.items():
        setv(k, v)
    if name:
        # set the internal patch name within the first STR chunk (keep its size → no landmark shift)
        s = d.find(b'STR ')
        if s >= 0:
            ln = struct.unpack_from('<I', d, s+4)[0]
            nb = name.encode('latin-1', 'replace')[:ln]
            d[s+8:s+8+ln] = nb + b'\x00' * (ln - len(nb))
    return bytes(d)

def verify(data, expect):
    prst, _, _ = landmarks(data)
    _, _, P = trace(data, prst)
    def ne(a, b):                      # compare as unsigned 32-bit (o1tune/o2tune are signed)
        return a is None or (a & 0xFFFFFFFF) != (b & 0xFFFFFFFF)
    return {k: (P.get(k), v) for k, v in expect.items() if ne(P.get(k), v)}

def patches():
    """Return list of (category, name, params). 10 each: Lead/Bass/MidBass/Pad/Pluck."""
    L = []
    # ---- LEADS: bright supersaw, poly/unison, sustained, light filter-env open, edge ----
    for i in range(10):
        f = i / 9.0
        L.append(("Leads", f"TR Lead {i+1:02d}", dict(
            o1wave=2 if i % 3 else 0, o1shape=140 + int(90*f),
            o2wave=2 if i % 3 else 0, o2level=150 + i*8, o2tune=cents(fine=8 + i),
            filterMode=19 if i % 2 else 15, cutoff=185 + i*6, reso=30 + i*5,
            e1a=8 + i*3, e1d=120, e1s=235, e1r=60 + i*8,
            e2a=2, e2d=130 + i*10, e2s=120, e2r=80,
            fcut1s=SRC_ENV2, fcut1d=40 + i*6,
            overdrive=45 + i*8, voiceMode=2 if i % 2 else 1,
            uniDetune=40 + i*10, polyDetune=20 + i*4, portamento=0 if i < 6 else 18 + i*3)))
    # ---- BASSES: offbeat rolling, mono, plucky, strong filter-env sweep, octave down ----
    for i in range(10):
        L.append(("Basses", f"TR Bass {i+1:02d}", dict(
            o1wave=1 if i % 4 == 0 else 0, o1shape=40 + i*12,
            o2level=120 + i*6, o2tune=cents(fine=4),
            filterMode=19 if i % 2 else 1, cutoff=55 + i*7, reso=95 + i*8,
            e1a=0, e1d=55 + i*6, e1s=0 if i < 7 else 40, e1r=22 + i*3,
            e2a=0, e2d=70 + i*8, e2s=0, e2r=30,
            fcut1s=SRC_ENV2, fcut1d=120 + i*9,
            overdrive=70 + i*8, voiceMode=0,
            o1tune=cents(octaves=-1) if i % 2 else NEUTRAL_TUNE, portamento=0 if i < 5 else 12 + i*2)))
    # ---- MID-BASSES: more body/sustain than bass, often unison, mid filter ----
    for i in range(10):
        L.append(("MidBasses", f"TR MidBass {i+1:02d}", dict(
            o1wave=0 if i % 3 else 2, o1shape=70 + i*10,
            o2wave=0, o2level=140 + i*7, o2tune=cents(fine=7 + i),
            filterMode=19 if i % 2 else 15, cutoff=95 + i*7, reso=70 + i*7,
            e1a=2, e1d=90 + i*8, e1s=110 + i*8, e1r=50 + i*6,
            e2a=0, e2d=95 + i*8, e2s=40, e2r=45,
            fcut1s=SRC_ENV2, fcut1d=85 + i*7,
            overdrive=55 + i*7, voiceMode=2 if i % 2 else 0,
            uniDetune=30 + i*8, portamento=0)))
    # ---- PADS: slow, lush, wide, slow filter LFO, full sustain ----
    for i in range(10):
        L.append(("Pads", f"TR Pad {i+1:02d}", dict(
            o1wave=2, o1shape=170 + i*7,
            o2wave=2, o2level=170 + i*6, o2tune=cents(fine=10 + i*2),
            filterMode=10 if i % 2 else 19, cutoff=120 + i*8, reso=18 + i*3,
            e1a=110 + i*14, e1d=200, e1s=255, e1r=170 + i*14,
            e2a=90 + i*10, e2d=220, e2s=200, e2r=180,
            fcut2s=SRC_LFO1, fcut2d=30 + i*5, lfo1rate=20 + i*5,
            overdrive=0 if i < 6 else 20 + i*3, voiceMode=1 if i % 2 else 2,
            uniDetune=70 + i*10, polyDetune=40 + i*6, portamento=0)))
    # ---- PLUCKS: percussive, instant attack, zero sustain, sharp filter-env tick ----
    for i in range(10):
        L.append(("Plucks", f"TR Pluck {i+1:02d}", dict(
            o1wave=1 if i % 3 == 0 else 0, o1shape=60 + i*14,
            o2level=110 + i*6, o2tune=cents(fine=6),
            filterMode=15 if i % 2 else 19, cutoff=45 + i*6, reso=100 + i*8,
            e1a=0, e1d=45 + i*5, e1s=0, e1r=28 + i*4,
            e2a=0, e2d=55 + i*6, e2s=0, e2r=32,
            fcut1s=SRC_ENV2, fcut1d=130 + i*8,
            overdrive=30 + i*6, voiceMode=1,
            uniDetune=0, portamento=0)))
    return L

def generate(template_path, out_root):
    template = open(template_path, 'rb').read()
    defs = patches()
    n_ok = 0; n_fail = 0
    for cat, name, params in defs:
        data = build_patch(template, params, name)
        bad = verify(data, params)
        outdir = os.path.join(out_root, cat)
        os.makedirs(outdir, exist_ok=True)
        fn = os.path.join(outdir, name + '.v2p')
        open(fn, 'wb').write(data)
        if bad:
            n_fail += 1
            print(f"  FAIL {name}: {bad}")
        else:
            n_ok += 1
    print(f"\nGenerated {n_ok} OK, {n_fail} failed -> {out_root}")
    # category counts
    from collections import Counter
    print("Per category:", dict(Counter(c for c, _, _ in defs)))
    return n_ok, n_fail

if __name__ == '__main__':
    if len(sys.argv) > 1 and sys.argv[1] == 'gen':
        tmpl = sys.argv[2] if len(sys.argv) > 2 else r'C:\Users\ken98\Desktop\flutlicht-hardtrance\flutlicht_bass_01.v2p'
        out  = sys.argv[3] if len(sys.argv) > 3 else r'C:\Users\ken98\Desktop\VAZClone Trance Presets'
        generate(tmpl, out)
        sys.exit(0)
    tmpl = sys.argv[1] if len(sys.argv) > 1 else r'C:\Users\ken98\Desktop\flutlicht-hardtrance\flutlicht_bass_01.v2p'
    d = open(tmpl, 'rb').read()
    prst, ms1, ms2 = landmarks(d)
    PS = prst + 12; sec3 = ms2 + 8 + 548
    ver, off, P = trace(d, prst)
    print(f"template={os.path.basename(tmpl)} size={len(d)} ver={ver}")
    print(f"PS={PS} sec3={sec3}")
    # VALIDATION: these offsets must match buildV2P's known-good fixed offsets
    checks = [('lfo1rate', PS+10), ('e1a', PS+54), ('e1d', PS+58), ('e1s', PS+62), ('e1r', PS+66),
              ('e2a', PS+74), ('e2s', PS+82), ('o1wave', PS+131), ('o1shape', PS+135),
              ('filterMode', sec3+28), ('cutoff', sec3+33), ('reso', sec3+37),
              ('overdrive', sec3+105), ('noise', sec3+23), ('o2level', sec3+14),
              ('voiceMode', sec3+117), ('uniDetune', sec3+134), ('portamento', sec3+142)]
    ok = True
    for name, exp in checks:
        got = off.get(name)
        rel = f"sec3+{got-sec3}" if got and got >= sec3 else (f"PS+{got-PS}" if got else "MISSING")
        mark = "OK" if got == exp else "*** MISMATCH ***"
        if got != exp: ok = False
        print(f"  {name:11s} off={got} ({rel})  expected={exp}  {mark}")
    print(f"\nVALIDATION: {'PASS' if ok else 'FAIL'}")
    # offsets buildV2P does NOT write (needed for the trance filter sweep)
    print("\nExtra offsets (relative to sec3 / PS):")
    for name in ['fcut1s','fcut1d','fcut2s','fcut2d','fresS','fresD','o1tune','o2tune','o2level','o2wave','o2shape']:
        o = off.get(name)
        rel = f"sec3+{o-sec3}" if o and o >= sec3 else (f"PS+{o-PS}" if o else "?")
        print(f"  {name:9s} off={o} ({rel})  value={P.get(name,'?')}")
    print("\nParsed key values:", {k: P[k] for k in ['o1wave','o1shape','filterMode','cutoff','reso','overdrive','e1a','e1d','e1s','e1r','e2a','e2d','e2s','e2r','voiceMode','uniDetune'] if k in P})
