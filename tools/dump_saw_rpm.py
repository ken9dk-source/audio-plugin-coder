"""dump_saw_rpm.py — RPM-dump VAZ's runtime-built oscillator wavetable DAT_005441d4[0..256] and
characterize it (naive linear ramp = aliased saw, vs band-limited). Same launch+ReadProcessMemory
method as dump_vaz_tables.py / dump_note_idx.py."""
import sys, os, ctypes, struct, statistics
from ctypes import wintypes, Structure, c_char, sizeof, byref, c_void_p, c_byte, c_size_t, POINTER
import pefile
sys.path.insert(0, os.path.join(os.path.dirname(__file__), 'vaz_auto'))
from vaz_auto import VazAuto
CORE = r'C:\Program Files (x86)\Steinberg\Vstplugins\VAZ Synths\VAZ 2010\Vaz2010Core.dll'
HERE = os.path.dirname(__file__); k32 = ctypes.windll.kernel32
class ME32(Structure):
    _fields_ = [('dwSize',wintypes.DWORD),('th32ModuleID',wintypes.DWORD),('th32ProcessID',wintypes.DWORD),
                ('GlblcntUsage',wintypes.DWORD),('ProccntUsage',wintypes.DWORD),('modBaseAddr',POINTER(c_byte)),
                ('modBaseSize',wintypes.DWORD),('hModule',wintypes.HMODULE),('szModule',c_char*256),('szExePath',c_char*260)]
def core_base(pid):
    snap=k32.CreateToolhelp32Snapshot(0x18,pid); me=ME32(); me.dwSize=sizeof(me)
    if k32.Module32First(snap,byref(me)):
        while True:
            if b'Vaz2010Core' in me.szModule: return ctypes.cast(me.modBaseAddr,c_void_p).value
            if not k32.Module32Next(snap,byref(me)): break
    return None
def readmem(h,addr,n):
    buf=(c_byte*n)(); got=c_size_t(0); k32.ReadProcessMemory(h,c_void_p(addr),buf,n,byref(got)); return bytes(bytearray(buf))[:got.value]
def main():
    pref=pefile.PE(CORE).OPTIONAL_HEADER.ImageBase
    vaz=VazAuto().launch(wait=5.0)
    try:
        pid=vaz.app.process; base=core_base(pid); delta=base-pref
        h=k32.OpenProcess(0x410,False,pid)
        raw=readmem(h,0x5441d4+delta,257*4); vals=struct.unpack('<257i',raw)
        with open(os.path.join(HERE,'vaz_tables','saw_0x5441d4.txt'),'w') as fp: fp.write('\n'.join(str(v) for v in vals))
        nz=sum(1 for v in vals if v!=0)
        print(f"DAT_005441d4: 257 vals, {nz} nonzero, range [{min(vals)},{max(vals)}]")
        print("first8:", vals[:8]); print("last8 :", vals[249:257])
        diffs=[vals[i+1]-vals[i] for i in range(256)]
        jump=max(range(256),key=lambda i:abs(diffs[i]))
        away=[diffs[i] for i in range(256) if abs(i-jump)>8]
        mono=all(d>=0 for d in diffs) or all(d<=0 for d in diffs)
        print(f"monotonic={mono}  jump@idx{jump}={diffs[jump]}  step-away mean={statistics.mean(away):.0f} std={statistics.pstdev(away):.0f}")
        print("=> naive linear ramp if std≈0 & monotonic; band-limited if ripple/overshoot (std large, non-monotonic near jump)")
        k32.CloseHandle(h)
    finally:
        vaz.close()
if __name__=='__main__': main()
