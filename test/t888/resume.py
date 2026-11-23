"""Independent T892 original evidence and T891 owner coverage checks."""
import argparse
from collections import Counter, defaultdict
from copy import deepcopy
import json
from pathlib import Path

from check import ART, OLD, exactly, load, long_window, need, off, off_pair, rows_at

COLD = load('t848_reader', 'test/t848/check.py')
REVOKED = {'REVOKED_REANCHOR', 'REVOKED_CLOSED', 'REVOKED_OVERFLOW', 'REVOKED_FAULT'}


def settling(rows):
    pointer = rows_at(rows, 'WIN32_POINTER')
    down, up = ([r for r in pointer if r['kind'] == kind] for kind in ('DOWN', 'UP'))
    need(len(down) == len(up) == 1, 'POINTER_DOWN_UP_COUNT')
    down, up = down[0], up[0]
    d, u = int(down['qpc']), int(up['qpc'])
    need(0 < d < u and 0 < int(down['seq']) < int(up['seq'])
         and down['geometry'] == up['geometry'] and down['hold'] != '0' and up['hold'] == '0', 'POINTER_EDGE')
    frames = [r for r in rows if r.get('state') == 'M1_QUANTUM_PRESENT_QUEUED']
    before, after = [], []
    last_exit = d
    for row in frames:
        frame, lease = row['windowFrame'], row['m1SceneLease']
        enter, exit = (int(frame[k]) for k in ('presentEnterQpc', 'presentExitQpc'))
        need(row['geometry'] == up['geometry'] and last_exit < enter <= exit
             and (exit < u or enter > u), 'POINTER_FRAME_ORDER')
        last_exit = exit
        sample = (lease['activeSand'], int(lease['sandStockQ']), exit, frame['acceptedRoot'])
        (before if exit < u else after).append(sample)
    need(before and after and before[-1][0] > 0 and after[0][0] > 0, 'UP_LIVE_NOT_OBSERVED')
    count = next((i for i, sample in enumerate(after) if sample[0] == 0), len(after))
    need(0 < count < len(after) and all(sample[0] == 0 for sample in after[count:]), 'SETTLING_NOT_PERSISTENT')
    need(len({sample[3] for sample in before + after}) == len(frames)
         and before[-1][1] > 0 and all(sample[1] == before[-1][1] for sample in after), 'SETTLING_STOCK')
    hz = int(frames[0]['qpcFrequency'])
    return dict(upQpc=u, priorRoot=before[-1][3], priorActive=before[-1][0],
                priorSampleToUpMs=(u-before[-1][2])*1000/hz, firstPostUpActive=after[0][0],
                postUpLiveRoots=count, firstZeroRoot=after[count][3],
                firstZeroMs=(after[count][2]-u)*1000/hz, postUpZeroRoots=len(after)-count,
                stockQ20=before[-1][1])


def cold_pages(rows, result, q, s):
    reserved = exactly(rows, 'M1_HOT_CAPTURE_RESERVED')
    pages = rows_at(rows, 'M1_COLD_PAGE')
    total = result['decodedBytes'] - COLD.HEADER.size
    root = result['root']
    at = 0
    for page in pages:
        size = min(65536, total-at)
        need(size > 0 and (page['offset'], page['count'], page['total']) == (at,size,total)
             and page['request'] == reserved['request'] == str(root['run'])
             and page['root'] == str(root['acceptedRoot']) and page['sha'] == result['payloadSHA256'],
             'COLD_PAGE_IDENTITY')
        at += size
    need(at == total, 'COLD_PAGE_COVERAGE')
    capture = exactly(rows, 'M1_QUANTUM_COLD_CAPTURE')
    need(tuple(capture[k] for k in COLD.SCOPE) == tuple(result['scope'][k] for k in COLD.SCOPE),
         'COLD_CAPTURE_SCOPE')
    terminal, = [r for r in rows if r.get('state') == 'M1_QUANTUM_PRESENT_RETIRED']
    need(terminal['m1PublicationReleased'] is True and terminal['m1WsiRetireVkResult'] == 0
         and terminal['windowFrame']['acceptedRoot'] == str(root['acceptedRoot']), 'COLD_RETIRE')
    pair = COLD.pair
    need(pair(q,77) == pair(q,71) == pair(s,77) == pair(s,71) == pair(q,69)
         and pair(s,69) == 0 and pair(s,77) == s[61], 'COLD_STOCK')
    return dict(payloadBytes=at, pageBytes=[r['count'] for r in pages], root=root, lineage=result['lineage'],
                qBefore=pair(q,69), qAfter=pair(q,77), sBefore=pair(s,69), sAfter=pair(s,77),
                rootEvents=[q[53],s[53]], bedEventCount=[q[95],s[95]],
                landState='MISSING_EVIDENCE' if q[95] == s[95] == 0 else 'NEEDS_EVENT_DECODE',
                roiState='MISSING_EVIDENCE')


def owner_coverage(e):
    """Check normalized T894 raw rows and per-session drain receipts."""
    key = lambda row: (row['run'], row['geometry'], row['session'], row['seq'])
    scope = tuple(e[k] for k in ('run', 'geometry'))
    producer, rates = {}, defaultdict(Counter)
    actions = set()
    for row in e['pulses']:
        k = key(row)
        need(k[:2] == scope and row['action'] not in actions and 1 <= row['action'] <= 52, 'OWNER_PRODUCER_KEY')
        actions.add(row['action'])
        if row['state'] == 'DELIVERED':
            need(min(k[2:]) > 0 and k not in producer and 0 < row['callback'] <= row['receipt'], 'OWNER_PRODUCER_KEY')
            producer[k] = row
        else:
            need(k[2:] == (0,0) and row.get('reason'), 'UNQUEUED_OWNER_SEQUENCE')
    need(actions == {s['actionId'] for s in OLD.D1.plan() if s['kind'] == 'PULSE'}, 'OWNER_PULSE_ACTIONS')
    seen = set()
    for row in e['terminals']:
        k = key(row)
        need(k in producer and k not in seen, 'OWNER_TERMINAL_KEY')
        sent = producer[k]
        need(all(row[f] == sent[f] for f in ('action','callback','receipt'))
             and sent['receipt'] <= row['terminalQpc'] <= e['drainQpc'], 'OWNER_TERMINAL_TIME')
        category = row['category']
        need(category in ('accepted','ownerRejected','ownerRevoked')
             and (category == 'ownerRevoked') == (row['reason'] in REVOKED)
             and (category == 'accepted') == (row['reason'] == 'PAGE_PULSE_ACCEPTED')
             and row['reason'], 'OWNER_TERMINAL_REASON')
        seen.add(k)
        rates[k[2]][category] += 1
    need(seen == producer.keys(), 'OWNER_PULSE_COVERAGE')
    expected = {}
    for session, count in rates.items():
        seqs = [k[3] for k in producer if k[2] == session]
        expected[session] = dict(session=session, firstSeq=min(seqs), lastSeq=max(seqs),
                                 queued=len(seqs), **{k:count[k] for k in ('accepted','ownerRejected','ownerRevoked')})
    need(len(e['sessions']) == len(expected) and sorted(e['sessions'], key=lambda row:row['session'])
         == sorted(expected.values(), key=lambda row:row['session']), 'OWNER_DRAIN_COVERAGE')
    return dict(state='OWNER_SCOPE_CLOSED', sessions=[expected[k] for k in sorted(expected)])


def selftest(bed, cold_rows, cold, q, s):
    actions = [s['actionId'] for s in OLD.D1.plan() if s['kind'] == 'PULSE']
    pulses = [dict(run=1,geometry=2,session=i//24+1,seq=i%24+1,action=action,
                   state='DELIVERED',callback=2*i+1,receipt=2*i+2) for i,action in enumerate(actions)]
    terminals = [dict(row,terminalQpc=100,category='accepted',reason='PAGE_PULSE_ACCEPTED') for row in pulses]
    terminals[23].update(category='ownerRevoked',reason='REVOKED_REANCHOR')
    sessions = [dict(session=i+1,firstSeq=1,lastSeq=24,queued=24,accepted=23+i,
                     ownerRejected=0,ownerRevoked=1-i) for i in range(2)]
    fixture = dict(run=1,geometry=2,pulses=pulses,terminals=terminals,sessions=sessions,drainQpc=110)
    positive = owner_coverage(fixture)
    negative = []
    for mutate, code in (
        (lambda e:e['terminals'].pop(), 'OWNER_PULSE_COVERAGE'),
        (lambda e:e['terminals'].append(e['terminals'][0]), 'OWNER_TERMINAL_KEY'),
        (lambda e:e['terminals'][0].update(geometry=3), 'OWNER_TERMINAL_KEY'),
        (lambda e:e['terminals'][0].update(receipt=0), 'OWNER_TERMINAL_TIME'),
        (lambda e:e['terminals'][23].update(category='accepted'), 'OWNER_TERMINAL_REASON'),
        (lambda e:e['sessions'][0].update(queued=23), 'OWNER_DRAIN_COVERAGE'),
    ):
        changed = deepcopy(fixture)
        mutate(changed)
        reject(lambda:owner_coverage(changed), code, negative)
    wrong_bed = deepcopy(bed)
    next(row for row in wrong_bed if 'm1SceneLease' in row)['m1SceneLease']['activeSand'] = 0
    reject(lambda:settling(wrong_bed), 'UP_LIVE_NOT_OBSERVED', negative)
    wrong_cold = deepcopy(cold_rows)
    rows_at(wrong_cold,'M1_COLD_PAGE')[1]['offset'] = 0
    reject(lambda:cold_pages(wrong_cold,cold,q,s), 'COLD_PAGE_IDENTITY', negative)
    return dict(positive=positive, negatives=negative)


def reject(check, code, negative):
    try:
        check()
    except ValueError as error:
        need(str(error) == code, f'NEGATIVE_EXPECTED_{code}: {error}')
    else:
        raise AssertionError(code)
    negative.append(code)


def original():
    base = ART/'t892'
    path = base/'bed-dual.jsonl'
    rows, _ = OLD.read_rows(path)
    window = long_window(path)
    settling_result = settling(rows)
    need(window['cleanRetire'] and not window['gaps'] and not window['sourceStartsAtOrAfterUp']
         and not window['sourceFencesAtOrAfterUp'] and window['acceptedSourceRoots'] == 1
         and window['admittedQ20'] == settling_result['stockQ20'], 'STOP_SOURCE_OR_RETIRE')
    cold_rows, _ = OLD.read_rows(base/'cold-busy.jsonl')
    archive = COLD.read_archive(base/'m1pcold1.bin')
    cold = COLD.join(archive,cold_rows)
    q,s = archive[2:]
    arms = [off(ART/'t887'/f'b-off{i}.jsonl') for i in (1,2)]
    clock = off_pair(*arms)
    need(clock['state'] == 'OWNER_PULSE_COVERAGE', 'HISTORICAL_OFF_STATE')
    a,b = (arm['actions'][0]['relativeNs'] for arm in arms)
    need(a != b, 'HISTORICAL_CLOCK_DIFF')
    return dict(window=window,settling=settling_result,cold=cold_pages(cold_rows,cold,q,s),
                historicalOFF=dict(clock,firstActionFromReadyNs=[a,b]),
                tests=selftest(rows,cold_rows,cold,q,s),
                pending=dict(actualOFF='T894',sameRootROI='T895'))


if __name__ == '__main__':
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--owner', type=Path)
    args = parser.parse_args()
    print(json.dumps(owner_coverage(json.loads(args.owner.read_bytes())) if args.owner else original(), indent=2))
