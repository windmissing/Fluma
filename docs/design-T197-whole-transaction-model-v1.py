"""Designer discrete proof/cost checks for T197, NOT product/AVL/GPU tests.

Sorted lists implement the mathematical reference here. No production module
or test runner is imported. This checks adjacency/min-pair/local invalidation
claims, not an implementation's AVL rotations, latency or memory use.
"""
from pathlib import Path
import bisect
import hashlib
import itertools
import json
import math
import random

ROOT = Path(__file__).resolve().parent.parent
MAXID = 2**32-1
COUNTS = dict(poolCases=0, eligibilityChecks=0, localUpdates=0,
              sequentialPairs=0, invalidInputsRejected=0)


def point(i, y, g=(0,0,1,1,0), slot=0, mass=1.):
    return dict(id=i,y=y,g=g,slot=slot,mass=mass)


def key(p):
    return p['g']+(p['y'],p['id'],p['slot'])


def valid(pool, radius):
    if not math.isfinite(radius) or radius<0:
        raise ValueError('unsupported radius')
    if (len({p['id'] for p in pool})!=len(pool) or
        len({p['slot'] for p in pool})!=len(pool)):
        raise ValueError('duplicate identity')
    for p in pool:
        if not 1<=p['id']<=MAXID or not math.isfinite(p['y']):
            raise ValueError('unsupported identity/height')


def legal(a,b,r):
    return a['g']==b['g'] and abs(a['y']-b['y'])<=r


def all_eligible(rows, r):
    result={}
    for i,p in enumerate(rows):
        result[p['id']]=((i>0 and legal(p,rows[i-1],r)) or
                         (i+1<len(rows) and legal(p,rows[i+1],r)))
    return result


def oracle(pool,r):
    best=None
    for a in pool:
        for b in pool:
            if a['id']<b['id'] and legal(a,b,r):
                candidate=(a['id'],b['id'],a['slot'],b['slot'])
                if best is None or candidate<best:best=candidate
    return best


def adjacency_pair(pool,r,eligibility=None):
    valid(pool,r)
    rows=sorted(pool,key=key)
    seen=all_eligible(rows,r)
    if eligibility is not None:assert eligibility==seen
    candidates=[p for p in rows if seen[p['id']]]
    for p in rows:
        assert seen[p['id']]==any(q['id']!=p['id'] and legal(p,q,r) for q in pool)
        COUNTS['eligibilityChecks']+=1
    if not candidates:return None
    a=min(candidates,key=lambda p:p['id'])
    position=rows.index(a)
    partners=[p for j,p in enumerate(rows) if j!=position and legal(a,p,r)]
    b=min(partners,key=lambda p:p['id'])
    assert a['id']<b['id']
    return (a['id'],b['id'],a['slot'],b['slot'])


def check(pool,r,eligibility=None):
    result=adjacency_pair(pool,r,eligibility)
    assert result==oracle(pool,r)
    COUNTS['poolCases']+=1
    return result


class LocalModel:
    def __init__(self,pool,r):
        valid(pool,r)
        self.rows=sorted([dict(p) for p in pool],key=key)
        self.r=r
        self.eligibility=all_eligible(self.rows,r)

    def update_near(self,indices):
        # Only immediate old/new neighbors may change, even across groups.
        for i in indices:
            if not 0<=i<len(self.rows):continue
            p=self.rows[i]
            self.eligibility[p['id']]=((i>0 and legal(p,self.rows[i-1],self.r)) or
                (i+1<len(self.rows) and legal(p,self.rows[i+1],self.r)))

    def erase(self,i):
        where=next(j for j,p in enumerate(self.rows) if p['id']==i)
        old=self.rows.pop(where)
        del self.eligibility[i]
        self.update_near([where-1,where])
        assert self.eligibility==all_eligible(self.rows,self.r)
        COUNTS['localUpdates']+=1
        return old

    def insert(self,p):
        assert p['id'] not in self.eligibility
        where=bisect.bisect_left([key(x) for x in self.rows],key(p))
        self.rows.insert(where,dict(p))
        self.update_near([where-1,where,where+1])
        assert self.eligibility==all_eligible(self.rows,self.r)
        COUNTS['localUpdates']+=1

    def find(self):return check(self.rows,self.r,self.eligibility)


def model_checks():
    exhaustive=0
    for n in range(5):
        ids=[MAXID,1,2**31,2][:n]
        for ys in itertools.product((-1.,0.,1.),repeat=n):
            for groups in itertools.product((0,1),repeat=n):
                ps=[point(ids[i],ys[i],(groups[i],0,1,1,0),i) for i in range(n)]
                for r in [0.,.5,1.]:
                    check(ps,r);exhaustive+=1
    # Witness need not be the lexicographically chosen partner.
    witness=[point(1,0.,slot=0),point(100,.1,slot=1),point(2,.2,slot=2)]
    assert check(witness,.5)[:2]==(1,2)
    boundary=[point(1,.2,slot=0),point(MAXID,-math.ulp(0.),slot=1)]
    assert check(boundary,.2) is not None
    assert not .2-.2<=-math.ulp(0.)
    for ps,r in [([point(1,-0.,slot=0),point(2,0.,slot=1)],0.),
                 ([point(1,-1e308,slot=0),point(2,1e308,slot=1)],1e308)]:
        check(ps,r)
    rng=random.Random(197)
    values=[-1e308,-1.,-.2,-math.ulp(0.),-0.,0.,math.ulp(0.),.2,
            math.nextafter(.2,math.inf),.7,1.,1e308]
    for n in [2,4,16,64,192,512]:
        ps=[point(i+1,rng.choice(values),(rng.randrange(2),0,1,1,0),i)
            for i in range(n)]
        m=LocalModel(ps,.5)
        m.find()
        for mutation in range(24):
            chosen=rng.choice(m.rows)
            p=m.erase(chosen['id'])
            if mutation%3!=0:
                p['y']=rng.choice(values)
                p['g']=(rng.randrange(2),0,1,rng.choice([1,2]),rng.randrange(2))
            m.insert(p)
            m.find()
    serial=LocalModel([point(i+1,y,slot=i) for i,y in enumerate([0.,.4,.65,.9])],.5)
    sequence=[]
    while True:
        pair=serial.find()
        if pair is None:break
        a=serial.erase(pair[0]);b=serial.erase(pair[1])
        mass=a['mass']+b['mass']
        a['y']=(a['y']*a['mass']+b['y']*b['mass'])/mass
        a['mass']=mass;serial.insert(a)
        sequence.append(list(pair[:2]));COUNTS['sequentialPairs']+=1
    assert sequence[:2]==[[1,2],[1,3]]
    for ps,r in [([point(0,0.)],1.),([point(1,0.,slot=0),point(1,1.,slot=1)],1.),
                 ([point(1,float('nan'))],1.),([point(1,0.)],-1.)]:
        try:valid(ps,r)
        except ValueError:COUNTS['invalidInputsRejected']+=1
        else:raise AssertionError('malformed proof input accepted')
    # This reject is MODEL-domain only; product preserves original cutpoint.
    return dict(finiteExhaustivePools=exhaustive,randomSeed=197,
                counts=COUNTS,serialMergePairs=sequence,
                witnessPartnerCounterexample=dict(heights=[0.,.1,.2],ids=[1,100,2],
                    pair=[1,2],nearestWitnessId=100),
                malformedScope='model validity only, NOT proposed early product rejection')


def structural_bounds(n):
    h=math.ceil(2*math.log2(n+1));r=math.ceil(math.log2(n+1))
    return dict(N=n,avlHeightUpper=h,rankSearchRounds=r,
                findTreeNodeBudgetExcludingHeader=6*r*h+4*h,
                primitiveKeyUpdateBudgetExcludingHeader=64*h,
                pairAndIdTreesPayloadPerView=96*n)


def workload(c,n,b,r,v,s):
    # Illustrative source-only envelope; NOT counts inferred from T156.
    # Deliberately varies B independently of N; no fixed B=N assumption.
    j=2*c*r
    m=b
    a=3*j
    u=3*(b+m)
    logs=math.ceil(math.log2(n+1))
    return dict(kind='symbolic envelope illustration, not executed physical workload',
        C=c,N=n,B_ent=b,M_find=m,R=r,V=v,S=s,J_source_upper=j,
        seedAuditLogicalUpper=a,
        baselineFindScale=m*(n+n*logs),
        # A scale, not instruction count and not latency.
        newPairScale=n+n*logs+u*logs+m*logs*logs,
        retainedAuditFoldScale=a*(n+c),
        etaColumnUpperPerSource=64*c*r,
        perFieldChainWaterReceiverUpper=c*c,
        frameSubstepUpper=s,previewUpperPerSubstep=v,
        caution='does not multiply J/M/frame totals by R/V/S again')


def nested_undo():
    state={'mass':1.,'sourceEpoch':0}
    outer=dict(state)
    state['mass']=2.;state['sourceEpoch']=1  # a successful SOURCE prefix
    after_source=dict(state)
    inner=dict(state)
    state['mass']=3.
    state.update(inner)  # failed STEP: preserve source success
    assert state==after_source and state!=outer
    # Nested transaction first-touch needs an old value for each frame.
    return dict(initial=outer,successfulSource=after_source,afterFailedStep=state,
                sameAddressWriteValues=[1.,2.,3.],
                conclusion='one outer old-value is insufficient for inner rollback')


def main():
    proof=model_checks()
    seq=0.
    for x in [1.,2.**-53,2.**-53]:seq+=x
    rebracket=1.+(2.**-53+2.**-53)
    assert seq!=rebracket
    paths=[
        'docs/architecture-T116-T156-whole-transaction-v3.md',
        'docs/analysis-T116-T156-arrival-v3.json',
        'docs/developer-T123-merge-exact-index.md',
        'docs/tester-T128-merge-index-differential-v1.md',
        'docs/architecture-T130-gpu-compute-route-v1.md',
        'docs/algorithm-T137-gpu-deterministic-pair-v1.md',
        'docs/architecture-T183-gpu-cost-next-v1.md',
        'entry/src/main/ets/physics/P011ParticleHot.ets',
        'entry/src/main/ets/physics/P011ParticleM2Field.ets',
        'entry/src/main/ets/physics/P011ParticleM2Sink.ets',
        'entry/src/main/ets/physics/P011ParticleHotBudget.ets',
        'entry/src/main/ets/physics/P011ParticleField.ets',
        'entry/src/main/ets/physics/P011MergeExactIndex.ets',
        'entry/src/main/ets/physics/WaterSandField.ets',
        'entry/src/main/ets/physics/P011RelaxCorrection.ets',
        'entry/src/main/ets/physics/P011TiltPlanningPass.ets',
        'entry/src/main/ets/physics/P011NaturalObservation.ets',
        'entry/src/main/ets/physics/P011NaturalObservationStore.ets',
        'entry/src/main/ets/physics/P011EndpointMetrics.ets',
        'entry/src/main/ets/physics/P011PeakMetrics.ets',
        'docs/design-T197-whole-transaction-model-v1.py']
    evidence=dict(revision='T197_DESIGN_DISCRETE_PROOF_AND_ENVELOPE_V1',date='2026-10-28',
        route='P011_WHOLE_TRANSACTION_V1',
        kind='designer finite mathematical model/cost reasoning, NOT product/AVL/GPU/device tests',
        rootCauseBaseline='T116 V3 only; R1/R2 actual N/B/M etc remain NOT_CAPTURED',
        inputSha256={p:hashlib.sha256((ROOT/p).read_bytes()).hexdigest() for p in paths},
        adjacencyModel=proof,structureBudgets=[structural_bounds(n) for n in [192,512]],
        orderedFoldCounterexample=dict(operands=[1.,2.**-53,2.**-53],
            originalSequential=seq,rebracketed=rebracket,
            conclusion='dirty numerical audit must retain original F64 fold, no sum associativity'),
        sourceThenStepRollbackModel=nested_undo(),
        illustrativeWorkloads=[workload(128,192,64,1,1,1),workload(128,512,128,2,3,2),
                               workload(128,512,1024,8,3,2)],
        queuePayloadExample=dict(queueSlots=16,C=128,headerBytes=256,
                                  maxInputPayloadBytesPerSlot=2304,queuePayloadBytes=36864,
                                  status='proposed resource profile, decision required'),
        wholeChainScopes=['direct-source-outside-frame','queued-source','source-lambda',
            'lift-collect-no-birth','lift-apply','seed','birth','merge-find','merge-commit',
            'pair-index-build-and-update','ID-lookup','ticket-reserve-refund-close',
            'collect-A-B-headers','audit-logical-execute-cert','pool-field-copy-undo',
            'planner-preview','authoritative','consistency-mirror','field-advection',
            'field-settle-percolate-bottom-deep-relax-correction','water-level-40',
            'injection-eta-64','guard-and-dirty-S','peak-endpoint-natural-observation',
            'immutable-publication','render-and-scene-preparation','GPU-exact-prepare',
            'GPU-SHA-pack-bridge-upload-dispatch-barrier-fence-map-consume',
            'JSON-encode-message-write-fsync-fence','owner-FIFO-backpressure-cancel'],
        sourceReadFacts=dict(particleMassAt='O(1) column-cache read',
            poolTotals='O(C+N) including resting scan',
            correctionRatioInnerScan='executes once then break; not quadratic by nesting alone',
            dirtyLegacyAuditReduction='slot order, not ID order or tree sum'),
        unchangedPolicy=dict(numericAuditsAtLogicalCutpoints=True,actualOrderedFoldRetained=True,
            massRadiusAndF64NotQuantized=True,birthsMergedInOriginalSerialOrder=True),
        requiresExplicitBehaviorVersion=['async pending/enqueue ACK API','atomic work admission rejection',
                                        'queue/backpressure/cancel/deadline policy'],
        pending=['device service rates/deadlines/work cap','complete allocation envelope choices',
                 'architect engineering of AVL/cert/write barriers','T198 independent oracle',
                 'product migration/whole-chain device stability and benefit'],
        notClaimed=['new AVL implementation tested','field/physics product equivalence verified',
                    'all audits eliminated','whole frame linear','actual failing transaction replayed',
                    'GPU production migrated','O2/device performance pass','freeze fixed'])
    out=ROOT/'docs/design-T197-whole-transaction-evidence-v1.json'
    out.write_text(json.dumps(evidence,ensure_ascii=False,indent=2)+'\n',encoding='utf-8')
    print(json.dumps(dict(adjacency=proof,structureBudgets=evidence['structureBudgets']),ensure_ascii=False))


if __name__=='__main__':
    main()
