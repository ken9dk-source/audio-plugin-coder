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
        imgsz=pefile.PE(CORE).OPTIONAL_HEADER.SizeOfImage; vt_lo,vt_hi=base,base+imgsz
        h=k32.OpenProcess(0x410,False,pid)
        print(f'Core.dll @0x{base:X}   TBaseMidSynth VMT=0x{vmt:X}')
        slots=struct.unpack('<12I', rd(h,vmt,48)); incode=sum(1 for s in slots if base<=s<base+0x200000)
        print(f'  VMT verify: {incode}/12 slots into .text')
        # start a loopback audio capture to CONFIRM the voice actually sounds (clone_diag.py pattern)
        cap={}
        try:
            import soundcard as sc, threading
            loops=[m for m in sc.all_microphones(include_loopback=True) if m.isloopback]
            def _rec(m):
                try:
                    with m.recorder(samplerate=48000,channels=2) as r: cap[m.name]=r.record(int(1.2*48000))
                except Exception as ex: cap[m.name]=('ERR',str(ex))
            th=[threading.Thread(target=_rec,args=(m,)) for m in loops[:2]]
            for t in th: t.start()
        except Exception as e:
            print('  (no loopback capture:',e,')'); th=[]
        # hold a note via vaz_auto's SHARED MidiOut (the exact route the working render harnesses use)
        try:
            from vaz_auto import shared_midiout
            mo=shared_midiout('loop'); mo.send_message([0x90,48+vaz.note_transpose,110]); time.sleep(0.4)
            print(f'  holding note {48+vaz.note_transpose} via shared MidiOut...')
        except Exception as e:
            print('  no MIDI hold (',e,')'); mo=None
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
        # release note + CONFIRM the voice actually sounded (loopback peak)
        if mo: mo.send_message([0x80,48+vaz.note_transpose,0])
        for t in th: t.join()
        for nm,bfr in cap.items():
            if isinstance(bfr,tuple): print(f'  audio {nm[:30]}: {bfr[1]}')
            else:
                pk=20*np.log10(max(float(np.max(np.abs(bfr))),1e-9))
                print(f'  audio {nm[:30]}: peak {pk:.1f} dB  {"<-- SOUNDING" if pk>-40 else "(SILENT -> note not reaching VAZ audio engine)"}')
        # cluster the MOVING words into contiguous objects -> the active-voice DSP state is the densest cluster
        def region_of(a):
            for b,s in regions:
                if b<=a<b+s: return b
            return None
        ch=sorted(changed); clusters=[]
        if ch:
            st=pv=ch[0]; cnt=1
            for a in ch[1:]:
                if a-pv>0x100: clusters.append((st,pv,cnt)); st=a; cnt=0
                pv=a; cnt+=1
            clusters.append((st,pv,cnt))
        clusters.sort(key=lambda c:-c[2])
        print(f'  {len(clusters)} moving-memory clusters; densest (= active voice/engine DSP state):')
        for st,en,cnt in clusters[:8]:
            print(f'   0x{st:X}..0x{en:X}  span 0x{en-st:X}  {cnt} words')
        if clusters:
            st,en,_=clusters[0]; ob=st&~0xF
            print(f'  densest @0x{ob:X}: changing words (offset: A->B, delta) — phase acc = big const delta, LFO = small:')
            shown=0
            for a in ch:
                if st<=a<=en and shown<16:
                    reg=region_of(a)
                    if reg is None: continue
                    va=struct.unpack('<i',snapA[reg][a-reg:a-reg+4])[0]; vb=struct.unpack('<i',snapB[reg][a-reg:a-reg+4])[0]
                    print(f'     +0x{a-ob:X}: {va} -> {vb}  d={vb-va}'); shown+=1
        k32.CloseHandle(h)
    finally:
        vaz.close()

if __name__=='__main__': main()
