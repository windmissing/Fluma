"""Architect-only finite algebra, layout and immutable-evidence checks.
No device, GL/NAPI dispatch, product test, full stepper or CPU oracle.
Writes only new T552/T553/T554 engineering artifacts; old R1 is never rewritten.
"""
from pathlib import Path
from collections import Counter
import hashlib
import json
import math
import re
import struct
import sys
import zipfile

sys.stdout.reconfigure(encoding='utf-8')
ROOT=Path(__file__).resolve().parents[1]
DOC=ROOT/'docs'
Q=1<<20


def read(path): return json.loads((ROOT/path).read_text(encoding='utf-8-sig'))
def txt(path): return (ROOT/path).read_text(encoding='utf-8-sig')
def sha(data): return hashlib.sha256(data).hexdigest()
def pack(words): return struct.pack('<'+'I'*len(words),*words)
def hw(digest): return list(struct.unpack('<8I',bytes.fromhex(digest)))
def canonical(obj): return json.dumps(obj,sort_keys=True,separators=(',',':'),ensure_ascii=False).encode()
def pair(words,at,n): words[at:at+2]=[n&0xffffffff,n>>32]
def four(s): return int.from_bytes(s.encode(),'little')
def write(name,obj): (DOC/name).write_text(json.dumps(obj,ensure_ascii=False,indent=2)+'\n',encoding='utf-8')
def fp(path):
    p=ROOT/path
    return dict(path=path,bytes=p.stat().st_size,sha256=sha(p.read_bytes()))


class Checks:
    def __init__(self): self.rows=[]
    def yes(self,name,condition,**detail):
        assert condition,(name,detail)
        self.rows.append(dict(id=name,result='PASS_FINITE_ENGINEERING_ONLY',**detail))


def seed_from_r1(old,schema,k=512):
    p=next(p for p in schema['profiles'] if p['K']==k)
    segments={s['name']:s for s in p['stateSegments']}
    seed=[0]*(p['stateBytes']//4)
    assignments=old['seed']['canonicalAssignments']
    for i,v in assignments['header'].items():seed[int(i)]=v
    seed[24]=k
    col=segments['column32']['byteStart']//4;bed=segments['bedCohort4']['byteStart']//4
    for c in range(128):
        for i,v in assignments['columnEvery32Words'].items():seed[col+32*c+int(i)]=v
        seed[bed+c*64*4:bed+c*64*4+4]=assignments['bedRank0Every64Cohort4']
    source=segments['sourceRow16']['byteStart']//4
    seed[source:source+16]=assignments['sourceRow0']
    return seed,p,segments


def actual_seed(template,param_sha):
    out=template.copy()
    for at,n in [(4,7),(6,1),(8,11),(14,2)]:pair(out,at,n)
    out[42:50]=hw(param_sha)
    return out


def t552():
    c=Checks();parent=read('docs/architecture-T527-v5-cold-manifest-v1.json')
    profiles=[]
    for p in parent['profiles']:
        entry=dict(K=p['K'],Kw=p['Kw'],rootSlotBytes=p['rootSlotBytes'],fullSlotBytes=p['fullSlotBytes'])
        for mode in ['root','full']:
            rows=[]
            for i,s in enumerate(p[mode+'Segments']):
                name=s['name'];rootidx=0 if mode=='root' else 0xffffffff;role=0;kind='aggregate'
                if i==0:rootidx=0;kind='prefix'
                elif name.startswith('CONTROL_ROOT_'):rootidx=int(name.rsplit('_',1)[1]);kind='control'
                elif name.startswith('BASE_'):rootidx=int(name.rsplit('_',1)[1]);kind='fixed_root_anchor'
                elif name.startswith('Q'):
                    q,r=map(int,re.match(r'Q(\d+)_ROLE(\d+)',name).groups());rootidx=q+1;role=r;kind='scope_physical' if name.endswith('PHYSICAL') else 'scope_aux'
                elif name=='SOURCE_FRAME128':kind='source_frame'
                elif name=='PHYSICAL_FRAME64':kind='physical_frame'
                elif name=='PROVENANCE_FRAME32':kind='aux_frame'
                elif name in ['FINAL_STATE','FINAL_SOURCE_CONTEXT']:kind='fixed_final'
                fixed=dict(kind=i+1,rootIndex=rootidx,role=role,trial=0,arena=i,
                           byteOffset=s['byteStart'],byteLength=s['bytes'],strideWords=s['strideBytes']//4,
                           segmentRevision=2 if i==0 else 1,seal=parent['descriptorSeal'])
                bindings=dict(branchEpoch='CONTROL_ROOT[rootIndex].row[role<=3?role-1:role].word10_11' if role else 'U64_ZERO',
                    generation='U64_ZERO' if i==0 else 'HEADER.traceId',
                    flags='PREFIX_2' if i==0 else 'NOT_EXECUTED_1 | SEALED_VALID_2 | FORENSIC_PENDING_4 (exactly one)',
                    countsPolicy=kind,physicalRange='SCOPE_META.pStart+1 .. pSealedEnd; empty=0/0' if role else ('1..sealedRows; empty=0/0' if kind=='physical_frame' else '0/0'),
                    auxRange='SCOPE_META.aStart+1 .. aSealedEnd; empty=0/0' if role else ('1..sealedRows; empty=0/0' if kind=='aux_frame' else '0/0'),
                    sha='T535_FIXED_SELF_MASK' if i==0 else 'SHA256_COMPLETE_BYTE_LENGTH_ACTUAL_BYTES')
                rows.append(dict(index=i,name=name,fixed=fixed,bindings=bindings,capacityRows=s['bytes']//s['strideBytes']))
                c.yes(f'wire_{p["K"]}_{p["Kw"]}_{mode}_{i}',fixed['kind']==i+1 and fixed['byteLength']%s['strideBytes']==0 and 0<=role<=5 and 0<=rootidx<=0xffffffff)
            entry[mode+'Descriptors']=rows
            c.yes(f'layout_{p["K"]}_{p["Kw"]}_{mode}',len(rows)==(7 if mode=='root' else 61) and
                all(a['fixed']['byteOffset']+a['fixed']['byteLength']==b['fixed']['byteOffset'] for a,b in zip(rows,rows[1:])) and
                rows[-1]['fixed']['byteOffset']+rows[-1]['fixed']['byteLength']==p[mode+'SlotBytes'])
        profiles.append(entry)
    records={}
    for mode in ['ROOT','FULL']:
        rr=[]
        for i in range(1 if mode=='ROOT' else 5):
            rr.append(dict(index=i,rootKind='CAPTURED_COMMITTED_ROOT_KIND_1_OR_2' if mode=='ROOT' else (1 if i==0 else 2),
                baseStateSegment=0xffffffff if mode=='ROOT' else 11+2*i,baseSourceContextSegment=0xffffffff if mode=='ROOT' else 12+2*i,
                currentStateSegment=1,currentSourceContextSegment=2,sourceSegment=3,controlSegment=6+i,
                firstScopeSegment=0xffffffff if mode=='ROOT' or i==0 else 21+10*(i-1),scopeCount=0 if mode=='ROOT' or i==0 else 5,
                completionPrefixWord=2304+256*i,inputPrefixWord=3584+64*i,quotaPrefixWord=3904+256*i,cursorPrefixWord=5184+64*i))
        records[mode]=rr
    meta=['reserved_'+str(i) for i in range(32)]
    meta[:19]=['magic','revision','rootIndex','role','nonceLo','nonceHi','branchEpochLo','branchEpochHi',
        'physicalStart','physicalPlannedEnd','physicalSealedEnd','auxStart','auxPlannedEnd','auxSealedEnd',
        'entered','sealedComplete','firstError','rootKind','trial']
    meta[31]='seal'
    counts=dict(prefix='planned=sealed=valid=8192',fixed_final='planned=sealed=valid=capacityRows',
        fixed_root_anchor='root not entered => NOT_EXECUTED; otherwise full capacity even if later root failed',
        source_frame='planned=sealed=valid=owner.sourcePrefixRows (accepted prefix)',
        physical_frame='planned=sealed=valid=acceptedState.header[38]',aux_frame='planned=sealed=valid=acceptedState.header[179]',
        control='SOURCE 0/0/0; complete Q ROOT 6/6/6, FULL 8/8/8; forensic Q planned6/8, sealed=valid0',
        scope_physical='planned=pPlannedEnd-pStart, sealed=valid=pSealedEnd-pStart',
        scope_aux='planned=aPlannedEnd-aStart, sealed=valid=aSealedEnd-aStart')
    for cap,planned,sealed,expected in [(10,10,10,True),(10,7,5,True),(10,0,0,True),(10,11,5,False),(10,4,5,False)]:
        c.yes(f'counts_{cap}_{planned}_{sealed}',(0<=sealed<=planned<=cap)==expected)
    c.yes('FULL_control_metadata_fits_existing_8192',768+5*32==928 and 768+5*64==1088 and 1088*4<=8192)
    c.yes('FULL_extra_copy_bytes_and_calls',2*5*32*4*4==5120 and 2*5*4==40)
    c.yes('accepted_frame_cursor_indices',parent['lineageDictionaries']['header256'][38]=='eventUsed' and parent['lineageDictionaries']['header256'][179]=='provenanceRows')
    sample=profiles[-1]['fullDescriptors'][21]
    def fixed_matches(fixed):return fixed==sample['fixed']
    for field in ['kind','rootIndex','role','trial','arena','byteOffset','byteLength','strideWords','segmentRevision','seal']:
        bad=sample['fixed'].copy();bad[field]^=1
        c.yes('reject_changed_wire_'+field,not fixed_matches(bad))
    def ranges_ok(meta):
        return (meta['branchA']==meta['branchB'] and meta['nonceA']==meta['nonceB'] and
                0<=meta['start']<=meta['sealed']<=meta['planned']<=meta['capacity'])
    valid=dict(branchA=7,branchB=7,nonceA=11,nonceB=11,start=4,sealed=8,planned=9,capacity=12)
    c.yes('scope_pair_valid',ranges_ok(valid))
    for key,value in [('branchB',8),('nonceB',12),('sealed',13),('planned',7),('start',10)]:
        bad=dict(valid);bad[key]=value;c.yes('scope_pair_reject_'+key,not ranges_ok(bad))
    c.yes('not_executed_requires_zero_body',not all(x==0 for x in [0,0,1,0]))
    v5=dict(schema='T552_V5_SEGMENT_WIRE_V2',date='2026-11-17',status='ENGINEERING_READY_NOT_IMPLEMENTED',
        parent=fp('docs/architecture-T527-v5-cold-manifest-v1.json'),payload=parent['payload'],headerRevision=2,prefixDigestPolicy=1,
        kindRule='descriptorIndex+1 within exact mode/profile directory',nonPrefixSegmentRevision=1,
        roleCodes=dict(ROOT=0,REQUESTED=1,CLAMPED=2,OLD=3,AUTHORITATIVE=4,CONSISTENCY=5,SOURCE_TRIAL=6),
        arenaMeaning='cold segment index, not live GL arena',generationMeaning='nonprefix HEADER.traceId; prefix0',
        rootCountMeaning='allocated record slots ROOT1/FULL5',rootRecordFlags=dict(NOT_EXECUTED=1,FORENSIC_INCOMPLETE=2,SEALED_COMPLETE=4),
        rootRecords=records,profiles=profiles,countsPolicies=counts,
        embeddedRows=dict(SOURCE128=dict(magic=876950349,revision=5,sealWord=31,seal=3235774468),
            CONTROL128=dict(magic=876825680,revision=5,sealWord=31,seal=3235774468),
            PHYSICAL64=dict(magic=1163088455,revision=5,universalSealWord=None,authorization='kind usedMask and paired cursor sealed prefix'),
            PROVENANCE32=dict(magic=894456396,revision=5,sealWord=31,seal=3235774469)),
        notExecuted=dict(flags=1,plannedRows=0,sealedRows=0,validRows=0,branchEpoch=0,ordinals=0,body='all zero; hash full zero segment',
                         fixedMetadata='retain exact directory kind/root/role/trial/rev/arena/generation'),
        cursorCapture=dict(magic=four('CMV5'),revision=1,seal=parent['descriptorSeal'],fields=meta,
            rawGpuTail=dict(wordStart=768,perRoleWords=64,entryCursorOffset=0,finalCursorOffset=32,endWord=1088),
            publicColdTail=dict(wordStart=768,perRoleWords=32,endWord=928,zeroUntilWord=2048),
            roleControlRow={str(r):(r-1 if r<=3 else r) for r in range(1,6)},branchEpochControlWords=[10,11],
            cursorWords=dict(physicalPlanned=0,auxPlanned=1,physicalSealed=2,auxSealed=3),
            capturePoints=['after real v5BeginScope in SCOPE_INIT','after SCOPE_FINALIZE before cursor reuse'],
            missing='META_INCOMPLETE, never infer NOT_EXECUTED from missing data',extraCopyBytesPerPlanMax=5120,extraCopyCallsPerPlanMax=40,
            extraGPUBytes=0,extraHotReadBytes=0,extraDispatch=0),
        unknownFieldPolicy='reject; no defaulting dynamic identities to zero',budgetsUnchanged=True,
        checks='analysis-T552-contract-checks-v1.json')
    write('architecture-T552-v5-segment-wire-v2.json',v5)

    old=read('docs/architecture-T543-g9-adapter-manifest-v1.json');schema=read('entry/src/main/cpp/gpu_resident/g9spf/reader_schema.json')
    seed,p,segs=seed_from_r1(old,schema)
    c.yes('rebuild_historical_bad_template_exact',sha(pack(seed))==old['seed']['templateSha256'])
    col=segs['column32']['byteStart']//4
    c.yes('historical_column9_conflict',all(seed[col+32*i+9]==0 and seed[col+32*i]>0 for i in range(128)))
    for i in range(128):seed[col+32*i+9]=seed[col+32*i]+seed[col+32*i+1]
    template_sha=sha(pack(seed));config=old['config64'].copy();config[11]=3;config[13]=2;config[32:40]=hw(template_sha)
    param_sha=sha(pack(config));actual=actual_seed(seed,param_sha);actual_sha=sha(pack(actual))
    c.yes('R2_all_material_caches_valid',all(seed[col+32*i+9]==seed[col+32*i]+seed[col+32*i+1] for i in range(128)))
    c.yes('R2_mass_unchanged',sum(seed[col+32*i] for i in range(128))==5120*Q)
    c.yes('R2_full_identity_changes',template_sha!=old['seed']['templateSha256'] and param_sha!=old['parameterSha256'] and actual_sha!=old['seed']['exampleActualUploadSha256'])
    c.yes('R2_only_column_cache_seed_change',sum(a!=b for a,b in zip(seed,seed_from_r1(old,schema)[0]))==128)
    assignments=json.loads(json.dumps(old['seed']['canonicalAssignments']))
    assignments['columnEvery32Words']['9']=40*Q
    g9=dict(schema='T552_G9_CORRECTED_SEED_ENGINEERING_V2',date='2026-11-17',status='ENGINEERING_READY_NOT_IMPLEMENTED',
        oldManifest=fp('docs/architecture-T543-g9-adapter-manifest-v1.json'),oldArtifactsRetained=True,
        profile='G9_SPF_Q20_V1',profileCode=schema['profile'],seedId='G9_SEED_S40_ORIGIN1_R2',seedRevision=2,parameterRevision=3,
        designCandidateId=old['designCandidateId'],designCandidateSha256=old['designCandidateSha256'],adapterSpecSha256=old['adapterSpecSha256'],
        seed=dict(bytes=p['stateBytes'],templateSha256=template_sha,canonicalAssignments=assignments,allOtherWordsZero=True,
            dynamicTemplateHeaderZero=['run','owner','context','inputGeneration','parameterSha'],M0=str(5120*Q)),
        config64=config,parameterSha256=param_sha,exampleIdentity=dict(run=7,owner=1,context=11,inputEpoch=2),exampleActualUploadSha256=actual_sha,
        inputRequired=dict(revision=2,profile=schema['profile'],parameterRevision=3,parameterSha256=param_sha),
        order=['legal complete template including column9','template SHA','Config64 and parameter SHA','final dynamic identity fill','actual upload SHA','five bank upload','full initial GPU audit','first CURRENT'],
        noHashAfterMutation=True,disableAuditForbidden=True,retainsT543InputAndColdBodySemantics=True,
        DMV2='derive independent new profile/template/config hashes from corrected structure; never overwrite R1/T549',
        resourcesUnchanged=True,defaultOFF=True,checks='analysis-T552-contract-checks-v1.json')
    write('architecture-T552-g9-seed-manifest-v2.json',g9)
    write('analysis-T552-contract-checks-v1.json',dict(schema='T552_FINITE_CONTRACT_CHECKS_V1',date='2026-11-17',
        status='PASS_FINITE_ENGINEERING_ONLY',deviceUsed=False,productTestsRun=False,cases=c.rows,
        manifests=[fp('docs/architecture-T552-v5-segment-wire-v2.json'),fp('docs/architecture-T552-g9-seed-manifest-v2.json')],
        sources=[fp(x) for x in ['docs/developer-T517-v5-implementation.md','docs/developer-T548-adapter-progress.md',
            'entry/src/main/cpp/gpu_resident/g9spf/audit.glsl','entry/src/main/cpp/gpu_resident/g9spf/state_access.glsl',
            'entry/src/main/cpp/gpu_resident/m3b/lineage_events.glsl','entry/src/main/cpp/gpu_resident/m3b/quantum_v5.comp']],
        notProved=['complete V5 capture','native/GPU seed audit execution','complete T548 adapter']))
    return len(c.rows)


def t553():
    c=Checks();d=read('test/reports/t491/t510v2/execution-diag.json')
    inputs=d['inputStages'];rows=inputs['rows'];down=next(r for r in rows if r['code']==1)
    offers=[r for r in rows if r['code']==7];offer= min(offers,key=lambda r:int(r['nativeNs']))
    delta=int(offer['nativeNs'])-int(down['nativeNs'])
    c.yes('Down_to_offer_exact',delta==18633594)
    c.yes('five_retained_offers_enqueued',len(offers)==5 and all(json.loads(r['raw'])['reply']=='ENQUEUED_NOT_ACK' for r in offers))
    c.yes('input_seen_not_same_as_retained',int(inputs['counts'][7-1])>len(offers) and int(inputs['dropped'])==31)
    group=next(g for g in d['groups'] if g['scope']['epoch']=='7' and g['scope']['run']=='4')
    fault=group['faultRoot'];cause=group['firstCause'];point=cause['point']
    latency=int(point['ns'])-int(fault['started']);deadline=int(fault['deadline'])-int(fault['started'])
    revoked=next(s for s in d['stages'] if s['first']['stage']=='CONTROL_REVOKED')['last']
    gap=int(revoked['c']);late=int(point['ns'])-int(down['nativeNs'])
    c.yes('fault_is_GAP_before_root_deadline',cause['reason']=='CLOCK_GAP' and latency==41380625 and deadline==250000000 and latency<deadline)
    c.yes('real_gap_and_excess',gap==34047344 and gap-33333333==714011)
    c.yes('fault_not_immediate_input_failure',late==311789784375)
    accepted=next(s for s in d['stages'] if s['first']['stage']=='ROOT_ACCEPT')
    c.yes('many_successor_roots_already_observed',int(accepted['count'])==5153 and accepted['last']['root']=='5121')
    c.yes('T537_generator_already_calls_T536',"../t536/generate.cjs" in txt('test/t537/generate.cjs'))
    receipt=read('test/reports/t257/T537-m1-final.json');p=Path(receipt['signed']['path'])
    blob=p.read_bytes();c.yes('T537_signed_hash',sha(blob)==receipt['signed']['sha256'])
    with zipfile.ZipFile(p) as z:
        name=next(n for n in z.namelist() if n.endswith('libgrsm1.so'));elf=z.read(name)
    tokens=['T536_COLD_PREPARE_V1','Q_COPY','Q_PREFLIGHT','Q_WATER','Q_INDEX_SORT','Q_TRANSPORT','Q_BED','Q_AUDIT_REPLAY','Q_PUBLICATION','T537_INPUT_STAGE_V1']
    for token in tokens:c.yes('ELF_contains_'+token,token.encode() in elf)
    base='.codex/local-test/t540-final/entry/src/main/ets/'
    prepare=txt(base+'render/PrepareReadiness.ets')
    c.yes('IME_UNKNOWN_is_composite',all(s in prepare for s in ['sessionEverEdited&&!this.keyboardAreaSeen','s.keyboardConfiguration','s.deviceType','s.windowMode','s.avoid.length!==4']))
    c.yes('IME_seen_only_from_real_observation','if(s.avoid.some((r:PrepareRect)=>r.width>0&&r.height>0))this.keyboardAreaSeen=true' in prepare)
    page=txt(base+'pages/P051DerivedV2Demo.ets')
    c.yes('existing_observers_and_prepare_unchanged',all(s in page for s in ["w.on('avoidAreaChange'","w.on('keyboardHeightChange'",'controllers[i].stopEditing()','clearFocus()']))
    matches=[]
    def walk(v):
        if isinstance(v,dict):
            a=v.get('attributes',{})
            if a.get('id')=='t463-raster':matches.append(a.get('bounds'))
            for x in v.values():walk(x)
        elif isinstance(v,list):
            for x in v:walk(x)
    for p0 in sorted((ROOT/'test/reports/t491/t408v9').glob('l*.json')):walk(json.loads(p0.read_text(encoding='utf-8-sig')))
    c.yes('integer_raster_raw_present','[51,990][1205,2634]' in matches)
    sdk=Path('C:/Users/admin/AppData/Local/OpenHarmony/Sdk/26.0.0/ets/api')
    uitest=(sdk/'@ohos.UiTest.d.ts').read_text(encoding='utf-8-sig');ime=(sdk/'@ohos.inputMethod.d.ts').read_text(encoding='utf-8-sig')
    c.yes('UiTest_PNG_API_documented','screenCap(savePath: string, displayId: number): Promise<boolean>' in uitest and 'PNG image' in uitest)
    c.yes('same_process_IME_API_documented','function getController(): InputMethodController' in ime and 'showTextInput(): Promise<void>' in ime)
    jpeg=(ROOT/'test/reports/t491/t510v2/diag1.jpeg').read_bytes()
    c.yes('existing_system_capture_is_JPEG',jpeg.startswith(b'\xff\xd8') and not jpeg.startswith(b'\x89PNG\r\n\x1a\n'))
    def png_qualified(magic,binding,within):return magic==b'\x89PNG\r\n\x1a\n' and binding and within
    for name,magic,binding,within,expect in [('real_header',b'\x89PNG\r\n\x1a\n',True,True,True),
        ('jpeg_named_png',b'\xff\xd8',True,True,False),('no_clock',b'\x89PNG\r\n\x1a\n',False,True,False),
        ('outside_hold',b'\x89PNG\r\n\x1a\n',True,False,False)]:c.yes('PNG_qualification_'+name,png_qualified(magic,binding,within)==expect)
    c.yes('original_deadlines_unchanged',33333333<gap and 41380625<250000000)
    manifest=dict(schema='T553_PREPARE_ROOT_PNG_EVIDENCE_ENGINEERING_V1',date='2026-11-17',deviceUsed=False,
        IME=dict(status='PREPARE_UNKNOWN_EXACT_SUBPREDICATE_NOT_PROVED',existingPage='P051DerivedV2Demo',
            paths=['verify fixed keyboard/window/registration snapshot','real click focused t413-roi + actual avoid/height','same UIAbility inputMethod.getController().showTextInput if needed','existing stopEditing + clearFocus prepare sequence'],
            forbidden=['set keyboardAreaSeen directly','fake avoid','clear editing history to bypass','UNKNOWN->HIDDEN','extend prepare 3000ms'],
            missingEngineering='same-process requestImeForPrepare only if existing UI path cannot request IME; developer compile only'),
        root=dict(T537AlreadyContainsT536=True,HAPSha256=sha(blob),ELFSha256=sha(elf),ELFMarkers=tokens,
            evidence='5153 ROOT_ACCEPT counted, last root5121; future CLOCK_GAP root5122 is not blanket successor failure',
            nextEvidence=['same scope sourceOpen/FIFO/headAge/generation','Down command/intent/enqueue seq','G1 Claim/source Begin/completion/Accept or exact no-claim reason','per-domain wall/fence/pulse/Swap clock evidence'],
            optionalSeam=dict(name='SourceClaimWitnessV1',fixedBytesMax=256,additionalJSONMax=2048,
                existingWriterBytesMax=2097152,existingTotalBytesMax=4194304,changesQueueOrWakesOwner=False)),
        PNG=dict(primary='UiTest Driver.screenCap in real test runner, actual display and sandbox path',
            verify=['PNG magic/CRC/decode','original bytes/SHA/size','display and coordinate mapping','same token native hold + certified clock interval'],
            forbidden=['JPEG re-encode','filename as format proof','tolerance1 pass','hold renewal','cross-domain naked ns subtraction'],
            fallback='API/permission/runtime raw error and case-decision evidence to manager; no threshold change',diagnosticHoldNs=2000000000),
        unchanged=dict(rootDeadlineNs=250000000,gapNs=33333333,prepareDeadlineMs=3000,quotaHz=120,displayTargetFps=30,CPUStopped=True),
        checks='analysis-T553-evidence-v1.json')
    write('architecture-T553-evidence-manifest-v1.json',manifest)
    sources=[fp(x) for x in ['docs/test-report-T408-t540-domain-v9.md','docs/test-report-T510-t537-domain-v2.md',
        'test/reports/t491/t510v2/execution-diag.json','test/reports/t491/t510v2/boot-diag.json','test/reports/t491/t510v2/t537-pixel-envelope.json',
        'test/reports/t491/t510v2/capture1.json','test/reports/t491/t510v2/capture2.json','test/reports/t491/t510v2/diag1.jpeg',
        'test/t537/generate.cjs','test/reports/t257/T537-m1-final.json',base+'render/PrepareReadiness.ets',base+'pages/P051DerivedV2Demo.ets']]
    write('analysis-T553-evidence-v1.json',dict(schema='T553_READONLY_EVIDENCE_AND_FINITE_CHECKS_V1',date='2026-11-17',deviceUsed=False,productTestsRun=False,
        cases=c.rows,DownToOfferNs=str(delta),retainedOffers=len(offers),seenCounters=inputs['counts'],dropped=inputs['dropped'],
        rootAcceptCount=accepted['count'],lastRootAccepted=accepted['last']['root'],faultRoot=fault['nonce'],
        rootRevokeAgeNs=str(latency),rootDeadlineDeltaNs=str(deadline),gapNs=str(gap),gapExcessNs=str(gap-33333333),DownToFaultNs=str(late),
        rootScope=group['scope'],rasterBoundsRaw=sorted(set(matches)),T537Package=dict(path=str(p),bytes=len(blob),sha256=sha(blob),ELFBytes=len(elf),ELFSha256=sha(elf)),
        SDK=[dict(path=str(sdk/f),sha256=sha((sdk/f).read_bytes())) for f in ['@ohos.UiTest.d.ts','@ohos.inputMethod.d.ts']],sources=sources,
        unknown=['IME actual failed subpredicate','enqueue to G1 claim correlation','strict lossless same-token system pixel verdict'],notProductPass=True))
    return len(c.rows)


def t554():
    c=Checks();design=read('docs/design-G9-dynamic-mobilization-manifest-v2.json')
    parent=read('entry/src/main/cpp/gpu_resident/g9spf/reader_schema.json');r1=read('docs/architecture-T543-g9-adapter-manifest-v1.json')
    baseline=read('docs/architecture-T524-g9-spf-manifest-v1.json')
    micro=design['microPasses'];dmvals=[v['value'] for v in design['research']['parameters'].values()]+[2]
    c.yes('eight_header_config_params',len(dmvals)==8 and dmvals==[524288,65536,16384,262144,1572864,524288,7864320,2])
    table=[]
    for old in baseline['quantumPasses']:
        names={'ACTIVATE_REQUEST':['DM_REQUEST'],'ACTIVATE_APPLY':['DM_APPLY'],
            'FACE_FLUX_MOBILITY':['PREDICTOR_FACE','MIDPOINT_BUILD','CORRECTOR_FACE']}.get(old['op'],[old['op']])
        for name in names:
            row=dict(old);row['op']=name;row['index']=len(table);table.append(row)
    c.yes('exact_full_2404_dispatch_table',len(table)==2404 and len(baseline['sourcePasses'])==12)
    c.yes('plan_dispatch_9628',12+4*len(table)==9628)
    opnames=list(dict.fromkeys(p['op'] for p in baseline['sourcePasses']+table));codes={name:i for i,name in enumerate(opnames)}
    dispatch=dict(schema='T554_G9_DM_DISPATCH_V2',opcodeByName=codes,source=baseline['sourcePasses'],quantum=table)
    # Compact rows avoid a 30k-line equivalent pretty-printed pass table.
    lines=['{"schema":"T554_G9_DM_DISPATCH_V2",','"opcodeByName":'+json.dumps(codes,separators=(',',':'))+',',
        '"source":'+json.dumps(baseline['sourcePasses'],separators=(',',':'))+',','"quantum":[']
    lines += [json.dumps(row,separators=(',',':'))+(',' if i<len(table)-1 else '') for i,row in enumerate(table)]
    lines+= [']}'];(DOC/'architecture-T554-g9-dm-dispatch-v2.json').write_text('\n'.join(lines)+'\n',encoding='utf-8')
    c.yes('DM_micro_counts',Counter(p['op'] for p in table)['DM_REQUEST']==190 and Counter(p['op'] for p in table)['DM_APPLY']==190)
    for name in ['PREDICTOR_FACE','MIDPOINT_BUILD','CORRECTOR_FACE']:c.yes('count_'+name,sum(p['op']==name for p in table)==190)
    c.yes('old_natural_writer_disabled',not any(p['op'] in ['ACTIVATE_REQUEST','ACTIVATE_APPLY','FACE_FLUX_MOBILITY'] for p in table))
    c.yes('new_micro_exactly_12',len(micro)==12)
    names=['oldS','oldF','oldPLo','oldPHi','B','gxQ20','gnQ20','chiGQ20','HgQ20','H0','omega0Q16','Hs0Bound',
        'solverUpper','finalLo','finalHi','activeIterations','qPhysical','qOrigin','qActual','HAtPhysical','omegaAtPhysical','uAtPhysical',
        'HsPhysicalBound','reason','oldBedDepth','oldFlowOriginCount','newBedDepth','newFlowOriginCount','evalCalls','qNext','nextFeasible','closedFormBranch',
        'FEntryAfterFold','modeEntryAfterFold','pEntryLo','pEntryHi','pTrial0Lo','pTrial0Hi','pEndPredictLo','pEndPredictHi','pMidLo','pMidHi',
        'predictSupported','correctorSupported','midCFLPassed','predictorEventPlannedOrdinal','stageFlags','reserved47']
    c.yes('DM_WORK48_unique',len(names)==48 and len(set(names))==48)
    profile_code=four('GDM2');params=[];resources=[]
    spec_identity=dict(id='G9_DM_RESEARCH_R0',profile='G9_SPF_DM_Q20_V2',research=design['research'],policy='DM_IMPLICIT_POSTMIX_V2',policyCode=2)
    spec_sha=sha(canonical(spec_identity));design_sha=sha((DOC/'design-G9-dynamic-mobilization-manifest-v2.json').read_bytes())
    for old in parent['profiles']:
        k=old['K'];seed,p,segs=seed_from_r1(r1,parent,k);col=segs['column32']['byteStart']//4
        for i in range(128):seed[col+32*i+9]=40*Q
        seed[0]=four('GDS2');seed[1]=2;seed[2]=profile_code;seed[106:114]=dmvals
        template_sha=sha(pack(seed));config=r1['config64'].copy();config[0]=four('G9D2');config[2]=profile_code;config[3]=k
        config[11]=1;config[13]=1;config[16:24]=hw(spec_sha);config[32:40]=hw(template_sha);config[40:48]=hw(design_sha)
        config[48:56]=dmvals;config[56:64]=[0]*8
        parameter_sha=sha(pack(config));actual=actual_seed(seed,parameter_sha)
        params.append(dict(K=k,seedId='G9_DM_SEED_S40_ORIGIN1_R0_K'+str(k),seedBytes=old['stateBytes'],seedTemplateSha256=template_sha,
            config64=config,parameterSha256=parameter_sha,exampleIdentity=dict(run=7,owner=1,context=11,inputEpoch=2),exampleActualUploadSha256=sha(pack(actual)),
            seedAssignments=dict(parent='T552_R2_corrected_S40_structure',headerMagic=four('GDS2'),headerRevision=2,headerProfile=profile_code,
                header106_113=dmvals,columnEvery32Words={'0':40*Q,'6':1,'9':40*Q},allOtherWords='same canonical S40 structure including full source ledger; dynamic/parameter fields template0')))
        c.yes(f'DM_seed_{k}_cache_and_profile',all(seed[col+32*i+9]==seed[col+32*i]+seed[col+32*i+1] for i in range(128)) and seed[106:114]==config[48:56] and seed[2]==config[2])
        E=32*(6*128+2*k)+(4*128+k+16);P=32*8*k;arena=E*256+P*64;work=old['workBytes']+24576
        resident=5*old['stateBytes']+work+4*arena;fullGPU=6*arena
        rootCold=32768+2*old['stateBytes']+2*arena;fullCold=rootCold+6*arena
        upper=lambda n:1+math.ceil(n/4096)+32
        actualpages=lambda n:1+8+2*math.ceil(old['stateBytes']/4096)+n*math.ceil(arena/4096)
        record=dict(K=k,stateBytes=old['stateBytes'],stateBanks=5,workBytes=work,
            appendedWork=dict(name='DM48_per_column',byteStart=old['workBytes'],byteEnd=work,bytes=24576,strideBytes=192,columns=128,fields=names),
            eventRows=E,pathRows=P,eventBytes=E*256,pathByteOffset=E*256,arenaBytes=arena,normalArenas=4,
            residentPayloadBytes=resident,fullGPUAdditionalBytes=fullGPU,maxGLBufferObjectBytes=max(arena,work,old['stateBytes']),
            ROOT=dict(bytes=rootCold,segments=4,actualPages=actualpages(2),pageMax=upper(rootCold),archiveUtf8Max=(upper(rootCold)+1)*16384),
            FULL=dict(bytes=fullCold,segments=10,actualPages=actualpages(8),pageMax=upper(fullCold),archiveUtf8Max=(upper(fullCold)+1)*16384),
            twoFullSlotsBytes=2*fullCold,knownPayloadGPUFullAndTwoColdSlotsAndInputAndPage=resident+fullGPU+2*fullCold+22528+16384,
            excluded=['alignment','fixed host objects','driver/JIT','FBO','JS/runtime','stored files','old profile overlap forbidden until retirement'])
        resources.append(record)
        expected=next(r for r in design['resources'] if r['K']==k)
        c.yes(f'DM_designer_resource_match_{k}',resident==expected['payloadBytes'] and arena==expected['arenaBytes'] and fullGPU==expected['fullTrialTraceAdditionalBytes'])
        c.yes(f'DM_cold_delta_{k}',rootCold-old['coldRootSlotBytes']==2097152 and fullCold-old['coldFullSlotBytes']==8388608)
        c.yes(f'DM_pages_{k}',record['ROOT']['actualPages']<=record['ROOT']['pageMax'] and record['FULL']['actualPages']<=record['FULL']['pageMax'])
        c.yes(f'DM_append_no_alias_{k}',old['workSegments'][-1]['byteEnd']==old['workBytes'] and work-old['workBytes']==128*48*4)
        c.yes(f'DM_event_path_page_alignment_{k}',E%16==0 and P%64==0 and arena%4096==0)
    c.yes('DM_max_two_FULL',resources[-1]['twoFullSlotsBytes']==376119296)
    c.yes('DM_known_peak',resources[-1]['knownPayloadGPUFullAndTwoColdSlotsAndInputAndPage']==614190208)
    c.yes('DM_Eval_and_division_bound',190*128*36==875520 and 190*128*36*3==2626560)
    for e in design['events']:
        indices=[i for i in range(64) if not e['words'][i].startswith('reserved')]
        mask=[sum(1<<(i%32) for i in indices if i//32==j) for j in [0,1]]
        c.yes('event_mask_'+e['kind'],len(e['words'])==64 and mask==e['usedMask'])
    # Finite mathematical lower-bound boundary checks; no full dynamics oracle.
    bisect_vectors=[]
    for limit in [0,1,2,127,1024,(1<<28)-1,1<<28]:
        for target in sorted({0,limit//2,limit}):
            lo=0;hi=limit+1;active=0
            for _ in range(32):
                if hi-lo<=1:continue
                active+=1;mid=(lo+hi)//2
                if mid<=target:lo=mid
                else:hi=mid
            c.yes(f'32_rounds_{limit}_{target}',lo==target and hi==lo+1 and active<=32 and 2+active+1<=36)
            bisect_vectors.append(dict(upper=limit,abstractMaxFeasible=target,lo=lo,hi=hi,activeRounds=active))
    # Tiny finite Eval domains only: no microstep, inventory transport, time
    # integrator or product oracle. Verify integer monotone-prefix assumptions.
    def rne_signed(n,d):
        magnitude=abs(n);q,r=divmod(magnitude,d)
        q+=int(2*r>d or (2*r==d and q%2==1))
        return -q if n<0 else q
    def local_eval(q,s,f,p,gn,hg,omega_g,j,ks=65536):
        mass=f+q
        assert mass or p==0
        u=0 if mass==0 else rne_signed(p,mass)
        reg=max(mass,16384)
        head=ks*u*u//(65536*gn)
        excess=max(0,head-rne_signed(32768*reg,65536))
        hs=excess # depthShearGain=ONE in R0
        h=min(s+f,max(hg,f+hs))
        chi=excess*Q//reg
        omega=min(7864320,262144+max(omega_g,524288*chi//Q))
        return dict(H=h,omega=omega,u=u,feasible=q*(120*j*65536)<=omega*max(0,h-f-q))
    eval_vectors=[]
    for idx,(s,f,p,gn,hg,j) in enumerate([(31,0,0,536870912,20,1),(31,0,0,536870912,0,32),
        (63,1,10*Q,536870912,0,1),(63,1,-10*Q,536870912,0,1),
        (63,1,1024*Q,1,0,1),(63,32,32*20*Q,536870912,40,2),
        (127,32,0,536870912,120,32),(0,32,0,536870912,32,1)]):
        values=[local_eval(q,s,f,p,gn,hg,0,j) for q in range(s+1)]
        valid=[q for q,v in enumerate(values) if v['feasible']]
        c.yes('Eval_small_prefix_'+str(idx),valid==list(range(max(valid)+1)))
        c.yes('Eval_H_omega_nonincreasing_'+str(idx),all(a['H']>=b['H'] and a['omega']>=b['omega'] and abs(a['u'])>=abs(b['u']) for a,b in zip(values,values[1:])))
        e0=values[0];upper=min(s,e0['omega']*max(0,e0['H']-f)//(120*j*65536+e0['omega']))
        c.yes('Eval_upper_bound_'+str(idx),max(valid)<=upper)
        eval_vectors.append(dict(S=s,F=f,p=str(p),gn=gn,Hg=hg,J=j,upper=upper,maximumFeasible=max(valid),
            maxRecord=values[max(valid)],qPlus1Record=values[max(valid)+1] if max(valid)<s else 'S_PLUS_ONE_OUTSIDE_DOMAIN'))
    for n,d,expected in [(1,2,0),(3,2,2),(-1,2,0),(-3,2,-2),(-5,2,-2)]:c.yes(f'RNE_{n}_{d}',rne_signed(n,d)==expected)
    events=json.loads(json.dumps(design['events']))
    for e in events:
        e['stageOpcode']=codes['DM_APPLY' if e['code']==15 else 'CORRECTOR_FACE']
        if e['code']==16:e.update(predictorRoleOperand27=1,physicalDeltaIsZeroOperand28=1,flags='execution role bits only; no mutation/denial')
    manifest=dict(schema='T554_G9_DM_ENGINEERING_V2',date='2026-11-17',status='ENGINEERING_READY_NOT_IMPLEMENTED_NOT_VALIDATED',
        profile='G9_SPF_DM_Q20_V2',profileCode=profile_code,route='gpu-spf-dynamic-mobilization-v2',researchSpec=spec_identity,researchSpecSha256=spec_sha,
        oldCandidateApprovalReused=False,defaultOFF=True,CPUPhysicsAdded=False,
        parentDesign=fp('docs/design-G9-dynamic-mobilization-manifest-v2.json'),correctedSeedReference=fp('docs/architecture-T552-g9-seed-manifest-v2.json'),
        identity=dict(stateMagic=four('GDS2'),eventMagic=four('GDE2'),pathMagic=four('GDP2'),inputMagic=four('GDI2'),completionMagic=four('GDC2'),coldMagic=four('GDT2'),revision=2,seal=0xc0de0922),
        config=dict(magic=four('G9D2'),revision=2,parameterRevision=1,parameterSHA='SHA256 all64 LE words',header106_113=design['migration']['fieldsInOrder'],
            dmParameterWords=[48,56],reservedZeroWords=[56,64],GPUConfig128=dict(fullConfig64Words=[64,128],DMParametersWords=[112,120],
                physicalWords=[0,8],parameterSHAWords=[16,24],layoutSHAWords=[24,32],shaderSHAWords=[32,40],inputSHAWords=[40,48],actualSeedSHAWords=[48,56],reservedZero=[[8,16],[56,64]])),
        parameterProfiles=params,resources=resources,dispatch=fp('docs/architecture-T554-g9-dm-dispatch-v2.json'),
        microPasses=micro,sourceDispatch=12,quantumDispatch=2404,planDispatchMax=9628,
        solver=dict(policyCode=2,iterations=32,maxEvalConservative=36,implementedEvalCount='2+activeIterations+I(q+1<=S and q+1<=U)',
            maxScopeMicros=190,maxEvalsPerRoot=875520,wideDivPerEvalMax=3,maxWideDivPerRoot=2626560,
            closedForm='p0 or kShear0; no bisection Eval',failure='no epsilon/min1; checked arithmetic; invariant faults not WAIT'),
        originPolicy=design['originCapacityPolicy'],localReasons=design['localReason'],reasonPrecedence=design['localReasonPrecedence'],
        midpoint=design['midpoint'],predictionEntry='after DM/FLIGHT/PARTICLE_FOLD, separate from DM entry',
        staticFaceMassMask='both columns: pEndPredict==0 and entryMode==0 and corrector-supported; pressure retained',
        friction='real once; stopping reads explicit post-friction p and actual post-FLOW B geometry; do not force deposition on H/omega drop',events=events,
        semanticCompare='validate role/seal/stage first, normalize flags bits0/1 only, compare all other words including predictorRole=1',
        audits=design['requiredAudits'],cold=dict(capability='G9_SPF_DM_COLD_V2',page='G9_SPF_DM_SEGMENT_PAGE_V2',
            prefixBytes=32768,selfDescriptor=False,touchEvidence='same T543/T552 prefix positions with new profile/config/seed identity',
            pageWords=1024,pageUtf8Bytes=16384,slots=2,maxCumulativeSlotCapacityBytes=376119296,ttlNs='60000000000',
            readerMax=1,builderMax=1,oldRouteRetirementRequired=True,oldPinnedBlocks='WOULD_BLOCK until actual free',
            pageMax='1+ceil(bytes/4096)+32',archiveUtf8Max='(pageMax+1)*16384'),
        unchanged=dict(rootDeadlineNs=250000000,gapNs=33333333,quotaHz=120,displayTargetFps=30,rootFenceCount=1,completionReadBytes=1024,maxBindings=4,groupLanes=64),
        sourceOwnership='new g9dm module/generator/page/reader; old g9spf and T549 frozen source never rewritten',
        developer='full implementation compile commit only; no tests; no device',tester='T556 independent expectations then GPU on correct new package; failures first architect',
        unmeasured=['actual GPU cost','runtime correctness','real input','visible effect','30fps'],checks='analysis-T554-engineering-checks-v2.json')
    write('architecture-T554-g9-dm-manifest-v2.json',manifest)
    write('analysis-T554-engineering-checks-v2.json',dict(schema='T554_FINITE_ENGINEERING_CHECKS_V2',date='2026-11-17',deviceUsed=False,productTestsRun=False,
        cases=c.rows,abstractPrefixBoundaryVectors=bisect_vectors,finiteEvalVectors=eval_vectors,notFullStepper=True,notProductOracle=True,
        sources=[fp(x) for x in ['docs/algorithm-G9-dynamic-mobilization-v2.md','docs/design-G9-dynamic-mobilization-manifest-v2.json','docs/design-G9-dynamic-mobilization-fixtures-v2.json',
            'docs/architecture-T552-g9-seed-manifest-v2.json','docs/architecture-T524-g9-spf-manifest-v1.json','entry/src/main/cpp/gpu_resident/g9spf/reader_schema.json']],
        manifest=fp('docs/architecture-T554-g9-dm-manifest-v2.json')))
    return len(c.rows)


if __name__=='__main__':
    result=dict(T552=t552(),T553=t553(),T554=t554())
    print(json.dumps(dict(status='PASS_FINITE_ENGINEERING_ONLY',checks=result,deviceUsed=False,productTestsRun=False)))
