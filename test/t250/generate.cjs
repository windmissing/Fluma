'use strict';
const fs=require('fs'),path=require('path'),cp=require('child_process'),crypto=require('crypto');
const root=path.resolve(__dirname,'../..'),out=path.resolve(process.argv[2]||'.codex/local-test/t250-final');
cp.execFileSync(process.execPath,[path.join(root,'test/p013_compute/tools/generate_tx_gpu_project.cjs'),out,'STRICT_O2'],{cwd:root,stdio:'pipe'});
const ets=path.join(out,'entry/src/main/ets'),edits=[],sha=s=>crypto.createHash('sha256').update(s).digest('hex');
function patch(rel,a,b){const p=path.join(ets,rel),s=fs.readFileSync(p,'utf8').replaceAll('\r\n','\n');if(s.split(a).length!==2)throw Error('T250_WHITELIST:'+rel+':'+a.slice(0,80));const v=s.replace(a,b);if(v.replace(b,a)!==s)throw Error('INVERSE');fs.writeFileSync(p,v);edits.push({rel,from:a,to:b,originalSha:sha(s),generatedSha:sha(v)});}
const backend='physics/tx/TxPairBackends.ets',fixture='physics/tx/TxGpuFixture.ets';
patch(backend,'export class TxPairBackends {',`export class TxPairBackends {
  readonly workloadRows:string[]=[];
  workload(kind:string,value:number=1):void {if(this.workloadRows.length>=131072)throw new Error('T250_WORKLOAD_FULL');this.workloadRows.push(JSON.stringify({root:this.metadata.rootId,role:this.metadata.role,kind:kind,value:value}));}
`);
patch(backend,'  merged(status:number,ms:number):void {',"  merged(status:number,ms:number):void {this.workload('MERGE_ATTEMPT');if(status===0)this.workload('M_MERGE_SUCCESS');");
patch(backend,'  enterRole(role:number):void {',"  enterRole(role:number):void {this.workload('A_ROLE_'+role.toString());");
patch('physics/P011ParticleHot.ets','        if (work.diagnosticEnabled) work.birthCompleted++;',"        work.txPairBackend?.workload('B_BIRTH_COMPLETED');\n        if (work.diagnosticEnabled) work.birthCompleted++;");
patch('physics/WaterSandField.ets','    for (let attempt = 0; attempt < 8; attempt++) {',"    for (let attempt = 0; attempt < 8; attempt++) {this.particleBinding?.work.txPairBackend?.workload('SOURCE_LAMBDA_ATTEMPT');");
patch(backend,'  numericPolicy?:string;backend:string;', '  freezeArm?:string;numericPolicy?:string;backend:string;');
patch(backend,"    this.config=JSON.parse", "    if(config.freezeArm!==undefined&&config.freezeArm!=='COMMON_FREEZE_ON'&&config.freezeArm!=='COMMON_FREEZE_OFF')throw new Error('T250_FREEZE_ARM');\n    this.config=JSON.parse");
patch(backend,'      let r:TxPairResult;','      if(this.config.freezeArm===\'COMMON_FREEZE_ON\'){const copyStart=this.now();this.frozen0.freeze(p);this.rows[o+60]=this.now()-copyStart;this.rows[o+61]=55*p.nmax+16*p.cols;}\n      let r:TxPairResult;');
// All roots archive original returned receipt/code/state, including failed source roots.
patch(fixture,'cpuCalls:Uint32Array;}','cpuCalls:Uint32Array;rootRows:string[];workloadRows:string[];freezeArm:string;faultPlan:string;}');
patch(fixture,"  const before=field.t171ArchiveState();", "  const rootRows:string[]=[];\n  const before=field.t171ArchiveState();");
patch(fixture,'      backend.beginRoot();const h=', '      backend.beginRoot();const rootBefore=field.t171ArchiveState();const h=');
patch(fixture,"      if(r===null||", "      rootRows.push(JSON.stringify({kind:'SOURCE',root:backend.metadata.rootId,offered:h[f.column],receipt:r,before:rootBefore,after:field.t171ArchiveState(),firstCode:r===null?'NULL':r.rejectedReason,rollback:'ORIGINAL_STATE_BYTES_ARCHIVED'}));\n      if(r===null||");
patch(fixture,"    for(let n=0;n<steps;n++){backend.beginRoot();const r=field.step(1/60);if(!r.ok)throw new Error('TX_GPU_PHYSICAL_FAILED:'+String(r.errorCode));}",
 "    for(let n=0;n<steps;n++){backend.beginRoot();const rootBefore=field.t171ArchiveState(),r=field.step(1/60);rootRows.push(JSON.stringify({kind:'STEP',root:backend.metadata.rootId,dt:1/60,receipt:r,before:rootBefore,after:field.t171ArchiveState(),firstCode:r.errorCode,rollback:'ORIGINAL_STATE_BYTES_ARCHIVED'}));if(!r.ok)throw new Error('TX_GPU_PHYSICAL_FAILED:'+String(r.errorCode));}");
patch(fixture,"return {revision:'TX_GPU_FIXTURE_RESULT_V1',", "return {rootRows:rootRows,workloadRows:backend.workloadRows,freezeArm:config.freezeArm??'COMMON_FREEZE_OFF',faultPlan:'20_CUT_INJECTION_NOT_IMPLEMENTED',revision:'T250_TX_FIXTURE_V2',");
const launcher='physics/tx/TxGpuPageLauncher.ets';
patch('physics/tx/TxGpuWorker.ets',"import worker,{MessageEvents}","import process from '@ohos.process';\nimport worker,{MessageEvents}");
patch('physics/tx/TxGpuWorker.ets','export interface TxGpuCommand {revision:string;', 'export interface TxGpuCommand {suite?:boolean;revision:string;');
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
const dictionary=[];
for(let i=0;i<64;i++)dictionary.push({slot:i,name:'backend_specific_'+i,unit:'UNDEFINED_DO_NOT_COMPARE',scope:'backend-private',aggregation:'NOT_COMPARABLE',denominator:'one selector request'});
const nativeNames=['nativeValidation','prepareValidate','prepareFreeze','prepareSort','prepareRange','prepareHashPack','prepareInclusive','upload','dispatchSubmit','fenceWait','map','requestInclusive','ownerConsume','nativeReplyObject','nativeReturnEncode'];
nativeNames.forEach((name,slot)=>dictionary[slot]={slot,name:name+'CpuMs',unit:'ms',scope:'native GPU_PAIR path only; original timer boundary in nativeRows',aggregation:[6,11].includes(slot)?'inclusive':'stage; never sum with inclusive parent',denominator:'one actual native select request',missing:'null/NaN NOT_MEASURED; CPU backend has no native call',gpuRuntime:false});
const valueNames={18:'viewSlot',19:'branchGeneration',20:'contentRevision',21:'realStep',22:'realAttempt',23:'realSubstep',24:'compatStep',25:'compatAttempt',29:'slot',31:'freezePayloadBytes',32:'wrapperObjects',33:'snapshotHashBytes',34:'snapshotHashBlocks',35:'payloadHashBytes',36:'payloadHashBlocks',37:'identityHashBytes',38:'identityHashBlocks',39:'uploadBytes',40:'dispatchCount',41:'fenceCount',42:'resultBytes',43:'ownerGenerationLo',44:'ownerGenerationHi',45:'contextEpochLo',46:'contextEpochHi',51:'nativeRowUtf8Bytes',52:'ownerMetadataGeneration',53:'realRunToken',54:'paramsRevision',55:'inputRevision',56:'seed',59:'barrierCount'};
for(const [key,name]of Object.entries(valueNames)){const slot=Number(key);dictionary[slot]={slot,name,unit:/Bytes$/.test(name)?'bytes':/Blocks$/.test(name)?'SHA256 64-byte compression blocks':/Count$|Objects$/.test(name)?'count':'original scalar/identity',scope:slot>=31&&slot<=46?'GPU_PAIR native request':'current selector request',aggregation:'value, not timing',denominator:'one selector request',missing:'NOT_MEASURED; never fill zero across backend'};}
for(const [slot,name]of [[30,'requiredAdapterFreezeMs'],[50,'nativeRowEncodeMs']])dictionary[slot]={slot,name,unit:'ms',scope:'GPU_PAIR adapter',aggregation:'stage included in selectorInclusiveMs',denominator:'one GPU_PAIR request',missing:'NOT_MEASURED'};
for(const [slot,name,unit,scope]of [[15,'bridgeInclusiveMs','ms','NAPI select call inclusive'],[16,'ownerId','identity','request'],[17,'rootId','identity','root'],[26,'candidateRole','enum','request'],[27,'candidateOrdinal','count','request'],[28,'requestSerial','identity','request'],[47,'slotA','slot','result'],[48,'slotB','slot','result'],[49,'selectorInclusiveMs','ms','entire selector inclusive (not exclusive)'],[57,'mergeStatus','status','mergePairHot return'],[58,'mergeInclusiveMs','ms','mergePairHot inclusive'],[60,'commonFreezeMs','ms','extra common freeze inclusive; excludes native required freeze'],[61,'commonFreezeBytes','bytes','extra common freeze only']])dictionary[slot]={slot,name,unit,scope,aggregation:unit==='ms'?'inclusive':'value',denominator:'one selector request',missing:'NOT_MEASURED_NOT_ZERO'};
const old=JSON.parse(fs.readFileSync(path.join(out,'t212-source-manifest.json'))),payload={revision:'T250_SAME_BINARY_V2',parentSourceSha:old.sourceManifestSha256,edits,dictionary};const sourceSha=sha(JSON.stringify(payload));
for(const n of ['physics/T159BuildIdentity.ets']){let p=path.join(ets,n);fs.writeFileSync(p,fs.readFileSync(p,'utf8').replaceAll(old.sourceManifestSha256,sourceSha));}
const cmake=path.join(out,'entry/src/main/cpp/p013_compute/CMakeLists.txt');fs.writeFileSync(cmake,fs.readFileSync(cmake,'utf8').replaceAll(old.sourceManifestSha256,sourceSha));
fs.writeFileSync(path.join(out,'t250-manifest.json'),JSON.stringify({...payload,sourceManifestSha256:sourceSha,treeBackend:old.treeBackend,productionTouched:false,faultCuts:Array.from({length:20},(_,i)=>({cut:i+1,status:'NOT_IMPLEMENTED'})),resourceBoundary:'native/driver total NOT_MEASURED; rootRows strings bounded by max34 roots, variable exact bytes archived by Worker'},null,2));
fs.writeFileSync(path.join(out,'t250-dictionary.json'),JSON.stringify({revision:'T250_LEDGER_V2',rows:dictionary,workload:{B:'B_BIRTH_COMPLETED at original birth write, includes failed/rolled-back work; accepted denominator remains receipt',M:'M_MERGE_SUCCESS only status0; MERGE_ATTEMPT separately',A:'A_ROLE_n actual enterRole boundary calls; not entire planner attempt count',sourceLambda:'SOURCE_LAMBDA_ATTEMPT at actual wet source loop; step planner lambda NOT_IMPLEMENTED'},faultCuts:'20_NOT_IMPLEMENTED'},null,2));console.log(out);
