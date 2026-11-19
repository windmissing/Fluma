"""T641 engineering schemas, finite seed bytes and protocol layout only.
No CPU field stepper, product/component test, product build, shader or device execution.
Default checks saved tables/receipts; --record writes this task's documents only.
"""
from pathlib import Path
import json,struct,hashlib,sys,ctypes
R=Path(__file__).resolve().parents[1]
def get(p):return json.loads((R/p).read_text(encoding='utf-8-sig'))
def sha(b):return hashlib.sha256(b).hexdigest()
def canon(x):return json.dumps(x,sort_keys=True,separators=(',',':'),ensure_ascii=False).encode()
def emit(p,x):(R/p).write_text(json.dumps(x,ensure_ascii=False,indent=2)+'\n',encoding='utf-8')
def fp(p):
 b=(R/p).read_bytes();return dict(path=p,bytes=len(b),sha256=sha(b))
def pack(w):return struct.pack('<'+'I'*len(w),*w)
def hw(s):return list(struct.unpack('<8I',bytes.fromhex(s)))
def magic(s):return int.from_bytes(s.encode(),'little')
def rows(n):return [dict(word=i,name='reserved_'+str(i),producer='ZERO',constant=0) for i in range(n)]
def put(a,start,n,name,producer,typ='U32',constant=None):
 for k in range(n):
  d=dict(word=start+k,name=name if n==1 else name+'.'+str(k),producer=producer,type=typ)
  if constant is not None:d['constant']=constant if n==1 else constant[k]
  a[start+k]=d
def manifest():
 m=get('docs/design-T637-manifest-v1.json');parent=get('docs/architecture-T554-g9-dm-manifest-v2.json')
 for key,obj in [('inputPolicySha256','inputPolicy'),('mappingSpecSha256','mappingPolicy'),('nativeClockSpecSha256','receiptClockPolicy')]:assert sha(canon(m[obj]))==m['specHashes'][key]
 assert fp(m['specHashes']['physicalSpecFile'])['sha256']==m['specHashes']['physicalSpecFileSha256']
 common=dict(date='2026-11-19',task='T641',deviceUsed=False,productTestsRun=False,defaultOFF=True,
  profile=m['profile'],profileCode=m['profileCode'],domain=m['domain'],design=fp('docs/design-T637-manifest-v1.json'))
 config=rows(64)
 constants={0:m['configMagic'],1:1,2:m['profileCode'],11:1,19:2,20:125000000,21:8333334,22:1000000000,23:250000000,24:8000000,25:256,26:16,27:63701,28:3,29:1,30:1,31:1000000}
 for f in m['configFields']:
  put(config,f['start'],f['words'],f['name'],'ZERO' if f['name']=='reserved' else 'T637 fixed / parent research parameters / exact named spec hash / computed new seed template')
 for i,v in constants.items():config[i]['constant']=v
 inputs=get('docs/architecture-T543-g9-adapter-manifest-v1.json')['input']['fields'][:]
 for i,n in {0:'GNRX_magic',1:'revision3',2:'GNR1_profile',14:'manualPoseSeqLo',15:'manualPoseSeqHi',16:'manualObservationNsLo',17:'manualObservationNsHi',18:'poseValidatedNsLo',19:'poseValidatedNsHi',43:'UP_receiptNsLo',44:'UP_receiptNsHi',45:'RX_flags',54:'mappingReferenceDurationNsLo',55:'mappingReferenceDurationNsHi',77:'RX_adapter63701'}.items():inputs[i]=n
 raw=rows(64)
 for i,name,value in [(0,'magic',magic('GRWP')),(1,'revision',1),(2,'words',64),(7,'ingressSource',1)]:put(raw,i,1,name,'CONSTANT',constant=value)
 for i,name in [(3,'eventType'),(4,'pointer'),(5,'changedCount'),(6,'activeTouches'),(10,'samplePresent')]:put(raw,i,1,name,'synchronous ArkUI observed argument; native verifies')
 put(raw,8,2,'opaqueSampleBits','observed IEEE64 or absent0','BITS64')
 for i,n in enumerate(['localX','localY','windowX','windowY','displayX','displayY','vp2pxScale','originX','originY']):put(raw,12+2*i,2,n,'same changedTouches point, DataView LE bits','BINARY64')
 put(raw,30,8,'expectedGeometryKeySHA','echo native hash; comparison only','SHA256_LE')
 for i,n in [(38,'expectedInputEpoch'),(40,'expectedPageSession'),(42,'expectedClockGeneration'),(44,'expectedLifecycleGeneration'),(46,'expectedMappingGeneration')]:put(raw,i,2,n,'echo native scope; never grants authority','U64_LE')
 put(raw,48,8,'expectedParameterSHA','echo native configuration','SHA256_LE')
 record=rows(256)
 for f in m['recordFields']:
  name=f['name'];p='same stroke native-owned values; frozen before GPU submission'
  if name=='reserved':p='ZERO'
  elif 'Sample' in name:p='opaque event Number IEEE64; never time arithmetic'
  elif name in ['freezeNs','claimNs','rootDeadlineNs','frozenRootNonce','frozenInputSha','freezeAgeNs']:p='root claim/freeze cut, after complete INPUT128 construction; immutable thereafter'
  elif name in ['parameterSha','actualSeedSha']:p='same new config/actually uploaded audited seed'
  elif name=='nativeBuildSha':p='actual loaded libgrs_g9rx.so whole-file SHA, not a self-embedded ELF digest'
  elif name.endswith('TokenSha'):p='actual successful shader present FrameWitness41 canonical SHA'
  elif name in ['stableCommandSha','moveSummarySha']:p='canonical byte policy in T641 runtime table'
  put(record,f['start'],f['words'],name,p,f['type'])
 for i,v in {0:m['recordMagic'],1:1,2:256,3:m['profileCode'],5:2,6:3,7:1,29:1,125:1,138:0,239:15}.items():record[i]['constant']=v
 wire=dict(schema='T641_RX_WIRE_V1',**common,encoding='U32 LE; U64 low/high; S64 twos-complement; SHA digest bytes packed LE per4; binary64 raw bits',
  Config64=config,ConfigHash='SHA256 exact256 LE bytes including reserved0; no self hash',INPUT128=inputs,
  inputRules=dict(magic=m['inputMagic'],revision=3,profile=m['profileCode'],parameterRevision=1,envelopeFlags=0,manualPoseClock=2,manualPoseSource=2,
   envelopeZero=[34,40],reserved=[78,128],impulse=dict(kind=2,flagsMask=11,rxBit=8,forbiddenBit=4,adapter=63701,Dref=125000000,zeroSourceWords=[64,77]),
   none='word40=0, words41..127 zero',source='word40=1, seq and native receipt; flags0; words46..63 and77..127 zero; parent source words64..76 only'),
  RAW64=raw,RECEIPT_CONTROL256=record,recordRules=m['recordRules'],
  RootConfig128=dict(physicalWords=['muStart','muStop','band','entrainRate','shear','settleTicks','physicalParameterRevision','g'],
   reservedZero=[[8,16],[56,64]],parameterSHA=[16,24],layoutSHA=[24,32],pipelineSHA=[32,40],inputSHA=[40,48],actualSeedSHA=[48,56],fullConfig64=[64,128],DMPhysicalWords=[76,84]),
  compatiblePhysicalLayout=parent['identity'],identityRule='DM state/event/path/completion magic+structure2 retained; profile/parameter/layout/algorithm/pipeline/cache are new; INPUT remains independent revision3')
 payload=[];at=0
 for name,bytes_ in [('WitnessDraftOrRecord20x1024',20480),('ConfigSeedCapabilityMapping4x256',1024),('SerializationAndRootConfigScratch',1024),('TerminalJournal16x64',1024),('Control256IncludingManualPose128',1024),('InputCapsules19x512',9728),('BridgeAndPadding',6656)]:
  payload.append(dict(name=name,byteStart=at,bytes=bytes_,byteEnd=at+bytes_));at+=bytes_
 assert at==40960
 bridge=[];at=34304
 for name,n in [('RAW64x2',512),('Reply',2048),('LoadedIdentityReceipt',1024),('FrameCommitSidecar20x16',320),('MappingScratch2x256',512),('HashWorkspace2x128',256),('SourceIntentKeys16x16',256),('ReservedAlignment',1728)]:
  bridge.append(dict(name=name,byteStart=at,bytes=n,byteEnd=at+n));at+=n
 assert at==40960
 cap=rows(64)
 for i,n,v in [(0,'magic',magic('GNRC')),(1,'revision',1),(2,'profile',m['profileCode'])]:put(cap,i,1,n,'CONSTANT',constant=v)
 for i,n in [(3,'statusMask'),(4,'actualClockId'),(5,'reportedResolutionNs'),(60,'nativeApiPolicyRevision'),(61,'readFailuresSaturated'),(62,'loaderPolicy')]:put(cap,i,1,n,'native checked capability, not JS input')
 put(cap,6,2,'clockGeneration','native checked increasing','U64_LE');put(cap,8,2,'lifecycleGeneration','native checked increasing','U64_LE');put(cap,10,18,'scope','live complete native scope','9xU64_LE')
 for i,n in [(28,'loadedModuleSHA'),(36,'clockConversionSourceSHA'),(44,'clockSpecSHA'),(52,'inputPolicySHA')]:put(cap,i,8,n,'actual runtime artifact / compiled source manifest / named spec','SHA256_LE')
 mapping=rows(64)
 for i,n,v in [(0,'magic',magic('GRMP')),(1,'revision',1),(3,'pointPolicy',1)]:put(mapping,i,1,n,'CONSTANT',constant=v)
 for i,n in [(2,'validMask'),(44,'acceptedAngleQ8'),(45,'viewportWidth'),(46,'viewportHeight')]:put(mapping,i,1,n,'successful present only; native actual values')
 for i,n in [(4,'geometryRevision'),(6,'viewportRevision'),(8,'mappingGeneration'),(26,'physicsCommit'),(28,'publication')]:put(mapping,i,2,n,'actual successful present view','U64_LE')
 for i,n in [(10,'GeometryKeySHA'),(18,'FrameWitnessSHA'),(48,'parameterSHA'),(56,'actualSeedSHA')]:put(mapping,i,8,n,'same immutable presented view','SHA256_LE')
 for i,n in enumerate(['left','top','width','height','vp2pxScale','originX','originY']):put(mapping,30+2*i,2,n,'same immutable presented geometry','BINARY64')
 hashes=dict(GeometryKey=dict(words=67,bytes=268,order=['domain=LE32(GRGK),rev1,profile,pointPolicy','record8..25 scope18','record56..59 geometry+viewport4','record142..143 mappingGeneration2','record100..107 rect8','record76..81 scale/origin6; require94..99 same','record238 angle1','record144..159 parameter+seed16','record168..175 mappingSpec8']),
  FrameWitness=dict(words=41,bytes=164,order=['domain=LE32(GRFW),rev1,profile','scope18','GeometryKeySHA8','actualPhysicsCommit2','actualPublication2','newPipelineSHA8']),
  StableCommand=dict(words=103,bytes=412,order=['domain=LE32(GRSC),rev1','INPUT2 profile','INPUT4..5 run','INPUT12..13 inputEpoch','INPUT26..33 parameterSHA','INPUT40..127 body88']),
  MoveInitial=dict(words=71,order=['domain=LE32(GRMD),rev1,profile','downIngressSeq2','clockGeneration2','RAW_DOWN64']),
  MoveStep=dict(words=78,order=['domain=LE32(GRMM),rev1','previousHash8','ingressSeq2','receiptNs2','RAW_MOVE64']),
  MoveFinal=dict(words=79,order=['domain=LE32(GRMU),rev1','previousHash8','upIngressSeq2','upReceiptNs2','moveCount1','RAW_UP64']))
 enums=dict(capabilityStatus=dict(CLOCK_READ_OK=1,RESOLUTION_OK=2,LOADED_IDENTITY_OK=4,ROUNDING_OK=8,SOURCE_MANIFEST_OK=16),capabilityQualifiedMask=31,
  nativeApiPolicyRevision=1,loaderPolicy=1,mappingValidMask=dict(PRESENTED_FRESH=1,GEOMETRY_KNOWN=2,COMPLETE_SCOPE=4),mappingQualifiedMask=7,
  recordPresence=dict(CURRENT=1,BASE=2),historyQualification=dict(BASE_CAP_REFERENCE_ONLY=1,PRIOR_ROOT_ARCHIVE_ABSENT=2),
  eventType=dict(DOWN=0,MOVE=1,UP=2,CANCEL=3),samplePresent=dict(ABSENT=0,PRESENT=1))
 runtime=dict(schema='T641_RX_RUNTIME_V1',**common,constants=m['constants'],enums=enums,capability64=cap,mapping64=mapping,hashes=hashes,
  clock=dict(first='dedicated ingress callback first operation CheckedClockRead before get_cb_info/Product/locks/parse',zeroNsCanBeValid=True,
   resolution='checked getres, 0<ns<=1000000',ordering='single synchronous ingress and owner each check causal reads; no global max across concurrently sampled timestamps',
   reverse='revoke capability/pending stroke/commands; never max(now,last)',vendorProofRequired=False,suspendLimit='unreported suspend wall age not guaranteed'),
  ingress=dict(channel='one synchronous UI .onTouch channel; no promise/worker/async repost',busy='try-lock/atomic gate failure cancels active stroke through invalidation generation',validationMaxNs=8000000,
   ABI='g9RxTouchIngress(raw64:Uint32Array):string; no JS receipt timestamp, force or verified boolean',typedArray='T629/T630 fixed provider unit profile, strict exact64/uint32/offset0 backing256; UNKNOWN safely rejects'),
  locking=['clock first with no protocol lock','claim ingress gate without waiting','try protocol mutex','copy current mapping/config/cap; validate+enqueue within8ms','release mutex and ingress gate before return',
   'GL owner never calls GL/IO/loader under protocol mutex','render/transport/Capture callbacks recheck outside GL and acquire only same protocol mutex'],
  FIFO=dict(capacity=16,noMoveEntries=True,noOverwrite=True,expiry='owner maintain before claim; expired in-flight retired before exact-slot pop',
   dedupe='compare original semantic words, not hash; queued/claimed/lastConsumed known duplicates ACK; older unretained keys STALE, never replay',
   source='same FIFO, native command serial; separate meaningful source intent, impulse never rearms BLOCKED'),
  APIs=['g9RxResearchConfig(k:number):string','g9RxConfigure(config64:Uint32Array,scopeAndMode:string):string','g9RxSession():string',
   'g9RxSetActive(active:boolean,expectedScope:string):string','g9RxPulse(pulse22:Uint32Array):string','g9RxTarget(target19:Uint32Array):string',
   'g9RxTouchIngress(raw64:Uint32Array):string','g9RxSource(request:string):string','g9RxStatus():string','g9RxStop(expectedScope:string):string',
   'g9RxLoadedIdentity():string','g9RxColdBegin(replaceId:string,mode:string):string','g9RxColdState(requestId:string):string',
   'g9RxColdPage(traceId:string,page:number):string','g9RxColdEnd(traceId:string):string'],
  files=dict(native='entry/src/main/cpp/gpu_resident/g9rx/',library='libgrs_g9rx.so',page='entry/src/main/ets/pages/GpuResidentG9DmRx.ets',
   bridge='entry/src/main/ets/physics/GpuG9RxPort.ets',collector='entry/src/main/ets/physics/GpuG9RxCold.ets',generator='tools/g9rx/generate_assets.py'),
  oldRoutesUnchanged=['g9spf REAL_TOUCH_R2','g9dm SYNTHETIC','T616 routeA'],qualificationNotProductPASS=True)
 header=rows(256)
 names=['magic','revision','prefixWords','seal','profile','mode','segmentCount','K','payloadBytesLo','payloadBytesHi','requestLo','requestHi','traceLo','traceHi','createdNsLo','createdNsHi','runLo','runHi','contextLo','contextHi','surfaceLo','surfaceHi','pageSessionLo','pageSessionHi','inputEpochLo','inputEpochHi','rootNonceLo','rootNonceHi','outcome','selectedJ','eventCount','pathCount']
 for i,n in enumerate(names):put(header,i,1,n,'same immutable root capture; no zero substitutes')
 for i,n in [(32,'currentDownCommit'),(34,'currentUpCommit'),(36,'baseDownCommit'),(38,'baseUpCommit')]:put(header,i,2,n,'record-matched FrameCommitSidecar, absent record0','U64_LE')
 put(header,40,64,'currentCapability64','actual frozen native capability64')
 for i,n in [(104,'currentGeometryKeySHA'),(112,'baseGeometryKeySHA'),(120,'pipelineSHA'),(128,'layoutSHA'),(136,'algorithmSHA'),(144,'configSHA'),(152,'actualSeedSHA'),(160,'clockCodeSourceSHA'),(168,'mappingCodeSourceSHA'),(176,'loadedIdentityReceiptSHA'),(184,'mappingSpecSHA'),(192,'inputPolicySHA'),(200,'clockSpecSHA'),(208,'seedTemplateSHA'),(216,'loadedModuleSHA'),(232,'journalRollingSHA'),(240,'compiledSourceManifestSHA')]:put(header,i,8,n,'actual named source / snapshot SHA, computed before prefix publication','SHA256_LE')
 for i,n in [(224,'recordPresenceMask'),(225,'historyQualification'),(228,'journalCount'),(229,'journalCursor')]:put(header,i,1,n,'actual bounded native snapshot')
 put(header,226,2,'journalTotal','actual counter','U64_LE');put(header,230,2,'journalLost','actual counter','U64_LE')
 for i,v in {0:magic('GRXC'),1:1,2:8192,3:0xc0de6371,4:m['profileCode']}.items():header[i]['constant']=v
 profiles=[]
 for p in parent['resources']:
  modes={}
  for mode in ['ROOT','FULL']:
   pos=32768;segs=[]
   for name,n in [('BASE',p['stateBytes']),('FINAL',p['stateBytes']),('SELECTED_EVENT_PATH',p['arenaBytes']),('AUTHORITATIVE_EVENT_PATH',p['arenaBytes'])]+([(f'DISCOVERY_J{j}_EVENT_PATH',p['arenaBytes']) for j in [1,2,4,8,16,32]] if mode=='FULL' else []):
    segs.append(dict(name=name,byteStart=pos,bytes=n,byteEnd=pos+n));pos+=n
   assert pos==p[mode]['bytes'];modes[mode]=dict(bytes=pos,segments=segs,actualPages=1+pos//4096,pageMax=p[mode]['pageMax'],archiveUtf8Max=p[mode]['archiveUtf8Max'])
  profiles.append(dict(K=p['K'],stateBytes=p['stateBytes'],eventBytes=p['eventBytes'],pathByteOffset=p['pathByteOffset'],arenaBytes=p['arenaBytes'],resident=p['residentPayloadBytes'],extraFullGPU=p['fullGPUAdditionalBytes'],modes=modes))
 seedmeta=rows(64)
 for i,n,v in [(0,'magic',magic('GRSM')),(1,'revision',1),(2,'words',64),(3,'seal',0xc0de6372),(33,'seedRevision',1),(36,'templateAlgorithm',1)]:put(seedmeta,i,1,n,'CONSTANT',constant=v)
 for i,n in [(4,'templateSHA'),(12,'actualUploadedSeedSHA'),(20,'parameterSHA')]:put(seedmeta,i,8,n,'exact bytes at named construction stage','SHA256_LE')
 for i,n in [(28,'initialM0'),(30,'initialOrigin'),(34,'uploadedSeedBytes')]:put(seedmeta,i,2,n,'same audited initial seed','U64_LE')
 put(seedmeta,32,1,'initialSourceId','same audited initial seed')
 journal=rows(16)
 for i,n in [(0,'kind'),(1,'status')]:put(journal,i,1,n,'terminal operation code; named external receipt must match')
 for i,n in [(2,'clockGeneration'),(4,'commandSeq'),(6,'rootNonce'),(8,'observedNs'),(10,'upReceiptNs'),(12,'commit'),(14,'publication')]:put(journal,i,2,n,'actual operation identity; unavailable commit/publication zero, not inferred','U64_LE')
 storage=dict(schema='T641_RX_STORAGE_COLD_V1',**common,payloadBytes=40960,payloadAlignment=64,payload=payload,bridge=bridge,SeedMeta64=seedmeta,TerminalJournal16=journal,
  sizeProof='alignas(64) array<uint32_t,10240>; sizeof must40960, sizeof Sync excluded fixed helper<=256 and reported separately; no second legacy input FIFO',
  witnessSlots=dict(queue=[0,16],active=16,currentFrozen=17,baseSnapshot=18,lastConsumed=19),
  inputSlots=dict(queue=[0,16],frozen=16,lastConsumedSource=17,lastConsumedImpulse=18),
  Control256=dict(controlWords=[0,128],manualPoseInput128=[128,256]),
  coldCapability=m['coldCapability'],coldPage='G9_DM_RX_SEGMENT_PAGE_V1',header256=header,
  prefix=dict(bytes=32768,header=[0,256],completion=[256,512],input=[512,640],rootConfig=[640,768],quota=[768,1024],scopeSummaries=[1024,1408],zeroGap=[1408,2048],descriptors=[2048,3072],currentRecord=[3072,3328],baseRecord=[3328,3584],config=[3584,3648],seedMeta=[3648,3712],zeroTail=[3712,8192]),
  prefixDigest='no self descriptor; external rawPrefixSHA/payloadRawSHA/pagesFileSHA; records have no embedded self hash',
  profiles=profiles,pageWords=1024,pageUtf8=16384,slots=2,builders=1,readers=1,ttlNs='60000000000',
  recordMeaning=dict(current='frozen before GPU, not accepted; claim/freeze/nonce/INPUT_SHA fixed',base='prior actually consumed impulse linked to BASE state capsule; no promotion on failure',
   frame='header32..39 stores actual commits to recompute FrameWitness41, never infer commit from publication',
   clock='BASE older generation may be reference-only; missing prior archive never full historical replay'),
  promotion='clone CURRENT on actual typed accept+commit only; lastConsumed copy role2, never edit published CURRENT; next root copies BASE snapshot before new candidate',
  budgets=m['budget'])
 seeds=[];a=get('docs/architecture-T552-g9-seed-manifest-v2.json')['seed']['canonicalAssignments']
 for p,pp in zip(parent['resources'],parent['parameterProfiles']):
  k=p['K'];w=[0]*(p['stateBytes']//4)
  for i,v in a['header'].items():w[int(i)]=v
  w[0]=parent['identity']['stateMagic'];w[1]=2;w[2]=m['profileCode'];w[24]=k;w[106:114]=pp['config64'][48:56]
  for c in range(128):
   for i,v in a['columnEvery32Words'].items():w[512+32*c+int(i)]=v
   w[4608+c*256:4612+c*256]=a['bedRank0Every64Cohort4']
  source=(280576+256*k)//4;w[source:source+16]=a['sourceRow0']
  templateSha=sha(pack(w));cfg=[0]*64
  for i,v in constants.items():cfg[i]=v
  cfg[3]=k;cfg[4:11]=pp['config64'][4:11];cfg[12:20]=pp['config64'][48:56]
  cfg[32:40]=hw(m['specHashes']['physicalSpecFileSha256']);cfg[40:48]=hw(m['specHashes']['inputPolicySha256']);cfg[48:56]=hw(templateSha)
  parameterSha=sha(pack(cfg));actual=w[:]
  for at,v in [(4,7),(6,1),(8,11),(14,2)]:actual[at:at+2]=[v,0]
  actual[42:50]=hw(parameterSha)
  seeds.append(dict(K=k,seedBytes=len(w)*4,templateSHA=templateSha,config64=cfg,parameterSHA=parameterSha,
   exampleIdentity=dict(run=7,owner=1,context=11,inputEpoch=2),exampleActualSeedSHA=sha(pack(actual)),
   assignment='T552 corrected S40, DM2 magic/rev+GNR1 profile, DM header106..113 from config12..19; all unused0',exampleIsSpecificationNotDeployedArtifact=True))
 seedtable=dict(schema='T641_RX_SEED_CONFIG_V1',**common,profiles=seeds,
  hashOrder=['canonical complete template dynamic/parameter fields0','template raw SHA','Config64 SHA','native actual scope/header parameter SHA','actual uploaded SHA','five same originals upload','initial GPU audit','valid new shader present mapping'],
  noSelfEmbeddedElfHash=True,noOldRunImport=True)
 return {'docs/architecture-T641-wire-v1.json':wire,'docs/architecture-T641-runtime-v1.json':runtime,'docs/architecture-T641-storage-cold-v1.json':storage,'docs/architecture-T641-seed-config-v1.json':seedtable}

INPUTS=['docs/algorithm-T637-v1.md','docs/design-T637-manifest-v1.json','docs/design-T637-fixtures-v1.json',
 'docs/architecture-T554-g9-dm-manifest-v2.json','docs/architecture-T543-g9-adapter-manifest-v1.json','docs/architecture-T552-g9-seed-manifest-v2.json',
 'entry/src/main/cpp/gpu_resident/g9dm/research_config.h','entry/src/main/cpp/gpu_resident/g9dm/controller.cpp','entry/src/main/cpp/gpu_resident/g9dm/input_gate.h',
 'entry/src/main/cpp/gpu_resident/g9dm/touch_adapter.h','entry/src/main/cpp/gpu_resident/g9dm/pipeline.comp','entry/src/main/cpp/gpu_resident/g9dm/scene.cpp',
 'entry/src/main/cpp/gpu_resident/g9dm/cold_capture.cpp','entry/src/main/cpp/gpu_resident/g9dm/cold_contract.h','entry/src/main/cpp/gpu_resident/g9dm/controller_cold.cpp',
 'entry/src/main/ets/pages/GpuResidentG9.ets']
def main():
 t=manifest();wire=t['docs/architecture-T641-wire-v1.json'];store=t['docs/architecture-T641-storage-cold-v1.json']
 for n,size in [('Config64',64),('RAW64',64),('RECEIPT_CONTROL256',256)]:assert [f['word'] for f in wire[n]]==list(range(size))
 assert len(wire['INPUT128'])==128 and sum(r['bytes'] for r in store['payload'])==40960
 assert ctypes.sizeof(ctypes.c_uint32*10240)==40960
 assert 98022528+140009472+376119296+40960+16384==614208640
 assert 12+4*2404==9628
 for p in store['profiles']:
  for mo in p['modes'].values():
   assert mo['segments'][-1]['byteEnd']==mo['bytes']
 assert 125000000*4==500000000 and 1000000000//500000000==2
 # Byte-contract examples only, not a writer/reader or physical oracle.
 raw=list(range(103));h=sha(pack(raw));raw[-1]^=1;assert h!=sha(pack(raw))
 abi=get('docs/analysis-T641-abi-v1.json')
 assert abi['exitCode']==0 and abi['layouts']['RxProtocolPayload']['sizeofBytes']==40960
 assert abi['layouts']['RxSyncState']['sizeofBytes']==64
 for item in ['compiler','specimen','log']:
  saved=abi[item];b=Path(saved['path']).read_bytes();assert len(b)==saved['bytes'] and sha(b)==saved['sha256']
 for n,size in [('SeedMeta64',64),('TerminalJournal16',16),('header256',256)]:assert [f['word'] for f in store[n]]==list(range(size))
 evidence=dict(schema='T641_FINITE_ENGINEERING_EVIDENCE_V1',deviceUsed=False,productTestsRun=False,productBuildsRun=False,engineeringAbiSyntaxCompile=True,
  checks=['64/128/256 complete field maps','40960 packed protocol storage without duplicate legacy FIFO','two new seed/config exact-byte specifications',
   'unchanged DM cold directory and GPU/dispatch budgets','fixed125ms gain exactly2/s, duration excluded','stable preimage mutation changes SHA'],
  limits=['isolated aarch64 ABI specimen only; real integrated T644 objects must report actual sizeof again','no actual deployed pipeline/ELF SHA fabricated','no gesture, physics, live clock, reader, performance or driver qualification'],
  abiReceipt=fp('docs/analysis-T641-abi-v1.json'),inputs=[fp(p) for p in INPUTS])
 if '--record' in sys.argv:
  for p,v in t.items():emit(p,v)
  emit('docs/analysis-T641-engineering-v1.json',evidence)
 else:
  for p,v in t.items():assert get(p)==v,p
  assert get('docs/analysis-T641-engineering-v1.json')==evidence,'source changed; inspect before re-record'
 print('PASS: T641 finite engineering schemas/seed bytes/layout/hash identities only; no product execution')
if __name__=='__main__':main()
