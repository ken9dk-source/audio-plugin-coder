"""disasm_noteon.py — disassemble FUN_004db288 @ 0x4db288 (VAZ per-voice note->pitch setup).
This is the function that is CALLED from the note-on (vaz_voice.c:55) but is NOT present in the
decompiled .c files. It decides whether VAZ indexes the pitch table at note*128 (standard A440) or
applies an octave offset. Read-only static disassembly. Flags: note-scaling (*0x80/shl 7), octave
constants (0x600=1536=one octave in 128-units/semitone), and any DAT_005445e0 (inc-table) reference."""
import pefile, struct
import capstone

PATH = r'tools\Vaz2010Core.dll'
pe = pefile.PE(PATH)
ib = pe.OPTIONAL_HEADER.ImageBase
data = open(PATH, 'rb').read()
IMAGE_SCN_MEM_EXECUTE = 0x20000000
text = next(s for s in pe.sections if s.Characteristics & IMAGE_SCN_MEM_EXECUTE)

def va_to_off(va):
    return text.PointerToRawData + (va - ib - text.VirtualAddress)

md = capstone.Cs(capstone.CS_ARCH_X86, capstone.CS_MODE_32)

def disasm_func(va_start, tag, max_bytes=0x400):
    print(f'\n===== {tag}: starting 0x{va_start:X} =====')
    off = va_to_off(va_start)
    code = data[off: off + max_bytes]
    depth = 0
    for ins in md.disasm(code, va_start):
        m, ops = ins.mnemonic, ins.op_str
        flag = ''
        # note scaling: *128 shows up as 'shl ...,7' or 'imul ...,...,0x80' or lea with *4/*8 chains
        if m == 'shl' and ops.endswith(', 7'):
            flag = '   <<< *128 (128 units/semitone?)'
        elif 'imul' in m and ('0x80' in ops):
            flag = '   <<< *0x80'
        elif '0x600' in ops or '1536' in ops:
            flag = '   <<< 0x600 = ONE OCTAVE (1536 = 12*128)'
        elif '0xc00' in ops:
            flag = '   <<< 0xc00 = TWO OCTAVES'
        elif '5445e0' in ops:
            flag = '   <<< DAT_005445e0 INC TABLE'
        elif m == 'call':
            flag = '   <<< CALL'
        elif m in ('ret', 'retn'):
            flag = '   <<< RET'
        print(f'  0x{ins.address:X}: {m:8s} {ops}{flag}')
        if m in ('ret', 'retn'):
            depth += 1
            if depth >= 1:
                break

disasm_func(0x4db288, 'FUN_004db288 note->pitch setup')
