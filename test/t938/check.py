"""T934 G16 FREEZE isolation and T933 private OFF evidence checks."""
import argparse
from copy import deepcopy
from hashlib import sha256
import importlib.util
import json
from pathlib import Path
import struct
import subprocess
import sys
from tempfile import TemporaryDirectory

ROOT = Path(__file__).resolve().parents[2]
ART = ROOT/'test/reports'
ABI = json.loads((ROOT/'docs/architecture-T928-abi-v1.json').read_bytes())
BANK_BYTES = 1024*64
REFERENCE_SHA = '922ddef7e9e26010f394068841df8f1d7551a1e501f685366eea17661ebd6951'
LOG = ART/'t927/runs/off-pair-t926-full/off-1.jsonl'


def need(condition,code):
    if not condition:
        raise ValueError(code)


def load(name,path):
    spec = importlib.util.spec_from_file_location(name,ROOT/path)
    mod = importlib.util.module_from_spec(spec)
    spec.loader.exec_module(mod)
    return mod


T928 = load('t928','test/t928/fixtures.py')


def sha(raw):
    return sha256(raw).hexdigest()


def words(raw):
    need(len(raw)%4==0,'WORD_ALIGNMENT')
    return list(struct.unpack(f'<{len(raw)//4}I',raw))


def wide(w,i):
    return w[i]|w[i+1]<<32


def first_bad(bank,n):
    need(n in (13,52) and len(bank)==BANK_BYTES,'DISK_BANK_SHAPE')
    need(bank[64*n:]==bytes(BANK_BYTES-64*n),'ACTIVE_TAIL_NONZERO')
    for i,raw in enumerate(T928.DISK.iter_unpack(bank[:n*64]),1):
        r,m,I=raw[10:13]
        expected=.005 if n==13 else .0025
        expected_m=.01 if n==13 else .0025
        f32=lambda x:struct.unpack('<f',struct.pack('<f',x))[0]
        if (raw[:4]!=(i,1,i,1) or r!=f32(expected) or m!=f32(expected_m)
                or not abs(I-.5*m*r*r)<=2e-14 or raw[13:]!=(1,0,0)):
            return i
    return None


def header(e):
    size=e['n']
    b0,b1=(bytes.fromhex(e[k]) for k in ('baseBefore','baseAfter'))
    c0,c1=(bytes.fromhex(e[k]) for k in ('candidateBefore','candidateAfter'))
    need(len(c0)==len(c1)==BANK_BYTES and len(b0)==len(b1)==BANK_BYTES,'DISK_BANK_SHAPE')
    old,new=e['prior'],e['after']
    disk=first_bad(b0,size)
    need(e['candidateBank']==1-old['activeBank'] and old['activeBank'] in (0,1),'BANK_BINDING')
    if disk is not None:
        need(e['completion']==dict(outcome=2,firstReason=2,firstStage=1,firstDisk=disk,stageMask=0x801)
             and e['sealed'] is True,'FIRST_DISK_REJECT')
        need(old==new and b0==b1 and e['acceptedRoot'] is None and e['newPublication'] is False,
             'REJECT_VISIBLE_SIDE_EFFECT')
        prefix=(disk-1)*64
        need(c1==b0[:prefix]+c0[prefix:],'PRIVATE_PREFIX')
        return dict(state='ATOMIC_REJECT',firstDisk=disk,privateCopied=disk-1)
    n=size*64
    need(e['completion']['outcome']==1 and e['sealed'] is True and b0==b1 and e['fenceVkResult']==0
         and e['completion']['firstReason']==0 and e['completion']['firstStage']==0
         and e['fenceGeneration']==e['expectedFenceGeneration'] and e['livePermit']
         and new['activeBank']==e['candidateBank'] and all(new[k]==old[k]+1 for k in
             ('commit','quanta','stateGeneration','sceneGeneration')),'ACCEPT_BANK_SWAP')
    need(first_bad(c1,size) is None and all(c1[i:i+16]==b0[i:i+16] and c1[i+40:i+64]==b0[i+40:i+64]
                                           for i in range(0,n,64)),'ACCEPT_DISK_IMMUTABLE')
    return dict(state='ACTIVE_BANK_VALID',bank=new['activeBank'])


def historical():
    base=ART/'t932/runs/g16b'
    results={}
    for path in sorted(base.glob('g16-*')):
        cfg=words((path/'final-config.u32le').read_bytes())
        t=words((path/'completions.u32le').read_bytes()[-1024:])
        raw=(path/'seed.disk64').read_bytes()
        n=cfg[4]
        seed=next(s for s in ABI['seeds'] if s['n']==n)
        need(sha(raw)==seed['proof']['uploadSHA256'] and (path/'base.disk64').read_bytes()==raw
             and not any((path/'candidate.disk64').read_bytes()),'T932_ORIGINAL_BANK')
        need(t[40:43]==[2,2,1] and t[45]==1 and t[63]==0x801 and t[54]==t[62]==0
             and t[88:128]==cfg[64:104] and t[48:52]==cfg[40:44]
             and wide(t,6)==wide(cfg,16),'T932_ORIGINAL_FREEZE')
        need(first_bad(raw.ljust(BANK_BYTES,b'\0'),n) is None,'T932_CURRENT_FLAGS')
        results[path.name]=dict(n=n,nonce=wide(t,6),firstDisk=t[45],seedSHA=sha(raw))
    need(len(results)==4,'T932_ORIGINAL_COUNT')
    return results


def fixture_capture(directory,log):
    rows=[json.loads(r) for r in log.read_bytes().splitlines()]
    stage=lambda s:[r for r in rows if r.get('stage')==s]
    final,=stage('G16_FREEZE_FIXTURE_RESULT')
    proof,=stage('G16_FREEZE_FIXTURE_BYTES')
    cold,=stage('G16_COLD')
    cfg=words((directory/'final-config.u32le').read_bytes())
    tail=words((directory/'completions.u32le').read_bytes()[-1024:])
    name=('trusted-base-bank','fixture-base-bank','fixture-candidate-bank')
    trusted,faulty,candidate=[(directory/(k+'.disk64')).read_bytes() for k in name]
    need(all(len(v)==BANK_BYTES for v in (trusted,faulty,candidate))
         and [sha(v) for v in (trusted,faulty,candidate)]==
             [proof[k] for k in ('trustedBaseSHA','faultyBaseSHA','privateCandidateSHA')],'CAPTURE_BANK_SHA')
    n=cfg[4]
    diffs=[i for i,(a,b) in enumerate(zip(words(trusted),words(faulty),strict=True)) if a!=b]
    fault=first_bad(faulty,n)
    need(first_bad(trusted,n) is None and diffs and len(diffs)==1 and fault==diffs[0]//16+1
         and diffs[0]%16 in (1,2,3,13,14,15),'CAPTURE_FAULT_DELTA')
    nonce=str(wide(cfg,16))
    root,=[r for r in stage('G16_PRIVATE_ROOT') if r['nonce']==nonce]
    need(final['passed'] is True and final['firstDisk']==fault
         and tail[40:43]==[2,2,1] and tail[45]==fault and tail[63]==0x801
         and wide(tail,6)==wide(cfg,16) and tail[88:128]==cfg[64:104]
         and all(tail[i]==0 for i in (20,21,24,25,54,56,58,59,62))
         and final['baseBank']==cfg[40] and final['privateCommit']==str(wide(cfg,22))
         and final['stateGeneration']==str(wide(cfg,26)) and final['sceneGeneration']==str(wide(cfg,30))
         and (directory/'base.disk64').read_bytes()==trusted[:64*n] and not (directory/'scene.scene32').exists()
         and all(r.get('acceptedRoot') is None for r in (root,cold,proof,final)), 'CAPTURE_VISIBLE_SIDE_EFFECT')
    need(cold['nonce']==proof['nonce']==nonce and root['fence']==cold['fence']
         and int(root['fenceGeneration'])+1==int(cold['fenceGeneration']) and root['identityOK'] is True
         and root['current'] is True and root['fresh'] is True
         and all(r[k]==0 for r in (root,cold) for k in ('submit','retire','vkResult')), 'CAPTURE_SAME_FENCE')
    prefix=(fault-1)*64
    prior_candidate=(directory/'candidate.disk64').read_bytes()
    need(len(prior_candidate)==64*n
         and candidate==faulty[:prefix]+prior_candidate[prefix:]+bytes(BANK_BYTES-64*n),'CAPTURE_PRIVATE_PREFIX')
    return dict(state='ACTUAL_ATOMIC_REJECT',disk=fault,bank=cfg[40],nonce=nonce)


def bundle_identity():
    base=ROOT/'.codex/local-test/t936-g16-bundle'
    manifest=base/'manifest.json'
    if not manifest.is_file():
        return {'state':'LOCAL_BUNDLE_MISSING'}
    doc=json.loads(manifest.read_bytes())
    for name,r in doc['files'].items():
        raw=(base/name).read_bytes()
        need(sha(raw)==r['sha256'] and len(raw)==r['bytes'],'T936_BUNDLE_HASH')
    module=doc['shader']['spvSHA']
    need(module=='786bb1db9b8d3a679042bdc36d014226d68647913770d503192c297e0c10e361'
         and all(module.encode() in (base/name).read_bytes()
                 for name in ('t909_angle_probe.exe','t936_g16_freeze_fixture.exe')),'T936_PE_MODULE_PAIR')
    controls=json.loads((base/'t936-g16-offline-controls.json').read_bytes())
    need(len(controls)==19 and all(r['exitCode']==2 for r in controls)
         and sum(r['reason']=='G16_MODULE_SHA' for r in controls)==2
         and sum(r['reason']=='G16_SEED_SHA' for r in controls)==10,'T936_HOST_CONTROL_RECEIPT')
    with TemporaryDirectory() as tmp:
        d=Path(tmp)
        seed=bytearray((base/'G16-B13.disk64').read_bytes())
        seed[12*64+52]=3
        bad=d/'late-flags.disk64'
        bad.write_bytes(seed)
        out=d/'must-not-exist'
        run=subprocess.run([str(base/'t909_angle_probe.exe'),'g16-multi',str(base/'g16_multi_v1.spv'),
                            str(bad),'static','128','1',str(out)],capture_output=True,timeout=10)
        need(run.returncode==2 and b'G16_SEED_SHA' in run.stderr+run.stdout and not out.exists(),
             'T936_INDEPENDENT_HOST_REJECT')
    return dict(state='STATIC_PAIR_VERIFIED',files=len(doc['files']),moduleSHA=module,
                hostControlsReceipt=len(controls),independentHostReject='G16_SEED_SHA')


def reference(raw):
    need(len(raw)==2480 and raw[:8]==b'M1OFF933' and sha(raw)==REFERENCE_SHA and raw[8:40].hex()==sha(LOG.read_bytes()),
         'REFERENCE_IDENTITY')
    cb0,cb1,rx0,rx1,hz,delta,session,q=struct.unpack_from('<8Q',raw,40)
    p=struct.unpack_from('<25Q',raw,104)
    cmd=words(raw[304:432])
    s=words(raw[432:1456])
    t=words(raw[1456:])
    rows=[json.loads(line) for line in LOG.read_bytes().splitlines()]
    a,b=[next(r for r in rows if r.get('stage')=='M1_D1_PULSE_OWNER' and r['actionId']==i) for i in (6,8)]
    row,=[r for r in rows if r.get('m1OfferAction')==7 and 'm1SourceWords' in r]
    need(tuple(row['m1ProducerWords'])==p and row['m1SourceWords']==cmd and row['m1Completion']==s
         and row['m1QuantumCompletion']==t and sha(raw[304:432])==row['m1SourceWordsSHA']
         and sha(raw[104:296])==row['m1ProducerSHA'],'REFERENCE_ORIGINAL_WORDS')
    ns=lambda x:x*10**9//hz
    need(cb0<=rx0<cb1<=rx1 and 0<ns(rx1)-ns(rx0)<=33_333_333 and ns(cb1)-ns(cb0)==delta
         and (p[10],p[12],p[23])==(cb1,hz,delta) and q==s[61]
         and all(int(r['session'])==session and int(r['run'])==p[0] and int(r['geometry'])==p[4]
                 and r['outcome']=='ACCEPTED' for r in (a,b))
         and int(b['seq'])==int(a['seq'])+1
         and [int(r['callbackQpc']) for r in (a,b)]==[cb0,cb1]
         and [int(r['nativeReceiptQpc']) for r in (a,b)]==[rx0,rx1]
         and p[19:21]==struct.unpack_from('<QQ',raw,368)
         and q==round(struct.unpack_from('<d',raw,384)[0]*(1<<20)),
         'REFERENCE_REAL_ADJACENCY')
    return dict(sha=sha(raw),delta=delta,q=q,command=cmd,pulses=[cb0,cb1,rx0,rx1],frequency=hz,
                run=p[0],source=s,quantum=t)


SECTIONS=('source-base','source-command','source-completion','quantum-base','quantum-candidate',
          'quantum-independent','source-ledger','source-events','quantum-events','quantum-publication',
          'quantum-completion')


def evidence(rows,ref,observe):
    final,=[r for r in rows if r.get('state')=='M1_PRIVATE_OFF_COMPLETE']
    for r in rows:
        need(r.get('acceptedRoot') is None and not r.get('windowFrame') and not r.get('m1SceneLease')
             and r.get('state') not in ('M1_QUANTUM_PRESENT_QUEUED','M1_EMPTY_PRESENT_QUEUED')
             and all(r.get(k,{}).get('acceptedRoot') is None for k in ('m1SourceAuthority','m1QuantumAuthority')),
             'PRIVATE_FAKE_PUBLICATION')
    need('acceptedRoot' in final and final['m1PrivateReferenceSHA']==ref['sha']
         and int(final['m1PrivateReferenceDeltaNs'])==ref['delta']
         and final['m1PrivateObservation'] is observe
         and bool(final.get('m1PrivateObservationSHA')) is observe,'PRIVATE_TERMINAL_MODE')
    return final


def private_arm(rows,files,ref,observe,observation=None):
    final=evidence(rows,ref,observe)
    need(set(files)==set(SECTIONS) and (observation is not None) is observe,'PRIVATE_FILE_SET')
    w={k:words(v) for k,v in files.items()}
    s,q,cmd=(w[k] for k in ('source-completion','quantum-completion','source-command'))
    need(len(s)==len(q)==256 and len(cmd)==32 and s[:4]==q[:4]==[1381192263,2,256,3235823838]
         and s[27:32]==[1,1,0,0,0] and q[27:32]==[2,1,0,0,0] and q[26]==1
         and s[14:18]==[0]*4 and s[61]==ref['q'] and wide(q,14)==wide(s,18) and wide(q,16)==wide(s,20)
         and wide(s,77)==wide(q,77)==ref['q'],'PRIVATE_ROOT_CHAIN')
    run=wide(s,6)
    need(run>0 and run!=ref['run'] and run==wide(q,6)==wide(cmd,4)
         and wide(s,38)==wide(q,38)==wide(cmd,12) and wide(s,40)==wide(q,40)==wide(cmd,28)
         and s[8:14]==q[8:14]
         and cmd[16:22]==ref['command'][16:22] and cmd[2:4]==ref['command'][2:4]
         and cmd[14:16]==s[59:61],'PRIVATE_COMMAND_MAPPING')
    hz=int(final['qpcFrequency'])
    for tail,key,result_key in ((s,'m1SourceAuthority','m1FenceVkResult'),(q,'m1QuantumAuthority','m1QuantumFenceVkResult')):
        auth=final[key]
        last=auth['lastWait']
        need(int(auth['run'])==run and int(auth['geometry'])==wide(tail,38)
             and int(auth['nonce'])==int(auth['privateRoot'])==wide(tail,4)
             and auth['submitVkResult']==final[result_key]==last[3]==0
             and auth['fencePending'] is False and int(auth['fenceObject'])>0 and hz>0
             and 0<int(auth['callbackTick'])<=int(auth['receiptTick'])<=int(last[1])
             and 0<int(last[0])<=int(last[1])
             and int(last[1])*10**9//hz<=int(auth['deadlineNs'])
             and int(auth['deadlineNs'])-int(auth['grantNs'])==250_000_000,'PRIVATE_CURRENT_FENCE')
    base_copy,end_copy=(final[k] for k in ('m1PrivateBaseCopy','m1PrivateEvidenceCopy'))
    need(all(r['vkResult']==0 and int(r['fence'])>0 and 0<int(r['enterQpc'])<=int(r['exitQpc']) for r in (base_copy,end_copy))
         and int(base_copy['exitQpc'])<int(final['m1SourceAuthority']['lastWait'][0])
         and int(final['m1QuantumAuthority']['lastWait'][1])<=int(end_copy['enterQpc']),'PRIVATE_COPY_FENCE')
    need(final['m1PrivateClockBalanced'] is True and final['m1PrivateConsumed']==1
         and final['m1PrivateGenerated']==sum(final[k] for k in
             ('m1PrivateConsumed','m1PrivatePending','m1PrivateActive','m1PrivateOverload')),'PRIVATE_CLOCK_ACCOUNTING')
    need(all(len(w[k])==7424 for k in ('source-base','quantum-base','quantum-candidate','quantum-independent'))
         and len(w['quantum-publication'])==7936 and not any(w['source-base'][:-256]),'PRIVATE_BASE_OR_INDEPENDENT')
    candidate,spare,base=(w[k] for k in ('quantum-candidate','quantum-independent','quantum-base'))
    h=7424-256
    post_finalize={47,48,72,73,76,77,78}
    differences=[i for i,(a,b) in enumerate(zip(candidate,spare,strict=True)) if a!=b]
    if differences:
        need(all(i-h in post_finalize for i in differences)
             and wide(candidate,h+47)==wide(q,108)
             and wide(spare,h+47)==wide(base,h+47)
             and wide(candidate,h+72)==wide(q,36)==wide(spare,h+72)+1
             and candidate[h+76:h+79]==[q[33],q[32],q[34]]
             and spare[h+76:h+79]==base[h+76:h+79],'PRIVATE_INDEPENDENT_FINALIZE')
    need(all(0<=t[52]<=t[53]<=t[54] and len(w[k])==t[53]*64
             for t,k in ((s,'source-events'),(q,'quantum-events')))
         and q[52]==0 and len(w['source-ledger'])==s[56]*16,'PRIVATE_EVENT_RANGE')
    if observe:
        need(len(observation)>=72,'PRIVATE_OBSERVATION_SIZE')
        magic,*sizes,digest=struct.unpack_from('<8s8I32s',observation)
        keys=('quantum-completion','quantum-candidate','quantum-events','quantum-publication')
        body=b''.join(files[k] for k in keys)
        need(magic==b'M1PROOT1' and sizes==[len(files[k]) for k in keys]+[0]*4
             and observation[72:]==body and digest.hex()==sha(body)==final['m1PrivateObservationSHA'],
             'PRIVATE_OBSERVATION_WORDS')
    normalized={k:list(v) for k,v in w.items()}
    for key in ('source-base','quantum-base','quantum-candidate','quantum-independent','quantum-publication'):
        h=len(w[key])-256
        need(wide(w[key],h+4)==run,'PRIVATE_STATE_RUN')
        stop=12 if key=='quantum-publication' else 14
        normalized[key][h+4:h+stop]=[0]*(stop-4)
    for key in ('source-completion','quantum-completion'):
        normalized[key][4:14]=[0]*10
    normalized['source-command'][4:6]=[0,0]
    normalized['source-command'][22:24]=[0,0]
    for key,t in (('source-events',s),('quantum-events',q)):
        for i in range(t[52],t[53]):
            j=64*i
            need(wide(w[key],j+2)==wide(t,4),'PRIVATE_EVENT_ROOT')
            normalized[key][j+2:j+4]=[0,0]
    modules=tuple(final[k] for k in ('m1SourceModuleSHA','m1QuantumCandidateSHA','m1QuantumCommitSHA'))
    need(all(len(h)==64 and len(bytes.fromhex(h))==32 for h in modules),'PRIVATE_MODULE_IDENTITY')
    return dict(run=run,rawSHA={k:sha(v) for k,v in files.items()},words=normalized,observe=observe,modules=modules,
                spareFinalizeDifferences=differences,carry=wide(q,108))


def compare_private(a,b):
    need(a['run']!=b['run'] and a['observe'] is False and a['modules']==b['modules'],'PRIVATE_PAIR_ROLE')
    for k in SECTIONS:
        x,y=a['words'][k],b['words'][k]
        if len(x)!=len(y):
            return dict(state='WORD_LENGTH_DIFF',section=k,left=len(x),right=len(y))
        for i,(u,v) in enumerate(zip(x,y,strict=True)):
            if u!=v:
                return dict(state='WORD_DIFF',section=k,word=i,left=u,right=v)
    return dict(state='PRIVATE_PHYSICS_WORD_MATCH',formalD1=False)


def disk_fixture(seed,slot=None,word=13,value=1,bank=0):
    raw,_=T928.pack_seed(seed,ABI['boxHalfExtentUm'],ABI['material'])
    base=bytearray(raw.ljust(BANK_BYTES,b'\0'))
    n=seed['n']
    if slot:
        struct.pack_into('<I',base,(slot-1)*64+4*word,value)
    old=dict(activeBank=bank,commit=bank,quanta=bank,stateGeneration=bank+1,sceneGeneration=bank+1,lease=f'old{bank}')
    prefix=(slot-1)*64 if slot else n*64
    new=dict(old) if slot else dict(activeBank=1-bank,commit=bank+1,quanta=bank+1,
                                   stateGeneration=bank+2,sceneGeneration=bank+2,lease=f'new{bank}')
    result=dict(outcome=2,firstReason=2,firstStage=1,firstDisk=slot,stageMask=0x801) if slot else (
        dict(outcome=1,firstReason=0,firstStage=0))
    return dict(n=n,baseBefore=base.hex(),baseAfter=base.hex(),candidateBefore='00'*BANK_BYTES,
                candidateAfter=(base[:prefix]+bytes(BANK_BYTES-prefix)).hex(),prior=old,after=new,candidateBank=1-bank,
                completion=result,sealed=True,acceptedRoot=None,newPublication=False,fenceVkResult=0,
                fenceGeneration=1,expectedFenceGeneration=1,livePermit=True)


def private_fixture(ref,run,observe=False):
    def packed(v):
        return struct.pack(f'<{len(v)}I',*v)
    s,q,cmd=[list(ref[k]) for k in ('source','quantum','command')]
    s[6:8]=q[6:8]=cmd[4:6]=[run,0]
    cmd[22:24]=[run+100,0]
    def state(n,active=0):
        w=[0]*n
        w[-252]=run
        w[0]=active
        return packed(w)
    def events(t):
        w=[0]*(t[53]*64)
        for i in range(t[52],t[53]):
            w[64*i+2:64*i+4]=t[4:6]
        return packed(w)
    files=dict(zip(SECTIONS,(state(7424),packed(cmd),packed(s),state(7424,7),state(7424,9),
                             state(7424,9),bytes(s[56]*64),events(s),events(q),state(7936,9),packed(q)),strict=True))
    obs=None
    row=dict(state='M1_PRIVATE_OFF_COMPLETE',acceptedRoot=None,qpcFrequency='10000000',
             m1PrivateReferenceSHA=ref['sha'],m1PrivateReferenceDeltaNs=str(ref['delta']),
             m1PrivateObservation=observe,m1FenceVkResult=0,m1QuantumFenceVkResult=0,
             m1PrivateClockBalanced=True,m1PrivateConsumed=1,m1PrivateGenerated=2,m1PrivatePending=1,
             m1PrivateActive=0,m1PrivateOverload=0,
             m1SourceModuleSHA='a'*64,m1QuantumCandidateSHA='b'*64,m1QuantumCommitSHA='c'*64,
             m1PrivateBaseCopy=dict(vkResult=0,fence='6',enterQpc='1',exitQpc='2'),
             m1PrivateEvidenceCopy=dict(vkResult=0,fence='7',enterQpc='31',exitQpc='32'))
    for key,t,wait in (('m1SourceAuthority',s,10),('m1QuantumAuthority',q,20)):
        row[key]=dict(run=str(run),geometry=str(wide(t,38)),nonce=str(wide(t,4)),privateRoot=str(wide(t,4)),
                      acceptedRoot=None,submitVkResult=0,fencePending=False,fenceObject='9',
                      callbackTick='4',receiptTick='5',lastWait=[str(wait),str(wait+1),2_000_000,0],
                      grantNs='900',deadlineNs='250000900')
    if observe:
        keys=('quantum-completion','quantum-candidate','quantum-events','quantum-publication')
        body=b''.join(files[k] for k in keys)
        digest=sha(body)
        row['m1PrivateObservationSHA']=digest
        obs=struct.pack('<8s8I32s',b'M1PROOT1',*[len(files[k]) for k in keys],0,0,0,0,bytes.fromhex(digest))+body
    return [row],files,obs


def rejects(call,code):
    try:
        call()
    except ValueError as exc:
        need(str(exc)==code,f'EXPECTED_{code}: {exc}')
    else:
        raise AssertionError(code)


def selftest():
    original=historical()
    negatives=[]
    disk=[]
    for seed in ABI['seeds']:
        raw,_=T928.pack_seed(seed,ABI['boxHalfExtentUm'],ABI['material'])
        need(sha(raw)==seed['proof']['uploadSHA256'],'HOST_SEED_SHA')
        mirror=bytearray(raw.ljust(BANK_BYTES,b'\0'))
        for slot in range(seed['n']):
            for word in (4,6,8,9):
                at=slot*64+word*4
                bits=struct.unpack_from('<I',mirror,at)[0]
                struct.pack_into('<I',mirror,at,bits^0x80000000)
        need(first_bad(mirror,seed['n']) is None and sha(mirror[:len(raw)])!=sha(raw),'MIRROR_ACTIVE_FLAGS')
        for bank in (0,1):
            header(disk_fixture(seed,bank=bank))
            for slot in (1,2,seed['n']):
                for word,value in ((13,0),(13,2),(13,3),(14,1),(15,1),(1,2),(2,99),(3,2)):
                    result=header(disk_fixture(seed,slot,word,value,bank))
                    need(result['firstDisk']==slot,'DISK_FIRST_SLOT')
                    disk.append(result)
        invalid=disk_fixture(seed,seed['n'],13,3,1)
        invalid['after']['lease']='fake'
        rejects(lambda:header(invalid),'REJECT_VISIBLE_SIDE_EFFECT')
        negatives.append('REJECT_VISIBLE_SIDE_EFFECT')
        invalid=disk_fixture(seed,2,13,2)
        invalid['candidateAfter']='00'*BANK_BYTES
        rejects(lambda:header(invalid),'PRIVATE_PREFIX')
        negatives.append('PRIVATE_PREFIX')
        invalid=disk_fixture(seed)
        invalid['baseBefore']=invalid['baseBefore'][:-2]+'01'
        rejects(lambda:header(invalid),'ACTIVE_TAIL_NONZERO')
        negatives.append('ACTIVE_TAIL_NONZERO')
    with TemporaryDirectory() as tmp:
        dest=Path(tmp)/'reference.bin'
        subprocess.run([sys.executable,'pc/vulkan/t933_off_reference.py',str(LOG),str(dest)],
                       cwd=ROOT,check=True,capture_output=True)
        raw=dest.read_bytes()
    ref=reference(raw)
    altered=raw[:80]+bytes([raw[80]^1])+raw[81:]
    rejects(lambda:reference(altered),'REFERENCE_IDENTITY')
    negatives.append('REFERENCE_IDENTITY')
    left,lf,lo=private_fixture(ref,100)
    right,rf,ro=private_fixture(ref,101)
    on,nf,no=private_fixture(ref,102,True)
    a=private_arm(left,lf,ref,False,lo)
    b=private_arm(right,rf,ref,False,ro)
    c=private_arm(on,nf,ref,True,no)
    comparisons=[compare_private(a,z) for z in (b,c)]
    need(all(r['state']=='PRIVATE_PHYSICS_WORD_MATCH' for r in comparisons),'PRIVATE_FIXTURE_MATCH')
    for key,value in (('acceptedRoot',99),('windowFrame',{'root':99})):
        wrong=deepcopy(left)
        wrong[0][key]=value
        rejects(lambda:private_arm(wrong,lf,ref,False),'PRIVATE_FAKE_PUBLICATION')
        negatives.append('PRIVATE_FAKE_PUBLICATION')
    wrong=deepcopy(right)
    wrong[0]['m1QuantumAuthority']['lastWait'][3]=2
    rejects(lambda:private_arm(wrong,rf,ref,False),'PRIVATE_CURRENT_FENCE')
    negatives.append('PRIVATE_CURRENT_FENCE')
    changed=bytes([rf['quantum-candidate'][0]^1])+rf['quantum-candidate'][1:]
    b=private_arm(right,dict(rf,**{'quantum-candidate':changed,'quantum-independent':changed}),ref,False)
    first=compare_private(a,b)
    need(first['state']=='WORD_DIFF' and first['section']=='quantum-candidate' and first['word']==0,'PRIVATE_DIFF')
    negatives.append('PRIVATE_FIRST_PHYSICAL_WORD_DIFF')
    return dict(t932=original,t936=bundle_identity(),diskRejectFixtures=len(disk),negativeControls=negatives,
                reference={k:ref[k] for k in ('sha','delta','q','pulses','frequency')},comparisons=comparisons,
                firstWordNegative=first,actualPrivate='PENDING_T937',formalD1='T496_ONLY')


def main():
    parser=argparse.ArgumentParser(description=__doc__)
    sub=parser.add_subparsers(dest='mode',required=True)
    sub.add_parser('selftest')
    disk=sub.add_parser('disk')
    disk.add_argument('receipt',type=Path)
    fault=sub.add_parser('fault')
    fault.add_argument('directory',type=Path)
    fault.add_argument('log',type=Path)
    private=sub.add_parser('private')
    for name in ('reference','left_log','left_dir','right_log','right_dir'):
        private.add_argument(name,type=Path)
    private.add_argument('--observe',action='store_true')
    args=parser.parse_args()
    if args.mode=='selftest':
        result=selftest()
    elif args.mode=='disk':
        result=header(json.loads(args.receipt.read_bytes()))
    elif args.mode=='fault':
        result=fixture_capture(args.directory,args.log)
    else:
        ref=reference(args.reference.read_bytes())
        arms=[]
        for path,directory,observe in ((args.left_log,args.left_dir,False),(args.right_log,args.right_dir,args.observe)):
            rows=[json.loads(line) for line in path.read_bytes().splitlines()]
            need((directory/'reference.bin').read_bytes()==args.reference.read_bytes(),'PRIVATE_REFERENCE_COPY')
            files={name:(directory/(name+'.u32le')).read_bytes() for name in SECTIONS}
            archives=sorted(directory.glob('observe*.m1root'))
            need(len(archives)==int(observe),'PRIVATE_OBSERVATION_FILE')
            arms.append(private_arm(rows,files,ref,observe,archives[0].read_bytes() if archives else None))
        result=dict(compare_private(*arms),armRawSHA=[a['rawSHA'] for a in arms])
    print(json.dumps(result,indent=2))
    return int(result.get('state') in ('WORD_DIFF','WORD_LENGTH_DIFF'))


if __name__=='__main__':
    sys.exit(main())
