'use strict';
// S4 complete HOT graph + 600 simulated seconds, 36,000 full dual-collect steps.
const fs=require('fs'),assert=require('assert/strict'),{performance,PerformanceObserver}=require('perf_hooks');
const ts=require('C:/Program Files/Huawei/DevEco Studio/tools/hvigor/hvigor/node_modules/typescript');
require.extensions['.ets']=(m,f)=>m._compile(ts.transpileModule(fs.readFileSync(f,'utf8'),
  {compilerOptions:{target:ts.ScriptTarget.ES2020,module:ts.ModuleKind.CommonJS}}).outputText,f);
const {audit}=require('./probe_p011_particle_hot_graph.cjs');
const {makeSandbox,stepHotSandbox}=require('./p011_particle_hot_sandbox.cjs');
const {exportParticleEvidence}=require('../entry/src/main/ets/physics/P011ParticleHotEvidence.ets');
const hot=require('../entry/src/main/ets/physics/P011ParticleHot.ets');
const graph=audit();assert.equal(graph.status,'ZERO_ALLOCATION',JSON.stringify(graph.unresolvedCalls));
const sandbox=makeSandbox(),work=sandbox.work;
const failureSandbox=makeSandbox(8,1),publishFailureSandbox=makeSandbox(8);
const failureFixture=failureSandbox.fixtures.find(f=>f.name==='spawn-tail');
const publishFailureFixture=publishFailureSandbox.fixtures.find(f=>f.name==='spawn-tail');
publishFailureFixture.failAfterPublish=1;
const iterations=Number(process.env.P011_HOT_PROBE_STEPS||36000);
assert.ok(Number.isSafeInteger(iterations)&&iterations>0);
const hotMs=new Float64Array(iterations),hotStart=new Float64Array(iterations),exportMs=new Float64Array(iterations),counts=new Uint32Array(sandbox.fixtures.length);
const bufferRefs=[],bufferLengths=[],bufferOwners=[],bufferKeys=[],visited=new Set();
function bind(value,owner=null,key=null){
  if(value===null||typeof value!=='object'||visited.has(value))return;visited.add(value);
  if(ArrayBuffer.isView(value)){bufferRefs.push(value);bufferLengths.push(value.byteLength);bufferOwners.push(owner);bufferKeys.push(key);return;}
  for(const key of Object.keys(value))bind(value[key],value,key);
}
bind(sandbox);
const originals=new Map(),traps={constructor:0,container:0,serialization:0,helper:0,coldCall:0};
let marker=false;
function wrap(object,name,type){
  const original=object[name];originals.set(object.constructor.name+':'+name+':'+originals.size,[object,name,original]);
  object[name]=function(...args){if(marker){traps[type]++;throw new Error('HOT_ALLOCATION_TRAP '+name);}return Reflect.apply(original,this,args);};
}
const nativeNames=['Array','Map','Set','Float64Array','Float32Array','Uint8Array','Uint32Array','Int32Array','Error'];
const nativeGlobals=nativeNames.map(name=>global[name]);
for(let i=0;i<nativeNames.length;i++){
  const original=nativeGlobals[i];global[nativeNames[i]]=new Proxy(original,{construct(target,args,newTarget){
    if(marker){traps.constructor++;throw new nativeGlobals[8]('HOT_CONSTRUCTOR_TRAP');}return Reflect.construct(target,args,newTarget);
  }});
}
for(const name of ['push','slice','map','filter','concat','sort','splice','flat','flatMap'])wrap(Array.prototype,name,'helper');
for(const name of ['from','of'])wrap(nativeGlobals[0],name,'helper');
for(const name of ['set','clear','delete'])wrap(Map.prototype,name,'container');
for(const name of ['add','clear','delete'])wrap(Set.prototype,name,'container');
wrap(JSON,'stringify','serialization');wrap(JSON,'parse','serialization');
for(const ctor of [nativeGlobals[3],nativeGlobals[4],nativeGlobals[5],nativeGlobals[6],nativeGlobals[7]]){
  for(const name of ['slice','subarray','map','filter'])wrap(ctor.prototype,name,'helper');
}
for(const name of ['snapshot','checkpoint','beginStep','prepare','publish','restore','discard','finishPatch','applyParticlePlan','collectParticlePlan']){
  wrap(Object.getPrototypeOf(work.pool),name,'coldCall');
}
let gcCount=0,gcDuration=0,hotGcCount=0;
const observer=new PerformanceObserver(list=>{for(const entry of list.getEntries()){
  gcCount++;gcDuration+=entry.duration;
  let low=0,high=completed-1,nearest=-1;
  while(low<=high){const mid=Math.floor((low+high)/2);if(hotStart[mid]<=entry.startTime){nearest=mid;low=mid+1;}else high=mid-1;}
  if(nearest>=0&&entry.startTime<hotStart[nearest]+hotMs[nearest])hotGcCount++;
}});observer.observe({entryTypes:['gc']});
const heapBefore=process.memoryUsage(),start=performance.now();let completed=0;
try{
  marker=true;const overflowStatus=stepHotSandbox(failureSandbox,failureFixture,1);marker=false;
  assert.equal(overflowStatus,hot.P011_HOT_JOURNAL_OVERFLOW);assert.equal(failureSandbox.work.active,0);
  assert.equal(failureSandbox.work.eventCount,0);assert.equal(failureSandbox.work.pool.snapshot().liveCount,0);
  marker=true;const publishStatus=stepHotSandbox(publishFailureSandbox,publishFailureFixture,1);marker=false;
  assert.equal(publishStatus,hot.P011_HOT_AUDIT_FAILED);assert.equal(publishFailureSandbox.work.active,0);
  publishFailureFixture.failAfterPublish=0;
  marker=true;const retryStatus=stepHotSandbox(publishFailureSandbox,publishFailureFixture,2);marker=false;
  assert.equal(retryStatus,0);exportParticleEvidence(publishFailureSandbox.work,publishFailureSandbox.work.committedPageToken);
  for(let i=0;i<iterations;i++){
    const fixtureIndex=i%sandbox.fixtures.length,fixture=sandbox.fixtures[fixtureIndex];
    const t=performance.now();hotStart[i]=t;marker=true;
    const status=stepHotSandbox(sandbox,fixture,i+1);
    marker=false;hotMs[i]=performance.now()-t;assert.equal(status,0,fixture.name);counts[fixtureIndex]++;
    assert.equal(work.legacyColdCallCount,0);assert.equal(work.bufferIdentityChanged,0);
    const exportStart=performance.now(),evidence=exportParticleEvidence(work,work.committedPageToken);
    exportMs[i]=performance.now()-exportStart;assert.equal(evidence.stepEpoch,work.stepEpoch);completed++;
  }
}finally{
  marker=false;for(const [object,name,original] of originals.values())object[name]=original;
  for(let i=0;i<nativeNames.length;i++)global[nativeNames[i]]=nativeGlobals[i];
}
let identityChanges=0;
for(let i=0;i<bufferRefs.length;i++)if(bufferRefs[i].byteLength!==bufferLengths[i]||bufferOwners[i][bufferKeys[i]]!==bufferRefs[i])identityChanges++;
assert.equal(identityChanges,0);assert.equal(work.storageValidHot(),true);
for(const value of Object.values(traps))assert.equal(value,0);
function quantiles(array){const sorted=Array.from(array).sort((a,b)=>a-b);return {p50:sorted[Math.floor(sorted.length*.5)],p95:sorted[Math.floor(sorted.length*.95)],max:sorted[sorted.length-1]};}
const requiredTelemetry=['hotContractVersion','ownerId','stepEpoch','attemptToken','activePatchToken','activePatchState',
  'requestCount','motionCount','resultCount','eventCount','lineageCount','requestCapacity','eventCapacity','hotStatus','errorStageCode',
  'rollbackCount','staleHandleRejectCount','patchBusyCount','overflowCount','tokenExhausted','bufferIdentityChanged','legacyColdCallCount','pageBacklog'];
const telemetry={};for(const key of requiredTelemetry){assert.equal(typeof work[key],'number',key);telemetry[key]=work[key];}
const report={status:completed===36000&&graph.status==='ZERO_ALLOCATION'?'ZERO_ALLOCATION':'PARTIAL_RUN',
  scope:'complete isolated M1 HOT path; cold evidence export measured separately',
  simulatedSeconds:completed/60,completedSteps:completed,wallMs:performance.now()-start,
  static:{legacySiteCount:graph.legacySiteCount,remainingSiteCount:graph.remainingSiteCount,reachableNodes:graph.reachableNodes.length,
    registryComplete:graph.registryComplete},runtime:{traps,fixedTypedArrayCount:bufferRefs.length,bufferIdentityChanges:identityChanges,
    fixtures:sandbox.fixtures.map((f,i)=>({name:f.name,count:counts[i]}))},telemetry,
  hotCpuMs:quantiles(hotMs),coldExportMs:quantiles(exportMs),heapBefore,heapAfter:process.memoryUsage(),
  failureBranches:{journalOverflow:true,publishedAuditFailure:true,newAttemptRetry:true},
  gc:{countObserved:null,durationMs:null,hotIntervalCountObserved:null,note:'VM GC includes cold export and JIT/runtime activity; it does not establish explicit source allocations'},
  limitation:'Node does not establish ArkTS device frame time or VM-internal allocation behavior'};
// Let perf_hooks deliver buffered GC entries after the synchronous experiment.
setTimeout(()=>{observer.disconnect();report.gc.countObserved=gcCount;report.gc.durationMs=gcDuration;
  report.gc.hotIntervalCountObserved=hotGcCount;console.log(JSON.stringify(report,null,2));
  if(report.status!=='ZERO_ALLOCATION')process.exitCode=2;
},50);
