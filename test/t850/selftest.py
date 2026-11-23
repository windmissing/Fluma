"""Validate the counter with retained PC diagnostics and synthetic display telemetry."""
from copy import deepcopy
import hashlib
import json
from pathlib import Path

from check import RESULTS, measure, metric, need, originals

ROOT = Path(__file__).resolve().parents[2]
BASE = ROOT / 'test/reports'
SCOPE = dict(pid=1, processStartTick='2', ticket='3', ownerTid=4, surface='5', geometry='6',
             windowEpoch='7', deviceEpoch='8', swapchainEpoch='9', run='1', config='2')


def fixture():
    plan = dict(SCOPE, startQpc=0, endQpc=120000, qpcFrequency=120000,
                realtimeTolerance=.001, inventoryComplete=True, firstSequence=1,
                endSequenceExclusive=121, refreshHz=120, displayTraceVerified=True,
                displayTraceSHA256=hashlib.sha256(b'synthetic-only').hexdigest())
    accepted, attempts = [], []
    for i in range(120):
        key = dict(SCOPE, acceptedRoot=str(10 + i), generation=i + 1,
                   publicationSlot=i % 2, sourceCommit=1, physicsCommit=i + 1)
        t = i * 1000
        accepted.append(dict(key, kind='PHYSICS', simDeltaNs=(i + 1) * 10**9 // 120 - i * 10**9 // 120,
                             acceptQpc=t, simTick=i + 1, computeMs=2))
        attempts.append(dict(key, **dict.fromkeys(RESULTS, 0), presentId=str(i),
                             sequence=i + 1, presentEnterQpc=t + 10, presentExitQpc=t + 20,
                             displayStatus='DISPLAYED', displayQpc=t + 25, displayId=str(i)))
    return plan, accepted, attempts


def main():
    t830 = originals(BASE / 't830/m1-source-window.jsonl')
    t840 = originals(BASE / 't840/run4-m1.jsonl')
    receipt = json.loads((BASE / 't830/m1-source-window-receipt.json').read_bytes())
    need(receipt['stdoutSHA'] == t830['sha256'], 'T830_SHA')
    need(all(len(r['drySourceDiagnostics']) == 1 and r['verifiedBusinessFps'] is None
             for r in (t830, t840)), 'ORIGINAL_CLASSIFICATION')
    plan, accepted, attempts = fixture()
    positive = measure(plan, accepted, attempts)
    need(positive['verdict'] == 'WINDOW_MEETS_RATE_AND_REALTIME'
         and positive['displayedNewPhysicsFps'] == 120
         and positive['displayIntervalsMs']['p99'] == 1000 / 120, 'POSITIVE')
    refresh_tests = {}
    for refresh, expected in ((144, 'WINDOW_MEETS_RATE_AND_REALTIME'),
                              (60, 'DISPLAY_MODE_BELOW_TARGET'), (None, 'PRESENT_EVIDENCE_PENDING')):
        verdict = measure(dict(plan, refreshHz=refresh), accepted, attempts)['verdict']
        need(verdict == expected, 'REFRESH')
        refresh_tests[str(refresh)] = verdict
    reclassified = deepcopy(attempts)
    for field in ('acceptedRoot', 'generation', 'publicationSlot', 'physicsCommit'):
        reclassified[1][field] = reclassified[0][field]
    reclassified[2]['kind'] = 'STALE'
    reclassified[3]['acceptedRoot'] = None
    reclassified[4]['present'] = -1
    reclassified[5]['kind'] = 'INTERPOLATED'
    reclassified[6]['displayStatus'] = 'DROPPED'
    accepted[7].update(kind='SOURCE', simDeltaNs=0)
    negative = measure(plan, accepted, reclassified)
    need(negative['verdict'] == 'BELOW_120_OR_REALTIME'
         and negative['displayedNewPhysicsFps'] == 113
         and all(negative['presentCounts'][k] == 1 for k in
                 ('REPEAT', 'STALE', 'EMPTY', 'FAILED', 'INTERPOLATED', 'DISPLAY_DROPPED', 'NEW_SOURCE')),
         'NEGATIVE_CLASSES')
    reclassified[8].pop('displayStatus')
    need(measure(plan, accepted, reclassified)['verdict'] == 'PRESENT_EVIDENCE_PENDING', 'MISSING_DISPLAY')
    reclassified.pop(8)
    try:
        measure(plan, accepted, reclassified)
    except ValueError as error:
        need(str(error) == 'ATTEMPT_SEQUENCE_GAP', 'WRONG_GAP_REASON')
    else:
        raise AssertionError('ATTEMPT_SEQUENCE_GAP')
    need(metric(range(1, 101)) == dict(count=100, mean=50.5, p95=95, p99=99, max=100),
         'NEAREST_RANK')
    print(json.dumps({'originalT830': t830, 'originalT840': t840, 'synthetic120': positive,
                      'syntheticNegative': negative, 'refreshControls': refresh_tests,
                      'otherControls': ['MISSING_DISPLAY', 'ATTEMPT_SEQUENCE_GAP', 'NEAREST_RANK']}, indent=2))


if __name__ == '__main__':
    main()
