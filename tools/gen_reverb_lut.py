#!/usr/bin/env python3
# Generate reference/vaz_reverb_coef_lut.h from the runtime dump vaz_coef_dump.csv.
import re
rows = {}       # size -> [10 coefs]
damp = {}       # damp -> damp2
for line in open(r"C:\APC\y\tools\vaz_coef_dump.csv"):
    line = line.strip()
    if line.startswith("R,"):
        p = line.split(","); rows[int(p[1])] = [int(x, 16) for x in p[2:12]]
    elif line.startswith("D,"):
        p = line.split(","); damp[int(p[1])] = int(p[2], 16)
assert len(rows) == 256 and len(damp) == 256, (len(rows), len(damp))

out = []
out.append("// vaz_reverb_coef_lut.h — EXACT VAZ reverb coefficients, runtime-dumped from Vaz2010Core.dll by calling")
out.append("// the real 80-bit x87 setters (FUN_00522fcc size->coef, FUN_00523194 damp->coef) at sr=44100, size/damp")
out.append("// 0..255. Replaces the RT60 approximation (VAZ's 80-bit float is not MSVC-reproducible). See tools/vaz_coef_dump.cpp.")
out.append("//   Dump validated: comb lengths came out = tunings {1116,1187,1277,1356,1422,1491,1557,1617,1203,1527} @44.1k.")
out.append("#pragma once")
out.append("#include <cstdint>")
out.append("namespace vazfx {")
out.append("// comb feedback coef (Q31) per size 0..255, combs 0..9 (render uses 0..8; comb9 vestigial). @sr=44100.")
out.append("inline constexpr uint32_t kReverbCombCoefLUT[256][10] = {")
for s in range(256):
    out.append("  {" + ",".join("0x%08Xu" % v for v in rows[s]) + "}," )
out.append("};")
out.append("// damping one-pole damp2 (Q28) per damp 0..255; damp1 = 0x10000000 - damp2.")
out.append("inline constexpr uint32_t kReverbDamp2LUT[256] = {")
for i in range(0, 256, 8):
    out.append("  " + ",".join("0x%08Xu" % damp[j] for j in range(i, i+8)) + ",")
out.append("};")
out.append("} // namespace vazfx")
open(r"C:\APC\y\plugins\VAZClone\reference\vaz_reverb_coef_lut.h", "w").write("\n".join(out) + "\n")
print("wrote vaz_reverb_coef_lut.h: %d sizes, %d damps" % (len(rows), len(damp)))
print("coef range comb0: size0=0x%08X size255=0x%08X" % (rows[0][0], rows[255][0]))
print("damp2 range: d0=0x%08X d255=0x%08X" % (damp[0], damp[255]))
