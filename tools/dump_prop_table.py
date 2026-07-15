"""dump_prop_table.py — decode VAZ's PROPERTY DESCRIPTOR TABLE in full.

Entry layout (20 bytes, confirmed from the raw dump):
    [u32 sectionName*][u32 propName*][u32 default][u32 A][u32 B]
This is VAZ's AUTHORITATIVE per-patch property surface — far stronger evidence than GUI control names
(which produced 3/3 false alarms). Auto-detects the table bounds by walking while both pointers still
resolve to strings, then groups by section.

    py dump_prop_table.py            # full table, grouped by section
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
    return s if s and 1 < len(s) < 40 and all(32 <= ord(c) < 127 for c in s) and any(c.isalpha() for c in s) else None

STRIDE = 0x14
def valid(va):
    return cstr(u32(va)) is not None and cstr(u32(va + 4)) is not None

# find the table bounds by walking out from a known-good anchor
anchor = 0x52B000                       # [Performance][Play Mode]
while valid(anchor - STRIDE): anchor -= STRIDE
lo = anchor
hi = anchor
while valid(hi): hi += STRIDE
print(f'=== property descriptor table: 0x{lo:X} .. 0x{hi:X}  ({(hi-lo)//STRIDE} entries, stride 0x{STRIDE:x}) ===')

from collections import OrderedDict
sections = OrderedDict()
for va in range(lo, hi, STRIDE):
    sec, prop = cstr(u32(va)), cstr(u32(va + 4))
    dflt, a, b = u32(va + 8), u32(va + 12), u32(va + 16)
    sections.setdefault(sec, []).append((prop, dflt, a, b, va))

for sec, items in sections.items():
    print(f'\n--- [{sec}]  ({len(items)} properties) ---')
    for prop, dflt, a, b, va in items:
        ds = 'TRUE(-1)' if dflt == 0xffffffff else str(dflt)
        print(f'   {prop:22s} default={ds:<10} A=0x{a:<6x} B=0x{b:x}')
print(f'\nTOTAL: {sum(len(v) for v in sections.values())} properties across {len(sections)} sections')
