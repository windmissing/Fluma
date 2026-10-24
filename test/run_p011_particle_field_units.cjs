// P-011 D1-A: actual ETS, isolated Node channel; B/field E2E not claimed.
'use strict';
const fs = require('fs');
const assert = require('assert/strict');
const ts = require('C:/Program Files/Huawei/DevEco Studio/tools/hvigor/hvigor/node_modules/typescript');
require.extensions['.ets'] = (mod, filename) => mod._compile(ts.transpileModule(
  fs.readFileSync(filename, 'utf8'), {compilerOptions: {
    target: ts.ScriptTarget.ES2020, module: ts.ModuleKind.CommonJS
  }}).outputText, filename);
const p = require('../entry/src/main/ets/physics/P011ParticleField.ets');
const params = {massPackage:0.1, nmax:512, terminalSpeedHeavy:64,
  terminalSpeedLight:38, dragHeavy:2, dragLight:2, liftSpeedHeavy:8,
  liftSpeedLight:8, liftSpread:2, mergeColRadius:1, mergeHeightRadius:2,
  contactEps:1e-6, channelRateCap:0};
const identity = {runId:'D1-A', substepId:1, dt:1/60, effectiveTiltRad:0,
  seed:42, paramsRevision:1, inputRevision:1};
const view = Object.freeze({cols:128, capacity:512, effectiveTiltRad:0,
  sandTopAt:()=>100, waterTopAt:()=>460, suspAt:()=>0, freeSpaceAt:()=>360});
function seed(mass=0.1, phase=0, x=60.5, y=200, vx=0, vy=-64) {
  return {mass, phase, x, y, vx, vy, sourceKind:0, sourceId:1};
}
const results=[];
function test(name, fn) {try {fn(); results.push({name,status:'PASS'});}
catch(e) {results.push({name,status:'FAIL',error:e.stack});}}
function plan(pool, id=identity, limit=4096) {
  return p.collectParticlePlan(id, view, pool.snapshot(), params,
    new p.P011ParticleJournal(id, limit));
}
function assertPool(pool) {
  const s=pool.snapshot(); const h=new Float64Array(128), l=new Float64Array(128);
  let count=0; const liveSlots=new Set(), ids=new Set();
  for(let i=0;i<s.nmax;i++) if(s.pState[i]) {
    count++; liveSlots.add(i); assert.ok(!ids.has(s.pId[i])); ids.add(s.pId[i]);
    assert.ok(s.pMass[i]>0); assert.ok(Number.isFinite(s.pY[i]));
    (s.pPhase[i]===0?h:l)[Math.floor(s.pX[i])]+=s.pMass[i];
  }
  assert.equal(count,s.liveCount); assert.equal(count+s.freeCount,s.nmax);
  const free=new Set();
  for(let i=0;i<s.freeCount;i++) {
    assert.ok(!liveSlots.has(s.freeSlots[i])); assert.ok(!free.has(s.freeSlots[i]));
    free.add(s.freeSlots[i]);
  }
  for(let c=0;c<128;c++) {
    assert.ok(Math.abs(h[c]-s.colParticleMassH[c])<1e-12);
    assert.ok(Math.abs(l[c]-s.colParticleMassL[c])<1e-12);
  }
}
test('route exact two-condition predicate; default off; no field dependency',()=>{
  assert.equal(p.particleRouteEnabled(null,null),false);
  assert.equal(p.particleRouteEnabled(p.P011_PARTICLE_ROUTE_ID,null),false);
  assert.equal(p.particleRouteEnabled('other',params),false);
  assert.equal(p.particleRouteEnabled(p.P011_PARTICLE_ROUTE_ID,params),true);
  const source=fs.readFileSync(require.resolve('../entry/src/main/ets/physics/P011ParticleField.ets'),'utf8');
  assert.ok(!/^import\s/m.test(source));
});
test('fixed pool fills, refuses without mutation, reuses free slot with new ID',()=>{
  const pool=new p.P011ParticleField(128,2);
  const a=pool.allocate(seed()), b=pool.allocate(seed(0.2,1));
  assert.equal(a,0); assert.equal(b,1); const full=pool.snapshot();
  assert.equal(pool.allocate(seed()),-1); assert.deepEqual(pool.snapshot(),full);
  pool.retainAfterDeposit(a,0.1,100,1e-6);
  assert.equal(pool.allocate(seed()),a); assert.equal(pool.snapshot().pId[a],3);
  assertPool(pool);
});
test('CONS-03 partial/zero/full grants retain identity and exact tiny remainder',()=>{
  const pool=new p.P011ParticleField(128,2); const slot=pool.allocate(seed(1));
  const before=pool.snapshot(); let bed=0;
  const apply=(g)=>{const r=pool.retainAfterDeposit(slot,g,100+bed+g,1e-6); bed+=g;
    assert.equal(bed+pool.totals().heavy,1); return r;};
  assert.equal(apply(0),1); assert.equal(pool.snapshot().pState[slot],2);
  assert.equal(apply(1-2**-40),2**-40);
  let s=pool.snapshot(); assert.equal(s.pId[slot],before.pId[slot]);
  assert.equal(s.pSourceId[slot],before.pSourceId[slot]); assert.equal(s.liveCount,1);
  assert.equal(s.pVy[slot],0); assert.equal(s.pMass[slot],2**-40);
  assert.equal(apply(2**-40),0); assert.equal(pool.totals().count,0);
  const released=pool.snapshot(); assert.throws(()=>apply(0),/EMPTY_SLOT/);
  assert.deepEqual(pool.snapshot(),released); assertPool(pool);
});
test('invalid seeds/grants reject before writes',()=>{
  const pool=new p.P011ParticleField(128,2);
  for(const change of [{mass:NaN},{x:128},{phase:2},{sourceId:0},{sourceId:2**32}]) {
    const before=pool.snapshot(); assert.throws(()=>pool.allocate({...seed(),...change}));
    assert.deepEqual(pool.snapshot(),before);
  }
  const slot=pool.allocate(seed()); const before=pool.snapshot();
  for(const g of [-1,NaN,0.2]) assert.throws(()=>pool.retainAfterDeposit(slot,g,100,1e-6));
  assert.deepEqual(pool.snapshot(),before);
});
test('reset preserves ID monotonicity; ID exhaustion never wraps',()=>{
  const pool=new p.P011ParticleField(128,2); pool.allocate(seed()); pool.reset();
  assert.equal(pool.allocate(seed()),0); assert.equal(pool.snapshot().pId[0],2);
  pool.nextId=2**32-1; pool.allocate(seed()); pool.retainAfterDeposit(0,0.1,100,1e-6);
  const before=pool.snapshot(); assert.throws(()=>pool.allocate(seed()),/ID_EXHAUSTED/);
  assert.deepEqual(pool.snapshot(),before); assertPool(pool);
});
test('snapshots own complete independent SoA/free stack/cache copies',()=>{
  const pool=new p.P011ParticleField(128,2); pool.allocate(seed());
  const frozen=pool.snapshot(); frozen.pMass[0]=999; frozen.freeSlots[0]=0;
  assert.equal(pool.snapshot().pMass[0],0.1); assertPool(pool);
});
test('double collect is deterministic and has zero live/snapshot/view writes',()=>{
  const pool=new p.P011ParticleField(128,8);
  pool.allocate(seed(0.1,0,60.5,100.1));
  pool.allocate(seed(0.2,1,62.9,300,400,0));
  pool.allocate(seed(0.3,0,64.5,300));
  const before=pool.snapshot(); const frozen=pool.snapshot();
  const frozenBefore=structuredClone(frozen);
  const a=p.collectParticlePlan(identity,view,frozen,params,new p.P011ParticleJournal(identity));
  const b=p.collectParticlePlan(identity,view,frozen,params,new p.P011ParticleJournal(identity));
  assert.deepEqual(a,b); assert.deepEqual(frozen,frozenBefore); assert.deepEqual(pool.snapshot(),before);
  assert.equal(a.requestCount,2); assert.equal(a.requests[0].kind,'deposit');
  assert.equal(a.requests[1].kind,'crossing'); assert.equal(a.totalRequested,0.1+0.2);
  assert.ok(Math.abs(a.requests[1].targetColumn-a.requests[1].column)===1);
});
test('resting mass retries explicitly even above bed; no same-step crossing',()=>{
  const pool=new p.P011ParticleField(128,2); const slot=pool.allocate(seed());
  pool.retainAfterDeposit(slot,0,100,1e-6);
  const a=plan(pool); assert.equal(a.requestCount,1);
  assert.equal(a.requests[0].kind,'deposit'); assert.equal(a.requests[0].requested,0.1);
});
test('journal refuses overflow/duplicates and never returns truncated success',()=>{
  const pool=new p.P011ParticleField(128,2); pool.allocate(seed(0.1,0,60.5,100));
  const r=plan(pool).requests[0]; const journal=new p.P011ParticleJournal(identity,1);
  journal.record(r); assert.throws(()=>journal.record({...r,requestKey:'next'}),/OVERFLOW/);
  assert.throws(()=>journal.finish(),/CLOSED/);
  const dup=new p.P011ParticleJournal(identity); dup.record(r);
  assert.throws(()=>dup.record(r),/DUPLICATE/); assert.throws(()=>dup.finish(),/CLOSED/);
});
test('journal copies recorded requests and identity rather than borrowing writer state',()=>{
  const pool=new p.P011ParticleField(128,2); pool.allocate(seed(0.1,0,60.5,100));
  const r=plan(pool).requests[0], original=structuredClone(r);
  const id={...identity}; const journal=new p.P011ParticleJournal(id);
  journal.record(r); r.requested=999; r.mergedIds.push(999); id.seed=999;
  const a=journal.finish(); assert.deepEqual(a.requests[0],original);
  assert.equal(a.identity.seed,42); assert.equal(a.totalRequested,0.1);
  assert.throws(()=>journal.record(original),/CLOSED/);
});
test('stub apply preserves requested=granted+all clipping; no hidden second deposit',()=>{
  const pool=new p.P011ParticleField(128,2); const slot=pool.allocate(seed(1,0,60.5,100));
  let bed=0; const a=plan(pool);
  const results=p.applyParticlePlanToStub(a,{apply:r=>{
    const g=0.25; const remainder=pool.retainAfterDeposit(slot,g,100+g,1e-6); bed+=g;
    return {requestKey:r.requestKey,committed:true,errorCode:null,requested:r.requested,
      granted:g,remainder,budgetClipped:0.5,guardRejected:0.25,capacityClipped:0,poolClipped:0,
      guard:null,eventBegin:0,eventCount:1};
  }});
  const r=results[0]; assert.equal(r.requested,r.granted+r.budgetClipped+r.guardRejected+r.capacityClipped+r.poolClipped);
  assert.equal(bed+pool.totals().heavy,1); assert.equal(r.remainder,0.75);
  assert.equal(pool.snapshot().pState[slot],2); assertPool(pool);
});
test('600s-equivalent randomized pool transfers conserve both phases to 1e-9',()=>{
  const pool=new p.P011ParticleField(128,512); let rng=11;
  const random=()=>((rng=(Math.imul(rng,1664525)+1013904223)>>>0)/2**32);
  const expected=[0,0], bed=[0,0];
  for(let step=0;step<36000;step++) {
    const phase=step%2, mass=0.001+random()*0.05;
    const slot=pool.allocate(seed(mass,phase,Math.floor(random()*128)+0.5));
    if(slot>=0) {
      expected[phase]+=mass;
      const g=mass*random(); pool.retainAfterDeposit(slot,g,100,1e-6); bed[phase]+=g;
      const rem=mass-g; pool.retainAfterDeposit(slot,rem,100,1e-6); bed[phase]+=rem;
    }
    const totals=pool.totals();
    assert.ok(Math.abs(bed[0]+totals.heavy-expected[0])<1e-9);
    assert.ok(Math.abs(bed[1]+totals.light-expected[1])<1e-9);
    if(step%600===0) assertPool(pool);
  }
  assertPool(pool);
});
const failed=results.filter(r=>r.status==='FAIL');
console.log(JSON.stringify({suite:'p011_particle_D1_A',passed:results.length-failed.length,
  failed:failed.length,results,pending:'D1-B atomic sink/six transactions and real-field integration are not claimed'},null,2));
if(failed.length) process.exitCode=1;
