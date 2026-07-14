"""Runtime dump attempt for the sequencer timing fields. A held note only triggers a voice, NOT the
sequencer transport, so we try MIDI Start (0xFA) + a MIDI Clock burst (0xF8) to actually RUN it, then
scan the heap for the sequencer object via its timing-field signature and read the confirmed offsets:
  timebase [+0x23f0], swing [+0x23f4], straight/swung interval slots [+0x26a8]/[+0x26ac], free-run [+0x2660]."""
import sys, os, ctypes, struct, time
from ctypes import Structure, sizeof, byref, c_void_p, c_byte, c_char, c_size_t, c_ulonglong, c_ulong
import numpy as np
sys.path.insert(0, os.path.join(os.path.dirname(__file__), 'vaz_auto'))
from vaz_auto import VazAuto
k32 = ctypes.windll.kernel32

class MBI(Structure):
    _fields_ = [('BaseAddress',c_ulonglong),('AllocationBase',c_ulonglong),('AllocationProtect',c_ulong),
                ('__a1',c_ulong),('RegionSize',c_ulonglong),('State',c_ulong),('Protect',c_ulong),('Type',c_ulong),('__a2',c_ulong)]
class ME32(Structure):
    _fields_ = [('dwSize',c_ulong),('th32ModuleID',c_ulong),('th32ProcessID',c_ulong),('GlblcntUsage',c_ulong),
                ('ProccntUsage',c_ulong),('modBaseAddr',c_void_p),('modBaseSize',c_ulong),('hModule',c_void_p),
                ('szModule',c_char*256),('szExePath',c_char*260)]
def core_base(pid):
    snap=k32.CreateToolhelp32Snapshot(0x18,pid)          # SNAPMODULE|SNAPMODULE32
    me=ME32(); me.dwSize=sizeof(me); ok=k32.Module32First(snap,byref(me))
    while ok:
        if 'vaz2010core' in me.szModule.decode(errors='replace').lower():
            b=ctypes.cast(me.modBaseAddr,c_void_p).value or 0; k32.CloseHandle(snap); return b,me.modBaseSize
        ok=k32.Module32Next(snap,byref(me))
    k32.CloseHandle(snap); return None,None
def rd(h,a,n):
    buf=(c_byte*n)(); got=c_size_t(0)
    return bytes(bytearray(buf))[:got.value] if k32.ReadProcessMemory(h,c_void_p(a),buf,n,byref(got)) else b''
def regions(h):
    out=[]; a=0x10000
    while a<0xFFFF0000:
        m=MBI()
        if k32.VirtualQueryEx(h,c_void_p(a),byref(m),sizeof(m))==0: break
        if m.State==0x1000 and m.Type==0x20000 and (m.Protect&0x04): out.append((m.BaseAddress,min(m.RegionSize,64*1024*1024)))
        a=m.BaseAddress+m.RegionSize
    return out

def main():
    vaz=VazAuto(midi_hint='loop').launch(wait=5.0)
    try:
        pid=vaz.app.process; h=k32.OpenProcess(0x410,False,pid)
        coreB,coreS=core_base(pid)
        print(f'  Vaz2010Core.dll @ 0x{coreB:X} (size 0x{coreS:X})' if coreB else '  Core.dll base NOT found')
        try:
            from vaz_auto import shared_midiout
            mo=shared_midiout('loop')
            mo.send_message([0x90,60,110])                       # hold a note (a voice)
            mo.send_message([0xFA])                              # MIDI Start -> transport
            for _ in range(96): mo.send_message([0xF8]); time.sleep(0.004)   # ~clock burst
            print('  sent: note 60 + MIDI Start(0xFA) + 96 clocks(0xF8)')
        except Exception as e:
            print('  midi error:', e)
        time.sleep(0.3)
        found=[]; checked=0
        for b,s in regions(h):
            d=rd(h,b,s)
            if len(d)<0x2800: continue
            arr=np.frombuffer(d[:len(d)//4*4],dtype='<u4')
            # candidate: two ADJACENT plausible sample-interval dwords (the straight/swung slots @ +0x26a8/+0x26ac)
            pl=(arr>500)&(arr<600000)
            cand=np.nonzero(pl[:-1]&pl[1:])[0]
            for i in cand[:200000]:
                obj=b+int(i)*4-0x26a8
                if obj<b: continue
                checked+=1
                head=rd(h,obj,4)                                 # [obj+0] must be a vtable pointer INTO Core.dll
                if len(head)!=4: continue
                vmt=struct.unpack('<I',head)[0]
                if not (coreB and coreB<=vmt<coreB+coreS): continue
                tb=rd(h,obj+0x23f0,8)
                if len(tb)!=8: continue
                t0,t4=struct.unpack('<ii',tb)
                st,sw=int(arr[i]),int(arr[i+1])
                fr=rd(h,obj+0x2660,4); frv=struct.unpack('<i',fr)[0] if len(fr)==4 else 0
                found.append((obj,vmt,t0,t4,st,sw,frv))
        print(f'  scanned; checked {checked} candidates; {len(found)} with a Core.dll vtable at [obj+0]:')
        for obj,vmt,tb,swv,straight,swung,fr in found[:16]:
            print(f'   obj@0x{obj:X} vmt=0x{vmt:X}  timebase[+0x23f0]={tb} swing[+0x23f4]={swv} straight[+0x26a8]={straight} swung[+0x26ac]={swung} free={fr}')
        if not found:
            print('   NONE — the sequencer object is not present with computed intervals: either it is not')
            print('   instantiated until its window is opened, or MIDI Start/Clock does not drive the transport.')
        k32.CloseHandle(h)
    finally:
        vaz.close()

if __name__=='__main__': main()
