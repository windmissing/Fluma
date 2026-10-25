'use strict';
const fs=require('fs'),assert=require('assert/strict');
const ts=require('C:/Program Files/Huawei/DevEco Studio/tools/hvigor/hvigor/node_modules/typescript');
require.extensions['.ets']=(mod,file)=>mod._compile(ts.transpileModule(fs.readFileSync(file,'utf8'),
  {compilerOptions:{target:ts.ScriptTarget.ES2020,module:ts.ModuleKind.CommonJS}}).outputText,file);
const hot=require('../entry/src/main/ets/physics/P011ParticleHot.ets');
const cold=require('../entry/src/main/ets/physics/P011ParticleField.ets');
const params={massPackage:0.1,nmax:8,terminalSpeedHeavy:64,terminalSpeedLight:38,dragHeavy:2,dragLight:2,
  liftSpeedHeavy:8,liftSpeedLight:8,liftSpread:2,mergeColRadius:1,mergeHeightRadius:2,contactEps:1e-6,channelRateCap:0};
function setup(extra={}) {
  const cfg={cols:128,nmax:8,requestCapacity:64,motionCapacity:8,eventCapacity:64,lineageCapacity:128,params,...extra};
  const w=hot.configureHot(cfg),f={cols:128,capacity:512,sourceInjectedH:0,sourceInjectedL:0,sourceEpoch:0};
  for(const key of ['waterTop','bh','bl','mh','ml','suspH','suspL','surface'])f[key]=new Float64Array(128);
  f.waterTop.fill(400);f.bh.fill(20);f.surface.fill(20);
  const input={runToken:1,substepId:1,attemptToken:1,dt:1/60,effectiveTiltRad:0.2,seed:17,paramsRevision:1,inputRevision:1,field:f};
  const start=()=>{assert.equal(hot.beginStepHot(w,input),0);return w.stepEpoch;};
  return {w,f,input,start};
}
function seed(pool,extra={}) {return pool.allocate({mass:1,x:20.5,y:30,vx:2,vy:-3,phase:0,sourceKind:0,sourceId:1,...extra});}
const kinds={spawn:1,deposit:2,crossing:3,lift:4,degrade:5,merge:6};
function planFor(w,kind,extra={}) {
  const r={requestKey:'r1',substepId:1,ordinal:1,kind,particleId:1,sourceId:1,phase:0,sourceKind:kind==='lift'?1:0,
    column:20,targetColumn:kind==='crossing'?21:-1,requested:1,totalWeight:kind==='deposit'||kind==='crossing'?1:0,
    predictedX:kind==='crossing'?21.5:20.5,predictedY:30,predictedVx:2,predictedVy:-3,mergedIds:[],...extra};
  const p=w.planA;Object.assign(p,{ownerId:w.ownerId,runToken:w.runToken,stepEpoch:w.stepEpoch,substepId:w.substepId,
    attemptToken:w.attemptToken,paramsRevision:w.paramsRevision,inputRevision:w.inputRevision,seed:w.seed,dt:w.dt,
    effectiveTiltRad:w.effectiveTiltRad,requestCount:1,motionCount:0,totalRequested:r.totalWeight,valid:1,consumed:0});
  p.kind[0]=kinds[kind];for(const key of ['phase','sourceKind','column','targetColumn','particleId','sourceId','ordinal','requested','totalWeight',
    'predictedX','predictedY','predictedVx','predictedVy'])p[key][0]=r[key];
  p.substep[0]=r.substepId;p.mergeStart[0]=0;p.mergeCount[0]=r.mergedIds.length;p.lineage.set(r.mergedIds);
  return {p,r};
}
function coldPublish(w,f,r,gh,gl,disposition) {
  const pool=new cold.P011ParticleField(128,8);pool.restoreSnapshot(w.pool.snapshot());
  pool.beginStep({runId:'1',substepId:1,dt:1/60,effectiveTiltRad:0.2,seed:17,paramsRevision:1,inputRevision:1},params,
    {cols:128,capacity:512,effectiveTiltRad:0.2,sandTopAt:c=>f.bh[c]+f.bl[c]+f.mh[c]+f.ml[c],waterTopAt:c=>f.waterTop[c],
      suspAt:c=>f.suspH[c]+f.suspL[c],freeSpaceAt:()=>300});
  const patch=pool.prepare(r,{requestKey:r.requestKey,grantedHeavy:gh,grantedLight:gl,disposition});
  assert.ok(patch);const actions=pool.patchActions(patch),merges=pool.patchMerges(patch),births=pool.patchBirths(patch);
  pool.publish(patch);return {state:pool.snapshot(),actions,merges,births};
}
let passed=0;function test(name,fn){fn();passed++;console.log('PASS '+name);}
for(const [kind,gh,gl,disp,code] of [['spawn',0.2,0.05,'PARTICLE',1],['lift',0.1,0.15,'PARTICLE',1],
  ['deposit',0.3,0,'BED',2],['crossing',1,0,'MOVE',4],['degrade',1,0,'SUSP',3],['merge',0,0,'MERGE',5]]) {
  test('S2 '+kind+' patch matches cold complete SoA and action order',()=>{
    const {w,f,start}=setup();if(!['spawn','lift'].includes(kind))seed(w.pool);
    if(kind==='merge')seed(w.pool,{mass:0.7,x:20.7,y:31});
    start();const {p,r}=planFor(w,kind,kind==='spawn'||kind==='lift'?{particleId:0,requested:0.25}:
      kind==='merge'?{mergedIds:[1,2]}:{});
    const reference=coldPublish(w,f,r,gh,gl,disp),before=w.pool.snapshot();
    const token=hot.preparePatchHot(w,p,0,gh,gl,code);assert.ok(token>0);assert.deepEqual(w.pool.snapshot(),before);
    assert.equal(w.patchActions.count,reference.actions.length);
    for(let i=0;i<reference.actions.length;i++)assert.equal(w.patchActions.kind[i],reference.actions[i].kind==='merge'?6:1);
    assert.equal(hot.publishPatchHot(w,token),0);assert.deepEqual(w.pool.snapshot(),reference.state);
    assert.equal(hot.finishPatchHot(w,token),0);assert.equal(hot.finishPatchHot(w,token),hot.P011_HOT_STALE_HANDLE);
    assert.equal(hot.preparePatchHot(w,p,0,gh,gl,code),0);assert.equal(w.lastStatus,hot.P011_HOT_DUPLICATE_REQUEST);
    p.predictedVx[0]++;assert.equal(hot.preparePatchHot(w,p,0,gh,gl,code),0);assert.equal(w.lastStatus,hot.P011_HOT_IDENTITY_MISMATCH);
  });
}
test('S2 discard/restore/finish preserve primitive old tokens and reject ABA/cross-owner/busy',()=>{
  const {w,start}=setup();seed(w.pool);const epoch=start(),{p}=planFor(w,'deposit'),before=w.pool.snapshot();
  const t1=hot.preparePatchHot(w,p,0,0.3,0,2);assert.ok(t1>0);
  const images=[w.patchBefore.pMass.slice(),w.patchAfter.pMass.slice()];
  assert.equal(hot.preparePatchHot(w,p,0,0.1,0,2),0);assert.equal(w.lastStatus,hot.P011_HOT_PATCH_BUSY);
  assert.deepEqual([w.patchBefore.pMass,w.patchAfter.pMass],images);
  assert.equal(hot.discardPatchHot(w,t1),0);const t2=hot.preparePatchHot(w,p,0,0.4,0,2);assert.ok(t2>t1);
  assert.equal(hot.publishPatchHot(w,t1),hot.P011_HOT_STALE_HANDLE);assert.deepEqual(w.pool.snapshot(),before);
  assert.equal(hot.publishPatchHot(w,t2),0);const changed=w.pool.snapshot();
  assert.equal(hot.publishPatchHot(w,t2),hot.P011_HOT_INVALID_PATCH_STATE);assert.deepEqual(w.pool.snapshot(),changed);
  assert.equal(hot.discardPatchHot(w,t2),hot.P011_HOT_INVALID_PATCH_STATE);
  const other=setup();other.start();assert.equal(hot.publishPatchHot(other.w,t2),hot.P011_HOT_STALE_HANDLE);
  assert.equal(hot.restorePatchHot(w,t2),0);assert.deepEqual(w.pool.snapshot(),before);
  assert.equal(hot.restorePatchHot(w,t2),hot.P011_HOT_STALE_HANDLE);
  const t3=hot.preparePatchHot(w,p,0,0.3,0,2);assert.ok(t3>t2);assert.equal(hot.rollbackStepHot(w,epoch),0);
  assert.equal(hot.publishPatchHot(w,t3),hot.P011_HOT_STALE_HANDLE);assert.deepEqual(w.pool.snapshot(),before);
});
test('S2 positive sub-EPS tail retained; exact zero alone releases',()=>{
  const {w,start}=setup();seed(w.pool);start();const {p}=planFor(w,'deposit');
  const grant=1-1e-14,t=hot.preparePatchHot(w,p,0,grant,0,2);assert.ok(t>0);hot.publishPatchHot(w,t);
  let state=w.pool.snapshot();assert.equal(state.liveCount,1);assert.equal(state.pMass[0],1-grant);assert.equal(state.pState[0],2);
  hot.restorePatchHot(w,t);const t2=hot.preparePatchHot(w,p,0,1,0,2);hot.publishPatchHot(w,t2);
  state=w.pool.snapshot();assert.equal(state.liveCount,0);assert.equal(state.freeCount,8);
});
test('S2 pool-full merge during spawn preserves cold state and refuses unmatched source atomically',()=>{
  const {w,f,start}=setup();for(let i=0;i<8;i++)seed(w.pool,{mass:0.1,x:20.2+i*0.05,y:30});start();
  const {p,r}=planFor(w,'spawn',{particleId:0,requested:0.25});const ref=coldPublish(w,f,r,0.25,0,'PARTICLE');
  const t=hot.preparePatchHot(w,p,0,0.25,0,1);assert.ok(t>0);assert.ok(w.patchActions.count>3);
  hot.publishPatchHot(w,t);assert.deepEqual(w.pool.snapshot(),ref.state);hot.restorePatchHot(w,t);
  p.sourceId[0]=99;const before=w.pool.snapshot();const fail=hot.preparePatchHot(w,p,0,0.25,0,1);
  // Existing particles can merge first, so use incompatible phases/source IDs for complete exhaustion.
  if(fail>0)hot.discardPatchHot(w,fail);assert.deepEqual(w.pool.snapshot(),before);
});
test('S2 journal and lineage overflow never publish partial pool or ID changes',()=>{
  for(const limits of [{eventCapacity:1},{lineageCapacity:1}]) {
    const {w,start}=setup(limits);if(limits.lineageCapacity)for(let i=0;i<8;i++)seed(w.pool,{mass:0.1});
    start();const {p}=planFor(w,'spawn',{particleId:0,requested:0.25});const before=w.pool.snapshot();
    assert.equal(hot.preparePatchHot(w,p,0,0.25,0,1),0);
    assert.equal(w.lastStatus,limits.lineageCapacity?hot.P011_HOT_LINEAGE_OVERFLOW:hot.P011_HOT_JOURNAL_OVERFLOW);
    assert.deepEqual(w.pool.snapshot(),before);assert.equal(w.activePatchToken,0);
  }
});
test('S2 prepare pins request fields independently of later caller edits',()=>{
  const {w,start}=setup();seed(w.pool);start();const {p}=planFor(w,'deposit');
  const t=hot.preparePatchHot(w,p,0,0.3,0,2);assert.ok(t>0);p.ordinal[0]=4;
  assert.equal(hot.publishPatchHot(w,t),0);assert.equal(hot.finishPatchHot(w,t),0);
  assert.equal(w.seenEpoch[1],w.stepEpoch);assert.equal(w.seenEpoch[4],0);
});
test('S2 last safe lease is issued once; exhaustion refuses without recycling',()=>{
  const Module=require('module'),path=require('path');
  const filename=path.resolve('entry/src/main/ets/physics/P011ParticleHot.ets'),isolated=new Module(filename,module);
  isolated.filename=filename;isolated.paths=module.paths;
  const source=fs.readFileSync(filename,'utf8').replace('let nextLeaseToken = 1;',
    'let nextLeaseToken = Number.MAX_SAFE_INTEGER;');
  isolated._compile(ts.transpileModule(source,{compilerOptions:{target:ts.ScriptTarget.ES2020,module:ts.ModuleKind.CommonJS}}).outputText,filename);
  const api=isolated.exports,{f,input}=setup(),w=api.configureHot({cols:128,nmax:8,requestCapacity:64,motionCapacity:8,eventCapacity:64,lineageCapacity:128,params});
  seed(w.pool);assert.equal(api.beginStepHot(w,input),0);const {p}=planFor(w,'deposit'),before=w.pool.snapshot();
  const last=api.preparePatchHot(w,p,0,0.3,0,2);assert.equal(last,Number.MAX_SAFE_INTEGER);
  assert.equal(api.discardPatchHot(w,last),0);assert.equal(api.preparePatchHot(w,p,0,0.3,0,2),0);
  assert.equal(w.lastStatus,hot.P011_HOT_TOKEN_EXHAUSTED);assert.equal(w.tokenExhausted,1);
  assert.deepEqual(w.pool.snapshot(),before);assert.equal(api.publishPatchHot(w,last),hot.P011_HOT_STALE_HANDLE);
});
console.log(JSON.stringify({suite:'run_p011_particle_hot_patch_units',passed,failed:0}));
