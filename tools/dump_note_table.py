"""dump_note_table.py — DEFINITIVE octave check. VAZ note-on (FUN_004db288 @0x4db2f0) maps the MIDI
note to a pitch index via DAT_006dd0c0[note], then DAT_005445e0[index] -> phase increment.
This dumps DAT_006dd0c0[0..127] from the DLL file and converts each MIDI note to Hz using the
increment-table curve dump, so we can see whether MIDI 69 == 440Hz (standard) or 220Hz (octave low)."""
import pefile, struct, os
PATH = r'tools\Vaz2010Core.dll'
pe = pefile.PE(PATH); ib = pe.OPTIONAL_HEADER.ImageBase
data = open(PATH, 'rb').read()

def va_to_off(va):
    for s in pe.sections:
        start = ib + s.VirtualAddress
        if start <= va < start + s.SizeOfRawData:
            return s.PointerToRawData + (va - start), s.Name.rstrip(b'\x00').decode('latin1')
    return None, None

NOTE_TBL = 0x6dd0c0
off, sec = va_to_off(NOTE_TBL)
print(f'DAT_006dd0c0 in section {sec} at file offset 0x{off:X}' if off else 'NOTE TABLE NOT IN FILE (bss?)')

# load the increment-table curve dump (normalized v/2^31): Hz(idx) = curve[idx] * sr / 2
HERE = os.path.dirname(__file__)
curve = None
cp = os.path.join(HERE, 'vaz_tables', 'curve_0x5445e0.txt')
if os.path.exists(cp):
    curve = [float(x) for x in open(cp).read().split()]
    print(f'loaded {len(curve)} increment-curve entries')

def idx_of(note):
    o, _ = va_to_off(NOTE_TBL + note*4)
    return struct.unpack('<i', data[o:o+4])[0]

def hz(idx, sr=44100):
    if curve and 0 <= idx < len(curve):
        return curve[idx] * sr / 2.0
    return float('nan')

NAMES = {0:'C',1:'C#',2:'D',3:'D#',4:'E',5:'F',6:'F#',7:'G',8:'G#',9:'A',10:'A#',11:'B'}
def std_hz(n):  # standard A440: MIDI 69 = 440
    return 440.0 * 2**((n-69)/12.0)

print('\nMIDI note -> VAZ pitch index (DAT_006dd0c0) -> VAZ Hz   vs standard A440 Hz   ratio')
for n in [0, 12, 24, 36, 48, 57, 60, 69, 72, 81, 84, 96, 108, 120, 127]:
    idx = idx_of(n)
    vhz = hz(idx)
    shz = std_hz(n)
    name = f'{NAMES[n%12]}{n//12 - 1}'
    ratio = vhz/shz if shz else 0
    flag = ''
    if 0.48 < ratio < 0.52: flag = '  <<< VAZ is ONE OCTAVE LOW'
    elif 0.98 < ratio < 1.02: flag = '  <<< matches standard'
    elif 1.9 < ratio < 2.1: flag = '  <<< VAZ is ONE OCTAVE HIGH'
    print(f'  MIDI {n:3d} ({name:>3s}): idx=0x{idx & 0xffffffff:04X}={idx:5d}  VAZ={vhz:8.2f}Hz  std={shz:8.2f}Hz  ratio={ratio:.3f}{flag}')

# also show the raw step between consecutive octaves in the note table
print('\nnote-table octave steps (should be 12*128=1536 if 128 units/semitone):')
for n in [48, 60, 69]:
    print(f'  idx[{n+12}]-idx[{n}] = {idx_of(n+12)-idx_of(n)}  (idx[{n}]={idx_of(n)}, idx[{n+12}]={idx_of(n+12)})')
