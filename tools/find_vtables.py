#!/usr/bin/env python3
# Find TFX* vtables via class-name-string xref, then list nearby CODE pointers (candidate virtual methods).
import struct

DLL = r"C:\APC\y\tools\Vaz2010Core.dll"
data = open(DLL, "rb").read()

pe = struct.unpack_from("<I", data, 0x3C)[0]
nsec = struct.unpack_from("<H", data, pe+6)[0]
opt = pe + 24
image_base = struct.unpack_from("<I", data, opt+28)[0]
sec_off = opt + struct.unpack_from("<H", data, pe+20)[0]
secs = []
for i in range(nsec):
    o = sec_off + i*40
    name = data[o:o+8].rstrip(b"\0").decode(errors="replace")
    vsize, vaddr, rawsize, rawptr = struct.unpack_from("<IIII", data, o+8)
    secs.append((name, vaddr, vsize, rawptr, rawsize))

def va_to_off(va):
    rva = va - image_base
    for name, vaddr, vsize, rawptr, rawsize in secs:
        if vaddr <= rva < vaddr + max(vsize, rawsize):
            if rva - vaddr >= rawsize: return None
            return rawptr + (rva - vaddr)
    return None
def off_to_va(off):
    for name, vaddr, vsize, rawptr, rawsize in secs:
        if rawptr <= off < rawptr + rawsize:
            return image_base + vaddr + (off - rawptr)
    return None

# CODE section bounds (for identifying method pointers)
code = [s for s in secs if s[0] == "CODE"][0]
code_lo = image_base + code[1]
code_hi = image_base + code[1] + code[2]

def find_string_offsets(s):
    # Delphi Pascal string: <len byte><chars>, no null. Match len-prefixed occurrences.
    b = s.encode()
    idx = data.find(b)
    out = []
    while idx != -1:
        if idx > 0 and data[idx-1] == len(s):   # length-prefix byte matches
            out.append(idx)
        idx = data.find(b, idx+1)
    return out

for cls in ["TFXReverb"]:
    print("=== %s ===" % cls)
    soffs = find_string_offsets(cls)
    # VMT vmtClassName points at the length byte (off-1)
    svas = [off_to_va(o-1) for o in soffs if off_to_va(o-1)]
    print("  name-string VA(s) [len byte]:", [hex(v) for v in svas])
    for sva in svas:
        needle = struct.pack("<I", sva)
        idx = data.find(needle)
        while idx != -1:
            ptr_va = off_to_va(idx)
            if ptr_va:
                # dump CODE-range pointers in the window [idx, idx+0x120) = likely the vtable methods
                meths = []
                for w in range(0, 0x200, 4):
                    off = idx + w
                    if off+4 > len(data): break
                    val = struct.unpack_from("<I", data, off)[0]
                    if code_lo <= val < code_hi:
                        meths.append(val)
                # class-own methods = FX range 0x515000..0x523000 (exclude shared TFXBase 0x4xxxxx)
                own = sorted(set(m for m in meths if 0x515000 <= m < 0x523000))
                if own:
                    print("  vtable @0x%X -> class-own FX methods: %s" % (
                        ptr_va, " ".join("0x%X" % m for m in own)))
            idx = data.find(needle, idx+1)
    print()
