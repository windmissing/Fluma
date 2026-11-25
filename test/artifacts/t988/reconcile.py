"""Reconcile archived root timings and qualify the narrow OP21 quotient math offline."""

import hashlib
import json
import random
import struct
from pathlib import Path


TOP = Path(__file__).resolve().parents[3]
REPORT = TOP/'test/reports'
MASK32 = (1 << 32) - 1


def sha(data):
    return hashlib.sha256(data).hexdigest()


def extract(path):
    return [json.loads(line) for line in path.read_bytes().splitlines() if line.startswith(b'{')]


def only(rows, **fields):
    row, = (item for item in rows if all(item.get(key) == value for key, value in fields.items()))
    return row


def elapsed(record, start, stop):
    assert start['available'] and stop['available'] and start['vkResult'] == stop['vkResult'] == 0
    delta = (int(stop['tick']) - int(start['tick'])) % (1 << record['validBits'])
    return delta*record['periodNs']/10**6


def g9_run(path):
    rows = extract(path)
    gpu = only(rows, stage='ROOT_GPU_TIMESTAMPS', kind='QUANTUM')
    root = only(rows, stage='ROOT', kind='QUANTUM')
    clock = only(rows, stage='QUANTUM_TIMELINE')
    assert all(root[key] == gpu[key] for key in ('run', 'nonce', 'plan', 'deadlineNs', 'fence', 'fenceGeneration'))
    assert root['fenceGeneration'] == gpu['queryResetSerial']
    assert gpu['fenceVkResult'] == 0 and root['vkResult'] == 2 and not root['accepted'] and not root['tailRead']
    assert gpu['state'] == 'LATE_RETIRED_DIAGNOSTIC' and gpu['acceptedRoot'] is None
    assert clock['stop'] == 'GRANT_DEADLINE' and int(root['deadlineNs']) - int(clock['grantNs']) == 250_000_000
    by_index = {point['i']: point for point in gpu['points']}
    spans = {entry['name']: elapsed(gpu, by_index[i], by_index[i+1]) for i, entry in enumerate(gpu['spans'][:14])}
    details = []
    for i in range(15, len(by_index), 2):
        a, b = by_index[i], by_index[i+1]
        assert a['edge'] == 'INPUT_READY' and b['edge'] == 'DISPATCH_DONE'
        assert all(a[key] == b[key] for key in ('slot', 'opcode', 'role'))
        details.append(dict(slot=a['slot'], opcode=a['opcode'], scope=a['role'], ms=elapsed(gpu, a, b)))
    whole = elapsed(gpu, by_index[0], by_index[14])
    assert abs(sum(spans.values())-whole) < 1e-6
    face = sum(item['ms'] for item in details if item['opcode'] in (25, 27))
    return dict(nonce=root['nonce'], fence=root['fence'], firstTick=by_index[0]['tick'], lastTick=by_index[14]['tick'],
                ms=whole, faceMs=face, residualAfterFaceMs=whole-face, spansMs=spans, detail=details,
                acceptedRoot=None)


def g9():
    order = ('A1','B1','B2','A2','B3','A3','A4','B4')
    parent = REPORT/'t987/runs/g9-paired-r2'
    trials = {name: g9_run(parent/(name+'.jsonl')) for name in order}
    historical = {name: g9_run(REPORT/'t968/runs/goal9'/(name+'.jsonl'))
                  for name in ('B-ops-r1','B-audit-r3')}
    return dict(trials=trials, historical=historical,
                pairedDelta=[{field: sum(trials[name][field]*(1 if name.startswith('B') else -1)
                                        for name in order[start:start+4])/2 for field in ('ms','faceMs')}
                             for start in (0, 4)])


def narrow(numerator, denominator):
    hi, lo = numerator >> 32, numerator & MASK32
    qhi, rem = divmod(hi, denominator)
    qlo = 0
    for bit in range(31, -1, -1):
        carry = rem >> 31
        stepped = ((rem << 1) | ((lo >> bit) & 1)) & MASK32
        if carry or stepped >= denominator:
            stepped = (stepped - denominator) & MASK32
            qlo |= 1 << bit
        rem = stepped
        assert 0 <= rem < denominator
    return (qhi << 32) | qlo, rem


def quotient():
    rng = random.Random(988)
    anchors = {1, 2, 3, (1 << 31)-1, 1 << 31, (1 << 31)+1, MASK32}
    anchors.update(120*j*65536 for j in (1,2,4,8,16,32))
    pairs = [(n,d) for d in sorted(anchors) for n in (0,1,d-1,d,d+1,MASK32,1<<32,(1<<64)-1)]
    pairs += [(rng.getrandbits(64),rng.randrange(1,1 << 32)) for _ in range(16384)]
    fallback = 0
    for _ in range(4096):
        omega, deficit = rng.getrandbits(32), rng.getrandbits(28)
        denominator = 120*rng.choice((1,2,4,8,16,32))*65536+omega
        if denominator > MASK32:
            fallback += 1
        else:
            pairs.append((omega*deficit, denominator))
    data = bytearray()
    for n,d in pairs:
        q,r = narrow(n,d)
        assert (q,r) == divmod(n,d) and q*d+r == n
        data.extend(struct.pack('<QQQQ',n,d,q,r))
    return dict(exactCases=len(pairs), outOfScopeWideDenominators=fallback, vectorSHA=sha(data))


def preview():
    result = {}
    parent = REPORT/'t981/runs'
    edges = {'gpuRoot':('ROOT_BEGIN','BUSINESS_COPIES_DONE'), 'actual':('ACTUAL_BEGIN','ACTUAL_END'),
             'replay':('REPLAY_BEGIN','REPLAY_END'), 'seal':('SEAL_BEGIN','SEAL_END'),
             'sha':('SEAL_PREPARE_END','SEAL_SHA_END'), 'actualJ1Pressure0':
             ('ACTUAL_J1_MICRO0_PRESSURE_CYCLE0_BEGIN','ACTUAL_J1_MICRO0_PRESSURE_CYCLE0_END'),
             'replayJ1Pressure0':('REPLAY_J1_MICRO0_PRESSURE_CYCLE0_BEGIN','REPLAY_J1_MICRO0_PRESSURE_CYCLE0_END')}
    for arm in ('circulation','projection','zero'):
        folder = parent/('g13-16-'+arm)
        rows = extract(folder/'preview.jsonl')
        gpu = only(rows, stage='T877_GPU_TIMESTAMPS', state='LATE_RETIRED_DIAGNOSTIC')
        done = only(rows, state='T978_COARSE_PREVIEW_ROOT_RETIRED')
        show = only(rows, stage='T978_COARSE_PREVIEW_PRESENT')
        coll = only([r for r in rows if 'copyBytes' in r], stage='T877_COLD_COPY')
        assert gpu['fence'] == show['rootFence'] and gpu['fenceGeneration'] == show['rootFenceGeneration']
        assert int(show['coldFenceGeneration']) == int(show['rootFenceGeneration'])+1
        assert done['g13CoarsePreview']['coldFence'] == coll['fence']
        assert gpu['nonce'] == show['nonce'] == coll['nonce']
        assert gpu['fenceVkResult'] == coll['fenceVkResult'] == show['graphicsFenceResult'] == show['present'] == 0
        assert gpu['acceptedRoot'] is show['acceptedRoot'] is None
        points = {row['point']: row for row in gpu['points']}
        times = {stage: elapsed(gpu, points[a],points[b]) for stage,(a,b) in edges.items()}
        times['actualJ1PressureBlocks'] = [elapsed(gpu, points[a], points[b]) for a,b in
                                           zip(['J1_OPERATOR_BEGIN']+[f'J1_OPERATOR_BLOCK_{i}_END' for i in range(15)],
                                               [f'J1_OPERATOR_BLOCK_{i}_END' for i in range(16)], strict=True)]
        archive = done['t833QuantumArchive']
        cold = (folder/'preview.quantum').read_bytes()
        assert sha(cold) == archive['sha256']
        assert archive['segments'][:3] == ['BASE','CANDIDATE','REPLAY']
        start = archive['fieldOffset']+archive['segmentsBytes'][0]
        state = cold[start:start+archive['segmentsBytes'][1]]
        assert sha(state) == done['g13CoarsePreview']['stateSHA']
        layout = archive['layoutWords']
        assert layout[:12] == [16,16,256,544,34080,13392,4160,3,9,528,800,1312]
        faces = struct.unpack_from('<544f',state,256*4)
        p = struct.unpack_from('<256q',state,800*4)
        qpc = int(done['qpcFrequency'])
        def duration(start_qpc, stop_qpc):
            return (int(stop_qpc)-int(start_qpc))*1000/qpc
        times['hostRootSubmitToFence'] = duration(done['t833QuantumSubmitQpc'],done['t833QuantumFenceQpc'])
        times['hostRootRetiredToPresent'] = duration(done['t833QuantumFenceQpc'],show['presentExitQpc'])
        times['coldHostSubmitToFence'] = duration(coll['submitEnterQpc'],coll['waitExitQpc'])
        times['graphicsHostSubmitToFence'] = duration(show['graphicsSubmitEnterQpc'],show['graphicsFenceExitQpc'])
        times['presentHostCall'] = duration(show['presentEnterQpc'],show['presentExitQpc'])
        result[arm] = dict(timeMs=times, plans={k:done['t833QuantumPlan'][k]
                                              for k in ('logical','dispatches','computeBarriers','copies')},
                           rootFence=gpu['fence'], coldFence=coll['fence'], stateSHA=sha(state), actualMACMax=max(map(abs,faces)),
                           pressureRawRange=[min(p),max(p)])
    return result


def source():
    result = {}
    path = REPORT/'t987/runs/g13-source'
    for arm in ('source-fast','source-grouped','source-singletons-opt'):
        rows = extract(path/(arm+'.jsonl'))
        start = only(rows, stage='T986_SOURCE_PLAN')
        submit = only(rows, stage='T986_SOURCE_RECORDED_SLOTS')
        end = only(rows, stage='T877_ROOT_ACCEPT')
        wait = only(rows, stage='T877_ROOT_WAIT')
        complete = only(rows, state='T986_SOURCE_ONLY_CAPTURED')
        assert submit['nonce'] == end['nonce'] == wait['nonce']
        assert submit['fence'] == wait['fence'] and submit['fenceGeneration'] == wait['fenceGeneration']
        assert submit['submitted'] and wait['lastBusinessWait']['vkResult'] == 0
        assert end['timelyDiagnosticMatch'] and end['acceptedRoot'] is None and end['originalOutcome'] == 1
        assert end['originalFirstErrorOp'] == end['originalFirstError'] == 0 and end['latchOK']
        assert len(start['slots']) == len(submit['slots']) == 16
        for planned, recorded in zip(start['slots'], submit['slots'], strict=True):
            assert all(planned[key] == recorded[key] for key in ('ordinal','role','flags','sha256','push','set'))
        cold = (path/(arm+'.source')).read_bytes()
        arch = complete['t833SourceArchive']
        assert sha(cold) == arch['sha256']
        compile_times = {}
        for name in ('OP55_58_INSTALL','OP56_59_AUDIT'):
            if arm == 'source-grouped':
                key = dict(schema='PC_G13_PIPELINE_PROGRESS_V1', call='vkCreateComputePipelines', role=name)
                enter, exited = only(rows, edge='ENTER', **key), only(rows, edge='EXIT', **key)
                assert exited['vkResult'] == 0 and enter['moduleSHA'] == exited['moduleSHA']
                compile_times[name] = (int(exited['qpc'])-int(enter['qpc']))/int(enter['qpcFrequency'])
        result[arm] = dict(grantToMatchMs=(int(end['acceptNs'])-int(wait['grantNs']))/10**6,
                           inputSHA=submit['inputSHA'], configSHA=submit['configBytesSHA'], archiveSHA=sha(cold),
                           sourceCandidateBank=arch['candidateBank'], sourceCandidateSHA=arch['segmentSHA'][1],
                           groupedCreateSeconds=compile_times, acceptedRoot=None)
    assert len({entry['inputSHA'] for entry in result.values()}) == 1
    return result


if __name__ == '__main__':
    print(json.dumps(dict(schema='T988_OFFLINE_V1', g9=g9(), op21Math=quotient(), preview16=preview(), source=source()), indent=2))
