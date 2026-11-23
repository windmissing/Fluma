"""T883 independent PC acceptance preparation and executable controls."""
import argparse
from copy import deepcopy
import hashlib
import importlib.util
import json
from pathlib import Path
import struct
import subprocess
import sys
from tempfile import TemporaryDirectory

import physics as P
from transactions import GAP, clock_pair, drain, input_trace, need, sha_words, source_mass

ROOT=Path(__file__).resolve().parents[2]


def load(name,path):
    spec=importlib.util.spec_from_file_location(name,ROOT/path)
    m=importlib.util.module_from_spec(spec)
    spec.loader.exec_module(m)
    return m


D1=load('t865','test/t865/check.py')
PRIOR=load('t496pc','test/t496/check_pc_d1.py')
PTR=load('t836','test/t836/check.py')
RATE=load('t850','test/t850/check.py')
G9=load('t838','test/t838/check.py')


def sha(raw):
    return hashlib.sha256(raw).hexdigest()


def sha_file(path):
    with path.open('rb') as f:
        return hashlib.file_digest(f,'sha256').hexdigest()


def archive(path,envelope):
    readers={'DM2':lambda p:load('dm','tools/g9dm/read_cold.py').read(p),
             'R1':lambda p:load('r1','test/t529/read_cold.py').read_archive(p),
             'RX':lambda p:G9.G9.decode(p)}
    return P.route(path,envelope,readers,sha_file)


def history():
    doc=json.loads((ROOT/'docs/architecture-T880-evidence-v1.json').read_bytes())
    run=doc['runs'][-1]
    rows,receipt=PRIOR.read_rows(ROOT/run['path'])
    need(len(rows)==run['lines'],'T880_RUN5_ROW_COUNT')
    selected=run['selectedFrameTransitions'][0]
    left=selected['prior']['presentExitQpc']
    end=selected['newFrame']['nonceQpc']
    calls={r['offer']:r for r in rows if r.get('stage')=='M1_OFFER_RETURN' and r.get('hold')==str(run['secondHold'])}
    ages=[]
    for offer in ('96','97','98','99'):
        row=calls[offer]
        age=(left-int(row['pulseTicks']))*1_000_000_000//int(row['frequency'])
        need(row['result']==0 and age>GAP and any(r.get('stage')=='M1_OFFER_TERMINAL'
             and r['offer']==offer and r['result']==7 for r in rows),'T880_FIFO_EXPIRED')
        ages.append(age)
    need(all(calls[o]['result']==5 for o in ('100','101')) and left<end,'T880_FULL_RETURN')
    d1=json.loads((ROOT/'test/reports/t496/pc-offline-join.json').read_bytes())
    observations=[]
    for name,r in d1['arms'].items():
        path=ROOT/'test/reports/t875'/f'{name}.jsonl'
        values=PRIOR.arm(path,r['mode'])[2]
        need(values['sha256']==r['sha256'] and values['drain']==r['drain']
             and str(values['sourceTerminals'][7]['admittedQ20'])==str(r['sourceTerminals']['7']['admittedQ20']),
             'T881_ORIGINAL_ARM')
        observations.append(dict(arm=name,drain=values['drain'],source=values['sourceTerminals'][7]))
    return dict(fifoLowerBoundNs=ages,queueFull=['100','101'],run5SHA=receipt['sha256'],D1=observations)


def source_fixture(reason='normal'):
    e=[dict(kind='DOWN',hold=1,ns=0)]
    def add(kind,ns,**fields):
        if kind=='PULSE':
            fields['nativeNs']=ns+1000
        e.append(dict(kind=kind,ns=ns,**fields))
    for i in range(1,5):
        add('PULSE',i*16_000_000,id=i,hold=1,grantable=True)
    add('GRANT',65_000_000)
    for i in range(4):
        add('PULSE',(5+i)*16_000_000,id=5+i,hold=1,grantable=True)
        add('QUANTUM',(5+i)*16_000_000+1)
    add('TAKE',129_000_000,take=8)
    if reason=='gap':
        add('PULSE',170_000_000,id=9,hold=1,grantable=True)
    elif reason=='lost':
        add('FOCUS_LOST',130_000_000)
        add('DOWN',131_000_000,hold=2)
        add('PULSE',132_000_000,id=9,hold=1,grantable=True)
    elif reason=='device':
        add('DEVICE_LOST',130_000_000)
    good=reason=='normal'
    end=380_000_000 if reason=='late' else 171_000_000 if reason=='gap' else 134_000_000
    add('SOURCE_TAIL',end,id=8,submitNs=129_000_000,fenceNs=end,fenceResult=0,
        admittedQ20=source_mass(16_000_000) if good else 0,physicsSteps=0)
    add('UP',end+1)
    return e


def drain_fixture(pending):
    stages=['END_INTAKE']+(['FENCE_PENDING'] if pending else [])+[
        'FENCE_SIGNALED','OWNER_POLL','WSI_RETIRE','ACCOUNT','DRAIN_DONE','CLOSE']
    rows=[]
    for i,s in enumerate(stages):
        r=dict(stage=s,qpc=i+1,run=1,owner=2,generation=3,device=4,fence=8,nonce=12,
               vkResult=2 if s=='FENCE_PENDING' else 0)
        if s=='END_INTAKE':
            r.update(lastOfferSerial=2)
        if s=='OWNER_POLL':
            r.update(tombstone=True,acceptedRoot=None,ownerIdle=True)
        if s=='ACCOUNT':
            r.update(generated=4,accepted=1,revoked=2,shed=1,finalOfferSerial=2,traceLoss=0)
        if s=='DRAIN_DONE':
            r.update(**dict.fromkeys(('offers','inFlight','leases','pending','active','pins','writers',
                                      'readers','credits','acceptedLate'),0),retired=True)
        rows.append(r)
    return rows


def off_fixture(n):
    rows=[]
    for action in D1.plan():
        qpc=n+action['scriptNs']
        row=dict(action=action,terminalId=action['actionId'],terminal='OFF_NOOP' if
                 action['kind'] in ('RESERVE','END') else 'ACCEPTED',
                 producerQpc=qpc,receiptQpc=qpc+1,terminalQpc=qpc+2,acceptedPulse=action.get('pulseSeq'))
        if action['kind']=='SOURCE':
            command=[1129534023,2]+[0]*30
            row.update(deltaNs=16_000_000,q20=source_mass(16_000_000),command=command,
                       commandSHA=sha_words(command))
        rows.append(row)
    return dict(mode='OFF',run=n,frozenContract=['fixture'],actions=rows,readyQpc=n,
                qpcFrequency=1_000_000_000,drain=drain_fixture(n==2))


def physics_fixture():
    scope={'run':1,'owner':2,'surface':3}
    roots=[]
    s=p=tick=0
    for i in range(133):
        admitted=100 if i in (2,3) else 0
        landed=40 if i in (30,61,100) else 0
        s+=landed
        p+=admitted-landed
        kind='INITIAL' if not i else 'SOURCE' if admitted else 'QUANTUM'
        tick+=int(kind=='QUANTUM')
        roots.append(dict(S=s,P=p,landed=landed,admitted=admitted,qpc=i*1000,tick=tick,
                          nonce=i+1,scope=scope,geometry=1,lease=i+1,kind=kind,accepted=True,fence=0))
    root=roots[100]
    image=bytes((100,80,60,255,190,180,120,255))
    probes={name:dict(lease=root['lease'],nonce=root['nonce'],geometry=1,index=i,
                      expectedRGBA=list(image[i*4:i*4+4]),backgroundRGBA=[0,0,0,255],
                      stockQ20=root['S' if name=='bed' else 'P'])
            for i,name in enumerate(('bed','particle'))}
    frame=dict(**dict.fromkeys(RATE.RESULTS,0),nonce=root['nonce'],scope=scope,geometry=1,
               lease=root['lease'],presentQpc=root['qpc']+1,renderer='SHADER',status='CURRENT',
               rgba=image.hex(),imageSHA=sha(image),width=2,height=1,probes=probes)
    return dict(roots=roots,frames=[frame],window=dict(frequency=120000,start=0,stop=10000,
                end=132000,minAfterStopMs=1000,scope=scope))


def tests():
    negative=[]
    def reject(fn,code):
        try:
            fn()
        except ValueError as err:
            need(str(err)==code,f'{code}: {err}')
        else:
            raise AssertionError(code)
        negative.append(code)

    a={k:input_trace(source_fixture(k)) for k in ('normal','gap','lost','device','late')}
    need(a['normal']['admittedQ20']==source_mass(16_000_000)
         and all(a[k]['admittedQ20']==0 for k in ('gap','lost','device','late'))
         and a['normal']['acceptedQuanta']==4 and a['normal']['terminals'][7]=='SUPERSEDED','A_POSITIVE')
    x=source_fixture()
    next(r for r in x if r['kind']=='TAKE')['take']=7
    reject(lambda:input_trace(x),'TAKE_WATERMARK')
    x=source_fixture()
    next(r for r in x if r['kind']=='QUANTUM')['kind']='GRANT'
    reject(lambda:input_trace(x),'EARLY_GRANT')
    x=source_fixture('gap')
    next(r for r in x if r['kind']=='SOURCE_TAIL')['admittedQ20']=1
    reject(lambda:input_trace(x),'SOURCE_LEDGER')
    x=source_fixture()
    next(r for r in x if r['kind']=='SOURCE_TAIL')['ns']=1
    reject(lambda:input_trace(x),'CLOCK_ORDER')

    left,right=off_fixture(1),off_fixture(2)
    equal=clock_pair(left,right,D1.plan())
    need(equal['state']=='OFF_CLOCK_MATCH_FOUR_ARMS_PENDING','OFF_EQUAL')
    right['actions'][6].update(deltaNs=15_000_000,q20=source_mass(15_000_000))
    differs=clock_pair(left,right,D1.plan())
    need(differs['action']==7 and differs['field']=='deltaNs','OFF_FIRST_DIFF')
    x=drain_fixture(True)
    x[1]['fence']=9
    reject(lambda:drain(x),'DRAIN_PENDING_FENCE')
    x=drain_fixture(False)
    x[-2]['writers']=1
    reject(lambda:drain(x),'DRAIN_RESOURCES')
    x=drain_fixture(False)
    x[2]['acceptedRoot']=3
    reject(lambda:drain(x),'FENCE_OWNER_RETIRE')

    c=physics_fixture()
    view=P.long_window(c,sha,RATE.RESULTS)
    need(view['state']=='PHYSICS_AND_PIXELS' and view['postStopLandedQ20']==120,'C_POSITIVE')
    need(P.long_window(dict(c,frames=[]),sha,RATE.RESULTS)['state']=='MISSING_EVIDENCE','MISSING_FRAME')
    x=deepcopy(c)
    x['roots'][10]['admitted']=1
    x['roots'][10]['P']+=1
    x['roots'][10]['kind']='SOURCE'
    reject(lambda:P.long_window(x,sha,RATE.RESULTS),'POST_STOP_SOURCE')
    x=deepcopy(c)
    x['frames'][0]['probes']['bed']['geometry']=2
    reject(lambda:P.long_window(x,sha,RATE.RESULTS),'PIXEL_PHYSICS_IDENTITY')
    plan=P.motion_plan(PTR.timeline())
    observed=[dict(action=p,geometry=p['generation'],activeGeometry=2,observedQpc=i*10+2,
                   inputQpc=i*10+1,acceptedInputQpc=i*10+1,root=str(i+1),shaderFrame=True,
                   sandQ20=0,particleQ20=0,accepted=p['kind']!='STALE_DOWN',effectiveGravity=p['args'])
              for i,p in enumerate(plan)]
    need(P.gravity(plan,observed)=='EMPTY_AND_GRAVITY_JOINED','GRAVITY_POSITIVE')
    observed[-2]['effectiveGravity']=[1,0]
    reject(lambda:P.gravity(plan,observed),'GRAVITY_ROOT')

    with TemporaryDirectory() as tmp:
        routed=[]
        for family,(schema,encoding,profile) in P.FAMILIES.items():
            words=[0]*8192
            words[:5]=P.HEADERS[family]
            tail=[0]*256
            tail[:5]=P.TAIL_HEADERS[family]
            tail[5:7]=[2,1]
            for i,v in ((8,5),(10,6),(18,7),(28,9),(30,10),(54,11)):
                tail[i]=v
            words[256:512]=tail
            path=Path(tmp)/family
            if encoding=='U32LE':
                path.write_bytes(struct.pack('<8192I',*words))
            else:
                path.write_text(''.join(json.dumps({'words':words[i:i+1024]})+'\n'
                                        for i in range(0,len(words),1024)),encoding='utf-8')
            key=dict(nonce='5',run='6',geometry='7',baseCommit='9',commit='10',tick='11')
            env=dict(family=family,schema=schema,encoding=encoding,backend='VULKAN_PC',
                     sha256=sha_file(path),acceptedInterval=key)
            need(archive(path,env)['state']=='MISSING_PC_ROLE_EVIDENCE','PC_FAIL_CLOSED')
            # Decoder spies exercise dispatch only. Real archives use original readers.
            spies={f:(lambda _, chosen=f:chosen) for f in P.FAMILIES}
            env['backend']='HISTORICAL'
            receipt=P.route(path,env,spies,sha_file)
            need(receipt['structure']==family and receipt['acceptedInterval']==key,'D_ROUTE')
            routed.append(family)
            reject(lambda:P.route(path,dict(env,schema='M1PCOLD1'),spies,sha_file),'ARCHIVE_ROUTE')
            reject(lambda:P.route(path,dict(env,sha256='0'*64),spies,sha_file),'G9_ARCHIVE_SHA')
        rejected=tail.copy()
        rejected[6]=4
        reject(lambda:P.interval(rejected,key),'G9_UNACCEPTED_OR_UNSEALED')
    return dict(A=a,B=dict(equal=equal,firstDifference=differs),C=view,D=routed,negative=negative)


def main():
    cmd=argparse.ArgumentParser(description=__doc__)
    cmd.add_argument('mode',choices=('selftest','history','emit','input','drain','off-pair','physics','gravity','archive'))
    cmd.add_argument('path',type=Path,nargs='?')
    cmd.add_argument('--envelope',type=Path)
    args=cmd.parse_args()
    key=args.mode
    if key=='selftest':
        result=tests()
        result['historical']=history()
        result['T836']=PTR.selftest()
        result['T838']=G9.selftest()
        originals=RATE.originals(ROOT/'test/reports/t830/m1-source-window.jsonl')
        need(originals['verifiedBusinessFps'] is None,'C_DIAGNOSTIC_SCOPE')
        result['C_original_diagnostic']=originals
        for name in ('t850','t852','t865'):
            subprocess.run([sys.executable,str(ROOT/f'test/{name}/selftest.py')],cwd=ROOT,check=True,capture_output=True)
        result['regressions']=['T836','T838','T850','T852','T865']
    elif key=='history':
        result=history()
    elif key=='emit':
        result=P.motion_plan(PTR.timeline())
    elif key=='archive':
        result=archive(args.path,json.loads(args.envelope.read_bytes()))
    else:
        data=json.loads(args.path.read_bytes())
        result={'input':lambda:input_trace(data['events']),
                'drain':lambda:drain(data['events']),
                'off-pair':lambda:clock_pair(data['left'],data['right'],D1.plan()),
                'physics':lambda:P.long_window(data,sha,RATE.RESULTS),
                'gravity':lambda:P.gravity(P.motion_plan(PTR.timeline()),data['observed'])}[key]()
    print(json.dumps(result,indent=2))


if __name__=='__main__':
    main()
