"""probe_control.py — for a VAZ control name: its form-object offset, its published handler(s), and
the handler's decompiled body (the msTimebaseChange shape is: read [form+ctrlOff] -> [ctrl+0x1c0]
value -> call setter(dspObj, value), which pins the DSP field). Reusable for the P2/P3 gap sweep.

  py probe_control.py msAmpPMSource msLFOPeriod ...
"""
import sys, json, struct, re
import pefile, capstone

CORE = r'C:\Program Files (x86)\Steinberg\Vstplugins\VAZ Synths\VAZ 2010\Vaz2010Core.dll'
pe = pefile.PE(CORE); base = pe.OPTIONAL_HEADER.ImageBase; data = pe.__data__
md = capstone.Cs(capstone.CS_ARCH_X86, capstone.CS_MODE_32)
uni = json.load(open(r'C:\APC\y\tools\vaz_name_universe.json'))
methods = {k: int(v, 16) for k, v in uni['methods'].items()}
fields  = {k: int(v, 16) for k, v in uni['fields'].items()}

def disasm(va, n=0x50, indent='      '):
    try: off = pe.get_offset_from_rva(va - base)
    except Exception: print(indent + 'bad VA'); return []
    out = []
    cnt = 0
    for ins in md.disasm(data[off:off + n], va):
        s = f'{indent}0x{ins.address:X}: {ins.mnemonic} {ins.op_str}'
        print(s); out.append((ins.mnemonic, ins.op_str, ins.address))
        cnt += 1
        if ins.mnemonic == 'ret': break
        if cnt > 40: break
    return out

def probe(name):
    print(f'\n===== {name} =====')
    if name in fields: print(f'  control field  @form+0x{fields[name]:x}')
    else:              print('  (no published field with that exact name)')
    hs = sorted([(k, v) for k, v in methods.items() if k.startswith(name)])
    if not hs:
        print('  NO published handler starting with this name')
        # sibling handlers that merely contain the stem
        stem = re.sub(r'^(ms|sb|bt|cb|ed|ud|rb)', '', name)
        sib = sorted([(k, v) for k, v in methods.items() if stem and stem.lower() in k.lower()])
        for k, v in sib[:6]: print(f'    ~ related handler: {k} -> 0x{v:X}')
        return
    for hn, ha in hs:
        print(f'  handler {hn} -> 0x{ha:X}')
        disasm(ha)

for n in sys.argv[1:]:
    probe(n)
