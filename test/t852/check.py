"""Check normalized T832 dry/wet evidence and the same-publication frame join."""
import argparse
from collections import Counter
import json
from pathlib import Path

Q = 1 << 20
CAP = 256 * Q
COLS = 128
SCOPE = ('adapter', 'device', 'window', 'surface', 'swapchain', 'geometry', 'config', 'optics', 'run')
ROOT = ('nonce', 'commit', 'sourceCommit', 'slot', 'generation')
LAYERS = ['background', 'S', 'P', 'W', 'eta', 'J', 'foreground']
FIELDS = ('S', 'P', 'W', 'J')


def need(ok, reason):
    if not ok:
        raise ValueError(reason)


def token(row, fields):
    return tuple(row[k] for k in fields)


def snapshot(rows):
    want = tuple(range(COLS))
    need(tuple(r['column'] for r in rows) == want, 'COLUMN_LAYOUT')
    result = [list(token(r, FIELDS)) for r in rows]
    for row, stock in zip(rows, result):
        need(all(type(q) is int and q >= 0 for q in stock) and sum(stock) <= CAP
             and row['eta'] == stock[0] + stock[2], 'COLUMN_STOCK')
    return result


def transfers(stock, kappa, serial):
    """Horizontal specialization of the T279/T280 integer head2 oracle."""
    result = []
    for color in (serial & 1, 1 - (serial & 1)):
        for edge in range(color, COLS - 1, 2):
            a, b = edge, edge + 1
            if stock[a][0] + stock[a][2] < stock[b][0] + stock[b][2]:
                a, b = b, a
            sa, _, wa, _ = stock[a]
            sb, _, wb, _ = stock[b]
            head = 2 * (sa + wa)
            delta = head - 2 * (sb + wb)
            q = min(kappa * delta // 240, delta // 4, wa,
                    max(0, head - 2 * max(sa, sb)) // 2, CAP - sum(stock[b]))
            if q:
                stock[a][2] -= q
                stock[b][2] += q
                result.append({'kind': 'WATER_EDGE', 'from': a, 'to': b, 'q': q})
    return result


def check_run(run):
    scope = token(run['scope'], SCOPE)
    kappa = run['kappa']
    need(type(kappa) is int and 0 <= kappa <= 960, 'KAPPA_DOMAIN')
    selected, hold, anchor = 'SAND', None, None
    seq, latest, input_ns = 0, -1, -1
    intents, pending, roots = {}, {}, {}
    stock = [[0, 0, 0, 0] for _ in range(COLS)]
    admitted, commit, source_commit, generation, quantum, nonce = [0, 0], 0, 0, 0, 0, 0
    counts, flow, phases = Counter(), Counter(), []
    dropped_after_stop = set()
    coexist = False

    for row in run['events']:
        kind = row['kind']
        counts[kind] += 1
        if kind in ('SELECT', 'DOWN', 'MOVE', 'UP', 'CANCEL', 'FOCUS_LOST', 'CAPTURE_LOST', 'OUTSIDE'):
            need(row['seq'] > seq and row['ns'] >= input_ns, 'INPUT_SEQUENCE')
            seq, input_ns = row['seq'], row['ns']
            if kind == 'DOWN':
                need(hold is None and row['geometry'] == run['scope']['geometry']
                     and 0 <= row['x'] < COLS * Q and 0 <= row['y'] <= CAP, 'DOWN')
                hold, anchor, latest = seq, None, -1
            elif kind == 'MOVE':
                need(hold is not None and row['geometry'] == run['scope']['geometry']
                     and 0 <= row['x'] < COLS * Q and 0 <= row['y'] <= CAP, 'MOVE')
            else:
                hold, anchor = None, None
                if kind == 'SELECT':
                    need(row['phase'] in ('SAND', 'WATER'), 'PHASE')
                    selected = row['phase']
            continue
        if kind == 'OFFER':
            need(hold is not None and row['hold'] == hold and row['phase'] == selected
                 and row['geometry'] == run['scope']['geometry'], 'OFFER_PERMIT')
            now = row['ns']
            need(now >= input_ns and row['sample'] > latest, 'SAMPLE_ORDER')
            latest = row['sample']
            dt = 0 if anchor is None else now - anchor
            anchor = now
            if dt < 0 or dt > 33_333_333:
                hold, anchor = None, None
                need(row['requested'] == 0 and row['intent'] is None, 'GAP_MINTS_OFFER')
                continue
            rate = run['sourceRateQ20PerSec'][selected]
            need(row['requested'] == rate * dt // 1_000_000_000
                 and 0 <= row['x'] < COLS * Q and 0 <= row['y'] <= CAP, 'OFFER_QUANTIZATION')
            intent = row['intent']
            if row['requested']:
                need(intent is not None and intent not in pending and intent not in intents, 'DUPLICATE_OFFER')
                pending[intent] = row
            else:
                need(intent is None, 'ZERO_OFFER')
            continue
        if kind == 'SOURCE_RESULT':
            outcome, intent = row['outcome'], row['intent']
            if outcome == 'REPLAY':
                need(intent in intents and row['admitted'] == intents[intent] and row['delta'] == 0
                     and row['commit'] == commit and row['sourceCommit'] == source_commit, 'DOUBLE_BOOK')
                continue
            need(intent in pending, 'UNKNOWN_OFFER')
            offer = pending.pop(intent)
            col = offer['x'] // Q
            s, _, w, _ = stock[col]
            current = offer['hold'] == hold
            timely = offer['ns'] <= row['fenceNs'] <= row['deadlineNs']
            phase, y = offer['phase'], offer['y']
            route = 'P' if phase == 'SAND' else 'BED_REJECT' if y < s else 'W' if y <= s + w else 'J'
            if outcome != 'ACCEPTED':
                reasons = {'REVOKED': not current, 'LATE': not timely, 'BED_REJECT': route == 'BED_REJECT'}
                need(reasons.get(outcome, False) and row['admitted'] == row['delta'] == 0
                     and row['commit'] == commit and row['sourceCommit'] == source_commit
                     and row['bankAfter'] == row['bankBefore'], 'REJECT_ADVANCE')
                counts[outcome] += 1
                continue
            need(current and timely and row['ownerCode'] == row['fenceResult'] == 0
                 and route in ('P', 'W', 'J') and row['route'] == route
                 and 0 < row['admitted'] == row['delta'] <= offer['requested'], 'SOURCE_ACCEPTANCE')
            need(row['requested'] == offer['requested']
                 and row['admitted'] + row['rejected'] == row['requested'], 'SOURCE_LEDGER')
            q = row['admitted']
            stock[col][FIELDS.index(route)] += q
            admitted[phase == 'WATER'] += q
            source_commit += 1
            intents[intent] = q
            phases.append(phase)
            counts[route] += 1
        elif kind == 'QUANTUM':
            quantum += 1
            need(row['serial'] == quantum and row['ownerCode'] == row['fenceResult'] == 0, 'QUANTUM_ACCEPTANCE')
            edges = transfers(stock, kappa, quantum)
            need(edges == row['edges'], 'HEAD2')
            for edge in edges:
                flow['left' if edge['to'] < edge['from'] else 'right'] += edge['q']
            for event in row['settled']:
                term, col, q = token(event, ('kind', 'column', 'q'))
                need(term in ('LAND', 'ARRIVE') and 0 <= col < COLS and q > 0, 'SETTLE')
                a, b = (1, 0) if term == 'LAND' else (3, 2)
                need(stock[col][a] >= q, 'SETTLE_STOCK')
                stock[col][a] -= q
                stock[col][b] += q
                counts[term] += 1
                if hold is None:
                    dropped_after_stop.add(term)
        elif kind == 'FRAME':
            key = token(row, SCOPE + ROOT)
            need(row['status'] == 'CURRENT' and key in roots and key == next(reversed(roots))
                 and row['leaseAcquired'] is True and row['gpuRelease'] is True and row['wsiRetired'] is True
                 and all(row[k] == 0 for k in ('acquire', 'submit', 'fence', 'present', 'imagePresent'))
                 and row['scene'] == 'GRS_GPU_SCENE_Q20_M1_V2'
                 and row['renderer'] == 'SHADER' and row['layers'] == LAYERS
                 and row['premultiplied'] is True and row['srgbEncodings'] == 1
                 and all(token(row[name], SCOPE + ROOT + ('imageIndex', 'presentId')) ==
                         token(row, SCOPE + ROOT + ('imageIndex', 'presentId')) for name in ('sand', 'water', 'pixel')),
                 'FRAME_IDENTITY')
            current = roots[key]
            col = row['pixel']['column']
            need(0 <= col < COLS, 'PIXEL_COLUMN')
            s, p, w, j = current[col]
            need(row['sand']['S'] == s and row['sand']['P'] == p
                 and row['water']['W'] == w and row['water']['J'] == j
                 and row['water']['eta'] == s + w, 'FRAME_STOCK')
            y = row['pixel']['y']
            need(0 <= y <= CAP and row['pixel']['depth'] == min(w, max(0, s+w-max(y, s))), 'PIXEL_DEPTH')
            if row['pixel'].get('packetProbe') is True:
                need(row['pixel']['glyph'] == bool(j), 'PIXEL_GLYPH')
            coexist |= s + p > 0 and w + j > 0
            continue
        elif kind != 'EMPTY':
            raise ValueError('EVENT_KIND')
        else:
            need(not roots and row['nonce'] == row['commit'] == row['sourceCommit'] == 0, 'EMPTY_ROOT')

        if kind != 'EMPTY':
            need(row['nonce'] > nonce and row['generation'] == generation + 1
                 and row['commit'] == commit + 1, 'ACCEPT_ORDER')
            nonce, commit, generation = row['nonce'], row['commit'], row['generation']
        need(token(row, SCOPE) == scope and row['sourceCommit'] == source_commit
             and row['slot'] in (0, 1), 'ROOT_IDENTITY')
        observed = snapshot(row['columns'])
        need(stock == observed and row['admittedTotals'] == admitted
             and [sum(s + p for s, p, w, j in observed), sum(w + j for s, p, w, j in observed)] ==
                 admitted, 'CONSERVATION')
        roots[token(row, SCOPE + ROOT)] = observed

    need(counts['EMPTY'] == 1 and counts['FRAME'] >= 3 and coexist and not pending
         and phases[:2] == run['order'] and counts['P'] and counts['J'] and counts['W']
         and dropped_after_stop == {'LAND', 'ARRIVE'}, 'COVERAGE')
    if kappa:
        need(flow['left'] > 0 and flow['right'] > 0, 'BIDIRECTIONAL')
    else:
        need(not flow, 'ZERO_KAPPA')
    return {'kappa': kappa, 'phases': phases, 'admittedQ20': admitted, 'edgesQ20': dict(flow),
            'counts': dict(counts), 'stoppedSettles': sorted(dropped_after_stop)}


def check_suite(runs):
    need(len(runs) == 4 and len({r['scope']['run'] for r in runs}) == 4, 'INDEPENDENT_RUNS')
    orders = {('SAND', 'WATER'), ('WATER', 'SAND')}
    need({tuple(r['order']) for r in runs if r['kappa'] > 0} == orders
         and {tuple(r['order']) for r in runs if r['kappa'] == 0} == orders, 'CONTROL_MATRIX')
    return [check_run(run) for run in runs]


if __name__ == '__main__':
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('evidence', type=Path)
    args = parser.parse_args()
    try:
        runs = json.loads(args.evidence.read_bytes())
        result = check_suite(runs)
    except (KeyError, TypeError, ValueError, OSError) as error:
        parser.exit(1, f'{error}\n')
    print(json.dumps(result, indent=2))
