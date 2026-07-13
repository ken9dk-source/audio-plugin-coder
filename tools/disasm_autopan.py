"""Settle VAZ autopan pan-law: LINEAR vs equal-power. Two robust checks:
(1) BYTE-SCAN the whole autopan code block for x87 transcendental opcodes (alignment-independent):
    fsin=D9 FE, fcos=D9 FF, fsqrt=D9 FA, fsincos=D9 FB, fptan=D9 F2, fpatan=D9 F3.
(2) Disassemble from the function start to show the actual pan calc + any CALLs (runtime cos/sin)."""
import sys
try:
    import pefile, capstone
except ImportError as e:
    print("MISSING:", e); sys.exit(1)
CORE = r'C:\Program Files (x86)\Steinberg\Vstplugins\VAZ Synths\VAZ 2010\Vaz2010Core.dll'
pe = pefile.PE(CORE); base = pe.OPTIONAL_HEADER.ImageBase
data = pe.__data__
md = capstone.Cs(capstone.CS_ARCH_X86, capstone.CS_MODE_32)

# (1) byte-scan the autopan function block 0x5170c4..0x517f64 for x87 transcendental opcodes
lo, hi = 0x5170c4, 0x517f70
off_lo = pe.get_offset_from_rva(lo - base); off_hi = pe.get_offset_from_rva(hi - base)
blk = data[off_lo:off_hi]
OPC = {b'\xd9\xfe':'fsin', b'\xd9\xff':'fcos', b'\xd9\xfa':'fsqrt', b'\xd9\xfb':'fsincos', b'\xd9\xf2':'fptan', b'\xd9\xf3':'fpatan'}
hits = []
for i in range(len(blk)-1):
    op = blk[i:i+2]
    if op in OPC: hits.append((lo+i, OPC[op]))
print(f'(1) BYTE-SCAN autopan block 0x{lo:X}..0x{hi:X}: transcendental opcodes = {hits if hits else "NONE"}')

# (2) disasm from the function start (prologue scan back from 0x517b22 for 55 8b ec = push ebp;mov ebp,esp)
start = None
for a in range(0x517b22, 0x517a00, -1):
    o = pe.get_offset_from_rva(a - base)
    if data[o:o+3] == b'\x55\x8b\xec': start = a; break
start = start or 0x517ae4
print(f'\n(2) disasm from function start 0x{start:X} (contains the pan calc @0x517b22):')
off = pe.get_offset_from_rva(start - base)
for ins in md.disasm(data[off:off+0x120], start):
    tag = ''
    if ins.mnemonic in ('fsin','fcos','fsqrt','fsincos','fptan','fpatan'): tag = '   <<< TRANSCENDENTAL'
    if ins.mnemonic == 'call': tag = '   <-- CALL'
    if ins.address >= 0x517b90: break
    print(f'  0x{ins.address:X}: {ins.mnemonic} {ins.op_str}{tag}')

# (3) decode the 80-bit constants K1 @0x517c3c, K2 @0x517c48 (fld xword ptr)
def ld80(va):
    o = pe.get_offset_from_rva(va - base); b = data[o:o+10]
    mant = int.from_bytes(b[0:8], 'little'); se = int.from_bytes(b[8:10], 'little')
    sign = -1.0 if (se >> 15) else 1.0; exp = se & 0x7fff
    if exp == 0: return sign * mant * 2.0**(-16382-63)
    return sign * mant * 2.0**(exp-16383-63)
K1 = ld80(0x517c3c); K2 = ld80(0x517c48)
print(f'\n(3) constants: K1@0x517c3c = {K1!r}   K2@0x517c48 = {K2!r}   sqrt(K1)*K2 = {(K1**0.5)*K2:.6g}  (=2^31? {2**31})')

# (4) find + disasm the RENDER: where table[obj+idx*4+0x280] is READ and applied to L/R (the shift)
print('\n(4) render reads of table (+0x280):')
import re
tgt = b'\x80\x02\x00\x00'  # disp32 = 0x280
hits = [m.start() for m in re.finditer(re.escape(tgt), data[pe.get_offset_from_rva(0x510000-base):pe.get_offset_from_rva(0x520000-base)])]
print(f'   {len(hits)} occurrences of disp 0x280 in fx code (render uses the built table)')
