#!/usr/bin/env python3
# Dump the float table after the 10 comb tunings (0x523158..0x523188) to find the 8 allpass base-lengths.
import struct
data = open(r"C:\APC\y\tools\Vaz2010Core.dll","rb").read()
pe = struct.unpack_from("<I",data,0x3C)[0]; nsec=struct.unpack_from("<H",data,pe+6)[0]
opt=pe+24; image_base=struct.unpack_from("<I",data,opt+28)[0]; sec_off=opt+struct.unpack_from("<H",data,pe+20)[0]
secs=[]
for i in range(nsec):
    o=sec_off+i*40; vsz,va,rsz,rp=struct.unpack_from("<IIII",data,o+8)
    secs.append((va,vsz,rp,rsz))
def off(va):
    rva=va-image_base
    for vaddr,vsize,rp,rsz in secs:
        if vaddr<=rva<vaddr+max(vsize,rsz):
            return rp+(rva-vaddr) if rva-vaddr<rsz else None
    return None
print("comb tunings 0x523158..0x523188 (10):")
for i in range(10):
    va=0x523158+i*4; o=off(va)
    print("  0x%X: f32=%r" % (va, struct.unpack_from("<f",data,o)[0]))
print("following table 0x52318c..0x5231cc (candidate allpass base-lengths + damp consts):")
for i in range(20):
    va=0x52318c+i*4; o=off(va)
    if o: print("  0x%X: f32=%r  i32=%d" % (va, struct.unpack_from("<f",data,o)[0], struct.unpack_from("<i",data,o)[0]))
