"""VAZ 2010 .v2p encoder / parser.

A faithful, offset-validated port of the VAZClone's parseV2P (PluginProcessor.cpp) — the .v2p
PRST block is a SEQUENTIAL, VERSION-GATED stream (not fixed offsets). We run the parser over a
real v2.0 template, recording the byte offset of every field, then write generated values at
exactly those offsets. The synth therefore reads back precisely what we write. The offset map
was validated field-by-field against the clone's own buildV2P writer (lfo1rate=PS+10, e1a=PS+54,
filterMode=sec3+28, cutoff=sec3+33, reso=sec3+37, overdrive=sec3+105 ... all match).
"""
from __future__ import annotations
import struct


def find_tag(d, tag, frm: int = 0) -> int:
    if isinstance(tag, str):
        tag = tag.encode()
    return d.find(tag, max(0, frm))


def landmarks(d):
    prst = find_tag(d, "PRST")
    ms1 = find_tag(d, "MSmp")
    ms2 = find_tag(d, "MSmp", ms1 + 4) if ms1 >= 0 else -1
    return prst, ms1, ms2


class _Cursor:
    def __init__(self, d, pos):
        self.d = d; self.n = len(d); self.pos = pos
        self.off = {}   # field -> byte offset
        self.val = {}   # field -> parsed value

    def u32(self, name=None):
        if name: self.off[name] = self.pos
        p = self.pos
        v = int.from_bytes(self.d[p:p + 4], "little") if p + 4 <= self.n else 0
        self.pos += 4
        if name: self.val[name] = v
        return v

    def byte(self, name=None):
        if name: self.off[name] = self.pos
        v = self.d[self.pos] if 0 <= self.pos < self.n else 0
        self.pos += 1
        if name: self.val[name] = v
        return v

    def modsrc(self, ver, name=None):
        if name: self.off[name] = self.pos
        p = self.pos
        v = int.from_bytes(self.d[p:p + 4], "little") if p + 4 <= self.n else 0
        self.pos += 4
        if ver < 200 and v > 6: v += 1
        if name: self.val[name] = v
        return v

    def strsample(self):
        self.byte(); self.byte(); ln = self.u32(); self.pos += ln

    def skipMsmp(self):
        p = self.pos
        if p + 8 <= self.n and self.d[p:p + 4] == b"MSmp":
            self.pos += 8 + int.from_bytes(self.d[p + 4:p + 8], "little")


def trace(d, prst):
    """Run the version-gated parse; return (version, offset_map, value_map)."""
    ver = int.from_bytes(d[prst + 8:prst + 12], "little")
    v = ver
    c = _Cursor(d, prst + 12)
    if v >= 0x67: c.byte("mono")
    if v >= 0x6d: c.u32()
    if v >= 0xc9: c.byte()
    if v >= 0xc9: c.u32()
    c.u32("lfo1rate"); c.u32("lfo1wave"); c.u32("lfo1shape"); c.byte("lfo1trig")
    if v >= 0xc9: c.byte()
    if v >= 0xc9: c.u32()
    c.u32("lfo2rate")
    if v >= 200: c.modsrc(v); c.u32()
    c.byte("lfo2trig")
    if v >= 200: c.u32("lfo2mode")
    else: c.byte()
    c.u32("lfo2delay"); c.u32("lfo3sel"); c.byte("lfo3wav")
    if v < 0x6b:
        c.u32("e1a"); c.u32("e1d"); c.u32("e1s"); c.u32("e1r"); c.byte(); c.byte()
    else:
        c.u32("e1a"); c.u32("e1d"); c.u32("e1s"); c.u32("e1r"); c.byte()
    c.byte("e1mode")
    if v >= 0x6b: c.byte()
    if v >= 0xca: c.byte()
    if v < 0x6c:
        c.u32("e2a"); c.u32("e2d"); c.u32("e2s"); c.u32("e2r"); c.byte(); c.byte()
    else:
        c.u32("e2a"); c.u32("e2d"); c.u32("e2s"); c.u32("e2r"); c.byte()
    c.byte()
    if v >= 0x6c: c.byte("e2mode")
    if v >= 0xca: c.byte()
    if v >= 200:
        c.modsrc(v, "e2modsrc"); c.u32("e2modamt"); c.u32("e2moddest")
    c.modsrc(v, "ma1in")
    if v >= 200: c.byte("ma1sq")
    c.modsrc(v, "ma1amsrc"); c.u32("ma1amamt")
    if v >= 200: c.modsrc(v, "ma2in")
    if v >= 200: c.modsrc(v, "ma2amsrc")
    c.u32("o1tune"); c.u32("o1wave"); c.u32("o1shape")
    if v >= 200: c.byte()
    c.modsrc(v, "o1fm1s"); c.u32("o1fm1d")
    c.modsrc(v, "o1fm2s"); c.u32("o1fm2d")
    c.modsrc(v, "o1pwms"); c.u32("o1pwmd")
    if v < 0x69: c.strsample()
    else: c.skipMsmp(); c.byte()
    c.u32("o2tune"); c.u32("o2wave"); c.byte("o1sync"); c.u32("o2shape")
    c.modsrc(v, "o2fm1s"); c.u32("o2fm1d")
    c.modsrc(v, "o2fm2s"); c.u32("o2fm2d")
    c.modsrc(v, "o2pwms"); c.u32("o2pwmd")
    if v < 0x6a: c.strsample()
    else: c.skipMsmp(); c.byte()
    if v >= 200: c.u32("mix1src")
    c.u32("o1level"); c.byte("mix1post")
    if v >= 200: c.u32("mix2src")
    c.u32("o2level"); c.byte("mix2post")
    c.u32("mix3src"); c.u32("noise"); c.byte("mix3post")
    c.u32("filterMode"); c.byte(); c.u32("cutoff"); c.u32("reso"); c.u32("bandwidth")
    if v >= 200: c.u32("hpCut")
    c.modsrc(v, "fcut1s"); c.u32("fcut1d")
    c.modsrc(v, "fcut2s"); c.u32("fcut2d")
    c.modsrc(v, "fcut3s"); c.u32("fcut3d")
    c.modsrc(v, "fresS"); c.u32("fresD")
    c.modsrc(v, "am1s"); c.u32("am1d")
    c.modsrc(v, "am2s"); c.u32("am2d")
    if v >= 200:
        c.modsrc(v, "am3s"); c.u32("am3d")
    c.u32("overdrive")
    if v >= 0x65: c.modsrc(v)
    if v >= 0x65: c.u32()
    c.u32("voiceMode"); c.u32(); c.byte()
    c.u32("bendRange")
    if v >= 200: c.u32("uniVoices")
    c.u32("uniDetune")
    if v >= 200: c.u32("polyDetune")
    c.u32("portamento")
    return ver, c.off, c.val


def encode(template: bytes, params: dict, name: str | None = None) -> bytes:
    """Write `params` (raw VAZ field -> int) into a copy of `template` at the field offsets the
    synth reads them from. Optionally set the internal patch name (within the STR chunk size)."""
    d = bytearray(template)
    prst, ms1, ms2 = landmarks(d)
    if prst < 0 or ms2 < 0:
        raise ValueError("template is not a valid v2.0 .v2p (missing PRST/MSmp landmarks)")
    _, off, _ = trace(d, prst)
    for field, value in params.items():
        o = off.get(field)
        if o is None:
            continue
        struct.pack_into("<I", d, o, int(value) & 0xFFFFFFFF)
    if name:
        s = d.find(b"STR ")
        if s >= 0:
            ln = struct.unpack_from("<I", d, s + 4)[0]
            nb = name.encode("latin-1", "replace")[:ln]
            d[s + 8:s + 8 + ln] = nb + b"\x00" * (ln - len(nb))
    return bytes(d)


def parse(data: bytes) -> dict:
    """Parse a .v2p into its value map (for QC re-validation / preset evolution)."""
    prst = find_tag(data, "PRST")
    if prst < 0:
        raise ValueError("not a .v2p (no PRST)")
    _, _, val = trace(data, prst)
    return val
