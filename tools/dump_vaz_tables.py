"""dump_vaz_tables.py — extract VAZ's runtime-built BSS LUTs (curve, wavetables, filter coefs)
directly from a running Vaz2010.exe via ReadProcessMemory. Bypasses the const-pool-interleaved
builder that linear disassembly can't follow. Gives the EXACT built data.

Tables (VA at the Core.dll preferred base): 0x5445e0 = envelope/curve LUT (exponential),
0x6dd2c0/0x6de2c0/... = oscillator wavetables (mip levels), 0x5535e4/0x69x5e4 = filter coefs.
Usage: py dump_vaz_tables.py   (launches VAZ, dumps, saves .txt to tools/vaz_tables/, closes).
"""
import sys, os, ctypes, struct
from ctypes import wintypes, Structure, c_char, sizeof, byref, c_void_p, c_byte, c_size_t, POINTER
import pefile
sys.path.insert(0, os.path.join(os.path.dirname(__file__), 'vaz_auto'))
from vaz_auto import VazAuto

CORE = r'C:\Program Files (x86)\Steinberg\Vstplugins\VAZ Synths\VAZ 2010\Vaz2010Core.dll'
OUT  = os.path.join(os.path.dirname(__file__), 'vaz_tables')
k32  = ctypes.windll.kernel32

class ME32(Structure):
    _fields_ = [('dwSize',wintypes.DWORD),('th32ModuleID',wintypes.DWORD),('th32ProcessID',wintypes.DWORD),
                ('GlblcntUsage',wintypes.DWORD),('ProccntUsage',wintypes.DWORD),('modBaseAddr',POINTER(c_byte)),
                ('modBaseSize',wintypes.DWORD),('hModule',wintypes.HMODULE),('szModule',c_char*256),('szExePath',c_char*260)]

def core_base(pid):
    snap = k32.CreateToolhelp32Snapshot(0x18, pid)      # SNAPMODULE | SNAPMODULE32
    me = ME32(); me.dwSize = sizeof(me)
    if k32.Module32First(snap, byref(me)):
        while True:
            if b'Vaz2010Core' in me.szModule:
                return ctypes.cast(me.modBaseAddr, c_void_p).value
            if not k32.Module32Next(snap, byref(me)): break
    return None

def readmem(h, addr, n):
    buf = (c_byte*n)(); got = c_size_t(0)
    k32.ReadProcessMemory(h, c_void_p(addr), buf, n, byref(got))
    return bytes(bytearray(buf))[:got.value]

# (name, VA, count) — int32 tables
TABLES = [('curve_0x5445e0', 0x5445e0, 15360),
          ('wave_0x6dd2c0',  0x6dd2c0, 1024),
          ('wave_0x6de2c0',  0x6de2c0, 1024),
          ('wave_0x6db2c0',  0x6db2c0, 1024),
          ('wave_0x6dc2c0',  0x6dc2c0, 1024)]

# Envelope RATE-coef tables (one-pole Q32 alphas; saved RAW int32 → alpha = (raw&0xffffffff)/2^32).
# Contiguous block from 0x6db7e8: decay/release rates (DAT_006db7e8); the attack table DAT_006db818
# is the SAME block +12 entries (0x6db818-0x6db7e8 = 0x30). Curve-mode sustain DAT_006dc0c0 +
# the stage-0 decrement const DAT_006dc0bc dumped from 0x6dc0bc.
ENVTABLES = [('env_rate_006db7e8', 0x6db7e8, 720),
             ('env_susc_006dc0bc', 0x6dc0bc, 264),
             ('lfo3_rate_006dc4c0', 0x6dc4c0, 256)]   # LFO3 rate table: phase increment per 0..174 selector

def main():
    os.makedirs(OUT, exist_ok=True)
    pref = pefile.PE(CORE).OPTIONAL_HEADER.ImageBase
    vaz = VazAuto().launch(wait=5.0)
    try:
        pid = vaz.app.process
        base = core_base(pid); delta = base - pref
        h = k32.OpenProcess(0x410, False, pid)           # VM_READ | QUERY_INFORMATION
        print(f'Core.dll @0x{base:X} (delta 0x{delta:X})')
        for name, va, cnt in TABLES:
            raw = readmem(h, va + delta, cnt*4)
            if len(raw) < cnt*4:
                print(f'  {name}: short read ({len(raw)}B)'); continue
            vals = struct.unpack(f'<{cnt}i', raw)
            f = [v/2**31 for v in vals]
            with open(os.path.join(OUT, name+'.txt'), 'w') as fp:
                fp.write('\n'.join(f'{v:.8f}' for v in f))
            print(f'  {name}: saved {cnt} vals  range [{min(f):+.3f}, {max(f):+.3f}]  '
                  f'first8: ' + ' '.join(f'{f[i]:+.3f}' for i in range(8)))
        for name, va, cnt in ENVTABLES:                  # env rate tables → RAW int32
            raw = readmem(h, va + delta, cnt*4)
            if len(raw) < cnt*4:
                print(f'  {name}: short read ({len(raw)}B)'); continue
            vals = struct.unpack(f'<{cnt}i', raw)
            with open(os.path.join(OUT, name+'.txt'), 'w') as fp:
                fp.write('\n'.join(str(v) for v in vals))
            u = [v & 0xffffffff for v in vals]
            print(f'  {name}: saved {cnt} ints  first12(hex): ' + ' '.join(f'{x:08x}' for x in u[:12]))
        k32.CloseHandle(h)
    finally:
        vaz.close()

if __name__ == '__main__':
    main()
