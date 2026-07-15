"""dump_prop_table.py — decode VAZ's PROPERTY DESCRIPTOR TABLE.
The "Performance"/"Portamento Auto"/... strings are referenced from a contiguous data table at
~0x52b000 with a regular 0x14 (20-byte) stride, i.e. entries of the shape
    [u32 sectionName*][u32 propName*][u32 ?][u32 ?][u32 ?]
This is the authoritative property map (section, name, type/field/default) — far better evidence than
GUI control names. Dump it and resolve every pointer that lands on a string.
"""
import struct, sys
import pefile
CORE = r'C:\Program Files (x86)\Steinberg\Vstplugins\VAZ Synths\VAZ 2010\Vaz2010Core.dll'
pe = pefile.PE(CORE); base = pe.OPTIONAL_HEADER.ImageBase; d = pe.__data__
size = pe.OPTIONAL_HEADER.SizeOfImage

def off(va):
    if not va or not (base <= va < base + size): return None
    try: o = pe.get_offset_from_rva(va - base)
    except Exception: return None
    return o if (o is not None and 0 <= o < len(d)) else None
def u32(va):
    o = off(va)
    return struct.unpack_from('<I', d, o)[0] if (o is not None and o + 4 <= len(d)) else None
def cstr(va):
    o = off(va)
    if o is None: return None
    e = d.find(b'\x00', o, o + 64)
    if e < 0: return None
    s = d[o:e].decode('latin-1', 'replace')
    return s if s and all(32 <= ord(c) < 127 for c in s) and any(c.isalpha() for c in s) else None

start = int(sys.argv[1], 16) if len(sys.argv) > 1 else 0x52b000
count = int(sys.argv[2]) if len(sys.argv) > 2 else 40
stride = 0x14
print(f'=== property descriptor table @0x{start:X}, stride 0x{stride:x} ===')
print(f'{"entryVA":>10}  {"section":<16} {"property":<22} {"f2":>10} {"f3":>10} {"f4":>10}')
for i in range(count):
    va = start + i * stride
    w = [u32(va + k * 4) for k in range(5)]
    if any(x is None for x in w): break
    sec, prop = cstr(w[0]), cstr(w[1])
    if sec is None and prop is None: continue
    f2 = f'0x{w[2]:x}' if w[2] is not None else '?'
    f3 = f'0x{w[3]:x}' if w[3] is not None else '?'
    f4 = f'0x{w[4]:x}' if w[4] is not None else '?'
    # if f3/f4 point at strings, show them
    s3, s4 = cstr(w[3]), cstr(w[4])
    print(f'0x{va:08X}  {str(sec):<16} {str(prop):<22} {f2:>10} {f3:>10}{"="+s3 if s3 else "":<12} {f4:>10}{"="+s4 if s4 else ""}')
