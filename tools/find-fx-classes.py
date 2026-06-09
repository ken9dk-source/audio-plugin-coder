# Find Borland RTTI class-name strings (Pascal: len byte + ASCII) for the FX modules,
# and the vtable method pointers immediately preceding each (Borland layout: vtable .. classname).
import struct, pefile
PATH = r'C:\Program Files (x86)\Steinberg\Vstplugins\VAZ Synths\VAZ 2010\Vaz2010Core.dll'
pe = pefile.PE(PATH); base = pe.OPTIONAL_HEADER.ImageBase
raw = open(PATH, 'rb').read()
secs = [(base + s.VirtualAddress, s.PointerToRawData, s.SizeOfRawData) for s in pe.sections]
def va_of(fileoff):
    for sva, rp, rs in secs:
        if rp <= fileoff < rp + rs: return sva + (fileoff - rp)
    return None
def rd(va, n):
    rva = va - base
    for s in pe.sections:
        if s.VirtualAddress <= rva < s.VirtualAddress + s.SizeOfRawData:
            o = s.PointerToRawData + (rva - s.VirtualAddress); return raw[o:o+n]
    return b''

names = [b'Flanger', b'Chorus', b'Phaser', b'Equalizer', b'Delay', b'Reverb', b'Distort', b'Comp']
for nm in names:
    off = 0
    while True:
        j = raw.find(nm, off); off = j + 1
        if j < 0: break
        va = va_of(j)
        if va is None: continue
        # Borland Pascal string: the byte before the 'T' of "TFX<name>" is the length.
        # look back for a plausible "TFX" prefix
        pre = raw[max(0,j-6):j]
        s = pre + nm
        # find printable class name ending here
        txt = ''
        k = j - 1
        while k >= 0 and 32 <= raw[k] < 127 and len(txt) < 24:
            txt = chr(raw[k]) + txt; k -= 1
        full = txt + nm.decode()
        if 'T' in full[:4]:
            print(f'  0x{va:X}: "{full}"  (class-name string)')
