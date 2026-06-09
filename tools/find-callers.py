# Scan Vaz2010Core.dll .text for `call rel32` (E8) sites whose target == any given VA.
# Maps the call graph so we can locate the per-block / per-sample flanger routine.
import sys, struct, pefile
PATH = r'C:\Program Files (x86)\Steinberg\Vstplugins\VAZ Synths\VAZ 2010\Vaz2010Core.dll'
targets = set(int(a, 16) for a in sys.argv[1:]) or {0x51f430}

pe = pefile.PE(PATH); base = pe.OPTIONAL_HEADER.ImageBase
raw = open(PATH, 'rb').read()
# also scan for the target appearing as a 32-bit immediate (push <addr> / mov reg,<addr> / dd vtable)
targ_le = {t: struct.pack('<I', t) for t in targets}

IMAGE_SCN_MEM_EXECUTE = 0x20000000
for s in pe.sections:
    name = s.Name.rstrip(b'\x00').decode('latin1')
    sva = base + s.VirtualAddress
    data = raw[s.PointerToRawData: s.PointerToRawData + s.SizeOfRawData]
    is_code = bool(s.Characteristics & IMAGE_SCN_MEM_EXECUTE)
    # E8 rel32 calls (only meaningful in executable sections)
    if is_code:
        for i in range(len(data) - 5):
            if data[i] == 0xE8:
                rel = struct.unpack('<i', data[i+1:i+5])[0]
                site = sva + i
                tgt = (site + 5 + rel) & 0xFFFFFFFF
                if tgt in targets:
                    print(f'  CALL @ 0x{site:X}  -> 0x{tgt:X}   [.text]')
            if data[i] == 0xE9:  # jmp rel32 (tail-call / thunk)
                rel = struct.unpack('<i', data[i+1:i+5])[0]
                site = sva + i
                tgt = (site + 5 + rel) & 0xFFFFFFFF
                if tgt in targets:
                    print(f'  JMP  @ 0x{site:X}  -> 0x{tgt:X}   [.text]')
    # raw 32-bit immediate occurrences (vtable entries, push addr) in any section
    for t, le in targ_le.items():
        off = 0
        while True:
            j = data.find(le, off)
            if j < 0:
                break
            print(f'  ADDR 0x{t:X} referenced @ 0x{sva+j:X}   [{name}]')
            off = j + 1
