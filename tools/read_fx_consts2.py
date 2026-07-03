#!/usr/bin/env python3
# Read Phaser/Decimator LUT-builder constants from Vaz2010Core.dll by VA.
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
    vsize, vaddr, rawsize, rawptr = struct.unpack_from("<IIII", data, o+8)
    secs.append((data[o:o+8].rstrip(b"\0").decode(errors='replace'), vaddr, vsize, rawptr, rawsize))
def off(va):
    rva = va - image_base
    for nm,vaddr,vsize,rawptr,rawsize in secs:
        if vaddr <= rva < vaddr+max(vsize,rawsize):
            if rva-vaddr >= rawsize: return None
            return rawptr+(rva-vaddr)
    return None
def f32(va):
    o=off(va);  return struct.unpack_from("<f",data,o)[0] if o else None
def f64(va):
    o=off(va);  return struct.unpack_from("<d",data,o)[0] if o else None
def i32(va):
    o=off(va);  return struct.unpack_from("<i",data,o)[0] if o else None

print("=== PHASER coef-LUT builder (FUN_00521aa0): coef[i] from i*5*ln2 ... ===")
for va,lbl in [(0x521b40,"b40"),(0x521b4c,"b4c"),(0x521b50,"b50"),(0x521b54,"b54"),(0x521b58,"b58"),(0x521b5c,"b5c")]:
    print("  DAT_00%X %s: f32=%r  f64=%r  i32=%r" % (va,lbl,f32(va),f64(va),i32(va)))
print("=== DECIMATOR quant-table builder (FUN_0051d784): table[i]=i*ln2*d7e4/d7f0/d7f4 ===")
for va,lbl in [(0x51d7e4,"d7e4"),(0x51d7f0,"d7f0"),(0x51d7f4,"d7f4")]:
    print("  DAT_00%X %s: f32=%r  f64=%r  i32=%r" % (va,lbl,f32(va),f64(va),i32(va)))
