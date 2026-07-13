"""dump_engine_vmt.py — locate the LIVE VAZ DSP engine + a sounding voice, then read its LFO object.
Unblocks Phase-2 #6/#8/#9 + Phase-4 (all share this "find the live engine struct" blocker, status-map §3.0).

Method (refined after run 1): the engine is new'd in the host EXE as a class derived from TBaseMidSynth, so
[engine+0] is a DERIVED vtable, not the base VMT @coreBase+0xd4614 (that VMT is verified live: 12/12 code ptrs).
At rest the voice array (engine+0x2534, 32 ptrs) is null, so we HOLD a MIDI note to allocate + animate voices,
take TWO heap snapshots ~60 ms apart, and find the object whose voices have CHANGING phase fields (voice+0x7c
phase / +0x80 inc / +0xbc pitch, from the decompile) — the unique signature of the sounding engine. Then read
voice[0]+0x28 = the LFO object."""
import sys, os, ctypes, struct, time
from ctypes import wintypes, Structure, c_char, sizeof, byref, c_void_p, c_byte, c_size_t, c_ulonglong, c_ulong, POINTER
import numpy as np, pefile
sys.path.insert(0, os.path.join(os.path.dirname(__file__), 'vaz_auto'))
from vaz_auto import VazAuto

CORE = r'C:\Program Files (x86)\Steinberg\Vstplugins\VAZ Synths\VAZ 2010\Vaz2010Core.dll'
VMT_RVA = 0xd4614; VOFF = 0x2534
k32 = ctypes.windll.kernel32

class ME32(Structure):
    _fields_ = [('dwSize',wintypes.DWORD),('a',wintypes.DWORD),('pid',wintypes.DWORD),('b',wintypes.DWORD),
                ('c',wintypes.DWORD),('modBaseAddr',POINTER(c_byte)),('modBaseSize',wintypes.DWORD),
                ('hModule',wintypes.HMODULE),('szModule',c_char*256),('szExePath',c_char*260)]
class MBI(Structure):
    _fields_ = [('BaseAddress',c_ulonglong),('AllocationBase',c_ulonglong),('AllocationProtect',c_ulong),
                ('__a1',c_ulong),('RegionSize',c_ulonglong),('State',c_ulong),('Protect',c_ulong),
                ('Type',c_ulong),('__a2',c_ulong)]

def core_base(pid):
    snap=k32.CreateToolhelp32Snapshot(0x18,pid); me=ME32(); me.dwSize=sizeof(me)
    if k32.Module32First(snap,byref(me)):
        while True:
            if b'Vaz2010Core' in me.szModule: return ctypes.cast(me.modBaseAddr,c_void_p).value
            if not k32.Module32Next(snap,byref(me)): break
def rd(h,a,n):
    buf=(c_byte*n)(); got=c_size_t(0)
    return bytes(bytearray(buf))[:got.value] if k32.ReadProcessMemory(h,c_void_p(a),buf,n,byref(got)) else b''

def main():
    vaz=VazAuto(midi_hint='loop').launch(wait=5.0)
    mo=None
    try:
        pid=vaz.app.process; base=core_base(pid); vmt=base+VMT_RVA
        h=k32.OpenProcess(0x410,False,pid)
        print(f'Core.dll @0x{base:X}   TBaseMidSynth VMT=0x{vmt:X}')
        slots=struct.unpack('<12I', rd(h,vmt,48)); incode=sum(1 for s in slots if base<=s<base+0x200000)
        print(f'  VMT verify: {incode}/12 slots into .text')
        # hold a note so voices allocate + move
        try:
            import rtmidi
            mo=rtmidi.MidiOut(); pi=next(i for i,p in enumerate(mo.get_ports()) if 'loop' in p.lower()); mo.open_port(pi)
            mo.send_message([0x90,60,110]); time.sleep(0.4); print('  holding note 60...')
        except Exception as e:
            print('  no MIDI hold (',e,') — voices may be null'); mo=None
        # committed private RW regions
        regions=[]; a=0x10000
        while a<0xFFFF0000:
            m=MBI()
            if k32.VirtualQueryEx(h,c_void_p(a),byref(m),sizeof(m))==0: break
            if m.State==0x1000 and m.Type==0x20000 and (m.Protect&0x04): regions.append((m.BaseAddress,min(m.RegionSize,96*1024*1024)))
            a=m.BaseAddress+m.RegionSize
        # two snapshots -> changed words
        snapA={b:rd(h,b,s) for b,s in regions}; time.sleep(0.06); snapB={}; changed=set()
        for b,s in regions:
            d2=rd(h,b,s); snapB[b]=d2; d1=snapA.get(b,b''); n=min(len(d1),len(d2))//4*4
            if n:
                a1=np.frombuffer(d1[:n],dtype='<u4'); a2=np.frombuffer(d2[:n],dtype='<u4')
                for w in np.nonzero(a1!=a2)[0]: changed.add(b+int(w)*4)
        print(f'  {len(regions)} heap regions, {len(changed)} words moving while the note sounds')
        # voice-array candidates whose voices have MOVING phase fields
        eng=[]
        for b,s in regions:
            data=snapB[b]
            if len(data)<200: continue
            arr=np.frombuffer(data[:len(data)//4*4],dtype='<u4')
            plaus=(arr>0x100000)&(arr<0xFFFF0000)&((arr&3)==0)
            cs=np.concatenate(([0],np.cumsum(plaus.astype(np.int32))))
            for j in range(0,len(arr)-6):
                if cs[j+6]-cs[j]==6:
                    objva=b+j*4-VOFF
                    if objva<0: continue
                    vs=[int(x) for x in arr[j:j+32]]
                    act=sum(1 for p in vs for off in (0x7c,0x80,0xbc) if (p+off) in changed)
                    if act>=3: eng.append((objva,act,vs))
        seen=set(); eng=[e for e in eng if not (e[0] in seen or seen.add(e[0]))]; eng.sort(key=lambda e:-e[1])
        print(f'  {len(eng)} engine candidate(s) with MOVING voice phase fields:')
        for objva,act,vs in eng[:4]:
            vtp=struct.unpack('<I',rd(h,objva,4))[0]
            lfop=struct.unpack('<I',rd(h,vs[0]+0x28,4))[0]
            print(f'   engine @0x{objva:X}  vtRVA 0x{vtp-base:X}{" ==TBaseMidSynth" if vtp==vmt else ""}  {act} moving fields  voice[0]=0x{vs[0]:X}  v0+0x28=0x{lfop:X}')
            if 0x100000<lfop<0xFFFF0000:
                lf=rd(h,lfop,0x40)
                if len(lf)>=0x40:
                    print('      LFO obj [0..0x40): '+' '.join(f'{struct.unpack("<i",lf[k:k+4])[0]}' for k in range(0,0x40,4)))
        if not eng: print('   none — note may not be reaching VAZ (check loopMIDI + VAZ MIDI-in), or voice offsets differ.')
        if mo: mo.send_message([0x80,60,0]); mo.close_port()
        k32.CloseHandle(h)
    finally:
        vaz.close()

if __name__=='__main__': main()
