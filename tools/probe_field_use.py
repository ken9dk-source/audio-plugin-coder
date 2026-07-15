"""probe_field_use.py — find where a FORM-object offset (a control field, e.g. msAmpPMSource@0x8d4)
is READ in code, and disassemble around each ref. The msTimebaseChange shape is:
    mov edx,[form+ctrlOff] ; mov edx,[ctrl+0x1c0] ; mov eax,[form+dspObjOff] ; call setter
so a read of the control offset pins the DSP setter it feeds.

  py probe_field_use.py 0x8d4 [0x6c0 ...]
"""
import sys, struct, re
import pefile, capstone
CORE = r'C:\Program Files (x86)\Steinberg\Vstplugins\VAZ Synths\VAZ 2010\Vaz2010Core.dll'
pe = pefile.PE(CORE); base = pe.OPTIONAL_HEADER.ImageBase; data = pe.__data__
md = capstone.Cs(capstone.CS_ARCH_X86, capstone.CS_MODE_32)
sec = next(s for s in pe.sections if s.Characteristics & 0x20000000)
tb = base + sec.VirtualAddress; td = sec.get_data()

def refs(disp):
    out = []
    pat = struct.pack('<I', disp)
    for m in re.finditer(re.escape(pat), td):
        va = tb + m.start()
        o = m.start()
        # instruction likely starts 2 bytes before the disp32 (opcode+modrm)
        out.append(va - 2)
    return out

def disasm_at(va, n=0x40):
    try: off = pe.get_offset_from_rva(va - base)
    except Exception: return
    for ins in md.disasm(data[off:off + n], va):
        print(f'      0x{ins.address:X}: {ins.mnemonic} {ins.op_str}')
        if ins.mnemonic == 'ret': break

for a in sys.argv[1:]:
    disp = int(a, 16)
    rs = refs(disp)
    print(f'\n===== form offset +0x{disp:x}: {len(rs)} code ref(s) =====')
    for va in rs[:6]:
        print(f'  --- ref @~0x{va:X} ---')
        disasm_at(va, 0x38)
