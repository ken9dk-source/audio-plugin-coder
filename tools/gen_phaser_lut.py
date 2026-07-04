#!/usr/bin/env python3
# Generate reference/vaz_phaser_coef_lut.h from the runtime dump (P lines in vaz_coef_dump.csv).
lut = {}
for line in open(r"C:\APC\y\tools\vaz_coef_dump.csv"):
    if line.startswith("P,"):
        p = line.split(","); lut[int(p[1])] = int(p[2], 16)
assert len(lut) == 512, len(lut)
out = []
out.append("// vaz_phaser_coef_lut.h — EXACT VAZ phaser 512-entry allpass-coef LUT, runtime-dumped from Vaz2010Core.dll")
out.append("// by calling the real 80-bit builder FUN_00521aa0 @0x521aa0 at sr=44100. coef[i] = (1 − i·5·ln2·440/255/sr)·2^30")
out.append("// clamp≥0 (DAT_00521b50=440, b4c=255, b58=2^30). Replaces the clone's tan()-bilinear approximation. Q30.")
out.append("//   Validated: monotonically decreasing 0.99988 … 0.87932; nonzero. See tools/vaz_coef_dump.cpp.")
out.append("#pragma once")
out.append("#include <cstdint>")
out.append("namespace vazfx {")
out.append("inline constexpr uint32_t kPhaserCoefLUT[512] = {")
for i in range(0, 512, 8):
    out.append("  " + ",".join("0x%08Xu" % lut[j] for j in range(i, i+8)) + ",")
out.append("};")
out.append("} // namespace vazfx")
open(r"C:\APC\y\plugins\VAZClone\reference\vaz_phaser_coef_lut.h", "w", encoding="utf-8").write("\n".join(out) + "\n")
print("wrote vaz_phaser_coef_lut.h: 512 entries, coef[0]=0x%08X coef[511]=0x%08X" % (lut[0], lut[511]))
