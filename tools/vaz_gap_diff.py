"""vaz_gap_diff.py — diff the COMPLETE published-name universe (harvest_vaz_names.py) against
everything the project has actually written down: the status maps, the .ideas docs, the decompiled
.c corpus, and the clone's ParameterIDs. Answers:
  Q1  named controls whose HANDLER was never followed/decompiled
  Q2  handler addresses referenced but never visited (no FUN_<addr> in the decompile corpus)
  Q4  names that appear NOWHERE in the mapping (never mentioned, never investigated)
Analysis only — no code changes.
"""
import json, os, re, glob
from collections import defaultdict

ROOT = r'C:\APC\y'
uni = json.load(open(os.path.join(ROOT, 'tools', 'vaz_name_universe.json')))
methods = {k: int(v, 16) for k, v in uni['methods'].items()}
fields  = {k: int(v, 16) for k, v in uni['fields'].items()}

# ---- the corpus of everything we've written down / decompiled ----
docs, decomp = [], []
for p in glob.glob(os.path.join(ROOT, 'plugins', 'VAZClone', '.ideas', '*.md')) + \
         glob.glob(os.path.join(ROOT, 'tools', '*.md')) + \
         glob.glob(os.path.join(ROOT, 'plugins', 'VAZClone', '*.md')):
    try: docs.append(open(p, encoding='utf-8', errors='ignore').read())
    except Exception: pass
for p in glob.glob(os.path.join(ROOT, 'tools', '*.c')) + glob.glob(os.path.join(ROOT, 'tools', '*.cpp')):
    try: decomp.append(open(p, encoding='utf-8', errors='ignore').read())
    except Exception: pass
src = []
for p in glob.glob(os.path.join(ROOT, 'plugins', 'VAZClone', 'Source', '**', '*.*'), recursive=True):
    if p.lower().endswith(('.h', '.hpp', '.cpp', '.html', '.js')):
        try: src.append(open(p, encoding='utf-8', errors='ignore').read())
        except Exception: pass
DOCS, DECOMP, SRC = '\n'.join(docs), '\n'.join(decomp), '\n'.join(src)
ALL = DOCS + DECOMP + SRC
print(f'corpus: {len(docs)} docs, {len(decomp)} decompiled .c, {len(src)} clone sources')

def decompiled(addr):
    """Has this address been visited in the decompile corpus (FUN_xxxxxx / 0xADDR)?"""
    a = f'{addr:08x}'; a2 = f'{addr:x}'
    return (f'FUN_{a}' in DECOMP) or (f'FUN_{a2}' in DECOMP) or (f'0x{a2}' in ALL) or (a2 in DECOMP)

# ---- Q1/Q2: methods (handlers) ----
never_named, never_visited, known = [], [], []
for n, a in sorted(methods.items()):
    in_docs = re.search(r'\b' + re.escape(n) + r'\b', ALL) is not None
    vis = decompiled(a)
    if not in_docs and not vis: never_named.append((n, a))
    elif not vis:              never_visited.append((n, a))
    else:                      known.append((n, a))
print(f'\n=== Q1/Q2 HANDLERS ({len(methods)} published methods) ===')
print(f'  known (named in our notes AND/OR decompiled): {len(known)}')
print(f'  named somewhere but handler NEVER decompiled : {len(never_visited)}')
print(f'  NEVER named anywhere AND never decompiled    : {len(never_named)}  <<< pure gaps')

# ---- Q4: fields never mentioned ----
SYNTHY = re.compile(r'(osc|lfo|env|filt|cut|res|mix|ring|noise|seq|step|pat|tune|detune|glide|porta|'
                    r'wave|shape|pwm|fm|am|mod|amp|vel|bend|unison|voice|arp|swing|tempo|gate|accent|'
                    r'foot|sync|sample|samp|delay|reverb|chorus|phas|flang|dist|drive|eq|comp|pan|lag)', re.I)
f_never, f_known = [], []
for n, off in sorted(fields.items(), key=lambda kv: kv[0]):
    if re.search(r'\b' + re.escape(n) + r'\b', ALL): f_known.append((n, off))
    else: f_never.append((n, off))
f_never_synth = [(n, o) for n, o in f_never if SYNTHY.search(n)]
print(f'\n=== Q4 FIELDS/CONTROLS ({len(fields)} published fields) ===')
print(f'  mentioned somewhere in our notes/code: {len(f_known)}')
print(f'  NEVER mentioned anywhere             : {len(f_never)}  (of which DSP/synth-ish: {len(f_never_synth)})')

print('\n--- NEVER-MENTIONED, DSP/SYNTH-RELEVANT CONTROLS (the real gap candidates) ---')
byk = defaultdict(list)
for n, o in f_never_synth: byk[n[:2] if n[:2] in ('ms','sb','bt','ed','cb','ud','rb','lb','gb') else 'other'].append((n, o))
for k in sorted(byk):
    if k == 'lb': continue          # labels are cosmetic
    for n, o in sorted(byk[k]):
        print(f'  [{k}] {n:32s} @obj+0x{o:x}')

print('\n--- NEVER-NAMED HANDLERS (published method, zero mentions, never decompiled) ---')
for n, a in never_named[:60]:
    print(f'  {n:34s} -> 0x{a:X}')
print(f'  ({len(never_named)} total)')

print('\n--- named but handler NEVER decompiled ---')
for n, a in never_visited[:40]:
    print(f'  {n:34s} -> 0x{a:X}')
print(f'  ({len(never_visited)} total)')
