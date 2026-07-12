"""dump_saw_table.py — dump VAZ's static oscillator wavetable DAT_005441d4[0..256] and characterize it:
naive ramp (aliased saw) vs band-limited. Read from the DLL (static .data) or note if BSS/runtime.
VAZ osc read: idx=phase>>24 (256 entries), lerp to [idx+1]. Compare shape to an ideal saw."""
import pefile, struct
PATH = r'tools\Vaz2010Core.dll'
pe = pefile.PE(PATH); ib = pe.OPTIONAL_HEADER.ImageBase
data = open(PATH, 'rb').read()
def va_to_off(va):
    for s in pe.sections:
        st = ib + s.VirtualAddress
        if st <= va < st + s.SizeOfRawData:
            return s.PointerToRawData + (va - st), s.Name.rstrip(b'\x00').decode('latin1')
    return None, None
VA = 0x5441d4
off, sec = va_to_off(VA)
if off is None:
    print("DAT_005441d4 NOT in file (BSS/runtime-built) — needs RPM dump like the other LUTs"); raise SystemExit
print(f"DAT_005441d4 in section {sec} @ file 0x{off:X}")
vals = struct.unpack('<257i', data[off:off+257*4])   # 256 + wrap
nz = sum(1 for v in vals if v != 0)
print(f"257 entries, {nz} nonzero, range [{min(vals)}, {max(vals)}]")
print("first 8:", vals[:8])
print("mid  8 :", vals[124:132])
print("last 8 :", vals[248:257])
# naive ramp? check linearity: a naive saw is monotonic linear. band-limited has Gibbs ripple/overshoot.
import statistics
diffs = [vals[i+1]-vals[i] for i in range(256)]
mono = all(d >= 0 for d in diffs) or all(d <= 0 for d in diffs)
print(f"monotonic={mono}  (naive ramp = monotonic linear; band-limited = has overshoot/ripple, non-monotonic near the jump)")
# where's the discontinuity (saw jump)?
jump = max(range(256), key=lambda i: abs(diffs[i]))
print(f"largest step at idx {jump}: {diffs[jump]}  (the saw discontinuity)")
# ripple: std of the step sizes away from the jump (naive = ~constant step)
away = [diffs[i] for i in range(256) if abs(i-jump) > 8]
print(f"step size away from jump: mean={statistics.mean(away):.0f} std={statistics.pstdev(away):.0f}  (std≈0 → naive linear ramp; std large → band-limited ripple)")
