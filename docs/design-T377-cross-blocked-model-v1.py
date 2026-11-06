"""Read existing T369 evidence and check T377 design arithmetic offline."""
from collections import defaultdict
from fractions import Fraction
from pathlib import Path
import hashlib
import json
import math
import random
import struct

ROOT = Path(__file__).resolve().parents[1]
EPS_X = 1e-9
CAPACITY = 256.0


def raw64(low, high):
    return struct.unpack('<d', struct.pack('<II', int(low), int(high)))[0]


def words(row, name):
    return row[name + 'Lo'], row[name + 'Hi']


def value(row, name):
    return raw64(*words(row, name))


def dry_vertical(y, vy, theta, dt):
    vy_free = vy - 9.8 * math.cos(theta) * dt
    return min(CAPACITY, y + vy_free * dt), vy_free


def read_window(name):
    path = ROOT / 'test/reports/t369' / name
    cuts = json.loads(path.read_bytes())['cuts']
    assert cuts['rowWidth'] == 72 and len(cuts['fields']) == 62
    assert cuts['roles'][1] == 'AUTHORITATIVE' and cuts['zeroGrant'][2] == 'BUDGET'
    stages = cuts['stages']
    ids = cuts['ids']
    assert ids == [86, 87, 88]
    latest = {r['id']: r for r in (
        dict(zip(cuts['fields'], cuts['lastAnchor'][i:i+72]))
        for i in range(0, len(cuts['lastAnchor']), 72)
    )}
    rows = defaultdict(dict)
    for raw in cuts['rows']:
        row = dict(zip(cuts['fields'], raw))
        if row['id'] in ids:
            rows[(row['attempt'], row['substep'], row['id'])][stages[row['stage']]] = row
    counts = defaultdict(int)
    slot_counts = defaultdict(int)
    max_delta = 0.
    for key, phases in rows.items():
        if not {'predicted', 'afterApply'} <= phases.keys():
            continue
        pred, after = phases['predicted'], phases['afterApply']
        if pred['role'] != 1 or pred['requestKind'] != 3 or pred['zeroGrant'] != 2:
            continue
        assert after['role'] == 1 and pred['disposition'] == 6
        assert pred['requested'] == pred['remainder'] == .1 and pred['granted'] == 0
        assert pred['budgetClipped'] == .1
        end = latest[key[2]]
        assert words(after, 'y') == words(end, 'y')
        assert words(after, 'vy') == words(end, 'vy')
        for stage in ('prepublication', 'bank', 'committedSubstep'):
            assert words(phases[stage], 'y') == words(after, 'y')
            assert words(phases[stage], 'vy') == words(after, 'vy')
        # A slot sample exists only at the last substep in each captured frame.
        if 'slot' in phases:
            assert words(phases['slot'], 'y') == words(after, 'y')
            assert words(phases['slot'], 'vy') == words(after, 'vy')
            slot_counts[key[2]] += 1
        expected = dry_vertical(value(after, 'y'), value(after, 'vy'),
                                pred['acceptedTiltRad'], pred['substepDt'])
        actual = value(pred, 'y'), value(pred, 'vy')
        max_delta = max(max_delta, *(abs(a-b) for a, b in zip(expected, actual)))
        assert max_delta <= 2e-14 and actual[0] < value(after, 'y')
        counts[key[2]] += 1
    assert set(counts) == set(ids) and all(n == 8 for n in counts.values())
    assert set(slot_counts) == set(ids) and all(n == 4 for n in slot_counts.values())
    a = latest[86]
    return {
        'sha256': hashlib.sha256(path.read_bytes()).hexdigest(),
        'run': cuts['cohortRun'],
        'epoch': cuts['cohortEpoch'],
        'cohort_admission': cuts['admissionTransactionId'],
        'commit': a['commit'],
        'elapsed': a['acceptedElapsed'],
        'after_y_vy_words': [words(a, 'y'), words(a, 'vy')],
        'after_y_vy': [value(a, 'y'), value(a, 'vy')],
        'authority_pairs_per_id': dict(counts),
        'slot_samples_per_id': dict(slot_counts),
        'candidate_max_abs_error': max_delta,
    }


def blocked(c, target, y0, y_free, vy_free, s_row, s_final, eps=1e-6):
    assert 0 <= c < 128 and 0 <= target < 128 and abs(c-target) == 1
    assert 0 <= s_row <= CAPACITY and 0 <= s_final <= CAPACITY
    wall = c + 1 - EPS_X if target > c else c + EPS_X
    assert math.floor(wall) == c
    endpoint = min(y0, y_free)
    if endpoint <= s_row:
        return wall, min(CAPACITY, s_final+eps), 0., 0., 2, 'ROW'
    if endpoint <= s_final:
        return wall, min(CAPACITY, s_final+eps), 0., 0., 2, 'TAIL'
    return wall, y_free, 0., vy_free, 1, 'AIR'


def exact_cross(mass, source, target, total, scale, free):
    if free < mass:
        return 0, 0, 0, 'CAPACITY'
    local = min(mass, source, target)
    reserved = min(local, local * scale, total)
    if reserved < mass:
        return reserved, reserved, 0, 'BUDGET'
    return reserved, 0, mass, 'CROSS'


def main():
    windows = [read_window(name) for name in ('v2-budget-t369.json', 'v2-budget-t369-final.json')]
    assert windows[0]['run'] == windows[1]['run']
    assert windows[0]['after_y_vy_words'] == windows[1]['after_y_vy_words']
    assert abs(windows[1]['elapsed']-windows[0]['elapsed']-21.1666666666667) < 1e-10
    y0, vy0 = windows[-1]['after_y_vy']
    y_free, vy_free = dry_vertical(y0, vy0, math.pi/6, 1/60)
    assert blocked(44, 45, y0, y_free, vy_free, 0., 0.)[1:5] == (y_free, 0., vy_free, 1)
    assert max(y_free, 48+1e-6) > y0 > y_free
    assert blocked(44, 45, 3., 1., -2., 2., .5)[1:] == (.500001, 0., 0., 2, 'ROW')
    assert blocked(44, 43, 3., 1., -2., .5, 2.)[-1] == 'TAIL'
    assert blocked(44, 43, 255., 256., 1., 256., 256.)[1] == 256.
    assert blocked(44, 43, 1., 2., 1., 1., 0.)[-1] == 'ROW'
    rng = random.Random(377)
    for _ in range(3000):
        c = rng.randrange(128)
        target = rng.choice([i for i in (c-1, c+1) if 0 <= i < 128])
        y = rng.uniform(0, CAPACITY)
        row = rng.uniform(0, CAPACITY)
        final = rng.uniform(0, CAPACITY)
        vy = rng.uniform(-256, 256)
        free_y = min(CAPACITY, y + vy/60)
        x, new_y, vx, new_vy, state, stage = blocked(c, target, y, free_y, vy, row, final)
        assert math.floor(x) == c and vx == 0 and final <= new_y <= CAPACITY
        assert (state == 2) == (min(y, free_y) <= max(row, final))
        assert new_vy == (0 if state == 2 else vy)
    m = Fraction(1, 10)
    arms = [
        ('zero_source', 0, m, m, 1, m, 'BUDGET'),
        ('zero_target', m, 0, m, 1, m, 'BUDGET'),
        ('zero_total', m, m, 0, 1, m, 'BUDGET'),
        ('scale_partial', m, m, m, Fraction(1, 2), m, 'BUDGET'),
        ('reserve_partial', m/2, m, m, 1, m, 'BUDGET'),
        ('capacity', m, m, m, 1, m/2, 'CAPACITY'),
        ('full', m, m, m, 1, m, 'CROSS'),
    ]
    ledger = {}
    for label, source, target, total, scale, free, expected in arms:
        reserve, refund, grant, reason = exact_cross(m, source, target, total, scale, free)
        assert reason == expected and reserve-refund == grant
        assert all(x-reserve+refund == x-grant for x in (source, target, total))
        ledger[label] = [str(x) for x in (reserve, refund, grant, reason)]
    for grant in (Fraction(0), m/4, m):
        stock = Fraction(5)
        assert stock+grant+m-grant == stock+m
        assert ((m-grant) == 0) == (grant == m)
    # Demonstrate that successful F64 refund need not be a bitwise undo.
    non_inverse = next((u, a) for u in [.1, .2, .3, .4]
                       for a in [.1, .2, .3, .4] if (u+a)-a != u)
    assert (non_inverse[0]+non_inverse[1])-non_inverse[1] != non_inverse[0]
    # A counterfactual only. Fixed flat bed and every would-be crossing blocked.
    y, vy, n = y0, vy0, 0
    while y > 0 and n < 1000:
        y, vy = dry_vertical(y, vy, math.pi/6, 1/60)
        n += 1
    assert n < 1000
    inputs = [
        'docs/architecture-T353-post-up-freeze-attribution-v1.md',
        'docs/architecture-P011-dual-field-particle-v1.md',
        'docs/test-report-T369-stop-cohort-device-v1.md',
        'entry/src/main/ets/physics/P011ParticleHot.ets',
        'entry/src/main/ets/physics/P011ParticleM2Sink.ets',
        'entry/src/main/ets/physics/P011ParticleHotBudget.ets',
        'entry/src/main/ets/physics/P011ParticleM2Field.ets',
        'entry/src/main/ets/physics/P011ParticleM2Projection.ets',
        'entry/src/main/ets/physics/WaterSandField.ets',
    ]
    result = {
        'status': 'DESIGN_MODEL_CHECKED',
        'scope': 'Existing device archive read-only plus mathematical fixtures. Product and rollback not executed.',
        'windows': windows, 'fixture_candidate_y_vy': [y_free, vy_free],
        'randomized_geometry_cases': 3000, 'exact_ticket_arms': ledger, 'exact_land_cases': 3,
        'f64_refund_non_inverse_used_and_amount': non_inverse,
        'flat_bed_counterfactual_substeps_to_contact': n,
        'inputs_sha256_at_run': {p: hashlib.sha256((ROOT / p).read_bytes()).hexdigest()
                                 for p in inputs},
    }
    (ROOT/'docs/design-T377-cross-blocked-evidence-v1.json').write_text(
        json.dumps(result, indent=2)+'\n', encoding='utf-8')
    print(json.dumps({k:v for k,v in result.items() if k != 'inputs_sha256_at_run'}, indent=2))


if __name__ == '__main__':
    main()
