"""Four-card architecture receipt and schema audit. No product execution/build/device.
Default verifies saved receipts. --record emits only this batch's engineering documents.
"""
from pathlib import Path
import json,hashlib,re,struct,zipfile,sys
R=Path(__file__).resolve().parents[1];D=R/'docs'
def sha(b):return hashlib.sha256(b).hexdigest()
def get(p):return json.loads((R/p).read_text(encoding='utf-8-sig'))
def text(p):return (R/p).read_text(encoding='utf-8-sig')
def receipt(p):
 b=(R/p).read_bytes();return dict(path=p,bytes=len(b),sha256=sha(b))
def emit(p,x):(R/p).write_text(json.dumps(x,ensure_ascii=False,indent=2)+'\n',encoding='utf-8')
def nodes(x):
 if isinstance(x,dict):
  t=x.get('attributes',{}).get('text','')
  if t:yield t
  for c in x.get('children',[]):yield from nodes(c)
def elf_exports(b):
 assert b[:6]==b'\x7fELF\x02\x01' and struct.unpack_from('<H',b,18)[0]==183
 shoff=struct.unpack_from('<Q',b,40)[0];ents,n=struct.unpack_from('<HH',b,58)
 sections=[struct.unpack_from('<IIQQQQIIQQ',b,shoff+i*ents) for i in range(n)]
 out=set()
 for sec in sections:
  if sec[1]!=11:continue
  st=sections[sec[6]];strings=b[st[4]:st[4]+st[5]]
  for off in range(sec[4],sec[4]+sec[5],sec[9]):
   name,info,other,index,value,size=struct.unpack_from('<IBBHQQ',b,off)
   if index and info>>4 in (1,2) and (other&3) in (0,3):out.add(strings[name:strings.index(b'\0',name)].decode())
 return out
def hap_receipt(task):
 m=get(f'test/reports/t257/{task}.json');p=Path(m['signed']['path']);data=p.read_bytes()
 assert sha(data)==m['signed']['sha256']
 members=[]
 with zipfile.ZipFile(p) as z:
  for n in z.namelist():
   if n.endswith('.so') and any(s in n for s in ['grs_g13','resource_lab','cold_slots']):
    b=z.read(n);r=dict(name=n,bytes=len(b),sha256=sha(b))
    if 'cold_slots' in n:r['exports']=sorted(x for x in elf_exports(b) if x.startswith('Grs'))
    members.append(r)
 return dict(input=m['input'],signed=m['signed'],members=members)

def make():
 lab=[]
 for name in ['lab-record-cancel.json','lab-record-5scancel.json']:
  p='test/reports/t491/t587b/'+name;x=get(p);rows=x['records'];duration=int(rows[-1]['ns'])-int(rows[0]['ns'])
  assert len(rows)==30 and duration<1000000000 and rows[-1]['result']=='COMPLETED_FIXTURE_NOT_PHYSICS'
  assert x['outcome']=='CANCELLED' and not any('CANCEL' in q['stage'] for q in rows)
  lab.append(dict(file=receipt(p),jobId=x['jobId'],nativeDurationNs=str(duration),records=len(rows),
   outcome=x['outcome'],endResult=rows[-1]['result'],cancelCallTimeObserved=False))
 statuses=[];decode=[]
 for dirname in ['t609a','t610a']:
  for p in sorted((R/'test/reports/t491'/dirname).glob('l*.json')):
   for t in nodes(json.loads(p.read_text(encoding='utf-8-sig'))):
    if not t.startswith('{'):continue
    try:x=json.loads(t)
    except ValueError:continue
    if x.get('api')=='g13Configure' and x.get('state')=='NAPI_DECODE':decode.append(dict(file=receipt(p.relative_to(R).as_posix()),reply=x))
    if 'native' in x and isinstance(x['native'],dict):
     n=x['native'];statuses.append(dict(file=receipt(p.relative_to(R).as_posix()),boot=n['fieldToken'].split(':')[1],
      state=n['state'],earned=n['earned'],discarded=n['discarded'],queued=n['queued'],scope=x['scope'],
      capability=n['capability'],compile=n['compile'],lastCommand=x.get('lastCommand')))
 assert decode and decode[0]['reply']['stage']=='TYPEDARRAY_INFO' and decode[0]['reply']['copiedWords']==0
 assert any(x['earned']=='4146' and x['scope']['run']=='0' for x in statuses)
 signed=hap_receipt('T608-g13');delivered=get('test/reports/t608/development.json')['production']
 assert signed['input']['sha256']!=delivered['HAP']['sha256']
 maps=[l.split(maxsplit=5) for l in text('test/reports/t491/t627a/maps-grsm0.txt').splitlines() if 'libgrs_cold_slots.so' in l]
 filemaps=[l for l in maps if l[-1].endswith('/libgrs_cold_slots.so')]
 bss=[l for l in maps if '[anon:' in l[-1]]
 assert len(filemaps)==4 and len(bss)==1 and len({(l[3],l[4]) for l in filemaps})==1
 pages=[json.loads(s) for s in text('test/reports/t491/t627a/gpu-timeline-libgrsm0.so-21802-1-1795050591488-part0.ndjson').splitlines() if s]
 timeline=[q for p in pages for q in p['rows']]
 assert pages[0]['count']==34 and len(timeline)==35 and pages[-1]['count']==35
 source=text('entry/src/main/cpp/gpu_resident/g11vis/visual_transport.h')
 native=text('entry/src/main/cpp/gpu_resident/g11vis/visual_transport.cpp')
 scene=text('entry/src/main/cpp/gpu_resident/g11vis/scene.cpp')
 stage=text('entry/src/main/cpp/gpu_resident/g11vis/visual_stage.glsl')
 assert 'std::array<uint32_t, 64> words' in source and 'std::memcpy(tail_.words.data(),mapped,1024)' in native
 assert 'PreparePrograms(transport.Resources()?0:0)' in scene
 assert len(re.findall(r'\bg11PlanWrite\s*\(',stage))==1
 counts={}
 for parent,q in [('r1',2543),('dm',2923)]:
  table=get(f'docs/architecture-T563-g11-{parent}-dispatch-v1.json')
  assert len(table['source'])==19 and len(table['quantum'])==q
  counts[parent]={k:dict(total=len(table[k]),vis=sum(r['op'].startswith('VIS_') for r in table[k])) for k in ['source','quantum']}
 p=hap_receipt('T600-composite')
 api=['GrsColdReserve','GrsColdBuilderDone','GrsColdReadBegin','GrsColdReadEnd','GrsColdFreed','GrsFullGpuReserve','GrsFullGpuFreed','GrsFullGpuContextRetired']
 broker=next(m for m in p['members'] if 'cold_slots' in m['name']);assert set(broker['exports'])==set(api[:5])
 finalBroker=receipt(delivered['brokerELF']['path'])
 assert finalBroker['sha256']==delivered['brokerELF']['sha256']
 finalBroker['exports']=sorted(x for x in elf_exports((R/finalBroker['path']).read_bytes()) if x.startswith('Grs'))
 assert set(api)<=set(finalBroker['exports'])
 return dict(schema='T633_T636_ARCHITECTURE_AUDIT_V1',date='2026-11-19',deviceUsed=False,buildRun=False,productTestsRun=False,
  T635=dict(nativeJobs=lab,signed=hap_receipt('T579-lab'),sourceMatchesBuildTree=(R/'entry/src/main/cpp/gpu_resident/resource_lab/lab.h').read_bytes()==(R/'.codex/local-test/t565-lab-tree/entry/src/main/cpp/gpu_resident/resource_lab/lab.h').read_bytes()),
  T636=dict(fileMappings=len(filemaps),bssMappings=len(bss),deviceInode=filemaps[0][3:5],timelineRows=len(timeline),initialTimelineCount=pages[0]['count'],
   timelineBrokerCallEvidence=False,observedPackageBroker=broker,availableFinalBroker=finalBroker,abiFunctions=api,componentRun=False),
  T633=dict(decodeRejections=decode,statusSamples=statuses,signedHandoff=signed,
   finalDeveloperDelivery=dict(HAP=delivered['HAP'],ELF=delivered['ELF']),differentInput=True,
   actualRawLengthObserved=False,coldDispatchReachedNotProven=True),
  T634=dict(tables=counts,completionDestinationBytes=64*4,completionCopyBytes=1024,overflowBytes=1024-64*4,
   zeroGenerationCall=True,materialPlanWriterOnlyDefinition=True,sourceFindingsNotDeviceExecution=True,
   cmakeOldGeneratorPath=str((R/'entry/src/main/cpp/gpu_resident/../../../test/t566_generate_composite.cjs').resolve()),
   cmakeOldGeneratorPathExists=(R/'entry/src/main/cpp/gpu_resident/../../../test/t566_generate_composite.cjs').exists()))

def tables():
 policy=dict(deviceUsed=False,productTestsRun=False,render='XComponent+shader only; FAULT or explicit STALE; no Canvas/softwareGL',
  CPU='stopped',goalOrder=[13,14,12],goal10='paused',executionPrompt='docs/architecture-T633-T636-execution-boundaries-v1.md')
 return {
 'docs/architecture-T635-lab-lifecycle-v1.json':dict(schema='T635_LAB_LIFECYCLE_V1',**policy,
  recordRevision='T531_RESOURCE_LAB_RECORD_V3',lifecycle=['RUNNING','CANCELLING','DRAINING','FINISHED'],
  terminal=['COMPLETED_FIXTURE','CANCELLED','CASE_FAILED','INCOMPLETE'],terminalImmutable=True,
  terminalPriority='real failure > cancellation observed under terminal lock > complete END > incomplete; postterminal cancel never rewrites',
  cancelResult=['REQUESTED','ALREADY_REQUESTED','ALREADY_TERMINAL','STALE_JOB'],
  checkpoints=['Build4096words','Hash4096words','Page','DiagnosticStage','CacheSubmit','DrainWait','ReaderBarrier'],
  terminalFields=['jobId','generation','terminalNs','cancelRequestedNs','cancelObservedNs','firstFailure','pendingResources'],
  drain=dict(testOnlyMacro='GRS_PROGRAM_CACHE_TEST_FAULTS',deadlineNs='5000000000',
   wait='same store mutex capacity+insert; wait_until for space and fence; notify cancel/pop/stop/done',
   actualRetirement='return TIMEOUT/CANCELLED does not imply worker or IO freed',productionEnqueueDrainUnchanged=True),
  files=dict(maxFiles=32,maxFileBytes=2097152,maxTotalBytes=67108864,chunkBytes=4096,jsonBytes=16384,ttlNs='60000000000',
   noRenew=True,wrongEnd='STALE_EXPORT leaves other active export unchanged',
   repairs=['quoted base64','RAII failed-open cleanup','checked page and fileId','fd/path stat before+after','frozen fileSHA vs collectorSHA','catalog reset only after export retirement']),
  controls=dict(maxRecords=256,recordBytes=256,newTerminalCancelMetadataMax=1024),
  injection=dict(scope='isolated lab/helper instance only',allowed=['CHECKPOINT_BARRIER','QUEUE_HOLD','CLOCK_EDGE','PRIVATE_FILE_MUTATION'],productionForceBusy=False)),
 'docs/architecture-T636-cross-dso-harness-v1.json':dict(schema='T636_CROSS_DSO_HARNESS_V1',**policy,
  owner='tester',productCodeRequired=False,productionEndpointAdded=False,
  files=['test/t636/adapter_a.cpp','test/t636/adapter_b.cpp','test/t636/driver.cpp','test/t636/CMakeLists.txt','test/t636/receipt.py'],
  linkage='two adapters DT_NEEDED one exact original broker DSO with matching archived header; prefer existing final T608 release8-symbol DSO; old T600 package has only5 symbols; do not merge identities',
  identity=['HAP_SHA','broker_ELF_SHA','BuildId','ABI aarch64 size_t64','pidStart','maps','dlopen errors','resolved symbol provider'],
  limits=dict(slots=2,builders=1,readers=1,processBytes=524375552,gpuTicketBytes=366564432,
   logRows=128,rowUtf8=512,manifestUtf8=16384,totalWallNs='30000000000'),
  concurrency='one builder and one reader may coexist on different slots',routeA=0x63601,routeB=0x63602,
  scenarios=['cross BBuilderDone/AReserve','cross BReadBegin/ATicket','builder and other-slot reader coexist',
   'global reader after both builders done','Freed-under-reader retained with later ReadBegin evidence','ReadEnd not automatic refund',
   'stale ticket after exact free','route sum8192 edge4096+4097','process sum524375552 edge','cross GPU ticket/context component fixture'],
  domains=dict(actual='broker cross-DSO component operations',notProven=['production route invocation','real payload memory release','EGL context destruction','driver physical free'])),
 'docs/architecture-T633-init-observation-v1.json':dict(schema='T633_G13_INIT_OBSERVATION_V1',**policy,
  lifecycle=['UNCONFIGURED','DECODE_REJECTED','CONFIG_QUEUED','WAIT_WINDOW','CONTEXT_CREATE','CAPABILITY','PROGRAM_BUILD','REFLECT',
   'ALLOCATE_SEED','INIT_DISPATCH64','INIT_AUDIT65','INIT_FENCE_PENDING','INIT_TAIL_VALID','COMPUTE_READY','CONFIG_FAILED','RETIRING'],
  configTicket=['boot','attempt','queueSerial','proposedIdentity','configRawSHA','seedReceiptSHA','cancelGeneration'],
  guards=dict(active='configured && transportReady && nonretiring && exact scope',pulse='same ready gate, then existing freshness/quota',
   initialization='independent lifecycle service; not gated by active/pulse/business root',deferred='same ticket and proposedIdentity; cancel on scope change'),
  observation=dict(totalBytes=32768,decodeSlots=4,decodeSlotBytes=1024,stageRows=64,stageRowBytes=256,rawLogs=2,rawLogBytes=4096,controlBytes=4096,
   statusUtf8Max=16384,pageUtf8Max=16384,maxPages=4,readonlyAPI='g13InitDiagnostics(attemptId:string,page:number):string',
   read='fixed snapshot; no owner enqueue/GL calls',unknown='NOT_OBSERVED, never fake zero'),
  fields=['sampleSeq','boot','ticket','attempt','queueSerial','window','context','ownerTid','phase','ENTER_EXIT','monotonicNs',
   'originalNapiStatus','rawReturnedLength','typedKind','offset','dataPresent','backingBytes','providerProfile','rawEGL','rawGL','initOp64_65','fenceResult','ctrl255','ctrl7','sealQualified','firstFailure','dropped','truncated'],
  handoff=dict(package='tester existing final delivery signing/deploy/mapping',decode='T629/T630 shared adapter; G13 actual consumer',
   lifecycle='developer implementation+compilation',independentMath='T632 parallel; no cross-gate')),
 'docs/architecture-T634-composite-closure-v1.json':dict(schema='T634_G11_COMPOSITE_CLOSURE_V1',**policy,
  parentContract='docs/architecture-T563-g11-manifest-v1.json',
  profiles=dict(R1=dict(sourcePasses=19,quantumPasses=2543,sourceAudit=0xcffff,quantumAudit=0xcffff),
   DM=dict(sourcePasses=19,quantumPasses=2923,sourceAudit=0xcffff,quantumAudit=0xfffff)),
  completionWords=256,completionBytes=1024,visualGeneration='U64',
  uniforms=dict(rootKind='explicit SOURCE1 or QUANTUM2',requiredPasses='actual table count',requiredAudits='mask not pass count'),
  audit=dict(planMapping=18,visualReplayGlyph=19,producer='actual successful GPU audits before ROOT_SEAL',CPUFabricationForbidden=True),
  maps=[dict(name=n,byteStart=i*131072,bytes=131072) for i,n in enumerate(['OLD_BED','NEW_BED','OLD_FLOW','NEW_FLOW'])],
  glyphWords=['oldXQ24','oldYQ23','newXQ24','newYQ23','kindFlags','blendQ16','oldPacketMassOr0','appearanceKey'],
  fixes=[dict(id=f'W{i:02}',description=s) for i,s in enumerate([
   'product owner and page wiring','256word completion memcpy safety','explicit root kind count mask',
   'GPU audits and typed acceptance','actual R1/DM parent facade','actual material plan and marker producers',
   'uniform barriers and valid GLSL scope','four maps source/replay identity','nonzero real generation and token',
   'glyph8 packet Pstyle mapping','joint leases complete FBO publication','real cold bytes broker and hashes'],1)],
  APIs=['g11ResearchConfigure(parentKind:string,parentRequest:string,visualConfig64:Uint32Array,seedReceipt:string):string',
   'g11BeginSession(scopeJSON:string):string','g11SetActive(active:boolean,scopeJSON:string):string','g11PagePulse(session:string,seq:string,uiNs:string):string',
   'g11Offer128(words:Uint32Array):string','g11Stop(scopeJSON:string):string','g11Status():string',
   'g11ColdBegin(token:string,mode:string):string','g11ColdPage(id:string,segment:number,offset:number):string','g11ColdEnd(id:string):string'],
  budgets=dict(computeSSBO=4,tailReadBytes=1024,rootFences=1,pageWords=1024,pageUtf8=16384,ttlNs='60000000000',
   maxDmTwoFullBytes=388308992,witness2048=3149824,witness8192=4722688,maxKnownWithWitness=641920128),
  evidence='real nonempty composite frame required for T600/T601; compiled strings/counts not product PASS')}

INPUTS={
 635:['docs/test-report-T587-lab-cancel-v2.md','docs/test-report-T587-lab-page-v1.md','docs/architecture-T531-resource-lab-and-export-v1.md',
 'entry/src/main/cpp/gpu_resident/resource_lab/lab.h','entry/src/main/cpp/gpu_resident/program_cache/artifact_store.h',
 'entry/src/main/cpp/gpu_resident/cold_protocol.h','test/t538/lab_bridge.cpp','test/t538/GpuResourceLab.ets',
 '.codex/local-test/t565-lab-tree/entry/src/main/cpp/gpu_resident/resource_lab/lab.h',
 '.codex/local-test/t565-lab-tree/entry/src/main/cpp/gpu_resident/program_cache/artifact_store.h',
 'test/reports/t491/t587b/lab-record-cancel.json','test/reports/t491/t587b/lab-record-5scancel.json','test/reports/t257/T579-lab.json'],
 636:['docs/test-report-T627-v1.md','docs/test-report-T617-cold-broker-v1.md','test/t617/test_broker.cpp',
 'entry/src/main/cpp/gpu_resident/cold_slot_broker.h','entry/src/main/cpp/gpu_resident/cold_slot_broker.cpp',
 'test/reports/t491/t627a/maps-grsm0.txt','test/reports/t491/t627a/gpu-timeline-libgrsm0.so-21802-1-1795050591488-manifest.json',
 'test/reports/t491/t627a/gpu-timeline-libgrsm0.so-21802-1-1795050591488-part0.ndjson','test/reports/t257/T600-composite.json'],
 633:['docs/test-report-T609-g13-quantify-v1.md','docs/test-report-T610-g13-observe-v1.md','test/reports/t608/development.json',
 'test/reports/t257/T608-g13.json','test/reports/t491/t609a/g13-status.json','test/reports/t491/t609a/l3.json','test/reports/t491/t609a/l6.json',
 'test/reports/t491/t610a/g13-status-t610.json','.codex/local-test/t608-delivery/entry/src/main/cpp/gpu_resident/g13flow/bridge.cpp',
 '.codex/local-test/t608-release/entry/src/main/cpp/gpu_resident/g13flow/bridge.cpp',
 'entry/src/main/cpp/gpu_resident/g13flow/controller.cpp','entry/src/main/cpp/gpu_resident/g13flow/transport.cpp','entry/src/main/ets/pages/GpuResidentG13.ets'],
 634:['docs/test-report-T600-composite-entry-v2.md','docs/architecture-T563-g11-accepted-flux-visual-v1.md',
 'docs/architecture-T563-g11-manifest-v1.json','docs/architecture-T563-g11-r1-dispatch-v1.json','docs/architecture-T563-g11-dm-dispatch-v1.json',
 'entry/src/main/cpp/gpu_resident/g11vis/visual_transport.h','entry/src/main/cpp/gpu_resident/g11vis/visual_transport.cpp',
 'entry/src/main/cpp/gpu_resident/g11vis/visual_stage.glsl','entry/src/main/cpp/gpu_resident/g11vis/scene.cpp','entry/src/main/cpp/gpu_resident/g11vis/cold.cpp',
 'entry/src/main/cpp/gpu_resident/g11vis/kernels.cmake','test/t566_generate_composite.cjs','test/reports/t257/T600-composite.json']}

def main():
 output=make();specs=tables()
 assert 4*1024+64*256+2*4096+4096==32768
 assert (4096+2)//3*4+1024<=16384
 assert 4096+4097>8192 and (524375552-4096)+4097>524375552
 if '--record' in sys.argv:
  for p,s in specs.items():emit(p,s)
  for n,paths in INPUTS.items():emit(f'docs/architecture-T{n}-evidence-v1.json',dict(schema=f'T{n}_SOURCE_RECEIPTS_V1',inputs=[receipt(p) for p in paths]))
  emit('docs/analysis-T633-T636-engineering-v1.json',output)
 else:
  assert get('docs/analysis-T633-T636-engineering-v1.json')==output,'audit changed: inspect sources before re-record'
  for p,s in specs.items():assert get(p)==s,p
  for n in INPUTS:
   for r in get(f'docs/architecture-T{n}-evidence-v1.json')['inputs']:assert r==receipt(r['path']),r['path']
 print('PASS: four architecture contracts, original joins and package/ELF checks; no build/device/product tests')
if __name__=='__main__':main()
