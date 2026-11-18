"""Six architect cards: raw/source/identity audit, no product/device tests.
No old generators, tester scripts, builds, native programs or physics are run.
"""
from pathlib import Path
import hashlib,json,re,zipfile

ROOT=Path(__file__).resolve().parents[1]
D=ROOT/'docs'


def read(p):return (ROOT/p).read_text(encoding='utf-8-sig')
def obj(p):return json.loads(read(p))
def digest(b):return hashlib.sha256(b).hexdigest()
def receipt(p):
    b=(ROOT/p).read_bytes();return dict(path=p,bytes=len(b),sha256=digest(b))
def emit(name,data):(D/name).write_text(json.dumps(data,ensure_ascii=False,indent=2)+'\n',encoding='utf-8')
def nodes(p,ids):
    out=[]
    def walk(x):
        if isinstance(x,dict):
            a=x.get('attributes',{})
            if a.get('id') in ids:out.append({k:a.get(k) for k in ['id','text','enabled','clickable','bounds','visible']})
            for v in x.values():walk(v)
        elif isinstance(x,list):
            for v in x:walk(v)
    walk(obj(p));return out


common=dict(date='2026-11-18',role='architect',deviceUsed=False,productTestsRun=False,
            productPASS=False,status='FIRST_ATTRIBUTION_AND_ENGINEERING_CONTRACT_DELIVERED')
base='.codex/local-test/'
sources={
 'T581':['test/t550_words_contract_overlay.cjs','test/t529/bridge_overlay.cjs','test/t522/bridge_overlay.cjs',
    'entry/src/main/cpp/gpu_resident/napi_words_contract.h','entry/src/main/cpp/gpu_resident/napi_words_contract.cpp',
    base+'t562-g9-diagnostic/entry/src/main/cpp/gpu_resident/app_bridge.cpp',
    base+'t562-m3-diagnostic/entry/src/main/cpp/gpu_resident/app_bridge.cpp',
    base+'t562-g9-diagnostic/entry/src/main/ets/pages/GpuResidentG9.ets',
    base+'t562-m3-diagnostic/entry/src/main/ets/pages/GpuResidentM3.ets'],
 'T584':['entry/src/main/cpp/gpu_resident/m3b/transport.cpp','entry/src/main/cpp/gpu_resident/m3b/cold_capture.cpp',
    'entry/src/main/cpp/gpu_resident/m3b/cold_contract.h','entry/src/main/cpp/gpu_resident/m3b/cold_layout_generated.h',
    'entry/src/main/cpp/gpu_resident/m3b/controller.cpp','entry/src/main/cpp/gpu_resident/m3b/controller.h',
    'entry/src/main/cpp/gpu_resident/m3b/cold_trace.h','entry/src/main/cpp/gpu_resident/m3b/kernels.cmake'],
 'T585':['test/t537/overlay.py','entry/src/main/cpp/gpu_resident/input_diagnostic/source_claim_witness.h',
    base+'t575-witness-tree/entry/src/main/cpp/gpu_resident/app_bridge.cpp',
    base+'t575-witness-tree/entry/src/main/ets/pages/GpuSandWaterExperience.ets',
    base+'t575-witness-tree/entry/src/main/ets/physics/GpuM1DemoController.ets'],
 'T586':['test/t574/t574_prepare_v4_overlay.cjs','test/t426/PrepareReadinessV4.ets',
    base+'t574-prepare-tree/entry/src/main/ets/pages/P051DerivedV2Demo.ets',
    base+'t574-prepare-tree/entry/src/main/ets/render/PrepareReadiness.ets'],
 'T588':[base+'t562-m2-fix/entry/src/main/ets/pages/GpuSandWaterExperience.ets',
    'docs/architecture-T547-create-and-empty-export-triage-v1.md','docs/architecture-T521-m2-zero-ledger-triage-v1.md'],
 'T589':['test/t572/t572_touch_bridge.inc','test/t572/t572_touch_overlay.cjs',
    'entry/src/main/cpp/gpu_resident/g9spf/touch_adapter.h',
    base+'t572-adapter-tree/entry/src/main/cpp/gpu_resident/app_bridge.cpp',
    base+'t572-adapter-tree/entry/src/main/ets/pages/GpuResidentG9.ets']}
reports={'T581':['docs/test-report-T545-m3-entry-v3.md','docs/test-report-T542-g9-entry-v3.md'],
 'T584':['docs/architecture-T527-v5-cold-manifest-v1.json','docs/architecture-T552-v5-segment-wire-v2.json'],
 'T585':['docs/test-report-T510-witness-ledger-v4.md'],
 'T586':['docs/test-report-T408-ready-v12.md'],
 'T588':['docs/test-report-T461-empty-raw-v3.md'],
 'T589':['docs/architecture-T543-g9-candidate-touch-adapter-v1.md','docs/architecture-T552-g9-seed-manifest-v2.json']}
data={n:dict(common,schema=n+'_FIRST_ATTRIBUTION_V1',sources=[receipt(p) for p in ps],
             inputs=[receipt(p) for p in reports[n]]) for n,ps in sources.items()}

# Verify actual handed-off package bytes/ELF without installing or executing.
delivery=obj('test/reports/t579/delivery.json')
packages={}
for name in ['T579-m2fix','T579-m3diag','T579-g9diag']:
    r=delivery[name];p=Path(r['hap'])
    assert p.exists() and p.stat().st_size==r['hapBytes']
    assert digest(p.read_bytes())==r['hapSha256']
    with zipfile.ZipFile(p) as z:elf=digest(z.read(r['elfName']))
    assert elf==r['elfSha256']
    packages[name]=dict(hapSha=r['hapSha256'],elfSha=elf,sourceTree=r['handoff']['source'],bytesVerified=True)
for name in ['T579b-prepare','T579b-witness','T579b-adapter']:
    rr='test/reports/t257/'+name+'.json';r=obj(rr);p=Path(r['signed']['path'])
    assert p.exists() and digest(p.read_bytes())==r['signed']['sha256']
    with zipfile.ZipFile(p) as z:
        names=[n for n in z.namelist() if n.endswith('/libgrsm1.so')];assert len(names)==1
        elf=digest(z.read(names[0]))
    packages[name]=dict(hapSha=r['signed']['sha256'],elfSha=elf,unsignedPath=r['input']['path'],receipt=receipt(rr),bytesVerified=True)

g9=read(sources['T581'][5]);m3=read(sources['T581'][6])
assert 'return text(e,"G9_RESEARCH_CONFIG_REJECTED")' in g9
assert 'return text(e,"M3_CONFIG_REJECTED")' in m3
assert '{"g9ResearchConfig",nullptr,g9ResearchConfig' in g9
assert '{"m3Configure",nullptr,m3Configure' in m3
assert 'gpuInputDiagnosticsV1' in g9 and 'gpuInputDiagnosticsV1' in m3
data['T581'].update(packages={k:packages[k] for k in ['T579-m3diag','T579-g9diag']},
    actualBinding='generated app_bridge existing property functions, not an unbound alternate napi.inc',
    established=['safe UI prevents observed JSON parse crash','legacy bound rejection still bare token','native diagnostic registered but page instruction text is not automatic call/export'],
    unproved=['exact failed NAPI predicate','M3/G9 identical typedarray cause','root GPU deadline/physics failure'],
    replyContract='existing endpoint GPU_NAPI_ERROR_V1 object +route/api/boot/diagGeneration/attemptId, semantic header failures included',
    evidence=dict(fileUtf8Max=16384,UIWordsMax=16,nativeSnapshotUtf8Max=4096,
      attemptsRetained='first rejected + last; exact full identity, no last-attempt substitution',credit='existing2MiB writer/4MiB total'),
    handoff='T582 product wiring first; tester own existing signing/deploy/collection after product closure, not a developer test gate')

v5=read(sources['T584'][0]);capture=read(sources['T584'][1]);layout=read(sources['T584'][3])
assert 'return false; // PREFIX_DIGEST_POLICY_REQUIRED' in v5
assert 'no persistent resident base' in capture
assert 'slot%2?"PROVENANCE":"PHYSICAL"' in capture
assert 'cold::Digest(out+s.begin,s.bytes/4' in capture
expected=obj(reports['T584'][0])
counts=[len(p['fullSegments']) for p in expected['profiles']]
assert counts==[61]*4 and 'array<SegmentSpec,61>' in layout
selection=[f'Q{s//5}_ROLE{s%5+1}_'+('PROVENANCE' if s%2 else 'PHYSICAL') for s in range(20)]
expected_streams=[f'Q{q}_ROLE{r}_{t}' for q in range(4) for r in range(1,6) for t in ['PHYSICAL','PROVENANCE']]
assert len(set(expected_streams)-set(selection))==20
truth=[]
for executed in [False,True]:
    for anchor in [False,True]:
        truth.append(dict(rootExecuted=executed,anchorCaptured=anchor,
            status='NOT_EXECUTED' if not executed else ('SEALED_COMPLETE' if anchor else 'INCOMPLETE_MISSING_REQUIRED_ANCHOR')))
data['T584'].update(formalFullSegmentCounts=counts,rootAnchorTruth=truth,
    selectedStreamNames=selection,missingCounterpartNames=sorted(set(expected_streams)-set(selection)),
    findings=['executed root cannot call missing resident BASE not executed',
      'writer loops20 slots selecting onlyone ofeach physical/provenance pair',
      'four Q control slots omit SOURCE root slot and copy768words without complete metadata',
      'all segment capacities labelled sealed instead of observed paired cursor prefix',
      'old rev1/prefix digest policy and byte/word digest pointer conflict with V5_2/T535/T552',
      'transport stub and cold_capture definition coexist in static library source set; actual selected symbol needs compile-link receipt, no runtime proof inferred',
      'per-controller trace managers alone do not prove process-global cross-route2-slot credit'],
    resources=[{k:p[k] for k in ['K','Kw','stateBytes','sourceContextBytes','fullSlotBytes','fullGPUReservationBytes','sourceContextBaseAnchorCopyBytesMax']} for p in expected['profiles']],
    worstKnownConcurrentPayload=expected['worstKnownConcurrentPayload'],
    policy='keep COMPLETE false until required executed-root anchors, both paired streams, root records and current V5_2 digest all present',
    handoff='developer required, not tester to prove missing implementation; T571 prior developer delivery remains registered')

x=obj('test/reports/t491/t510d/gpu-snapshot-execution.json')
assert not x['sourceClaimWitness']['rows']
assert x['inputStages']['scope']!=x['scope']['control']
app=read(sources['T585'][2]);witness=read(sources['T585'][1])
assert 'std::lock_guard<std::mutex> l(mutex);claimWitness.onClaim' in app
assert 'std::array<Witness,8>' in witness and '(void)enqueueSeq;' in witness
assert app.index('claimWitness.onClaim')<app.index('auto c=run->Offer(command);lastRunOffer')
data['T585'].update(package=packages['T579b-witness'],raw=[receipt('test/reports/t491/t510d/'+n) for n in ['gpu-snapshot-execution.json','boot-touch.json']],
    currentScope=x['scope'],inputStageScope=x['inputStages']['scope'],inputStageCounts=x['inputStages']['counts'],
    witness=x['sourceClaimWitness'],
    established=['empty ledger field and fixed JSON shape observed','captured input-stage scope differs from final execution scope'],
    unproved=['swipe entered touch handler','same intent got typed enqueue','same enqueue got claim or terminal completion','1187 empty Q proves SOURCE progress'],
    sourceRisks=['nested same std::mutex at actual claim hook','pre-Offer CLAIMED with previous nonce reused as planId and guessed kind',
      'enqueueSeq argument ignored and oldest row selected across scope','no terminal completion hook','8x176B rows exceed original one256B witness payload and can exceed2048B JSON',
      'nothrow claimCopy dereferenced without null branch/accounting'],
    handoff='fix actual generator witness hook/identity/bounds; T593 collect sustained real Down over multiple pulses, independent of G9 T572')

ready=obj('test/reports/t491/t408vC/prepB-READY.json')['readiness']
page=read(sources['T586'][2]);v4=read(sources['T586'][1]);overlay=read(sources['T586'][0])
assert ready['state']=='READY' and ready['revision']=='T532_PREPARE_V3'
buttons={f:nodes('test/reports/t491/t408vC/'+f,{'t413-reserve','t413-status-text'}) for f in ['l2.json','l4.json','l6.json']}
assert all(any(n['id']=='t413-reserve' and n['enabled']=='false' and n['clickable']=='false' for n in a) for a in buttons.values())
assert ".enabled(this.prepareGate.state==='READY')" in page
assert 'this.evidence!==null' in page and "prepareCurrentWindow570():number{return 0;}" in page
assert page.count('prepareV4.begin(')==1 and 'prepareV4.sampleFrame(' not in page and 'prepareV4.checkForReserve(' not in page
assert 'if(nowMs-this.started>=3000)' in v4
data['T586'].update(package=packages['T579b-prepare'],raw=[receipt('test/reports/t491/t408vC/'+n) for n in ['prepB-PREPARING.json','prepB-READY.json','l2.json','l4.json','l6.json']],
    ready=dict(revision=ready['revision'],prepareId=ready['prepareId'],pageGeneration=ready['pageGeneration'],
      firstFrame=ready['permit']['firstFrame'],secondFrame=ready['permit']['secondFrame'],readyMonoMs=ready['permit']['readyMonoMs']),
    buttons=buttons,
    findings=['READY genuine under existing gate; button disabled/clickablefalse in actual layouts',
      'enabled reads nonreactive plain gate field, text uses separate reactive prepareText',
      'reserve preguard blocks evidence!=null, not evidence==null',
      '3000ms bounds PREPARING in active old gate, not a demonstrated READY expiry',
      'v4 side object begin before old begin uses zero owner helpers, inactive default and no frame/change/check/consume binding'],
    unproved=['three clicks entered handler','unique early return reason','v4 full semantics passed'],
    handoff='developer reactive affordance +one authoritative v4 integration and bounded reserve receipt; T592 precise hit/check/result/IO evidence')

npath='test/reports/t491/t461b/t468-create-1-1794963701327-native.json'
mpath='test/reports/t491/t461b/t468-create-1-1794963701327-manifest.json'
n=obj(npath);m=obj(mpath)
assert len((ROOT/npath).read_bytes())==4778 and digest((ROOT/npath).read_bytes())==m['native']['sha256']
assert not n['sealed'] and n['inCall'] and n['nativeEndNs']=='0' and n['current']['stage']=='LINK'
for raw in m['rawLogs']:
    p=ROOT/'test/reports/t491/t461b'/raw['name'];assert p.stat().st_size==0 and digest(p.read_bytes())==raw['sha256']
compile_event=next(e for e in n['events'] if e['stage']=='COMPILE')
compile_ns=int(compile_event['exitNs'])-int(compile_event['enterNs'])
data['T588'].update(package=packages['T579-m2fix'],raw=[receipt(npath),receipt(mpath)],
    currentCreate={k:n[k] for k in ['bootSerial','epoch','candidateRun','nativeBeginNs','nativeEndNs','sealed','inCall','inCallDurationNs','current','firstFailure']},
    compileObservedDurationNs=compile_ns,externalDeclaration=m['declaration'],
    logStatus=[dict(object=p['object'],compile=p['compileLog'],link=p['linkLog']) for p in n['programs']],
    findings=['actual SOURCE_V3 compile completed and LINK entered, open interval not completed result',
      'code0 with sealedfalse/end0 is not Create OK or physical error',
      'empty link file when queryfalse is not observed empty link log',
      'signed package/ELF receipts can be externally supplied now without rewriting native declarations',
      'V5 FULL T571 not an M2 Create prerequisite; offline T496 scope not same device attempt'],
    handoff='T591 pair original same-attempt snapshots with existing external signed-package/ELF/install/PID/context receipt; code change only on precise missing product observation')

touch=read(sources['T589'][0]);header=read(sources['T589'][2]);g9page=read(sources['T589'][4])
assert 'touchAdapter.AuthorizeClock(m)' in touch
assert 'std::array<uint32_t,64> words' in header and 'prefix+3200' in header
assert 'g9TouchBegin(' not in g9page and 'g9TouchEnd(' not in g9page
assert 'return text(e,"UP_READY_NOT_CONSUMED")' in touch
data['T589'].update(package=packages['T579b-adapter'],
    findings=['native touch endpoints exist but page has no Down/Move/Up calls',
      'mapping endpoint grants clock solely from mapping, no platform proof producer',
      'presented generation/commit fabricated from call count, current identity combined with prior token risks',
      'UP computes evidence only, no INPUT128 queue/Offer connection',
      'cold metadata endpoint not full record export; no consumed BASE promotion',
      'TouchEvidence256 actually64U32; writes base at3200 vs required3328 and overwrites field0 with kind',
      'Update/End lack same safe integer/nonnegative checks as Begin'],
    contract=dict(clock='independent trusted native platform/source/monotonic proof; page cannot authorize via token or timestamps',
      mapping='GL owner only after same-scope successful Swap; immutable lease commit/viewport/uBox and idempotent generation',
      touchEvidenceSlots=[dict(startWord=3072,words=256),dict(startWord=3328,words=256)],
      Config64StartWord=3584,SeedMeta64StartWord=3648,nativePayloadBudget=22528),
    handoff='full product integration/producer proof before T544 real-flick demo; safe INPUT_CLOCK_UNVERIFIED remains if actual platform proof unavailable')

for n,d in data.items():emit('analysis-'+n+'-source-evidence-v1.json',d)
emit('architecture-T581-T589-closure-matrix-v1.json',dict(common,
    tasks={n:dict(report='architecture-'+n+'-first-attribution-v1.md',evidence='analysis-'+n+'-source-evidence-v1.json') for n in data},
    notTask='T569 typo corrected to T589; no tester card state or tests changed',
    checks=['signed package and ELF bytes','raw file hashes','READY button actual attributes',
      'M2 open compile/link interval','V5 directory completeness and parity selection',
      'actual generator mutex/seq/endpoint/page connections','cold record word ranges'],
    limits=['static/source inference is not device reproduction','prior narrow PASS retained only within proved domain',
      'no product code/build/sign/deploy/test done by architect']))
print(json.dumps({'cards':list(data),'signedPackagesChecked':len(packages),'formalV5FullSegments':counts,
                  'compileNs':compile_ns,'READYButtonDisabled':True,'testsRun':False},ensure_ascii=False))
