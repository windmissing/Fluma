"""Verify T903 PC replay archives and family-specific SOURCE retirement."""
import argparse
from collections import defaultdict
from copy import deepcopy
import json
import math
from pathlib import Path
import struct
import sys
from tempfile import TemporaryDirectory

ROOT = Path(__file__).resolve().parents[2]
ART = ROOT/'test/reports'


sys.path.insert(0,str(ROOT/'test/t496'))
from check_t896 import T897, T888, replay

OLD, need, pair = T888.OLD, T888.need, T888.pair
FAMILIES = {
    'DM2': ('3639e57f834308bab001ca783f321b6a503c232caa924e04b1d8f1d7e3333ca6',
            ('PREPARE','SOURCE_WRITER','AUDIT_COMPARE_PACK_SEAL')),
    'R1': ('f7d812d64eba3c932fcfc41d70fbed48835349819f235b6d9d61fb58216641f3',
           ('PREPARE','ACTUAL_WRITER','REPLAY_AUDIT_APPLY','COMPARE_PACK_AUDIT_SEAL')),
}
COPY = ('COLD_COPY_SUBMITTED','COLD_COPY_WAIT','COLD_COPY_READBACK')


def strict_rows(path):
    rows, receipt = OLD.read_rows(path)
    need(not receipt['offerQuoteRepairLines'],'STRICT_JSON')
    return rows


def sand_rows(path, family):
    lines = path.read_bytes().splitlines()
    if lines and lines[-1]==f'{family} ROOT_NOT_ACCEPTED'.encode():
        lines = lines[:-1]
    return [json.loads(line) for line in lines]


def cold(rows, files):
    roots = OLD.accepted(rows)
    copies = defaultdict(list)
    for row in rows:
        if row.get('stage') in COPY:
            copies[row['nonce']].append(row)
    archives = {}
    waits = []
    for path in files:
        result = OLD.root_archive(path,roots)
        nonce = result['nonce']
        need(nonce not in archives,'DUPLICATE_ARCHIVE')
        root = roots[nonce]
        t = root['words']
        run, geo, generation = (str(pair(t,i)) for i in (6,38,36))
        evidence = copies.get(nonce,[])
        need([r['stage'] for r in evidence]==list(COPY),'COPY_STAGE_CHAIN')
        submit, wait, read = evidence
        for r in evidence:
            need((r['run'],r['geometry'],r['publicationGeneration'],r['rootKind'])==(run,geo,generation,root['kind'])
                 and tuple(r[k] for k in ('baseBank','finalBank','publicationSlot','eventArena','eventStart','eventEnd','eventCapacity'))
                 ==tuple(t[i] for i in (32,33,35,51,52,53,54)), 'COPY_ROOT_SCOPE')
        f = ('businessFence','businessFenceEnterQpc','businessFenceExitQpc','ranges')
        need(all(r[k]==submit[k] for r in evidence for k in f)
             and all(r['businessFenceVkResult']==0 for r in evidence) and int(submit['businessFence'])>0
             and submit['copyFence']==wait['copyFence'] and int(wait['copyFence'])>0
             and 0<int(submit['businessFenceEnterQpc'])<=int(submit['businessFenceExitQpc'])
             <=int(submit['submitEnterQpc'])<=int(submit['submitExitQpc'])<=int(wait['waitEnterQpc'])
             <=int(wait['waitExitQpc']), 'COPY_FENCE_ORDER')
        need(submit['submitVkResult']==wait['waitVkResult']==read['mapVkResult']==read['invalidateVkResult']==0,
             'COPY_NOT_COMPLETE')
        auth_key = 'm1SourceAuthority' if root['kind']=='SOURCE' else 'm1QuantumAuthority'
        need(any(a['nonce']==nonce and a['fenceObject']==submit['businessFence']
                 and a['lastWait'][:2]==[submit['businessFenceEnterQpc'],submit['businessFenceExitQpc']]
                 and a['lastWait'][3]==0 and a['acceptedRoot']==nonce
                 for r in root['rows'] if (a:=r.get(auth_key))), 'COPY_BUSINESS_ROOT_FENCE')
        ranges = submit['ranges']
        need([r['bytes'] for r in ranges]==[1024,29696,256*t[53],31744]
             and len({r['slot'] for r in ranges})==4
             and all(int(r['buffer'])>0 and int(r['allocation'])>0 and r['generation']>0 and r['offset']==0
                     for r in ranges), 'COPY_RANGES')
        field = 'm1SourceColdBacking' if root['kind']=='SOURCE' else 'm1ColdBacking'
        need(any([int(x[k]) for k in ('bankBuffer','eventBuffer','publicationBuffer')]
                 ==[int(r['buffer']) for r in ranges[1:]] for record in root['rows'] if (x:=record.get(field))),
             'COPY_BACKING_IDENTITY')
        archives[nonce] = result
        waits.append(wait)
    need(archives and set(archives)==set(copies)
         and set(archives)==set(roots) and {r['kind'] for r in archives.values()}=={'SOURCE','QUANTUM'},
         'COPY_INVENTORY')
    stages = ('END_INTAKE','M1_D1_DRAIN','M1_D1_CLOSE')
    need(all(len(T888.rows_at(rows,s))==1 for s in stages),'COPY_DRAIN_MISSING')
    seal, drain, close = (T888.exactly(rows,s) for s in stages)
    need(rows.index(seal)<rows.index(drain)<rows.index(close),'COPY_DRAIN_ORDER')
    data = T897.d1(rows)
    authority = [r[key] for r in rows for key in ('m1SourceAuthority','m1QuantumAuthority') if key in r]
    status = T888.d1_drain(seal,drain['m1D1Drain'],close,data['hz'],authority,waits)
    return dict(archives=archives,drain=status,ownerSessions=data['sessions'])


def sand(rows, family):
    role_sha, spans = FAMILIES[family]
    timelines = T888.rows_at(rows,'ROOT_TIMELINE')
    need(len(timelines)==1,'ROOT_TIMELINE_MISSING')
    t = timelines[0]
    root, = [r for r in T888.rows_at(rows,'ROOT') if r['kind']=='SOURCE']
    need(t['family']==family and t['roleSetSHA']==role_sha and t['rootKind']=='SOURCE'
         and min(int(t[k]) for k in ('run','nonce','device','queue','plan','fence','fenceGeneration'))>0, 'ROOT_FAMILY')
    need(all(root[k]==t[k] for k in ('family','run','nonce','plan','fence','fenceGeneration','deadlineNs'))
         and root['accepted']==(t['acceptedRoot'] is not None)
         and root['dispatches']==(12 if family=='DM2' else 19)
         and t['staging']['bytes']==1024, 'ROOT_RECEIPT')
    ns = lambda r,k:int(r[k])
    need(ns(t,'deadlineNs')-ns(t,'grantNs')==250_000_000
         and 0<ns(t,'grantNs')<=ns(t,'inputFrozenNs')<=ns(t,'expectedReadyNs')
         <=ns(t,'recordEnterNs')<=ns(t,'recordExitNs')<=ns(t,'submitEnterNs')<=ns(t,'submitExitNs')
         <=ns(t,'stopNs') and t['submitVkResult']==0, 'ROOT_TIMELINE_ORDER')
    clock = t['clock']
    need(0<ns(clock,'seq') and 0<ns(clock,'callbackNs')<=ns(clock,'receiptNs')<=ns(clock,'observedNs')
         and clock['ageNs']==ns(clock,'observedNs')-ns(clock,'receiptNs'), 'ROOT_PULSE_CLOCK')
    polls = t['polls']
    need(len(polls)<=t['pollCount'] and bool(t['lastPoll'])==bool(t['pollCount'])
         and (not t['pollCount'] or polls and
              (len(polls)<t['pollCount'] or polls[-1]==t['lastPoll'])), 'ROOT_POLL_COUNT')
    previous = ns(t,'submitExitNs')
    for poll in polls + ([t['lastPoll']] if len(polls)<t['pollCount'] else []):
        need(previous<=ns(poll,'enterNs')<=ns(poll,'exitNs')<=ns(t,'stopNs')
             and 0<ns(poll,'timeoutNs')<=2_000_000, 'ROOT_POLL_ORDER')
        previous = ns(poll,'exitNs')
    accepted = t['acceptedRoot'] is not None
    if accepted:
        need(str(t['acceptedRoot'])==t['nonce'] and t['lastPoll']['vkResult']==t['readbackFenceVkResult']
             ==t['invalidateVkResult']==0 and ns(t,'stopNs')<=ns(t,'deadlineNs')
             and 0<ns(t,'readbackEnterNs')<=ns(t,'readbackExitNs')<=ns(t,'gateEnterNs')
             <=ns(t,'gateExitNs')<=ns(t,'promoteEnterNs')<=ns(t,'promoteExitNs'), 'ROOT_ACCEPTANCE')
    else:
        need(ns(t,'promoteEnterNs')==ns(t,'promoteExitNs')==0, 'REJECTED_PROMOTED')
    evidence = [r for r in rows if r.get('stage') in ('RETIRE','DEVICE_DRAIN')]
    end = ns(t,'stopNs')
    fence = t['lastPoll']['vkResult'] if t['lastPoll'] else 1
    retired = fence==0
    for r in evidence:
        need(not retired and not accepted and r['family']==family and str(r['nonce'])==t['nonce']
             and r['fence']==t['fence'] and r['fenceGeneration']==t['fenceGeneration']
             and r['acceptedRoot'] is None and end<=ns(r,'enterNs')<=ns(r,'exitNs'), 'RETIRE_FENCE_IDENTITY')
        end = ns(r,'exitNs')
        if r['stage']=='DEVICE_DRAIN':
            need((r['device'],r['queue'])==(t['device'],t['queue']), 'RETIRE_DEVICE_QUEUE')
            fence = r['fenceVkResult']
            if fence==0:
                need(r['idleVkResult']==0 and end<=ns(r,'confirmEnterNs')<=ns(r,'confirmExitNs'), 'RETIRE_DRAIN_CONFIRM')
                end = ns(r,'confirmExitNs')
        else:
            need(r['run']==t['run'],'RETIRE_RUN')
            fence = r['vkResult']
        need(r['state']==({0:'RETIRED',-4:'DEVICE_LOST'}.get(fence,'RETIRE_PENDING')), 'RETIRE_STATE')
        retired = fence==0
    queries = T888.rows_at(rows,'ROOT_GPU_TIMESTAMPS')
    need(len(queries)==1 or not retired and not queries,'QUERY_ROW_MISSING')
    availability = None
    if queries:
        q = queries[0]
        need((not evidence or rows.index(evidence[-1])<rows.index(q)) and all(str(q[k])==str(t[k]) for k in
             ('family','run','nonce','plan','device','queue','fence','fenceGeneration'))
             and q['acceptedRoot']==t['acceptedRoot'] and q['fenceVkResult']==fence,'QUERY_ROOT_SCOPE')
        expected = 'ACCEPTED_FENCE' if accepted else 'LATE_RETIRED_DIAGNOSTIC' if retired and evidence else (
            'REJECTED_ROOT_DIAGNOSTIC' if retired else 'DEVICE_LOST' if fence==-4 else 'RETIRE_PENDING')
        need(q['state']==expected,'QUERY_STATE')
        points = q['points']
        enabled = q['timestampRequested'] and q['queryCreateVkResult']==0 and int(q['queryPool'])>0
        readable = retired and enabled
        bits, period = q['validBits'], q['periodNs']
        need([p['i'] for p in points]==list(range(len(spans)+3))
             and (not enabled or 1<=bits<=64 and math.isfinite(period) and period>0),'QUERY_LAYOUT')
        for p in points:
            need((p['tick'] is not None)==p['available']
                 and (not p['available'] or readable and p['vkResult']==0 and int(p['tick'])>=0),
                 'QUERY_AVAILABILITY')
        need([s['name'] for s in q['spans']]==['UPLOAD',*spans,'TAIL_COPY'],'QUERY_FAMILY_SPANS')
        for i,s in enumerate(q['spans']):
            a,b = points[i:i+2]
            if a['available'] and b['available']:
                duration = ((int(b['tick'])-int(a['tick'])) % (1<<bits))*period
                need(s['ns'] is not None and math.isclose(s['ns'],duration,rel_tol=2e-5,abs_tol=1),'QUERY_SPAN')
            else:
                need(s['ns'] is None,'QUERY_MISSING_SPAN')
        availability = sum(p['available'] for p in points)
    return dict(family=family,accepted=accepted,firstStop=t['firstStop'],
                retirement='RETIRED' if retired else 'DEVICE_LOST' if fence==-4 else 'RETIREMENT_UNPROVEN',
                pointsAvailable=availability)


def rejects(call, code):
    try:
        call()
    except ValueError as exc:
        need(str(exc)==code,f'EXPECTED_{code}: {exc}')
    else:
        raise AssertionError(code)
    return code


def sand_fixture(family):
    role_sha, spans = FAMILIES[family]
    scope = dict(family=family,run='1',nonce='2',plan='3',device='4',queue='5',fence='6',fenceGeneration='7')
    poll = dict(enterNs='249000000',exitNs='250000002',timeoutNs='2000000',vkResult=2)
    timeline = dict(scope,stage='ROOT_TIMELINE',roleSetSHA=role_sha,rootKind='SOURCE',grantNs='1',
                    deadlineNs='250000001',inputFrozenNs='2',expectedReadyNs='3',recordEnterNs='4',
                    recordExitNs='5',submitEnterNs='6',submitExitNs='7',submitVkResult=0,stopNs='250000002',
                    firstStop='GRANT_DEADLINE',clock=dict(seq='9',callbackNs='9',receiptNs='10',observedNs='11',ageNs=1),
                    polls=[poll],pollCount=1,lastPoll=poll,promoteEnterNs='0',promoteExitNs='0',
                    acceptedRoot=None,staging={'bytes':1024})
    receipt = dict(scope,stage='ROOT',kind='SOURCE',accepted=False,deadlineNs=timeline['deadlineNs'],
                   dispatches=12 if family=='DM2' else 19)
    retire = dict(scope,stage='RETIRE',enterNs='250000003',exitNs='250000004',vkResult=0,
                  state='RETIRED',acceptedRoot=None)
    query = dict(scope,stage='ROOT_GPU_TIMESTAMPS',state='LATE_RETIRED_DIAGNOSTIC',acceptedRoot=None,
                 fenceVkResult=0,timestampRequested=True,queryCreateVkResult=0,queryPool='8',validBits=8,periodNs=2,
                 points=[dict(i=i,available=True,vkResult=0,tick=str((250+10*i)%256)) for i in range(len(spans)+3)],
                 spans=[dict(name=k,ns=20) for k in ('UPLOAD',*spans,'TAIL_COPY')])
    return [receipt,retire,query,timeline]


def cold_fixture(directory):
    rows = strict_rows(ART/'t896/b-off1.jsonl')
    files = []
    for nonce, root in OLD.accepted(rows).items():
        t = root['words']
        source = root['kind']=='SOURCE'
        key = 'm1SourceAuthority' if source else 'm1QuantumAuthority'
        carrier = next(r for r in root['rows'] if r.get(key,{}).get('nonce')==nonce)
        auth = carrier[key]
        sizes = [1024,29696,t[53]*256,31744,0,0,0,0]
        payload = OLD.D1.packed(t,256)+bytes(sum(sizes)-1024)
        digest = OLD.sha(payload)
        file = directory/f'fixture-{nonce}.m1root'
        file.write_bytes(OLD.HEADER.pack(b'M1PROOT1',*sizes,bytes.fromhex(digest))+payload)
        files.append(file)
        carrier['m1SourceReplaySHA' if source else 'm1QuantumReplaySHA'] = digest
        carrier['m1SourceColdBacking' if source else 'm1ColdBacking'] = {
            'bankSlot':t[33],'publicationSlot':t[35],'eventArena':t[51],'eventEnd':t[53],
            'fence':0,'invalidate':0,'format':'M1PROOT1','bankBuffer':'2','eventBuffer':'3','publicationBuffer':'4'}
        enter,exit = auth['lastWait'][:2]
        b = dict(run=str(pair(t,6)),geometry=str(pair(t,38)),publicationGeneration=str(pair(t,36)),
                 rootKind=root['kind'],nonce=nonce,businessFence=auth['fenceObject'],
                 businessFenceEnterQpc=enter,businessFenceExitQpc=exit,businessFenceVkResult=0,
                 **dict(zip(('baseBank','finalBank','publicationSlot','eventArena','eventStart','eventEnd','eventCapacity'),
                            (t[i] for i in (32,33,35,51,52,53,54)))),
                 ranges=[dict(slot=i,buffer=str(i+1),allocation=str(i+5),generation=1,offset=0,bytes=size)
                         for i,size in enumerate(sizes[:4])])
        rows.extend([dict(b,stage=COPY[0],copyFence='8',submitEnterQpc=exit,submitExitQpc=exit,submitVkResult=0),
                     dict(b,stage=COPY[1],copyFence='8',waitEnterQpc=exit,waitExitQpc=exit,waitVkResult=0),
                     dict(b,stage=COPY[2],mapVkResult=0,invalidateVkResult=0)])
    return rows, files


def selftest():
    controls = []
    sand_results = {}
    for family in FAMILIES:
        rows = sand_fixture(family)
        late = sand(rows,family)
        pending = sand([rows[0],rows[3]],family)
        need(late['retirement']=='RETIRED' and pending['retirement']=='RETIREMENT_UNPROVEN','FIXTURE_SAND')
        partial = deepcopy(rows)
        partial[2]['points'][0].update(available=False,tick=None,vkResult=1)
        partial[2]['spans'][0]['ns']=None
        sand_results[family]=dict(late=late,pending=pending,partial=sand(partial,family))
        for index,change,code in ((1,{'fence':'999'},'RETIRE_FENCE_IDENTITY'),
                                  (2,{'acceptedRoot':2},'QUERY_ROOT_SCOPE')):
            altered = deepcopy(rows)
            altered[index].update(change)
            controls.append(rejects(lambda:sand(altered,family),code))
        controls.append(rejects(lambda:sand([rows[0],rows[1],rows[3]],family),'QUERY_ROW_MISSING'))
    with TemporaryDirectory() as tmp:
        rows, files = cold_fixture(Path(tmp))
        result = cold(rows,files)
        cold_rows = deepcopy(rows)
        drain = T888.exactly(cold_rows,'M1_D1_DRAIN')['m1D1Drain']
        wait = T888.rows_at(cold_rows,COPY[1])[-1]
        drain.update(ownerFenceKind='COLD_COPY_SIGNALED',ownerFence=wait['copyFence'],
                     ownerFenceNonce=wait['nonce'],ownerFenceEnterQpc=wait['waitEnterQpc'],
                     ownerFenceExitQpc=wait['waitExitQpc'])
        need(cold(cold_rows,files)['drain']=='COLD_COPY_SIGNALED','FIXTURE_COLD_DRAIN')
        drain['ownerFence']='123'
        controls.append(rejects(lambda:cold(cold_rows,files),'DRAIN_COLD_FENCE'))
        controls.append(rejects(lambda:cold(rows,[*files,files[0]]),'DUPLICATE_ARCHIVE'))
        controls.append(rejects(lambda:cold([r for r in rows if r.get('stage')!='M1_D1_CLOSE'],files),'COPY_DRAIN_MISSING'))
        for stage, field, value, code in ((COPY[1],'geometry','0','COPY_ROOT_SCOPE'),
                                         (COPY[1],'waitVkResult',2,'COPY_NOT_COMPLETE')):
            altered = deepcopy(rows)
            T888.rows_at(altered,stage)[0][field]=value
            controls.append(rejects(lambda:cold(altered,files),code))
        path = files[0]
        raw = path.read_bytes()
        path.write_bytes(raw[:-1]+bytes([raw[-1]^1]))
        controls.append(rejects(lambda:cold(rows,files),'ROOT_CONTAINER'))
    history = {p.stem:replay(p) for p in sorted((ART/'t896').glob('b-replay*.jsonl'))}
    need(len(history)==3,'T896_REPLAY_INVENTORY')
    historical_sand = {}
    for family in FAMILIES:
        p = ART/'t896'/f'c-{family.lower()}-boot.jsonl'
        raw = p.read_bytes()
        *structured, terminal = raw.splitlines()
        need(terminal==f'{family} ROOT_NOT_ACCEPTED'.encode(),'T896_SAND_TERMINAL')
        rows = sand_rows(p,family)
        root = T888.exactly(rows,'ROOT')
        retire = T888.exactly(rows,'RETIRE')
        need(root['family']==family and root['vkResult']==2 and root['accepted'] is False,'T896_SAND_REJECT')
        historical_sand[family] = dict(sha256=OLD.sha(raw),sourceVkResult=2,legacyRetireVkResult=retire['vkResult'],
                                       currentGate=rejects(lambda:sand(rows,family),'ROOT_TIMELINE_MISSING'))
    return dict(syntheticArchives=len(result['archives']),syntheticSand=sand_results,negativeControls=controls,
                historicalReplayFailure=history,historicalSand=historical_sand,strictFourArm='T496_T865_ONLY')


def main():
    p = argparse.ArgumentParser(description=__doc__)
    sub = p.add_subparsers(dest='mode',required=True)
    sub.add_parser('selftest')
    c = sub.add_parser('cold')
    c.add_argument('jsonl',type=Path)
    c.add_argument('archive_dir',type=Path)
    s = sub.add_parser('sand')
    s.add_argument('family',choices=tuple(FAMILIES))
    s.add_argument('jsonl',type=Path)
    args = p.parse_args()
    try:
        if args.mode=='cold':
            result = cold(strict_rows(args.jsonl),sorted(args.archive_dir.glob('*.m1root')))
        elif args.mode=='sand':
            result = sand(sand_rows(args.jsonl,args.family),args.family)
        else:
            result = selftest()
    except (ValueError,KeyError,IndexError,OSError) as exc:
        print(json.dumps(dict(state='INVALID_OR_MISSING_EVIDENCE',reason=str(exc))))
        return 2
    print(json.dumps(result,indent=2))
    if args.mode=='sand' and result['retirement']!='RETIRED':
        return 3
    return 0


if __name__=='__main__':
    sys.exit(main())
