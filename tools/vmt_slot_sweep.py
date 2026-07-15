"""vmt_slot_sweep.py — the VMT-slot sweep of the DSP classes (not the GUI forms).

The earlier passes enumerated PUBLISHED methods (GUI handlers). VMT slots are a DIFFERENT gap class:
virtual methods of the DSP classes, which carry no published name. Using tools/vaz_classes.json
(566 classes with per-class vmt slots), classify GUI vs DSP by walking the parent chain, then report
which DSP-class slots have never been decompiled (no FUN_<addr> in the decompile corpus).
"""
import json, os, re, glob
from collections import defaultdict

ROOT = r'C:\APC\y'
classes = json.load(open(os.path.join(ROOT, 'tools', 'vaz_classes.json')))

# corpus of everything decompiled / written down
corpus = []
for p in glob.glob(os.path.join(ROOT, 'tools', '*.c')) + glob.glob(os.path.join(ROOT, 'tools', '*.cpp')) + \
         glob.glob(os.path.join(ROOT, 'tools', '*.md')) + glob.glob(os.path.join(ROOT, 'plugins', 'VAZClone', '.ideas', '*.md')):
    try: corpus.append(open(p, encoding='utf-8', errors='ignore').read())
    except Exception: pass
CORP = '\n'.join(corpus)

def seen(addr):
    a = int(addr, 16)
    return (f'FUN_{a:08x}' in CORP) or (f'FUN_{a:x}' in CORP) or (f'0x{a:x}' in CORP) or (f'{a:x}' in CORP.lower())

# --- classify: GUI (VCL) vs DSP/other, by parent chain ---
GUI_ROOTS = {'TForm', 'TCustomForm', 'TWinControl', 'TControl', 'TGraphicControl', 'TComponent',
             'TCustomControl', 'TPanel', 'TCustomPanel', 'TButton', 'TLabel', 'TMenuItem', 'TPopupMenu',
             'TCollection', 'TCollectionItem', 'TPersistent', 'TStrings', 'TCanvas', 'TGraphic'}
def chain(n, depth=0):
    out = [n]
    while depth < 30:
        p = classes.get(out[-1], {}).get('parent')
        if not p or p in out: break
        out.append(p); depth += 1
    return out

gui, dsp = [], []
for n, c in classes.items():
    ch = chain(n)
    (gui if any(x in GUI_ROOTS for x in ch[1:]) or n in GUI_ROOTS else dsp).append(n)

print(f'classes: {len(classes)}  |  GUI/VCL lineage: {len(gui)}  |  non-GUI (DSP/core/other): {len(dsp)}')

# --- DSP candidates: non-GUI classes that actually HAVE vmt slots ---
cand = [(n, classes[n]) for n in dsp if classes[n]['slots']]
cand.sort(key=lambda kv: -len(kv[1]['slots']))
print(f'\n=== non-GUI classes WITH vmt slots: {len(cand)} ===')
print(f'{"class":34s} {"slots":>5} {"undecompiled":>12}  parent')
tot_slots = tot_new = 0
rows = []
for n, c in cand:
    slots = c['slots']
    unseen = [s for s in slots if not seen(s)]
    tot_slots += len(slots); tot_new += len(unseen)
    rows.append((n, c, slots, unseen))
    print(f'{n:34s} {len(slots):5d} {len(unseen):12d}  {c["parent"]}')
print(f'\nTOTAL: {tot_slots} DSP-class vmt slots, {tot_new} never decompiled')

print('\n=== the biggest untouched DSP classes (slot addresses to visit) ===')
for n, c, slots, unseen in rows[:6]:
    if not unseen: continue
    print(f'\n-- {n}  (vmt {c["vmt"]}, instSize {hex(c["instSize"])}, {len(unseen)}/{len(slots)} slots unvisited) --')
    print('   ' + ' '.join(unseen[:16]) + (' …' if len(unseen) > 16 else ''))
