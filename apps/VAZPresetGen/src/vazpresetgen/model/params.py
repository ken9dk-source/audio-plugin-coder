"""Single source of truth for the synth's parameter vocabulary: oscillator waves, voice modes,
modulation-source indices and tuning helpers. All values are the native .v2p integer encodings
that the engine writes (see engine/v2p.py)."""
from __future__ import annotations

NEUTRAL_TUNE = -2400                         # parseV2P o1tune/o2tune base -> setTune(tune+2400)=0 (unison)

WAVE = {"saw": 0, "pulse": 1, "multisaw": 2, "sample": 3, "ext": 4}
WAVE_NAME = {0: "Saw", 1: "Pulse", 2: "MultiSaw", 3: "Sample", 4: "Ext/Sync"}
VMODE = {"mono": 0, "poly": 1, "unison": 2}
VMODE_NAME = {0: "Mono", 1: "Poly", 2: "Unison"}

# Modulation-source indices (clone choice order), verified implemented in SynthVoice.h
SRC = {"none": 0, "lfo1": 1, "lfo2": 2, "lfo3": 3, "env1": 4, "env2": 5,
       "keytrack": 10, "velocity": 17}

# Native unit denominators (for display / normalisation only — the engine writes raw ints)
UNIT_255 = ("cutoff", "reso", "overdrive", "o1level", "o2level", "noise",
            "uniDetune", "polyDetune", "portamento", "e1s", "e2s", "o1shape", "o2shape")
UNIT_425 = ("e1a", "e1d", "e1r", "e2a", "e2d", "e2r")


def cents(semitones: int = 0, octaves: int = 0, fine: int = 0) -> int:
    return NEUTRAL_TUNE + octaves * 1200 + semitones * 100 + fine
