"""T944/T945 offline evidence gates for UHD 630 candidates."""
import argparse
from collections import Counter, defaultdict
from copy import deepcopy
from hashlib import sha256
import importlib.util
import json
import math
from pathlib import Path
import statistics
import struct
import subprocess
import sys

ROOT=Path(__file__).resolve().parents[2]
ART=ROOT/'test/reports'


def module(name,path):
    spec=importlib.util.spec_from_file_location(name,ROOT/path)
    m=importlib.util.module_from_spec(spec)
    spec.loader.exec_module(m)
    return m


T905=module('t905','test/t905/check.py')
T922=module('t922','test/t922/check.py')
T866=module('t866','test/t866/check.py')
T888=T905.T888
need=T888.need
PLAN=json.loads((ROOT/'docs/architecture-T554-g9-dm-dispatch-v2.json').read_bytes())['quantum']
ROLES={'viscosity':{48},'pressure':{49},'other':{52,54,55,56,57,58,59,60,62}}
IDENTITY=('run','geometry','generation','nonce','device','queue','fence')


def sha(raw):
    return sha256(raw).hexdigest()


def raw(e):
    data=bytes.fromhex(e['hex'])
    need(sha(data)==e['sha256'] and len(data)%4==0,'RAW_ARTIFACT_SHA')
    return data


def first_word(a,b):
    if len(a)!=len(b):
        return dict(leftBytes=len(a),rightBytes=len(b))
    return next((dict(word=i,left=x[0],right=y[0]) for i,(x,y) in enumerate(
        zip(struct.iter_unpack('<I',a),struct.iter_unpack('<I',b),strict=True)) if x!=y),None)


def inventory(data):
    """Scan FullM1 SoA and derive independent sums from the entire allocated range."""
    need(len(data)==29696,'M1_STATE_RANGE')
    w=T922.words(data)
    col=16*(192+128)
    head=col+16*128
    sums=[]
    for offset,k,id_word,generation in ((0,192,5,10),(16*192,128,5,11)):
        seen=set()
        per_column=[0]*128
        for i in range(k):
            mass=w[offset+i]
            if not mass:
                continue
            ident=w[offset+id_word*k+i]
            x=w[offset+k+i]>>20
            need(x<128 and w[offset+generation*k+i]>0 and ident>0 and ident not in seen,
                 f'M1_INVENTORY_SLOT:{offset}:{i}')
            seen.add(ident)
            per_column[x]+=mass
        sums.append((len(seen),per_column))
    for field,group in ((8,0),(7,1)):
        need(w[col+field*128:col+(field+1)*128]==sums[group][1],f'M1_COLUMN_INVENTORY:{field}')
    s=sum(w[col+2*128:col+3*128])+sum(sums[0][1])
    water=sum(w[col+6*128:col+7*128])+sum(sums[1][1])
    need(s==T922.pair(w,head+60) and water==T922.pair(w,head+62),'M1_GLOBAL_INVENTORY')
    return dict(sandQ20=s,waterQ20=water,activeSand=sums[0][0],activeWater=sums[1][0])


def timed(r,auth):
    need(all(str(r[k])==str(auth[k]) for k in ('run','geometry','nonce','fence','queue'))
         and r['kind']==auth['kind'] and r['kind'] in ('SOURCE','QUANTUM')
         and int(r['pool'])>0 and int(r['poolGeneration'])>0 and int(r['resetSerial'])>0,'TIMING_IDENTITY')
    bits,period=r['validBits'],r['periodNs']
    need(1<=bits<=64 and math.isfinite(period) and period>0,'TIMING_DEVICE_DOMAIN')
    points=r['points']
    need(len({p['point'] for p in points})==len(points)
         and all((p['tick'] is not None)==p['available'] and
                 (not p['available'] or p['vkResult']==r['fenceVkResult']==0) for p in points),'TIMING_AVAILABILITY')
    vals=[int(p['tick']) for p in points if p['available']]
    duration=None if len(vals)<2 or not points[0]['available'] or not points[-1]['available'] else (
        (vals[-1]-vals[0])%(1<<bits))*period
    return dict(kind=r['kind'],durationNs=duration,accepted=r['acceptedRoot']==r['nonce'],
                missing=[p['point'] for p in points if not p['available']])


def m1(e):
    left,right=e['arms']
    freeze=('device','driver','seedSHA','inputSHA','sourceDeltaNs','angle','grantCap','clockProfile')
    need(all(left[k]==right[k] for k in freeze) and e['axis'] in ('inventory','motion-replay','observer'),
         'M1_UNEQUAL_INPUT')
    out={}
    for arm in (left,right):
        need(arm['kind'] in ('SOURCE','QUANTUM'),'M1_ROOT_KIND')
        t=timed(arm['timestamps'],arm['authority'])
        need(t['kind']==arm['kind'],'M1_TIMING_CLASS')
        need(arm['profileDispatches']==arm['recordedDispatches'] and arm['profileDispatches']>0,'M1_DISPATCH_PROFILE')
        out[arm['name']]=dict(timing=t)
    need(len(out)==2 and left['kind']==right['kind'],'M1_ARM_IDENTITY')
    order=('base','input','candidate','spare','completion','events','sourceLedger','untouched')
    need(all(set(a['raw'])==set(order) for a in (left,right)),'M1_COMPLETE_ARTIFACTS')
    a,b=({k:raw(v) for k,v in arm['raw'].items()} for arm in (left,right))
    need(a['base']==b['base'] and a['input']==b['input']
         and sha(a['base'])==left['seedSHA'] and sha(a['input'])==left['inputSHA'],'M1_UNEQUAL_BASE_OR_INPUT')
    inventories=[]
    for arm,files in ((left,a),(right,b)):
        tail=T922.words(files['completion'])
        need(len(tail)==256 and tail[:4]==[1381192263,2,256,3235823838]
             and tail[27]==(1 if arm['kind']=='SOURCE' else 2) and 0<=tail[52]<=tail[53]<=tail[54]
             and len(files['events'])==256*(tail[53]-tail[52])
             and len(files['sourceLedger'])==64*tail[56],'M1_COMPLETION_RANGES')
        need(arm['firstAuditError']==(tail[30:32] if tail[30] else None),'M1_FIRST_AUDIT_RAW')
        before=inventory(files['base'])
        after=inventory(files['candidate'])
        need(after==inventory(files['spare']),'M1_SPARE_INVENTORY')
        d=arm['admittedQ20']
        need(d>=0 and d==(tail[61] if arm['kind']=='SOURCE' else 0)
             and T922.pair(tail,77)==after['sandQ20'] and T922.pair(tail,79)==after['waterQ20']
             and arm['sourceKind'] in ('sand','water')
             and after['sandQ20']-before['sandQ20']==(d if arm['sourceKind']=='sand' else 0)
             and after['waterQ20']-before['waterQ20']==(d if arm['sourceKind']=='water' else 0),'M1_SOURCE_CONSERVATION')
        inventories.append(after)
    diff=next((dict(section=k,**d) for k in order if (d:=first_word(a[k],b[k]))),None)
    return dict(state='RAW_WORD_DIFF' if diff else 'RAW_WORD_MATCH',firstDiff=diff,
                firstAuditEqual=left['firstAuditError']==right['firstAuditError'],inventory=inventories,timing=out)


def dm2(e):
    need(e['family']=='DM2' and e['selectedJ'] in (1,2,4,8,16,32)
         and e['latchGeneration']==e['generation'] and e['latchAuthoritative'] is True,'DM2_LATCH')
    need(e['logicalCount']==len(PLAN)==2404,'DM2_LOGICAL_PLAN')
    removed=e['emptySlots']
    need(removed==sorted(set(removed)) and all(0<=i<len(PLAN) for i in removed),'DM2_SLOT_ORDER')
    for i in removed:
        row=PLAN[i]
        need(row['role'] in ('AUTHORITATIVE','REPLAY') and row['micro']>=e['selectedJ']
             and row['micro']>=0 and e['emptyWitness'][str(i)]==dict(generation=e['generation'],writes=0,events=0,paths=0),
             'DM2_ACTIVE_OR_UNPROVEN_SLOT')
    need(e['physicalDispatches']==len(PLAN)-len(removed) and e['barrierContractRetained'] is True,
         'DM2_PHYSICAL_ACCOUNTING')
    need(e['replayFromBASE'] is True and not e['replayAliasesActual'],'DM2_REPLAY_INDEPENDENCE')
    keys=('BASE','ACTUAL','SELECTED','REPLAY','EVENT','PATH','REJECTED_PREFIX','UNTOUCHED','COMPLETION')
    need(set(e['control'])==set(e['candidate'])==set(keys),'DM2_REQUIRED_PREFIX')
    for k in keys:
        need(raw(e['control'][k])==raw(e['candidate'][k]),f'DM2_WORD_DIFF:{k}')
    need(raw(e['candidate']['ACTUAL'])==raw(e['candidate']['REPLAY']),'DM2_REPLAY_WORDS')
    need(e['firstReject']==e['controlFirstReject'] and e['stageMask']==e['controlStageMask'],'DM2_FIRST_REJECT_OR_MASK')
    return dict(selectedJ=e['selectedJ'],removed=len(removed),logical=2404,physical=e['physicalDispatches'])


def g13(e):
    actual={}
    for r in e['roles']:
        name=r['name']
        need(name in ROLES and name not in actual and set(r['opcodes'])==ROLES[name]|{66}
             and len(r['opcodes'])==len(ROLES[name])+1 and r['bindings']==[0,1,2,3,4]
             and r['pushBytes']==24 and r['localSize']==[64,1,1] and 0<r['sourceBytes']<=49152
             and r['spirvValidated'] is True and len(bytes.fromhex(r['moduleSHA']))==32
             and r['layoutSHA']==e['layoutSHA'] and r['generation']==e['generation']
             and r['pipelineVkResult']==0 and r['op66VkResult']==0,'G13_CLOSURE')
        actual[name]=r
    need(actual.keys()==ROLES.keys() and e['logicalSlots']==820,'G13_CLOSURE_SET')
    ranges=e['ranges']
    need([r['binding'] for r in ranges]==list(range(5)) and all(
        r['generation']==e['generation'] and r['offset']>=0 and r['bytes']>0 and
        r['offset']+r['bytes']<=r['bufferBytes'] for r in ranges),'G13_BINDING_RANGE')
    need(all(e['root'][k]==e['fieldBinding'][k] for k in IDENTITY) and e['root']['generation']==e['generation']
         and e['root']['submit']==e['root']['retire']==0,'G13_FIELD_SCOPE')
    p=e['project']
    need(p['n']==64 and p['acceptedRoot'] is None and p['fence']==0,'G13_F02_PRIVATE')
    result=T866.project(p)
    u,v=([T866.F.float_word(x) for x in p[k]] for k in ('u32','v32'))
    n=p['n']
    div=[n*(u[y*(n+1)+x+1]-u[y*(n+1)+x]+v[(y+1)*n+x]-v[y*n+x])
         for y in range(n) for x in range(n)]
    need(T866.rms(div)<=2**-14 and max(map(abs,div))<=2**-12,'G13_F02_DIVERGENCE')
    visc=[T866.F.float_word(w) for w in e['viscosityResidualU32']]
    need(len(visc)==2*n*(n+1) and all(math.isfinite(x) for x in visc)
         and T866.rms(visc)<=2**-16/p['J'] and max(map(abs,visc))<=2**-14/p['J'],'G13_VISCOSITY_CERT')
    return dict(result,divergenceRMS=T866.rms(div),viscosityRMS=T866.rms(visc),businessAccepted=False)


def history():
    log=ART/'t941/runs/goal6-stage-timing/demo.jsonl'
    rows,_=T888.OLD.read_rows(log)
    groups=defaultdict(list)
    rejected=Counter()
    ids=set()
    for r in rows:
        if r.get('stage')!='M1_GPU_TIMESTAMPS':
            continue
        k=tuple(r[f] for f in ('run','queue','pool','poolGeneration','resetSerial'))
        need(k not in ids,'HISTORICAL_QUERY_REUSE')
        ids.add(k)
        t=timed(r,r)
        if t['accepted'] and t['durationNs'] is not None:
            groups[t['kind']].append(t['durationNs']/1e6)
        else:
            rejected[t['kind']]+=1
    need(set(groups)=={'SOURCE','QUANTUM'},'HISTORICAL_TIMING_GROUPS')
    m1_result={k:dict(samples=len(v),p50Ms=statistics.median(v),
                      p95Ms=sorted(v)[math.ceil(.95*len(v))-1],rejected=rejected[k]) for k,v in groups.items()}
    base=ART/'t923/runs/t924-runs/raw-dm2-t913-split-v2-at-t921-steps1-r2'
    def envelope(kind):
        p,=(base/kind).glob('*/envelope.json')
        doc=json.loads(p.read_bytes())
        data={}
        for f in doc['files']:
            b=(p.parent/f['name']).read_bytes()
            need(len(b)==f['bytes'] and sha(b)==f['sha256'],'HISTORICAL_DM2_HASH')
            data[f['name']]=b
        return doc,data
    source,s=envelope('accepted')
    q,files=envelope('diagnostic')
    tail=T922.words(files['COMPLETION256.bin'])
    need(source['acceptedRoot'] is not None and q['acceptedRoot'] is None and q['rootVkResult']==2
         and int(q['diagnosticGeneration'])==int(q['rootFenceGeneration'])+1 and tail[39:42]==[2404]*3
         and files['BASE.bin']==files['CURRENT_BASE.bin']==s['ACTUAL.bin']
         and files['INPUT128.bin']==files['GPU_INPUT128.bin'],'HISTORICAL_DM2_REJECT')
    last=next(r for r in T888.OLD.read_rows(ART/'t901/b-hot-slots.jsonl')[0] if r.get('stage')=='T877_GPU_TIMESTAMPS'
              and r.get('state')=='LATE_RETIRED_DIAGNOSTIC')
    need(last['firstStop']=='GRANT_DEADLINE' and last['acceptedRoot'] is None and last['fenceVkResult']==0,
         'HISTORICAL_G13_REJECT')
    f02={name:sha((ART/'t868/fixtures'/f'F02-{name}.bin').read_bytes()) for name in ('config128','state')}
    need(f02['config128'].startswith('295c075e') and f02['state'].startswith('e1c6b49e'),'HISTORICAL_F02_IDENTITY')
    return dict(m1=m1_result,dm2=dict(acceptedRoot=None,businessVkResult=q['rootVkResult'],
                state='LATE_PRIVATE_DIAGNOSTIC',dispatches=tail[39:42],eventSealed=q['eventSealed'],
                pathSealed=q['pathSealed'],sourceBaseJoined=True),
                g13=dict(firstStop=last['firstStop'],acceptedRoot=None,gpuSpanNs=last['spansNs']['root'],f02=f02))


def artifact(words):
    b=T922.blob(words)
    return dict(hex=b.hex(),sha256=sha(b))


def fixtures():
    ident=dict(run=1,geometry=2,generation=3,nonce=4,device=5,queue=6,fence=7)
    ticks=dict(ident,kind='QUANTUM',pool=8,poolGeneration=1,resetSerial=1,validBits=8,periodNs=2,
               acceptedRoot=4,fenceVkResult=0,points=[dict(point='BEGIN',available=True,tick=250,vkResult=0),
                                                    dict(point='END',available=True,tick=10,vkResult=0)])
    bank=[0]*7424
    bank[0]=123
    bank[192*5]=1
    bank[192*10]=1
    bank[5120+8*128]=123
    bank[-256+60]=123
    blobs={k:artifact(bank if k in ('base','candidate','spare') else [0])
           for k in ('base','input','candidate','spare','completion','events','sourceLedger','untouched')}
    tail=[0]*256
    tail[:4]=[1381192263,2,256,3235823838]
    tail[27]=2
    tail[77]=123
    blobs.update(completion=artifact(tail),events=artifact([]),sourceLedger=artifact([]))
    arm=dict(name='base',device='UHD630',driver=1656924,seedSHA=blobs['base']['sha256'],
             inputSHA=blobs['input']['sha256'],sourceDeltaNs=0,angle=0,grantCap=1,clockProfile='fixture',
             kind='QUANTUM',timestamps=ticks,authority=dict(ident,kind='QUANTUM'),profileDispatches=35,
             recordedDispatches=35,raw=blobs,firstAuditError=None,admittedQ20=0,sourceKind='sand')
    selected=next(i for i,row in enumerate(PLAN) if row['role']=='REPLAY' and row['micro']==2)
    files={k:artifact([0]) for k in ('BASE','ACTUAL','SELECTED','REPLAY','EVENT','PATH','REJECTED_PREFIX','UNTOUCHED','COMPLETION')}
    d=dict(family='DM2',selectedJ=2,latchGeneration=2,generation=2,latchAuthoritative=True,
           logicalCount=2404,emptySlots=[selected],emptyWitness={str(selected):dict(generation=2,writes=0,events=0,paths=0)},
           physicalDispatches=2403,barrierContractRetained=True,replayFromBASE=True,replayAliasesActual=False,
           control=files,candidate=deepcopy(files),firstReject=None,controlFirstReject=None,stageMask=262143,controlStageMask=262143)
    roles=[dict(name=k,opcodes=sorted(v|{66}),bindings=list(range(5)),pushBytes=24,localSize=[64,1,1],
                sourceBytes=1,spirvValidated=True,moduleSHA=str(i+1)*64,layoutSHA='a'*64,generation=3,
                pipelineVkResult=0,op66VkResult=0) for i,(k,v) in enumerate(ROLES.items())]
    n=64
    h=1/n
    amp=.25*math.pi/(2*math.sin(math.pi*h/2)/h)
    pressure=[round(amp*math.cos(math.pi*(i+.5)*h)*math.cos(math.pi*(j+.5)*h)*2**32)
              for j in range(n) for i in range(n)]
    p=dict(n=n,J=1,mode='PRIVATE_PROJECT_DIAGNOSTIC',acceptedRoot=None,queueSubmit=0,fence=0,qpcSubmit=1,
           qpcFence=2,pressureSolveObserved=True,analyticWrite=False,piQ32=pressure,u32=[0]*(n*(n+1)),v32=[0]*(n*(n+1)))
    f=dict(roles=roles,layoutSHA='a'*64,generation=3,logicalSlots=820,
           ranges=[dict(binding=i,generation=3,offset=0,bytes=32,bufferBytes=64) for i in range(5)],
           root=dict(ident,submit=0,retire=0),fieldBinding=ident,project=p,viscosityResidualU32=[0]*(2*n*(n+1)))
    return dict(arms=[arm,deepcopy(dict(arm,name='candidate'))],axis='inventory'),d,f


def selftest():
    a,b,c=fixtures()
    positive=dict(m1=m1(a),dm2=dm2(b),g13=g13(c))
    source=deepcopy(a)
    for arm in source['arms']:
        for record in (arm,arm['authority'],arm['timestamps']):
            record['kind']='SOURCE'
        tail=T922.words(raw(arm['raw']['completion']))
        tail[27]=1
        arm['raw']['completion']=artifact(tail)
    positive['sourceTiming']=m1(source)['timing']
    controls=[]
    for fn,e,mutate,reason in (
        (m1,a,lambda e:e['arms'][1].update(sourceDeltaNs=1),'M1_UNEQUAL_INPUT'),
        (m1,a,lambda e:e['arms'][1]['authority'].update(fence=99),'TIMING_IDENTITY'),
        (m1,a,lambda e:e['arms'][1]['raw'].update(candidate=artifact([0])),'M1_STATE_RANGE'),
        (dm2,b,lambda e:e.update(latchGeneration=1),'DM2_LATCH'),
        (dm2,b,lambda e:e.update(replayAliasesActual=True),'DM2_REPLAY_INDEPENDENCE'),
        (dm2,b,lambda e:e['candidate'].update(REJECTED_PREFIX=artifact([1])),'DM2_WORD_DIFF:REJECTED_PREFIX'),
        (dm2,b,lambda e:e.update(selectedJ=4),'DM2_ACTIVE_OR_UNPROVEN_SLOT'),
        (g13,c,lambda e:e['roles'][0].update(opcodes=[48,49,66]),'G13_CLOSURE'),
        (g13,c,lambda e:e['ranges'][1].update(offset=61),'G13_BINDING_RANGE'),
        (g13,c,lambda e:e['fieldBinding'].update(generation=2),'G13_FIELD_SCOPE'),
        (g13,c,lambda e:e['project']['piQ32'].__setitem__(0,0),'F02_GAUGE'),
        (g13,c,lambda e:e['viscosityResidualU32'].__setitem__(0,1065353216),'G13_VISCOSITY_CERT'),
    ):
        e=deepcopy(e)
        mutate(e)
        controls.append(T905.rejects(lambda:fn(e),reason))
    return dict(synthetic=positive,controls=controls,history=history(),candidateHardware='PENDING_T946')


def main():
    p=argparse.ArgumentParser(description=__doc__)
    p.add_argument('mode',choices=('selftest','m1','dm2','g13'))
    p.add_argument('input',type=Path,nargs='?')
    args=p.parse_args()
    result=selftest() if args.mode=='selftest' else {'m1':m1,'dm2':dm2,'g13':g13}[args.mode](
        json.loads(args.input.read_bytes()))
    print(json.dumps(result,indent=2))
    return int(result.get('state')=='RAW_WORD_DIFF' or result.get('firstAuditEqual') is False)


if __name__=='__main__':
    sys.exit(main())
