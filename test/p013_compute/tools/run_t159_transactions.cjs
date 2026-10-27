'use strict';
// Real generated field/M2/Hot closure. GPU selector here is explicitly an OFFLINE CPU STAND-IN.
const fs=require('node:fs'),path=require('node:path'),assert=require('node:assert/strict'),crypto=require('node:crypto');
const ts=require('C:/Program Files/Huawei/DevEco Studio/tools/hvigor/hvigor/node_modules/typescript');
require.extensions['.ets']=(m,f)=>m._compile(ts.transpileModule(fs.readFileSync(f,'utf8'),{compilerOptions:{target:ts.ScriptTarget.ES2020,module:ts.ModuleKind.CommonJS}}).outputText,f);
const base=path.resolve('.codex/local-test/t159-project/entry/src/main/ets/physics');
const {WaterSandField}=require(path.join(base,'WaterSandField.ets'));
const {defaultP011ParticleParams}=require(path.join(base,'P011ParticleM2Field.ets'));
const selector=require(path.join(base,'T159Selector.ets')),meta=require(path.join(base,'T159Metadata.ets'));
const fields=['pMass','pX','pY','pVx','pVy','pPhase','pSource','pState','pId','pSourceId','freeSlots','colParticleMassH','colParticleMassL'];
const digest=b=>crypto.createHash('sha256').update(b).digest('hex');
function poolBytes(p){return fields.map(k=>Buffer.from(p[k].buffer,p[k].byteOffset,p[k].byteLength).toString('hex'));}
let cases=[],references=new Map();
for(const nmax of [192,512])for(const route of ['LEGACY_SCAN','T123','GPU']){
 meta.t159ResetMetadata();let calls=0,roles=new Set(),mutations=[],previous='';
 selector.t159Arm(route,route==='GPU'?{selectPair(p,r,w){calls++;roles.add(meta.t159CandidateRole());const v=meta.t159PoolVersion(p);const version=v.hi+':'+v.lo;
   assert.notEqual(version,previous,'each full-pool birth/merge must change the scratch version');previous=version;mutations.push(version);
   const found=w.mergeIndex.find(p,r);return {status:found?'PAIR':'NO_PAIR',slotA:w.mergeIndex.selectedA,slotB:w.mergeIndex.selectedB,errorCode:'OFFLINE_CPU_STAND_IN'};}}:null);
 try {
  const field=new WaterSandField(128,512),params=defaultP011ParticleParams();params.nmax=nmax;assert.equal(field.setParticleRoute(params).ok,true);field.setNaturalObservationRouteId('dual-field-particle-v1');assert.ok(field.particleRouteActive());
  const before=field.snapshot();const outcomes=[];
  for(let input=0;input<2;input++) {const h=new Float64Array(128),l=new Float64Array(128);h[64]=input===0?nmax*params.massPackage:0.25;
    const receipt=field.injectGestureSand({perColumnHeavy:h,perColumnLight:l,sourceId:17,sequence:input+1,inputTime:10+input,x:64.5,y:100,seed:13});assert.ok(receipt);assert.ok(Math.abs(receipt.admittedH-h[64])<1e-10);}
  for(let step=0;step<3;step++)outcomes.push(field.step(1/60));
  const after=field.snapshot(), image={snapshot:after,poolBytes:poolBytes(after.particleState),outcomes,events:field.getParticleEventTail(16384)};
  const normalized=JSON.parse(JSON.stringify(image,(k,v)=>k==='physicsTimeMs'?0:k==='runId'?'NORMALIZED_RUN_ID':v));
  if(route==='LEGACY_SCAN')references.set(nmax,normalized);else assert.deepEqual(normalized,references.get(nmax));
  if(route==='GPU'){assert.ok(calls>0);assert.ok(roles.has(1));}
  cases.push({nmax,route:route==='GPU'?'GPU_INTERFACE_CPU_STAND_IN':route,status:'PASS',selectorCalls:calls,roles:[...roles],poolSha256:digest(image.poolBytes.join('')),beforeLive:before.particleState.liveCount,afterLive:after.particleState.liveCount});
 }finally{selector.t159Disarm();}
}
// Counter carry and overflow are independent of small current production IDs.
const words=new meta.T159Words();words.lo=4294967295;words.hi=7;meta.t159Increment(words);assert.deepEqual([words.lo,words.hi],[0,8]);
words.lo=words.hi=4294967295;assert.throws(()=>meta.t159Increment(words),/EXHAUSTED/);
const high=meta.t159Split(9007199254740991);assert.deepEqual([high.lo,high.hi],[4294967295,2097151]);assert.throws(()=>meta.t159Split(9007199254740992));
console.log(JSON.stringify({revision:'T159_OFFLINE_TRANSACTION_V2',cases,passed:cases.length+3,failed:0,nativeWorkerExecution:'NOT_RUN',gpuExecution:'NOT_RUN',standIn:'INTERFACE_ONLY_NOT_GPU'},null,2));
