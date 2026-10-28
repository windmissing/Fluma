"""T207 designer exact arithmetic/domain model, not product or device tests.

No production imports. Exact integers/Fraction are independent design oracles.
Writes only docs/design-T207-dry-derived-evidence-v1.json.
"""
from fractions import Fraction as F
from pathlib import Path
import hashlib
import json
import math
import random
import struct

ROOT=Path(__file__).resolve().parent.parent
LIMBS=70
SCALE=1<<1074
COUNTS=dict(encodeRoundCases=0,memberMutations=0,nonnegativeProfileCases=0,
            historicalArrayHashesVerified=0)


def bits(x):return struct.pack('>d',x).hex()


def encode(x):
    assert math.isfinite(x) and 0<=x<=256
    raw=struct.unpack('>Q',struct.pack('>d',x))[0]
    exp=(raw>>52)&2047;frac=raw&((1<<52)-1)
    return frac if exp==0 else ((1<<52)|frac)<<(exp-1)


def words(n):
    assert 0<=n<1<<(16*LIMBS)
    return [(n>>(16*i))&65535 for i in range(LIMBS)]


def integer(a):return sum(x<<(16*i) for i,x in enumerate(a))


def add(a,b):
    out=[];carry=0
    for x,y in zip(a,b):
        v=x+y+carry;assert 0<=v<=131071
        out.append(v&65535);carry=v>>16
    if carry:raise ValueError('ACC_OVERFLOW')
    return out


def subtract(a,b):
    out=[];borrow=0
    for x,y in zip(a,b):
        v=x-y-borrow;borrow=int(v<0)
        out.append(v+65536 if borrow else v)
    if borrow:raise ValueError('ACC_NEGATIVE')
    return out


def round_n(n):
    # Mirrors the proposed bit-level conversion, not Python float(Fraction).
    assert 0<=n<1<<(16*LIMBS)
    if n==0:return 0.
    if n<1<<52:
        raw=n
    else:
        shift=max(0,n.bit_length()-53)
        mant=n>>shift
        if shift:
            rem=n-(mant<<shift);half=1<<(shift-1)
            if rem>half or (rem==half and mant&1):mant+=1
        if mant==1<<53:mant>>=1;shift+=1
        exponent=shift+1
        if exponent>=2047:raise ValueError('ROUND_OVERFLOW')
        raw=(exponent<<52)|(mant-(1<<52))
    result=struct.unpack('>d',struct.pack('>Q',raw))[0]
    oracle=float(F(n,SCALE))
    assert bits(result)==bits(oracle),(n,result,oracle)
    assert result>=0 and (n==0 or result>0)
    COUNTS['encodeRoundCases']+=1
    return result


def arithmetic_cases():
    rng=random.Random(207)
    inputs=[0.,-0.,math.ulp(0.),math.nextafter(2.**-1022,0.),2.**-1022,
            2.**-53,2.**-54,.1,.2,.3,1.,math.nextafter(256.,0.),256.]
    for x in inputs:
        n=encode(x);assert F(n,SCALE)==F(x)
        assert bits(round_n(n))==bits(0. if x==0 else x)
    for mass_list in [[.5,2.**-54],[math.nextafter(.5,1.),2.**-54],
                      [1.,2.**-54,2.**-53],[math.ulp(0.)]*512,[256.]*512]:
        n=sum(encode(v) for v in mass_list);round_n(n)
    for _ in range(600):
        vals=[math.ldexp(rng.random(),rng.randrange(-1073,9)) for _ in range(rng.randrange(1,9))]
        n=sum(encode(v) for v in vals);round_n(n)
    try:subtract(words(0),words(1))
    except ValueError:pass
    else:raise AssertionError('negative accepted')


def membership_cases():
    rng=random.Random(207)
    members={}
    acc={}
    def bucket(k):return acc.setdefault(k,words(0))
    for iteration in range(2500):
        slot=rng.randrange(512)
        old=members.pop(slot,None)
        touched=set()
        if old:
            c,phase,m=old;k=(c,phase)
            acc[k]=subtract(bucket(k),words(encode(m)));touched.add(k)
        if iteration%5:
            m=rng.choice([math.ulp(0.),2.**-54,2.**-53,.1,.2,.3,1.,128.,256.])
            new=(rng.randrange(128),rng.randrange(2),m)
            members[slot]=new;c,phase,m=new;k=(c,phase)
            acc[k]=add(bucket(k),words(encode(m)));touched.add(k)
        for k in touched:
            independent=sum((F(v[2]) for v in members.values() if v[:2]==k),F(0))
            assert F(integer(acc[k]),SCALE)==independent
            actual=round_n(integer(acc[k]));assert bits(actual)==bits(float(independent))
        COUNTS['memberMutations']+=1
    return dict(finalLiveCount=len(members),mutationCount=2500,
                scope='synthetic stored-mass membership/phase/column changes, not product writers')


def old_negative_example():
    masses=[1.,2.**-54,2.**-53]
    old=0.;acc=words(0);records=[]
    for m in masses:
        old+=m;acc=add(acc,words(encode(m)))
        records.append(dict(op='birth',mass=m,legacy=old,v2=round_n(integer(acc))))
    for m in masses:
        old-=m;acc=subtract(acc,words(encode(m)))
        records.append(dict(op='retire',mass=m,legacy=old,v2=round_n(integer(acc))))
    assert old==-3*2.**-54 and integer(acc)==0 and bits(round_n(0))=='0000000000000000'
    return dict(scope='synthetic mechanism example, NOT reconstructed T202 writer history',
                records=records,legacyFinalBits=bits(old),v2FinalBits=bits(round_n(0)))


def domain_cases():
    rng=random.Random(207)
    for _ in range(500):
        profiles=[[],[],[],[]]  # top/envelope/bottom/mix
        for c in range(8):
            bh,bl,mh,ml=[rng.random()*8 for _ in range(4)]
            ph,pl=[rng.random()*16 for _ in range(2)]
            mix=bh+bl;top=(mix+mh)+ml;env=((top+0.)+0.)+(ph+pl)
            assert 0<=bh<=mix<=top<=env<=256
            for arr,value in zip(profiles,[top,env,bh,mix]):arr.append(value)
        for pass_ in range(4):
            profiles=[[(p[max(0,c-1)]+p[c]+p[min(7,c+1)])/3 for c in range(8)] for p in profiles]
        for c in range(8):
            top,env,bh,mix=[p[c] for p in profiles]
            assert 0<=bh<=mix<=top<=env<=256
            top,env,bh,mix=[struct.unpack('f',struct.pack('f',v))[0] for v in [top,env,bh,mix]]
            assert 0<=bh<=mix<=top<=env<=256
        COUNTS['nonnegativeProfileCases']+=1
    ghost=((.1+.3)-.1)-.3
    assert ghost>0
    u=F(1,1<<53);gamma=512*u/(1-512*u)
    error_bound=(gamma+u)*257
    assert error_bound<F(15,10**12) and error_bound<F(1,10**9)
    return dict(profileCases=500,ghostWaterExample=dict(top=.1,P=.3,envelope=.4,
                oldInferredWater=ghost,v2DefinedDryWater=0.),
                slotFoldDifferenceUpperForSupportedT=str(error_bound),
                slotFoldDifferenceUpperFloat=float(error_bound),oldCacheAuditAbsTolerance=1e-9)


def actual_writer_mass_examples():
    a,b=.1,.2
    old_exact=encode(a)+encode(b)
    merged=a+b
    acc=subtract(subtract(words(old_exact),words(encode(a))),words(encode(b)))
    acc=add(acc,words(encode(merged)))
    assert integer(acc)==encode(merged) and F(merged)!=F(a)+F(b)
    m,q=1.,2.**-54
    remainder=m-q
    # Actual remainder has not changed: no amount-based subtraction is allowed.
    acc=subtract(words(encode(m)),words(encode(m)))
    acc=add(acc,words(encode(remainder)))
    assert integer(acc)==encode(m) and encode(m)-encode(q)!=encode(remainder)
    return dict(merge=dict(oldMasses=[a,b],newStoredMass=merged,
                 oldExactSum=str(F(a)+F(b)),newExactSum=str(F(merged)),
                 cacheAfter=round_n(integer(words(encode(merged))))),
                partialLandNoMassEffect=dict(mass=m,controlQ=q,actualRemainder=remainder,
                    derivedMassAfter=round_n(integer(acc)),
                    conclusion='update membership from actual before/after; do not subtract control q'))


def historical_capture():
    p=ROOT/'docs/analysis-T202-reassembled-material-v1.json'
    e=json.loads(p.read_text(encoding='utf-8'))
    values={}
    for a in e['arrays']:
        data=bytes.fromhex(a.get('rawHex',''))
        if a.get('sha256') is not None:
            assert hashlib.sha256(data).hexdigest()==a['sha256']
            COUNTS['historicalArrayHashesVerified']+=1
        if a['name'] in ['sandTop','waterTop','colParticleMassH','colParticleMassL']:
            values[a['name']]=struct.unpack_from('<d',data,57*8)[0]
    assert values['sandTop']==0 and values['waterTop']==values['colParticleMassH']==-3*2.**-54
    assert values['colParticleMassL']==0 and e['point']['predicateMask']==128
    assert e['receipt']['presentationState']=='STALE' and not e['poolReplayComplete']
    return dict(column=57,values=values,predicateMask=128,
                token=e['firstFailure']['frameToken'],presentation='STALE',
                poolReplayComplete=False,uniqueFirstWriter='NOT_CAPTURED_NOT_INFERRED')


def main():
    arithmetic_cases();members=membership_cases();negative=old_negative_example()
    domain=domain_cases();writers=actual_writer_mass_examples();historical=historical_capture()
    sources=['docs/architecture-T202-demo-material-arrival-v1.md',
        'docs/analysis-T202-demo-material-capture-v1.json',
        'docs/analysis-T202-reassembled-material-v1.json',
        'docs/algorithm-T197-whole-transaction-complexity-v1.md',
        'docs/analysis-T205-tx-resource-work-manifest-v1.json',
        'entry/src/main/ets/physics/P011ParticleHot.ets',
        'entry/src/main/ets/physics/P011ParticleM2Sink.ets',
        'entry/src/main/ets/physics/P011ParticleField.ets',
        'entry/src/main/ets/physics/WaterSandField.ets',
        'entry/src/main/ets/physics/P011PublishedRenderState.ets',
        'entry/src/main/ets/render/P013FrameAdapter.ets',
        'entry/src/main/ets/render/preparation/P013MaterialDataPreparation.ets',
        'docs/design-T207-dry-derived-model-v1.py']
    evidence=dict(revision='T207_DESIGN_EXACT_MASS_AND_DOMAIN_V1',date='2026-10-28',
        kind='designer arithmetic/domain and existing-capture checks, not product/device tests',
        inputSha256={p:hashlib.sha256((ROOT/p).read_bytes()).hexdigest() for p in sources},
        proposedCacheRevision='P011_LIVE_MASS_EXACT70_V2',
        counts=COUNTS,membershipModel=members,syntheticNegativeHistory=negative,
        domainModel=domain,actualWriterMassExamples=writers,historicalFacts=historical,
        resourceProposal=dict(C=128,Nmax=512,limbs=70,perViewBytes=37560,
                              views=8,auditViews=1,baseNewPayloadBytes=338040,
                              excludes=['undo/COW capacity','metadata/publication headers','runtime/driver/old scene/resources']),
        realSemanticChanges=['live-cache reduction bits','cache-dependent capacity/guard inputs and future trajectory',
                             'dry-water statistic defined +0 instead of envelope subtraction',
                             'strict dry publishable domain and revision-aware snapshots'],
        notClaimed=['T202 pool was empty','unique first writer determined','new physics bitwise equivalent to V1',
                    'V2 implemented','whole-chain tests passed','current frame rendered successfully',
                    'same-DEMO device stability verified','GPU production migration'])
    (ROOT/'docs/design-T207-dry-derived-evidence-v1.json').write_text(json.dumps(evidence,ensure_ascii=False,indent=2)+'\n',encoding='utf-8')
    print(json.dumps(dict(counts=COUNTS,historical=historical,resources=evidence['resourceProposal']),ensure_ascii=False))


if __name__=='__main__':main()
