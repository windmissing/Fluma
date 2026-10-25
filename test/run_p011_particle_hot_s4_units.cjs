'use strict';
const fs=require('fs'),assert=require('assert/strict');
const ts=require('C:/Program Files/Huawei/DevEco Studio/tools/hvigor/hvigor/node_modules/typescript');
require.extensions['.ets']=(m,f)=>m._compile(ts.transpileModule(fs.readFileSync(f,'utf8'),
  {compilerOptions:{target:ts.ScriptTarget.ES2020,module:ts.ModuleKind.CommonJS}}).outputText,f);
const hot=require('../entry/src/main/ets/physics/P011ParticleHot.ets');
const {exportParticleEvidence}=require('../entry/src/main/ets/physics/P011ParticleHotEvidence.ets');
const {makeSandbox,stepHotSandbox}=require('./p011_particle_hot_sandbox.cjs');
const keys=['waterTop','bh','bl','mh','ml','suspH','suspL','surface'];
function fixture(s,name){return s.fixtures.find(x=>x.name===name);}
let passed=0;function test(n,f){f();passed++;console.log('PASS '+n);}
test('S4 caller aliases, detached arrays and replaced event/results fail before canonical writes',()=>{
  for(const corrupt of [s=>s.field.bh=s.field.bl,s=>s.field.bh=s.work.frozenBh,
    s=>s.work.frozenPool.pMass=s.work.patchBefore.pMass,
    s=>structuredClone(s.work.planA.requested.buffer,{transfer:[s.work.planA.requested.buffer]})]){
    const s=makeSandbox(8),before=s.work.pool.snapshot();corrupt(s);
    assert.equal(hot.beginStepHot(s.work,s.input),3);assert.equal(s.work.stepEpoch,0);assert.equal(s.work.active,0);
    assert.deepEqual(s.work.pool.snapshot(),before);
  }
  for(const corrupt of [s=>s.work.results.granted=s.field.bh,s=>s.work.events.massBefore=s.work.patchAfter.pMass]){
    const s=makeSandbox(8);assert.equal(hot.beginStepHot(s.work,s.input),0);const epoch=s.work.stepEpoch;
    hot.collectParticlePlanInto(s.work,epoch,s.work.planA);hot.collectParticlePlanInto(s.work,epoch,s.work.planB);
    const before=s.work.pool.snapshot();corrupt(s);
    assert.equal(hot.applyParticlePlanInto(s.work,epoch,s.work.planA,s.sink,s.work.results),3);
    assert.deepEqual(s.work.pool.snapshot(),before);assert.equal(s.work.results.resultCount,0);
  }
});
test('S4 snapshot out rejects canonical/frozen/patch/field alias; owned copy stays independent',()=>{
  const s=makeSandbox(8),w=s.work,out=w.pool.snapshot(),before=w.pool.snapshot();
  assert.equal(w.copyStateToHot(out),0);
  for(const internal of [w.frozenPool,w.patchBefore,w.patchAfter,w.patchScratch]){
    assert.equal(w.copyStateToHot(internal),3);assert.deepEqual(w.pool.snapshot(),before);
  }
  out.pMass=w.pool.pMass;assert.equal(w.copyStateToHot(out),3);assert.deepEqual(w.pool.snapshot(),before);
});
test('S4 evidence page prevents overwrite; export pins field/particle/plan/result and rejects old leases',()=>{
  const s=makeSandbox(8),w=s.work;assert.equal(stepHotSandbox(s,fixture(s,'partial-land'),1),0);
  const token=w.committedPageToken,epoch=w.stepEpoch,granted=w.results.granted[0],pool=w.pool.snapshot();
  const predicted=w.planA.predictedY[0],bed=s.field.bh[20];
  w.planA.predictedY[0]=999;w.results.granted[0]=999;s.field.bh[20]=88;
  w.pool.allocate({mass:.1,x:21.5,y:80,vx:0,vy:0,phase:0,sourceKind:0,sourceId:99});
  assert.equal(hot.beginStepHot(w,s.input),9);assert.equal(w.stepEpoch,epoch);
  const evidence=exportParticleEvidence(w,token);assert.equal(evidence.plan.requests[0].predictedY,predicted);
  assert.equal(evidence.results[0].granted,granted);assert.deepEqual(evidence.pool,pool);assert.equal(evidence.field.bh[20],bed);
  assert.equal(evidence.events[0].eventLevelValidationOwnership,'ENDPOINT_CHECKER');assert.equal(w.pageBacklog,0);
  assert.throws(()=>exportParticleEvidence(w,token),/STALE_PAGE/);
  const live=w.pool.snapshot();evidence.pool.pMass.fill(0);evidence.field.bh.fill(0);assert.deepEqual(w.pool.snapshot(),live);
  s.input.attemptToken++;s.input.substepId++;assert.equal(hot.beginStepHot(w,s.input),0);
  assert.throws(()=>exportParticleEvidence(w,token),/STALE_PAGE/);
});
test('S4 journal overflow and post-publish fault restore full buckets/outs/sidecar and allow new attempt',()=>{
  for(const failure of ['journal','publish']){
    const s=makeSandbox(8,failure==='journal'?1:4096),w=s.work,f=fixture(s,'spawn-tail');
    f.failAfterPublish=failure==='publish'?1:0;
    const outs=JSON.stringify([s.sink.ticket,s.sink.refund,s.sink.guard]);
    assert.equal(stepHotSandbox(s,f,1),failure==='journal'?9:13);assert.deepEqual(w.pool.snapshot(),f.pool);
    for(const k of keys)assert.deepEqual(s.field[k],f.field[k]);
    assert.equal(w.active,0);assert.equal(w.results.resultCount,0);assert.equal(w.events.count,0);assert.equal(w.activePatchToken,0);
    assert.equal(s.sink.budget.totalUsed,0);assert.equal(s.sink.budget.denominator,0);
    assert.equal(JSON.stringify([s.sink.ticket,s.sink.refund,s.sink.guard]),outs);assert.equal(w.failureAttemptToken,1);
    if(failure==='publish'){f.failAfterPublish=0;assert.equal(stepHotSandbox(s,f,2),0);assert.equal(w.stepEpoch,2);}
  }
});
test('S4 full sandbox exercises all six transactions, merge/degrade, retained tail and partial budget',()=>{
  const s=makeSandbox(8),seen=new Set();
  for(let i=0;i<s.fixtures.length;i++){
    const f=s.fixtures[i];assert.equal(stepHotSandbox(s,f,i+1),0,f.name);
    const e=exportParticleEvidence(s.work,s.work.committedPageToken);for(const event of e.events)seen.add(event.kind);
    if(f.name==='positive-tail'){assert.ok(e.pool.pMass[0]>0&&e.pool.pMass[0]<1e-9);assert.equal(e.pool.pState[0],2);}
    if(f.name==='full-lift-degrade')assert.ok(e.events.some(x=>x.kind==='DEGRADE'));
  }
  assert.deepEqual([...seen].sort(),['SPAWN','LAND','CROSS_COLUMN','LIFT','DEGRADE','MERGE'].sort());
});
test('S4 replacing prepared after-image rejects publish before touching canonical pool',()=>{
  const s=makeSandbox(8),f=fixture(s,'partial-land'),w=s.work;
  w.pool.transferHotState(f.pool,true);for(const k of keys)s.field[k].set(f.field[k]);
  assert.equal(hot.beginStepHot(w,s.input),0);hot.collectParticlePlanInto(w,w.stepEpoch,w.planA);
  const token=hot.preparePatchHot(w,w.planA,0,.2,0,2);assert.ok(token>0);const before=w.pool.snapshot();
  const saved=w.patchAfter.pMass;w.patchAfter.pMass=w.patchBefore.pMass;
  assert.equal(hot.publishPatchHot(w,token),3);assert.deepEqual(w.pool.snapshot(),before);
  w.patchAfter.pMass=saved;assert.equal(hot.discardPatchHot(w,token),0);
});
console.log(JSON.stringify({suite:'run_p011_particle_hot_s4_units',passed,failed:0}));
