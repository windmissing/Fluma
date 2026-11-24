"""T914 version-aware sand gate: same structural checks as T905 sand(), with
roleSetSHA taken from the run's own BOOT row (baseline must still equal the
frozen historical SHA). Dispatch count follows the run's sourceSchedule.
"""
import argparse
import json
import math
from pathlib import Path
import sys

ROOT = Path(__file__).resolve().parents[2]
sys.path.insert(0, str(ROOT / 'test/t496'))
from check_t896 import T888  # noqa: E402

need = T888.need

FAMILIES = {
    'DM2': dict(baseline='3639e57f834308bab001ca783f321b6a503c232caa924e04b1d8f1d7e3333ca6', dispatches=12,
                spans=('PREPARE', 'SOURCE_WRITER', 'AUDIT_COMPARE_PACK_SEAL')),
    'R1': dict(baseline='f7d812d64eba3c932fcfc41d70fbed48835349819f235b6d9d61fb58216641f3', dispatches=19,
               spans=('PREPARE', 'ACTUAL_WRITER', 'REPLAY_AUDIT_APPLY', 'COMPARE_PACK_AUDIT_SEAL')),
}
VARIANTS = {
    't913-dm2-t913-v1': 'ada9bc45d057e8f819499232acbf5d62733d4929a87504f0896ad5f6a59453df',
    't913-r1-t913-v1': '86b479daf81320fef11080b742d20b3d9600e9c43fc7148f057adb595e98eb27',
    't913-dm2-t913-split-v2': 'f51229f446032a95144594151d75fd8500716086774e01ed630c6d06bcdf3f3f',
    't913-r1-t913-split-v2': '58c999caf018194bb22917fce8668838e6381174ab53c708233332fb71787327',
}
FINE_DISPATCH = {'DM2': 14, 'R1': 27}


def sand_t914(rows, family):
    spec = FAMILIES[family]
    timelines = T888.rows_at(rows, 'ROOT_TIMELINE')
    need(len(timelines) == 1, 'ROOT_TIMELINE_MISSING')
    t = timelines[0]
    root, = [r for r in T888.rows_at(rows, 'ROOT') if r['kind'] == 'SOURCE']
    boot, = T888.rows_at(rows, 'BOOT')
    role_sha = boot['roleSetSHA']
    schedule = boot.get('sourceSchedule', 'V1')
    source_dispatches = boot.get('sourceDispatches', spec['dispatches'])
    need(t['family'] == family and t['roleSetSHA'] == role_sha == boot['roleSetSHA']
         and t['rootKind'] == 'SOURCE'
         and min(int(t[k]) for k in ('run', 'nonce', 'device', 'queue', 'plan', 'fence', 'fenceGeneration')) > 0,
         'ROOT_FAMILY')
    expected_dispatch = FINE_DISPATCH[family] if schedule == 'T913_SPLIT_V2' else spec['dispatches']
    need(source_dispatches == expected_dispatch
         and root['dispatches'] == expected_dispatch, 'ROOT_DISPATCH_COUNT')
    need(all(root[k] == t[k] for k in ('family', 'run', 'nonce', 'plan', 'fence', 'fenceGeneration', 'deadlineNs'))
         and root['accepted'] == (t['acceptedRoot'] is not None)
         and t['staging']['bytes'] == 1024, 'ROOT_RECEIPT')
    ns = lambda r, k: int(r[k])
    need(ns(t, 'deadlineNs') - ns(t, 'grantNs') == 250_000_000
         and 0 < ns(t, 'grantNs') <= ns(t, 'inputFrozenNs') <= ns(t, 'expectedReadyNs')
         <= ns(t, 'recordEnterNs') <= ns(t, 'recordExitNs') <= ns(t, 'submitEnterNs') <= ns(t, 'submitExitNs')
         <= ns(t, 'stopNs') and t['submitVkResult'] == 0, 'ROOT_TIMELINE_ORDER')
    clock = t['clock']
    need(0 < ns(clock, 'seq') and 0 < ns(clock, 'callbackNs') <= ns(clock, 'receiptNs') <= ns(clock, 'observedNs')
         and clock['ageNs'] == ns(clock, 'observedNs') - ns(clock, 'receiptNs'), 'ROOT_PULSE_CLOCK')
    polls = t['polls']
    need(len(polls) <= t['pollCount'] and bool(t['lastPoll']) == bool(t['pollCount'])
         and (not t['pollCount'] or polls and
              (len(polls) < t['pollCount'] or polls[-1] == t['lastPoll'])), 'ROOT_POLL_COUNT')
    previous = ns(t, 'submitExitNs')
    for poll in polls + ([t['lastPoll']] if len(polls) < t['pollCount'] else []):
        need(previous <= ns(poll, 'enterNs') <= ns(poll, 'exitNs') <= ns(t, 'stopNs')
             and 0 < ns(poll, 'timeoutNs') <= 2_000_000, 'ROOT_POLL_ORDER')
        previous = ns(poll, 'exitNs')
    accepted = t['acceptedRoot'] is not None
    if accepted:
        need(str(t['acceptedRoot']) == t['nonce'] and t['lastPoll']['vkResult'] == t['readbackFenceVkResult'] == 0
             and ns(t, 'stopNs') <= ns(t, 'deadlineNs')
             and 0 < ns(t, 'readbackEnterNs') <= ns(t, 'readbackExitNs') <= ns(t, 'gateEnterNs')
             <= ns(t, 'gateExitNs') <= ns(t, 'promoteEnterNs') <= ns(t, 'promoteExitNs'), 'ROOT_ACCEPTANCE')
    else:
        need(ns(t, 'promoteEnterNs') == ns(t, 'promoteExitNs') == 0, 'REJECTED_PROMOTED')
    evidence = [r for r in rows if r.get('stage') in ('RETIRE', 'DEVICE_DRAIN')]
    end = ns(t, 'stopNs')
    fence = t['lastPoll']['vkResult'] if t['lastPoll'] else 1
    retired = fence == 0
    for r in evidence:
        need(not retired and not accepted and r['family'] == family and str(r['nonce']) == t['nonce']
             and r['fence'] == t['fence'] and r['fenceGeneration'] == t['fenceGeneration']
             and r['acceptedRoot'] is None and end <= ns(r, 'enterNs') <= ns(r, 'exitNs'), 'RETIRE_FENCE_IDENTITY')
        end = ns(r, 'exitNs')
        if r['stage'] == 'DEVICE_DRAIN':
            need((r['device'], r['queue']) == (t['device'], t['queue']), 'RETIRE_DEVICE_QUEUE')
            fence = r['fenceVkResult']
            if fence == 0:
                need(r['idleVkResult'] == 0 and end <= ns(r, 'confirmEnterNs') <= ns(r, 'confirmExitNs'),
                     'RETIRE_DRAIN_CONFIRM')
                end = ns(r, 'confirmExitNs')
        else:
            need(r['run'] == t['run'], 'RETIRE_RUN')
            fence = r['vkResult']
        need(r['state'] == ({0: 'RETIRED', -4: 'DEVICE_LOST'}.get(fence, 'RETIRE_PENDING')), 'RETIRE_STATE')
        retired = fence == 0
    if accepted:
        retired = True
    queries = T888.rows_at(rows, 'ROOT_GPU_TIMESTAMPS')
    need(len(queries) == 1 or not retired and not queries, 'QUERY_ROW_MISSING')
    availability = None
    detail_points = None
    if queries:
        q = queries[0]
        need((not evidence or rows.index(evidence[-1]) < rows.index(q))
             and all(str(q[k]) == str(t[k]) for k in
                     ('family', 'run', 'nonce', 'plan', 'device', 'queue', 'fence', 'fenceGeneration'))
             and q['acceptedRoot'] == t['acceptedRoot'] and q['fenceVkResult'] == fence, 'QUERY_ROOT_SCOPE')
        expected = ('ACCEPTED_FENCE' if accepted else 'LATE_RETIRED_DIAGNOSTIC' if retired and evidence
                    else 'REJECTED_ROOT_DIAGNOSTIC' if retired else 'DEVICE_LOST' if fence == -4 else 'RETIRE_PENDING')
        need(q['state'] == expected, 'QUERY_STATE')
        points = q['points']
        enabled = q['timestampRequested'] and q['queryCreateVkResult'] == 0 and int(q['queryPool']) > 0
        readable = retired or accepted
        readable = (retired or accepted) and enabled
        bits, period = q['validBits'], q['periodNs']
        detail = bool(schedule == 'T913_SPLIT_V2') or len(q['spans']) > len(spec['spans']) + 2
        expected_points = (2 * expected_dispatch + 3) if detail else len(spec['spans']) + 3
        need([p['i'] for p in points] == list(range(len(q['spans']) + 1)), 'QUERY_LAYOUT')
        need(len(points) == expected_points, 'QUERY_POINT_COUNT')
        need((not enabled or 1 <= bits <= 64 and math.isfinite(period) and period > 0), 'QUERY_LAYOUT')
        for p in points:
            need((p['tick'] is not None) == p['available']
                 and (not p['available'] or readable and p['vkResult'] == 0 and int(p['tick']) >= 0),
                 'QUERY_AVAILABILITY')
        names = [s['name'] for s in q['spans']]
        if detail:
            need(names[0] == 'UPLOAD' and names[-1] == 'TAIL_COPY'
                 and all(n.startswith('SLOT_') for n in names[1:-1])
                 and len(names) == 2 * expected_dispatch + 2, 'QUERY_DETAIL_SPANS')
        else:
            need(names == ['UPLOAD', *spec['spans'], 'TAIL_COPY'], 'QUERY_FAMILY_SPANS')
        for i, s in enumerate(q['spans']):
            a, b = points[i:i + 2]
            if a['available'] and b['available']:
                duration = ((int(b['tick']) - int(a['tick'])) % (1 << bits)) * period
                need(s['ns'] is not None and math.isclose(s['ns'], duration, rel_tol=2e-5, abs_tol=1),
                     'QUERY_SPAN')
            else:
                need(s['ns'] is None, 'QUERY_MISSING_SPAN')
        availability = sum(p['available'] for p in points)
        detail_points = dict(spans=len(names), available=availability)
    return dict(family=family, accepted=accepted, firstStop=t['firstStop'],
                retirement='RETIRED' if retired else 'DEVICE_LOST' if fence == -4 else 'RETIREMENT_UNPROVEN',
                pointsAvailable=availability, roleSetSHA=role_sha, dispatches=expected_dispatch,
                detail=detail_points)


def main():
    p = argparse.ArgumentParser(description=__doc__)
    p.add_argument('family', choices=tuple(FAMILIES))
    p.add_argument('jsonl', type=Path)
    args = p.parse_args()
    lines = args.jsonl.read_bytes().splitlines()
    if lines and lines[-1] == f'{args.family} ROOT_NOT_ACCEPTED'.encode():
        lines = lines[:-1]
    rows = [json.loads(line) for line in lines]
    try:
        result = sand_t914(rows, args.family)
    except (ValueError, KeyError, IndexError, OSError) as exc:
        print(json.dumps(dict(state='INVALID_OR_MISSING_EVIDENCE', reason=str(exc))))
        return 2
    print(json.dumps(result, indent=2))
    return 0


if __name__ == '__main__':
    sys.exit(main())
