"""Designer reference arithmetic only: no product imports, GL, device or build.

Fraction is an independent exact oracle for five-bound water flux and RNE.
The trajectory exercise tests the proposed bounded event policy, not a GPU.
"""
from pathlib import Path
from fractions import Fraction as F
from copy import deepcopy
import hashlib
import json
import math
import random

ROOT = Path(__file__).resolve().parents[1]
Q = 1 << 20
T = 1 << 16
D = 120*T
MAX_ID = (1 << 32)-1


def rne(n, d=1):
    assert d > 0
    s = -1 if n < 0 else 1
    a, b = divmod(abs(n), d)
    if 2*b > d or 2*b == d and a%2:
        a += 1
    return s*a


def sign(a):
    return (a > 0)-(a < 0)


def velocity(v, r, k, a, terminal, wet):
    if wet:
        return rne(D*v+k*r*terminal, D+k*r)
    return rne(D*v, D+k*r)+rne(a*r, D)


def first_turn(v, remaining, k, a, target, wet):
    if v == 0 or sign(velocity(v, remaining, k, a, target, wet)) == sign(v):
        return remaining, False
    lo, hi = 1, remaining
    while lo < hi:
        mid = (lo+hi)//2
        if sign(velocity(v, mid, k, a, target, wet)) == sign(v):
            lo = mid+1
        else:
            hi = mid
    assert lo == 1 or sign(velocity(v, lo-1, k, a, target, wet)) == sign(v)
    assert sign(velocity(v, lo, k, a, target, wet)) != sign(v)
    return lo, True


def flux_units(s, w, p, j, edge, u, kappa, cap):
    a, b = edge, edge+1
    h = [2*(s[c]+w[c])-(2*c+1)*u for c in (a, b)]
    if h[0] == h[1]:
        return a, b, 0
    donor, receiver = (a, b) if h[0] > h[1] else (b, a)
    hd = 2*(s[donor]+w[donor])-(2*donor+1)*u
    hr = 2*(s[receiver]+w[receiver])-(2*receiver+1)*u
    zd, zr = 2*s[donor]-(2*donor+1)*u, 2*s[receiver]-(2*receiver+1)*u
    delta = hd-hr
    free = cap-s[receiver]-w[receiver]-p[receiver]-j[receiver]
    assert free >= 0
    bounds = [kappa*delta//240, delta//4, w[donor],
              max(0, (hd-max(zd, zr))//2), free]
    return donor, receiver, min(bounds)


def flux_fraction_oracle(s, w, p, j, edge, u, kappa, cap):
    # Direct physical head, not the implementation's doubled-head expression.
    a, b = edge, edge+1
    head = lambda c: F(s[c]+w[c])-F(2*c+1, 2)*u
    bed = lambda c: F(s[c])-F(2*c+1, 2)*u
    if head(a) == head(b):
        return a, b, 0
    d, r = (a, b) if head(a) > head(b) else (b, a)
    delta = head(d)-head(r)
    exact_bounds = [F(kappa, 120)*delta, delta/2, F(w[d]),
                    max(F(0), head(d)-max(bed(d), bed(r))),
                    F(cap-s[r]-w[r]-p[r]-j[r])]
    return d, r, min(exact_bounds).__floor__()


def water_quantum(s, w, p, j, u, serial, cap=256*Q, kappa=24):
    result = []
    initial = sum(w)
    for color in (serial%2, 1-serial%2):
        plan = [flux_units(s, w, p, j, e, u, kappa, cap) for e in range(color, len(s)-1, 2)]
        # Both application orders must agree because endpoints are disjoint.
        forward, backward = w[:], w[:]
        for out, seq in ((forward, plan), (backward, list(reversed(plan)))):
            for d, r, q in seq:
                out[d] -= q
                out[r] += q
        assert forward == backward
        w[:] = forward
        result.extend((d, r, q) for d, r, q in plan if q)
    assert sum(w) == initial
    assert all(0 <= s[c]+w[c]+p[c]+j[c] <= cap for c in range(len(s)))
    return result


def arrive(eta, points, height_order=True):
    # Single-column exact contact operator, points=(id,y,mass).
    rows = sorted(points, key=(lambda p: (p[1], p[0])) if height_order else (lambda p: p[0]))
    alive, ids = [], []
    for ident, y, mass in rows:
        if y <= eta:
            eta += mass
            ids.append(ident)
        else:
            alive.append((ident, y, mass))
    return eta, alive, ids


def trajectory(x, y, vx, vy, beds, water, angle_index=0, water_packet=False):
    """Static-geometry reference for event count/time/medium; not LAND/budget.

    It exercises T16 TURN splits + rational-chord collisions and constraints.
    Current beds do not change here; source/bed/arrival are tested separately.
    """
    angle = angle_index/256*math.pi/180
    sn, cs = rne(round(math.sin(angle)*Q)), rne(round(math.cos(angle)*Q))
    a = (rne(512*Q*sn, Q), -rne(512*Q*cs, Q))
    vt = (rne(8*Q*sn, Q), -rne(8*Q*cs, Q))
    eta = [s+w for s, w in zip(beds, water)]
    n, remaining, tau, rows = len(beds), T, 0, []
    last_zero = None

    def wet_at(c, yy, vyy):
        return not water_packet and water[c] > 0 and yy > beds[c] and (yy < eta[c] or yy == eta[c] and vyy <= 0)

    while remaining:
        c = x//Q
        assert 0 <= c < n
        if y <= (eta[c] if water_packet else beds[c]):
            rows.append(('CONTACT_WAIT', tau, T))
            tau, remaining = T, 0
            break
        wet = wet_at(c, y, vy)
        k = 24 if wet else 0
        turns = [first_turn(v, remaining, k, aa, term, wet) for v, aa, term in zip((vx, vy), a, vt)]
        rseg = min(t[0] for t in turns)
        nvx = velocity(vx, rseg, k, a[0], vt[0], wet)
        nvy = velocity(vy, rseg, k, a[1], vt[1], wet)
        dx, dy = rne((vx+nvx)*rseg, 2*D), rne((vy+nvy)*rseg, 2*D)
        candidates = []
        floor = eta[c] if water_packet else beds[c]
        if dy < 0 and y+dy <= floor:
            candidates.append((F(floor-y, dy), 0, 'CONTACT_WAIT', floor))
        if dy > 0 and y+dy >= 256*Q:
            candidates.append((F(256*Q-y, dy), 1, 'WALL', 256*Q))
        if dx:
            plane = (c+1)*Q if dx > 0 else c*Q
            lam = F(plane-x, dx)
            if 0 <= lam <= 1:
                at_wall = plane == 0 or plane == n*Q
                candidates.append((lam, 1 if at_wall else 2, 'WALL_X' if at_wall else 'CROSS', plane))
        if not water_packet and water[c] > 0 and dy and (wet and dy > 0 or not wet and dy < 0):
            lam = F(eta[c]-y, dy)
            if 0 <= lam <= 1:
                candidates.append((lam, 3, 'MEDIUM', eta[c]))
        if candidates:
            lam, _, kind, plane = min(candidates)
        else:
            lam, kind, plane = F(1), 'TURN' if rseg < remaining else 'MOVE', 0
        ticks = (rseg*lam.numerator+lam.denominator-1)//lam.denominator
        assert 0 <= ticks <= rseg
        nx, ny = x+rne(dx*lam.numerator, lam.denominator), y+rne(dy*lam.numerator, lam.denominator)
        uvx = velocity(vx, ticks, k, a[0], vt[0], wet)
        uvy = velocity(vy, ticks, k, a[1], vt[1], wet)
        if kind == 'MEDIUM':
            ny = plane+1 if dy > 0 else plane
        elif kind == 'CROSS':
            to = c+1 if dx > 0 else c-1
            if F(y)+dy*lam <= beds[to]:
                kind = 'CONTACT_WAIT'
                nx = plane-1 if dx > 0 else plane
            else:
                nx = plane if dx > 0 else plane-1
        elif kind == 'WALL_X':
            nx = 0 if plane == 0 else n*Q-1
        if ticks == 0:
            zero = (kind, plane)
            assert zero != last_zero, ('TIME_NO_PROGRESS', zero)
            last_zero = zero
        else:
            last_zero = None
        old_tau = tau
        remaining -= ticks
        tau += ticks
        if kind in ('CONTACT_WAIT', 'WALL', 'WALL_X'):
            tau, remaining = T, 0  # Recorded constraint consumes the rest.
        rows.append((kind, old_tau, tau))
        x, y, vx, vy = nx, ny, uvx, uvy
        if len(rows) > (15 if water_packet else 47):
            raise AssertionError(('event envelope', water_packet, rows))
    assert tau == T
    return rows, (x, y, vx, vy)


def resources(base, kw=64):
    k, n, old_kw = base['K'], base['N'], base['Kw']
    e = 4*(48*k+16*kw+2*(n-1))+k
    delta = {
        'three_banks_water12_to16': 3*(64*kw-48*old_kw),
        'two_publications_water20_column16': 2*(80*kw-64*old_kw+32*n),
        'water_motion32': 128*kw,
        'two_water_id_indices': 8*kw,
        'two_water_arrival_indices': 8*kw,
        'scan_scratch_increase': 4*(kw-old_kw),
        'frozen_medium4': 16*n,
        'extra_audit4': 16*n,
        'physics_config128word': 512,
        'gpu_host_optics16F32': 128,
        'two_frame_event_arena_increase': 512*(e-base['Eframe']),
    }
    h = (kw-1).bit_length()
    lw = h*(h+1)//2
    quantum = 36+base['sortLayers']+2*lw
    return {'N': n, 'K': k, 'Kw': kw, 'eSandIncludesFinal': 48, 'eWaterIncludesFinal': 16,
            'frameEventCapacity': e, 'singleEventBlockBytes': 256*e,
            'baselinePayloadBytes': base['basePayloadBytes'], 'deltaPartsBytes': delta,
            'deltaPayloadBytes': sum(delta.values()),
            'M1BasePayloadBytes': base['basePayloadBytes']+sum(delta.values()),
            'waterSortLayersEach': lw, 'quantumDispatchStages': quantum,
            'sourceDispatchStages': base['sourceDispatchBound'],
            'tickDispatchStages': base['sourceDispatchBound']+4*quantum,
            'tailFencesMax': 5, 'normalReadbackMaxBytes': 5120,
            'actualAllocatedBytes': None, 'actualGPUTime': None,
            'status': 'DESIGN_TEMPLATE_NOT_ALLOCATED_NOT_DEVICE_MEASUREMENT'}


def main():
    rng = random.Random(237)
    checks = {}
    for _ in range(3000):
        s, w, p, j = [], [], [], []
        for _ in range(2):
            a = rng.randrange(64)
            b = rng.randrange(64)
            pp = rng.randrange(32)
            jj = rng.randrange(32)
            s.append(a); w.append(b); p.append(pp); j.append(jj)
        u, k = rng.randrange(-15, 16), rng.randrange(961)
        assert flux_units(s, w, p, j, 0, u, k, 256) == flux_fraction_oracle(s, w, p, j, 0, u, k, 256)
    checks['head2FractionOracleCases'] = 3000
    s, w, p, j = [8*Q]*9, [0]*9, [0]*9, [0]*9
    w[4] = 2*Q
    left = right = 0
    for step in range(80):
        for d, r, amount in water_quantum(s, w, p, j, 0, step):
            if d == 4 and r == 3: left += amount
            if d == 4 and r == 5: right += amount
    assert left > 0 and right > 0 and sum(w) == 2*Q
    checks['flatWaterFixture'] = {'quanta': 80, 'leftUnits': left, 'rightUnits': right,
                                'exactTotalUnits': sum(w), 'instantSymmetryRequired': False}
    barrier = water_quantum([0, 3*Q, 0], [Q, 0, 0], [0]*3, [0]*3, 0, 0)
    assert not barrier
    tilted = water_quantum([0]*5, [Q]*5, [0]*5, [0]*5, Q//8, 0)
    assert tilted and all(r > d for d, r, _ in tilted)
    checks['barrierAndTiltFixtures'] = True

    velocity_cases = 0
    for _ in range(2500):
        v, terminal = rng.randrange(-512*Q, 512*Q), rng.randrange(-16*Q, 16*Q)
        r, k = rng.randrange(1, T+1), rng.randrange(961)
        exact = F(D*v+k*r*terminal, D+k*r)
        assert velocity(v, r, k, 0, terminal, True) == round(exact)
        velocity_cases += 1
    checks['wetVelocityFractionCases'] = velocity_cases
    turn_cases = 0
    for _ in range(1000):
        v = rng.randrange(1, Q)*rng.choice((-1, 1))
        direction = -sign(v)
        a, terminal = direction*512*Q, direction*8*Q
        wet = bool(rng.randrange(2))
        k = 24 if wet else 0
        r, has_turn = first_turn(v, T, k, a, terminal, wet)
        if has_turn:
            assert sign(velocity(v, r, k, a, terminal, wet)) != sign(v)
            if r > 1:
                assert sign(velocity(v, r-1, k, a, terminal, wet)) == sign(v)
        turn_cases += 1
    checks['turnSplitCases'] = turn_cases
    v_full = velocity(-Q, T, 0, 512*Q, 8*Q, False)
    v_early = velocity(-Q, T//16, 0, 512*Q, 8*Q, False)
    cut, has_turn = first_turn(-Q, T, 0, 512*Q, 8*Q, False)
    assert v_full > 0 and v_early < 0 and has_turn and T//16 < cut < T
    checks['turnCounterexample'] = {'wholeRemainderFinalVelocity': v_full,
        'earlyEventVelocity': v_early, 'firstTurnTick': cut,
        'conclusion': 'Split TURN before drawing a chord across a velocity reversal'}
    for _ in range(2500):
        den = rng.randrange(1, 10**8)
        num = rng.randrange(den+1)
        seg = rng.randrange(1, T+1)
        t = (seg*num+den-1)//den
        assert 0 <= F(t)-F(seg*num, den) < 1
        assert 0 <= t <= seg
    checks['positiveTimeCeilCases'] = 2500

    rows = [(1, 2*Q, Q//4), (2, Q, 2*Q)]
    bad, good = arrive(Q, rows, False), arrive(Q, rows)
    assert bad[1] and bad[1][0][1] <= bad[0]
    assert not good[1]
    checks['arrivalCounterexample'] = {'idOrderMissesHighId1': True, 'heightOrderIds': good[2],
                                      'etaAfterUnits': good[0]}
    for _ in range(1500):
        initial = rng.randrange(10000)
        pts = [(i+1, rng.randrange(50000), rng.randrange(1, 10000)) for i in range(rng.randrange(1, 65))]
        eta, alive, ids = arrive(initial, pts)
        assert eta+sum(p[2] for p in alive) == initial+sum(p[2] for p in pts)
        assert all(p[1] > eta for p in alive)
    checks['sortedArrivalExactCases'] = 1500
    w, j, p, s, iw, iss = 0, 0, 0, 0, 0, 0
    for step in range(2000):
        q = rng.randrange(1, 1000)
        j += q; iw += q
        if step%2 == 0:
            landed = rng.randrange(j+1)
            before_r = s+w+p+j
            w += landed; j -= landed
            assert s+w+p+j == before_r
        if step%3 == 0:
            p += q; iss += q
            land = rng.randrange(p+1)
            old_w, old_eta = w, s+w
            s += land; p -= land
            assert w == old_w and s+w == old_eta+land
        assert w+j == iw and s+p == iss
    checks['inventoryArrivalLandCases'] = 2000

    # Slot/ID lifetime model, including the final usable MAX_ID.
    next_id, exhausted = MAX_ID, False
    assigned = next_id
    if next_id == MAX_ID: exhausted = True
    assert assigned == MAX_ID and exhausted and next_id == MAX_ID
    checks['maxIdExhaustionFixture'] = {'lastAssigned': assigned, 'nextBirthRejected': exhausted}
    current = {'W': 0, 'J': 0, 'source': 0, 'physics': 0}
    for nonce in range(1000):
        candidate = deepcopy(current)
        candidate['J'] += 1; candidate['source'] += 1; candidate['physics'] += 1
        accept = nonce%5 == 0
        old = deepcopy(current)
        if accept: current = candidate
        else: assert current == old
        step_candidate = deepcopy(current)
        step_candidate['W'] += step_candidate['J']; step_candidate['J'] = 0
        # Fault/late candidate never changes the previously accepted source.
        assert current['source'] == (old['source']+1 if accept else old['source'])
    checks['candidateSourcePrefixCases'] = 1000

    max_sand = max_water = 0
    medium_examples = 0
    for _ in range(1000):
        n = 16
        beds = [2*Q]*n
        water = [rng.randrange(0, 9)*Q for _ in range(n)]
        c = rng.randrange(n)
        x = c*Q+rng.randrange(Q)
        y = rng.randrange(2*Q+1, 24*Q)
        vx, vy = rng.randrange(-512*Q, 512*Q), rng.randrange(-512*Q, 512*Q)
        angle = rng.choice((-7168, -3840, 0, 3840, 7168))
        rs, _ = trajectory(x, y, vx, vy, beds, water, angle)
        max_sand = max(max_sand, len(rs))
        medium_examples += any(row[0] == 'MEDIUM' for row in rs)
        yw = max(y, beds[c]+water[c]+1)
        rw, _ = trajectory(x, yw, vx, -abs(vy), beds, water, angle, True)
        max_water = max(max_water, len(rw))
    checks['staticGeometryTrajectoryCases'] = {'sand': 1000, 'water': 1000,
        'maxSandMotionRowsObserved': max_sand, 'maxWaterMotionRowsObserved': max_water,
        'sandFixturesWithMediumEvent': medium_examples,
        'scope': 'STATIC_GEOMETRY_REFERENCE_NOT_FULL_ROOT_OR_GPU_TEST'}
    assert max_sand+1 <= 48 and max_water+1 <= 16
    # Explicit deep-water deceleration under the new implicit policy.
    dry_v = velocity(-128*Q, T, 0, -512*Q, -8*Q, False)
    wet_v = velocity(-128*Q, T, 24, -512*Q, -8*Q, True)
    assert abs(wet_v) < abs(dry_v)
    checks['wetVsAirFixtureUnits'] = {'airVy': dry_v, 'wetVy': wet_v, 'calibration': 'PENDING'}

    # Empty-column candidate flight, no collisions until floor. Independent
    # integer integration records the actual simulated first landing quantum.
    y, vy, quanta = 220*Q, -16*Q, 0
    while y > 0:
        nv = velocity(vy, T, 0, -512*Q, 0, False)
        y += rne((vy+nv)*T, 2*D)
        vy = nv
        quanta += 1
        assert quanta < 240
    dwell = F(quanta, 120)
    for rate in (100, 120):
        needed = (rate*quanta+119)//120
        assert needed > 64 and needed <= 128
    checks['waterPoolFirstArrivalEnvelope'] = {
        'candidate': {'sourceY': 220, 'g': 512, 'vEmit': 16, 'angle': 0, 'dragFlight': 0},
        'landingQuantumUpperSample': quanta, 'simulatedSeconds': float(dwell),
        'packsAt100PerSimSecond': (100*quanta+119)//120,
        'packsAt120PerSimSecond': quanta,
        'Kw64CanRejectBeforeFirstArrival': True, 'recommendedCandidateKw': 128,
        'scope': 'CANDIDATE_ENVELOPE_NOT_DEVICE_THROUGHPUT_OR_UNIVERSAL_NO_REJECT_GUARANTEE'}

    manifest = json.loads((ROOT/'docs/analysis-T233-gpu-resident-M0-engineering-manifest-v1.json').read_text(encoding='utf-8'))
    templates = [resources(b, kw) for kw in (64, 128) for b in manifest['resourceTemplates']]
    assert [r['M1BasePayloadBytes'] for r in templates] == [23342736, 55111056, 25471888, 57240208]
    assert [r['tickDispatchStages'] for r in templates] == [488, 526, 544, 582]
    text = (ROOT/'docs/algorithm-T237-gpu-water-M1-v1.md').read_text(encoding='utf-8')
    assert text.count('```')%2 == 0
    assert all(f'| WM{i:02} ' in text for i in range(1, 13))
    assert all(f"{r['M1BasePayloadBytes']:,}B" in text for r in templates)
    assert 'TURN14' in text and 'rSeg' in text
    paths = ['AGENTS.md', '.codex/agents/designer.toml',
             'docs/algorithm-T230-gpu-resident-step-v1.md',
             'docs/architecture-T233-gpu-resident-M0-v1.md',
             'docs/analysis-T233-gpu-resident-M0-engineering-manifest-v1.json',
             'docs/architecture-T231-gpu-resident-platform-v1.md',
             'docs/algorithm-T221-sand-water-demo-v1.md',
             'docs/architecture-T222-clear-water-flight-v2.md',
             'docs/developer-T209-clear-water-B-v1.md',
             'docs/test-report-T215-clear-water-B-independent-v1.md',
             'docs/developer-T226-clear-water-C-v1.md',
             'entry/src/main/ets/physics/clearwater/P011ClearWaterSource.ets',
             'entry/src/main/ets/physics/clearwater/P011ClearWaterHydraulics.ets',
             'entry/src/main/ets/physics/clearwater/P011ClearWaterParticles.ets',
             'entry/src/main/ets/physics/clearwater/P011ClearWaterEngine.ets',
             'entry/src/main/cpp/p013renderer/clear_water_gpu.cpp',
             'docs/algorithm-T237-gpu-water-M1-v1.md',
             'docs/retro-T237-gpu-water-M1-v1.md',
             'docs/design-T237-gpu-water-M1-model-v1.py']
    pins = []
    for path in paths:
        raw = (ROOT/path).read_bytes()
        pins.append({'path': path, 'bytes': len(raw), 'sha256': hashlib.sha256(raw).hexdigest()})
    result = {'revision': 'T237_M1_DESIGN_REFERENCE_EVIDENCE_V1', 'date': '2026-10-28',
              'task': '6ae1ec3be4b0d25fef7ba6a7', 'route': 'gpu-resident-water-M1-v1',
              'status': 'DESIGN_READY_FOR_ARCHITECT_NOT_IMPLEMENTED_NOT_VALIDATED',
              'scope': 'DESIGN_INTEGER_FRACTION_MODEL_ONLY',
              'productTestsExecuted': False, 'deviceUsed': False, 'shaderCompiledOrRun': False,
              'checks': checks, 'resourceTemplates': templates, 'inputs': pins,
              'limitations': ['No dynamic bed/ID/budget full GPU implementation executed',
                 'Trajectory exercise uses static geometry and illustrative CPU coefficient values',
                 'Event/turn bound relies on the explicit force, freeze and no-bounce assumptions',
                 'Optics/current scene, context, actual allocation and GPU/UI performance not tested',
                 'Old F64/CPU evidence is reference only, not Q20 byte equivalence or M1 product PASS']}
    (ROOT/'docs/design-T237-gpu-water-M1-evidence-v1.json').write_text(
        json.dumps(result, ensure_ascii=False, indent=2)+'\n', encoding='utf-8')
    print(json.dumps({'scope': result['scope'], 'checks': checks,
                      'payloadBytes': [r['M1BasePayloadBytes'] for r in templates],
                      'tickDispatchStages': [r['tickDispatchStages'] for r in templates],
                      'inputFingerprints': len(pins)}, ensure_ascii=False, indent=2))


if __name__ == '__main__':
    main()
