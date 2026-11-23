"""Measure accepted and displayed PC business frames in a fixed QPC window."""
import argparse
from collections import Counter
import hashlib
import json
import math
from pathlib import Path

SCOPE = ('pid', 'processStartTick', 'ticket', 'ownerTid', 'surface', 'geometry')
WINDOW_SCOPE = SCOPE + ('windowEpoch', 'deviceEpoch', 'swapchainEpoch')
RESULTS = ('acquire', 'submit', 'fence', 'present', 'imagePresent')
COSTS = ('computeMs', 'renderMs', 'submitMs', 'fenceMs')
TARGET = 120


def need(ok, code):
    if not ok:
        raise ValueError(code)


def metric(values):
    ordered = sorted(values)
    if not ordered:
        return None
    return {'count': len(ordered), 'mean': sum(ordered) / len(ordered),
            'p95': ordered[math.ceil(.95 * len(ordered)) - 1],
            'p99': ordered[math.ceil(.99 * len(ordered)) - 1], 'max': ordered[-1]}


def identity(row, fields=SCOPE):
    return tuple(str(row[k]) for k in fields)


def root(frame):
    return tuple(int(frame[k]) for k in
                 ('run', 'config', 'acceptedRoot', 'generation', 'publicationSlot',
                  'sourceCommit', 'physicsCommit'))


def originals(path):
    raw = path.read_bytes()
    rows = [json.loads(line) for line in raw.splitlines() if line.strip()]
    queued = [r for r in rows if r.get('state') == 'M1_DRY_SOURCE_PRESENT_QUEUED']
    retired = [r for r in rows if r.get('state') == 'M1_DRY_SOURCE_PRESENT_RETIRED']
    diagnostics = []
    for row in queued:
        frame = row['windowFrame']
        same = [r for r in retired if identity(r) == identity(row) and r['windowFrame'] == frame
                and r.get('m1PublicationReleased') is True and r.get('m1WsiResult') == 0]
        w = row['m1Completion']
        nonce = w[4] | w[5] << 32
        need(len(same) == 1 and len(w) == 256 and tuple(w[:4]) == (1381192263, 2, 256, 3235823838)
             and w[27:29] == [1, 1] and row['m1OwnerCode'] == 0 and row['m1FenceVkResult'] == 0
             and w[61] == row['m1AdmittedQ'] > 0
             and str(nonce) == frame['acceptedRoot'] == row['m1Nonce']
             and int(frame['generation']) == row['m1PublicationGeneration'] and frame['publicationSlot'] == w[35]
             and all(frame[k] == 0 for k in RESULTS) and frame['pixel'] == 0xff00ff00
             and int(row['qpcFrequency']) > 0
             and 0 < int(frame['presentEnterQpc']) <= int(frame['presentExitQpc']), 'DRY_SOURCE_IDENTITY')
        diagnostics.append({'scope': identity(row), 'acceptedRoot': str(nonce),
                            'presentExitQpc': frame['presentExitQpc'], 'admittedQ20': w[61]})
    return {'sha256': hashlib.sha256(raw).hexdigest(), 'drySourceDiagnostics': diagnostics,
            'verifiedBusinessFps': None, 'displayEvidence': 'ABSENT'}


def measure(plan, accepted, attempts):
    """Read normalized independent streams with a preregistered QPC window."""
    start, end, hz = (int(plan[k]) for k in ('startQpc', 'endQpc', 'qpcFrequency'))
    need(hz > 0 and start >= 0 and end - start >= hz, 'MEASUREMENT_WINDOW')
    scope = identity(plan, WINDOW_SCOPE)
    run_config = (int(plan['run']), int(plan['config']))
    seconds = (end - start) / hz
    tolerance = float(plan['realtimeTolerance'])
    need(0 <= tolerance < 1, 'REALTIME_TOLERANCE')
    need(isinstance(plan['inventoryComplete'], bool), 'INVENTORY_FLAG')
    roots, physics = {}, set()
    accept_counts = Counter()
    business_costs = {key: [] for key in COSTS}
    sim_ns = 0
    last_time = -1
    last_progress = -1
    last_generation = -1
    last_physics = -1
    for row in accepted:
        need(identity(row, WINDOW_SCOPE) == scope, 'ACCEPT_SCOPE')
        tick = int(row['acceptQpc'])
        need(tick >= last_time, 'ACCEPT_ORDER')
        last_time = tick
        if tick >= end:
            continue
        kind = row['kind']
        need(kind in ('PHYSICS', 'SOURCE'), 'ACCEPT_KIND')
        key = root(row)
        run, config, nonce, generation, slot, source, commit = key
        need((run, config) == run_config and run > 0 and config > 0
             and nonce > 0 and generation > last_generation
             and slot >= 0 and source >= 0 and commit > last_physics and key not in roots,
             'ACCEPT_IDENTITY')
        last_generation, last_physics = generation, commit
        delta = int(row['simDeltaNs'])
        if kind == 'PHYSICS':
            progress = int(row['simTick'])
            need(delta > 0 and progress > last_progress, 'PHYSICS_PROGRESS')
            last_progress = progress
            physics.add(key)
        else:
            need(delta == 0, 'SOURCE_TIME')
        roots[key] = tick
        if start <= tick < end:
            accept_counts[kind] += 1
            sim_ns += delta
            for name in COSTS:
                if row.get(name) is not None:
                    cost = float(row[name])
                    need(math.isfinite(cost) and cost >= 0, 'STAGE_COST')
                    business_costs[name].append(cost)

    counters = Counter()
    queued_times, displayed_times = [], []
    queued_roots, displayed_roots = set(), set()
    display_ids, attempt_ids = set(), set()
    last_exit = -1
    expected_attempt = plan.get('firstSequence')
    for row in attempts:
        enter, exited = int(row['presentEnterQpc']), int(row['presentExitQpc'])
        need(last_exit <= exited and 0 <= enter <= exited, 'PRESENT_ORDER')
        last_exit = exited
        if not start <= exited < end:
            continue
        if expected_attempt is not None:
            need(int(row['sequence']) == expected_attempt, 'ATTEMPT_SEQUENCE_GAP')
            expected_attempt += 1
        attempt_id = row['presentId']
        need(attempt_id and attempt_id not in attempt_ids, 'PRESENT_ID')
        attempt_ids.add(attempt_id)
        if any(k in row and row[k] != 0 for k in RESULTS):
            counters['FAILED'] += 1
            continue
        if row.get('acceptedRoot') in (None, 0, '0', ''):
            counters['EMPTY'] += 1
            continue
        if not all(k in row for k in RESULTS):
            counters['INCOMPLETE'] += 1
            continue
        if row.get('kind') == 'INTERPOLATED':
            counters['INTERPOLATED'] += 1
            continue
        key = root(row)
        if (row.get('kind') == 'STALE' or identity(row, WINDOW_SCOPE) != scope
                or key not in roots or roots[key] > enter):
            counters['STALE'] += 1
            continue
        if key in queued_roots or roots[key] < start:
            counters['REPEAT'] += 1
        elif key in physics:
            counters['NEW_PHYSICS'] += 1
            queued_times.append(exited)
        else:
            counters['NEW_SOURCE'] += 1
        queued_roots.add(key)
        outcome = row.get('displayStatus')
        if outcome == 'DROPPED':
            counters['DISPLAY_DROPPED'] += 1
        elif outcome == 'DISPLAYED':
            qpc = int(row['displayQpc'])
            display_id = row['displayId']
            need(display_id and display_id not in display_ids and qpc >= enter, 'DISPLAY_JOIN')
            display_ids.add(display_id)
            if start <= qpc < end and roots[key] >= start and key in physics and key not in displayed_roots:
                displayed_times.append(qpc)
                displayed_roots.add(key)
        else:
            counters['DISPLAY_UNCONFIRMED'] += 1
    if plan['inventoryComplete']:
        need(expected_attempt is not None and expected_attempt == plan['endSequenceExclusive'],
             'ATTEMPT_INVENTORY')
    refresh = plan.get('refreshHz')
    need(refresh is None or (math.isfinite(float(refresh)) and float(refresh) > 0), 'REFRESH_MODE')
    display_ready = (plan.get('displayTraceVerified') is True and plan['inventoryComplete']
                     and not counters['DISPLAY_UNCONFIRMED'] and not counters['INCOMPLETE']
                     and isinstance(plan.get('displayTraceSHA256'), str)
                     and len(plan['displayTraceSHA256']) == 64)
    if refresh is None or not display_ready:
        verdict = 'PRESENT_EVIDENCE_PENDING'
    elif float(refresh) < TARGET:
        verdict = 'DISPLAY_MODE_BELOW_TARGET'
    elif len(displayed_times) / seconds < TARGET or sim_ns / 1e9 / seconds < 1 - tolerance:
        verdict = 'BELOW_120_OR_REALTIME'
    else:
        verdict = 'WINDOW_MEETS_RATE_AND_REALTIME'

    def intervals(ticks):
        ticks = sorted(ticks)
        return metric([(b - a) * 1000 / hz for a, b in zip(ticks, ticks[1:])])

    return {'verdict': verdict, 'seconds': seconds, 'refreshHz': refresh,
            'accepted': dict(accept_counts), 'acceptedPhysicsFps': accept_counts['PHYSICS'] / seconds,
            'simulationToWall': sim_ns / 1e9 / seconds, 'presentCounts': dict(counters),
            'queuedNewPhysicsFps': len(queued_times) / seconds, 'queueIntervalsMs': intervals(queued_times),
            'displayedNewPhysicsFps': len(displayed_times) / seconds if display_ready else None,
            'displayIntervalsMs': intervals(displayed_times) if display_ready else None,
            'acceptedPhysicsWithoutConfirmedDisplayInWindow':
                accept_counts['PHYSICS'] - len(displayed_times) if display_ready else None,
            'stageCostMs': {name: metric(values) for name, values in business_costs.items()}}


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('mode', choices=('original', 'measure'))
    parser.add_argument('input', type=Path)
    parser.add_argument('--display-trace', type=Path, help='original trace with the declared SHA-256')
    args = parser.parse_args()
    try:
        if args.mode == 'original':
            result = originals(args.input)
        else:
            evidence = json.loads(args.input.read_bytes())
            plan = evidence['plan']
            plan['displayTraceVerified'] = False
            if args.display_trace:
                with args.display_trace.open('rb') as trace:
                    need(hashlib.file_digest(trace, 'sha256').hexdigest() == plan['displayTraceSHA256'],
                         'DISPLAY_TRACE_SHA')
                plan['displayTraceVerified'] = True
            result = measure(plan, evidence['accepted'], evidence['attempts'])
    except (OSError, ValueError, KeyError, TypeError) as error:
        parser.exit(1, f'{error}\n')
    print(json.dumps(result, indent=2, allow_nan=False))


if __name__ == '__main__':
    main()
