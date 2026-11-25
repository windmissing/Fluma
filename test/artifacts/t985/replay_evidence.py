"""Recompute the T985 offline receipt from the frozen T977 files."""

import hashlib
import json
import struct
from fractions import Fraction
from pathlib import Path


ROOT = Path(__file__).resolve().parents[3]
RUNS = ROOT / 'test/reports/t977/runs'
PROBE_WORDS = [1065353217, 1065353216, 1, 4194304, 7, 0, 0, 1179661106] * 2
ARMS = ('A1-off-r1', 'B1-on-r3', 'B2-on-r1', 'A2-off-r1')
DETAIL = ((13, 25, 1), (15, 27, 1), (819, 25, 2), (821, 27, 2), (2017, 25, 4), (2019, 27, 4))


def sha(data):
    return hashlib.sha256(data).hexdigest()


def text_sha(path):
    return sha(path.read_bytes().replace(b'\r\n', b'\n'))


def rows(path):
    return [json.loads(line) for line in path.read_bytes().splitlines()]


def row(trace, **fields):
    match, = (line for line in trace if all(line.get(key) == value for key, value in fields.items()))
    return match


def identity(line):
    return tuple(line[key] for key in ('role', 'moduleSHA', 'pid', 'processStartTick', 'ownerTid'))


def g13():
    receipt = {}
    for path in sorted((RUNS / 'qual').glob('*.jsonl')):
        trace = rows(path)
        schema = 'PC_G13_PIPELINE_PROGRESS_V1'
        shader = row(trace, schema=schema, call='vkCreateShaderModule', edge='EXIT')
        enter = row(trace, schema=schema, call='vkCreateComputePipelines', edge='ENTER')
        assert identity(shader) == identity(enter) and shader['vkResult'] == 0
        assert enter['pipelineCache'] is None and enter['vkResult'] is None
        section = dict(logTextSHA=text_sha(path), moduleSHA=enter['moduleSHA'],
                       role=enter['role'], flags=enter['flags'], process=enter['pid'],
                       qpcFrequency=enter['qpcFrequency'], createEnter=enter['qpc'],
                       layoutSHA=enter['layoutSHA'])
        if path.stem.endswith('-legacy'):
            assert trace[-1] == enter
            section['observed'] = 'PIPELINE_ENTER_WITHOUT_EXIT'
        else:
            exited = row(trace, schema=schema, call='vkCreateComputePipelines', edge='EXIT')
            probe = row(trace, schema='PC_G13_STEADY_OP66_V1')
            assert identity(enter) == identity(exited) == identity(probe)
            submitted = row(trace, schema=schema, call='vkQueueSubmit', edge='EXIT')
            fenced = row(trace, schema=schema, call='vkWaitForFences', edge='EXIT')
            assert identity(probe) == identity(submitted) == identity(fenced)
            assert [line['vkResult'] for line in (exited, submitted, fenced)] == [0, 0, 0]
            assert trace.index(enter) < trace.index(exited) < trace.index(submitted) < trace.index(fenced) < trace.index(probe)
            assert probe['probeWords'] == PROBE_WORDS and probe['mismatchMasks'] == [0, 0]
            assert probe['acceptedRoot'] is None and enter['flags'] == int(path.stem.endswith('-noopt'))
            delta = int(exited['qpc']) - int(enter['qpc'])
            assert delta > 0 and enter['qpcFrequency'] == exited['qpcFrequency']
            section.update(observed='ROLE_QUALIFIED', createExit=exited['qpc'],
                           createMs=float(Fraction(1000 * delta, int(enter['qpcFrequency']))),
                           createBracketNs=probe['pipelineNs'], probeWords=probe['probeWords'])
        receipt[path.stem] = section
    assert len(receipt) == 14
    for role in ('OP55', 'OP56', 'OP58', 'OP59', 'OP_SOURCE_DRIVE_ONLY'):
        noopt, opt = (receipt[f'{role}-{mode}'] for mode in ('noopt', 'opt'))
        assert noopt['moduleSHA'] == opt['moduleSHA'] and noopt['layoutSHA'] == opt['layoutSHA']
        assert noopt['createMs'] < opt['createMs']
    return receipt


def read_bundle(path, fields):
    files = {item['name']: item for item in fields}
    result = {}
    names = ('BASE.bin', 'ACTUAL.bin', 'SELECTED.bin', 'REPLAY.bin', 'CURRENT_BASE.bin',
             'EVENT.bin', 'PATH.bin', 'WORK.bin', 'COMPLETION256.bin', 'INPUT128.bin',
             'GPU_INPUT128.bin', 'FIXTURE128.bin')
    for name in names:
        raw = (path / name).read_bytes()
        assert len(raw) == files[name]['bytes'] and sha(raw) == files[name]['sha256'], name
        result[name] = raw
    identity_names = ('BASE.bin', 'ACTUAL.bin', 'SELECTED.bin', 'REPLAY.bin',
                      'AUTHORITY_ARENA.bin', 'SELECTED_ARENA.bin')
    assert len({files[name]['bufferGeneration'] for name in identity_names}) == len(identity_names)
    arena_sha = json.loads((path / 'arena-sha.json').read_bytes())
    assert arena_sha == {name: files[name]['sha256'] for name in identity_names[-2:]}
    return result, arena_sha


def cold_evidence(path, root, gpu):
    if not path:
        return {'availability': 'TIMESTAMP_LOG_ONLY'}
    envelope = json.loads(path.read_bytes())
    permit = envelope['permit']
    assert all(root[key] == permit[key] for key in ('run', 'nonce', 'plan', 'deadlineNs'))
    assert root['fence'] == envelope['rootFence'] and root['fenceGeneration'] == envelope['rootFenceGeneration']
    assert envelope['diagnosticGeneration'] != gpu['fenceGeneration']
    assert envelope['rootVkResult'] == 2 and envelope['acceptedRoot'] is None
    assert gpu['faceExactDivide'] == envelope['faceExactDivide']
    bundle, arena_sha = read_bundle(path.parent, envelope['files'])
    assert bundle['INPUT128.bin'] == bundle['GPU_INPUT128.bin']
    assert sha(bundle['INPUT128.bin']) == envelope['inputSHA'] == root['inputSHA']
    assert bundle['BASE.bin'] == bundle['CURRENT_BASE.bin'] and bundle['SELECTED.bin'] == bundle['REPLAY.bin']
    tail = struct.unpack('<256I', bundle['COMPLETION256.bin'])
    assert tuple(tail[i] for i in (37, 44, 46, 48, 49)) == (1, 260, 1, 0, 0)
    compare = [sha(bundle[name]) for name in ('FIXTURE128.bin', 'EVENT.bin', 'PATH.bin')]
    return dict(availability='PARTIAL_BUNDLE_WITH_ARENA_DIGESTS', envelopeTextSHA=text_sha(path),
                fixtureEventPathSHA=compare, arenaDeclaredSHA=arena_sha,
                physicalFileSHA={name: sha(raw) for name, raw in bundle.items()})


def g9():
    receipt = {}
    common = None
    for arm in ARMS:
        path = RUNS / 'face' / (arm + '.jsonl')
        trace = rows(path)
        root = row(trace, stage='ROOT', kind='QUANTUM')
        clock = row(trace, stage='QUANTUM_TIMELINE')
        retire = row(trace, stage='RETIRE', nonce=root['nonce'])
        gpu = row(trace, stage='ROOT_GPU_TIMESTAMPS', kind='QUANTUM')
        found = list((RUNS / 'face' / ('raw-' + arm) / 'diagnostic').glob('*/envelope.json'))
        assert len(found) <= 1
        assert all(root[key] == gpu[key] for key in ('run', 'nonce', 'plan', 'deadlineNs'))
        assert root['fence'] == gpu['fence'] == retire['fence']
        assert root['fenceGeneration'] == clock['fenceGeneration'] == retire['fenceGeneration'] == gpu['fenceGeneration'] == gpu['queryResetSerial']
        assert int(root['deadlineNs']) - int(clock['grantNs']) == 250_000_000
        assert root['vkResult'] == retire['vkResult'] == 2
        assert root['accepted'] is False and root['tailRead'] is False and clock['stop'] == 'GRANT_DEADLINE'
        assert gpu['acceptedRoot'] is None
        assert gpu['state'] == 'LATE_RETIRED_DIAGNOSTIC' and gpu['fenceVkResult'] == 0
        assert gpu['faceExactDivide'] == arm.startswith('B')
        assert gpu['timestampRequested'] and gpu['queryCreateVkResult'] == 0
        points = gpu['points']
        assert gpu['validBits'] == 36 and len(points) == 27
        assert all(p['i'] == i and p['vkResult'] == 0 and p['available'] for i, p in enumerate(points))
        period = Fraction(str(gpu['periodNs']))

        def measure(lo, hi):
            ticks = (int(points[hi]['tick']) - int(points[lo]['tick'])) % (1 << gpu['validBits'])
            return dict(first=points[lo]['tick'], last=points[hi]['tick'], ticks=ticks,
                        ms=float(ticks * period / 10**6))

        def bounded_dispatch(slot, opcode, scope, lo):
            for edge, point in zip(('INPUT_READY', 'DISPATCH_DONE'), points[lo:lo+2], strict=True):
                assert (point['slot'], point['opcode'], point['role'], point['edge']) == (slot, opcode, scope, edge)
            return dict(slot=slot, opcode=opcode, scope=scope, **measure(lo, lo+1))

        ms = measure(0, 14)
        spans = [dict(name=gpu['spans'][i]['name'], **measure(i, i+1)) for i in range(14)]
        details = [bounded_dispatch(*detail, 15+2*i) for i, detail in enumerate(DETAIL)]
        assert sum(span['ticks'] for span in spans) == ms['ticks']
        elapsed_edges = [measure(0, i)['ticks'] for i in range(15, 27)]
        assert elapsed_edges == sorted(elapsed_edges) and elapsed_edges[-1] <= ms['ticks']
        face_ms = sum(x['ticks'] for x in details) * period / 10**6
        cold = cold_evidence(found[0] if found else None, root, gpu)
        if 'fixtureEventPathSHA' in cold:
            compare = cold['fixtureEventPathSHA']
            assert common is None or common == compare
            common = compare
        bindings = [row(trace, stage='T976_FACE_DIVIDE_BINDING', opcode=op) for op in (25, 27)]
        assert all(bind['faceExactDivide'] == arm.startswith('B') for bind in bindings)
        receipt[arm] = dict(logTextSHA=text_sha(path),
                            run=root['run'], fence=root['fence'], generation=root['fenceGeneration'],
                            periodNs=gpu['periodNs'], clockMHz=float(1000 / period),
                            root=ms, spans=spans, detail=details, faceSumMS=float(face_ms),
                            residualIfFaceZeroMS=float(ms['ticks'] * period / 10**6 - face_ms),
                            lateOriginalFenceResult=root['vkResult'], acceptedRoot=None,
                            coldEvidence=cold, bindings=bindings)
    misses = {}
    for arm in ('B1-on-r1', 'B1-on-r2'):
        path = RUNS / 'face' / (arm + '.jsonl')
        trace = rows(path)
        source = row(trace, stage='ROOT', kind='SOURCE')
        clock = row(trace, stage='ROOT_TIMELINE', rootKind='SOURCE')
        assert source['accepted'] is False and clock['firstStop'] == 'GRANT_DEADLINE'
        assert not any(line.get('kind') == 'QUANTUM' for line in trace)
        misses[arm] = dict(logTextSHA=text_sha(path), sourceStop=clock['firstStop'],
                           elapsedMs=(int(clock['stopNs']) - int(clock['grantNs']))/10**6)
    a1, b1, b2, a2 = (receipt[arm]['root']['ms'] for arm in ARMS)
    assert receipt[ARMS[0]]['bindings'] == receipt[ARMS[3]]['bindings']
    assert receipt[ARMS[1]]['bindings'] == receipt[ARMS[2]]['bindings']
    assert all(left['moduleSHA'] != right['moduleSHA'] for left, right in
               zip(receipt[ARMS[0]]['bindings'], receipt[ARMS[1]]['bindings'], strict=True))
    return dict(arms=receipt, sourceOnlyAttempts=misses,
                rootEstimateBMinusAMs=(b1+b2-a1-a2)/2,
                rootAdjacentBMinusAMs=[b1-a1,b2-a2])


if __name__ == '__main__':
    print(json.dumps(dict(schema='T985_OFFLINE_V1', goal13=g13(), goal9=g9()), indent=2))
