"""dump_engine_vmt.py — locate the LIVE TBaseMidSynth engine object in a running VAZ by heap-scanning for its
vtable pointer, then walk to its voice array. Unblocks the mod-LFO waveform shapes (#8), Osc3 footage (#9),
and the .v2p-loader ground truth (Phase 4) — all shared this one blocker (status-map §3.0).

Recipe: engine is new'd in the host EXE; [engine+0] == Core.dll VMT @ VA 0x4d4614 (rebased coreBase+0xd4614).
Its voice array is at engine+0x2534 (32 pointers); voice+0x28 = the LFO object. Scan MEM_PRIVATE RW heap for the
rebased VMT value; validate a hit by checking engine+0x2534 holds plausible heap pointers."""
import sys, os, ctypes, struct
from ctypes import wintypes, Structure, c_char, sizeof, byref, c_void_p, c_byte, c_size_t, c_ulonglong, c_ulong, POINTER
import pefile
sys.path.insert(0, os.path.join(os.path.dirname(__file__), 'vaz_auto'))
from vaz_auto import VazAuto

CORE = r'C:\Program Files (x86)\Steinberg\Vstplugins\VAZ Synths\VAZ 2010\Vaz2010Core.dll'
VMT_RVA = 0xd4614          # TBaseMidSynth vtable RVA (VA 0x4d4614 - ImageBase 0x400000)
VOICE_ARRAY_OFF = 0x2534   # engine+0x2534 = 32-entry voice pointer array
k32 = ctypes.windll.kernel32

class ME32(Structure):
    _fields_ = [('dwSize',wintypes.DWORD),('th32ModuleID',wintypes.DWORD),('th32ProcessID',wintypes.DWORD),
                ('GlblcntUsage',wintypes.DWORD),('ProccntUsage',wintypes.DWORD),('modBaseAddr',POINTER(c_byte)),
                ('modBaseSize',wintypes.DWORD),('hModule',wintypes.HMODULE),('szModule',c_char*256),('szExePath',c_char*260)]
class MBI(Structure):      # 64-bit caller layout (Python x64 querying WOW64 target)
    _fields_ = [('BaseAddress',c_ulonglong),('AllocationBase',c_ulonglong),('AllocationProtect',c_ulong),
                ('__a1',c_ulong),('RegionSize',c_ulonglong),('State',c_ulong),('Protect',c_ulong),
                ('Type',c_ulong),('__a2',c_ulong)]

def core_base(pid):
    snap = k32.CreateToolhelp32Snapshot(0x18, pid); me = ME32(); me.dwSize = sizeof(me)
    if k32.Module32First(snap, byref(me)):
        while True:
            if b'Vaz2010Core' in me.szModule: return ctypes.cast(me.modBaseAddr, c_void_p).value
            if not k32.Module32Next(snap, byref(me)): break
    return None

def readmem(h, addr, n):
    buf = (c_byte*n)(); got = c_size_t(0)
    ok = k32.ReadProcessMemory(h, c_void_p(addr), buf, n, byref(got))
    return bytes(bytearray(buf))[:got.value] if ok else b''

def main():
    pref = pefile.PE(CORE).OPTIONAL_HEADER.ImageBase
    vaz = VazAuto().launch(wait=5.0)
    try:
        pid = vaz.app.process
        base = core_base(pid); vmt = base + VMT_RVA
        h = k32.OpenProcess(0x410, False, pid)   # VM_READ | QUERY_INFORMATION
        print(f'Core.dll @0x{base:X}  (pref 0x{pref:X})   target VMT value = 0x{vmt:X}')
        # 0. VERIFY the vtable: its slots should be code pointers into Core.dll's .text
        vmtbytes = readmem(h, vmt, 0x30)
        slots = struct.unpack('<12I', vmtbytes) if len(vmtbytes) >= 48 else ()
        incode = sum(1 for s in slots if base <= s < base + 0x200000)
        print(f'  VMT slots @0x{vmt:X}: {incode}/12 point into Core.dll .text  ->', 'LOOKS LIKE A VTABLE' if incode >= 8 else 'NOT a vtable (RVA wrong?)')
        print('   first 4 slots:', ' '.join(f'0x{s:X}' for s in slots[:4]))
        import numpy as np
        imgsz = pefile.PE(CORE).OPTIONAL_HEADER.SizeOfImage
        vt_lo, vt_hi = base, base + imgsz            # a vtable ptr points somewhere into Core.dll's image

        # 1. VOICE-ARRAY PATTERN scan (robust to derived VMTs): find >=32 consecutive plausible heap pointers;
        #    the engine object sits 0x2534 before, and its [0] must be a vtable into Core.dll.
        engines = []; addr = 0x10000; scanned = 0; RUN = 24
        while addr < 0xFFFF0000:
            mbi = MBI()
            if k32.VirtualQueryEx(h, c_void_p(addr), byref(mbi), sizeof(mbi)) == 0: break
            size = mbi.RegionSize
            if mbi.State == 0x1000 and mbi.Type == 0x20000 and (mbi.Protect & 0x04):   # committed private RW = heap
                data = readmem(h, mbi.BaseAddress, min(size, 96*1024*1024)); scanned += len(data)
                if len(data) >= (RUN+1)*4:
                    arr = np.frombuffer(data[:len(data)//4*4], dtype='<u4')
                    plaus = (arr > 0x100000) & (arr < 0xFFFF0000) & ((arr & 3) == 0)
                    # sliding count of consecutive plausible pointers
                    csum = np.concatenate(([0], np.cumsum(plaus.astype(np.int32))))
                    for j in range(0, len(arr) - RUN):
                        if csum[j+RUN] - csum[j] == RUN:          # RUN consecutive plausible ptrs @ word j
                            objva = mbi.BaseAddress + j*4 - VOICE_ARRAY_OFF
                            if objva < mbi.BaseAddress: continue
                            vp = readmem(h, objva, 4)
                            if len(vp) == 4:
                                vtp = struct.unpack('<I', vp)[0]
                                if vt_lo <= vtp < vt_hi:           # [obj] is a Core.dll vtable ptr → real engine
                                    engines.append((objva, vtp, struct.unpack('<32I', data[j*4:j*4+128])))
            addr = mbi.BaseAddress + size
        # dedup by object
        seen=set(); engines=[e for e in engines if not (e[0] in seen or seen.add(e[0]))]
        # STRONG filter: the first few voice ptrs must point to objects sharing ONE vtable into Core.dll
        def vt_of(p):
            b = readmem(h, p, 4); return struct.unpack('<I', b)[0] if len(b)==4 else 0
        real = []
        for objva, vtp, ptrs in engines:
            vts = [vt_of(ptrs[k]) for k in range(4)]
            if vts[0] and vt_lo <= vts[0] < vt_hi and all(v == vts[0] for v in vts):
                real.append((objva, vtp, ptrs, vts[0]))
        print(f'scanned {scanned/1e6:.0f} MB heap; {len(engines)} pattern hits -> {len(real)} with a consistent voice-class vtable')
        for objva, vtp, ptrs, vvt in real[:6]:
            print(f'  engine @0x{objva:X}  vtable RVA 0x{vtp-base:X}{"  ==TBaseMidSynth" if vtp==vmt else ""}  voiceClass RVA 0x{vvt-base:X}  voice[0]=0x{ptrs[0]:X}')
        engines = [(o, v, p) for (o, v, p, _) in real]
        for objva, vtp, ptrs in engines[:6][:0]:  # (old print skipped)
            print(f'  engine @0x{objva:X}  vtable=0x{vtp:X} (RVA 0x{vtp-base:X}{"  == TBaseMidSynth" if vtp==vmt else ""})  voice[0]=0x{ptrs[0]:X}')
        # 2. walk voice[0]+0x28 (the LFO object) for the best candidate(s)
        for objva, vtp, ptrs in engines[:3]:
            v0 = ptrs[0]
            lfo = readmem(h, v0 + 0x28, 4)
            if len(lfo) == 4:
                lfoObj = struct.unpack('<I', lfo)[0]
                tag = '' if 0x100000 < lfoObj < 0xFFFF0000 else '  (not a ptr)'
                print(f'  engine 0x{objva:X}: voice[0]=0x{v0:X}  voice[0]+0x28 (LFO obj) = 0x{lfoObj:X}{tag}')
                if 0x100000 < lfoObj < 0xFFFF0000:
                    f = readmem(h, lfoObj, 0x30)
                    if len(f) >= 0x30:
                        print(f'     LFO obj [0..0x30): ' + ' '.join(f'{struct.unpack("<i",f[k:k+4])[0]}' for k in range(0,0x30,4)))
        if not engines: print('  none — try holding a note (send MIDI) so voices are allocated, or widen RUN/offset.')
        k32.CloseHandle(h)
    finally:
        vaz.close()

if __name__ == '__main__':
    main()
