"""General Core.dll function disassembler (capstone). Usage: python disasm_fn.py <hexVA> [nbytes]
Flags fsin/fcos/fsqrt/call and prints x87 + memory ops — used to transcribe FX renders that are
outside the decompiled FX dump (e.g. the shared delay/flanger render FUN_004c3ad0)."""
import sys
try:
    import pefile, capstone
except ImportError as e:
    print("MISSING:", e); sys.exit(1)
CORE = r'C:\Program Files (x86)\Steinberg\Vstplugins\VAZ Synths\VAZ 2010\Vaz2010Core.dll'
va = int(sys.argv[1], 16); n = int(sys.argv[2], 0) if len(sys.argv) > 2 else 0x200
pe = pefile.PE(CORE); base = pe.OPTIONAL_HEADER.ImageBase; data = pe.__data__
md = capstone.Cs(capstone.CS_ARCH_X86, capstone.CS_MODE_32)
off = pe.get_offset_from_rva(va - base)
count = 0; ret_seen = False
for ins in md.disasm(data[off:off+n], va):
    m = ins.mnemonic; tag = ''
    if m in ('fsin','fcos','fsqrt','fsincos','fptan','fpatan'): tag = '   <<< TRANSCENDENTAL'
    if m == 'call': tag = '   <-- CALL ' + ins.op_str
    if m == 'ret': ret_seen = True
    print(f'  0x{ins.address:X}: {m} {ins.op_str}{tag}')
    count += 1
    if m == 'ret' and count > 3: break
print(f'\n{count} instrs, ret_seen={ret_seen}')
