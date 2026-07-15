"""xref_synth_vmt.py — identify the 7 un-decompiled TBaseMidSynth virtuals by EVIDENCE, not prologue.

Two independent angles:
  (A) DIRECT call sites: scan the code for E8 rel32 whose target == the method -> who calls it, and the
      surrounding context. (Virtual-dispatch calls go through [vmt+slot*4] and carry no address, so a
      method may legitimately have zero direct xrefs.)
  (B) FIELD TOUCH: disassemble the body and report which KNOWN fields of the synth object it reads or
      writes. The field map is what this session established from named properties + setters, so a
      method touching e.g. porta/cutoff/glide fields identifies itself.

Report only. Anything ambiguous stays flagged, not guessed.
"""
import struct, re, sys
import pefile, capstone

CORE = r'C:\Program Files (x86)\Steinberg\Vstplugins\VAZ Synths\VAZ 2010\Vaz2010Core.dll'
pe = pefile.PE(CORE); base = pe.OPTIONAL_HEADER.ImageBase; data = pe.__data__
md = capstone.Cs(capstone.CS_ARCH_X86, capstone.CS_MODE_32)
sec = next(s for s in pe.sections if s.Characteristics & 0x20000000)
tb = base + sec.VirtualAddress; td = sec.get_data()

TARGETS = [0x4d7c6c, 0x4d83e0, 0x4daaf0, 0x4db1b8, 0x4de548, 0x4e0f80, 0x4db844]
KNOWN = {   # field map established this session (named properties + setters + render analysis)
    0x14: 'osc pitch out', 0x28: 'osc pitch out2', 0xb0: 'ramp acc', 0xb8: 'GLIDE target',
    0xbc: 'GLIDE current', 0x1b8: 'GLIDE rate', 0x264: 'Cutoff', 0x26c: 'Resonance',
    0x270: 'Bandwidth/Modifier', 0x278: 'FM1 src', 0x27c: 'FM1 depth', 0x290: 'RM src',
    0x294: 'RM depth', 0x304: 'Porta EXP', 0x308: 'Porta AUTO', 0x30c: 'Porta TIME',
    0x254: 'filter mode', 0x25b4: 'sub-obj ptr', 0x2dc: 'osc mode?', 0x2f4: 'osc field?',
    0x84: 'LFO waveform', 0x8c: 'LFO waveshape', 0xd0: 'LFO mode',
}

def direct_callers(target):
    out = []
    for m in re.finditer(rb'\xe8', td):
        o = m.start()
        if o + 5 > len(td): continue
        rel = struct.unpack_from('<i', td, o + 1)[0]
        site = tb + o
        if site + 5 + rel == target: out.append(site)
    return out

def body(va, n=0x160):
    try: off = pe.get_offset_from_rva(va - base)
    except Exception: return []
    out = []
    for ins in md.disasm(data[off:off + n], va):
        out.append(ins)
        if ins.mnemonic == 'ret': break
    return out

def fields_touched(va):
    hits = {}
    for ins in body(va):
        for mm in re.finditer(r'\+ 0x([0-9a-f]+)\]', ins.op_str):
            d = int(mm.group(1), 16)
            if d in KNOWN:
                w = ins.op_str.strip().startswith('dword ptr') or ins.op_str.strip().startswith('byte ptr')
                hits.setdefault(d, set()).add('W' if w else 'R')
    return hits

def ctx(va, back=0x24):
    print(f'      --- caller context @0x{va:X} ---')
    for ins in body(va - back, 0x40):
        mark = '  <<<' if ins.address == va else ''
        print(f'        0x{ins.address:X}: {ins.mnemonic} {ins.op_str}{mark}')

for t in TARGETS:
    print(f'\n================ 0x{t:X} ================')
    dc = direct_callers(t)
    print(f'  direct call sites: {len(dc)} ' + (' '.join(f'0x{x:X}' for x in dc[:6]) if dc else '(none — virtual-dispatch only)'))
    ft = fields_touched(t)
    if ft:
        print('  known fields touched:')
        for d, rw in sorted(ft.items()):
            print(f'     +0x{d:<5x} {"/".join(sorted(rw)):3s}  {KNOWN[d]}')
    else:
        print('  known fields touched: (none in the first ~0x160 bytes)')
    calls = [i for i in body(t) if i.mnemonic == 'call']
    if calls:
        print('  calls out to: ' + ' '.join(i.op_str for i in calls[:8]))
