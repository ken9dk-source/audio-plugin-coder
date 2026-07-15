"""harvest_vaz_names.py — SECOND-PASS gap hunt: enumerate EVERY published Borland name in
Vaz2010Core.dll with full context, then diff against what the status maps actually mention.

Borland/Delphi RTTI (packed):
  TMethodEntry = [u16 size][u32 addr][u8 nameLen][chars]    -> size == 7+nameLen ; addr in CODE
  TFieldEntry  = [u32 offset][u16 classIdx][u8 nameLen][chars] -> offset = the field's OBJECT offset

So for a name at file offset O:
  method: u16@O-7 == 7+len  AND  u32@O-5 is a code VA      -> (name, handler address)
  field : u32@O-7 is a plausible object offset (<0x10000)  -> (name, object offset)

Output: the complete name universe -> feeds the gap diff. NO code changes; analysis only.
"""
import struct, re, sys, json
import pefile

CORE = r'C:\Program Files (x86)\Steinberg\Vstplugins\VAZ Synths\VAZ 2010\Vaz2010Core.dll'
pe = pefile.PE(CORE)
base = pe.OPTIONAL_HEADER.ImageBase
data = pe.__data__
sec_code = next(s for s in pe.sections if s.Characteristics & 0x20000000)
code_lo = base + sec_code.VirtualAddress
code_hi = code_lo + sec_code.Misc_VirtualSize

def va_of(off):
    try: return base + pe.get_rva_from_offset(off)
    except Exception: return None

# every plausible ShortString: preceding byte == length, chars are printable ident-ish
NAME_RE = re.compile(rb'[A-Za-z_][A-Za-z0-9_ .\-]{2,40}')
methods, fields = {}, {}
for m in NAME_RE.finditer(data):
    o, s = m.start(), m.group()
    n = len(s)
    # try every prefix length (the regex may over-match past the string end)
    for ln in range(n, 2, -1):
        if o < 8: break
        if data[o-1] != ln: continue
        name = s[:ln].decode('latin-1')
        # METHOD?
        size = struct.unpack_from('<H', data, o-7)[0]
        addr = struct.unpack_from('<I', data, o-5)[0]
        if size == ln + 7 and code_lo <= addr < code_hi:
            methods.setdefault(name, addr)
            break
        # FIELD?
        foff = struct.unpack_from('<I', data, o-7)[0]
        cidx = struct.unpack_from('<H', data, o-3)[0]
        if 0 < foff < 0x10000 and cidx < 0x400:
            fields.setdefault(name, foff)
            break

print(f'PUBLISHED METHODS (name -> handler VA): {len(methods)}')
print(f'PUBLISHED FIELDS  (name -> object offset): {len(fields)}')

def prefix(n):
    for p in ('ms','sb','bt','ed','cb','ud','lb','pb','pnl','img','tb','mi','pu','gb','rb','ck'):
        if n.startswith(p) and len(n) > len(p) and n[len(p)].isupper(): return p
    return ''

# group controls by Delphi naming prefix = control type
from collections import defaultdict
bykind = defaultdict(list)
for n, off in sorted(fields.items(), key=lambda kv: kv[1]):
    bykind[prefix(n)].append((n, off))
print('\n--- FIELDS by control-type prefix (ms=menu/select, sb=slider, bt=button, ed=edit, cb=combo, ud=updown) ---')
for k in sorted(bykind):
    if not k: continue
    items = bykind[k]
    print(f'  {k:4s} x{len(items):3d}: ' + ', '.join(f'{n}@0x{o:x}' for n, o in items[:8]) + (' …' if len(items) > 8 else ''))
print(f'  (unprefixed/other: {len(bykind[""])})')

json.dump({'methods': {k: hex(v) for k, v in methods.items()},
           'fields':  {k: hex(v) for k, v in fields.items()}},
          open(r'C:\APC\y\tools\vaz_name_universe.json', 'w'), indent=1, sort_keys=True)
print('\nwrote tools/vaz_name_universe.json')
