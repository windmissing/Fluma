"""T915 offline held-source and OP48 evidence gates."""
import argparse
from copy import deepcopy
import importlib.util
import json
from pathlib import Path
import re
import struct

ROOT = Path(__file__).resolve().parents[2]
ART = ROOT/'test/reports'
spec = importlib.util.spec_from_file_location('t888',ROOT/'test/t888/check.py')
T888 = importlib.util.module_from_spec(spec)
spec.loader.exec_module(T888)
OLD, need, pair = T888.OLD,T888.need,T888.pair
GAP = 33_333_333
IDENTITY = ('run','geometry','down','hold','captureEpoch','material')
CANCEL = {'UP','CANCEL','OUTSIDE','CAPTURE_LOST','FOCUS_LOST','MODE_CHANGED','RESIZE','CLOSE','DEVICE_LOST'}
OP48 = 'f01959cc0f2fdecff46c953cf6c6073498f531d91bec0ae7d900b35c4c18aceb'


def ticks_ns(ticks,hz):
    return int(ticks)*1_000_000_000//hz


def same(a,b,keys):
    return all(a[k]==b[k] for k in keys)


def resumed(e):
    down, stop, retire, base, rearm = (e[k] for k in ('down','suspend','retire','baseline','rearm'))
    hz = e['frequency']
    ns = lambda q:ticks_ns(q,hz)
    call0,call1,rx0,rx1 = stop['gapEndpoints']
    need(hz>0 and stop['reason']=='CLOCK_GAP' and down['kind']=='DOWN' and down['qpc']==down['down']
         and same(down,stop,IDENTITY) and 0<down['down']<=call0<call1<=rx1<=stop['qpc']
         and call0<=rx0<rx1 and max(ns(call1)-ns(call0),ns(rx1)-ns(rx0))>GAP
         and stop['authorized'] is False and stop['queued']==0 and stop['anchorCleared'] is True,
         'SUSPEND_REAL_GAP')
    need(same(stop,retire,('run','geometry','nonce','fence','device','queue','fenceGeneration'))
         and int(retire['fence'])>0 and stop['qpc']<=retire['enter']<=retire['exit']<retire['idleQpc']
         and retire['vkResult']==0 and retire['idle'] is True and retire['tombstonePoll']>0
         and retire['baseRoot']==stop['lastAcceptedRoot'] and retire['presentedRoot']==stop['lastPresentedRoot']
         and stop['nonce'] not in (retire['baseRoot'],retire['presentedRoot'])
         and retire['pending']==retire['active']==retire['carry']==retire['pin']==0
         and retire['revoked']-retire['revokedBefore']==stop['pending']+stop['active']
         and retire['clearedCarry']==stop['carry']
         and retire['generated']==retire['accepted']+retire['overload']+retire['revoked'],
         'OLD_FENCE_OR_CREDIT')
    need(same(base,down,('run','geometry')) and base['session']!=stop['session'] and base['seq']==1
         and retire['idleQpc']<base['callback']<=base['receipt']<=base['readyQpc']<=rearm['qpc'],
         'NEW_BASELINE')
    old = e['oldOffers']
    terminals = e['oldTerminals']
    need(len(set(old))==len(old)==len(terminals)
         and {t['offer'] for t in terminals}==set(old)
         and all(t['session']==stop['session'] and t['epoch']==stop['epoch'] and t['state']=='REVOKED'
                 and t['root'] is None for t in terminals),'OLD_OFFERS_NOT_CLOSED')
    cancel = [r for r in e['inputs'] if down['qpc']<r['qpc']<=e['observationEndQpc']
              and r['kind'] in CANCEL|{'DOWN'}]
    first_cancel = min((r['qpc'] for r in cancel),default=None)
    if first_cancel is not None and first_cancel<=rearm['qpc']:
        need(rearm['authorized'] is False and not e['newOffers'] and not e['pulses'],'INPUT_CANCELLED')
        return dict(state='PHYSICS_ONLY_AFTER_CANCEL')
    need(rearm['authorized'] is True and same(rearm,down,IDENTITY) and rearm['captureValid'] is True
         and rearm['inBounds'] is True and rearm['session']==base['session'] and rearm['epoch']!=stop['epoch'],
         'HOLD_REVALIDATION')
    pulses=e['pulses']
    need(len(pulses)>=2,'PAIR_MISSING')
    for i,p in enumerate(pulses):
        need(same(p,rearm,(*IDENTITY,'session','epoch')) and base['seq']<p['seq']
             and rearm['qpc']<p['callback']<=p['receipt']<=p['consumed']
             and (first_cancel is None or p['consumed']<first_cancel)
             and (i==0 or p['seq']==pulses[i-1]['seq']+1
                  and pulses[i-1]['callback']<p['callback'] and pulses[i-1]['receipt']<p['receipt']),
             'NEW_PULSE_IDENTITY')
    need(pulses[0]['offer'] is None and pulses[0]['admittedQ20']==0,'FIRST_PULSE_MINTED_SOURCE')
    by_seq={p['seq']:p for p in pulses}
    known_offers,permits,nonces=set(old),set(e['oldPermits']),{stop['nonce'],retire['baseRoot'],retire['presentedRoot']}
    commit=stop['sourceCommit']
    mass=stop['stockQ20']
    positive=0
    used_pulses=set()
    for row in e['newOffers']:
        a,b=(by_seq.get(row[k]) for k in ('previousSeq','currentSeq'))
        need(a and b and a['seq']+1==b['seq'] and same(row,rearm,(*IDENTITY,'session','epoch'))
             and b['offer']==row['offer'] and row['offer'] not in known_offers
             and row['permit'] not in permits and row['nonce'] not in nonces and b['seq'] not in used_pulses,
             'NEW_OFFER_REUSE_OR_IDENTITY')
        known_offers.add(row['offer'])
        permits.add(row['permit'])
        nonces.add(row['nonce'])
        used_pulses.add(b['seq'])
        delta=ns(b['callback'])-ns(a['callback'])
        need(0<delta<=GAP and 0<ns(b['receipt'])-ns(a['receipt'])<=GAP
             and b['consumed']<=row['grant']<=row['submit']<=row['terminal']
             and ns(row['submit'])-ns(b['receipt'])<=GAP and ns(row['terminal'])-ns(row['grant'])<=250_000_000
             and (first_cancel is None or row['terminal']<first_cancel)
             and row['deltaNs']==delta and row['requestedQ20']==8*delta*(1<<20)//1_000_000_000,
             'PAIR_CLOCK_OR_FRESHNESS')
        q=row['admittedQ20']
        need(row['terminalState'] in ('ACCEPTED','CAPACITY_REJECTED') and q>=0
             and (row['terminalState']=='ACCEPTED')==(q>0) and row['rejectedQ20']>=0
             and q+row['rejectedQ20']==row['requestedQ20']
             and row['sourceCommit']==commit+int(q>0) and row['stockQ20']==mass+q
             and row['physicsSteps']==0 and row['fenceResult']==0 and b['admittedQ20']==q,
             'SOURCE_ACCOUNTING')
        commit=row['sourceCommit']
        mass=row['stockQ20']
        positive+=q
    need({p['seq'] for p in pulses if p['offer'] is not None}==used_pulses,'MISSING_OFFER_TERMINAL')
    need(positive>0 or e['expectCapacityOnly'] and bool(e['newOffers']),'NO_RESUMED_SOURCE')
    return dict(state='SAME_DOWN_RESUMED' if positive else 'CAPACITY_ZERO_CONFIRMED',admittedQ20=positive)


def op48(rows,limit_ms):
    calls=[r for r in rows if r.get('role')=='OP_48' and r.get('call')=='vkCreateComputePipelines']
    need([r['edge'] for r in calls] in (['ENTER'],['ENTER','EXIT']),'OP48_CALL_CHAIN')
    entry=calls[0]
    keys=('pid','processStartTick','ownerTid','qpcFrequency','moduleSHA','layoutSHA','flags','pipelineCache')
    need(int(entry['pid'])>0 and int(entry['processStartTick'])>0 and int(entry['ownerTid'])>0
         and int(entry['qpcFrequency'])>0 and entry['moduleSHA']==OP48 and entry['vkResult'] is None,
         'OP48_IDENTITY')
    prior=[(r['role'],r['moduleSHA']) for r in rows[:rows.index(entry)]
           if r.get('call')=='vkCreateComputePipelines' and r.get('edge')=='ENTER']
    waits=T888.rows_at(rows,'T877_ROOT_WAIT')
    if len(calls)==1:
        need(not waits,'OP48_UNRETURNED_WITH_BUSINESS')
        return dict(pipeline='ENTER_WITHOUT_EXIT',elapsedMs=None,business='NOT_REACHED',enter=entry,prior=prior)
    exit=calls[1]
    need(same(entry,exit,keys) and int(exit['qpc'])>=int(entry['qpc']) and isinstance(exit['vkResult'],int),
         'OP48_EXIT_IDENTITY')
    ms=(int(exit['qpc'])-int(entry['qpc']))*1000/int(entry['qpcFrequency'])
    business='NOT_REACHED'
    if waits:
        need(exit['vkResult']==0 and len(waits)==1 and rows.index(waits[0])>rows.index(exit),'F02_PIPELINE_ORDER')
        diag=[r for r in rows if r.get('stage')=='T877_GPU_TIMESTAMPS' and r.get('state')=='LATE_RETIRED_DIAGNOSTIC']
        need(len(diag)==1 and diag[0]['nonce']==waits[0]['nonce'] and diag[0]['fence']==waits[0]['fence']
             and diag[0]['businessRejected'] is True and diag[0]['acceptedRoot'] is None
             and diag[0]['fenceVkResult']==0 and waits[0]['firstStop']=='GRANT_DEADLINE','F02_REJECT_OR_RETIRE')
        business='F02_REJECTED_LATE_RETIRE'
    return dict(pipeline='RETURNED_ERROR' if exit['vkResult'] else 'LATE_EXIT' if ms>limit_ms else 'EXIT',
                elapsedMs=ms,business=business,enter=entry,prior=prior)


def pipeline_pair(e):
    a,b=e['arms']
    need(e['axis'] in ('PE','SNAPSHOT') and a['pid']!=b['pid'] and a['start']!=b['start']
         and e['limitMs']>0,'PAIR_EXPERIMENT')
    frozen=('cwd','imageName','device','driver','queue','fixture','configSHA','stateSHA','logging',
            'environment','loader','icd','compiler','cacheIsolation','createInfo','observationPolicy')
    need(all(a[k] and a[k]==b[k] for k in frozen),'PAIR_FROZEN_PARAMETERS')
    for arm in (a,b):
        for key in ('peSHA','configSHA','stateSHA'):
            need(bool(re.fullmatch('[0-9a-f]{64}',arm[key])),'PAIR_DIGEST')
        for key in ('loader','icd','compiler'):
            need(all(arm[key].get(k) for k in ('path','version','sha256'))
                 and re.fullmatch('[0-9a-f]{64}',arm[key]['sha256']),'PAIR_LOADED_MODULE')
        params=arm['createInfo']
        need(params==create_info(arm['layoutSHA']),'PAIR_CREATE_INFO')
        argv=arm['argv']
        expected=['--g13-candidate',arm['output'],'--g13-fixture','F02',arm['configPath'],arm['statePath'],
                  '--g13-diagnostics','hot']
        if arm['snapshot']:
            expected+=['--g13-slot-snapshot']
        need(argv==expected,'PAIR_ARGV')
    proof=[op48(arm['rows'],e['limitMs']) for arm in (a,b)]
    for r,arm in zip(proof,(a,b),strict=True):
        entry=r['enter']
        need((entry['pid'],entry['processStartTick'],entry['ownerTid'],entry['layoutSHA'])==
             (arm['pid'],arm['start'],arm['ownerTid'],arm['layoutSHA'])
             and entry['flags']==arm['createInfo']['flags'] and entry['pipelineCache']==arm['createInfo']['cache'],
             'PAIR_EMITTED_IDENTITY')
    x,y=(r['prior'] for r in proof)
    if e['axis']=='PE':
        need(a['peSHA']!=b['peSHA'] and not a['snapshot'] and not b['snapshot'] and x==y
             and a['workBytes']==b['workBytes'],'PAIR_PE_PREFIX')
    else:
        need(a['peSHA']==b['peSHA'] and not a['snapshot'] and b['snapshot'] and b['workBytes']-a['workBytes']==2352
             and len(x)==len(y) and sum(l!=r for l,r in zip(x,y))==1
             and all(l==r or (l[0],r[0])==('OP17_PRIVATE_LEDGER','OP17_PRIVATE_LEDGER_SNAPSHOT')
                     for l,r in zip(x,y)), 'PAIR_SNAPSHOT_PREFIX')
    return dict(state='MATCHED_CALL_PARAMETERS',results=[{k:r[k] for k in ('pipeline','elapsedMs','business')} for r in proof])


def create_info(layout_sha):
    return dict(stage='compute',entryPoint='main',stageFlags=0,specialization=None,basePipeline=None,baseIndex=0,
                allocator=None,pNext=None,stagePNext=None,flags=0,cache=None,
                push=dict(stage='compute',offset=0,bytes=24),setIndex=0,
                bindings=[dict(binding=i,kind='storage' if i<4 else 'uniform',count=1,stage='compute')
                          for i in range(5)],layoutSHA=layout_sha)


def historical():
    runs=ART/'t912/runs'
    paths={'t912-no-tail':runs/'g13-t909-no-tail.jsonl','t912-baseline':runs/'g13-baseline-tail.jsonl',
           't890-control':runs/'g13-control-t890.jsonl','t901-hot':ART/'t901/b-hot-slots.jsonl'}
    proof={}
    for name,path in paths.items():
        rows,receipt=OLD.read_rows(path)
        need(not receipt['offerQuoteRepairLines'],'ORIGINAL_G13_JSON')
        r=op48(rows,240_000)
        proof[name]=dict(**r,sha256=receipt['sha256'])
    need((runs/'g13-baseline-tail.jsonl').read_bytes()==(runs/'g13-stall-evidence.jsonl').read_bytes(),
         'T912_DUPLICATE_IDENTITY')
    need(len({v['enter']['pid'] for v in proof.values()})==len(proof)
         and proof['t912-no-tail']['prior']==proof['t912-baseline']['prior']==proof['t901-hot']['prior']
         and proof['t890-control']['prior']!=proof['t912-baseline']['prior'],'HISTORICAL_PREFIX')
    log=runs/'2a-sand-observe/demo.jsonl'
    rows,receipt=OLD.read_rows(log)
    roots=OLD.accepted(rows)
    source={k:v for k,v in roots.items() if v['kind']=='SOURCE'}
    for r in source.values():
        raw=r['rows'][0]
        OLD.D1.raw_bytes(raw['m1SourceWords'],raw['m1SourceWordsSHA'],32)
        need(OLD.sha(struct.pack('<24Q',*raw['m1ProducerWords'][:-1]))==raw['m1ProducerSHA'],'ORIGINAL_SOURCE_SHA')
    pointers=T888.rows_at(rows,'WIN32_POINTER')
    down,=[r for r in pointers if r['kind']=='DOWN']
    up,=[r for r in pointers if r['kind']=='UP']
    gap,=[r for r in rows if r.get('state')=='M1_STALE_WAIT_REAL_PULSE'
          and r.get('m1Recovery',{}).get('reason')=='CLOCK_GAP']
    recover=gap['m1Recovery']
    auth=gap['m1QuantumAuthority']
    start=next(r for r in rows[rows.index(gap)+1:] if r.get('state')=='M1_READY_PHYSICS')
    hz=int(gap['qpcFrequency'])
    cb=ticks_ns(auth['producerTick'],hz)-ticks_ns(auth['callbackTick'],hz)
    rx=ticks_ns(auth['producerReceiptTick'],hz)-ticks_ns(auth['receiptTick'],hz)
    need(min(cb,rx)>GAP and auth['nonce']==recover['failedNonce'] and auth['retireVkResult']==0
         and auth['fencePending'] is False and int(auth['fenceObject'])>0 and recover['balanced'] is True
         and auth['retireExitQpc']==recover['oldRetiredQpc']
         and int(auth['retireExitQpc'])<int(start['m1RecoveryBaselineCallbackQpc'])
         <=int(start['m1RecoveryBaselineReceiptQpc'])<int(up['qpc'])
         and start['m1RecoveryReadySession']!=auth['session'],'HISTORICAL_GAP_FENCE_BASELINE')
    continuing={}
    for r in rows[rows.index(start)+1:]:
        frame=r.get('windowFrame')
        if r.get('state')=='M1_QUANTUM_PRESENT_QUEUED' and frame and int(frame['presentExitQpc'])<int(up['qpc']):
            k=frame['acceptedRoot']
            need(roots[k]['kind']=='QUANTUM' and str(pair(roots[k]['words'],18))==frame['sourceCommit']=='16'
                 and all(frame[k]==0 for k in T888.RATE.RESULTS),'HISTORICAL_WSI_ROOT')
            continuing[k]=r
    mass=sum(r['words'][61] for r in source.values())
    need(len(source)==16 and mass==2064374 and len(continuing)==51
         and all(int(k)<int(auth['nonce']) for k in source)
         and not any(int(r['m1SourceBoundary']['offer']) for r in continuing.values() if 'm1SourceBoundary' in r),
         'HISTORICAL_SOURCE_BOUNDARY')
    return dict(g13=proof,duplicateBaseline=True,historicalPair='ACTUAL_ARGV_ICD_AND_EQUAL_PREFIX_MISSING',
                m1=dict(sha256=receipt['sha256'],down=down['qpc'],up=up['qpc'],sourceCount=len(source),
                        stockQ20=mass,callbackGapNs=cb,receiptGapNs=rx,fence=auth['fenceObject'],
                        baselineSession=start['m1RecoveryReadySession'],continuingQuanta=len(continuing),
                        state='PHYSICS_CONTINUED_SOURCE_DID_NOT_RESUME'))


def m1_fixture():
    h=dict(run=1,geometry=2,down=1,hold=3,captureEpoch=4,material=0)
    s=dict(h,session=1,epoch=1,reason='CLOCK_GAP',qpc=62,gapEndpoints=[20,60,21,61],
           authorized=False,queued=0,anchorCleared=True,nonce=9,fence=5,device=7,queue=8,fenceGeneration=2,
           lastAcceptedRoot=8,lastPresentedRoot=7,pending=1,active=1,carry=40,sourceCommit=2,stockQ20=100)
    r=dict(run=1,geometry=2,nonce=9,fence=5,device=7,queue=8,fenceGeneration=2,enter=63,exit=65,idleQpc=66,
           vkResult=0,idle=True,tombstonePoll=5,baseRoot=8,presentedRoot=7,pending=0,active=0,carry=0,pin=0,
           revoked=3,revokedBefore=1,clearedCarry=40,generated=7,accepted=3,overload=1)
    rearm=dict(h,session=2,epoch=2,qpc=72,authorized=True,captureValid=True,inBounds=True)
    q=8*15_000_000*(1<<20)//1_000_000_000
    pulses=[dict(h,session=2,epoch=2,seq=2,callback=74,receipt=75,consumed=76,offer=None,admittedQ20=0),
            dict(h,session=2,epoch=2,seq=3,callback=89,receipt=90,consumed=91,offer=12,admittedQ20=q)]
    offers=[dict(h,session=2,epoch=2,offer=12,permit=6,nonce=11,previousSeq=2,currentSeq=3,grant=92,submit=93,
                 terminal=100,deltaNs=15_000_000,requestedQ20=q,admittedQ20=q,rejectedQ20=0,sourceCommit=3,
                 stockQ20=100+q,physicsSteps=0,fenceResult=0,terminalState='ACCEPTED')]
    return dict(down=dict(h,kind='DOWN',qpc=1),suspend=s,retire=r,
                baseline=dict(run=1,geometry=2,session=2,seq=1,callback=68,receipt=69,readyQpc=71),
                rearm=rearm,frequency=1000,observationEndQpc=110,oldOffers=[10],oldPermits=[5],
                oldTerminals=[dict(offer=10,session=1,epoch=1,state='REVOKED',root=None)],
                inputs=[],pulses=pulses,newOffers=offers,expectCapacityOnly=False)


def pipeline_fixture():
    a=dict(pid=1,start='11',ownerTid=21,peSHA='a'*64,cwd='fixture',imageName='hello_world_pc.exe',
           device={'uuid':'fixture'},driver={'version':'fixture'},queue={'family':0,'priority':1},
           fixture='F02',configSHA='b'*64,stateSHA='c'*64,logging='file',environment={'VK_DRIVER_FILES':'fixture'},
           loader={'path':'loader','version':'1','sha256':'d'*64},icd={'path':'icd','version':'1','sha256':'e'*64},
           compiler={'path':'compiler','version':'1','sha256':'f'*64},cacheIsolation='UNKNOWN',
           observationPolicy='fixture',layoutSHA='1'*64,workBytes=90000,output='out',configPath='cfg',statePath='state',
           argv=['--g13-candidate','out','--g13-fixture','F02','cfg','state','--g13-diagnostics','hot'],snapshot=False)
    a['createInfo']=create_info(a['layoutSHA'])
    a['rows']=[dict(role='OP_48',call='vkCreateComputePipelines',edge='ENTER',pid=1,processStartTick='11',
                    ownerTid=21,qpcFrequency='1000',qpc='10',vkResult=None,flags=0,pipelineCache=None,
                    moduleSHA=OP48,layoutSHA=a['layoutSHA'])]
    b=deepcopy(a)
    b.update(pid=2,start='12',ownerTid=22,peSHA='2'*64)
    b['rows'][0].update(pid=2,processStartTick='12',ownerTid=22)
    b['rows'].append(dict(b['rows'][0],edge='EXIT',qpc='100',vkResult=0))
    return dict(axis='PE',limitMs=50,arms=[a,b])


def selftest():
    m=m1_fixture()
    p=pipeline_fixture()
    good=dict(resumed=resumed(m),pair=pipeline_pair(p))
    capacity=deepcopy(m)
    offer=capacity['newOffers'][0]
    offer.update(admittedQ20=0,rejectedQ20=offer['requestedQ20'],sourceCommit=2,stockQ20=100,
                 terminalState='CAPACITY_REJECTED')
    capacity['pulses'][-1]['admittedQ20']=0
    capacity['expectCapacityOnly']=True
    good['capacity']=resumed(capacity)
    snapshot=deepcopy(p)
    snapshot['axis']='SNAPSHOT'
    a,b=snapshot['arms']
    b.update(peSHA=a['peSHA'],workBytes=a['workBytes']+2352,snapshot=True)
    b['argv'].append('--g13-slot-snapshot')
    for arm,name,digest in ((a,'OP17_PRIVATE_LEDGER','a'*64),(b,'OP17_PRIVATE_LEDGER_SNAPSHOT','b'*64)):
        arm['rows'].insert(0,dict(arm['rows'][0],role=name,moduleSHA=digest))
    good['snapshot']=pipeline_pair(snapshot)
    denied=[]
    for kind in ('UP','CAPTURE_LOST','MODE_CHANGED','OUTSIDE','RESIZE'):
        blocked=deepcopy(m)
        blocked['inputs']=[dict(kind=kind,qpc=67)]
        blocked['rearm']['authorized']=False
        blocked['newOffers']=blocked['pulses']=[]
        need(resumed(blocked)['state']=='PHYSICS_ONLY_AFTER_CANCEL','CANCEL_POSITIVE')
        denied.append(kind)
    tests=[]
    for gate,e,mutate,reason in (
        (resumed,m,lambda x:x['retire'].update(fence=99),'OLD_FENCE_OR_CREDIT'),
        (resumed,m,lambda x:x['retire'].update(vkResult=2),'OLD_FENCE_OR_CREDIT'),
        (resumed,m,lambda x:x['baseline'].update(session=1),'NEW_BASELINE'),
        (resumed,m,lambda x:x['pulses'][0].update(geometry=8),'NEW_PULSE_IDENTITY'),
        (resumed,m,lambda x:x['pulses'][0].update(admittedQ20=1),'FIRST_PULSE_MINTED_SOURCE'),
        (resumed,m,lambda x:x['newOffers'][0].update(epoch=1),'NEW_OFFER_REUSE_OR_IDENTITY'),
        (resumed,m,lambda x:x['newOffers'][0].update(offer=10),'NEW_OFFER_REUSE_OR_IDENTITY'),
        (resumed,m,lambda x:x['newOffers'].clear(),'MISSING_OFFER_TERMINAL'),
        (resumed,m,lambda x:x['newOffers'][0].update(deltaNs=39_000_000),'PAIR_CLOCK_OR_FRESHNESS'),
        (resumed,m,lambda x:x['inputs'].append(dict(kind='UP',qpc=95)),'PAIR_CLOCK_OR_FRESHNESS'),
        (pipeline_pair,p,lambda x:x['arms'][1]['argv'].append('--disable-optimization'),'PAIR_ARGV'),
        (pipeline_pair,p,lambda x:x['arms'][1]['icd'].update(sha256='0'*64),'PAIR_FROZEN_PARAMETERS'),
        (pipeline_pair,p,lambda x:x['arms'][1]['rows'][-1].update(ownerTid=7),'OP48_EXIT_IDENTITY'),
        (pipeline_pair,p,lambda x:[a['createInfo'].update(push={}) for a in x['arms']],'PAIR_CREATE_INFO'),
    ):
        case=deepcopy(e)
        mutate(case)
        try:
            gate(case)
        except ValueError as exc:
            need(str(exc)==reason,f'EXPECTED_{reason}: {exc}')
        else:
            raise AssertionError(reason)
        tests.append(reason)
    return dict(synthetic=good,negativeControls=tests,cancelControls=denied,original=historical())


def main():
    p=argparse.ArgumentParser(description=__doc__)
    p.add_argument('mode',choices=('selftest','resume','pair'))
    p.add_argument('input',type=Path,nargs='?')
    args=p.parse_args()
    output=selftest() if args.mode=='selftest' else (
        resumed if args.mode=='resume' else pipeline_pair)(json.loads(args.input.read_bytes()))
    print(json.dumps(output,indent=2))


if __name__=='__main__':
    main()
