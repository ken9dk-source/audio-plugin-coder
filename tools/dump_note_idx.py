"""dump_note_idx.py — RPM-dump VAZ's runtime note->pitch-index table DAT_006dd0c0[0..127] and
convert each MIDI note to Hz via the increment curve. Definitive standard-vs-octave check.
Same launch+ReadProcessMemory method as dump_vaz_tables.py (the accepted deterministic dump)."""
import sys, os, ctypes, struct
from ctypes import wintypes, Structure, c_char, sizeof, byref, c_void_p, c_byte, c_size_t, POINTER
import pefile
sys.path.insert(0, os.path.join(os.path.dirname(__file__), 'vaz_auto'))
from vaz_auto import VazAuto

CORE = r'C:\Program Files (x86)\Steinberg\Vstplugins\VAZ Synths\VAZ 2010\Vaz2010Core.dll'
HERE = os.path.dirname(__file__)
k32  = ctypes.windll.kernel32

class ME32(Structure):
    _fields_ = [('dwSize',wintypes.DWORD),('th32ModuleID',wintypes.DWORD),('th32ProcessID',wintypes.DWORD),
                ('GlblcntUsage',wintypes.DWORD),('ProccntUsage',wintypes.DWORD),('modBaseAddr',POINTER(c_byte)),
                ('modBaseSize',wintypes.DWORD),('hModule',wintypes.HMODULE),('szModule',c_char*256),('szExePath',c_char*260)]

def core_base(pid):
    snap = k32.CreateToolhelp32Snapshot(0x18, pid)
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

NAMES = {0:'C',1:'C#',2:'D',3:'D#',4:'E',5:'F',6:'F#',7:'G',8:'G#',9:'A',10:'A#',11:'B'}
def std_hz(n): return 440.0 * 2**((n-69)/12.0)

def main():
    curve = [float(x) for x in open(os.path.join(HERE,'vaz_tables','curve_0x5445e0.txt')).read().split()]
    pref = pefile.PE(CORE).OPTIONAL_HEADER.ImageBase
    vaz = VazAuto().launch(wait=5.0)
    try:
        pid = vaz.app.process
        base = core_base(pid); delta = base - pref
        h = k32.OpenProcess(0x410, False, pid)
        print(f'Core.dll @0x{base:X} (delta 0x{delta:X})')
        raw = readmem(h, 0x6dd0c0 + delta, 128*4)
        idx = struct.unpack('<128i', raw)
        # save raw
        with open(os.path.join(HERE,'vaz_tables','note_idx_006dd0c0.txt'),'w') as fp:
            fp.write('\n'.join(str(v) for v in idx))
        def hz(i): return curve[i]*44100/2.0 if 0 <= i < len(curve) else float('nan')
        print('\nMIDI -> VAZ idx -> VAZ Hz   vs std A440   ratio')
        for n in [0,12,24,36,48,57,60,69,72,81,84,96,108,120,127]:
            i = idx[n]; vhz = hz(i); shz = std_hz(n); r = vhz/shz if shz else 0
            fl = ''
            if 0.48<r<0.52: fl='  <<< OCTAVE LOW'
            elif 0.98<r<1.02: fl='  <<< standard'
            elif 1.9<r<2.1: fl='  <<< OCTAVE HIGH'
            print(f'  MIDI {n:3d} ({NAMES[n%12]:>2s}{n//12-1}): idx={i:5d}(0x{i&0xffffffff:04X})  VAZ={vhz:8.2f}Hz  std={shz:8.2f}Hz  r={r:.3f}{fl}')
        print(f'\noctave step idx[60]-idx[48]={idx[60]-idx[48]}  idx[72]-idx[60]={idx[72]-idx[60]}  (1536=12*128 expected)')
        print(f'idx[69] (A4) = {idx[69]}  — A440 increment-curve index is ~8837 if standard')
        k32.CloseHandle(h)
    finally:
        vaz.close()

if __name__ == '__main__':
    main()
