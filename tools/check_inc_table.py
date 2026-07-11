"""check_inc_table.py — is VAZ's pitch->increment table (DAT_005445e0) equal to hz/sr*2^32?
If yes, the clone's incU matches and iv8 (band-limiting) is identical. Uses the RPM dump (raw scale)."""
import os, struct
HERE = os.path.dirname(__file__)

# Prefer a RAW re-read from the running-process dump if present; else the normalized curve dump.
norm_path = os.path.join(HERE, 'vaz_tables', 'curve_0x5445e0.txt')
vals = None
if os.path.exists(norm_path):
    with open(norm_path) as f:
        norm = [float(x) for x in f.read().split()]
    # dump stored v/2^31  ->  raw inc = norm * 2^31
    vals = [int(round(v * 2**31)) & 0xffffffff for v in norm]
    print(f'loaded {len(vals)} entries from curve_0x5445e0.txt (normalized /2^31)')
else:
    print('curve dump not found'); raise SystemExit

# If inc = f/sr * 2^32, then f = inc/2^32 * sr. Show the implied Hz across the used pitch range 0x80..0x3b80.
def hz(idx, sr):
    return (vals[idx] / 2**32) * sr

print('\npitch idx -> implied Hz (inc = value/2^32 * sr):')
for idx in [0x80, 0x400, 0x800, 0x1000, 0x1800, 0x2000, 0x2800, 0x3000, 0x3800, 0x3b80]:
    if idx < len(vals):
        print(f'  0x{idx:04X}: raw=0x{vals[idx]:08X}  Hz@44100={hz(idx,44100):9.2f}  Hz@48000={hz(idx,48000):9.2f}')

# Is it a clean 12-TET exponential? An octave (2x Hz) should be a constant index step.
# Find index step per octave by locating where Hz doubles from a reference.
def find_double(base_idx, sr):
    base = hz(base_idx, sr)
    for i in range(base_idx+1, len(vals)):
        if hz(i, sr) >= 2*base:
            return i - base_idx
    return None
for bi in [0x1000, 0x2000, 0x2800]:
    step = find_double(bi, 44100)
    print(f'  octave step from pitch 0x{bi:04X}: {step} pitch-units  (=> {step/12:.1f} units/semitone)' if step else f'  0x{bi:04X}: no double found')

# Where does A440 land, and is the table monotonic there?
tgt = 440.0
best = min(range(0x80, min(0x3b80, len(vals))), key=lambda i: abs(hz(i,44100)-tgt))
print(f'\nA440@44100 lands near pitch 0x{best:04X} (Hz={hz(best,44100):.2f}); neighbours: '
      + ', '.join(f'{hz(best+d,44100):.2f}' for d in (-2,-1,0,1,2)))
