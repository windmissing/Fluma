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
const negativeControls=[]; // manager R9: retain old pair-refund differences
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
// D1-B rev1: reference field/budget storage, still no WaterSandField wiring.
const proto=require('../entry/src/main/ets/physics/P011ParticlePrototype.ets');
let ordinal=0;
function env(nmax=32, step=1, dt=1/60, scale=1) {
  const pool=new p.P011ParticleField(128,nmax), par={...params,nmax};
  const id={...identity,substepId:step,dt};
  const budgets=new proto.P011ParticlePrototypeBudget(128,dt,scale,step);
  const sink=new proto.P011ParticlePrototypeSink(pool,par,id,budgets);
  sink.bh.fill(50); sink.bl.fill(50); sink.waterTop.fill(460);
  return {pool,par,id,budgets,sink};
}
function requestB(e,kind,mass=0.1,slot=-1,c=60,target=-1) {
  const s=e.pool.snapshot(); const seq=++ordinal;
  return {requestKey:JSON.stringify([e.id.runId,e.id.substepId,kind,seq]),
    substepId:e.id.substepId,ordinal:seq,kind,particleId:slot<0?0:s.pId[slot],
    sourceId:slot<0?1:s.pSourceId[slot],phase:slot<0?0:s.pPhase[slot],
    sourceKind:slot<0?(kind==='lift'?1:0):s.pSource[slot],column:c,targetColumn:target,
    requested:mass,totalWeight:kind==='deposit'||kind==='crossing'?mass:0,
    predictedX:target<0?c+0.5:target+0.5,predictedY:200,predictedVx:0,predictedVy:-64,mergedIds:[]};
}
function doB(e,r) {const result=e.sink.apply(r,e.pool); assert.ok(result.committed,result.errorCode);
  near(result.requested,result.granted+result.budgetClipped+result.guardRejected+result.capacityClipped+result.poolClipped);
  return result;}
function near(a,b,tol=1e-9) {assert.ok(Math.abs(a-b)<=tol,`${a} != ${b}`);}
function inventory(e) {const t=e.pool.totals(); let h=t.heavy,l=t.light;
  for(let c=0;c<128;c++) {h+=e.sink.bh[c]+e.sink.mh[c]+e.sink.suspH[c]; l+=e.sink.bl[c]+e.sink.ml[c]+e.sink.suspL[c];}
  return [h,l];}
function allSlots(e) {const s=e.pool.snapshot(); return Array.from(s.pState.keys()).filter(i=>s.pState[i]);}
test('B T1 packages+tail, seeded birth clamp/phase speed, admitted=0 no particles',()=>{
  const e=env(); const r=requestB(e,'spawn',0.23); r.predictedY=1;
  const before=inventory(e); doB(e,r);
  const s=e.pool.snapshot(), slots=allSlots(e);
  assert.equal(slots.length,3); near(e.pool.totals().heavy,0.23);
  assert.deepEqual(slots.map(i=>s.pMass[i]),[0.1,0.1,0.03]);
  for(const i of slots) {assert.ok(s.pX[i]>60&&s.pX[i]<61); near(s.pY[i],100+params.contactEps); assert.equal(s.pVy[i],-64);}
  near(inventory(e)[0]-before[0],0.23); assert.equal(e.budgets.totalUsed,0);
  const births=e.sink.events.filter(x=>x.kind==='SPAWN'); assert.equal(births.length,3);
  assert.ok(births.every(x=>x.particleId>0));
  const count=e.pool.totals().count; doB(e,requestB(e,'spawn',0)); assert.equal(e.pool.totals().count,count);
});
test('B prepare/publish/restore/discard restore every SoA/free-stack/nextId',()=>{
  const e=env(); e.pool.beginStep(e.id,e.par,e.sink.view());
  const r=requestB(e,'spawn',0.23), before=e.pool.snapshot();
  const grant={requestKey:r.requestKey,grantedHeavy:0.23,grantedLight:0,disposition:'PARTICLE'};
  let patch=e.pool.prepare(r,grant); assert.ok(patch); assert.deepEqual(e.pool.snapshot(),before);
  e.pool.publish(patch); assert.equal(e.pool.totals().count,3);
  assert.throws(()=>e.pool.publish(patch),/ALREADY_PUBLISHED/);
  e.pool.restore(patch); assert.deepEqual(e.pool.snapshot(),before);
  assert.throws(()=>e.pool.restore(patch),/STALE/);
  patch=e.pool.prepare(r,grant); e.pool.discard(patch); assert.deepEqual(e.pool.snapshot(),before);
  assert.throws(()=>e.pool.publish(patch),/STALE/);
});
test('B atomic multiphase spawn cannot leave successful H when L lacks slots',()=>{
  const e=env(1); e.pool.beginStep(e.id,e.par,e.sink.view());
  const r=requestB(e,'lift',0.2); const before=e.pool.snapshot();
  const patch=e.pool.prepare(r,{requestKey:r.requestKey,grantedHeavy:0.1,grantedLight:0.1,disposition:'PARTICLE'});
  assert.equal(patch,null); assert.deepEqual(e.pool.snapshot(),before);
});
test('B T3 budget partial LAND retains original ID and real guard/refund identity',()=>{
  const e=env(); const slot=e.pool.allocate(seed(1,0,60.5,100));
  const before=inventory(e); const id=e.pool.snapshot().pId[slot];
  const r=doB(e,requestB(e,'deposit',1,slot)); near(r.granted,28/60);
  assert.equal(e.pool.snapshot().pId[slot],id); assert.equal(e.pool.snapshot().pState[slot],2);
  near(e.pool.totals().heavy,1-r.granted); near(e.sink.mh[60],r.granted);
  near(e.budgets.totalUsed,r.granted); assert.deepEqual(inventory(e),before);
  const event=e.sink.events.find(x=>x.kind==='LAND'); near(event.massAfter,event.remainder);
  assert.equal(event.remainderDisposition,'RETAIN_PARTICLE'); assert.equal(event.released,false);
});
test('B non-worsening guard rejects worsening lift, keeps mass in bed, refunds actual reservation',()=>{
  const e=env(); e.sink.bh[60]=0; e.sink.bl[60]=0.1;
  const before=inventory(e); const r=doB(e,requestB(e,'lift',0.1));
  assert.equal(r.granted,0); near(r.guardRejected,0.1); assert.equal(e.pool.totals().count,0);
  assert.deepEqual(inventory(e),before); near(e.budgets.totalUsed,0);
  assert.deepEqual(r.guard.affectedEdgeIds,[59,60]);
});
test('B clip formula local-before-scale, closed tickets immutable, overrefund/replay rejected',()=>{
  const e=env(32,1,1/60,0.5); e.budgets.localRemaining[60]=0.2;
  const r=requestB(e,'deposit',1); const t=e.budgets.reserve(r);
  near(t.localAllowed,0.2); near(t.reserved,0.1); // NOT min(r*scale,l)=0.2
  t.reserved=999; e.budgets.refund(t.ticketId,0.04); near(e.budgets.totalUsed,0.06);
  assert.throws(()=>e.budgets.refund(t.ticketId,0.061),/OVER_REFUND/);
  e.budgets.close(t.ticketId,0.06);
  assert.throws(()=>e.budgets.close(t.ticketId,0.06),/STALE/);
  assert.throws(()=>e.budgets.refund(t.ticketId,0.01),/STALE/);
  assert.throws(()=>e.budgets.reserve(r),/DUPLICATE/); assert.equal(e.budgets.bottomUsed,0);
  const other=env(32,2); assert.throws(()=>other.budgets.reserve(r),/INVALID_TICKET/);
});
test('B T4 capacity-first whole-particle wait; partial budget fully refunded; paid double local',()=>{
  const e=env(); const slot=e.pool.allocate(seed(0.1));
  const full=e.pool.snapshot(); e.sink.waterTop[61]=100;
  let r=doB(e,requestB(e,'crossing',0.1,slot,60,61)); assert.equal(r.capacityClipped,0.1);
  assert.equal(e.budgets.totalUsed,0); assert.equal(Math.floor(e.pool.snapshot().pX[slot]),60);
  e.sink.waterTop[61]=460; e.budgets.localRemaining[61]=0.05;
  r=doB(e,requestB(e,'crossing',0.1,slot,60,61)); assert.equal(r.budgetClipped,0.1);
  near(e.budgets.totalUsed,0); near(e.budgets.localUsed[60],0); near(e.budgets.localUsed[61],0);
  e.budgets.localRemaining[61]=28/60;
  r=doB(e,requestB(e,'crossing',0.1,slot,60,61)); near(r.granted,0.1);
  const s=e.pool.snapshot(); assert.equal(Math.floor(s.pX[slot]),61);
  near(s.colParticleMassH[60],0); near(s.colParticleMassH[61],0.1);
  near(e.budgets.localUsed[60],0.1); near(e.budgets.localUsed[61],0.1); near(e.budgets.totalUsed,0.1);
  near(e.pool.totals().heavy,full.pMass[slot]); assert.equal(e.budgets.bottomUsed,0);
});
test('B T2 mixed/light/heavy exposure -> airborne -> partial deposit closes inventory',()=>{
  const e=env(); e.sink.mh[60]=0.01; e.sink.ml[60]=0.01;
  e.sink.bh[60]-=0.02; const before=inventory(e);
  const r=doB(e,requestB(e,'lift',0.1)); near(r.granted,0.1);
  const s=e.pool.snapshot(); near(e.pool.totals().heavy,0.01); near(e.pool.totals().light,0.09);
  for(const i of allSlots(e)) {assert.equal(s.pSource[i],1); assert.ok(s.pVy[i]>0);}
  for(const i of allSlots(e)) doB(e,requestB(e,'deposit',e.pool.snapshot().pMass[i],i));
  near(inventory(e)[0],before[0]); near(inventory(e)[1],before[1]); assert.equal(e.pool.totals().count,0);
});
test('B T6 deterministic same-column/source merge preserves mass and x/y/v first moments',()=>{
  const e=env(); const a=e.pool.allocate({...seed(0.1,0,60.2,200,2,-4),sourceId:9});
  const b=e.pool.allocate({...seed(0.2,0,60.8,201,5,-7),sourceId:9});
  const before=e.pool.snapshot(); const r=requestB(e,'merge',0.1,a); r.mergedIds=[before.pId[a],before.pId[b]];
  const used=e.budgets.totalUsed; doB(e,r); const after=e.pool.snapshot();
  near(after.pMass[a],0.3); assert.equal(after.pState[b],0); assert.equal(after.pId[a],before.pId[a]);
  for(const key of ['pX','pY','pVx','pVy']) near(after[key][a]*after.pMass[a],before[key][a]*0.1+before[key][b]*0.2);
  near(e.budgets.totalUsed,used); near(after.colParticleMassH[60],before.colParticleMassH[60]);
  assert.equal(e.sink.events.filter(x=>x.kind==='MERGE').length,1);
});
test('B full pool: merge frees slot; incompatible source degrades lift once; T1 refuses',()=>{
  const e=env(2); e.pool.allocate(seed(0.1,0,60.2)); e.pool.allocate(seed(0.1,0,60.8));
  doB(e,requestB(e,'spawn',0.1)); assert.equal(e.pool.totals().count,2); near(e.pool.totals().heavy,0.3);
  assert.ok(e.sink.events.some(x=>x.kind==='MERGE'));
  const f=env(1); f.pool.allocate({...seed(0.1),sourceId:77}); const before=inventory(f);
  const used=f.budgets.totalUsed; doB(f,requestB(f,'lift',0.1));
  near(f.sink.suspL[60],0.1); near(f.budgets.totalUsed-used,0.1);
  assert.equal(f.pool.totals().count,1); near(inventory(f)[0],before[0]); near(inventory(f)[1],before[1]);
  assert.ok(f.sink.events.some(x=>x.flag==='PARTICLE_DEGRADED_TO_COLUMN'));
  const r=doB(f,requestB(f,'spawn',0.1)); assert.equal(r.granted,0); assert.equal(r.poolClipped,0.1);
});
test('B T5 explicit degrade transfers only pool->susp and does not debit a second budget',()=>{
  const e=env(); const slot=e.pool.allocate(seed(0.2)); const before=inventory(e);
  const r=doB(e,requestB(e,'degrade',0.2,slot)); assert.equal(r.granted,0.2);
  assert.equal(e.pool.totals().count,0); near(e.sink.suspH[60],0.2);
  assert.deepEqual(inventory(e),before); assert.equal(e.budgets.totalUsed,0);
  assert.equal(e.sink.events[0].flag,'PARTICLE_DEGRADED_TO_COLUMN');
});
test('B faults after pool+bed publication and journal overflow roll back full state',()=>{
  for(const mode of ['publish','overflow']) {
    const e=env(); const slot=e.pool.allocate(seed(0.2,0,60.5,100));
    if(mode==='publish') e.sink.failAfterPublish=true;
    else Object.defineProperty(e.sink,'eventLimit',{value:0});
    const ps=e.pool.snapshot(), fs=e.sink.snapshot(); const r=e.sink.apply(requestB(e,'deposit',0.2,slot),e.pool);
    assert.equal(r.committed,false); assert.match(r.errorCode,mode==='publish'?/INJECTED_FAILURE/:/OVERFLOW/);
    assert.deepEqual(e.pool.snapshot(),ps); assert.deepEqual(e.sink.snapshot(),fs);
  }
});
test('B shadow/live clones consume same double-collected plan and produce identical grants/state',()=>{
  const a=env(), b=env();
  for(const e of [a,b]) {e.pool.allocate(seed(0.1,0,60.5,100)); e.pool.allocate(seed(0.1,1,62.9,200,30,0));
    e.pool.allocate(seed(0.1,0,64.5,200)); e.pool.beginStep(e.id,e.par,e.sink.view());}
  const before=a.pool.snapshot();
  const pa=a.pool.collectParticlePlan(a.id,a.sink.view(),before,new p.P011ParticleJournal(a.id));
  const pb=b.pool.collectParticlePlan(b.id,b.sink.view(),b.pool.snapshot(),new p.P011ParticleJournal(b.id));
  p.assertParticlePlansEqual(pa,pb); assert.deepEqual(a.pool.snapshot(),before);
  const ra=a.pool.applyParticlePlan(pa,a.sink), rb=b.pool.applyParticlePlan(pb,b.sink);
  assert.deepEqual(ra,rb); assert.deepEqual(a.pool.snapshot(),b.pool.snapshot());
  assert.deepEqual(a.sink.snapshot(),b.sink.snapshot());
  assert.ok(a.pool.snapshot().pY[2]<before.pY[2]); // free flight is committed, not lost
  const tampered=structuredClone(pa); tampered.requests[0].requested+=1;
  assert.throws(()=>p.assertParticlePlansEqual(pa,tampered),/MISMATCH/);
  assert.throws(()=>a.pool.applyParticlePlan(tampered,a.sink),/MISMATCH/);
  const moved=structuredClone(pa); moved.motions[0].y+=1;
  assert.throws(()=>a.pool.applyParticlePlan(moved,a.sink),/MISMATCH/);
});
test('B exact tan30 partial guard refunds only debit and preserves legal adjacent edges',()=>{
  for(const kind of ['deposit','lift']) {
    const e=env(32,1,1), before=inventory(e);
    const slot=kind==='deposit'?e.pool.allocate(seed(1,0,60.5,100)):-1;
    const pre=inventory(e); const r=doB(e,requestB(e,kind,1,slot));
    near(r.granted,Math.tan(Math.PI/6)); near(r.guardRejected,1-r.granted);
    near(r.requested,r.granted+r.guardRejected); near(e.budgets.totalUsed,r.granted);
    near(e.budgets.localUsed[60],r.granted);
    assert.ok(Math.abs(e.sink.sand(60)-e.sink.sand(59))<=Math.tan(Math.PI/6)+1e-9);
    near(inventory(e)[0],pre[0]); near(inventory(e)[1],pre[1]);
  }
});
test('B merge refuses cross-column, cross-source and cross-phase without writes',()=>{
  for(const change of [{x:61.2},{sourceId:2},{sourceKind:1},{phase:1}]) {
    const e=env(); const a=e.pool.allocate(seed(0.1,0,60.2));
    const b=e.pool.allocate({...seed(0.1,0,60.8),...change});
    const s=e.pool.snapshot(), field=e.sink.snapshot();
    const r=requestB(e,'merge',0.1,a); r.mergedIds=[s.pId[a],s.pId[b]];
    assert.equal(e.sink.apply(r,e.pool).committed,false);
    assert.deepEqual(e.pool.snapshot(),s); assert.deepEqual(e.sink.snapshot(),field);
  }
});
test('B whole-plan rollback removes earlier successful LAND and permits exact retry',()=>{
  const e=env(); e.pool.allocate(seed(0.1,0,60.5,100)); e.pool.allocate(seed(0.1,0,61.5,100));
  e.pool.beginStep(e.id,e.par,e.sink.view());
  const a=e.pool.collectParticlePlan(e.id,e.sink.view(),e.pool.snapshot(),new p.P011ParticleJournal(e.id));
  const ps=e.pool.snapshot(), fs=e.sink.snapshot();
  const original=e.sink.apply.bind(e.sink); let calls=0;
  e.sink.apply=(r,pool)=>{e.sink.failAfterPublish=++calls===2; return original(r,pool);};
  const failed=e.sink.executeParticlePlan(a); assert.equal(failed.ok,false);
  assert.deepEqual(e.pool.snapshot(),ps); assert.deepEqual(e.sink.snapshot(),fs);
  assert.equal(e.sink.failedAttempts.length,1);
  e.sink.apply=original; e.sink.failAfterPublish=false;
  const retried=e.sink.executeParticlePlan(a); assert.equal(retried.ok,true,retried.errorCode);
  assert.equal(e.pool.totals().count,0); assert.equal(e.sink.events.length,2);
});
test('B representation journal preserves birth-before-merge ID trace order when pool fills',()=>{
  const e=env(2); e.par.mergeHeightRadius=4;
  doB(e,requestB(e,'spawn',0.5)); const alive=new Map();
  for(const event of e.sink.events) {
    if(event.kind==='SPAWN') {assert.ok(!alive.has(event.particleId)); alive.set(event.particleId,event.mass);}
    else if(event.kind==='MERGE') {
      const [a,b]=event.mergedIds; assert.ok(alive.has(a)&&alive.has(b));
      near(alive.get(a)+alive.get(b),event.massAfter); alive.set(a,event.massAfter); alive.delete(b);
    }
  }
  near([...alive.values()].reduce((a,b)=>a+b,0),0.5); near(e.pool.totals().heavy,0.5);
});
test('B shared 300/s cap and denominator inclusion never give free whole-particle transport',()=>{
  for(const scale of [1,5/11.4]) {
    const e=env(64,1,1/60,scale);
    for(let i=0;i<64;i++) e.pool.allocate(seed(0.1,0,2*i+0.9,200,30,0));
    e.pool.beginStep(e.id,e.par,e.sink.view());
    const before=e.pool.snapshot();
    const a=e.pool.collectParticlePlan(e.id,e.sink.view(),before,new p.P011ParticleJournal(e.id));
    near(a.totalRequested,6.4); assert.equal(a.requestCount,64);
    const execution=e.sink.executeParticlePlan(a); assert.equal(execution.ok,true,execution.errorCode);
    const used=execution.results.reduce((sum,r)=>sum+r.granted,0);
    near(used,e.budgets.totalUsed); assert.ok(used<=5+1e-9);
    const after=e.pool.snapshot();
    for(let i=0;i<64;i++) {
      const moved=Math.floor(after.pX[i])!==Math.floor(before.pX[i]);
      assert.equal(moved,execution.results[i].granted===0.1);
    }
    if(scale<1) {assert.equal(used,0); assert.ok(execution.results.every(r=>r.budgetClipped===0.1));}
  }
});
test('B seed-repeatability and rejected source identity/invalid cap are explicit',()=>{
  const states=[];
  for(const value of [42,42,1234567]) {
    const e=env(); e.id.seed=value; const r=requestB(e,'spawn',0.2); r.ordinal=7;
    doB(e,r); states.push(e.pool.snapshot());
  }
  assert.deepEqual(states[0],states[1]); assert.notDeepEqual(states[0].pX,states[2].pX);
  const e=env(); assert.throws(()=>p.validateParticleParams({...e.par,channelRateCap:1}),/INVALID_PARAMS/);
  const slot=e.pool.allocate(seed(0.1)); const before=e.pool.snapshot();
  const r=requestB(e,'degrade',0.1,slot); r.sourceId=99;
  assert.equal(e.sink.apply(r,e.pool).committed,false); assert.deepEqual(e.pool.snapshot(),before);
});
test('R9 reserve matches real clip grid; adapter refund vs rev1; legacy pair is known-incompatible',()=>{
  const {WaterSandField}=require('../entry/src/main/ets/physics/WaterSandField.ets');
  for(const scale of [0,0.13,0.5,1]) for(const local of [0,0.07,0.2,1])
  for(const total of [0,0.03,0.4,5]) for(const target of [-1,61]) {
    const e=env(16,1,1/60,scale), field=new WaterSandField(128,512);
    field.beginSubstepMotionBudget(1/60,false,scale);
    e.budgets.localRemaining[60]=local; e.budgets.totalRemaining=total;
    field.substepMotionBudgetRemaining[60]=local; field.totalMotionBudgetRemaining=total;
    const r=requestB(e,target<0?'deposit':'crossing',0.8,-1,60,target);
    const ticket=e.budgets.reserve(r); const actual=field.clipSubstepMotion(60,target,0.8);
    assert.equal(ticket.reserved,actual);
    assert.deepEqual(e.budgets.localRemaining,field.substepMotionBudgetRemaining);
    assert.equal(e.budgets.totalRemaining,field.totalMotionBudgetRemaining);
    const refund=actual*0.4; e.budgets.refund(ticket.ticketId,refund);
    field.materialGuardRefund(refund,target<0?[60]:[60,61],false);
    if(target<0) assert.deepEqual(e.budgets.localRemaining,field.substepMotionBudgetRemaining);
    else {
      near(e.budgets.localRemaining[60]-field.substepMotionBudgetRemaining[60],refund/2);
      near(e.budgets.localRemaining[61]-field.substepMotionBudgetRemaining[61],refund/2);
      near(e.budgets.localUsed[60],actual-refund);
      near(e.budgets.localUsed[61],actual-refund);
      if(refund>0) negativeControls.push({scale,local,total,reserved:actual,refund,
        adapterSourceRefund:refund,adapterTargetRefund:refund,
        legacySourceRefund:refund/2,legacyTargetRefund:refund/2,
        sourceDifference:refund/2,targetDifference:refund/2});
    }
    near(e.budgets.totalRemaining,field.totalMotionBudgetRemaining);
    near(e.budgets.totalUsed,field.totalMotionBudgetUsed);
  }
});
test('B 600s six-transaction sandbox: in-flight conservation/capacity/budgets every substep',()=>{
  const e=env(16); const expected=inventory(e); let countCross=0,countLand=0,countLift=0;
  let rng=211; const random=()=>((rng=(Math.imul(rng,1664525)+1013904223)>>>0)/2**32);
  const initialFields=e.sink.snapshot();
  for(let step=1;step<=36000;step++) {
    const prev=step===1?initialFields:e.sink.snapshot(); e.id={...identity,substepId:step};
    e.budgets=new proto.P011ParticlePrototypeBudget(128,1/60,1,step);
    e.sink=new proto.P011ParticlePrototypeSink(e.pool,e.par,e.id,e.budgets);
    for(const key of ['bh','bl','mh','ml','suspH','suspL','waterTop']) e.sink[key].set(prev[key]);
    if(step%60===1) {
      const r=requestB(e,'spawn',0.03,-1,60+Math.floor(random()*3));
      r.phase=step%120===1?0:1; r.predictedY=100.4;
      const result=doB(e,r); expected[r.phase]+=result.granted;
    }
    // Occasional explicit, budgeted adjacent transport before integration.
    if(step%300===2 && allSlots(e).length) {
      const slot=allSlots(e)[0], s=e.pool.snapshot(), c=Math.floor(s.pX[slot]);
      const r=requestB(e,'crossing',s.pMass[slot],slot,c,c===63?62:c+1);
      doB(e,r); countCross++;
    }
    e.pool.beginStep(e.id,e.par,e.sink.view());
    const snap=e.pool.snapshot(), fv=e.sink.view();
    const a=e.pool.collectParticlePlan(e.id,fv,snap,new p.P011ParticleJournal(e.id,64));
    const b=e.pool.collectParticlePlan(e.id,fv,snap,new p.P011ParticleJournal(e.id,64));
    p.assertParticlePlansEqual(a,b); assert.deepEqual(e.pool.snapshot(),snap);
    const applied=e.pool.applyParticlePlan(a,e.sink);
    assert.ok(applied.every(r=>r.committed),JSON.stringify(applied));
    countLand+=a.requests.filter(r=>r.kind==='deposit').length;
    if(step%300===0) {doB(e,requestB(e,'lift',0.02,-1,61)); countLift++;}
    if(step%900===5 && allSlots(e).length) {
      const slot=allSlots(e)[0], s=e.pool.snapshot(); doB(e,requestB(e,'degrade',s.pMass[slot],slot,Math.floor(s.pX[slot])));
    }
    e.sink.audit(); const actual=inventory(e);
    near(actual[0],expected[0]); near(actual[1],expected[1]);
    near(e.budgets.totalUsed+e.budgets.totalRemaining,300/60);
    assert.ok(e.budgets.totalUsed<=300/60+1e-9);
    for(let c=0;c<128;c++) near(e.budgets.localUsed[c]+e.budgets.localRemaining[c],28/60);
    assert.equal(e.budgets.bottomUsed,0);
    if(step%600===0) assertPool(e.pool);
  }
  assert.ok(countCross>0&&countLand>0&&countLift>0);
});
const failed=results.filter(r=>r.status==='FAIL');
console.log(JSON.stringify({suite:'p011_particle_D1_AB',passed:results.length-failed.length,
  failed:failed.length,results,negativeControls,pending:'D2 real WaterSandField integration is not claimed'},null,2));
if(failed.length) process.exitCode=1;
