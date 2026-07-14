"""Pinpoint the mod-LFO / osc3 shared waveform GENERATOR. Its object fields are known:
+0x84 Waveform, +0x8c WaveShape, +0xd0 mode(6=S&H). The generator reads all three -> find the code
region where the disps 0x84, 0x8c, 0xd0 cluster within one function, then that's the shape generator."""
import struct, re, bisect
import pefile, capstone
CORE = r'C:\Program Files (x86)\Steinberg\Vstplugins\VAZ Synths\VAZ 2010\Vaz2010Core.dll'
pe = pefile.PE(CORE); base = pe.OPTIONAL_HEADER.ImageBase
sec = next(s for s in pe.sections if s.Characteristics & 0x20000000)
tb = base + sec.VirtualAddress; td = sec.get_data()
def refs(disp):
    return sorted(tb + m.start() for m in re.finditer(re.escape(struct.pack('<I', disp)), td))
r84, r8c, rd0 = refs(0x84), refs(0x8c), refs(0xd0)
print(f'refs: +0x84={len(r84)}  +0x8c={len(r8c)}  +0xd0={len(rd0)}')
# a generator references +0x84 AND +0xd0 AND +0x8c all within ~0x200 bytes
hits = []
for a in r84:
    lo = bisect.bisect_left(rd0, a-0x200); hi = bisect.bisect_right(rd0, a+0x200)
    if hi <= lo: continue
    if any(abs(x-a) < 0x200 for x in r8c):
        hits.append((a, rd0[lo]))
print(f'\n{len(hits)} region(s) referencing +0x84 & +0x8c & +0xd0 together (the shared LFO/osc3 generator):')
for a, d0 in hits[:12]:
    print(f'  +0x84 @0x{a:X}   (+0xd0 @0x{d0:X})')

md = capstone.Cs(capstone.CS_ARCH_X86, capstone.CS_MODE_32)
def rd_f32(va):
    try: o=pe.get_offset_from_rva(va-base); return struct.unpack('<f', pe.__data__[o:o+4])[0]
    except Exception: return None
def disasm(sva, n):
    print(f'\n===== disasm 0x{sva:X}..0x{sva+n:X} (generator) =====')
    o=pe.get_offset_from_rva(sva-base)
    for ins in md.disasm(pe.__data__[o:o+n], sva):
        m=ins.mnemonic; op=ins.op_str; tag=''
        if m[0]=='f': tag=' <fpu>'
        if m in ('fsin','fcos','fsqrt','fptan','fpatan','frndint','f2xm1','fyl2x'): tag=' <<<TRANSC'
        if m=='call': tag=' <-CALL'
        mm=re.search(r'\[0x([0-9a-f]+)\]', op)
        if m[0]=='f' and mm:
            cv=rd_f32(int(mm.group(1),16))
            if cv is not None and abs(cv)<1e12: tag+=f' (const={cv:g})'
        for d in ('0x84','0x8c','0xd0','0x88','0x90','0xd4'):
            if f'+ {d}]' in op: tag+=f' <<{d}'
        if m[0]=='f' or m in ('call','ret','cmp','test','jmp') or m.startswith('j') or '<<' in tag:
            print(f'  0x{ins.address:X}: {m} {op}{tag}')

# +0x84 READ sites (opcode 0x8B = mov reg,[base+0x84]) — the generator READS the waveform selector
print('\n=== +0x84 READ sites (mov reg,[base+0x84]) — the generator reads the waveform selector ===')
d = pe.__data__
reads = []
for va in r84:
    o = pe.get_offset_from_rva(va - base)
    if d[o-3] == 0x8B: reads.append(va-2)          # instruction start = disp_off - 2 (modrm) - ... use va-2 as modrm
for va in r84:
    o = pe.get_offset_from_rva(va - base)
    if d[o-3] == 0x8B:
        print(f'  read @0x{va-3:X}')

