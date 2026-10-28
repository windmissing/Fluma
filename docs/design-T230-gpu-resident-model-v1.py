"""Designer-only integer/spec arithmetic for T230; no product/GL imports.

References are Python integer, Fraction and Decimal. This is not a tester
report, not a GLSL implementation, and not an end-to-end simulator.
"""
from pathlib import Path
from fractions import Fraction
from decimal import Decimal, localcontext, ROUND_HALF_EVEN
from bisect import bisect_left
import hashlib
import json
import math
import random
import struct

ROOT = Path(__file__).resolve().parents[1]
Q = 1 << 20
H = 605394
MASK = (1 << 32)-1
PI = Decimal('3.1415926535897932384626433832795028841971693993751058209749445923')


def ceildiv(a, b):
    assert b > 0
    return -((-a)//b)


def interval(d, k, h):
    if k == 0:
        return (-10**30, 10**30) if abs(d) <= h else (1, 0)
    if k < 0:
        return interval(-d, -k, h)
    return ceildiv(-h-d, k), (h-d)//k


def max_grant(edges, req, h):
    lo, hi = 0, req
    for d, k in edges:
        a, b = interval(d, k, h)
        lo, hi = max(lo, a), min(hi, b)
    return hi if lo <= hi else None


def rne_div(a, b):
    assert b > 0
    sign = -1 if a < 0 else 1
    v, r = divmod(abs(a), b)
    if r*2 > b or r*2 == b and v%2:
        v += 1
    return sign*v


def mul_u32_words(a, b):
    # 16-bit schoolbook limbs; independent Python int multiplication is oracle.
    aa, bb = (a & 65535, a >> 16), (b & 65535, b >> 16)
    out = [0]*4
    for i in range(2):
        carry = 0
        for j in range(2):
            t = out[i+j]+aa[i]*bb[j]+carry
            out[i+j], carry = t & 65535, t >> 16
        k = i+2
        while carry:
            t = out[k]+carry
            out[k], carry = t & 65535, t >> 16
            k += 1
    return out[0]+(out[1] << 16), out[2]+(out[3] << 16)


def floor_scaled_binary64(v):
    raw = struct.unpack('<Q', struct.pack('<d', v))[0]
    sign, exponent, fraction = raw >> 63, (raw >> 52) & 2047, raw & ((1 << 52)-1)
    assert exponent != 2047 and (not sign or fraction == 0 and exponent == 0)
    if exponent == 0:
        mantissa, shift = fraction, -1074+20
    else:
        mantissa, shift = (1 << 52)+fraction, exponent-1023-52+20
    return mantissa << shift if shift >= 0 else mantissa >> (-shift)


def pair_brute(points, radius):
    best = None
    for i, a in enumerate(points):
        for b in points[i+1:]:
            if a[0] == b[0] and abs(a[1]-b[1]) <= radius:
                p = tuple(sorted((a[2], b[2])))
                best = p if best is None else min(p, best)
    return best


def pair_witness(sorted_points, radius):
    eligible = []
    for i, p in enumerate(sorted_points):
        for j in (i-1, i+1):
            if 0 <= j < len(sorted_points):
                b = sorted_points[j]
                if p[0] == b[0] and abs(p[1]-b[1]) <= radius:
                    eligible.append(p)
                    break
    if not eligible:
        return None
    a = min(eligible, key=lambda p: p[2])
    b = min(p[2] for p in sorted_points
            if p[2] != a[2] and p[0] == a[0] and abs(p[1]-a[1]) <= radius)
    return a[2], b


def bed_plans(s, color, total, soft=3, h=7, local_limit=5):
    # Abstract grid fixture, same guard algebra, not product channel code.
    proposed = []
    for edge in range(color, len(s)-1, 3):
        d = s[edge+1]-s[edge]
        if abs(d) <= soft:
            continue
        donor, receiver = (edge+1, edge) if d > 0 else (edge, edge+1)
        changes = {donor: -1, receiver: 1}
        edges = [(s[j+1]-s[j], changes.get(j+1, 0)-changes.get(j, 0))
                 for j in range(max(0, edge-1), min(len(s)-1, edge+2))]
        req = min((abs(d)-soft)//2, s[donor], 256-s[receiver], local_limit)
        q = max_grant(edges, req, h)
        assert q is not None
        proposed.append((edge, donor, receiver, q))
    out, prefix = [], 0
    for edge, d, r, req in proposed:
        grant = min(req, max(0, total-prefix))
        prefix += req
        out.append((edge, d, r, grant))
    return out


def table_reference():
    sha = hashlib.sha256()
    worst_error = Decimal(0)
    unsafe_floor_without_margin = None
    flat_indices = []
    with localcontext() as ctx:
        ctx.prec = 58
        qd = Decimal(Q)
        h_exact = Decimal.from_float(math.tan(math.pi/6))
        for j in range(-7680, 7681):
            x = Decimal(j)*PI/Decimal(46080)
            sin, cos, ts, tc = x, Decimal(1), x, Decimal(1)
            for k in range(1, 23):
                ts = -ts*x*x/Decimal((2*k)*(2*k+1))
                tc = -tc*x*x/Decimal((2*k-1)*(2*k))
                sin += ts
                cos += tc
            tangent = sin/cos
            u = int(abs(tangent)*qd)*(1 if j >= 0 else -1)
            sn = int((sin*qd).to_integral_value(rounding=ROUND_HALF_EVEN))
            cs = int((cos*qd).to_integral_value(rounding=ROUND_HALF_EVEN))
            sha.update(struct.pack('<4i', u, sn, cs, 0))
            error = abs(Decimal(u)/qd-tangent)
            assert error < 1/qd
            worst_error = max(worst_error, error)
            # Physical relative slope after integer guard d=+/-H, including LUT error.
            for d in (-H, H):
                relative = Decimal(d+u)/qd-tangent
                assert abs(relative) < h_exact
            # Old bound floor(H30*Q), without extra cell, can violate physical guard.
            for d in (-605395, 605395):
                relative = Decimal(d+u)/qd-tangent
                if 0 < abs(j) < 7680 and abs(relative) > h_exact and unsafe_floor_without_margin is None:
                    unsafe_floor_without_margin = {'angleIndex': j, 'tanUnits': u, 'dUnits': d,
                                                  'physicalAbsResidual': str(abs(relative)),
                                                  'oldHardLimitF64': str(h_exact)}
            if abs(u) <= H:
                flat_indices.append(j)
        assert unsafe_floor_without_margin is not None
        assert max(flat_indices) == 7679
    return {'rows': 15361, 'layoutBytes': 16, 'designReferenceSHA256': sha.hexdigest(),
            'maxTanQuantizationError': str(worst_error), 'Hfloor': 605395, 'Hsafe': H,
            'floorWithoutMarginCounterexample': unsafe_floor_without_margin,
            'flatMaxAngleIndex': 7679, 'flatMaxAngleDeg': 7679/256,
            'status': 'DESIGN_DECIMAL_REFERENCE_NOT_PRODUCTION_APPROVED_TABLE',
            'limits': 'Decimal Taylor reference is not device arithmetic or CPU-engine equivalence certification'}


def resource(k, n=128, kw=64, queue=16, dedup=4096):
    e = 4*(33*k+n-1)+k
    p = {
        'three_state_banks': 3*(64*k+48*kw+64*n+1024),
        'two_publications': 2*(64*k+64*kw+32*n+1024),
        'motion32': 128*k, 'two_index_arrays': 8*k,
        'scan_scratch': 4*(k+kw+n), 'edge_proposals': 64*n, 'column_audit': 16*n,
        'two_frame_event64_arenas': 2*256*e,
        'dedup64': 256*dedup, 'source_event16': 64*dedup, 'queue32': 128*queue,
        'two_status256': 2048, 'two_gpu_completion256': 2048,
        'two_host_completion256': 2048, 'source_receipts64': 256*queue,
        'angle_table': 245776, 'fixed_stats': 1024, 'fixed_control': 1024,
    }
    h = (k-1).bit_length()
    layers = h*(h+1)//2
    return {'K': k, 'N': n, 'KwReserved': kw, 'Q': queue, 'D': dedup,
            'eventsPerFrameCapacity': e, 'singleEventBlockBytes': 256*e,
            'partsBytes': p, 'basePayloadBytes': sum(p.values()),
            'sortLayers': layers, 'quantumDispatchBound': 23+layers,
            'sourceDispatchBound': 16+2*h,
            'tickDispatchBound': 4*(23+layers)+16+2*h,
            'rootFenceBoundPerTick': 5, 'completionBytesPerRoot': 1024,
            'completionReadbackBytesPerTickBound': 5120,
            'status': 'DESIGN_TEMPLATE_NOT_ALLOCATED_NOT_RSS',
            'excludes': ['driver/program/alignment', 'render VBO/textures/fullframe 8*w*h',
                         'NAPI/cold objects', 'IO/checkpoint', 'M1/M2/M3 additional buffers']}


def main():
    rng = random.Random(230)
    c = {}
    for _ in range(3000):
        a, b = rng.randrange(1 << 32), rng.randrange(1 << 32)
        lo, hi = mul_u32_words(a, b)
        assert lo+(hi << 32) == a*b
    c['mul32To64Cases'] = 3000
    rne_count = 0
    for n in range(-150, 151):
        for d in range(1, 33):
            assert rne_div(n, d) == round(Fraction(n, d))
            rne_count += 1
    c['signedRNECases'] = rne_count
    for _ in range(1500):
        v = math.ldexp(rng.random(), rng.randrange(-1070, 9))
        assert floor_scaled_binary64(v) == (Fraction.from_float(v)*Q).__floor__()
    assert floor_scaled_binary64(math.ldexp(1, -1074)) == 0
    assert floor_scaled_binary64(256.) == 256*Q
    assert floor_scaled_binary64(-0.) == 0
    c['binary64InputFloorCases'] = 1500
    c['largeTotal64Fixture'] = {'maxVolumeUnits': 128*256*Q,
                                'exceedsUint32': 128*256*Q > MASK}

    guard_cases = 0
    for d in range(-7, 8):
        for k in range(-2, 3):
            for req in range(25):
                brute = [q for q in range(req+1) if abs(d+k*q) <= 7]
                assert max_grant([(d, k)], req, 7) == max(brute)
                guard_cases += 1
    for _ in range(2000):
        edges = [(rng.randrange(-7, 8), rng.randrange(-2, 3)) for _ in range(3)]
        req = rng.randrange(50)
        brute = [q for q in range(req+1) if all(abs(d+k*q) <= 7 for d, k in edges)]
        assert max_grant(edges, req, 7) == max(brute)
        guard_cases += 1
    c['guardIntervalCases'] = guard_cases
    for n in range(2, 129):
        for color in range(3):
            edges = list(range(color, n-1, 3))
            for i, e in enumerate(edges):
                we, re = {e, e+1}, set(range(max(0, e-1), min(n, e+3)))
                ge = set(range(max(0, e-1), min(n-1, e+2)))
                for f in edges[i+1:]:
                    wf, rf = {f, f+1}, set(range(max(0, f-1), min(n, f+3)))
                    gf = set(range(max(0, f-1), min(n-1, f+2)))
                    assert not we & rf and not wf & re and not ge & gf
    c['threeColorDomainsCheckedN'] = [2, 128]
    bed_cases = 0
    for _ in range(400):
        s = [50]
        for _ in range(rng.randrange(4, 33)):
            s.append(s[-1]+rng.randrange(-7, 8))
        offset = max(0, 5-min(s))
        s = [x+offset for x in s]
        for color in range(3):
            plans = bed_plans(s, color, total=13)
            a, b = s[:], s[:]
            for _, d, r, q in plans:
                a[d] -= q
                a[r] += q
            shuffle = plans[:]
            rng.shuffle(shuffle)
            for _, d, r, q in shuffle:
                b[d] -= q
                b[r] += q
            assert a == b and sum(a) == sum(s)
            assert sum(p[3] for p in plans) <= 13
            assert all(0 <= v <= 256 for v in a)
            assert all(abs(a[j+1]-a[j]) <= 7 for j in range(len(a)-1))
            s = a
            bed_cases += 1
    c['threeColorBudgetPatchCases'] = bed_cases

    carry = total_ns = quanta = 0
    for _ in range(5000):
        dt = rng.randrange(33333334)
        total_ns += dt
        f, carry = divmod(carry+120*dt, 10**9)
        assert f <= 4
        quanta += f
        assert total_ns*120 == quanta*10**9+carry
    c['timeCarryCases'] = 5000
    c['timeRemainderNumerator'] = carry
    crossing_max = 0
    for _ in range(2000):
        x = rng.randrange(128*Q)
        v = rng.randrange(-1024*Q, 1024*Q+1)
        dx = rne_div(v, 120)
        count = abs((x+dx)//Q-x//Q)+1
        crossing_max = max(crossing_max, count)
        assert count <= 10  # Includes one conservative endpoint bookkeeping plane.
    assert math.ceil(1024/120)+1 == 10
    c['DdaCrossingBoundFixtures'] = {'randomCases': 2000, 'maxCountIncludingEndpointPadding': crossing_max,
                                   'straightChordPhysicalColumnPlaneBound': 10,
                                   'motionRowsReserved': 32, 'scope': 'BOUND_ONLY_NOT_TRAJECTORY_EXECUTION'}

    # Exact conserved components; external source increments I only once.
    sand = [rng.randrange(Q) for _ in range(16)]
    water = [rng.randrange(Q) for _ in range(16)]
    flight = [rng.randrange(Q) for _ in range(16)]
    initial_s, initial_w = sum(sand), sum(water)+sum(flight)
    input_s, input_w = 0, 0
    for step in range(5000):
        d, r = rng.sample(range(16), 2)
        arr = sand if step%3 == 0 else water if step%3 == 1 else flight
        q = rng.randrange(arr[d]+1)
        arr[d] -= q
        arr[r] += q
        if step%7 == 0:
            q = rng.randrange(1024)
            flight[d] += q
            input_w += q
        if step%11 == 0:
            q = rng.randrange(flight[d]+1)
            flight[d] -= q
            water[d] += q
        if step%13 == 0:
            q = rng.randrange(1024)
            sand[d] += q
            input_s += q
        assert sum(sand) == initial_s+input_s
        assert sum(water)+sum(flight) == initial_w+input_w
    c['exactInventoryTransferCases'] = 5000

    pair_cases = updates = 0
    counter = [(0, 0, 1, 5), (0, 2, 20, 5), (0, 4, 2, 5)]
    assert pair_witness(sorted(counter), 4) == (1, 2)
    for _ in range(600):
        count = rng.randrange(2, 26)
        ids = rng.sample(range(1, 10000), count-1)+[MASK]
        points = [(rng.randrange(3), rng.randrange(-15, 16), ident, rng.randrange(1, 20)) for ident in ids]
        radius = rng.randrange(7)
        resident = sorted(points)
        next_id = max(i for i in ids if i != MASK)+1
        for _ in range(5):
            brute = pair_brute(points, radius)
            assert pair_witness(resident, radius) == brute
            pair_cases += 1
            if brute is None:
                break
            a, b = (next(p for p in points if p[2] == ident) for ident in brute)
            merged = (a[0], rne_div(a[1]*a[3]+b[1]*b[3], a[3]+b[3]), a[2], a[3]+b[3])
            points = [p for p in points if p[2] not in brute]+[merged]
            resident = [p for p in resident if p[2] not in brute]
            resident.insert(bisect_left(resident, merged), merged)
            assert resident == sorted(points)
            updates += 1
    c['integerPairOracleComparisons'] = pair_cases
    c['residentDeleteReinsertCases'] = updates
    c['nonAdjacentMinBFixture'] = [1, 2]

    # Protocol state-machine model: candidate never visible before acceptance.
    current, revision, accepted_commands = (1, 2, 3), 0, set()
    for nonce in range(1000):
        base = current
        candidate = tuple(x+nonce+1 for x in base)
        outcome = rng.choice(('success', 'algorithm_fail', 'timeout_late', 'cancel', 'gl_error'))
        ready = True
        token_ok = outcome == 'success'
        if ready and token_ok and nonce not in accepted_commands:
            current = candidate
            revision += 1
            accepted_commands.add(nonce)
        else:
            assert current == base
        # Late completion cannot override a root tombstone.
        if outcome == 'timeout_late':
            late_ok = False
            assert not late_ok and current == base
    c['hostAcceptanceStateModelCases'] = 1000
    c['hostAcceptanceModelAccepted'] = len(accepted_commands)
    c['angleLUTDesignReference'] = table_reference()

    res = [resource(k) for k in (192, 512)]
    assert [r['basePayloadBytes'] for r in res] == [15059984, 36997904]
    assert [r['quantumDispatchBound'] for r in res] == [59, 68]
    assert [r['tickDispatchBound'] for r in res] == [268, 306]
    doc = (ROOT/'docs/algorithm-T230-gpu-resident-step-v1.md').read_text(encoding='utf-8')
    assert doc.count('```')%2 == 0
    assert all(f'| GR{i:02} ' in doc for i in range(1, 13))
    assert all(f"{r['basePayloadBytes']:,}B" in doc for r in res)
    assert '605394' in doc and 'dedup64word' in doc
    raw131 = json.loads((ROOT/'test/reports/t131/t131.json').read_text(encoding='utf-8'))['t131']
    c['T131RawLimitsRead'] = raw131['limits']
    c['T131ComputeBlocksFieldInThisJSON'] = 'computeStorageBlocks' in raw131['limits']
    c['T231LimitsAttribution'] = 'T231 reports compute stage blocks=16; not present in inspected t131.json; preserve distinction'
    paths = ['AGENTS.md', '.codex/PIPELINE.md', '.codex/agents/designer.toml',
             'docs/algorithm-T197-whole-transaction-complexity-v1.md',
             'docs/architecture-T205-whole-transaction-v1.md',
             'docs/analysis-T205-tx-resource-work-manifest-v1.json',
             'docs/developer-T211-whole-transaction-d1-v1.md',
             'docs/developer-T212-tx-gpu-bridge-v1.md',
             'docs/test-report-T213-tx-d1-independent-v1.md',
             'docs/test-report-T142-gpu-device-differential-v1.md',
             'docs/test-report-T161-gpu-full-transaction-device-v1.md',
             'docs/test-report-T199-gpu-strict-device-v1.md',
             'docs/algorithm-T207-dry-derived-render-domain-v1.md',
             'docs/algorithm-T221-sand-water-demo-v1.md',
             'docs/architecture-T222-clear-water-flight-v2.md',
             'docs/architecture-T231-gpu-resident-platform-v1.md',
             'test/reports/t131/t131.json', 'test/p013_compute/pair_executor_v1.cpp',
             'test/p013_compute/glsl/pair_build_v1.comp',
             'entry/src/main/cpp/p013renderer/egl_context.cpp',
             'entry/src/main/cpp/p013renderer/scene_gpu.cpp',
             'entry/src/main/ets/physics/WaterSandField.ets',
             'entry/src/main/ets/physics/P011ParticleHot.ets',
             'entry/src/main/ets/physics/P011ParticleField.ets',
             'entry/src/main/ets/physics/P011ParticleHotBudget.ets',
             'entry/src/main/ets/physics/P051EmptySandDemo.ets',
             'docs/algorithm-T230-gpu-resident-step-v1.md',
             'docs/retro-T230-gpu-resident-step-v1.md',
             'docs/design-T230-gpu-resident-model-v1.py']
    inputs = []
    for path in paths:
        b = (ROOT/path).read_bytes()
        inputs.append({'path': path, 'bytes': len(b), 'sha256': hashlib.sha256(b).hexdigest()})
    evidence = {
        'revision': 'T230_DESIGN_INTEGER_EVIDENCE_V1', 'date': '2026-10-28',
        'task': '6ae1c798e4b0c3103c116f0a', 'route': 'gpu-resident-integer-step-v1',
        'status': 'DESIGN_READY_FOR_ARCHITECT_NOT_IMPLEMENTED_NOT_VALIDATED',
        'scope': 'DESIGN_ARITHMETIC_AND_PROTOCOL_MODEL_ONLY',
        'deviceUsed': False, 'productTestsRun': False, 'GLExecuted': False,
        'shaderCompiled': False, 'checks': c, 'resourceTemplates': res, 'inputs': inputs,
        'specReferences': [
            'https://registry.khronos.org/OpenGL/specs/es/3.1/GLSL_ES_Specification_3.10.pdf',
            'https://registry.khronos.org/OpenGL/specs/es/3.1/es_spec_3.1.pdf',
            'https://registry.khronos.org/EGL/extensions/KHR/EGL_KHR_fence_sync.txt'],
        'limits': ['No full motion/channel/GLSL implementation verified',
                   'CPU integer/Fraction/Decimal checks are designer models, not independent product tests',
                   'LUT is a design reference only; not a frozen production table/device certificate',
                   'T231 same-context first; cross-context/actual allocations/performance still need probes',
                   'M0 single H dry only; M1/M2/M3 are explicitly unimplemented',
                   'Payload templates exclude driver/render/checkpoint/IO/runtime objects']}
    (ROOT/'docs/design-T230-gpu-resident-evidence-v1.json').write_text(
        json.dumps(evidence, ensure_ascii=False, indent=2)+'\n', encoding='utf-8')
    print(json.dumps({'scope': evidence['scope'],
                      'caseCounts': {k: v for k, v in c.items() if k.endswith('Cases') or k.endswith('Comparisons')},
                      'guardMarginCounterexample': c['angleLUTDesignReference']['floorWithoutMarginCounterexample'],
                      'LUTRows': 15361, 'basePayloadBytes': [r['basePayloadBytes'] for r in res],
                      'tickDispatchBounds': [r['tickDispatchBound'] for r in res],
                      'inputFingerprints': len(inputs)}, ensure_ascii=False, indent=2))


if __name__ == '__main__':
    main()
