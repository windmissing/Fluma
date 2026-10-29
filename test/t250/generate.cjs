'use strict';
const fs=require('fs'),path=require('path'),cp=require('child_process'),crypto=require('crypto');
const root=path.resolve(__dirname,'../..'),out=path.resolve(process.argv[2]||'.codex/local-test/t250-final');
cp.execFileSync(process.execPath,[path.join(root,'test/p013_compute/tools/generate_tx_gpu_project.cjs'),out,'STRICT_O2'],{cwd:root,stdio:'pipe'});
const ets=path.join(out,'entry/src/main/ets'),edits=[],sha=s=>crypto.createHash('sha256').update(s).digest('hex');
function patch(rel,a,b){const p=path.join(ets,rel),s=fs.readFileSync(p,'utf8').replaceAll('\r\n','\n');if(s.split(a).length!==2)throw Error('T250_WHITELIST:'+rel+':'+a.slice(0,80));const v=s.replace(a,b);if(v.replace(b,a)!==s)throw Error('INVERSE');fs.writeFileSync(p,v);edits.push({rel,from:a,to:b,originalSha:sha(s),generatedSha:sha(v)});}
const backend='physics/tx/TxPairBackends.ets',fixture='physics/tx/TxGpuFixture.ets';
patch(backend,"import { P011ParticleSnapshot }", "import { TxCutFault } from './TxCutFault';\nimport { P011ParticleSnapshot }");
patch(backend,"import { TxCutFault }", "import { TxWorkTrace } from './TxWorkTrace';\nimport { TxCutFault }");
patch(backend,'export class TxPairBackends {',`export class TxPairBackends {
  readonly scopeRows:TxWorkTrace=new TxWorkTrace(4096);
  get scopeTrace():TxWorkTrace|null {if(this.config.scopeTraceEnabled===false)return null;this.scopeRows.detail=true;this.scopeRows.clock=this.clock;return this.scopeRows;}
  scopeArchive():string {const t=this.scopeRows;return JSON.stringify({revision:'T250_ORIGINAL_SCOPES_V1',enabled:this.config.scopeTraceEnabled!==false,clockDomain:'CALLER_MONOTONIC_MS',rows:t.rows.slice(0,t.count*32),rowCount:t.count,depth:t.depth,maxDepth:t.maxDepth,incomplete:t.incomplete,capacity:4096,inclusive:t.wallInclusive,exclusive:t.wallExclusive,calls:t.wallCalls,context:'READ_SET_CONTEXT_NOT_CAPTURED'});}
`);
patch(backend,'export class TxPairBackends {',"export class TxPairBackends {readonly cutFault:TxCutFault=new TxCutFault();");
const hot='physics/P011ParticleHot.ets';
patch('physics/WaterSandField.ets','  injectGestureSand(request: P011GestureSandRequest): P011InjectionReceipt | null {',`  injectGestureSand(request: P011GestureSandRequest): P011InjectionReceipt | null {
  const t250SourceFault=this.particleBinding?.work.txPairBackend?.cutFault;
  const t250SourceInjected=t250SourceFault?.enterSource(this.particleBinding?.work.txPairBackend?.metadata.rootId??0,request)??false;`);
patch('physics/WaterSandField.ets','finally{txTrace?.leave(txScope,1,txCode);}', 'finally{if(t250SourceInjected)t250SourceFault?.leaveSource(request,txCode);txTrace?.leave(txScope,1,txCode);}');
patch(hot,'function requestIdentityHot(work: P011ParticleHotWorkspace, p: P011ParticleHotPlanBuffer, row: number): number {',`function requestIdentityHot(work: P011ParticleHotWorkspace, p: P011ParticleHotPlanBuffer, row: number): number {
  const t250Fault=work.txPairBackend?.cutFault;
  const t250Injected=t250Fault?.enter(work.txPairBackend?.metadata.rootId??0,p,row)??false;`);
// Restore the transient corruption before the original caller handles/rolls back.
const hotText=fs.readFileSync(path.join(ets,hot),'utf8'),start=hotText.indexOf('function requestIdentityHot('),end=hotText.indexOf('\nfunction ',start+1);
const fn=hotText.slice(start,end),match=fn.match(/finally\{txTrace\?\.leave\(txScope,6,txCode\);\}/);
if(!match)throw Error('T250_REQUEST_VALIDATE_FINALLY');
patch(hot,match[0],"finally{if(t250Injected)t250Fault?.leave(p,txCode);txTrace?.leave(txScope,6,txCode);}");
patch(backend,'export class TxPairBackends {',`interface TxVisibleArrays {selectorRows:number;cpuTotals:number;cpuCounts:number;metadataRows:number;metadataOrdinals:number;frozenCopies:number;frozenOffsets:number;frozenLengths:number;}
interface TxVisibleStrings {rootRows:number;workloadRows:number;nativeRows:number;errors:number;states:number;faultRows:number;}
interface TxVisibleResource {revision:string;scope:string;typedArrays:TxVisibleArrays;stringsUtf8Bytes:TxVisibleStrings;rootRowsCount:number;selectorRowsCount:number;nativeSlotsAtConfigure:number;wrapperObjectsPerGpuSelect:string;externalCopies:string;driverBytes:string;rssBytes:string;}
export class TxPairBackends {
  readonly workloadRows:string[]=[];
  workload(kind:string,value:number=1):void {if(this.workloadRows.length>=131072)throw new Error('T250_WORKLOAD_FULL');this.workloadRows.push(JSON.stringify({root:this.metadata.rootId,role:this.metadata.role,kind:kind,value:value}));}
  resourceEvidence(rootRows:string[]):string {
    let frozen=0;for(const v of this.frozen0.copies)frozen+=v.byteLength;for(const v of this.frozen1.copies)frozen+=v.byteLength;
    const bytes=(rows:string[]):number=>{let n=0;for(const v of rows)n+=txUtf8Bytes(v);return n;};
    const evidence:TxVisibleResource={revision:'T250_VISIBLE_RESOURCE_V1',scope:'retained fixture owner after last root before GC',
      typedArrays:{selectorRows:this.rows.byteLength,cpuTotals:this.cpuTotals.byteLength,cpuCounts:this.cpuCounts.byteLength,
        metadataRows:this.metadata.rows.byteLength,metadataOrdinals:this.metadata.ordinals.byteLength,
        frozenCopies:frozen,frozenOffsets:this.frozen0.offsets.byteLength+this.frozen1.offsets.byteLength,
        frozenLengths:this.frozen0.lengths.byteLength+this.frozen1.lengths.byteLength},
      stringsUtf8Bytes:{rootRows:bytes(rootRows),workloadRows:bytes(this.workloadRows),nativeRows:bytes(this.nativeRows),
        errors:bytes(this.errors),states:bytes(this.states),faultRows:bytes(this.cutFault.rows)},
      rootRowsCount:rootRows.length,selectorRowsCount:this.count,nativeSlotsAtConfigure:this.ready===null?0:this.ready.liveBuffers,
      wrapperObjectsPerGpuSelect:'26_ESTIMATE_NOT_MEASURED',externalCopies:'NOT_MEASURED',driverBytes:'NOT_MEASURED',rssBytes:'NOT_MEASURED'};
    return JSON.stringify(evidence);
  }
`);
patch(backend,'  merged(status:number,ms:number):void {',"  merged(status:number,ms:number):void {this.workload('MERGE_ATTEMPT');if(status===0)this.workload('M_MERGE_SUCCESS');");
patch(backend,'  enterRole(role:number):void {',"  enterRole(role:number):void {this.workload('A_ROLE_'+role.toString());");
patch(backend,'  cancel():void {',"  cancel():void {this.workload('OWNER_CANCEL');");
patch(backend,'      this.metadata.bind(p,w);',"      this.workload('M_FIND');this.metadata.bind(p,w);");
patch(backend,'      this.rows[o+47]=r.slotA;',"      this.workload(r.status==='PAIR'?'M_FOUND':'M_NO_PAIR');\n      this.rows[o+47]=r.slotA;");
patch(backend,"    }catch(error){const message=", "    }catch(error){this.workload('M_ERROR');const message=");
patch('physics/P011ParticleHot.ets','        if (work.diagnosticEnabled) work.birthCompleted++;',"        work.txPairBackend?.workload('B_BIRTH_COMPLETED');\n        if (work.diagnosticEnabled) work.birthCompleted++;");
patch('physics/P011ParticleHot.ets','      for (let k = 0; k < births; k++) {work.txD1?.trace.add(9);',"      for (let k = 0; k < births; k++) {work.txPairBackend?.workload('B_BIRTH_ENTER');work.txD1?.trace.add(9);");
patch('physics/P011ParticleHot.ets','        if (actionCommonHot(work, p, slot, P011_HOT_KIND_SPAWN) < 0) return P011_HOT_JOURNAL_OVERFLOW;',"        if (actionCommonHot(work, p, slot, P011_HOT_KIND_SPAWN) < 0) return P011_HOT_JOURNAL_OVERFLOW;\n        work.txPairBackend?.workload('B_BIRTH_COMMIT');");
patch('physics/P011ParticleHot.ets',"try{txTrace?.add(23);txTrace?.add(24);", "try{work.txPairBackend?.workload('A_AUDIT_LOGICAL');work.txPairBackend?.workload('A_AUDIT_EXECUTE');txTrace?.add(23);txTrace?.add(24);");
patch('physics/WaterSandField.ets','    for (let attempt = 0; attempt < 8; attempt++) {',"    for (let attempt = 0; attempt < 8; attempt++) {this.particleBinding?.work.txPairBackend?.workload('SOURCE_LAMBDA_ATTEMPT');");
patch('physics/P011TiltPlanningPass.ets','  trace: P011TiltPreviewTrace | null = null;',`  trace: P011TiltPreviewTrace | null = null;
  txMeter:((kind:string)=>void)|null=null;`);
patch('physics/P011TiltPlanningPass.ets',"    return delegate.previewAt(theta);",`    const result=delegate.previewAt(theta);
    this.txMeter?.('STEP_PREVIEW_ATTEMPT_'+role);
    this.txMeter?.(result===null?'STEP_PREVIEW_NULL':'STEP_PREVIEW_VALID');
    return result;`);
patch('physics/P011TiltPlanningPass.ets','    this.lastCommitted = preview;',"    this.txMeter?.('STEP_PLANNER_ACCEPTED');\n    this.lastCommitted = preview;");
patch('physics/P011TiltPlanningPass.ets','    return new P011PlanningPassPreviewHandle(this, input, result, false);',"    this.txMeter?.('STEP_PLANNER_REJECTED');\n    return new P011PlanningPassPreviewHandle(this, input, result, false);");
patch('physics/WaterSandField.ets','this.particleBinding.work.txPairBackend=backend;}',"this.particleBinding.work.txPairBackend=backend;this.planningPassService.txMeter=(kind:string):void=>backend.workload(kind);}");
patch(backend,'  numericPolicy?:string;backend:string;', '  scopeTraceEnabled?:boolean;freezeArm?:string;numericPolicy?:string;backend:string;');
patch(backend,'this.metadata.beginRoot();}', 'this.metadata.beginRoot();this.scopeRows.root=this.metadata.rootId;}');
patch(backend,"    this.config=JSON.parse", "    if(config.freezeArm!==undefined&&config.freezeArm!=='COMMON_FREEZE_ON'&&config.freezeArm!=='COMMON_FREEZE_OFF')throw new Error('T250_FREEZE_ARM');\n    this.config=JSON.parse");
patch(backend,'      let r:TxPairResult;','      if(this.config.freezeArm===\'COMMON_FREEZE_ON\'){const copyStart=this.now();this.frozen0.freeze(p);this.rows[o+60]=this.now()-copyStart;this.rows[o+61]=55*p.nmax+16*p.cols;}\n      let r:TxPairResult;');
// All roots archive original returned receipt/code/state, including failed source roots.
patch(fixture,'cpuCalls:Uint32Array;}','cpuCalls:Uint32Array;rootRows:string[];workloadRows:string[];freezeArm:string;faultPlan:string;resourceEvidence:string;}');
patch(fixture,'resourceEvidence:string;}','resourceEvidence:string;scopeArchive:string;}');
patch(fixture,"import { WaterSandField }", "import { TxFaultPlan,validateTxFaultPlan } from './TxFaultPlan';\nimport { t171Evidence } from '../T171Evidence';\nimport { WaterSandField }");
patch(fixture,'tree:TxTreeBackend|null=null):TxGpuFixtureResult {','tree:TxTreeBackend|null=null,faultPlan:TxFaultPlan|null=null,pid:number=0):TxGpuFixtureResult {');
patch(fixture,'  const field=new WaterSandField','  t171Evidence.reset();\n  const field=new WaterSandField');
patch(fixture,'field.txAttachBackend(backend);',`field.txAttachBackend(backend);
  if(faultPlan!==null){const verdict=validateTxFaultPlan(faultPlan,fixtureId,config.buildManifestSha256,config.backend,config.freezeArm??'COMMON_FREEZE_OFF',pid);if(!verdict.startsWith('READY_'))throw new Error(verdict);backend.cutFault.plan=faultPlan;}
  backend.cutFault.capture=():string=>JSON.stringify({authority:field.t171ArchiveState(),eventsLineage:t171Evidence.exportJson(true)});`);
patch(fixture,"  const before=field.t171ArchiveState();", "  const rootRows:string[]=[];\n  const before=field.t171ArchiveState();");
patch(fixture,'      backend.beginRoot();const h=', '      backend.beginRoot();const rootBefore=field.t171ArchiveState();const h=');
patch(fixture,"      if(r===null||", "      rootRows.push(JSON.stringify({kind:'SOURCE',root:backend.metadata.rootId,offered:h[f.column],receipt:r,before:rootBefore,after:field.t171ArchiveState(),firstCode:r===null?'NULL':r.rejectedReason,rollback:'ORIGINAL_STATE_BYTES_ARCHIVED'}));\n      if(r===null||");
patch(fixture,"    for(let n=0;n<steps;n++){backend.beginRoot();const r=field.step(1/60);if(!r.ok)throw new Error('TX_GPU_PHYSICAL_FAILED:'+String(r.errorCode));}",
 "    for(let n=0;n<steps;n++){backend.beginRoot();const rootBefore=field.t171ArchiveState(),r=field.step(1/60);rootRows.push(JSON.stringify({kind:'STEP',root:backend.metadata.rootId,dt:1/60,receipt:r,before:rootBefore,after:field.t171ArchiveState(),firstCode:r.errorCode,rollback:'ORIGINAL_STATE_BYTES_ARCHIVED'}));if(!r.ok)throw new Error('TX_GPU_PHYSICAL_FAILED:'+String(r.errorCode));}");
patch(fixture,"return {revision:'TX_GPU_FIXTURE_RESULT_V1',", "return {scopeArchive:backend.scopeArchive(),rootRows:rootRows,workloadRows:backend.workloadRows,resourceEvidence:backend.resourceEvidence(rootRows),freezeArm:config.freezeArm??'COMMON_FREEZE_OFF',faultPlan:JSON.stringify({requested:faultPlan,hit:backend.cutFault.hit,rows:backend.cutFault.rows,sealedEventsLineage:t171Evidence.exportJson(true),coverage:'CUT1_AND_6_BEFORE_ONLY'}),revision:'T250_TX_FIXTURE_V4',");
const launcher='physics/tx/TxGpuPageLauncher.ets';
patch('physics/tx/TxGpuWorker.ets',"import worker,{MessageEvents}","import process from '@ohos.process';\nimport worker,{MessageEvents}");
patch('physics/tx/TxGpuWorker.ets',"import fs from '@ohos.file.fs';", "import { TxFaultPlan,validateTxFaultPlan } from './TxFaultPlan';\nimport fs from '@ohos.file.fs';");
fs.copyFileSync(path.join(root,'test/t250/TxFaultPlan.ets'),path.join(ets,'physics/tx/TxFaultPlan.ets'));
fs.copyFileSync(path.join(root,'test/t250/TxCutFault.ets'),path.join(ets,'physics/tx/TxCutFault.ets'));
patch('physics/tx/TxGpuWorker.ets','export interface TxGpuCommand {revision:string;', 'export interface TxGpuCommand {enqueuedAtMs?:number;faultPlan?:TxFaultPlan;suite?:boolean;revision:string;');
patch('physics/tx/TxGpuWorker.ets',"    const start=now(),result=",`    if(c.faultPlan!==undefined){const verdict=validateTxFaultPlan(c.faultPlan,c.fixtureId,c.config.buildManifestSha256,c.config.backend,c.config.freezeArm??'COMMON_FREEZE_OFF',process.pid);if(!verdict.startsWith('READY_'))throw new Error(verdict);}
    const start=now(),queueMs=c.enqueuedAtMs===undefined?NaN:Math.max(0,start-c.enqueuedAtMs),result=`);
patch('physics/tx/TxGpuWorker.ets',"new TxGpuTreeAdapter(c.config.nmax):null),encodeStart=now();","new TxGpuTreeAdapter(c.config.nmax):null,c.faultPlan??null,process.pid),encodeStart=now();");
patch('physics/tx/TxGpuWorker.ets','serviceMs:now()-start,gpuTimer:', 'queueMs:queueMs,serviceMs:now()-start,gpuTimer:');
patch('physics/tx/TxGpuWorker.ets','    physicalStatus=result.physicalStatus;',`    const suiteRows:string[]=[];
    if(c.suite===true)for(const arm of ['COMMON_FREEZE_OFF','COMMON_FREEZE_ON'])for(const backend of ['LEGACY_SCAN','TX_ADJ_AVL','GPU_PAIR_V1']){
      const config=JSON.parse(JSON.stringify(c.config)) as TxGpuConfig;config.backend=backend;config.freezeArm=arm;config.configId+='|suite:'+backend+'|freeze:'+arm;
      const suiteStart=now(),row=runTxGpuFixture(config,c.fixtureId,c.steps,backend==='GPU_PAIR_V1'?new NativeTransport():null,now,backend==='TX_ADJ_AVL'?new TxGpuTreeAdapter(config.nmax):null);
      suiteRows.push(JSON.stringify({pid:process.pid,source:T159_SOURCE_SHA,build:config.buildManifestSha256,config:config,serviceThroughRetirementMs:now()-suiteStart,result:row}));
    }
    physicalStatus=result.physicalStatus;`);
patch('physics/tx/TxGpuWorker.ets','JSON.stringify({command:c,result:result})','JSON.stringify({pid:process.pid,command:c,result:result,suiteRows:suiteRows})');
patch(launcher,"revision:'TX_GPU_COMMAND_V1',fixtureId:","revision:'TX_GPU_COMMAND_V1',suite:p['txSuite']===true,fixtureId:");
patch(launcher,'config:{nmax:f.nmax,',"config:{freezeArm:p['txFreezeArm']===undefined?'COMMON_FREEZE_OFF':String(p['txFreezeArm']),nmax:f.nmax,");
patch(launcher,"import fs from '@ohos.file.fs';","import process from '@ohos.process';import time from '@ohos.systemDateTime';import fs from '@ohos.file.fs';");
patch(launcher,'this.work.postMessage(c);',`if(p['txFaultCut']!==undefined)c.faultPlan={fixtureId:c.fixtureId,buildSHA:c.config.buildManifestSha256,backend:c.config.backend,freezeArm:c.config.freezeArm??'COMMON_FREEZE_OFF',PID:process.pid,rootId:Number(p['txFaultRoot']??1),cutId:Number(p['txFaultCut']),edge:String(p['txFaultEdge']??'BEFORE'),ordinal:Number(p['txFaultOrdinal']??1)};
    c.enqueuedAtMs=time.getUptime(time.TimeType.ACTIVE,false);this.work.postMessage(c);`);
const dictionary=[];
for(let i=0;i<64;i++)dictionary.push({slot:i,name:'backend_specific_'+i,unit:'UNDEFINED_DO_NOT_COMPARE',scope:'backend-private',aggregation:'NOT_COMPARABLE',denominator:'one selector request'});
const nativeNames=['nativeValidation','prepareValidate','prepareFreeze','prepareSort','prepareRange','prepareHashPack','prepareInclusive','upload','dispatchSubmit','fenceWait','map','requestInclusive','ownerConsume','nativeReplyObject','nativeReturnEncode'];
nativeNames.forEach((name,slot)=>dictionary[slot]={slot,name:name+'CpuMs',unit:'ms',scope:'native GPU_PAIR path only; original timer boundary in nativeRows',aggregation:[6,11].includes(slot)?'inclusive':'stage; never sum with inclusive parent',denominator:'one actual native select request',missing:'null/NaN NOT_MEASURED; CPU backend has no native call',gpuRuntime:false});
const valueNames={18:'viewSlot',19:'branchGeneration',20:'contentRevision',21:'realStep',22:'realAttempt',23:'realSubstep',24:'compatStep',25:'compatAttempt',29:'slot',31:'freezePayloadBytes',32:'wrapperObjects',33:'snapshotHashBytes',34:'snapshotHashBlocks',35:'payloadHashBytes',36:'payloadHashBlocks',37:'identityHashBytes',38:'identityHashBlocks',39:'uploadBytes',40:'dispatchCount',41:'fenceCount',42:'resultBytes',43:'ownerGenerationLo',44:'ownerGenerationHi',45:'contextEpochLo',46:'contextEpochHi',51:'nativeRowUtf8Bytes',52:'ownerMetadataGeneration',53:'realRunToken',54:'paramsRevision',55:'inputRevision',56:'seed',59:'barrierCount'};
for(const [key,name]of Object.entries(valueNames)){const slot=Number(key);dictionary[slot]={slot,name,unit:/Bytes$/.test(name)?'bytes':/Blocks$/.test(name)?'SHA256 64-byte compression blocks':/Count$|Objects$/.test(name)?'count':'original scalar/identity',scope:slot>=31&&slot<=46?'GPU_PAIR native request':'current selector request',aggregation:'value, not timing',denominator:'one selector request',missing:'NOT_MEASURED; never fill zero across backend'};}
for(const [slot,name]of [[30,'requiredAdapterFreezeMs'],[50,'nativeRowEncodeMs']])dictionary[slot]={slot,name,unit:'ms',scope:'GPU_PAIR adapter',aggregation:'stage included in selectorInclusiveMs',denominator:'one GPU_PAIR request',missing:'NOT_MEASURED'};
for(const [slot,name,unit,scope]of [[15,'bridgeInclusiveMs','ms','NAPI select call inclusive'],[16,'ownerId','identity','request'],[17,'rootId','identity','root'],[26,'candidateRole','enum','request'],[27,'candidateOrdinal','count','request'],[28,'requestSerial','identity','request'],[47,'slotA','slot','result'],[48,'slotB','slot','result'],[49,'selectorInclusiveMs','ms','entire selector inclusive (not exclusive)'],[57,'mergeStatus','status','mergePairHot return'],[58,'mergeInclusiveMs','ms','mergePairHot inclusive'],[60,'commonFreezeMs','ms','extra common freeze inclusive; excludes native required freeze'],[61,'commonFreezeBytes','bytes','extra common freeze only']])dictionary[slot]={slot,name,unit,scope,aggregation:unit==='ms'?'inclusive':'value',denominator:'one selector request',missing:'NOT_MEASURED_NOT_ZERO'};
function scoped(rel,old,replacement,count){const p=path.join(ets,rel),s=fs.readFileSync(p,'utf8').replaceAll('\r\n','\n');if(s.split(old).length-1!==count)throw Error('T250_SCOPE_VERSION:'+rel);const v=s.replaceAll(old,replacement);fs.writeFileSync(p,v);edits.push({rel,scopeSites:count,originalSha:sha(s),generatedSha:sha(v)});}
scoped(hot,'work.txD1===null?null:work.txD1.trace','work.txD1===null?(work.txPairBackend?.scopeTrace??null):work.txD1.trace',33);
scoped('physics/WaterSandField.ets','this.txD1===null?null:this.txD1.trace','this.txD1===null?(this.particleBinding?.work.txPairBackend?.scopeTrace??null):this.txD1.trace',206);
const old=JSON.parse(fs.readFileSync(path.join(out,'t212-source-manifest.json'))),payload={revision:'T250_SAME_BINARY_V8',parentSourceSha:old.sourceManifestSha256,edits,dictionary,faultPlanModuleSha:sha(fs.readFileSync(path.join(root,'test/t250/TxFaultPlan.ets'))),cutInjectorSha:sha(fs.readFileSync(path.join(root,'test/t250/TxCutFault.ets')))};const sourceSha=sha(JSON.stringify(payload));
for(const n of ['physics/T159BuildIdentity.ets']){let p=path.join(ets,n);fs.writeFileSync(p,fs.readFileSync(p,'utf8').replaceAll(old.sourceManifestSha256,sourceSha));}
const cmake=path.join(out,'entry/src/main/cpp/p013_compute/CMakeLists.txt');fs.writeFileSync(cmake,fs.readFileSync(cmake,'utf8').replaceAll(old.sourceManifestSha256,sourceSha));
fs.writeFileSync(path.join(out,'t250-manifest.json'),JSON.stringify({...payload,sourceManifestSha256:sourceSha,treeBackend:old.treeBackend,productionTouched:false,faultCuts:Array.from({length:20},(_,i)=>({cut:i+1,status:(i===0||i===5)?'BEFORE_IMPLEMENTED':'NOT_IMPLEMENTED',gap:(i===0||i===5)?'AFTER_EDGE_NOT_IMPLEMENTED':'NO_ORIGINAL_CODE_CONTROLLABLE_CUT_INJECTOR'})),resourceBoundary:'retained owner typed arrays and archived UTF8 measured; external/driver/RSS NOT_MEASURED'},null,2));
fs.writeFileSync(path.join(out,'t250-dictionary.json'),JSON.stringify({revision:'T250_LEDGER_V6',rows:dictionary,workload:{B:'B_BIRTH_ENTER at each original birth-loop entry, B_BIRTH_COMPLETED after slot fields and derived finish, B_BIRTH_COMMIT after action journal append; all are attempts including rolled-back work',M:'M_FIND inside selectPair try after prechecks; FOUND/NO_PAIR after original predicate and status validation; ERROR on thrown selector; MERGE_ATTEMPT/SUCCESS after mergePairHot; OWNER_CANCEL is retirement not M_CANCEL',A:'A_ROLE_n at original role boundary; A_AUDIT_LOGICAL=A_AUDIT_EXECUTE at auditParticleHot entry, certHit=0 because this fixture calls full audit; missing audit scopes remain INCOMPLETE',sourceLambda:'SOURCE_LAMBDA_ATTEMPT at actual wet source loop',stepPlanner:'STEP_PREVIEW_ATTEMPT_ROLE and STEP_PLANNER_ACCEPTED/REJECTED from actual planning service; no step lambda scalar exists in current implementation',stepLambda:'NOT_IMPLEMENTED_NO_STEP_LAMBDA_VARIABLE'},faultCuts:'CUT6_BEFORE_IMPLEMENTED_OTHER_39_EDGES_NOT_IMPLEMENTED'},null,2));console.log(out);
