"""Replicate VAZ's FUN_004da270 sequential .v2p reader to find the EXACT per-version
byte layout. Each entry = (label, kind). kinds: u32, byte, bool, mod (u32), nop (no read),
fmode (u32 + version<200 remap). MSmp sub-chunks are auto-skipped when hit.
Usage: python v2p_trace.py <file.v2p>
"""
import sys, struct, os

# The read sequence transcribed from FUN_004da270 @ 0x4da270 (Vaz2010Core.dll).
# Setter (struct offset) annotations in comments. Label is my best-guess knob name.
SEQ = [
    ("voices_enable",   "bool"),  # r1  de664 (+0x78)
    ("r2_save",         "u32"),   # r2  -> de5e0 at end (+? )
    ("e1_attack?",      "u32"),   # r3  de804 (+0x80)
    ("e1_mode",         "fmode"), # r4  de8a0 (+0x84)  ver<200: (v==1)?4:0
    ("e1_decay?",       "u32"),   # r5  de930 (+0x8c)
    ("e1_byte90",       "byte"),  # r6  de8f8 (+0x90)
    ("e2_a?",           "u32"),   # r7  dead8 (+0xc4)
    ("e2_byteDC",       "byte"),  # r8  debf8 (+0xdc)
    ("e2_mode",         "bool"),  # r9  deba0 (+0xd0) c?6:1
    ("e2_d?",           "u32"),   # r10 dec30 (+0xd8)
    ("o1_p118",         "u32"),   # r11 dee54(+0x3f) (+0x118)
    ("o1_p11c",         "u32"),   # r12 dee74(+0x94) (+0x11c)
    ("o1_p120",         "u32"),   # r13 deeac (+0x120)
    ("o1_p124",         "u32"),   # r14 deef8(+0x94) (+0x124)
    ("o1_b128",         "byte"),  # r15 def30 (+0x128)
    ("o1_b12c",         "byte"),  # r16 def40 (+0x12c)
    ("o2_p150",         "u32"),   # r17 df054(+0x3f) (+0x150)
    ("o2_p154",         "u32"),   # r18 df074(+0x94) (+0x154)
    ("o2_p158",         "u32"),   # r19 df0ac (+0x158)
    ("o2_p15c",         "u32"),   # r20 df0f8(+0x94) (+0x15c)
    ("o2_b160",         "byte"),  # r21 df130 (+0x160)
    ("o2_b164",         "byte"),  # r22 df140 (+0x164)
    ("pitch_p1a8",      "u32"),   # r23 df300 (+0x1a8)
    ("flag_1ac",        "u32"),   # r24 df3c0 (+0x1ac)
    ("flag_1b0",        "u32"),   # r25 df3d0 (+0x1b0)
    ("modsrc_1b8",      "u32"),   # r26 df440 DAT_0052b0ac (+0x1b8)
    ("modamt_1bc",      "mod"),   # r27 df454 (+0x1bc)
    ("modsrc_1c8",      "u32"),   # r28 df4e8 DAT_0052b0ac (+0x1c8)
    ("modamt_1cc",      "mod"),   # r29 df4fc (+0x1cc)
    ("p_df838",         "u32"),   # r30 df838
    ("modsrc_a8b8",     "u32"),   # r31 df8f8 DAT_0052a8b8 (diff table)
    ("b_df908",         "byte"),  # r32 df908
    ("p_df918",         "u32"),   # r33 df918
    ("modsrc_990",      "u32"),   # r34 df990 DAT_0052b0ac
    ("modamt_7e",       "mod"),   # r35 df9a4
    ("modsrc_9e4",      "u32"),   # r36 df9e4 DAT_0052b0ac
    ("modamt_80",       "mod"),   # r37 df9f8
    ("modsrc_a38",      "u32"),   # r38 dfa38 DAT_0052b0ac
    ("modamt_82",       "mod"),   # r39 dfa4c
    ("__SAMPLE__",      "sample"),# step42 (reads/skips MSmp; ver<106 = name+len)
    ("pan",             "u32"),   # r40 dfd5c/dfe28
    ("pan_mode",        "u32"),   # r41 dfd2c/dfd4c
    ("p_dffb0",         "u32"),   # r42 dffb0
    ("b_e018c",         "byte"),  # r43 e018c
    ("p_e01a4",         "u32"),   # r44 e01a4
    ("p_e01b4",         "u32"),   # r45 e01b4
    ("p_e01c4",         "u32"),   # r46 e01c4 (gated)
    ("modsrc_9f",       "u32"),   # r47 e01e4 DAT_0052b0ac
    ("modamt_9f",       "mod"),   # r48 e01f8
    ("modsrc_a1",       "u32"),   # r49 e0238 DAT
    ("modamt_a1",       "mod"),   # r50 e024c
    ("modsrc_a3",       "u32"),   # r51 e028c DAT
    ("modamt_a3",       "mod"),   # r52 e02a0
    ("modsrc_a5",       "u32"),   # r53 e02e0 DAT
    ("modamt_a5",       "mod"),   # r54 e02f4
    ("modsrc_a7",       "u32"),   # r55 e0334 DAT
    ("modamt_a7",       "mod"),   # r56 e0348
    ("modsrc_a9",       "u32"),   # r57 e0388 DAT
    ("modamt_a9",       "mod"),   # r58 e039c
    ("p_e0430",         "u32"),   # r59 e0430
    ("p_e0530",         "u32"),   # r60 e0530
    ("p_e05f0",         "u32"),   # r61 e05f0
    ("b_e0600",         "bool"),  # r62 e0600
    ("p_e0610",         "u32"),   # r63 e0610
    ("p_e09d8",         "u32"),   # r64 e09d8
    ("b_e09b8",         "bool"),  # r65 e09b8
    # controller block + tail handled specially in code
]


def trace(path, verbose=True):
    d = open(path, 'rb').read()
    prst = d.find(b'PRST')
    psize = struct.unpack_from('<I', d, prst+4)[0]
    ver = struct.unpack_from('<I', d, prst+8)[0]
    pend = prst + 12 + psize           # PRST data end
    pos = prst + 12                    # first param (after name/size/version)
    out = []

    def hit_msmp():
        return d[pos:pos+4] == b'MSmp'

    def skip_msmp():
        nonlocal pos
        sz = struct.unpack_from('<I', d, pos+4)[0]
        out.append(('__MSmp__', pos, 8+sz, 'skip'))
        pos += 8 + sz

    def rd(kind):
        nonlocal pos
        if kind in ('u32', 'mod', 'fmode'):
            v = struct.unpack_from('<i', d, pos)[0]; w = 4
        else:  # byte/bool
            v = d[pos]; w = 1
        o = pos; pos += w
        return o, w, v

    for label, kind in SEQ:
        if kind == 'sample':
            # version>=106: a real MSmp sub-chunk sits here; skip it. (ver<106 unhandled for now)
            if hit_msmp():
                skip_msmp()
            out.append((label, pos, 0, '(sample marker)'))
            continue
        # auto-skip any MSmp we run into
        while hit_msmp():
            skip_msmp()
        o, w, v = rd(kind)
        out.append((label, o, w, v))

    if verbose:
        print('FILE %-26s ver=%d  PRST@0x%x size=%d end=0x%x' % (os.path.basename(path), ver, prst, psize, pend))
        for label, o, w, v in out:
            rel = o - (prst+12)
            print('  %-14s @0x%04x (PS+%-4d) w%d = %s' % (label, o, rel, w, v))
        print('  ... end pos=0x%x  PRST end=0x%x  (%d bytes unread/tail)' % (pos, pend, pend-pos))
    return ver, out


if __name__ == '__main__':
    trace(sys.argv[1])
