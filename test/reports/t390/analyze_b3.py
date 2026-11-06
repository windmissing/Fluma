import json, hashlib, os
from collections import defaultdict

rows = []
for fn in ('B3-part0.ndjson','B3-part1.ndjson','B3-part2.ndjson','B3-part3.ndjson','B3-part4.ndjson','B3-part5.ndjson','B3-part6.ndjson'):
    for line in open(f'D:/hmprojects/HelloWorld/test/reports/t390/{fn}', encoding='utf-8'):
        line = line.strip()
        if not line:
            continue
        obj = json.loads(line)
        if 'rows' in obj:
            rows.extend(obj['rows'])
rows.sort(key=lambda r: int(r['serial']))
eps = sorted(set((r['epoch'], r['run']) for r in rows))
ep, rn = eps[-1]
seg = [r for r in rows if r['epoch'] == ep and r['run'] == rn]
print(f'B3 run: e{ep}/r{rn}')
for r in [r for r in seg if r['stage'] == 42]:
    print(f"  42 carry={r['carry']} delta={int(r['delta'])/1e6:.2f}ms")
cold = [r['stage'] for r in seg if r['stage'] >= 52]
print('  cold chain:', cold)
out = []
cur = None
for r in seg:
    if r['stage'] == 24:
        cur = [r]
    elif cur and r['stage'] == 25:
        cur.append(r)
        out.append(cur)
        cur = None
    elif cur:
        cur.append(r)
for i, b in enumerate(out):
    w = (int(b[-1]['ns']) - int(b[0]['ns'])) / 1e6
    lastop = [x for x in b if x['stage'] == 35]
    tail = (int(b[-1]['ns']) - int(lastop[-1]['ns'])) / 1e6 if lastop else None
    print(f'  batch{i+1}: wall={w:.3f}ms tail={round(tail,3) if tail else None}ms ops={sum(1 for x in b if x["stage"]==34)}')
print('  47/48/50:', len([r for r in seg if r['stage'] in (47,48,50)]))

ev = {
    'revision': 'T390_DEVICE_AB_V1',
    'packageSha256': '7c35ff60a6f4618bcd676b3c98046ac5ccb5284d54d7d69193a7708495623395',
    'arms': {
        'A(T375)': {
            'runs': ['e2/r1','e3/r2','e6/r5','e8/r7'],
            'batch2_tail_ms': [44.315, 46.99, 44.893, 29.154],
            'warmup_ms': [26.38, 33.3, 26.62, 20.96],
            'cold_chain': ['52 only']
        },
        'B(two-batch)': {
            'runs': ['e5/r4','e7/r6'],
            'batch_walls_ms': {'e5/r4':[3.276,13.238,0.681,0.337,1.443,0.123,58.099], 'e7/r6':[0.191,44.359]},
            'cold_chain': ['52(carry=1)','53','54x2(0-10,10-20)','57x2','55 fence 37.1us','56 cleanup 11.8MB']
        }
    },
    'findings': {
        'cold_chain_B': 'COMPLETE device-side: 52(carry=1 B-marker) -> 53 staging flush -> 54 batch segments (0-10,10-20) + 57 per-segment flush -> 55 fence 37.1us -> 56 cleanup 11,843,584 bytes (contract K192/W64=10,795,008 approx match)',
        'stall_migration': 'B strategy: batch2 tail 11.5ms (vs A 44-47ms) with batches 3-6 clean <0.2ms — first-flush stall REDUCED but stall APPEARS at batch7 tail 57.9ms (e5/r4) — migration not elimination; e7/r6 batch2 tail 35.5ms with 1 op (mid-pump abort at GAP)',
        'warmup_42': 'A: 20.9-33.3ms stable; B: 77-90ms (two-batch cold prep makes first warmup larger — honest recording)',
        'accepted_token': 'NOT ACHIEVED all arms — stage47/48/50 zero, statusAfter CLOCK_GAP/PAUSED (same structural GAP revoke as T376/T384: flush tax still exceeds 33.33ms GAP window in surviving stalls)',
        'gap_negative_control': 'GAP revoke observed every run (statusAfter PAUSED_CLOCK) — GAP path exercised',
        'K5': 'timeline exports COMPLETE (226/559/668/766/875/1006 rows, cumulative serial continuity)'
    },
    'evidenceSha256': {}
}
for fn in os.listdir('D:/hmprojects/HelloWorld/test/reports/t390'):
    p = 'D:/hmprojects/HelloWorld/test/reports/t390/' + fn
    if os.path.isfile(p):
        ev['evidenceSha256'][fn] = hashlib.sha256(open(p, 'rb').read()).hexdigest()[:20]
json.dump(ev, open('D:/hmprojects/HelloWorld/test/reports/t390/t390_verdicts.json', 'w'), indent=1, ensure_ascii=False)
print('saved; files:', len(ev['evidenceSha256']))
