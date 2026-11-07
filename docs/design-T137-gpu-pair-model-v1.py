"""Designer finite model of T137, NOT a GPU/product test or production module.

Run from workspace: python docs/design-T137-gpu-pair-model-v1.py
Writes only docs/design-T137-gpu-pair-evidence-v1.json.
"""
import bisect
import hashlib
import itertools
import json
import math
from pathlib import Path
import random
import struct

ROOT = Path(__file__).resolve().parent.parent
UINT_MAX = 2**32 - 1
COUNTS = dict(pools=0, activeQueries=0, buildWrites=0, segments=0,
              malformedRejected=0, continuousMergeSelections=0)


def particle(i, y, group=(0, 1, 1, 1, 0), slot=None, active=True):
    return dict(id=i, y=y, group=group, slot=i if slot is None else slot,
                active=active, mass=1.0)


def prepare(pool, radius):
    if not math.isfinite(radius) or radius < 0:
        raise ValueError('radius')
    live = [p for p in pool if p['active']]
    ids = sorted(p['id'] for p in live)
    if any(not isinstance(i, int) or not 1 <= i <= UINT_MAX for i in ids):
        raise ValueError('ID domain')
    if len(set(ids)) != len(ids):
        raise ValueError('duplicate active ID')
    if len({p['slot'] for p in live}) != len(live):
        raise ValueError('duplicate slot')
    if any(not math.isfinite(p['y']) for p in live):
        raise ValueError('Y domain')
    rows = sorted(live, key=lambda p: (p['group'], p['y'], p['id'], p['slot']))
    desc = []
    g0 = 0
    while g0 < len(rows):
        g1 = g0 + 1
        while g1 < len(rows) and rows[g1]['group'] == rows[g0]['group']:
            g1 += 1
        for a_rank in range(g0, g1):
            a = rows[a_rank]
            # Independent lower/upper binary search using original abs(RN(a-b)).
            lo, hi = g0, g1
            while lo < hi:
                mid = lo + (hi - lo) // 2
                if rows[mid]['y'] <= a['y']:
                    lo = mid + 1
                else:
                    hi = mid
            split = lo
            lo, hi = g0, split
            while lo < hi:
                mid = lo + (hi - lo) // 2
                if abs(a['y'] - rows[mid]['y']) <= radius:
                    hi = mid
                else:
                    lo = mid + 1
            left = lo
            lo, hi = split, g1
            while lo < hi:
                mid = lo + (hi - lo) // 2
                if abs(a['y'] - rows[mid]['y']) <= radius:
                    lo = mid + 1
                else:
                    hi = mid
            right = lo
            exact_positions = [i for i in range(g0, g1)
                               if abs(a['y'] - rows[i]['y']) <= radius]
            assert exact_positions == list(range(left, right))
            assert g0 <= left <= a_rank < right <= g1
            desc.append((a_rank, left, right, g0, g1))
        g0 = g1
    return rows, desc


def record_key(r):
    # Record layout id,slot,valid,originalYRank.
    return (1-r[2], r[0], r[1], r[3])


def gpu_model(rows, desc, capacity):
    k = 1 << (max(1, capacity)-1).bit_length()
    h = k.bit_length()-1
    leaf = [(p['id'], p['slot'], 1, i) for i, p in enumerate(rows)]
    leaf += [(0, 0, 0, i) for i in range(len(rows), k)]
    levels = [leaf]
    for level in range(1, h+1):
        previous = levels[-1]
        size = 1 << level
        target = [None]*k
        writes = [0]*k
        for j, item in enumerate(previous):
            start = (j//size)*size
            half = start + (size//2 if j-start >= size//2 else 0)
            other = start if half != start else start+size//2
            # The model uses library lower_bound only on the other sorted half.
            keys = [record_key(x) for x in previous[other:other+size//2]]
            rank = j-half + bisect.bisect_left(keys, record_key(item))
            out = start + rank
            writes[out] += 1
            assert target[out] is None, ('write collision', level, j, out)
            target[out] = item
        assert all(w == 1 for w in writes)
        for start in range(0, k, size):
            assert target[start:start+size] == sorted(leaf[start:start+size], key=record_key)
        COUNTS['buildWrites'] += k
        levels.append(target)
    candidates = []
    for a_rank, left, right, g0, g1 in desc:
        aid = rows[a_rank]['id']
        lo, hi, level = left+k, right+k, 0
        covered = []
        best = None
        while lo < hi:
            segments = []
            if lo & 1:
                segments.append(lo-(k >> level))
                lo += 1
            if hi & 1:
                hi -= 1
                segments.append(hi-(k >> level))
            for segment in segments:
                size = 1 << level
                start = segment*size
                assert g0 <= left <= start < start+size <= right <= g1
                covered.extend(range(start, start+size))
                table = levels[level][start:start+size]
                begin, end = 0, size
                while begin < end:
                    mid = begin+(end-begin)//2
                    r = table[mid]
                    if not r[2] or r[0] > aid:
                        end = mid
                    else:
                        begin = mid+1
                if begin < size and table[begin][2]:
                    r = table[begin]
                    candidate = (aid, r[0], rows[a_rank]['slot'], r[1], a_rank, r[3])
                    if best is None or candidate < best:
                        best = candidate
                COUNTS['segments'] += 1
            lo >>= 1
            hi >>= 1
            level += 1
        assert sorted(covered) == list(range(left, right))
        b = [p for p in rows[left:right] if p['id'] > aid]
        expected_b = min(b, key=lambda p: p['id']) if b else None
        assert (best[1] if best else None) == (expected_b['id'] if expected_b else None)
        candidates.append(best)
        COUNTS['activeQueries'] += 1
    candidates += [None]*(k-len(candidates))
    # Simulates workgroup min semantics; this is not a shared-memory GL test.
    while len(candidates) > 1:
        reduced = []
        for start in range(0, len(candidates), 64):
            valid = [v for v in candidates[start:start+64] if v is not None]
            reduced.append(min(valid) if valid else None)
        candidates = reduced
    result = candidates[0]
    return result[:4] if result else None


def scan_oracle(pool, radius):
    valid = []
    for a in pool:
        if not a['active']:
            continue
        for b in pool:
            if (b['active'] and a['id'] < b['id'] and a['group'] == b['group']
                    and abs(a['y']-b['y']) <= radius):
                valid.append((a['id'], b['id'], a['slot'], b['slot']))
    return min(valid) if valid else None


def check(pool, radius):
    rows, ranges = prepare(pool, radius)
    result = gpu_model(rows, ranges, len(pool))
    expected = scan_oracle(pool, radius)
    assert result == expected, (pool, radius, result, expected)
    COUNTS['pools'] += 1
    return result


def main():
    # Finite exhaustive discrete model, not an exhaustive F64/GPU proof.
    exhaustive = 0
    for n in range(5):
        ids = [UINT_MAX, 1, 2**31, 2][:n]
        for ys in itertools.product((-1., 0., 1.), repeat=n):
            for groups in itertools.product((0, 1), repeat=n):
                pool = [particle(ids[i], ys[i], (groups[i],1,1,1,0), i) for i in range(n)]
                for radius in (0., .5, 1.):
                    check(pool, radius)
                    exhaustive += 1
    # Synthetic adversarial and boundary pools.
    a, b, r = .2, -math.ulp(0.), .2
    assert abs(a-b) <= r and not a-r <= b <= a+r
    targeted = [
        ([particle(1,a,slot=0),particle(UINT_MAX,b,slot=1)],r),
        ([particle(1,-0.,slot=0),particle(2,0.,slot=1)],0.),
        ([particle(1,-1e308,slot=0),particle(2,1e308,slot=1)],1e308),
        ([particle(1,0.,slot=0),particle(2,math.ulp(0.),slot=1)],math.ulp(0.)),
    ]
    for pool, radius in targeted:
        check(pool, radius)
    rng = random.Random(137)
    choices = [-1., -.2, -math.ulp(0.), -0., 0., math.ulp(0.), .2,
               math.nextafter(.2, math.inf), 1., 24., 24.5, 1e308, -1e308]
    for iteration in range(240):
        n = rng.choice([0,1,2,3,7,16,31,64,192,512])
        ids = rng.sample(range(1, 2**31), n)
        if n: ids[-1] = UINT_MAX
        pool = [particle(ids[i],rng.choice(choices),
                         (rng.randrange(2),rng.randrange(2),rng.choice([1,2,2**40]),
                          rng.choice([1,2]),rng.randrange(3)),i,rng.random()>.15)
                for i in range(n)]
        # Include dense same-group random cases, not only easy dispersed groups.
        if iteration % 3 == 0:
            for p in pool: p['group'] = (0,1,1,1,0)
        check(pool, rng.choice([0.,math.ulp(0.),.2,.5,1.,1e308]))
    for n in [192,512]:
        check([particle(n-i,float(i)*2,slot=i) for i in range(n)],.5)  # no pair
        check([particle(n-i,1.,slot=i) for i in range(n)],0.)        # all equal
    malformed = [
        ([particle(7,0.,slot=0),particle(7,0.,(1,2,3,1,1),1)],1.),
        ([particle(0,0.,slot=0)],1.),
        ([particle(2**32,0.,slot=0)],1.),
        ([particle(1,float('nan'),slot=0)],1.),
        ([particle(1,0.,slot=0)],-1.),
        ([particle(1,0.,slot=0)],float('inf')),
    ]
    for pool,radius in malformed:
        try: prepare(pool,radius)
        except ValueError: COUNTS['malformedRejected'] += 1
        else: raise AssertionError('malformed accepted')
    pool = [particle(i+1,y,slot=i) for i,y in enumerate([0.,.4,.65,.9])]
    serial_pairs = []
    for _ in range(3):
        pair = check(pool,.5)
        if pair is None: break
        serial_pairs.append(pair[:2])
        aa = next(p for p in pool if p['id']==pair[0] and p['active'])
        bb = next(p for p in pool if p['id']==pair[1] and p['active'])
        mass = aa['mass']+bb['mass']
        aa['y']=(aa['y']*aa['mass']+bb['y']*bb['mass'])/mass
        aa['mass']=mass;bb['active']=False;bb['mass']=0.
        COUNTS['continuousMergeSelections'] += 1
    assert serial_pairs[:2] == [(1,2),(1,3)]
    paths = ['docs/architecture-T130-gpu-compute-route-v1.md',
             'docs/architecture-T136-compute-arrival-decision-v1.md',
             'docs/architecture-T116-pool-recurrence-index-routing-v1.md',
             'docs/test-report-T131-compute-capability-v1.md',
             'test/reports/t131/t131_stats_summary.json',
             'test/reports/t131/t131.json','test/reports/t131/t131_run5.json',
             'test/reports/t131/compute_shader.glsl',
             'test/p013_native/t131_compute_probe.cpp',
             'entry/src/main/ets/physics/P011ParticleHot.ets',
             'docs/design-T137-gpu-pair-model-v1.py']
    costs=[]
    for n in [192,512]:
        k=1 << (n-1).bit_length();h=k.bit_length()-1
        reductions=0;length=k
        while length>1: length=(length+63)//64;reductions+=1
        costs.append(dict(Nmax=n,K=k,h=h,buildComparisonUpper=k*h*(h+1)//2,
                          tableBytes=16*k*(h+1),slotPayloadBytes=16*k*(h+1)+96*k+384,
                          dualSlotPayloadBytes=2*(16*k*(h+1)+96*k+384),
                          uploadUpperBytes=48*k+384,readbackBytes=128,
                          dispatches=h+1+reductions,fencesPerRequest=1))
    evidence=dict(revision='T137_DESIGN_DISCRETE_MODEL_V1',date='2026-10-26',
                  route='P011_GPU_PAIR_PROTOTYPE_V1',
                  kind='designer finite algorithm/model checks; NOT GLES/product/device tests',
                  inputSha256={p:hashlib.sha256((ROOT/p).read_bytes()).hexdigest() for p in paths},
                  counts=COUNTS,finiteExhaustivePools=exhaustive,randomSeed=137,
                  boundaryCounterexample=dict(a=a,b=b,radius=r,originalPredicate=True,
                    roundedEndpointPredicate=False,
                    bits={key:struct.pack('>d',v).hex() for key,v in [('a',a),('b',b),('radius',r)]}),
                  continuousMergeSyntheticPairSequence=serial_pairs,costs=costs,
                  notClaimed=['GLES layer/barrier/reduction correctness verified',
                              'full transaction/rollback equivalence verified',
                              'target-device speedup','GPU production migration','freeze fixed'])
    out=ROOT/'docs/design-T137-gpu-pair-evidence-v1.json'
    out.write_text(json.dumps(evidence,ensure_ascii=False,indent=2)+'\n',encoding='utf-8')
    print(json.dumps(dict(counts=COUNTS,exhaustive=exhaustive,costs=costs),ensure_ascii=False))


if __name__ == '__main__':
    main()
