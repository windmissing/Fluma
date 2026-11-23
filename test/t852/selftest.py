"""Fixed T832 scripts, fault injections, and historical numerical/pixel reuse."""
import argparse
from copy import deepcopy
import hashlib
import json
import math
from pathlib import Path
import struct
import sys

from check import COLS, FIELDS, LAYERS, Q, check_run, check_suite, need

BASE = Path(__file__).resolve().parents[2]
C = 64
MASS = Q // 8


def fixture(run_id, order, kappa):
    scope = dict(adapter='fixture', device=1, window=1, surface=1, swapchain=2,
                 geometry=3, config=4, optics=5, run=run_id)
    events, columns, totals = [], [[0] * 4 for _ in range(COLS)], [0, 0]
    seq, commit, source = 0, 0, 0

    def input_row(kind, **extra):
        nonlocal seq
        seq += 1
        events.append(dict(kind=kind, seq=seq, ns=seq * 100_000_000, **extra))
        return seq

    def root(kind, **extra):
        stock = [dict(zip(FIELDS, v), column=i, eta=v[0] + v[2]) for i, v in enumerate(columns)]
        row = dict(scope, kind=kind, nonce=commit, generation=commit, commit=commit,
                   sourceCommit=source, slot=commit % 2, columns=stock, admittedTotals=totals.copy(), **extra)
        events.append(row)
        return row

    def frame(accepted):
        identity = {k: v for k, v in accepted.items() if k not in {
            'kind', 'columns', 'admittedTotals', 'serial', 'edges', 'settled', 'ownerCode', 'fenceResult',
            'outcome', 'intent', 'route', 'requested', 'admitted', 'rejected', 'delta', 'fenceNs', 'deadlineNs'}}
        image = dict(identity, presentId=str(commit), imageIndex=commit % 3)
        s, p, w, j = columns[C]
        events.append(dict(image, kind='FRAME', status='CURRENT', leaseAcquired=True, gpuRelease=True,
                           wsiRetired=True, acquire=0, submit=0, fence=0, present=0, imagePresent=0,
                           scene='GRS_GPU_SCENE_Q20_M1_V2', renderer='SHADER', layers=LAYERS.copy(),
                           premultiplied=True, srgbEncodings=1, sand=dict(image, S=s, P=p),
                           water=dict(image, W=w, J=j, eta=s+w),
                           pixel=dict(image, column=C, y=s, depth=w, packetProbe=True, glyph=bool(j))))

    def offer(phase, y, intent):
        hold = input_row('DOWN', geometry=scope['geometry'], x=C*Q, y=y)
        input_row('MOVE', geometry=scope['geometry'], x=C*Q, y=y)
        for i in range(2):
            events.append(dict(kind='OFFER', hold=hold, phase=phase, geometry=scope['geometry'],
                               sample=i+1, ns=seq*100_000_000+15_625_000*i, requested=MASS*i,
                               intent=intent if i else None, x=C*Q, y=y))

    def accepted(phase, route, intent):
        nonlocal commit, source
        columns[C][FIELDS.index(route)] += MASS
        totals[phase == 'WATER'] += MASS
        commit += 1
        source += 1
        return root('SOURCE_RESULT', intent=intent, outcome='ACCEPTED', route=route, requested=MASS,
                    admitted=MASS, delta=MASS, rejected=0,
                    deadlineNs=seq*100_000_000+250_000_000, fenceNs=seq*100_000_000+20_000_000,
                    ownerCode=0, fenceResult=0)

    frame(root('EMPTY'))
    for i, phase in enumerate(order):
        input_row('SELECT', phase=phase)
        offer(phase, 2*Q, i+1)
        row = accepted(phase, 'P' if phase == 'SAND' else 'J', i+1)
        events.append(dict(kind='SOURCE_RESULT', outcome='REPLAY', intent=i+1, admitted=MASS,
                           delta=0, commit=commit, sourceCommit=source))
        input_row('UP')
    frame(row)
    columns[C] = [MASS, 0, MASS, 0]
    commit += 1
    settle = [{'kind': 'LAND', 'column': C, 'q': MASS}, {'kind': 'ARRIVE', 'column': C, 'q': MASS}]
    frame(root('QUANTUM', serial=1, edges=[], settled=settle, ownerCode=0, fenceResult=0))

    edges = []
    if kappa:
        # Fixed numbers for kappa=24, serial=2, horizontal bed at column 64.
        for a, b, q in ((64, 65, 52428), (64, 63, 41943), (65, 66, 10485)):
            columns[a][2] -= q
            columns[b][2] += q
            edges.append({'kind': 'WATER_EDGE', 'from': a, 'to': b, 'q': q})
    commit += 1
    frame(root('QUANTUM', serial=2, edges=edges, settled=[], ownerCode=0, fenceResult=0))

    input_row('SELECT', phase='WATER')
    offer('WATER', MASS, 3)
    frame(accepted('WATER', 'W', 3))
    input_row('UP')
    for intent, outcome in enumerate(('REVOKED', 'LATE', 'BED_REJECT'), 4):
        offer('WATER', 0, intent)
        if outcome == 'REVOKED':
            input_row('SELECT', phase='SAND')
        deadline = seq*100_000_000+250_000_000
        events.append(dict(kind='SOURCE_RESULT', outcome=outcome, intent=intent, admitted=0, delta=0,
                           commit=commit, sourceCommit=source,
                           fenceNs=deadline+1 if outcome == 'LATE' else deadline-1,
                           deadlineNs=deadline, bankAfter=0, bankBefore=0))
        if outcome == 'REVOKED':
            input_row('SELECT', phase='WATER')
        else:
            input_row('UP')
    return dict(scope=scope, events=events, order=list(order), kappa=kappa,
                sourceRateQ20PerSec={'SAND': 8*Q, 'WATER': 8*Q})


def historical_replay(path):
    sys.path.insert(0, str(BASE / 'test/gpu_resident'))
    import check_t279_quantum as oracle

    raw = path.read_bytes()
    cases, water, col, h, n = oracle.fixtures(192, 128)
    need(struct.unpack_from('<4I', raw) == (279, 192, 128, len(cases)), 'REPLAY_HEADER')
    offset = 16

    def take(count):
        nonlocal offset
        words = list(struct.unpack_from(f'<{count}I', raw, offset))
        offset += count * 4
        return words

    for case in cases:
        a = case['a']
        a[h+124] = a[h+125] = 1
        a[h+14] = a[h+16] = a[h+33] = a[h+34]
        a[h+74] = a[h+33]
        for phase in (0, 1):
            initial = sum(case['ledger'][i+6] for i in range(0, 16*a[h+34], 16)
                          if case['ledger'][i+4] == phase)
            oracle.pair(a, h+60+2*phase, initial)
            oracle.pair(a, h+64+2*phase, oracle.u64(a, h+56+2*phase) - initial)
        status, info, bank = take(16), take(3), take(n)
        words = take(info[2])
        events = [words[i:i+64] for i in range(0, len(words), 64)]
        if not case['error']:
            need(oracle.u64(bank, h+16) == oracle.u64(a, h+16)+1, 'REPLAY_COMMIT')
            oracle.pair(bank, h+16, 1)
        oracle.verify(case, status, bank, events, info, 192, 128, water, col, h)
    need(offset == len(raw), 'REPLAY_TRAILING')
    return {'cases': len(cases), 'sha256': hashlib.sha256(raw).hexdigest()}


def historical_pixels(folder):
    receipt = json.loads((BASE / 'test/reports/t281/pixels-192-128.json').read_bytes())
    originals = {(r['name'], r['layer']): r for r in receipt['captures']}

    def image(name, layer):
        data = (folder / f'{name}-192-128-layer{layer}.rgba').read_bytes()
        expected = originals[name, layer]
        need(len(data) == expected['bytes'] and hashlib.sha256(data).hexdigest() == expected['sha256'],
             'T281_CAPTURE_SHA')
        return data

    def pixel(data, x, y):
        at = (y * 256 + x) * 4
        return list(data[at:at+4])

    def srgb(v):
        return 12.92*v if v <= .0031308 else 1.055*v**(1/2.4)-.055

    alpha = 1 - math.exp(-.03*(64-32.5))
    expected = [round(255*srgb(a*(1-alpha)+b*alpha)) for a, b in zip((.006,.008,.014), (.04,.13,.18))] + [255]
    deep, empty = image('deep', 3), image('empty', 2)
    actual = pixel(deep, 129, 32)
    need(max(abs(a-b) for a, b in zip(actual, expected)) <= 2, 'T281_DEEP')
    need(pixel(deep, 127, 32) == pixel(empty, 127, 32), 'T281_DRY_GAP')
    need(all(empty == image('empty', k) for k in (3,4,6)), 'T281_W0')
    surface, glyph = image('arrival', 4), image('arrival', 6)
    need(all(pixel(surface,x,y) == pixel(glyph,x,y) for x in range(132,136) for y in range(55,75)),
         'T281_ARRIVED_GLYPH')
    need(image('flight', 4) != image('flight', 6), 'T281_LIVE_GLYPH')
    return {'checks': 5, 'deep': actual, 'expected': expected, 'toleranceLsb': 2}


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--emit', type=Path)
    parser.add_argument('--replay', type=Path)
    parser.add_argument('--pixels', type=Path)
    args = parser.parse_args()
    runs = [fixture(i, order, kappa) for i, (order, kappa) in enumerate(
        ((('SAND','WATER'),24), (('WATER','SAND'),24), (('SAND','WATER'),0), (('WATER','SAND'),0)), 1)]
    if args.emit:
        args.emit.write_text(json.dumps(runs), encoding='utf-8')
    positive = check_suite(runs)
    controls = [
        ('DOWN', 0, ('x',), 128*Q, 'DOWN'),
        ('OFFER', 2, ('geometry',), 98, 'OFFER_PERMIT'),
        ('SOURCE_RESULT', 1, ('delta',), MASS, 'DOUBLE_BOOK'),
        ('SOURCE_RESULT', 0, ('deadlineNs',), 0, 'SOURCE_ACCEPTANCE'),
        ('SOURCE_RESULT', 0, ('ownerCode',), 1, 'SOURCE_ACCEPTANCE'),
        ('SOURCE_RESULT', 5, ('commit',), 99, 'REJECT_ADVANCE'),
        ('QUANTUM', 1, ('edges',), [], 'HEAD2'),
        ('QUANTUM', 0, ('columns', C, 'W'), 0, 'COLUMN_STOCK'),
        ('QUANTUM', 0, ('admittedTotals', 1), 0, 'CONSERVATION'),
        ('FRAME', 1, ('water','nonce'), 1, 'FRAME_IDENTITY'),
        ('FRAME', 1, ('pixel','swapchain'), 9, 'FRAME_IDENTITY'),
        ('FRAME', 1, ('gpuRelease',), False, 'FRAME_IDENTITY'),
        ('FRAME', 1, ('status',), 'STALE', 'FRAME_IDENTITY'),
        ('FRAME', 1, ('renderer',), 'CANVAS', 'FRAME_IDENTITY'),
        ('FRAME', 2, ('pixel','glyph'), True, 'PIXEL_GLYPH'),
    ]
    rejected = []
    for kind, index, keys, value, reason in controls:
        altered = deepcopy(runs[0])
        node = [r for r in altered['events'] if r['kind'] == kind][index]
        for key in keys[:-1]:
            node = node[key]
        node[keys[-1]] = value
        try:
            check_run(altered)
        except ValueError as error:
            need(str(error) == reason, f'{reason}: {error}')
        else:
            raise AssertionError(reason)
        rejected.append(reason)
    report = {'synthetic': positive, 'negatives': rejected,
              'pcVulkanFullM1': 'PENDING_PRODUCT_EVIDENCE'}
    if args.replay:
        report['historicalReplay'] = historical_replay(args.replay)
    if args.pixels:
        report['historicalPixels'] = historical_pixels(args.pixels)
    print(json.dumps(report, indent=2))


if __name__ == '__main__':
    main()
