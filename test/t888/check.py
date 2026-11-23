"""T888: independent original PC long-window and T882 B evidence gates."""
import argparse
import hashlib
import importlib.util
import json
from pathlib import Path
import sys

ROOT=Path(__file__).resolve().parents[2]
ART=ROOT/'test/reports'


def load(name,path):
    spec=importlib.util.spec_from_file_location(name,ROOT/path)
    module=importlib.util.module_from_spec(spec)
    spec.loader.exec_module(module)
    return module


OLD=load('t496pc','test/t496/check_pc_d1.py')
RATE=load('t850','test/t850/check.py')
sys.path.insert(0,str(ROOT/'test/t883'))
P=load('t883physics','test/t883/physics.py')
TX=load('t883tx','test/t883/transactions.py')
need=OLD.need
pair=OLD.COLD.pair
TERMINAL='M1_QUANTUM_PRESENT_RETIRED'
PE_SHA='12d454b11469160ef2ff5f29dd9aaf8905a2d512d95bf070dc391d00ea44db28'


def rows_at(rows,stage):
    return [r for r in rows if r.get('stage')==stage]


def exactly(rows,stage):
    match=rows_at(rows,stage)
    need(len(match)==1,'MISSING_OR_DUPLICATE_'+stage)
    return match[0]


def long_window(path):
    rows, receipt=OLD.read_rows(path)
    receipt['offerQuoteRepairCount']=len(receipt.pop('offerQuoteRepairLines'))
    queued=[r for r in rows if r.get('state')=='M1_QUANTUM_PRESENT_QUEUED']
    if not queued:
        return dict(receipt,queued=0,fieldState='MISSING_EVIDENCE',
                    ready=sum(r.get('stage')=='M1_POINTER_READY' for r in rows),
                    displayedFps=None)
    roots=OLD.accepted(rows)
    up=next((int(r['qpc']) for r in rows_at(rows,'WIN32_POINTER') if r['kind']=='UP'),None)
    seen={}
    ticks=[]
    stocks=[]
    active=[]
    upper=[]
    leases=0
    unknown=[]
    qpcs={int(r['qpcFrequency']) for r in queued}
    need(len(qpcs)==1 and min(qpcs)>0,'QPC_FREQUENCY')
    hz=qpcs.pop()
    scope=tuple(queued[0][k] for k in OLD.SCOPE)
    for row in queued:
        frame=row['windowFrame']
        r=roots[frame['acceptedRoot']]
        tw=r['words']
        need(r['kind']=='QUANTUM' and r['scope']==scope==tuple(row[k] for k in OLD.SCOPE)
             and tuple(map(int,(frame['run'],frame['config'],frame['sourceCommit'],frame['physicsCommit'],
                                frame['generation'],frame['publicationSlot'])))==
                    (*[pair(tw,i) for i in (6,40,18,20,36)],tw[35])
             and OLD.COLD.pair(tw,38)==int(row['geometry']) and all(frame[k]==0 for k in RATE.RESULTS)
             and int(row['m1QuantumFenceNs'])<=int(row['m1QuantumDeadlineNs'])
             and 0<int(frame['presentEnterQpc'])<=int(frame['presentExitQpc']),
             'QUANTUM_PRESENT_SCOPE')
        exit_qpc=int(frame['presentExitQpc'])
        need(not ticks or exit_qpc>ticks[-1],'PRESENT_ORDER')
        ticks.append(exit_qpc)
        nonce=frame['acceptedRoot']
        if nonce in seen:
            need(seen[nonce]==tuple(tw),'REDISPLAY_ROOT_DRIFT')
            continue
        seen[nonce]=tuple(tw)
        stock,water=pair(tw,77),pair(tw,79)
        need(stock==pair(tw,71) and water==pair(tw,75),'QUANTUM_INVENTORY_LEDGER')
        lease=row.get('m1SceneLease')
        if lease is not None:
            need(lease['abi']=='GRS_GPU_SCENE_Q20_M1_V2' and lease['geometry']==row['geometry']
                 and (int(lease['sandStockQ']),int(lease['waterStockQ']))==(stock,water)
                 and (int(lease['sandAdmittedTotalQ']),int(lease['waterAdmittedTotalQ']))==
                      (pair(tw,71),pair(tw,75)) and (lease['activeSand'],lease['activeWater'])==(tw[88],tw[90]),
                 'LEASE_STOCK')
            leases+=1
        if stocks:
            prior=stocks[-1]
            need(prior[0]<pair(tw,20) and prior[1]<pair(tw,36),'NEW_ROOT_ORDER')
            if prior[0]+1!=pair(tw,20) or prior[1]+1!=pair(tw,36):
                unknown.append(nonce)
        stocks.append((pair(tw,20),pair(tw,36),pair(tw,18),stock,water))
        active.append(tw[88])
        upper.append(exit_qpc)
    first_inactive=next((t for t,n in zip(upper,active) if not n),None)
    last_active=next((t for t,n in reversed(list(zip(upper,active))) if n),None)
    post=[i for i,t in enumerate(upper) if up is not None and t>=up]
    source_roots={k:v for k,v in roots.items() if v['kind']=='SOURCE'}
    source_after=[]
    source_fence_after=[]
    if up is not None:
        for k,r in source_roots.items():
            if int(k)>=up:
                source_after.append(k)
            if int(r['rows'][0]['m1FenceNs'])*hz>=up*1_000_000_000:
                source_fence_after.append(k)
    need(len(source_roots)>0,'SOURCE_POSITIVE')
    admitted=sum(r['words'][61] for r in source_roots.values())
    after_stocks={stocks[i][3] for i in post}
    retired=[r for r in rows if r.get('state')==TERMINAL]
    clean_retire=(len(retired)==1 and retired[0].get('m1PublicationReleased') is True
                  and retired[0].get('m1WsiResult')==retired[0].get('m1WsiRetireVkResult')==0)
    # T883 requires per-LAND event words and layer-attributed pixel pairs.
    missing=P.long_window({'window':{},'roots':[]},OLD.sha,RATE.RESULTS)['missing']
    interval=lambda data:RATE.metric([(b-a)*1000/hz for a,b in zip(data,data[1:])])
    span=(ticks[-1]-ticks[0])/hz
    return dict(receipt,queued=len(queued),newRoots=len(seen),repeat=len(queued)-len(seen),
                newQueueRate=(len(seen)-1)/span if span else None,observedSpanSeconds=span,
                queueIntervalMs=interval(ticks),newRootIntervalMs=interval(upper),
                displayState='PRESENT_EVIDENCE_PENDING',displayedFps=None,refreshHz=None,
                acceptedSourceRoots=len(source_roots),admittedQ20=admitted,sourceStartsAtOrAfterUp=source_after,
                sourceFencesAtOrAfterUp=source_fence_after,
                cleanRetire=clean_retire,stockQ20=sorted({s[3] for s in stocks}),sceneLeases=leases,
                postUpNewRoots=len(post),postUpNonzeroActive=sum(active[i]>0 for i in post),
                postUpStockQ20=sorted(after_stocks),upQpc=up,
                firstZeroActiveRelativeToUpMs=(first_inactive-up)*1000/hz if first_inactive is not None and up else None,
                lastNonzeroActiveRelativeToUpMs=(last_active-up)*1000/hz if last_active is not None and up else None,
                gaps=unknown,fieldState='MISSING_EVIDENCE',
                missing=missing+['individual LAND event words','layer-attributed same-root RGBA probes',
                                 'verified display trace / active refresh mode'],
                bulkRetentionSupported=bool(post and len(after_stocks)==1 and min(after_stocks)>0
                                            and not source_after and not source_fence_after
                                            and len({s[2] for s in stocks})==1))


def actual_build():
    p=ROOT/'.codex/local-test/t882-b-final.exe'
    with p.open('rb') as f:
        digest=hashlib.file_digest(f,'sha256').hexdigest()
        f.seek(0)
        start=f.read(64)
        f.seek(int.from_bytes(start[60:64],'little'))
        prefix=f.read(6)
    need(digest==PE_SHA and start[:2]==b'MZ' and prefix==b'PE\0\0\x64\x86','T882_B_PE')
    return {'sha256':digest,'windowsX64':True,'bytes':p.stat().st_size}


def d1_drain(seal, drain, close, hz, authority=()):
    need(seal['run']==drain['run'] and seal['geometry']==drain['generation']
         and all(drain[k]==seal[k] for k in ('endNs','lastPulseSeq','lastOffer'))
         and seal['lastAction']==52 and int(close['qpc'])*1_000_000_000>=int(drain['drainNs'])*hz,
         'DRAIN_INTAKE_OR_CLOSE')
    need(drain['state']=='RETIRED' and drain['retired'] is True
         and 0<=int(drain['drainNs'])-int(drain['endNs'])<=1_000_000_000
         and all(drain[k]==0 for k in ('offers','inFlight','leases','pending','permits','captureRequests',
                                     'writer','reader','pin','traceLoss','ownerState')),'DRAIN_INCOMPLETE')
    need(drain['wsiUsed'] is True and drain['retireVkResult']==0,'DRAIN_WSI')
    if int(drain['ownerFence']):
        need(drain['ownerFenceVkResult']==0
             and 0<int(drain['ownerFenceEnterQpc'])<=int(drain['ownerFenceExitQpc'])
             and int(drain['ownerFenceNonce'])>0 and
                 (drain['ownerTombstonePoll']==5 or
                  drain['ownerTombstonePoll'] is None and drain['writersCompleted']>0),
             'DRAIN_FENCE')
        probe=drain['ownerProbeVkResult']
        if probe in (0,2):
            need(int(drain['ownerFenceEnterQpc'])<=int(drain['ownerProbeQpc'])<=int(drain['ownerFenceExitQpc']),
                 'DRAIN_PROBE_TIMING')
            fence_state='FENCE_PENDING' if probe==2 else 'FENCE_SIGNALED'
        else:
            need(probe==1 and drain['ownerProbeQpc']=='0'
                 and any(a['nonce']==drain['ownerFenceNonce'] and a['fenceObject']==drain['ownerFence']
                         and a['retireVkResult']==0 and a['fencePending'] is False for a in authority),
                 'DRAIN_PRIOR_FENCE')
            fence_state='FENCE_RETIRED_BEFORE_DRAIN_PROBE'
    else:
        fence_state='NO_FENCE_TO_RETIRE'
    c=drain['credit']
    need(c['balanced'] is True and c['generated']==c['accepted']+c['overload']+c['revoked']
         and c['carry']==0 and c['clearedCarry']-c['clearedCarryBefore']==c['carryBefore']
         and c['revoked']-c['revokedBefore']==c['pendingBefore']+c['activeBefore'],'DRAIN_CREDIT')
    return fence_state


def off(path):
    rows, receipt=OLD.read_rows(path)
    need(not receipt['offerQuoteRepairLines'] and not rows_at(rows,'PC_TRACE_INCOMPLETE'),'T882_B_STRICT_JSON')
    ready=exactly(rows,'M1_D1_READY')
    seal=exactly(rows,'END_INTAKE')
    drow=exactly(rows,'M1_D1_DRAIN')
    close=exactly(rows,'M1_D1_CLOSE')
    stage_at=lambda row:next(i for i,r in enumerate(rows) if r is row)
    need(stage_at(seal)<stage_at(drow)<stage_at(close),'DRAIN_RECORD_ORDER')
    hz=int(ready['qpcFrequency'])
    need(hz>0 and ready['mode']=='OFF' and ready['run']==seal['run'],'D1_READY')
    authority=[r['m1QuantumAuthority'] for r in rows if 'm1QuantumAuthority' in r]
    drain=d1_drain(seal,drow['m1D1Drain'],close,hz,authority)
    scripts=OLD.D1.plan()
    actions=rows_at(rows,'M1_D1_ACTION')
    need([(r['id'],OLD.KINDS[r['kind']],int(r['scriptNs'])) for r in actions]==
         [(s['actionId'],s['kind'],s['scriptNs']) for s in scripts],'D1_ACTION_TABLE')
    calls=rows_at(rows,'M1_D1_PULSE_OWNER')
    need(len({p['actionId'] for p in calls})==len(calls),'OWNER_PULSE_DUPLICATE')
    owner={p['actionId']:p for p in calls}
    source=OLD.offers(rows,ready['run'],scripts,OLD.accepted(rows))
    for call in rows_at(rows,'M1_OFFER_TERMINAL'):
        a=source[call['actionId']]
        if a['outcome']=='EXPIRED':
            need(call.get('reason') and int(call['checkedNs'])>=int(call['pulseNs'])
                 and int(call['ageNs'])==int(call['checkedNs'])-int(call['pulseNs']),'SOURCE_EXPIRY_EVIDENCE')
            a.update(ageNs=int(call['ageNs']),reason=call['reason'])
    lines=[]
    missing=[]
    for action,row in zip(scripts,actions,strict=True):
        aid=action['actionId']
        rate_key={}
        if action['kind']=='PULSE':
            if aid not in owner:
                if row['state']=='DELIVERED':
                    missing.append(dict(action=aid,seq=row['nativeSeq'],session=row['session']))
                    rate_key['state']='OWNER_TERMINAL_MISSING'
                else:
                    need(row['state']=='REVOKED' and row['nativeSeq']==row['session']=='0','D1_PULSE_TRANSPORT')
                    rate_key['state']='REVOKED_BEFORE_NATIVE'
            else:
                p=owner[aid]
                need(row['state']=='DELIVERED' and all(row[a]==p[b] for a,b in
                     (('session','session'),('nativeSeq','seq'),('callbackQpc','callbackQpc'),
                      ('nativeReceiptQpc','nativeReceiptQpc'))) and int(p['seq'])>0
                     and 0<int(p['callbackQpc'])<=int(p['nativeReceiptQpc'])<=int(p['ownerQpc']),
                     'D1_OWNER_PULSE_JOIN')
                rate_key={k:p[k] for k in ('state','generated','carry','overload','pending')}
                rate_key.update(receiptDelayQpc=int(p['nativeReceiptQpc'])-int(p['callbackQpc']),
                                ownerDelayQpc=int(p['ownerQpc'])-int(p['callbackQpc']))
        elif action['kind']=='SOURCE':
            need(row['state']=='AWAIT_PULSE' and source[aid]['outcome']!='MISSING_OFFER','SOURCE_TERMINAL')
            rate_key={k:v for k,v in source[aid].items() if k not in ('offer','root','commandSHA')}
            if 'deltaNs' in rate_key:
                need(rate_key['admittedQ20']==TX.source_mass(rate_key['deltaNs']),'D1_SOURCE_MASS')
        else:
            need(row['state']=='OFF_NOOP','OFF_CAPTURE_NOOP')
        lines.append(dict(id=aid,**rate_key,relativeNs=int(row['actualNs'])-int(ready['readyNs'])))
    last=next(r for r in reversed(actions) if r['kind']==0 and int(r['nativeSeq']))
    need(last['nativeSeq']==seal['lastPulseSeq'] and last['session']==seal['lastPulseSession']
         and int(seal['lastOffer'])==max(int(v['offer']) for v in source.values()),'FINAL_INTAKE_WATERMARK')
    need(any(r['outcome']=='ACCEPTED' for r in source.values()),'D1_SOURCE_POSITIVE')
    return dict(receipt,run=ready['run'],hz=hz,drain=drain,actions=lines,source=source,
                missingOwnerTerminals=missing)


def off_pair(a,b):
    need(a['run']!=b['run'] and a['hz']==b['hz'],'OFF_PAIR_IDENTITY')
    if a.get('missingOwnerTerminals') or b.get('missingOwnerTerminals'):
        return dict(state='OWNER_PULSE_COVERAGE',left=a.get('missingOwnerTerminals'),
                    right=b.get('missingOwnerTerminals'),
                    source7DeltaNs=[r['source'][7]['deltaNs'] for r in (a,b)],
                    source7Q20=[r['source'][7]['admittedQ20'] for r in (a,b)],
                    source7CommandSHA=[r['source'][7]['commandSHA'] for r in (a,b)])
    for x,y in zip(a['actions'],b['actions'],strict=True):
        need(x['id']==y['id'],'OFF_PAIR_ACTION')
        for k in ('state','deltaNs','admittedQ20','requested','generated','pending','carry','overload','outcome',
                  'relativeNs','receiptDelayQpc','ownerDelayQpc'):
            if x.get(k)!=y.get(k):
                return dict(state='CLOCK_EQUIVALENCE_UNPROVEN',action=x['id'],field=k,
                            left=x.get(k),right=y.get(k))
    return dict(state='OFF_PAIR_CLOCK_MATCH_FOUR_ARMS_PENDING')


def selftest():
    from copy import deepcopy
    from tempfile import TemporaryDirectory
    seal=dict(run='1',geometry='2',lastAction=52,endNs='1000',lastPulseSeq='48',lastOffer='2',
              lastPulseSession='1')
    record=dict(seal,generation='2',drainNs='2000',state='RETIRED',retired=True,wsiUsed=True,
                retireVkResult=0,ownerFence='16',ownerFenceNonce='42',ownerProbeVkResult=2,
                ownerFenceVkResult=0,ownerFenceEnterQpc='1',ownerProbeQpc='2',ownerFenceExitQpc='3',
                ownerTombstonePoll=5,writersCompleted=0,credit=dict(balanced=True,generated=6,accepted=3,
                    overload=1,revoked=2,revokedBefore=0,pendingBefore=1,activeBefore=1,carry=0,
                    carryBefore=4,clearedCarry=4,clearedCarryBefore=0))
    record.update(**dict.fromkeys(('offers','inFlight','leases','pending','permits','captureRequests',
                                  'writer','reader','pin','traceLoss','ownerState'),0))
    close={'qpc':'20'}
    need(d1_drain(seal,record,close,10_000_000)=='FENCE_PENDING','PENDING_FIXTURE')
    need(d1_drain(seal,dict(record,ownerProbeVkResult=0),close,10_000_000)=='FENCE_SIGNALED',
         'SIGNALED_FIXTURE')
    negative=[]
    for changes,reason in (
        (dict(ownerFenceVkResult=2),'DRAIN_FENCE'),(dict(inFlight=1),'DRAIN_INCOMPLETE'),
        (dict(ownerTombstonePoll=None),'DRAIN_FENCE'),
        (dict(credit=dict(record['credit'],carry=1)),'DRAIN_CREDIT'),
        (dict(lastOffer='1'),'DRAIN_INTAKE_OR_CLOSE'),
    ):
        try:
            d1_drain(seal,dict(record,**changes),close,10_000_000)
        except ValueError as error:
            need(str(error)==reason,'SELFTEST_REASON')
        else:
            raise AssertionError(reason)
        negative.append(reason)
    a=dict(run='1',hz=10_000_000,actions=[dict(id=1,relativeNs=100),dict(id=2,relativeNs=200)])
    b=deepcopy(a)
    b['run']='2'
    need(off_pair(a,b)['state']=='OFF_PAIR_CLOCK_MATCH_FOUR_ARMS_PENDING','CLOCK_FIXTURE')
    b['actions'][1]['relativeNs']=201
    need(off_pair(a,b)['action']==2 and off_pair(a,b)['field']=='relativeNs','FIRST_DIFF_FIXTURE')
    # Exercise the full B parser with inherited historical SOURCE bytes and
    # explicitly synthetic B-only pulse/drain fields in a temporary file.
    rows,_=OLD.read_rows(ART/'t875/off2.jsonl')
    ready=exactly(rows,'M1_D1_READY')
    ready['qpcFrequency']='10000000'
    pulses=[]
    for item in rows_at(rows,'M1_D1_ACTION'):
        if item['kind']!=0:
            continue
        tick=int(item['actualNs'])//100
        item.update(callbackQpc=str(tick),nativeReceiptQpc=str(tick+1),
                    session='1',nativeSeq=str(len(pulses)+1))
        pulses.append(dict(stage='M1_D1_PULSE_OWNER',actionId=item['id'],session=item['session'],
                           seq=item['nativeSeq'],callbackQpc=item['callbackQpc'],
                           nativeReceiptQpc=item['nativeReceiptQpc'],ownerQpc=str(tick+2),
                           state='PAGE_PULSE_ACCEPTED',generated=0,pending=0,carry=0,overload=0))
    for item in rows_at(rows,'M1_OFFER_TERMINAL'):
        if item['result']==7:
            item.update(checkedNs=str(int(item['pulseNs'])+40_000_000),ageNs='40000000',reason='AGE')
    seal['run']=ready['run']
    record['run']=ready['run']
    rows += [dict(seal,stage='END_INTAKE'),*pulses,dict(stage='M1_D1_DRAIN',m1D1Drain=record),
             dict(close,stage='M1_D1_CLOSE')]
    with TemporaryDirectory() as directory:
        path=Path(directory)/'synthetic-off.jsonl'
        path.write_text(''.join(json.dumps(row)+'\n' for row in rows),encoding='utf-8')
        observed=off(path)
        need(len(observed['actions'])==52 and observed['source'][7]['outcome']=='ACCEPTED',
             'NATIVE_FORMAT_FIXTURE')
    return {'negatives':negative,'firstDifference':off_pair(a,b),'syntheticNativeActions':52}


def main():
    p=argparse.ArgumentParser(description=__doc__)
    p.add_argument('--off',nargs=2,type=Path)
    p.add_argument('--selftest',action='store_true')
    args=p.parse_args()
    output={'B_build':actual_build()}
    if args.selftest:
        output['selftest']=selftest()
    if args.off:
        a,b=map(off,args.off)
        output['B_OFF']=off_pair(a,b)
        output['B_arms']=[{k:v for k,v in r.items() if k!='actions'} for r in (a,b)]
    else:
        output['A']={f.stem:long_window(f) for f in sorted((ART/'t884').glob('*.jsonl'))}
        output['B_OFF']='PENDING_T887_OFF_PAIR'
    print(json.dumps(output,indent=2))


if __name__=='__main__':
    main()
