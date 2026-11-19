"""Read-only saved-artifact/source engineering checks. --record writes docs only.
Never builds, deploys, launches a product, executes a shader or calls a device.
"""
from pathlib import Path
import json,hashlib,zipfile,re,sys
R=Path(__file__).resolve().parents[1]
def txt(p):return (R/p).read_text(encoding='utf-8-sig')
def get(p):return json.loads(txt(p))
def sha(b):return hashlib.sha256(b).hexdigest()
def fp(p):
 b=(R/p).read_bytes();return dict(path=p,bytes=len(b),sha256=sha(b))
def emit(p,v):(R/p).write_text(json.dumps(v,ensure_ascii=False,indent=2)+'\n',encoding='utf-8')
def cuts(p,needles):return dict(path=p,matches=[dict(line=i+1,text=s) for i,s in enumerate(txt(p).splitlines()) if any(n in s for n in needles)])
def texts(x):
 out=[]
 def visit(y):
  if isinstance(y,dict):
   s=y.get('attributes',{}).get('text','')
   if s and s not in out:out.append(s)
   for v in y.values():visit(v)
  elif isinstance(y,list):
   for v in y:visit(v)
 visit(x);return out
def packages(receipt,library):
 m=get(receipt);out={}
 for key in ['input','signed']:
  p=Path(m[key]['path']);b=p.read_bytes();assert sha(b)==m[key]['sha256']
  with zipfile.ZipFile(p) as z:
   out[key]=dict(path=str(p),bytes=len(b),sha256=sha(b),entries=[dict(path=n,bytes=len(z.read(n)),sha256=sha(z.read(n))) for n in [library,'ets/modules.abc']])
 assert out['input']['entries']==out['signed']['entries']
 return out
def common(n):return dict(task='T'+str(n),date='2026-11-19',role='architect',deviceUsed=False,productBuildsRun=False,productTestsRun=False,sourceReviewOnly=True)
def collect649():
 base='entry/src/main/cpp/gpu_resident/g13flow/';source=txt(base+'transport.cpp')
 assert source.index('InitStageEvent("CAPABILITY", "EXIT"')<source.index('if (firstGl_ || blocks < 4')<source.index('InitStageEvent("PROGRAM_BUILD", "ENTER"')
 after=get('test/reports/t491/t643a/after-configure.json');assert after['firstConfigFailure']=='CONFIGURE_ERROR_29' and after['firstFailureAttempt']=='232'
 assert after['lastProgress']=='CAPABILITY:EXIT' and after['handledTicket']=='0'
 files=[str(p.relative_to(R)).replace('\\','/') for p in (R/'test/reports/t491/t643a').iterdir() if p.is_file()]
 rawPages=[]
 for p in files:
  if p.endswith('.json'):
   m=get(p)
   if isinstance(m,dict) and 'base64' in m:rawPages.append(p)
 assert not rawPages
 receipt=packages('test/reports/t257/T643-g13.json','libs/arm64-v8a/libgrs_g13.so')
 dev=get('test/reports/t638/development.json');assert receipt['input']['sha256']==next(v for p,v in dev['build']['artifacts'].items() if p.endswith('.hap'))
 sourceAgreement=[]
 for p,digest in dev['sources'].items():
  built='.codex/local-test/t638-delivery/'+p
  sourceAgreement.append(dict(path=p,receiptSHA=digest,mainMatch=fp(p)['sha256']==digest,builtMatch=fp(built)['sha256']==digest))
 assert all(x['builtMatch'] for x in sourceAgreement)
 maps=[s for s in txt('test/reports/t491/t643a/maps-g13.txt').splitlines() if 'grs_g13' in s]
 assert len(maps)==5 and sum('57371' in s for s in maps)==4
 thresholds=[dict(field=n,required=v,producer=p) for n,v,p in [
  ('blocks','>=4','glGetIntegerv(GL_MAX_COMPUTE_SHADER_STORAGE_BLOCKS)'),('bindings','>=4','glGetIntegerv(GL_MAX_SHADER_STORAGE_BUFFER_BINDINGS)'),
  ('invocations','>=64','glGetIntegerv(GL_MAX_COMPUTE_WORK_GROUP_INVOCATIONS)'),('floatPrecision','>=23','glGetShaderPrecisionFormat(COMPUTE,HIGH_FLOAT)'),
  ('laneX','>=64','glGetIntegeri_v(GL_MAX_COMPUTE_WORK_GROUP_SIZE,0)'),('shared','>=4096','glGetIntegerv(GL_MAX_COMPUTE_SHARED_MEMORY_SIZE)'),
  ('maxBlock','>=profile.ww*4 bytes','glGetInteger64v(GL_MAX_SHADER_STORAGE_BLOCK_SIZE)'),('rawGL','==0 at existing read','existing glGetError result')]]
 contract=dict(schema='T649_EXISTING_EVIDENCE_CONTINUATION_V1',**common(649),attempt='232',missingRawPages=4,
  API=['g13Status() complete JSON, especially native.capability and lastCommand','g13InitDiagnostics(attemptId,page0..3) exact base64 pages'],
  capabilityThresholds=thresholds,capabilityExitMeans='query stage ended; threshold check is later, not qualified PASS',
  pageABI=dict(bytes=8192,count=4,total=32768,decode=[0,4096],stages=[4096,20480],logs=[20480,28672],control=[28672,32768]),
  sampleSeqMeaning='stage increment only, not complete payload revision; equal values alone do not prove all-page atomic snapshot',
  collector=dict(noNewConfigureWhileReading=True,noBusinessPulseOrActiveRequired=True,reuseSameLiveAttemptFirst=True,requireBootPidStartAndActualMappedModule=True,maxSnapshotRounds=3,maxStatusReads=4,utf8RecordMax=16384,directoryMax=1048576),
  nextRole='tester for precise existing fields; no new product stage/relaxed shader or precision threshold now',
  classification='UNRESOLVED_CAPABILITY_OR_LATER_STAGE; capability branch is first candidate, not runtime proven',
  downstream='T609/T610 full physics/effect remain not exercised; T643 limited evidence handoff is complete')
 inputs=['docs/test-report-T643-v1.md','test/reports/t638/development.json','test/reports/t257/T643-g13.json','docs/developer-T638-init-abi-v1.json']+files+[base+'transport.cpp',base+'controller.cpp',base+'bridge.cpp',base+'init_diagnostics.h']
 evidence=dict(schema='T649_SAVED_EVIDENCE_REVIEW_V1',**common(649),summary=after,actualFiles=[fp(p) for p in files],rawPageFiles=rawPages,
  packages=receipt,sourceAgreement=sourceAgreement,maps=maps,mappedBytesSHAOrBuildIdArchived=False,
  sourceCuts=[cuts(base+'transport.cpp',['CAPABILITY','if (firstGl_ || blocks','precision < 23','maxBlock <','PROGRAM_BUILD','return 29;']),
   cuts(base+'init_diagnostics.h',['++c.samples','void Ticket','void State','void Failure','void Handled','sampleSeq']),cuts(base+'bridge.cpp',['InitObservation().Handled','CONFIGURE_ERROR_'])],inputs=[fp(p) for p in inputs],
  limits=['rawBytes32768 is declared capacity, not saved binary','no exact failing capability inequality available','no per-stage232 identity/source or program log saved','maps is 4 file mappings plus anon BSS, not mapped byte digest'])
 return contract,evidence
def collect650():
 built='.codex/local-test/t596-t614-delivery/';bridge=built+'entry/src/main/cpp/gpu_resident/app_bridge.cpp';s=txt(bridge)
 a=s.index('if(m5Selected){',s.index('// Existing owner has retired old route/EGL'))
 create=s.index('auto next=std::make_shared<grs::m3b::ControllerV5>();',a)
 publish=s.index('m5Controller=next;phase=permission="M5_WAIT_PAGE_PULSE"',create)
 assert create<publish and s.index('auto result=next->Configure(',create)<publish and s.index('m5Scene.PrepareTargets(',create)<publish
 assert 'm5Controller->Pulse' not in s[create:publish]
 assert 'Semantic(words[15]==0,"PREFIX_IDENTITY_CONTRACT_REQUIRED"' in s
 pkg=packages('test/reports/t257/T594-v5.json','libs/arm64-v8a/libgrsm1.so');dev=get('test/reports/t596/development-t619.json')
 agreement=[]
 for p,h in dev['sourceSHA'].items():
  candidate=built+p;agreement.append(dict(path=p,mainMatch=fp(p)['sha256']==h,builtMatch=fp(candidate)['sha256']==h if (R/candidate).exists() else None))
 # tools/v5 reader is outside the isolated entry tree.
 page=txt(built+'entry/src/main/ets/pages/GpuResidentM5.ets');assert 'GpuInputAttempt.waitSession' in page and 'this.identity = identity; this.sourceGeneration = source; this.attachFrame();' in page
 archived=get('test/reports/t594/v5-cold-part.jsonl');rejected=json.loads(archived['rejectedRaw']);assert rejected['code']=='M5_NOT_READY'
 segments=[dict(name=n,byteStart=a,bytes=b,byteEnd=a+b) for n,a,b in [('control',0,1024),('attempt4x512',1024,2048),('stages32x256',3072,8192),('logs2x2048',11264,4096),('reservedZero',15360,1024)]]
 assert sum(x['bytes'] for x in segments)==16384 and segments[-1]['byteEnd']==16384
 stageFields=[dict(offset=8*i,bytes=8,name=n) for i,n in enumerate(['seq','nativeNs','requestTicket','configAttempt','diagGeneration','cancelGeneration','ownerTid','epoch','window','context'])]
 stageFields += [dict(offset=80,bytes=72,name='scope9U64')]+[dict(offset=152+4*i,bytes=4,name=n) for i,n in enumerate(['actualReturnS32','rawGLS32','rawEGLS32','apiReturnS32'])]
 stageFields += [dict(offset=168,bytes=32,name='phaseASCII'),dict(offset=200,bytes=8,name='edgeASCII'),dict(offset=208,bytes=32,name='sourceSHAOrPulse4U64'),dict(offset=240,bytes=4,name='knownFlags'),dict(offset=244,bytes=4,name='op'),dict(offset=248,bytes=4,name='programRole'),dict(offset=252,bytes=4,name='reservedZero')]
 assert sorted(i for f in stageFields for i in range(f['offset'],f['offset']+f['bytes']))==list(range(256))
 attemptFields=[dict(start=i,words=1,name=n) for i,n in enumerate(['magic','revision1','words128','ticketKind'])]
 for i,n in enumerate(['requestTicket','configAttempt','diagGeneration','cancelGeneration','epoch','window','context','ownerTid','enqueueNs','ownerBeginNs','terminalNs']):attemptFields.append(dict(start=4+2*i,words=2,name=n))
 for at,n,w in [(26,'scope9U64',18),(44,'Config32Original',32),(76,'configSHA',8),(84,'sourceManifestSHA',8),(92,'shaderSHA',8)]:attemptFields.append(dict(start=at,words=w,name=n))
 for i,n in enumerate(['controllerPresent','currentScope','active','lifecycle','actualReturnS32','firstFailureReturnS32','firstFailureStage','terminal']):attemptFields.append(dict(start=100+i,words=1,name=n))
 for i,n in enumerate(['firstPulseSeq','lastAcceptedPulseSeq','lastOwnerSeenPulseSeq','firstPulseReceiptNs','lastPulseReceiptNs']):attemptFields.append(dict(start=108+2*i,words=2,name=n))
 attemptFields.append(dict(start=118,words=10,name='reservedZero'))
 assert sorted(i for f in attemptFields for i in range(f['start'],f['start']+f['words']))==list(range(128))
 control=[dict(start=i,words=1,name=n) for i,n in enumerate(['magicM5ID','revision1','payloadBytes16384','pageBytes2048'])]
 for i,n in enumerate(['nativeBoot','pid','processStartNsOrUnknown','snapshotRevision','currentTicket','latestConfigAttempt','diagGeneration','stageTotal','stageLost','attemptTotal','attemptLost','truncatedBytes','lastMutationNs']):control.append(dict(start=4+2*i,words=2,name=n))
 for i,n in enumerate(['stageCount','stageCursor','attemptCount','attemptCursor','knownFlags','namespaceM5I1']):control.append(dict(start=30+i,words=1,name=n))
 for i,n in [(36,'sourceManifestSHA'),(44,'loadedModuleSHAOrUnknown'),(52,'shaderSHA')]:control.append(dict(start=i,words=8,name=n))
 for i,n in [(60,'log0Copied'),(61,'log0Reported'),(62,'log0Flags'),(63,'log0Role'),(66,'log1Copied'),(67,'log1Reported'),(68,'log1Flags'),(69,'log1Role')]:control.append(dict(start=i,words=1,name=n))
 for i,n in [(64,'log0Ticket'),(70,'log1Ticket')]:control.append(dict(start=i,words=2,name=n))
 control += [dict(start=72,words=8,name='log0SourceSHA'),dict(start=80,words=8,name='log1SourceSHA'),dict(start=88,words=168,name='reservedZero')]
 assert sorted(i for f in control for i in range(f['start'],f['start']+f['words']))==list(range(256))
 contract=dict(schema='T650_M5_INIT_OBSERVATION_V1',**common(650),payloadBytes=16384,segments=segments,stage256=stageFields,attempt128=attemptFields,control256=control,
  controlKnownFlags=dict(PROCESS_START_KNOWN=1,LOADED_MODULE_SHA_KNOWN=2,SOURCE_MANIFEST_KNOWN=4,SHADER_SHA_KNOWN=8,REVISION_EXHAUSTED=16),
  namespaceMagic=int.from_bytes(b'M5I1','little'),controlMagic=int.from_bytes(b'M5ID','little'),attemptMagic=int.from_bytes(b'M5AT','little'),
  encoding='LE32/LE64 low first; stage raw status unknown -1; SHA32 digest bytes; ASCII fixed arrays nul or truncation flag',
  API='m5InitDiagnostics(ticket:string,page:number):string; 8 pages x2048B raw/base64 <=4096B JSON; readonly no GL owner queue',
  lifecycleCodes={n:i for i,n in enumerate(['NONE','REQUEST_QUEUED','WAIT_WINDOW','WAIT_RETIRED','OWNER_CREATE','CONTROLLER_CONFIGURED','SCENE_READY','PUBLISHED_WAIT_PAGE_PULSE','INPUT_READY','PULSE_ACCEPTED','RUNNING','FAILED','CANCELLED'])},
  stageCodes={n:i for i,n in enumerate(['NONE','CONFIG_REQUEST','WINDOW_RETAIN','RETIRE_PREVIOUS','EGL_READY','CONTROLLER_CONFIGURE','PROGRAM_SOURCE','PROGRAM_INTERMEDIATE','PROGRAM_FINAL','SCENE_PROGRAM','SCENE_TARGETS','PUBLISH_CONTROLLER','SESSION_READ','FRAME_ATTACH','INPUT_OFFER','PULSE_DIRECT','OWNER_OBSERVE_PULSE','PLAN_ADVANCE','RETIRE_CONTROLLER'])},
  scopeOrder=['run','owner','context','inputEpoch','pageSession','permission','window','config','surface'],
  stages=['CONFIG_REQUEST','WINDOW_RETAIN','RETIRE_PREVIOUS','EGL_READY','CONTROLLER_CONFIGURE','PROGRAM_SOURCE','PROGRAM_INTERMEDIATE','PROGRAM_FINAL','SCENE_PROGRAM','SCENE_TARGETS','PUBLISH_CONTROLLER','SESSION_READ','FRAME_ATTACH','INPUT_OFFER','PULSE_DIRECT','OWNER_OBSERVE_PULSE','PLAN_ADVANCE','RETIRE_CONTROLLER'],
  producerRules=['controller published before wait-page-pulse','m5Pulse currently direct synchronous CPU control validation, not an owner queue','owner later reads real quota/Advance, no invented consume ACK',
   'config ticket independent of words-call attempt overwritten by later cold/session reads','publish requires same native cancel generation/epoch/window/config ticket','first error per attempt plus last progress; all mutations increment snapshot revision'],
  knownFlags=dict(RAW_GL=1,RAW_EGL=2,API_RETURN=4,SOURCE_SHA=8,ACTUAL_RETURN=16,SCOPE_KNOWN=32,PULSE_PAYLOAD=64),
  pulseStagePayload=dict(offsetBytes=208,words=8,orderU64=['pulseSeq','uiCallbackNs','nativeReceiptArgumentNs','ownerObservedAcceptedPulseSeq'],exclusiveWithSourceSHA=True),
  frontendEvidence='FRAME_ATTACH/client callback lives in bounded existing ArkTS receipt, never forged as native stage or new authority; readonly API has no write mode',
  programRole=dict(NONE=0,SOURCE=1,INTERMEDIATE=2,FINAL=3,SCENE=4),
  log2048=dict(headerBytes=64,rawBytes=1984,ticketU64Word=0,roleWord=2,copiedWord=3,reportedWord=4,flagsWord=5,sourceSHAWords=[6,14],reservedWords=[14,16]),
  budgets=dict(newGpuDispatch=0,newGpuReadback=0,newFences=0,newGlQueries=0,hotIO=False,normalGAPNs=33333333,physicalMathUnchanged=True),
  FULLGuard='align existing bridge word15 FULL admission with completed T619 identity/capacity contract; do not simply discard gate or expose unsupported COMPLETE',
  handoff='tester existing exact identity/session/status/input diagnostics now; developer minimal lifecycle observation and confirmed leftover FULL guard, no fake Ready; T594 retains original full verification')
 inputs=['docs/test-report-T594-v5-cold-v1.md','docs/architecture-T619-v1.md','test/reports/t596/development-t619.json','test/reports/t257/T594-v5.json','test/reports/t594/v5-page-state.json','test/reports/t594/v5-cold-part.jsonl',bridge,
  built+'entry/src/main/ets/pages/GpuResidentM5.ets',built+'entry/src/main/ets/physics/GpuInputAttempt.ets',built+'entry/src/main/cpp/gpu_resident/napi_words_contract.cpp','entry/src/main/cpp/gpu_resident/m3b/bridge_create.inc','entry/src/main/cpp/gpu_resident/m3b/napi.inc','entry/src/main/cpp/gpu_resident/m3b/bridge_pump.inc']
 evidence=dict(schema='T650_SOURCE_PACKAGE_AND_SAVED_EVIDENCE_V1',**common(650),packages=pkg,developerRecordedHAP=dev['HAP'],actualHAPMatchesDeveloperReceipt=pkg['input']['sha256']==dev['HAP']['sha256'],selectedSourceAgreement=agreement,
  UIRelevant=[x for x in texts(get('test/reports/t594/v5-page-state.json')) if x.startswith('{') or 'INCOMPLETE:' in x],archiveFailure=archived,
  controllerAbsentAtColdQuery=rejected,WAIT_PAGE_PULSE_SameConfigIdentityProven=False,
  sources=[cuts(bridge,['m5Controller=next','M5_WAIT_PAGE_PULSE','m5Controller.reset','PREFIX_IDENTITY_CONTRACT_REQUIRED','m5SessionImplementation','controller->Pulse(pulse']),cuts(inputs[7],['waitSession','attachFrame','m5Configure','offerSynthetic','m5Pulse'])],inputs=[fp(p) for p in inputs],
  limits=['m3b matching files do not prove all bridge/generation/ABC/source inputs','current isolated source is reviewed, no actual runtime per-stage failure proven','no evidence of same G13 cause or surface/EGL failure'])
 return contract,evidence
def collect651():
 base='.codex/local-test/t562-m2-fix/entry/src/main/';probe=base+'ets/pages/T591Probe.ets';app=base+'cpp/gpu_resident/app_bridge.cpp';store=base+'cpp/gpu_resident/m2/create_diagnostics.h'
 p=txt(probe);assert 'XComponent(' not in p and 'native.newRun()' in p and 'Number(m[1])' in p
 t='\n'.join(texts(get('test/reports/t591b/probe-log.json')));samples=re.findall(r'^\[S(\d+) ',t,re.M);assert list(map(int,samples))==list(range(1,65)) and t.count('NO_CREATE_ATTEMPT')==64
 assert 'u("bootSerial",a.serial)' in txt(store) and 'nativeEndNs' in txt(store)
 a=txt(app);fn=a[a.index('napi_value m2CreateDiagnostics('):a.index('napi_value m2CreateLog(')];assert 'Global().Copy' in fn and 'app().' not in fn
 assert 'createScope.Finish(uint32_t(code))' in a
 contract=dict(schema='T651_M2_SAME_PAGE_COLLECTOR_CONTINUATION_V1',**common(651),nextRole='tester, original T591',newProductFallbackAuthorizedByEvidence=False,
  productChannel='same real GpuResidentM1 page -> libgrsm1 m2CreateDiagnostics/Log -> independent Store mutex; owner creates M2 on its GL thread',
  fixtureFiles=['same build tree ets/physics/T591CreateCollector.ets (new tester module)','same build tree ets/pages/GpuResidentM1.ets (tester-only arm before XComponent, preserve original lifecycle)','tester collector file writer/index and build/source identity receipt'],
  serialField='bootSerial decimal string, never JS Number or regex for numeric serial',identity=['process boot/start','pid','module path/BuildId or raw-byte relation','bootSerial','epoch','window','eglContext','candidateRun','ownerTid'],
  terminal='sealed===true AND nativeEndNs>0 AND actual code present; exit/destructor/timeout never success',
  budget=dict(intervalMs=1000,maxTotalNativeCalls=64,maxSamplingTicks=64,maxWindowSeconds=60,maxUtf8RecordBytes=16384,maxDirectoryBytes=1048576,indexAndTerminalReserveBytes=32768,rawChunkBytes=8192),
  capture=['initial read/collector receipt before XComponent creates; 0 is discovery only','after discovery pin exact bootSerial+scope, stale returns terminate rather than retarget','read logs0..2 only when fields say captured logs changed or terminal, under same 64 total call budget','save complete JSON/raw logs using indexed <=8KiB raw chunks/base64, no substrings as originals'],
  fallbackTrigger='real M2 attempt with same identity open call, existing same-page timer/async channel demonstrated unable to read; preserve client ENTER/EXIT and owner stage/stack before requesting architect fallback',
  fallbackReserved='T622 <=64KiB three-slot/non-GL snapshot only if trigger proven; not a current implementation prerequisite',
  noPhysicalRepair=True,noCpuPhysics=True,noNewManagerGate=True)
 inputs=['docs/architecture-T622-v1.md','test/reports/t591b/collector-result.json','test/reports/t591b/probe-log.json',probe,app,store,base+'ets/pages/GpuResidentM1.ets']
 evidence=dict(schema='T651_SAVED_PROBE_AND_SOURCE_REVIEW_V1',**common(651),probeSamples=64,allNoCreateAttempt=True,baseline=52,afterInvalidate=12,serialLocked=None,
  facilityBugs=['standalone probe has no real M2 XComponent/active lifecycle','regex serial numeric mismatches native bootSerial string','Number conversion loses U64 identity','120/80 and1400 substring screens are not full originals','terminal fallback code0/endNs heuristic mismatches native nativeEndNs/sealed semantics','64 ticks with2 reads is128 calls not64 query budget'],
  sources=[cuts(probe,['serial','slice(','sealed','setInterval','newRun']),cuts(store,['Copy(','bootSerial','nativeEndNs','Finish(']),cuts(app,['m2CreateDiagnostics','m2CreateLog','create468::Scope','createScope.Finish','napi_value newRun'])],
  inputs=[fp(x) for x in inputs],conclusion='existing same-page collector path has not been tried; no demonstrated native read channel blockage or missing Finish')
 return contract,evidence
def main():
 for n,fn in [(649,collect649),(650,collect650),(651,collect651)]:
  c,e=fn()
  for p,v in [(f'docs/architecture-T{n}-contract-v1.json',c),(f'docs/analysis-T{n}-evidence-v1.json',e)]:
   if '--record' in sys.argv:emit(p,v)
   else:assert get(p)==v,p
  print(f'PASS: T{n} saved artifacts/source and finite engineering schema only; no product/device run')
if __name__=='__main__':main()
