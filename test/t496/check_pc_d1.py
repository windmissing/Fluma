"""Independent T875/T869 offline join for the T496 Windows Vulkan continuation."""
import hashlib
import importlib.util
import json
from pathlib import Path
import re
import struct

ROOT = Path(__file__).resolve().parents[2]
ART = ROOT/'test/reports'
HEADER = struct.Struct('<8s8I32s')
TAIL = struct.Struct('<256I')
KINDS = ('PULSE', 'SOURCE', 'RESERVE', 'END')
SCOPE = ('pid', 'processStartTick', 'ticket', 'ownerTid', 'surface', 'geometry')


def load(name, path):
    spec=importlib.util.spec_from_file_location(name,ROOT/path)
    module=importlib.util.module_from_spec(spec)
    spec.loader.exec_module(module)
    return module


D1=load('t865','test/t865/check.py')
COLD=load('t848','test/t848/check.py')
need=D1.need


def sha(raw):
    return hashlib.sha256(raw).hexdigest()


def read_rows(path):
    raw=path.read_bytes()
    rows=[]
    corrected=[]
    for number, line in enumerate(raw.splitlines(),1):
        try:
            row=json.loads(line)
        except ValueError:
            need(line.startswith((b'{"stage":"M1_OFFER_RETURN",',b'{"stage":"M1_OFFER_TERMINAL",')),
                 'UNRECOGNIZED_JSON_ERROR')
            fixed, n=re.subn(rb'("actionId":[0-9]+)"(,"sample":)',rb'\1\2',line)
            need(n==1,'UNRECOGNIZED_OFFER_JSON_ERROR')
            row=json.loads(fixed)
            corrected.append(number)
        rows.append(row)
    return rows, {'sha256':sha(raw),'lines':len(rows),'offerQuoteRepairLines':corrected}


def accepted(rows):
    found={}
    for row in rows:
        for kind,name,nonce,owner,fence in (
                ('SOURCE','m1Completion','m1Nonce','m1OwnerCode','m1FenceVkResult'),
                ('QUANTUM','m1QuantumCompletion','m1QuantumNonce','m1QuantumOwnerCode','m1QuantumFenceVkResult')):
            if name not in row or row.get(owner)!=0 or row.get(fence)!=0:
                continue
            words=row[name]
            D1.packed(words,256)
            ident=str(COLD.pair(words,4))
            need(ident==row[nonce] and tuple(words[:4])==COLD.SEAL and words[28]==1
                 and words[27]==(1 if kind=='SOURCE' else 2),'ACCEPTED_TAIL')
            scope=tuple(row[k] for k in SCOPE)
            if ident in found:
                prev=found[ident]
                need(prev['scope']==scope and prev['words']==words and prev['kind']==kind,'ROOT_CONFLICT')
                prev['rows'].append(row)
            else:
                found[ident]=dict(scope=scope,words=words,kind=kind,rows=[row])
    return found


def offers(rows, run, plan, roots):
    calls=[r for r in rows if r.get('stage') in ('M1_OFFER_RETURN','M1_OFFER_TERMINAL')]
    need(not any(r.get('stage')=='M1_OFFER_TRACE_INCOMPLETE' for r in rows),'OFFER_TRACE_LOSS')
    need([int(r['serial']) for r in calls]==list(range(1,len(calls)+1)),'OFFER_SERIAL')
    out={}
    fields=('run','geometry','offer','actionId','sample','hold','pulseTicks','sampleTicks','frequency',
            'pulseNs','sampleNs','pixelX','pixelY','material')
    scope=next(iter(roots.values()))['scope'] if roots else None
    for action in (a for a in plan if a['kind']=='SOURCE'):
        aid=action['actionId']
        pair=[c for c in calls if c['actionId']==aid]
        if not pair:
            out[aid]={'outcome':'MISSING_OFFER'}
            continue
        need(len(pair)==2 and [p['stage'] for p in pair]==['M1_OFFER_RETURN','M1_OFFER_TERMINAL'],
             'OFFER_TERMINAL_CHAIN')
        returned, done=pair
        need(all(returned[k]==done[k] for k in fields) and returned['result']==0
             and done['run']==run and done['material']==0
             and (scope is None or str(scope[-1])==done['geometry']),'OFFER_IDENTITY')
        code=done['result']
        if code!=2:
            need(code in (3,4,7) and done['root']=='0','OFFER_REJECT_TERMINAL')
            out[aid]={'outcome':{3:'REJECTED',4:'REVOKED',7:'EXPIRED'}[code],'offer':done['offer']}
            continue
        root=roots[done['root']]
        need(root['kind']=='SOURCE' and str(COLD.pair(root['words'],6))==run,'OFFER_SOURCE_ROOT')
        bindings=[r for r in root['rows'] if r.get('m1OfferAction')==aid]
        need(bool(bindings),'OFFER_ACTION_JOIN')
        row=bindings[0]
        producer=row['m1ProducerWords']
        need(len(producer)==25 and sha(struct.pack('<24Q',*producer[:-1]))==row['m1ProducerSHA']
             and [producer[i] for i in (0,4,8,9,10,11,12,13,14,15,16,17,18,21,22,24)]==
                 [int(done[k]) if k else expected for k,expected in (
                    ('run',0),('geometry',0),('offer',0),('pulseNs',0),('pulseTicks',0),('sampleTicks',0),
                    ('frequency',0),(None,2),('hold',0),('sample',0),('sampleNs',0),('pixelX',0),('pixelY',0),
                    ('material',0),('actionId',0),('root',0))], 'PRODUCER_WORDS')
        command=row['m1SourceWords']
        raw=D1.raw_bytes(command,row['m1SourceWordsSHA'],32)
        requested=struct.unpack_from('<d',raw,80)[0]
        words=root['words']
        need(command[:3]==[1129534023,2,0] and COLD.pair(command,4)==COLD.pair(words,6)
             and COLD.pair(command,12)==COLD.pair(words,38)
             and COLD.pair(command,28)==COLD.pair(words,40) and command[14:16]==words[59:61]
             and words[61]==row['m1AdmittedQ']>0 and struct.unpack_from('<2d',raw,64)==(action['x'],action['y']),
             'COMMAND_AND_SOURCE_TAIL')
        out[aid]=dict(outcome='ACCEPTED',offer=done['offer'],root=done['root'],admittedQ20=words[61],
                      requested=requested,deltaNs=producer[23],commandSHA=row['m1SourceWordsSHA'])
    need(len(calls)==2*sum(v['outcome']!='MISSING_OFFER' for v in out.values()),'EXTRA_OFFER')
    return out


def arm(path, mode, *, primary=True):
    rows, artifact=read_rows(path)
    plan=D1.plan()
    actions=[r for r in rows if r.get('stage')=='M1_D1_ACTION']
    ready,=[r for r in rows if r.get('stage')=='M1_D1_READY']
    need(ready['mode']==mode and [(r['id'],KINDS[r['kind']],int(r['scriptNs'])) for r in actions]==
         [(a['actionId'],a['kind'],a['scriptNs']) for a in plan],'D1_ACTION_PLAN')
    stamps=[int(row['actualNs']) for row in actions]
    need(stamps==sorted(stamps) and min(stamps)>=int(ready['readyNs']),'D1_ACTION_CLOCK')
    roots=accepted(rows)
    resolved=offers(rows,ready['run'],plan,roots)
    retired=[r for r in rows if r.get('state')=='M1_QUANTUM_PRESENT_RETIRED']
    if primary:
        need(len(retired)==1 and retired[0]['m1PublicationReleased'] is True
             and retired[0]['m1WsiResult']==retired[0]['m1WsiRetireVkResult']==0,'D1_WSI_RETIRE')
        need(all(r['state']=='DELIVERED' for r in actions if r['kind']==0)
             and all(r['state']=='AWAIT_PULSE' for r in actions if r['kind']==1),'D1_ACTION_TERMINAL')
        need(all(str(COLD.pair(r['words'],6))==ready['run'] for r in roots.values()) and roots,
             'D1_RUN')
    pages=[r for r in rows if r.get('stage')=='M1_D1_HOT_PAGE']
    for page in pages:
        nonce=page['root']
        need(page['request']==19 and int(page['safePoint'])>0 and roots[nonce]['words']==page['completion']
             and actions[18]['state']=='RESERVED' and actions[42]['state']=='ENDED','D1_HOT_JOIN')
    return rows, roots, dict(artifact, mode=mode, run=ready['run'], actionRows=len(actions),
                             sourceTerminals=resolved, acceptedRoots=len(roots), wsiRetired=bool(retired),
                             drain=retired[0].get('m1D1Drain') if retired else None,
                             d1HotRoots=[p['root'] for p in pages],
                             captureActions=[actions[i]['state'] for i in (18,42)],
                             faults=sorted({str(r[k]) for r in rows
                                            for k in ('m1PulseFault','m1QuantumReject','m1SourceReject') if r.get(k)}))


def normalized_arm(rows, report):
    """Project the recorded action and drain evidence to the T865 front gates."""
    ready=next(r for r in rows if r.get('stage')=='M1_D1_READY')
    actions=[r for r in rows if r.get('stage')=='M1_D1_ACTION']
    drain=dict(report['drain'])
    drain['fence']=drain.pop('retireVkResult')
    projected=[]
    for action,row in zip(D1.plan(),actions,strict=True):
        state=row['state']
        if action['kind']=='SOURCE':
            state=report['sourceTerminals'][action['actionId']]['outcome']
        elif action['kind']=='PULSE' and state=='DELIVERED':
            state='ACCEPTED'
        projected.append(dict(action=action,terminal=state,terminalId=row['id'],actualNs=int(row['actualNs'])))
    return dict(mode=report['mode'],synthetic=False,process=report['run'],readyNs=int(ready['readyNs']),
                contract={},clockEquivalent=False,drain=drain,actions=projected,roots=[])


def root_archive(path, mapping, busy=False):
    raw=path.read_bytes()
    need(len(raw)>=HEADER.size,'ROOT_HEADER')
    magic,*sizes,digest=HEADER.unpack_from(raw)
    payload=raw[HEADER.size:]
    need(magic==b'M1PROOT1' and sum(sizes)==len(payload) and hashlib.sha256(payload).digest()==digest
         and sizes[:2]==[1024,29696] and sizes[3:]==[31744,0,0,0,0], 'ROOT_CONTAINER')
    words=list(TAIL.unpack_from(payload))
    nonce=str(COLD.pair(words,4))
    if not busy:
        need(path.stem.rsplit('-',1)[-1]==nonce,'ROOT_FILE_NONCE')
    r=mapping[nonce]
    need(r['words']==words and sizes[2]==256*words[53] and words[52]<=words[53]<=words[54],
         'ROOT_WORDS_AND_RANGE')
    source=r['kind']=='SOURCE'
    key='m1ColdPayloadSHA' if busy else 'm1SourceReplaySHA' if source else 'm1QuantumReplaySHA'
    backing='m1SourceColdBacking' if source else 'm1ColdBacking'
    matches=[x for x in r['rows'] if x.get(key)==digest.hex() and backing in x]
    need(matches,'ROOT_BACKING_RECEIPT')
    for x in matches:
        b=x[backing]
        need((b['bankSlot'],b['publicationSlot'],b['eventArena'],b['eventEnd'])==
             tuple(words[i] for i in (33,35,51,53))
             and b['fence']==b['invalidate']==0 and b['format']=='M1PROOT1'
             and len({int(b[k]) for k in ('bankBuffer','eventBuffer','publicationBuffer')})==3
             and all(int(b[k])>0 for k in ('bankBuffer','eventBuffer','publicationBuffer')),'ROOT_BACKING')
    sections=[]; offset=0
    for size in sizes[:4]:
        sections.append(sha(payload[offset:offset+size]))
        offset+=size
    return dict(kind=r['kind'],nonce=nonce,scope=r['scope'],payloadSHA=digest.hex(),bytes=len(payload),
                sectionSHA=sections, fileSHA=sha(raw))


def pages(rows, digest, payload_bytes, root):
    data=[r for r in rows if r.get('stage')=='M1_COLD_PAGE']
    reserve,=[r for r in rows if r.get('stage')=='M1_HOT_CAPTURE_RESERVED']
    at=0
    for r in data:
        need(r['request']==reserve['request'] and r['root']==root and r['offset']==at
             and 0<r['count']<=65536 and r['total']==payload_bytes and r['sha']==digest,'COLD_PAGES')
        at+=r['count']
    need(at==payload_bytes and data,'COLD_PAGE_COVERAGE')
    return len(data)


def d2():
    directory=ART/'t869'
    a,a_receipt=read_rows(directory/'runA-coldbusy.jsonl')
    full=COLD.join(COLD.read_archive(directory/'runA-m1pcold1.bin'),a)
    a_receipt['pages']=pages(a,full['payloadSHA256'],full['decodedBytes']-72,str(full['root']['acceptedRoot']))
    a_receipt['root']=full['root']['acceptedRoot']
    b,b_receipt=read_rows(directory/'runB-rootbusy.jsonl')
    mapping=accepted(b)
    r=root_archive(directory/'runB-m1proot1.bin',mapping,busy=True)
    need(any(id_!=r['nonce'] and root['kind']=='QUANTUM' and int(id_)<int(r['nonce'])
             and root['scope']==tuple(r['scope']) for id_,root in mapping.items()),'FOLLOWING_ROOT')
    b_receipt.update(root=r,pages=pages(b,r['payloadSHA'],r['bytes'],r['nonce']))
    c,c_receipt=read_rows(directory/'runC-hothot.jsonl')
    mapping=accepted(c)
    anchors=[]
    for page in [x for x in c if x.get('stage')=='M1_HOT_CAPTURE']:
        root=mapping[page['root']]
        anchors.append(dict(request=page['request'],root=page['root'],run=str(COLD.pair(root['words'],6)),
                            completionSHA=sha(D1.packed(root['words'],256)),accepted=True))
    c_receipt.update(D1.hot(c,anchors))
    return {'first':a_receipt,'following':b_receipt,'hot':c_receipt}


def controls(rows):
    from copy import deepcopy
    plan=D1.plan()
    ready=next(r for r in rows if r.get('stage')=='M1_D1_READY')
    result=[]
    for field,value,reason in (('actionId',41,'OFFER_TERMINAL_CHAIN'),('pulseTicks','0','OFFER_IDENTITY')):
        altered=deepcopy(rows)
        next(r for r in altered if r.get('stage')=='M1_OFFER_TERMINAL')[field]=value
        try:
            offers(altered,ready['run'],plan,accepted(altered))
        except ValueError as error:
            need(str(error)==reason,'NEGATIVE_REASON')
        else:
            raise AssertionError(reason)
        result.append(reason)
    return result


def main():
    directory=ART/'t875'
    roles=dict(off='OFF',on='ON',**{'replay-off':'REPLAY_OFF','replay-on':'REPLAY_ON','on-retry':'ON'})
    main_arms={}; historical={}; by_root={}; projected={}
    for prefix,mode in roles.items():
        for suffix in ('','2') if prefix!='on-retry' else ('',):
            name=prefix+suffix
            rows, roots, report=arm(directory/(name+'.jsonl'),mode,primary=bool(suffix))
            (main_arms if suffix else historical)[name]=report
            if suffix:
                projected[mode]=normalized_arm(rows,report)
            if mode.startswith('REPLAY'):
                for nonce,root in roots.items():
                    need(nonce not in by_root,'CROSS_PROCESS_NONCE_COLLISION')
                    by_root[nonce]=root
    archive={p.name:root_archive(p,by_root) for p in sorted(directory.glob('*.m1root'))}
    need(len(archive)==len(json.loads((directory/'summary.json').read_bytes())['replayRoots']),'REPLAY_INVENTORY')
    masses=[r['sourceTerminals'][7]['admittedQ20'] for r in main_arms.values()]
    actual_ns=[r['sourceTerminals'][7]['deltaNs'] for r in main_arms.values()]
    need(len(set(masses))>1 and len(set(actual_ns))>1,'RECHECK_EQUIVALENCE')
    gates={}
    for mode,row in projected.items():
        try:
            D1.arm(row,mode)
        except ValueError as error:
            need(str(error) in ('DRAIN','CLOCK_EQUIVALENCE_UNPROVEN'),'T865_UNEXPECTED_GATE')
            gates[mode]=str(error)
        else:
            raise AssertionError('strict T865 unexpectedly accepted unmatched real clocks')
    try:
        D1.compare(projected)
    except ValueError as error:
        need(str(error)=='DRAIN','T865_UNEXPECTED_GATE')
    else:
        raise AssertionError('strict T865 unexpectedly accepted incomplete manifests')
    negative=controls(read_rows(directory/'off2.jsonl')[0])
    print(json.dumps(dict(schema='T496_PC_OFFLINE_JOIN_V1',arms=main_arms,historical=historical,
                          replayArchives=archive,d2=d2(),strictD1='NOT_COMPARABLE_REAL_INPUT_DIFF',
                          strictT865Gates=gates,observedSourceQ20=masses,observedSourceDeltaNs=actual_ns,
                          negativeControls=negative),indent=2))


if __name__=='__main__':
    main()
