"""Find the sequencer clock/timing handlers in Vaz2010Core.dll.
Borland published-method table entry: {u16 size, u32 addr, u8 nameLen, char name[nameLen]}, so for a
method NAME at file offset O the entry's method ADDRESS = u32 at O-5 (and nameLen at O-1 must == len).
Decode the addresses for the timing controls' OnChange/OnClick handlers, then disassemble each to find
where msTimebase/sbSwing/btFreeRunning values become the step interval (timebase table, swing, free-run)."""
import sys, struct
try:
    import pefile, capstone
except ImportError as e:
    print("MISSING:", e); sys.exit(1)
CORE = r'C:\Program Files (x86)\Steinberg\Vstplugins\VAZ Synths\VAZ 2010\Vaz2010Core.dll'
pe = pefile.PE(CORE); base = pe.OPTIONAL_HEADER.ImageBase; data = pe.__data__
lo, hi = base, base + pe.OPTIONAL_HEADER.SizeOfImage
md = capstone.Cs(capstone.CS_ARCH_X86, capstone.CS_MODE_32)

def method_addr(name):
    nb = name.encode(); i = 0
    while True:
        o = data.find(nb, i)
        if o < 0: return None
        if o >= 5 and data[o-1] == len(name):                 # nameLen byte precedes the chars
            a = struct.unpack('<I', data[o-5:o-1])[0]
            if lo <= a < hi: return a                          # method addr into the image
        i = o + 1

def disasm(va, n, label):
    print(f'\n===== {label}  @0x{va:X} =====')
    try: off = pe.get_offset_from_rva(va - base)
    except Exception as e: print('  bad VA', e); return
    cnt = 0
    for ins in md.disasm(data[off:off+n], va):
        m = ins.mnemonic; tag = ''
        if m in ('fmul','fdiv','fdivr','fadd','fsub','fld','fst','fstp','fild','fmulp','fdivp','fsubp','fdivrp'): tag = '   <fpu>'
        if m in ('fsin','fcos','fsqrt','fptan','fpatan','fyl2x','f2xm1'): tag = '   <<< TRANSCENDENTAL'
        if m == 'call': tag = '   <-- CALL ' + ins.op_str
        if tag or m.startswith('j') or m in ('ret','mov','movzx','movsx'):
            print(f'  0x{ins.address:X}: {m} {ins.op_str}{tag}')
        cnt += 1
        if m == 'ret' and cnt > 6: break

for n in ['msTimebaseChange','sbSwingChange','sbGateTimeChange','msTempoChange','btFreeRunningClick','btFreeRunningX']:
    a = method_addr(n)
    print(f'{n:22s} -> {hex(a) if a else "NOT FOUND"}')

# scan the image for code references to the sequencer timing-field cluster (timebase @+0x23f0, +/- nearby)
print('\n=== code refs to the seq timing-field cluster (disp 0x23e0..0x2410) — the clock reads these ===')
import re
sec_text = next(s for s in pe.sections if s.Characteristics & 0x20000000)   # MEM_EXECUTE (Borland: "CODE")
print(f'  (code section {sec_text.Name.rstrip(chr(0).encode()).decode(errors="replace")} @VA 0x{base+sec_text.VirtualAddress:X})')
tbase = base + sec_text.VirtualAddress; tdata = sec_text.get_data()
hits = {}
for disp in list(range(0x23e0, 0x2414, 4)) + [0x2668, 0x2674, 0x2660, 0x2658]:
    pat = struct.pack('<I', disp)
    for m in re.finditer(re.escape(pat), tdata):
        va = tbase + m.start()
        hits.setdefault(disp, []).append(va)
for disp in sorted(hits):
    vas = hits[disp]
    print(f'  +0x{disp:X}: {len(vas)} ref(s)  e.g. ' + ' '.join(f'0x{v:X}' for v in vas[:4]))

import re as _re
def rd_f32 (va):
    try: o = pe.get_offset_from_rva (va - base); return struct.unpack ('<f', data[o:o+4])[0]
    except Exception: return None
def disasm_from (sva, n, label, full=False):
    print(f'\n===== {label} @0x{sva:X}..0x{sva+n:X} =====')
    o = pe.get_offset_from_rva (sva - base)
    for ins in md.disasm (data[o:o+n], sva):
        m = ins.mnemonic; op = ins.op_str; tag = ''
        if m[0] == 'f': tag = ' <fpu>'
        if m in ('fsin','fcos','fsqrt','fptan','fpatan'): tag = ' <<<TRIG'
        if m == 'call': tag = ' <-CALL'
        mm = _re.search (r'\[0x([0-9a-f]+)\]', op)
        if m[0] == 'f' and mm:
            cv = rd_f32 (int (mm.group(1), 16))
            if cv is not None and abs (cv) < 1e12: tag += f'  (const={cv:g})'
        for d in ('0x23f0','0x23f4','0x23f8','0x23fc','0x2400','0x2668','0x2674','0x1c0'):
            if d in op: tag += f' <<{d}'
        if full or tag: print(f'  0x{ins.address:X}: {m} {op}{tag}')

disasm_from (0x4CFD60, 0x24, '+0x2674 site A (0x4CFD6E)', full=True)
disasm_from (0x4D0088, 0x2C, '+0x2674 site B (0x4D0096)', full=True)
disasm_from (0x4CFB70, 0x18, '+0x2674 site C (0x4CFB78)', full=True)
