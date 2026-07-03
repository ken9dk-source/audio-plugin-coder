#!/usr/bin/env python3
# Read static reverb constants from Vaz2010Core.dll by VA (PE section map).
import struct, sys

DLL = r"C:\APC\y\tools\Vaz2010Core.dll"
data = open(DLL, "rb").read()

# --- minimal PE parse ---
pe = struct.unpack_from("<I", data, 0x3C)[0]
assert data[pe:pe+4] == b"PE\0\0"
nsec = struct.unpack_from("<H", data, pe+6)[0]
opt = pe + 24
image_base = struct.unpack_from("<I", data, opt+28)[0]
sec_off = opt + struct.unpack_from("<H", data, pe+20)[0]  # size of optional header
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
            if rva - vaddr >= rawsize:
                return None, name  # in BSS (uninitialised, zero in file)
            return rawptr + (rva - vaddr), name
    return None, "?"

print("image_base = 0x%X" % image_base)
print("sections:", [(s[0], hex(image_base+s[1])) for s in secs])
print()

def show_i32(va, label):
    off, sec = va_to_off(va)
    if off is None:
        print("  %-16s @0x%X  [%s]  (not in file / BSS)" % (label, va, sec))
        return
    v = struct.unpack_from("<i", data, off)[0]
    u = v & 0xFFFFFFFF
    print("  %-16s @0x%X  int=%d  hex=0x%08X  /2^31=%.9f  /2^28=%.9f  [%s]"
          % (label, va, v, u, u/2**31, u/2**28, sec))

def show_f32(va, label):
    off, sec = va_to_off(va)
    if off is None:
        print("  %-16s @0x%X  [%s]  (not in file / BSS)" % (label, va, sec)); return
    f = struct.unpack_from("<f", data, off)[0]
    print("  %-16s @0x%X  f32=%r  [%s]" % (label, va, f, sec))

print("=== allpass gain (shared, DAT_0052ba54) ===")
show_i32(0x52ba54, "DAT_0052ba54")
show_f32(0x52ba54, "  (as f32)")
print("=== comb-coef seed const DAT_0052314c (-1.35e-5) ===")
show_f32(0x52314c, "DAT_0052314c")
print("=== a few neighbours of ba54 (in case it's a small table) ===")
for va in (0x52ba50, 0x52ba54, 0x52ba58, 0x52ba5c):
    show_i32(va, "i32")
