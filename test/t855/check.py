"""Judge normalized Windows M1 SOURCE initialization and generation traces."""
import argparse
from collections import Counter
import hashlib
import json
from pathlib import Path

ROOT = Path(__file__).resolve().parents[2]
PROCESS = ('pid', 'processStartTick', 'ticket', 'ownerTid', 'device', 'surface')
SET_COUNT = 16
ANGLE_BYTES = 245776
FAILURES = {'SHORT_ALLOC', 'NULL_SET', 'ALLOC_FAILED', 'REVOKED'}


def require(ok, code):
    if not ok:
        raise ValueError(code)


def verified_file(item):
    data = (ROOT / item['path']).read_bytes()
    require(len(data) == item['bytes']
            and hashlib.sha256(data).hexdigest() == item['sha256'], 'T851_ORIGINAL_SHA')
    return data


def historical():
    receipt = json.loads((ROOT / 'test/reports/t851/capture.json').read_bytes())
    desc = receipt['descriptors']
    require((desc['routeCount'], desc['passesPerRoute'], desc['poolMaxSets'],
             desc['allocatedSets'], desc['plannedWriteSets']) == (2, 8, 16, 8, 16)
            and desc['unallocatedIndices'] == list(range(8, 16)), 'T851_DESCRIPTOR_FINDING')
    stack = verified_file(receipt['stack'])
    disassembly = verified_file(receipt['disassembly'])
    require(b'code=0xc0000005' in stack and b'arg1=0x55' in stack and b'igvk64.dll' in stack
            and b'00000001400177DB:' in disassembly and b'0000000140017BA5:' in disassembly,
            'T851_NATIVE_ORIGINAL')
    runs = []
    for expected in receipt['runs']:
        lines = list(map(json.loads, verified_file(expected['file']).splitlines()))
        final = lines[-1]
        require(len(lines) == expected['rows']
                and all(final[k] == expected[k] for k in ('pid', 'processStartTick', 'ownerTid'))
                and final['stage'] == expected['lastStage'] == 'M1_QUANTUM_COLD_ANGLES'
                and final['state'] == expected['lastState'] == 'RUNNING'
                and all('M1_POINTER_READY' not in (r.get('stage'), r.get('state')) for r in lines),
                'T851_TRACE_BOUNDARY')
        runs.append({'pid': final['pid'], 'rows': len(lines), 'lastStage': final['stage']})
    require(receipt['angle']['coldUploadCallReturnedBeforeObservedException']
            and receipt['exception']['appCall'] == 'vkUpdateDescriptorSets.M1_SOURCE'
            and receipt['exception']['windowsThreadMatchesOwner'], 'T851_STACK_SCOPE')
    return {'state': 'HISTORICAL_SOURCE_UPDATE_CRASH', 'runs': runs,
            'allocated': desc['allocatedSets'], 'planned': desc['plannedWriteSets']}


def check(trace, synthetic=False):
    require(trace['synthetic'] is synthetic, 'EVIDENCE_MODE')
    route = trace['route']
    require(route in ('FIXED_SOURCE', 'INTERACTIVE'), 'ROUTE')
    start = trace['identity']
    scope = tuple(start[k] for k in PROCESS)
    geometry = start['geometry']
    require(all(v not in (None, 0, '', '0') for v in (*scope, geometry)), 'OWNER_SCOPE')
    stages = Counter()
    angles = False
    alloc = None
    reason = None
    revoked = False
    queue_fences = set()
    last_qpc = -1
    terminal = None
    for row in trace['events']:
        event = row['event']
        require(tuple(row[k] for k in PROCESS) == scope and row['geometry'] == geometry,
                'OWNER_OR_GEOMETRY')
        require(terminal is None and row['qpc'] >= last_qpc, 'EVENT_ORDER')
        last_qpc = row['qpc']
        stages[event] += 1
        require(stages[event] == 1 or event in ('SUBMITTED', 'FENCE_RETIRED'), 'DUPLICATE_STAGE')
        if event == 'SUBMITTED':
            require(not revoked and not reason and stages['SOURCE_WRITES_DONE'] == 0
                    and row['fence'] and row['fence'] not in queue_fences, 'SUBMIT_AFTER_REVOKE')
            queue_fences.add(row['fence'])
        elif event == 'FENCE_RETIRED':
            require(row['fence'] in queue_fences and row['vkResult'] == 0, 'FENCE_RETIRE')
            queue_fences.remove(row['fence'])
        elif event == 'ANGLE_DONE':
            require(route == 'INTERACTIVE' and alloc is None and row['vkResult'] == 0
                    and row['bytes'] == ANGLE_BYTES and not queue_fences and not revoked, 'ANGLE_FENCE')
            angles = True
        elif event == 'SOURCE_ALLOC':
            require(not revoked and (route == 'FIXED_SOURCE' or angles), 'ALLOC_BEFORE_ANGLE')
            require(row['expected'] == SET_COUNT and row['poolMaxSets'] == SET_COUNT
                    and row['storageCapacity'] == 64 and row['uniformCapacity'] == 16, 'POOL_LAYOUT')
            handles = row['handles']
            require(len(handles) == SET_COUNT and all(type(h) is int and h >= 0 for h in handles)
                    and row['nonNull'] == sum(bool(h) for h in handles)
                    and row['firstNull'] == next((i for i, h in enumerate(handles) if not h), None),
                    'HANDLE_REPORT')
            if row['vkResult'] != 0:
                reason = 'ALLOC_FAILED'
            elif row['requested'] != SET_COUNT:
                reason = 'SHORT_ALLOC'
            elif row['nonNull'] != SET_COUNT:
                reason = 'NULL_SET'
            else:
                require(len(set(handles)) == SET_COUNT, 'ALIASED_SET')
                alloc = row
        elif event == 'SOURCE_WRITES_BEGIN':
            require(alloc is not None and not reason and not revoked, 'UPDATE_BEFORE_VALID_ALLOCATION')
            require(row['pool'] == alloc['pool'], 'UPDATE_POOL')
        elif event == 'SOURCE_WRITES_DONE':
            require(stages['SOURCE_WRITES_BEGIN'] == 1 and not reason and not revoked, 'UPDATE_ORDER')
            require(row['pool'] == alloc['pool'] and row['apiReturned'] is True
                    and 'vkResult' not in row, 'UPDATE_RECEIPT')
            expected = [(bank, p, alloc['handles'][bank * 8 + p], 4, 1)
                        for bank in range(2) for p in range(8)]
            require([(w['bank'], w['pass'], w['dstSet'], w['storage'], w['uniform'])
                     for w in row['writes']] == expected, 'WRITE_COVERAGE')
        elif event == 'REVOKE':
            require(not stages['READY'] and not stages['FIXED_INIT_DONE']
                    and row['reason'] in ('RESIZE', 'CLOSE', 'CANCEL')
                    and (row['reason'] != 'RESIZE' or row['newGeometry'] != geometry),
                    'REVOKE_IDENTITY')
            revoked = True
            reason = reason or 'REVOKED'
        elif event in ('READY', 'FIXED_INIT_DONE'):
            require(not reason and not revoked and not queue_fences and stages['SOURCE_WRITES_DONE'] == 1
                    and event == ('READY' if route == 'INTERACTIVE' else 'FIXED_INIT_DONE'),
                    'PREMATURE_OR_STALE_READY')
            terminal = 'INIT_PASS'
        elif event == 'FAULT':
            require(reason in FAILURES and row['reason'] == reason
                    and not queue_fences and row['resourcesRetired'] is True, 'FAULT_OR_RETIRE')
            terminal = reason
        else:
            raise ValueError('UNKNOWN_EVENT')
    require(terminal is not None, 'INCOMPLETE_INITIALIZATION')
    return {'outcome': terminal, 'route': route, 'updatesStarted': stages['SOURCE_WRITES_BEGIN'],
            'updatesCompleted': stages['SOURCE_WRITES_DONE'],
            'readyPublished': bool(stages['READY']), 'scope': start}


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('mode', choices=('historical', 'check'))
    parser.add_argument('input', nargs='?', type=Path)
    parser.add_argument('--synthetic', action='store_true')
    args = parser.parse_args()
    if args.mode == 'check' and args.input is None:
        parser.error('check requires a normalized trace')
    try:
        if args.mode == 'historical':
            result = historical()
        else:
            evidence = json.loads(args.input.read_bytes())
            require(isinstance(evidence, list) and evidence, 'EMPTY_TRACE_SET')
            result = [check(t, args.synthetic) for t in evidence]
    except (OSError, KeyError, TypeError, ValueError) as err:
        parser.exit(1, f'{err}\n')
    print(json.dumps(result, indent=2))


if __name__ == '__main__':
    main()
