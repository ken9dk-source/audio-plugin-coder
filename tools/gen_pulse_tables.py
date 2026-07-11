"""gen_pulse_tables.py — emit VAZPulseTables.h from the raw-dumped BLEP step tables
(pulse_step_006dd2c0 = rising, pulse_step_006de2c0 = falling). RAW int32, as the recurrence uses them."""
import os
HERE = os.path.dirname(__file__)
TAB  = os.path.join(HERE, 'vaz_tables')
OUT  = os.path.join(HERE, '..', 'plugins', 'VAZClone', 'Source', 'VAZPulseTables.h')

def load(name):
    with open(os.path.join(TAB, name + '.txt')) as f:
        return [int(x) for x in f.read().split()]

rise = load('pulse_step_006dd2c0')   # DAT_006dd2c0
fall = load('pulse_step_006de2c0')   # DAT_006de2c0
n = min(len(rise), len(fall))

def emit(arr, name):
    lines = [f'static constexpr int32_t {name}[{n}] = {{']
    for i in range(0, n, 8):
        lines.append('    ' + ', '.join(str(v) for v in arr[i:i+8]) + ',')
    lines.append('};')
    return '\n'.join(lines)

hdr = f'''// VAZPulseTables.h — RAW int32 BLEP step tables for VAZ's band-limited pulse (vaz_big.c:206-241).
// Runtime-dumped from Vaz2010Core.dll via tools/dump_vaz_tables.py (RPM) -> tools/gen_pulse_tables.py.
//   kStepRise = DAT_006dd2c0 (= trunc(-2^30/(iv8+1)));  kStepFall = DAT_006de2c0 (non-trivial).
// Both indexed by the freq-dependent BLEP transition-width index iv8. Do not edit by hand — regenerate.
#pragma once
#include <cstdint>

namespace VAZPulseT {{
static constexpr int kN = {n};
{emit(rise, 'kStepRise')}
{emit(fall, 'kStepFall')}
}} // namespace VAZPulseT
'''
with open(OUT, 'w', encoding='utf-8') as f:
    f.write(hdr)
print(f'wrote {OUT}  ({n} entries each)')
print(f'  kStepRise[0..4] = {rise[:5]}')
print(f'  kStepFall[0..4] = {fall[:5]}')
