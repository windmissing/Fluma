"""Validate independently captured T703 map/accept witnesses and clock anchors."""
from collections import Counter, defaultdict
import hashlib
import json
from pathlib import Path
import re
import struct
import sys

from verify import read

OBJECT = ('contextGeneration', 'shareGroup', 'name', 'life', 'storage', 'binding')
KEY_WORDS = (4, 5, 6, 7, 18, 19, 20, 21, 22, 23, 24, 25, 27, 33, 35, 51, 52, 53, 54)
CLOCK = re.compile(rb'T703_V1 W=1 S=([^ ]+) T=([^ ]+) R=([01])')
STEADY_TAIL_STAGES = {
    43:'BatchCall',44:'Flush',47:'RealPoll',52:'ColdFixture',
    55:'ColdFence',56:'ColdCleanup',66:'BatchFlushEnter',67:'BatchFlushExit',
}

def fields(body):
    return {key.decode(): value for key,value in re.findall(rb'([A-Za-z0-9_]+)=([^ ]+)',body)}

def number(row,key):
    return int(row[key])

def identity(row):
    return tuple(number(row,key) for key in OBJECT)

def original_bytes(row):
    raw=bytes.fromhex(row['hex'].decode())
    assert len(raw)==1024, 'WRONG_COMPLETION_SIZE'
    return raw

def clocks(stamp):
    match=CLOCK.fullmatch(stamp)
    assert match, 'WRONG_CLOCK_SCHEMA'
    script, transport=(group.decode().split(',') for group in match.groups()[:2])
    assert len(script)==7 and len(transport)==9, 'CLOCK_ARITY'
    assert script[0] in ('OWNER','DELIVERY','NA'), 'SCRIPT_CLOCK_MODE'
    assert transport[0] in ('PERMIT','AUTHORIZE','UNKNOWN','NA'), 'TRANSPORT_CLOCK_MODE'
    return (script[0],tuple(map(int,script[1:]))),(transport[0],tuple(map(int,transport[1:]))),match[3]==b'1'

def tail_steady_samples(path,pid):
    samples={}
    for line in path.read_bytes().splitlines():
        snapshot=json.loads(line)
        if snapshot.get('buildProfile')!='T496_HOST_DIAGNOSTIC':
            continue
        assert int(snapshot['pid'])==pid, 'TAIL_PROCESS_SCOPE'
        for attempt in snapshot['attempts']:
            for row in attempt['rows']:
                if row['stage']!='TAIL_OBSERVER' or int(row['a']) not in STEADY_TAIL_STAGES:
                    continue
                stage=STEADY_TAIL_STAGES[int(row['a'])]
                start,end=int(row['ns']),int(row['b'])
                assert start>0 and (end>=start or (stage=='BatchFlushEnter' and end==0))
                samples.setdefault(stage,{'startNs':start,'endNs':end,'rootNonce':int(row['root'])})
    return samples

def validate(directory, *, complete=True):
    root=Path(directory)
    launch=json.loads((root/'launch.json').read_bytes())
    result=json.loads((root/'result.json').read_bytes())
    assert result['binaryTraceSealed'], 'UNSEALED_ARM'
    if complete:
        assert result['exitCodeUnsigned']==0, 'ARM_FAILED'
    attempt=launch['attempt']
    steady=tail_steady_samples(root/f't496-stages_{attempt}.ndjson',launch['pid'])
    stem=root/f'gltrace-{attempt}.ndjson.t693'
    meta=Path(str(stem)+'.meta').read_text().splitlines()[1].split()
    assert meta==[launch['runId'],'create_'+launch['arm'],attempt,str(launch['pid'])], 'WATCHDOG_SCOPE'
    assert launch['parentPid']!=launch['pid'] and launch['totalDeadline']-launch['launch']==420
    watchdog={(s['callId'],s['function'],s['ownerTid']) for s in result['samples']
              if s['consistentEvenVersion'] and int(s['hostSteadyNs']) and s['phase'] in (1,2,3,4)}
    matched_watchdog=set()
    parts=result['partitionBytes']
    assert parts['api']<=12<<20 and parts['businessRaw']<=4<<20 and parts.get('root',0)<=16<<20
    business=list(map(json.loads,(root/f't496_log_{attempt}.ndjson').read_bytes().splitlines()))
    actions={int(row['actionId']):row['declaredScriptNs'] for row in business
             if str(row.get('actionId','')).isdigit()}
    busy=[row['detail'] for row in business if row.get('phase')=='CONTROL_CHECKPOINT']
    pages=[json.loads(row['detail']) for row in business if row.get('phase')=='PAGE_RAW']
    if complete:
        assert len(actions)==52, 'INCOMPLETE_ACTION_SCRIPT'
        assert {'RealPoll','ColdFixture'}<=steady.keys(), 'MISSING_TAIL_STEADY_OBSERVATIONS'
        assert busy==[f'cursor={index} ownerPending=1' for index in (6,18,28,42)], 'BUSY_CUT_COVERAGE'
        if launch['arm']=='D1_ON':
            assert pages and all(p['state']=='FROZEN' and not p['incomplete'] for p in pages), 'CAPTURE_NOT_FROZEN'
        else:
            assert not pages, 'CAPTURE_OFF_PAGES'
    totals=Counter()
    bases=set()
    mapped={}
    replay_maps=defaultdict(list)
    accepts={}
    closed={}
    calls=defaultdict(dict)
    first_unknown=None
    for seq,wall,stamp,tid,fn,edge,detail in read(str(stem)):
        if (seq,fn,tid) in watchdog:
            matched_watchdog.add((seq,fn,tid))
        script,transport,replay=clocks(stamp)
        mode,(first,last,declared,received,checkpoint,consumed)=script
        totals['script_'+mode]+=1
        if mode!='NA':
            assert first in actions and last in actions and checkpoint>0 and 0<received<=wall
            assert actions[first]==actions[last] and declared>=int(actions[first])
            bases.add(declared-int(actions[first]))
            assert ((mode=='OWNER' and received<=consumed<=wall) or
                    (mode=='DELIVERY' and consumed==0)), 'ACTION_CONSUMPTION'
        mode,(nonce,kind,cut,start,deadline,revision,authorize,stamp_value)=transport
        totals['transport_'+mode]+=1
        if mode=='UNKNOWN' and first_unknown is None:
            first_unknown={'seq':seq,'function':fn,'edge':edge,'nonce':nonce,'cut':cut}
        if mode in ('PERMIT','AUTHORIZE'):
            assert nonce and (kind in (1,2) or (kind==0 and cut==9))
            assert cut and revision and 0<start<=authorize<=deadline
            if mode=='AUTHORIZE':
                assert stamp_value==0
        totals['wall_PROCESS_STEADY']+=1
        if replay and fn in (418,419) and edge==1:
            totals['replay_map' if fn==418 else 'replay_unmap']+=1
        if fn in (418,419):
            assert edge not in calls[(tid,seq,fn)], 'DUPLICATE_CALL_EDGE'
            calls[(tid,seq,fn)][edge]=(fields(detail),transport,replay)
        elif fn==980 and edge==5:
            assert mode=='PERMIT' and cut==8 and not replay, 'ACCEPT_PERMIT'
            row=fields(detail)
            raw=original_bytes(row)
            words=struct.unpack('<256I',raw)
            assert nonce==(words[4]|words[5]<<32) and kind==words[27]
            assert tuple(words[i] for i in KEY_WORDS)==tuple(number(row,f'w{i}') for i in KEY_WORDS)
            assert nonce not in accepts, 'DUPLICATE_ACCEPTED_ROOT'
            accepts[nonce]=raw
    for (tid,seq,fn),edges in calls.items():
        assert {1,2,4}<=edges.keys(), 'INCOMPLETE_WRAPPER_CALL'
        enter,root_anchor,replay=edges[1]
        ret,returned_anchor,returned_replay=edges[4]
        assert root_anchor==returned_anchor and replay==returned_replay, 'MAP_PERMIT_CHANGED'
        if fn==419:
            token=number(enter,'mapToken')
            if token:
                assert token not in closed, 'DUPLICATE_UNMAP_TOKEN'
                closed[token]=(enter,ret,tid,seq,root_anchor,number(edges[2][0],'return'))
            continue
        if replay:
            assert root_anchor[0] in ('AUTHORIZE','PERMIT'), 'REPLAY_MAP_AUTHORITY'
            nonce=root_anchor[1][0]
            token=number(ret,'mapToken')
            assert token and number(ret,'nonNull')==1 and all(identity(enter)), 'REPLAY_MAP_IDENTITY'
            assert identity(enter)==identity(ret)
            replay_maps[nonce].append((token,tid,seq,enter,root_anchor,number(edges[2][0],'return')))
            continue
        if 3 not in edges:
            continue
        body,byte_anchor,_=edges[3]
        if replay or root_anchor[1][2]!=6:
            continue
        assert byte_anchor==root_anchor and root_anchor[0]=='PERMIT', 'MISSING_MAP_PERMIT'
        assert number(enter,'offset')==0 and number(enter,'length')==1024 and number(enter,'access')&1, 'MAP_RANGE_OR_ACCESS'
        assert number(ret,'nonNull')==1 and number(edges[2][0],'return')!=0, 'MAP_RETURN_NULL'
        token=number(ret,'mapToken')
        assert token and all(identity(enter)) and identity(enter)==identity(ret)==identity(body), 'MAP_OBJECT_IDENTITY'
        assert number(body,'mapToken')==token, 'MAP_TOKEN_MISMATCH'
        raw=original_bytes(body)
        assert hashlib.sha256(raw).hexdigest().encode()==body['sha256']
        nonce=root_anchor[1][0]
        assert nonce not in mapped
        mapped[nonce]=(raw,token,tid,seq,enter,root_anchor)
    assert mapped.keys()==accepts.keys() and mapped, 'ACCEPTED_MAP_COVERAGE'
    assert len(bases)==1 and matched_watchdog, 'CLOCK_OR_WATCHDOG_SCOPE'
    if complete:
        assert len(mapped)==92, 'INCOMPLETE_ROOT_COVERAGE'
        assert first_unknown is None, ('TRANSPORT_CLOCK_UNKNOWN',first_unknown)
    def assert_unmapped(token,tid,seq,enter,anchor):
        assert token in closed, 'MISSING_UNMAP_TOKEN'
        unmap,returned,owner,sequence,unmap_anchor,original_result=closed[token]
        assert (tid==owner and seq<sequence and anchor==unmap_anchor and
                identity(unmap)==identity(enter) and
                number(unmap,'target')==number(enter,'target')), 'UNMAP_OBJECT_MISMATCH'
        assert number(returned,'success')==1 and number(returned,'matched')==1 and original_result==1, 'UNMAP_FAILED'
    for nonce,(raw,token,tid,seq,enter,anchor) in mapped.items():
        assert accepts[nonce]==raw, 'ACCEPTED_BYTES_DIFFER'
        assert_unmapped(token,tid,seq,enter,anchor)
    replay_path=root/f't693-replay-{attempt}.root.ndjson'
    replay_count=0
    if replay_path.is_file():
        *rows, footer=map(json.loads,replay_path.read_bytes().splitlines())
        assert footer['roots']==len(rows) and footer['firstRejected']==0, 'REPLAY_ROOT_FOOTER'
        assert len(rows)==len(accepts), 'REPLAY_ROOT_COVERAGE'
        for row in rows:
            nonce=row['nonceLo']|row['nonceHi']<<32
            columns=[('bank','bankMapped','bankBytes')]
            if row['eventBytes']:
                columns.append(('event','eventsMapped','eventBytes'))
            observations=replay_maps.pop(nonce)
            assert len(observations)==len(columns), 'REPLAY_MAP_CARDINALITY'
            assert row['startedSteadyNs']>0 and row['elapsedNs']>0
            for (token,tid,seq,enter,anchor,pointer),(suffix,mapped_field,bytes_field) in zip(observations,columns):
                assert pointer==row[mapped_field]
                assert tuple(number(enter,k) for k in ('name','target','offset','length','access'))==(
                    row[suffix+'Name'],row[suffix+'Target'],row[suffix+'Offset'],
                    row[bytes_field],row[suffix+'Access'])
                assert_unmapped(token,tid,seq,enter,anchor)
                replay_count+=1
    assert not replay_maps, 'UNBOUND_REPLAY_MAP'
    return {'arm':launch['arm'],'actions':len(actions),'acceptedRoots':len(accepts),'partitionBytes':parts,
            'completeArm':complete,'exitCode':result['exitCodeUnsigned'],
            'watchdogMatchedCalls':len(matched_watchdog),'declaredClockBase':next(iter(bases)),
            'replayReadMapsMatched':replay_count,
            'busyCuts':busy,'frozenPages':len(pages),
            'tailObserverSteadyExamples':steady,
            'clockRows':dict(totals),'firstTransportUnknown':first_unknown,'device':'NOT_RUN'}

if __name__=='__main__':
    import argparse
    args=argparse.ArgumentParser()
    args.add_argument('--partial',action='store_true')
    args.add_argument('arms',nargs='+')
    options=args.parse_args()
    print(json.dumps([validate(path,complete=not options.partial) for path in options.arms],indent=2))
