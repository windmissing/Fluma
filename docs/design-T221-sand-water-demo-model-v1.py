"""T221 designer arithmetic/spec model. No product imports/tests/device/GL.

Run from repository root with Python 3.12. Evidence belongs to this design only.
Fractions check ideal conservation; float cases illustrate inherited RN risks.
"""
from fractions import Fraction as Q
from pathlib import Path
import hashlib
import json
import math
import random
import re
import struct

ROOT = Path(__file__).resolve().parents[1]
DOC = ROOT / 'docs/algorithm-T221-sand-water-demo-v1.md'


def settle(eta, rows, by_height=True):
    """Exact ideal contact operator; rows=(id,y,m), single column only."""
    rows = sorted(rows, key=(lambda p: (p[1], p[0])) if by_height else (lambda p: p[0]))
    alive, arrived = [], []
    for ident, y, m in rows:
        if y <= eta:
            eta += m
            arrived.append(ident)
        else:
            alive.append((ident, y, m))
    return eta, alive, arrived


def area(lo, hi, a, b):
    return max(0., min(hi, b)-max(lo, a))


def surface_alpha(ys, ye, pixel_y, width, gain, slope=0., f0=.02, power=32):
    if ye <= ys:
        return 0.
    coverage = min(1., area(pixel_y-.5, pixel_y+.5, max(ys, ye-width), ye))
    nx, ny = -slope/math.hypot(slope, 1.), 1./math.hypot(slope, 1.)
    lx, ly = -.4/math.hypot(-.4, 1.), 1./math.hypot(-.4, 1.)
    f = f0 + (1-f0)*(1-min(1., max(0., ny)))**5
    g = min(1., max(0., nx*lx+ny*ly))**power
    return gain*coverage*(f+(1-f)*g)


def ideal_water_step(s, w, u, serial, reserved=None):
    """Independent rational five-bound derivation, no product RN execution."""
    n, out = len(w), []
    reserved = reserved or [Q(0)]*n
    total = sum(w)
    for turn in range(2):
        for edge in range((serial+turn)%2, n-1, 2):
            h = [s[j]+w[j]-(Q(j)+Q(1, 2))*u for j in (edge, edge+1)]
            if h[0] == h[1]:
                continue
            d, r = (edge, edge+1) if h[0] > h[1] else (edge+1, edge)
            hd = s[d]+w[d]-(Q(d)+Q(1, 2))*u
            hr = s[r]+w[r]-(Q(r)+Q(1, 2))*u
            zd, zr = s[d]-(Q(d)+Q(1, 2))*u, s[r]-(Q(r)+Q(1, 2))*u
            delta = hd-hr
            q = min(Q(4, 60)*delta, delta/2, w[d],
                    max(Q(0), hd-max(zd, zr)), max(Q(0), 256-s[r]-w[r]-reserved[r]))
            if q > 0:
                before = w[d]+w[r]
                w[d] -= q
                w[r] += q
                assert w[d]+w[r] == before
                out.append((d, r, q))
    assert sum(w) == total and min(w) >= 0
    return out


def budgets(k, kw=64, n=128, e=8, ew=8, d=4096):
    parts = {
        'five_water_state_banks': 5*(74*kw+8*n+128),
        'two_water_event48_header64_summary16': 2*(48*8*ew*kw+64*8+16*8*kw),
        'two_tapes_u32_pair': 2*2*4*(e*k+2*(n-1)+ew*kw),
        'sort_marks_flight_scratch': 9*kw+256,
        'source_dedup_event_suffix_and_scratch': 16*8*d+8*8*d+16*8,
        'four_water_proof_xy_meta_packets': 384*kw,
        'four_profile_and_two_gpu_texture_delta': 96*n,
        'two_gl_vbo_header_optics_delta': 32*kw+1024+256,
    }
    total = sum(parts.values())
    return {'N': n, 'K': k, 'Kw': kw, 'e': e, 'ew': ew, 'D': d,
            'baseArrayDeltaBytes': total, 'parts': parts,
            'T209OwnerBytes': {192: 4230656, 512: 6312256}[k],
            'T219CTemplateBytes': {192: 207112, 512: 331272}[k],
            'combinedTemplateBytes': total+{192: 4437768, 512: 6643528}[k],
            'fullFrameTargetAdditionalBytes': '8*widthPx*heightPx, if not already accounted/reused',
            'peakRSS': 'NOT_MEASURED_EXCLUDES_DRIVER_OBJECTS_OTHER_LIVE_BUFFERS'}


def main():
    rng = random.Random(221)
    checks = {}
    rows = [(1, Q(2), Q(1, 4)), (2, Q(1), Q(2))]
    naive = settle(Q(1), rows, False)
    ordered = settle(Q(1), rows)
    assert naive[1] and naive[1][0][1] <= naive[0]
    assert not ordered[1] and ordered[0] == Q(13, 4)
    checks['arrivalOrderCounterexample'] = {
        'initialEta': 1, 'id1': {'y': 2, 'm': .25}, 'id2': {'y': 1, 'm': 2},
        'idOrderMissesEngulfedId1': True, 'heightOrderArrivedIds': ordered[2],
        'finalEta': float(ordered[0]), 'scope': 'IDEAL_CONTACT_MODEL_NOT_PRODUCT'}
    ncases = 0
    for _ in range(1500):
        entry = Q(rng.randrange(17), 4)
        points = [(i+1, Q(rng.randrange(33), 4), Q(rng.randrange(1, 9), 8))
                  for i in range(rng.randrange(1, 33))]
        eta, alive, arrived = settle(entry, points)
        assert eta+sum(p[2] for p in alive) == entry+sum(p[2] for p in points)
        assert all(y > eta for _, y, _ in alive)
        assert len(arrived)+len(alive) == len(points)
        ncases += 1
    checks['idealSortedContactCases'] = ncases

    # Source success and arrival are distinct transfers; stop preserves flight.
    w, flight, initial, ledger = Q(0), Q(0), Q(0), Q(0)
    for _ in range(500):
        q = Q(rng.randrange(1, 100), 256)
        before_w = w
        flight += q
        ledger += q
        assert w == before_w and w+flight == initial+ledger
        landed = flight*Q(rng.randrange(5), 4)
        w += landed
        flight -= landed
        assert w+flight == initial+ledger
    stop_total = w+flight
    w += flight
    flight = Q(0)
    assert w == stop_total and flight == 0
    checks['idealSourceArrivalStopTransfers'] = 500

    # Illustrative path under the proposed semi-implicit chord, not analytic roots.
    def zero_angle_arrival(y, eta=Q(1), g=Q(12), dt=Q(1, 60)):
        elapsed, vy = Q(0), Q(0)
        for _ in range(600):
            vy1 = vy-g*dt
            y1 = y+vy1*dt
            if y1 <= eta:
                lam = (y-eta)/(y-y1)
                assert 0 <= lam <= 1
                return elapsed+dt*lam
            y, vy, elapsed = y1, vy1, elapsed+dt
        raise AssertionError('fixture did not arrive')
    lower, higher = zero_angle_arrival(Q(2)), zero_angle_arrival(Q(5))
    assert 0 < lower < higher
    checks['chordArrivalFixtures'] = {'lowerSimulationSeconds': float(lower),
                                    'higherSimulationSeconds': float(higher),
                                    'g': 12, 'dt': '1/60', 'scope': 'IDEAL_EXAMPLE_NOT_CALIBRATION'}

    s, w = [Q(4)]*9, [Q(0)]*9
    w[4] = Q(2)
    left = right = Q(0)
    visits = 0
    for serial in range(24):
        for d, r, q in ideal_water_step(s, w, Q(0), serial):
            visits += 1
            if d == 4 and r == 3:
                left += q
            if d == 4 and r == 5:
                right += q
    assert left > 0 and right > 0 and sum(w) == 2
    checks['idealFlatFiveBoundFlow'] = {'steps': 24, 'nonzeroEdgeTransfers': visits,
                                      'leftOut': float(left), 'rightOut': float(right),
                                      'requiresInstantMirrorSymmetry': False}
    tilted = ideal_water_step([Q(0)]*5, [Q(1)]*5, Q(1, 8), 0)
    assert tilted and all(r > d for d, r, _ in tilted)
    barrier_w = [Q(1), Q(0), Q(0)]
    barrier = ideal_water_step([Q(0), Q(3), Q(0)], barrier_w, Q(0), 0)
    assert not barrier and barrier_w == [1, 0, 0]
    checks['idealSlopeBarrierFixtures'] = {'tiltPositiveAllRight': True, 'highBedBlocks': True}

    # Float storage risks must be reported, never repaired/clamped.
    one, tiny = 1., math.ldexp(1., -54)
    assert one+tiny == one
    assert Q.from_float(.1)+Q.from_float(.2) != Q.from_float(.1+.2)
    checks['RNExamples'] = {'positiveArrivalCanLeaveWUnchanged': True,
                            'roundedWaterAdditionChangesExactMass': True,
                            'tinyMassLEHex': struct.pack('<d', tiny).hex()}

    optics_cases = 0
    for _ in range(2000):
        ys = rng.uniform(-10, 10)
        thickness = 10**rng.uniform(-8, 2)
        ye = ys+thickness
        px = ys+rng.uniform(-2, thickness+2)
        gain, width, slope = rng.random(), rng.uniform(.01, 8), rng.uniform(-256, 256)
        a = surface_alpha(ys, ye, px, width, gain, slope)
        assert math.isfinite(a) and 0 <= a <= gain
        assert surface_alpha(ys, ys, px, width, gain, slope) == 0
        assert surface_alpha(ys, ye, px, width, 0, slope) == 0
        optics_cases += 1
    shallow = [surface_alpha(0, h, 0., 1.5, .35) for h in (1e-3, 1e-6, 1e-9)]
    assert shallow[0] > shallow[1] > shallow[2] >= 0
    # Dry middle column contributes no coverage, regardless of wet neighbors.
    assert surface_alpha(1., 1., 1., 1.5, .35, 256.) == 0.
    checks['opticsRangeCases'] = optics_cases
    checks['shallowWaterAlphaLimit'] = shallow

    glyph_cases = 0
    for _ in range(500):
        mass = 2**rng.uniform(-40, 8)
        radius = min(4., max(.5, 2*math.sqrt(mass/.125)))
        opacity = .6
        for distance in (0., radius, radius+.5, radius+1.):
            alpha = opacity*min(1., max(0., radius+.5-distance))
            assert 0 <= alpha <= opacity
        assert opacity*min(1., max(0., radius+.5-(radius+1.))) == 0
        glyph_cases += 1
    checks['waterGlyphCases'] = glyph_cases

    def trial(a0, raw, h=1/60):
        bounded = max(-30., min(30., raw))
        change = (bounded-a0)*(1-math.exp(-h/.12))
        return a0+max(-90*h, min(90*h, change))
    accepted = 0.
    for i in range(1000):
        raw = rng.uniform(-89, 89)
        candidate = trial(accepted, raw)
        assert -30 <= candidate <= 30 and abs(candidate-accepted) <= 1.5+1e-14
        old = accepted
        success = i%3 != 0
        if success:
            accepted = candidate
        else:
            assert accepted == old
    a1 = trial(0, 89)
    a2 = trial(a1, 89)
    assert 0 < a1 < a2 < 30
    checks['gravityTrialCases'] = 1000
    checks['gravityFixture'] = {'rawTarget': 89, 'boundedTarget': 30,
                                 'firstSuccess': a1, 'secondSuccess': a2,
                                 'secondFailureKeeps': a1, 'shake': 'DIAGNOSTIC_ONLY',
                                 'scope': 'FORMULA_MODEL_NOT_SENSOR_TEST'}

    text = DOC.read_text(encoding='utf-8')
    expected = [f'SW{i:02}' for i in range(1, 15)]
    assert all(re.search(r'\| '+item+r'\b', text) for item in expected)
    assert text.count('```')%2 == 0
    for filename in re.findall(r'`((?:algorithm|design|retro)-[^` /]+\.(?:md|json|py))`', text):
        # Evidence file is written below; document and model files already exist.
        assert (ROOT/'docs'/filename).exists() or filename == 'design-T221-sand-water-demo-evidence-v1.json'
    source = (ROOT/'entry/src/main/ets/physics/clearwater/P011ClearWaterSource.ets').read_text(encoding='utf-8')
    assert 'if(water)work.W[c]+=q;' in source
    assert 'const c=Math.floor(this.request[8])' in source
    checks['staticSourceContract'] = 'LOCAL_W_ONLY_NO_FLIGHT_IN_CURRENT_SOURCE'
    checks['testContractsPresent'] = expected
    resource = [budgets(k) for k in (192, 512)]
    assert [r['baseArrayDeltaBytes'] for r in resource] == [1304480, 1345440]
    assert [r['combinedTemplateBytes'] for r in resource] == [5742248, 7988968]
    for r in resource:
        assert f"{r['baseArrayDeltaBytes']:,}B" in text
    paths = [
        'AGENTS.md', '.codex/agents/designer.toml', '.codex/PIPELINE.md',
        'docs/algorithm-T196-clear-water-B-step-v1.md',
        'docs/architecture-T203-clear-water-B-v1.md',
        'docs/developer-T209-clear-water-B-v1.md',
        'docs/test-report-T215-clear-water-B-independent-v1.md',
        'docs/architecture-T219-clear-water-C-v1.md',
        'docs/analysis-T219-clear-water-C-engineering-manifest-v1.json',
        'docs/architecture-T202-demo-material-arrival-v1.md',
        'docs/algorithm-T197-whole-transaction-complexity-v1.md',
        'docs/algorithm-T207-dry-derived-render-domain-v1.md',
        'entry/src/main/ets/physics/P051GravityInputAdapter.ets',
    ]
    paths += [p.relative_to(ROOT).as_posix() for p in sorted(
        (ROOT/'entry/src/main/ets/physics/clearwater').glob('*.ets'))]
    paths += ['docs/algorithm-T221-sand-water-demo-v1.md',
              'docs/retro-T221-sand-water-demo-v1.md',
              'docs/design-T221-sand-water-demo-model-v1.py']
    inputs = []
    for p in paths:
        raw = (ROOT/p).read_bytes()
        inputs.append({'path': p, 'bytes': len(raw), 'sha256': hashlib.sha256(raw).hexdigest()})
    result = {'revision': 'T221_DESIGN_ARITHMETIC_EVIDENCE_V1', 'date': '2026-10-28',
              'task': '6ae19fb6e4b0f11bdf2b2ca2', 'route': 'sand-water-demo-flight-v1',
              'designStatus': 'DESIGN_READY_FOR_ARCHITECT', 'productStatus': 'NOT_IMPLEMENTED_NOT_VALIDATED',
              'scope': 'DESIGN_MATH_STATIC_CONTRACT_ONLY', 'productTestsExecuted': False,
              'deviceUsed': False, 'GLExecuted': False, 'checks': checks,
              'resources': resource, 'inputs': inputs,
              'limits': ['Ideal Fraction models do not verify product RN graphs or event trajectory implementation',
                         'No product owner, shader, sensor, replay or device performance executed',
                         'Optics/gravity/flight parameters remain PENDING_CALIBRATION',
                         'T219 is partial engineering; this does not change its task state',
                         'Resource table is a design payload template, not peak RSS']}
    out = ROOT/'docs/design-T221-sand-water-demo-evidence-v1.json'
    out.write_text(json.dumps(result, ensure_ascii=False, indent=2)+'\n', encoding='utf-8')
    print(json.dumps({'scope': result['scope'], 'checks': checks,
                      'resourceDeltaBytes': [r['baseArrayDeltaBytes'] for r in resource],
                      'combinedTemplateBytes': [r['combinedTemplateBytes'] for r in resource],
                      'inputFingerprints': len(inputs)}, ensure_ascii=False, indent=2))


if __name__ == '__main__':
    main()
