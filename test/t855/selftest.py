"""Emit synthetic pre-READY controls and regress historical PC evidence readers."""
import argparse
from copy import deepcopy
import json
from pathlib import Path
import subprocess
import sys

from check import ROOT, check, historical, require


def fixture(route, outcome):
    identity = dict(pid=1, processStartTick='100', ticket='2', ownerTid=3,
                    device='4', surface='5', geometry='6')
    rows = []

    def emit(name, **fields):
        rows.append(dict(identity, event=name, qpc=len(rows)+1, **fields))

    if route == 'INTERACTIVE':
        emit('SUBMITTED', fence='angle')
        if outcome == 'REVOKED_IN_FLIGHT':
            emit('REVOKE', reason='RESIZE', newGeometry='7')
            emit('FENCE_RETIRED', fence='angle', vkResult=0)
            emit('FAULT', reason='REVOKED', resourcesRetired=True)
            return dict(synthetic=True, route=route, identity=identity, events=rows)
        emit('FENCE_RETIRED', fence='angle', vkResult=0)
        emit('ANGLE_DONE', vkResult=0, bytes=245776)
    handles = {
        'SHORT_ALLOC': list(range(1, 9)) + [0] * 8,
        'NULL_SET': list(range(1, 16)) + [0],
        'ALLOC_FAILED': [0] * 16,
    }.get(outcome, list(range(1, 17)))
    emit('SOURCE_ALLOC', expected=16, poolMaxSets=16, storageCapacity=64, uniformCapacity=16,
         requested=8 if outcome == 'SHORT_ALLOC' else 16, handles=handles, nonNull=sum(map(bool, handles)),
         firstNull=next((i for i, h in enumerate(handles) if not h), None),
         vkResult=-2 if outcome == 'ALLOC_FAILED' else 0, pool='source-pool')
    if outcome in ('INIT_PASS', 'REVOKED'):
        emit('SOURCE_WRITES_BEGIN', pool='source-pool')
        emit('SOURCE_WRITES_DONE', pool='source-pool', apiReturned=True,
             writes=[dict(bank=b, **{'pass': p}, dstSet=handles[b*8+p], storage=4, uniform=1)
                     for b in range(2) for p in range(8)])
    if outcome == 'INIT_PASS':
        emit('READY' if route == 'INTERACTIVE' else 'FIXED_INIT_DONE')
    else:
        if outcome == 'REVOKED':
            emit('REVOKE', reason='RESIZE', newGeometry='7')
        emit('FAULT', reason=outcome, resourcesRetired=True)
    return dict(synthetic=True, route=route, identity=identity, events=rows)


def selftest():
    names = ('INIT_PASS', 'SHORT_ALLOC', 'NULL_SET', 'ALLOC_FAILED', 'REVOKED')
    cases = [(route, outcome) for route in ('FIXED_SOURCE', 'INTERACTIVE') for outcome in names]
    cases.append(('INTERACTIVE', 'REVOKED_IN_FLIGHT'))
    samples = [fixture(*case) for case in cases]
    observed = [check(sample, True) for sample in samples]
    require([r['outcome'] for r in observed] == [outcome.removesuffix('_IN_FLIGHT') for _, outcome in cases],
            'OUTCOME_MATRIX')
    for (_, outcome), actual in zip(cases, observed):
        if outcome in ('SHORT_ALLOC', 'NULL_SET', 'ALLOC_FAILED', 'REVOKED_IN_FLIGHT'):
            require(actual['updatesStarted'] == actual['updatesCompleted'] == 0
                    and not actual['readyPublished'], 'FAIL_CLOSED')

    def mutate(route, outcome, old_event, **changes):
        sample = fixture(route, outcome)
        next(r for r in sample['events'] if r['event'] == old_event).update(changes)
        return sample

    faults = [
        (mutate('INTERACTIVE', 'SHORT_ALLOC', 'FAULT', event='SOURCE_WRITES_BEGIN'), 'UPDATE_BEFORE_VALID_ALLOCATION'),
        (mutate('INTERACTIVE', 'NULL_SET', 'FAULT', event='SOURCE_WRITES_BEGIN'), 'UPDATE_BEFORE_VALID_ALLOCATION'),
        (mutate('FIXED_SOURCE', 'ALLOC_FAILED', 'FAULT', event='SOURCE_WRITES_BEGIN'), 'UPDATE_BEFORE_VALID_ALLOCATION'),
        (mutate('INTERACTIVE', 'REVOKED', 'FAULT', event='READY'), 'PREMATURE_OR_STALE_READY'),
        (mutate('INTERACTIVE', 'REVOKED_IN_FLIGHT', 'FENCE_RETIRED', vkResult=1), 'FENCE_RETIRE'),
        (mutate('INTERACTIVE', 'INIT_PASS', 'READY', geometry='9'), 'OWNER_OR_GEOMETRY'),
        (mutate('INTERACTIVE', 'INIT_PASS', 'READY', ownerTid=999), 'OWNER_OR_GEOMETRY'),
        (mutate('INTERACTIVE', 'INIT_PASS', 'ANGLE_DONE', vkResult=-4), 'ANGLE_FENCE'),
        (mutate('FIXED_SOURCE', 'INIT_PASS', 'SOURCE_WRITES_DONE', writes=[]), 'WRITE_COVERAGE'),
        (mutate('INTERACTIVE', 'INIT_PASS', 'SOURCE_WRITES_DONE', vkResult=0), 'UPDATE_RECEIPT'),
        (mutate('FIXED_SOURCE', 'INIT_PASS', 'SOURCE_ALLOC', nonNull=8), 'HANDLE_REPORT'),
    ]
    bad = fixture('INTERACTIVE', 'INIT_PASS')
    bad['events'].pop()
    faults.append((bad, 'INCOMPLETE_INITIALIZATION'))
    stale = fixture('INTERACTIVE', 'REVOKED_IN_FLIGHT')
    del stale['events'][-2]
    faults.append((stale, 'FAULT_OR_RETIRE'))
    rejected = []
    for sample, reason in faults:
        try:
            check(sample, True)
        except ValueError as err:
            require(str(err) == reason, f'{reason}: {err}')
        else:
            raise AssertionError(reason)
        rejected.append(reason)
    return samples, observed, rejected


def sibling(path, *args):
    result = subprocess.run([sys.executable, str(ROOT / path), *args], cwd=ROOT,
                            check=True, capture_output=True, text=True)
    return json.loads(result.stdout)


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--emit', type=Path)
    args = parser.parse_args()
    samples, observed, rejected = selftest()
    if args.emit:
        args.emit.write_text(json.dumps(samples, indent=2), encoding='utf-8')
    t836 = sibling('test/t836/check.py', 'selftest')
    t848 = sibling('test/t848/selftest.py')
    fixed = sibling('test/t850/check.py', 'original', 'test/reports/t840/run4-m1.jsonl')
    require(t836['state'] == 'SYNTHETIC_PASS' and len(t848['negativeControls']) == 10
            and len(fixed['drySourceDiagnostics']) == 1, 'REUSE_REGRESSION')
    print(json.dumps({'historical': historical(), 'samples': observed, 'negativeControls': rejected,
                      't836': t836, 't848Negatives': t848['negativeControls'],
                      'nativeCold': t848['nativeCold'], 'historicalFixed': fixed['drySourceDiagnostics'],
                      'newPE': 'PENDING_EFFECTOR'}, indent=2))


if __name__ == '__main__':
    main()
