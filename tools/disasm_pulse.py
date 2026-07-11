"""disasm_pulse.py — go back to the actual machine code (not the decompile TEXT) for VAZ's pulse engine.
Locates references to the BLEP step tables (0x6dd2c0/0x6de2c0), disassembles the containing code, and flags
x87 float ops / call targets / the table-indexing instructions. Reports addresses. Read-only static analysis."""
import pefile
try:
    import capstone
    HAVE_CS = True
except Exception:
    HAVE_CS = False

PATH = r'tools\Vaz2010Core.dll'
pe = pefile.PE(PATH)
ib = pe.OPTIONAL_HEADER.ImageBase
data = open(PATH, 'rb').read()
print(f'ImageBase 0x{ib:X}   capstone={HAVE_CS}')

IMAGE_SCN_MEM_EXECUTE = 0x20000000
text = None
for s in pe.sections:
    nm = s.Name.rstrip(b'\x00').decode('latin1', 'replace')
    exe = bool(s.Characteristics & IMAGE_SCN_MEM_EXECUTE)
    print(f'  section {nm:10s} RVA 0x{s.VirtualAddress:06X} fileoff 0x{s.PointerToRawData:06X} size 0x{s.SizeOfRawData:06X} exec={exe}')
    if exe and text is None:
        text = s
print(f'=> code section: {text.Name.rstrip(chr(0).encode()).decode("latin1")}')

def off_to_va(off):
    return ib + text.VirtualAddress + (off - text.PointerToRawData)
def va_to_off(va):
    return text.PointerToRawData + (va - ib - text.VirtualAddress)

# 1) find references to the two step tables anywhere in the file
for name, va in [('dd2c0', 0x6dd2c0), ('de2c0', 0x6de2c0)]:
    pat = va.to_bytes(4, 'little')
    offs = []
    i = data.find(pat)
    while i != -1 and len(offs) < 16:
        offs.append(i); i = data.find(pat, i + 1)
    intext = [o for o in offs if text.PointerToRawData <= o < text.PointerToRawData + text.SizeOfRawData]
    print(f'{name} (0x{va:X}) refs in .text at VA:', [hex(off_to_va(o)) for o in intext])

# 2) disassemble the regions that use the tables; flag x87 (f-prefix) + calls + table loads
md = capstone.Cs(capstone.CS_ARCH_X86, capstone.CS_MODE_32)
md.detail = False
def disasm(va_start, va_end, tag):
    print(f'\n===== REGION {tag}: 0x{va_start:X}..0x{va_end:X} =====')
    off = va_to_off(va_start)
    code = data[off: off + (va_end - va_start)]
    x87 = 0
    for ins in md.disasm(code, va_start):
        m = ins.mnemonic
        flag = ''
        if m and m[0] == 'f' and m not in ('fs','famous'):  # x87 float
            flag = '   <<< X87 FLOAT'; x87 += 1
        elif m == 'call':
            flag = '   <<< CALL'
        elif '6dd2c0' in ins.op_str or '6de2c0' in ins.op_str:
            flag = '   <<< STEP TABLE'
        print(f'  0x{ins.address:X}: {m:7s} {ins.op_str}{flag}')
    print(f'  [x87 float instructions in region {tag}: {x87}]')

disasm(0x4dc1c0, 0x4dc25c, 'A-SETUP (iv8 + pulse-width + edge clamp, decompile 207-222)')

# find the increment table DAT_005445e0 (pitch->phase-increment) refs near the osc
for va in [0x5445e0]:
    pat = va.to_bytes(4, 'little')
    offs = []; i = data.find(pat)
    while i != -1 and len(offs) < 20:
        offs.append(i); i = data.find(pat, i + 1)
    intext = [off_to_va(o) for o in offs if text.PointerToRawData <= o < text.PointerToRawData + text.SizeOfRawData]
    print(f'\nDAT_005445e0 (pitch->inc table) refs in .text at VA:', [hex(v) for v in intext])

disasm(0x4dc140, 0x4dc1c6, 'INCREMENT (pitch -> phase inc, decompile 145-161)')

# find callers of the voice/osc render function FUN_004dbddc (call rel32 = E8, target = addr+5+rel)
RENDER = 0x4dbddc
callers = []
b = data
i = text.PointerToRawData
end = text.PointerToRawData + text.SizeOfRawData
while i < end - 5:
    if b[i] == 0xE8:
        rel = int.from_bytes(b[i+1:i+5], 'little', signed=True)
        callva = off_to_va(i)
        if callva + 5 + rel == RENDER:
            callers.append(callva)
        i += 5
    else:
        i += 1
print(f'\ncallers of render FUN_0x{RENDER:X}: {[hex(c) for c in callers]}')
for c in callers[:4]:
    disasm(c - 0x40, c + 0x20, f'CALLER @0x{c:X} (is there a 2x oversample loop?)')

# check the increment table scale: dump raw int32 at a few indices around a mid note
import struct
tva = 0x5445e0
def read_table(idx):
    off = va_to_off(tva + idx * 4)
    return struct.unpack('<i', data[off:off+4])[0]
print('\nDAT_005445e0 raw int32 samples (pitch idx -> inc):')
for idx in [0x80, 0x1000, 0x1D00, 0x2000, 0x2A00, 0x3000, 0x3b80]:
    v = read_table(idx)
    # if inc = f/sr * 2^32, then f = v/2^32 * sr; show implied Hz at 44100 and 48000
    print(f'  pitch 0x{idx:04X}: inc={v & 0xffffffff:>10} (0x{v & 0xffffffff:08X})  -> f@44100={ (v & 0xffffffff)/2**32*44100:8.2f}Hz  f@48000={ (v & 0xffffffff)/2**32*48000:8.2f}Hz')
