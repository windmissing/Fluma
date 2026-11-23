"""Synthetic four-arm, terminal and hot-page controls with historical T496 reuse."""
from copy import deepcopy
import argparse
import hashlib
import json
from runpy import run_path
import subprocess
from pathlib import Path

from check import MODES, ROOT, SEAL, TAIL_IDS, compare, hot, need, packed, plan, source_trace


def digest(words):
    return hashlib.sha256(packed(words,len(words))).hexdigest()


def fixture(mode, i):
    actions=[]
    for action in plan():
        status='ACCEPTED'
        if action['kind'] in ('RESERVE','END'):
            status='OFF_NOOP' if mode.endswith('OFF') else {'RESERVE':'RESERVED','END':'ENDED'}[action['kind']]
        actions.append(dict(action=action,terminal=status,actualNs=action['scriptNs']+1,
                            terminalId=action['actionId']))
    roots=[]
    for kind,action,number in (('SOURCE',7,1),('QUANTUM',8,2)):
        ids={key:str(i*100+n) for n,key in enumerate(TAIL_IDS,1)}
        ids['nonce']=str(i*1000+number)
        words=list(SEAL)+[0]*252
        for name,at in TAIL_IDS.items():
            words[at]=int(ids[name])
        words[27:29]=[1 if kind=='SOURCE' else 2,1]
        words[53:55]=[1,193]
        words[61]=131072 if kind=='SOURCE' else 0
        result=dict(kind=kind,actionId=action,ids=ids,completion=words,completionSHA=digest(words),
                    ownerCode=0,submit=0,fence=0,externalProducer=True,replayDone=True,publicationConsumed=True)
        if kind=='SOURCE':
            cmd=[1129534023,2]+[0]*30
            cmd[4],cmd[12]=int(ids['run']),int(ids['geometry'])
            producer=bytes([action])
            result.update(command=cmd,commandSHA=digest(cmd),producerHex=producer.hex(),
                          producerSHA=hashlib.sha256(producer).hexdigest())
        if mode.startswith('REPLAY'):
            b,e=bytes(29696),bytes(256)
            result.update(copyFence=0,mapResult=0,invalidateResult=0,binding=[0,1],captureBinding=[0,1],
                          copyBeforeReuse=True,bankHex=b.hex(),eventHex=e.hex(),
                          bankSHA=hashlib.sha256(b).hexdigest(),eventSHA=hashlib.sha256(e).hexdigest())
        roots.append(result)
    actions[28]['terminal']='REJECTED'
    return dict(mode=mode,process=i,synthetic=True,contract=dict(clockAnchorSHA='fixture'),
                clockAnchorSHA='fixture',clockEquivalent=True,readyNs=1,actions=actions,roots=roots,
                drain=dict(retired=True,fence=0,offers=0,inFlight=0,leases=0,permits=0),
                credits=dict(apiBytes=200,indexBytes=100,rawBytes=60000))


def main():
    parser=argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--emit',type=Path,help='write synthetic D1 and D2 JSON into this directory')
    args=parser.parse_args()
    samples={m:fixture(m,i) for i,m in enumerate(MODES,1)}
    positive=compare(samples,True)
    c=samples['OFF']['roots'][0]
    # In T860 the hot request is the run token, separate from accepted-root nonce.
    req=c['ids']['run']
    rows=[dict(stage='M1_HOT_CAPTURE_RESERVED',request=req,requestNs='100'),
          dict(stage='M1_HOT_CAPTURE',request=req,requestedNs='100',frozenNs='101',safePoint='1',
               blocker=1,root=c['ids']['nonce'],completion=c['completion'])]
    anchor=[dict(request=req,run=req,root=c['ids']['nonce'],accepted=True,completionSHA=c['completionSHA'])]
    call=dict(run=req,geometry='1',offer='1',sample='2',hold='3',pulseTicks='103',
              sampleTicks='100',pulseNs='10300',sampleNs='10000',frequency='10000000',
              pixelX=10,pixelY=10,material=0)
    trace=[dict(call,stage='M1_OFFER_RETURN',serial='1',result=0,root='0'),
           dict(call,stage='M1_OFFER_TERMINAL',serial='2',result=2,root=c['ids']['nonce'])]
    need(source_trace(trace)['accepted']==1,'NATIVE_TRACE')
    if args.emit:
        args.emit.mkdir(parents=True,exist_ok=True)
        for name,content in (('d1.json',samples),('hot.json',dict(rows=rows,accepted=anchor))):
            (args.emit/name).write_text(json.dumps(content),encoding='utf-8')
    need(hot(rows,anchor)['terminal']==['FROZEN_HOT_PAGE'],'HOT_FIXTURE')
    for code in ('CANCELLED','TIMED_OUT'):
        need(hot(rows[:1]+[dict(stage='M1_HOT_CAPTURE_TERMINAL',request=req,state=code)],[])['terminal']==[code],code)
    neg=[]
    for reason,change in (
        ('ACTION_ORDER_OR_CONTENT',lambda a:a['ON']['actions'].pop()),
        ('ACTION_TERMINAL',lambda a:a['ON']['actions'][6].update(terminal='PENDING')),
        ('DRAIN',lambda a:a['ON']['drain'].update(leases=1)),
        ('CLOCK_EQUIVALENCE_UNPROVEN',lambda a:a['REPLAY_ON'].update(clockEquivalent=False)),
        ('REPLAY_BACKING',lambda a:a['REPLAY_ON']['roots'][0].update(captureBinding=[1,0])),
        ('REPLAY_CREDIT',lambda a:a['REPLAY_ON']['credits'].update(apiBytes=13*2**20)),
    ):
        cp=deepcopy(samples)
        change(cp)
        try:
            compare(cp,True)
        except ValueError as err:
            need(str(err)==reason,str(err))
        else:
            raise AssertionError(reason)
        neg.append(reason)
    for changed,expected in (
        (trace[:1],'OFFER_TRACE_PENDING'),
        (trace+[dict(stage='M1_OFFER_TRACE_INCOMPLETE',dropped=1)],'OFFER_TRACE_LOSS'),
    ):
        try:
            source_trace(changed)
        except ValueError as err:
            need(str(err)==expected,str(err))
        else:
            raise AssertionError(expected)
        neg.append(expected)
    for reason,change in (
        ('HOT_TIME_OR_BLOCKER',lambda a:a[1].update(frozenNs='1000000100')),
        ('HOT_ROOT_OR_SHA',lambda a:a[1].update(root='77')),
        ('HOT_PENDING',lambda a:a.pop()),
        ('HOT_DUPLICATE_OR_STALE',lambda a:a.append(a[-1])),
    ):
        cp=deepcopy(rows)
        change(cp)
        try:
            hot(cp,anchor)
        except ValueError as err:
            need(str(err)==reason,str(err))
        else:
            raise AssertionError(reason)
        neg.append(reason)
    historical='test/reports/t496/t657milestoneoff/t496_log_t657milestoneoff.ndjson'
    raw=subprocess.check_output(['git','show',f'HEAD:{historical}'],cwd=ROOT)
    result=run_path(str(ROOT/'test/t496/check_actions.py'))['check'](list(map(json.loads,raw.splitlines())))
    print(json.dumps(dict(positive=positive,negativeControls=neg,hot=hot(rows,anchor),
                          T496=result,actual='PENDING_PC_ADAPTER'),indent=2))


if __name__=='__main__':
    main()
