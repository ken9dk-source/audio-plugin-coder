"""harvest_vaz_classes.py — CLASS-AWARE harvest. Supersedes harvest_vaz_names.py, which collected
names globally with no owning class (offsets from different forms are NOT comparable — that made the
first pass's "offset family" clustering unsafe).

Delphi VMTs are self-identifying: u32[V-76] == V (vmtSelfPtr). From a VMT we get, at fixed negative
offsets:
    -76 SelfPtr   -56 FieldTable   -52 MethodTable   -48 DynamicTable
    -44 ClassName (ShortString*)   -40 InstanceSize  -36 Parent(**)
and the VIRTUAL SLOTS live at V+0, V+4, ... → this also answers the VMT-slot sweep (DSP classes, not
just GUI handlers).

    TFieldTable  = [u16 count][u32 classTab][ entries: [u32 off][u16 clsIdx][ShortString name] ]
    TMethodTable = [u16 count][ entries: [u16 size][u32 addr][ShortString name] ]

Output: tools/vaz_classes.json  — every class with its own fields/methods/vmt slots.
"""
import struct, json, sys
import pefile

CORE = r'C:\Program Files (x86)\Steinberg\Vstplugins\VAZ Synths\VAZ 2010\Vaz2010Core.dll'
pe = pefile.PE(CORE); base = pe.OPTIONAL_HEADER.ImageBase; data = pe.__data__
size = pe.OPTIONAL_HEADER.SizeOfImage
sec_code = next(s for s in pe.sections if s.Characteristics & 0x20000000)
code_lo = base + sec_code.VirtualAddress
code_hi = code_lo + sec_code.Misc_VirtualSize

def off(va):
    if not va or not (base <= va < base + size): return None      # must be inside the image
    try: o = pe.get_offset_from_rva(va - base)
    except Exception: return None
    return o if (o is not None and 0 <= o < len(data)) else None
def u32(va):
    o = off(va)
    if o is None or o + 4 > len(data): return None
    return struct.unpack_from('<I', data, o)[0]
def u16(va):
    o = off(va)
    if o is None or o + 2 > len(data): return None
    return struct.unpack_from('<H', data, o)[0]
def sstr(va):
    if not va or not (base <= va < base + size): return None
    o = off(va)
    if o is None or o >= len(data): return None
    try:
        ln = data[o]
    except Exception:
        return None
    if ln == 0 or o + 1 + ln > len(data): return None
    try: return data[o+1:o+1+ln].decode('latin-1')
    except Exception: return None

def parent_name(pp):
    """vmtParent is a POINTER TO a VMT pointer; deref twice, then ClassName at -44."""
    if not pp: return None
    v = u32(pp)
    if not v or not (base <= v < base + size): return None
    cn = u32(v - 44)
    return sstr(cn) if cn else None

# ---- find VMTs -------------------------------------------------------------
classes = {}
for s in pe.sections:
    lo = base + s.VirtualAddress; hi = lo + s.Misc_VirtualSize
    for va in range(lo + 76, hi - 4, 4):
        if u32(va - 76) != va: continue                      # vmtSelfPtr signature
        nm = sstr(u32(va - 44))
        if not nm or not nm[0].isalpha(): continue
        inst = u32(va - 40); parent_pp = u32(va - 36)
        parent = parent_name(parent_pp)
        ft, mt = u32(va - 56), u32(va - 52)
        # fields (own, this class only)
        fields = []
        if ft:
            cnt = u16(ft) or 0
            p = ft + 2 + 4
            for _ in range(min(cnt, 400)):
                fo = u32(p); ci = u16(p + 4); n = sstr(p + 6)
                if n is None: break
                fields.append((n, fo))
                p += 6 + 1 + len(n)
        # methods (own published)
        meths = []
        if mt:
            cnt = u16(mt) or 0
            p = mt + 2
            for _ in range(min(cnt, 400)):
                sz = u16(p); ad = u32(p + 2); n = sstr(p + 6)
                if n is None or not sz: break
                meths.append((n, ad))
                p += sz
        # virtual slots
        slots = []
        p = va
        while True:
            f = u32(p)
            if f is None or not (code_lo <= f < code_hi): break
            slots.append(f); p += 4
            if len(slots) > 300: break
        classes[nm] = dict(vmt=va, instSize=inst, parent=parent,
                           fields=fields, methods=meths, slots=slots)

print(f'classes found: {len(classes)}')
json.dump({k: dict(vmt=hex(v['vmt']), instSize=v['instSize'], parent=v['parent'],
                   fields=[[n, hex(o)] for n, o in v['fields']],
                   methods=[[n, hex(a)] for n, a in v['methods']],
                   slots=[hex(s) for s in v['slots']])
           for k, v in classes.items()},
          open(r'C:\APC\y\tools\vaz_classes.json', 'w'), indent=1, sort_keys=True)

# ---- report: which classes actually own the P2/P3 controls? ---------------
WANT = ['msAmpPMSource', 'msAmpAMSource', 'msLFOPeriod', 'msLFO2Period', 'msLFOMode', 'msLFO2Mode',
        'msLFO2RMSource', 'msFilterModModSource', 'btPortaAuto', 'btPortaExp', 'btOsc1Link',
        'btOsc2Sync', 'cbOversample', 'msLagSource', 'btModAmpSQ', 'msMixSource1', 'msMixSource3',
        'sbImpUnisonDetune', 'cbImpAutoGlide', 'edImpBendRange']
print('\n--- owning class for each P2/P3 control (offsets are only comparable WITHIN a class) ---')
for w in WANT:
    hits = [(cn, o) for cn, c in classes.items() for n, o in c['fields'] if n == w]
    if not hits: print(f'  {w:22s} : (not found in any class field table)')
    for cn, o in hits:
        print(f'  {w:22s} : {cn}  @+0x{o:x}  (instSize=0x{classes[cn]["instSize"]:x})')

print('\n--- biggest classes by own-field count (the real forms/panels) ---')
for cn, c in sorted(classes.items(), key=lambda kv: -len(kv[1]['fields']))[:12]:
    print(f'  {cn:28s} fields={len(c["fields"]):4d} methods={len(c["methods"]):4d} slots={len(c["slots"]):3d} parent={c["parent"]}')
