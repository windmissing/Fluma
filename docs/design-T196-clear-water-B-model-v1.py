"""Designer arithmetic/cost model for T196. Not product, integration or GPU tests.

Uses exact integers/Fraction only here, not as proposed runtime dependencies.
Writes docs/design-T196-clear-water-B-evidence-v1.json; no other files modified.
"""
from fractions import Fraction as F
from pathlib import Path
import hashlib
import json
import math
import random
import struct

ROOT = Path(__file__).resolve().parent.parent
WIDTH = 70
BASE = 1 << 16
MASK = (1 << (WIDTH*16))-1


def encode_mass(x):
    assert math.isfinite(x) and 0 <= x <= 256
    bits = struct.unpack('>Q', struct.pack('>d', x))[0]
    exponent = (bits >> 52) & 2047
    fraction = bits & ((1 << 52)-1)
    # Both zero encodings map to zero; runtime stored-sign preservation separate.
    return fraction if exponent == 0 else ((1 << 52) | fraction) << (exponent-1)


def limbs(n):
    assert 0 <= n <= MASK
    return [(n >> (16*i)) & 65535 for i in range(WIDTH)]


def integer(xs):
    return sum(v << (16*i) for i, v in enumerate(xs))


def add(a, b):
    out = []
    carry = 0
    for aa, bb in zip(a, b):
        v = aa+bb+carry
        assert 0 <= v <= 131071
        out.append(v & 65535)
        carry = v >> 16
    assert carry == 0
    return out


def subtract(a, b):
    out = []
    borrow = 0
    for aa, bb in zip(a, b):
        v = aa-bb-borrow
        borrow = 1 if v < 0 else 0
        out.append(v+BASE if borrow else v)
    if borrow:
        raise ValueError('NEGATIVE_MASS_ACCUMULATOR')
    return out


def check_accumulator():
    rng = random.Random(196)
    fixed = [0., -0., math.ulp(0.), math.nextafter(0., 1.), 2.**-1022,
             2.**-53, 2.**-54, 1., math.nextafter(256., 0.), 256.]
    cases = 0
    for x in fixed:
        a = encode_mass(x)
        assert F(a, 1 << 1074) == F(x)
        assert integer(limbs(a)) == a
        cases += 1
    for _ in range(500):
        # Finite single-mass support domain, including very small exponents.
        x = math.ldexp(rng.random(), rng.randrange(-1073, 9))
        y = math.ldexp(rng.random(), rng.randrange(-1073, 9))
        ax, ay = encode_mass(x), encode_mass(y)
        assert F(ax, 1 << 1074) == F(x)
        assert integer(add(limbs(ax), limbs(ay))) == ax+ay
        assert integer(subtract(limbs(ax+ay), limbs(ax))) == ay
        cases += 1
    acc = limbs(0)
    for _ in range(512):
        acc = add(acc, limbs(encode_mass(256.)))
    assert integer(acc) == 512*encode_mass(256.)
    for _ in range(512):
        acc = subtract(acc, limbs(encode_mass(256.)))
    assert integer(acc) == 0
    try:
        subtract(limbs(0), limbs(1))
    except ValueError:
        negative_rejected = True
    else:
        raise AssertionError('negative subtraction accepted')
    return dict(finiteEncodeAddSubtractCases=cases,
                repeatedMaximumMassAdds=512, repeatedMaximumMassSubtracts=512,
                negativeRejected=negative_rejected,
                limbCount=WIDTH, limbBits=16, totalBits=WIDTH*16,
                physicalScaleExponent=-1074,
                maximumSupportedRealExclusivePower=WIDTH*16-1074)


def water_example(name, sd, wd, pd, sr, wr, pr, cap, kdt, u=0):
    sd,wd,pd,sr,wr,pr,cap,kdt,u=map(F,(sd,wd,pd,sr,wr,pr,cap,kdt,u))
    zd,zr = sd,sr-u
    hd,hr=zd+wd,zr+wr
    delta=hd-hr
    q=F(0) if delta <= 0 else min(kdt*delta,delta/2,wd,
                                  max(F(0),hd-max(zd,zr)),max(F(0),cap-sr-wr-pr))
    newd,newr=wd-q,wr+q
    assert newd>=0 and newr>=0 and newr+sr+pr<=cap
    assert newd+newr==wd+wr
    de=-q*delta+q*q
    assert de<=0
    return dict(name=name,q=str(q),waterAfter=[str(newd),str(newr)],
                realEnergyDelta=str(de),kind='real-arithmetic illustration, not RN product test')


def budget_examples():
    dt=F(1,60)
    total=300*dt
    local=[28*dt]*3
    records=[]
    # LAND, cross (two local endpoints), bed; no additional issue after refund.
    for kind,cols,request,guard_cap in [
        ('LAND',[0],F(1,4),F(1,8)),
        ('CROSS',[0,1],F(1,4),F(1,4)),
        ('BED',[1,2],F(1,2),F(1,10))]:
        reserved=min(request,total,*(local[c] for c in cols))
        # Cross is all-or-nothing, unlike land/bed.
        q=(request if reserved==request else F(0)) if kind=='CROSS' else min(reserved,guard_cap)
        refund=reserved-q
        total-=reserved
        for c in cols:local[c]-=reserved
        total+=refund
        for c in cols:local[c]+=refund
        assert total>=0 and all(v>=0 for v in local)
        records.append(dict(kind=kind,columns=cols,request=str(request),reserved=str(reserved),
                            accepted=str(q),refundOnce=str(refund),remainingTotal=str(total),
                            remainingLocal=list(map(str,local))))
    used=sum(F(r['accepted']) for r in records)
    assert total+used==300*dt
    return records


def cost_config(k, a_payload=None):
    n,e=128,8
    water=bed=n-1
    events=e*k+water+bed
    parts=dict(replayState=63*k+80*n,originWaitFiveBanks=45*k,
               medium=9*n,exactAcc=280*n+288,budget=48*n+256,
               doubleParticleSummaries=256*k,doubleEvent40=640*events,
               doubleLineage3=96*events,scalarHeaders=2048)
    return dict(N=n,K=k,e=e,hydraulicEdges=water,bedEdges=bed,
                particleEventUpper=e*k,eventRowsPerBank=events,lineageRowsPerBank=2*events,
                maxFullAuditsPerPositiveStep=3,physicsCandidates=1,lambdaRetries=0,
                perBirthMergeOrIndexRebuild=0,proposedBPayloadParts=parts,
                proposedBPayloadBytes=sum(parts.values()),
                coldAttachIntervalBytes=16*4096,
                proposedBPeakIncludingColdAttach=sum(parts.values())+16*4096,
                observedABytes=a_payload,
                proposedAplusBHotBytes=(a_payload+sum(parts.values())) if a_payload else None,
                proposedAplusBPeakBytes=(a_payload+sum(parts.values())+16*4096) if a_payload else None)


def main():
    acc=check_accumulator()
    tiny=2.**-53
    masses=[1.,tiny,tiny]
    folded=0.
    for m in masses:folded+=m
    incremental=folded-1.
    refolded=0.
    for m in masses[1:]:refolded+=m
    assert incremental!=refolded
    exact_at_capacity=F(256.)+F(2.**-54)
    assert 256.+2.**-54==256. and exact_at_capacity>F(256)
    allocation=json.loads((ROOT/'test/reports/t188/allocation-ledger.json').read_text(encoding='utf-8'))
    actual_a=allocation['payloadBytes']
    assert actual_a==2802784
    sources=[
        'docs/algorithm-single-sand-clear-water-v1.md',
        'docs/algorithm-T122-single-sand-clear-water-handoff-v1.md',
        'docs/architecture-single-sand-clear-water-v1.md',
        'docs/developer-T188-clear-water-source-v1.md',
        'docs/test-report-T189-clear-water-A-independent-v1.md',
        'docs/test-report-single-sand-clear-water-baseline-v1.md',
        'test/reports/t189/independent.json','test/reports/t188/allocation-ledger.json',
        'entry/src/main/ets/physics/clearwater/P011ClearWaterTypes.ets',
        'entry/src/main/ets/physics/clearwater/P011ClearWaterState.ets',
        'entry/src/main/ets/physics/clearwater/P011ClearWaterSource.ets',
        'entry/src/main/ets/physics/clearwater/P011ClearWaterEngine.ets',
        'entry/src/main/ets/physics/P011MaterialGuard.ets',
        'docs/design-T196-clear-water-B-model-v1.py']
    report=json.loads((ROOT/'test/reports/t189/independent.json').read_text(encoding='utf-8'))
    assert report['total']==report['passed']==15 and report['failed']==0
    evidence=dict(revision='T196_DESIGN_ARITHMETIC_AND_COST_V1',date='2026-10-28',
        kind='designer arithmetic/cost/source reading only; not product, GPU or device tests',
        route='SINGLE_SAND_CLEAR_WATER_V1',supplement='CW_B_STEP_DESIGN_V1',
        inputSha256={p:hashlib.sha256((ROOT/p).read_bytes()).hexdigest() for p in sources},
        inheritedAReport=dict(total=15,passed=15,failed=0,rerun=False,
                              scope='A inventory/source only; B/C/restore NOT_IMPLEMENTED'),
        accumulatorModel=acc,
        incrementalCacheCounterexample=dict(masses=masses,originalIDfold=folded,
            afterRemovingFirstIncremental=incremental,afterRemovingFirstIDfold=refolded,
            conclusion='do not replace canonical ID-order reduction by incremental +/-'),
        roundedCapacityCounterexample=dict(S=0,W=0,masses=[256.,2.**-54],
            IDfold=256,roundedR=256,exactE=str(exact_at_capacity),
            boundPolicy='freeze Bcap=max(C,Eentry) as auxiliary only; final original R<=C remains mandatory'),
        waterExamples=[water_example('wet-to-dry',1,4,0,1,0,0,10,F(1,4)),
                       water_example('high-bed-wall',1,2,0,4,0,0,10,F(1,4)),
                       water_example('particle-reserved-receiver',1,6,0,1,1,7,10,1),
                       water_example('tilt',1,3,0,1,3,0,10,F(1,2),F(1,2))],
        exactBudgetExamples=budget_examples(),
        postBedContactCases=[
            dict(name='bed rises into a live point',yBefore=.3,newBedS=.4,
                 yAfter=.4,stateAfter='BED_WAIT',massChange=0,waterChange=0,
                 sourceChange=0,extraEventRows=1,timeAdvanced=0),
            dict(name='support falls away',yBefore=.4,newBedS=.2,
                 yAfter=.4,stateAfter='AIRBORNE',massChange=0,waterChange=0,
                 sourceChange=0,extraEventRows=1,timeAdvanced=0)],
        eventResourcePolicy=dict(perParticleLimit=8,motionLoopLimit=7,
                                  postBedContactReserved=1,
                                  summaryDoesNotPromiseContiguousPerParticleEvents=True),
        proposedCostAndPayload=[cost_config(192),cost_config(512,actual_a)],
        aggregation=dict(sourceBoundariesDefault=1,sourceQueueMaxPerBoundary=64,
                         frameSubstepsMax=2,formula='enqueue_sum + source_boundaries_sum + substeps_sum(V*Tstep) + C_when_implemented',
                         singleBoundaryBirthUpper='entry freeCount <= K',
                         noClaimThatFrameBirthsAlwaysLeK=True),
        notClaimed=['B code implemented','new exact helper independently tested in product',
                    'full step or restore validated','C renderer/touch enabled','device performance pass',
                    'aggregate latency reduced','overall strict-state route enabled'])
    out=ROOT/'docs/design-T196-clear-water-B-evidence-v1.json'
    out.write_text(json.dumps(evidence,ensure_ascii=False,indent=2)+'\n',encoding='utf-8')
    print(json.dumps(dict(accumulator=acc,cacheCounterexample=evidence['incrementalCacheCounterexample'],
                         costs=evidence['proposedCostAndPayload']),ensure_ascii=False))


if __name__=='__main__':
    main()
