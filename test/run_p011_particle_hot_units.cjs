'use strict';
const fs = require('fs');
const assert = require('assert/strict');
const ts = require('C:/Program Files/Huawei/DevEco Studio/tools/hvigor/hvigor/node_modules/typescript');
require.extensions['.ets'] = (mod, file) => mod._compile(ts.transpileModule(fs.readFileSync(file, 'utf8'),
  { compilerOptions: { target: ts.ScriptTarget.ES2020, module: ts.ModuleKind.CommonJS } }).outputText, file);
const hot = require('../entry/src/main/ets/physics/P011ParticleHot.ets');
const params = { massPackage: 0.1, nmax: 8, terminalSpeedHeavy: 64, terminalSpeedLight: 38,
  dragHeavy: 2, dragLight: 2, liftSpeedHeavy: 8, liftSpeedLight: 8, liftSpread: 2,
  mergeColRadius: 1, mergeHeightRadius: 2, contactEps: 1e-6, channelRateCap: 0 };
const config = { cols: 128, nmax: 8, requestCapacity: 64, motionCapacity: 8,
  eventCapacity: 64, lineageCapacity: 128, params };
function field() {
  return { cols: 128, capacity: 512, waterTop: new Float64Array(128), bh: new Float64Array(128),
    bl: new Float64Array(128), mh: new Float64Array(128), ml: new Float64Array(128),
    suspH: new Float64Array(128), suspL: new Float64Array(128), surface: new Float64Array(128),
    sourceInjectedH: 0, sourceInjectedL: 0, sourceEpoch: 0 };
}
let passed = 0;
function test(name, fn) { fn(); passed++; console.log(`PASS ${name}`); }
test('configure allocates fixed capacities and distinct plan buffers', () => {
  const w = hot.configureHot(config);
  assert.equal(hot.warmUpHot(w), hot.P011_HOT_OK);
  assert.equal(w.planA.requested.length, 64); assert.equal(w.planB.requested.length, 64);
  assert.equal(w.planA.requested.buffer === w.planB.requested.buffer, false);
  assert.equal(w.order.length, 8); assert.equal(w.eventKind.length, 64);
  assert.equal(w.pool.nmax, 8);
});
test('invalid config rejects before creating a workspace', () => {
  assert.throws(() => hot.configureHot({ ...config, motionCapacity: 7 }), /PARTICLE_HOT_INVALID_CONFIG/);
});
test('begin validates inputs before epoch or field state changes', () => {
  const w = hot.configureHot(config); const f = field();
  const result = hot.beginStepHot(w, { runToken: 1, substepId: 1, attemptToken: 1, dt: 0,
    effectiveTiltRad: 0, seed: 1, paramsRevision: 1, inputRevision: 1, field: f });
  assert.equal(result, hot.P011_HOT_INVALID_INPUT); assert.equal(w.stepEpoch, 0); assert.equal(w.active, 0);
});
test('step identities copy scalar inputs and keep all buffers stable across steps', () => {
  const w = hot.configureHot(config); const f = field(); const refs = [w.planA.requested.buffer,
    w.planB.requested.buffer, w.order.buffer, w.frozenBh.buffer, w.eventKind.buffer];
  const input = { runToken: 42, substepId: 1, attemptToken: 7, dt: 1 / 60,
    effectiveTiltRad: 0.2, seed: 12, paramsRevision: 3, inputRevision: 4, field: f };
  assert.equal(hot.beginStepHot(w, input), hot.P011_HOT_OK);
  const epoch1 = w.stepEpoch; input.runToken = 999; f.sourceInjectedH = 10;
  assert.equal(w.runToken, 42); assert.equal(w.sourceInjectedH, 0);
  assert.equal(hot.finishStepHot(w, epoch1), hot.P011_HOT_OK);
  input.runToken = 43; input.substepId = 2; input.attemptToken = 8;
  assert.equal(hot.beginStepHot(w, input), hot.P011_HOT_OK);
  assert.equal(w.stepEpoch, epoch1 + 1); assert.deepEqual([w.planA.requested.buffer, w.planB.requested.buffer,
    w.order.buffer, w.frozenBh.buffer, w.eventKind.buffer], refs);
});
test('stale epoch rejection has no workspace side effects', () => {
  const w = hot.configureHot(config); assert.equal(hot.beginStepHot(w, { runToken: 1, substepId: 1,
    attemptToken: 1, dt: 1 / 60, effectiveTiltRad: 0, seed: 1, paramsRevision: 1,
    inputRevision: 1, field: field() }), hot.P011_HOT_OK);
  const before = [w.active, w.stepEpoch, w.rollbackCount];
  assert.equal(hot.finishStepHot(w, w.stepEpoch - 1), hot.P011_HOT_STALE_HANDLE);
  assert.deepEqual([w.active, w.stepEpoch, w.rollbackCount], before);
});
test('numeric dual collect is deterministic and preserves distinct plan buffers', () => {
  const w = hot.configureHot(config); const f = field();
  f.bh.fill(200);
  w.pool.allocate({ mass: 0.2, x: 20.5, y: 80, vx: 0, vy: 0, phase: 0, sourceKind: 0, sourceId: 1 });
  w.pool.allocate({ mass: 0.3, x: 21.5, y: 120, vx: 0, vy: 0, phase: 1, sourceKind: 0, sourceId: 2 });
  const input = { runToken: 2, substepId: 4, attemptToken: 5, dt: 1 / 60,
    effectiveTiltRad: 0, seed: 3, paramsRevision: 1, inputRevision: 1, field: f };
  assert.equal(hot.beginStepHot(w, input), hot.P011_HOT_OK);
  assert.equal(hot.collectParticlePlanInto(w, w.stepEpoch, w.planA), hot.P011_HOT_OK);
  assert.equal(hot.collectParticlePlanInto(w, w.stepEpoch, w.planB), hot.P011_HOT_OK);
  assert.equal(hot.compareParticlePlansHot(w.planA, w.planB), hot.P011_HOT_OK);
  assert.notEqual(w.planA.requested.buffer, w.planB.requested.buffer);
  assert.equal(w.pool.snapshot().liveCount, 2);
  assert.equal(w.planA.requestCount, 2);
  w.planB.predictedX[0] += 0.001;
  assert.equal(hot.compareParticlePlansHot(w.planA, w.planB), hot.P011_HOT_PLAN_MISMATCH);
});
test('request capacity overflow rejects before publishing a valid plan', () => {
  const tiny = hot.configureHot({ ...config, requestCapacity: 1 }); const f = field();
  f.bh.fill(200);
  tiny.pool.allocate({ mass: 0.2, x: 20.5, y: 80, vx: 0, vy: 0, phase: 0, sourceKind: 0, sourceId: 1 });
  tiny.pool.allocate({ mass: 0.3, x: 21.5, y: 120, vx: 0, vy: 0, phase: 1, sourceKind: 0, sourceId: 2 });
  assert.equal(hot.beginStepHot(tiny, { runToken: 2, substepId: 4, attemptToken: 5, dt: 1 / 60,
    effectiveTiltRad: 0, seed: 3, paramsRevision: 1, inputRevision: 1, field: f }), hot.P011_HOT_OK);
  assert.equal(hot.collectParticlePlanInto(tiny, tiny.stepEpoch, tiny.planA), hot.P011_HOT_PLAN_OVERFLOW);
  assert.equal(tiny.planA.valid, 0);
});
const legacy = require('../entry/src/main/ets/physics/P011ParticleField.ets');
function inputFor(f) { return { runToken: 2, substepId: 4, attemptToken: 5, dt: 1/60,
  effectiveTiltRad: 0.3, seed: 3, paramsRevision: 1, inputRevision: 1, field: f }; }
function reference(w, f, input) {
  const identity={runId:'2',substepId:input.substepId,dt:input.dt,effectiveTiltRad:input.effectiveTiltRad,
    seed:input.seed,paramsRevision:input.paramsRevision,inputRevision:input.inputRevision};
  const view={cols:128,capacity:512,effectiveTiltRad:input.effectiveTiltRad,
    sandTopAt:c=>f.bh[c]+f.bl[c]+f.mh[c]+f.ml[c],waterTopAt:c=>f.waterTop[c],
    suspAt:c=>f.suspH[c]+f.suspL[c],freeSpaceAt:c=>512-f.surface[c]};
  return legacy.collectParticlePlan(identity,view,w.pool.snapshot(),params,new legacy.P011ParticleJournal(identity,64));
}
test('S1 all mixed-phase candidates and motions equal cold reference for 64 frozen scenes',()=>{
  for(let scene=0;scene<64;scene++) {
    const w=hot.configureHot(config), f=field(); f.waterTop.fill(400);
    f.bh.fill(10);f.bl.fill(70);f.mh.fill(30);f.ml.fill(90);f.surface.fill(200);
    for(let i=0;i<8;i++) {
      const c=20+i,kind=(scene+i)%4;
      w.pool.allocate({mass:0.1*(i+1),x:c+(kind===2?0.99:0.5),y:kind===0?199:kind===1?200.000001:300,
        vx:kind===2?120:0,vy:kind===1?0:kind===3?-1:0,phase:i%2,sourceKind:i%2,sourceId:i+1});
    }
    const input=inputFor(f), old=reference(w,f,input), before=w.pool.snapshot();
    assert.equal(hot.beginStepHot(w,input),0);const epoch=w.stepEpoch;
    assert.equal(hot.collectParticlePlanInto(w,epoch,w.planA),0);
    f.bh.fill(0);f.bl.fill(0);f.waterTop.fill(0);
    assert.equal(hot.collectParticlePlanInto(w,epoch,w.planB),0);
    assert.equal(hot.compareParticlePlansHot(w.planA,w.planB),0);
    assert.deepEqual(w.pool.snapshot(),before);
    assert.equal(w.planA.requestCount,old.requestCount);assert.equal(w.planA.totalRequested,old.totalRequested);
    for(let i=0;i<old.requestCount;i++) {
      const r=old.requests[i],p=w.planA;
      assert.equal(p.kind[i],r.kind==='deposit'?2:3);assert.equal(p.ordinal[i],i+1);
      for(const key of ['phase','sourceKind','column','targetColumn','particleId','sourceId','requested',
        'totalWeight','predictedX','predictedY','predictedVx','predictedVy'])assert.equal(p[key][i],r[key],key);
      assert.equal(p.mergeCount[i],0);
    }
    assert.equal(w.planA.motionCount,old.motions.length);
    for(let i=0;i<old.motions.length;i++)for(const [key,source] of Object.entries({motionParticleId:'particleId',
      motionColumn:'column',motionX:'x',motionY:'y',motionVx:'vx',motionVy:'vy'}))assert.equal(w.planA[key][i],old.motions[i][source]);
  }
});
test('S1 compares lineage and rejects equal NaNs, aliased plans and forged capacities',()=>{
  const w=hot.configureHot(config),f=field();f.bh.fill(200);
  w.pool.allocate({mass:0.2,x:20.5,y:80,vx:0,vy:0,phase:0,sourceKind:0,sourceId:1});
  assert.equal(hot.beginStepHot(w,inputFor(f)),0);
  assert.equal(hot.collectParticlePlanInto(w,w.stepEpoch,w.planA),0);
  assert.equal(hot.collectParticlePlanInto(w,w.stepEpoch,w.planB),0);
  assert.equal(hot.compareParticlePlansHot(w.planA,w.planA),hot.P011_HOT_PLAN_MISMATCH);
  w.planA.mergeCount[0]=w.planB.mergeCount[0]=2;w.planA.lineage[0]=w.planB.lineage[0]=1;
  w.planA.lineage[1]=2;w.planB.lineage[1]=3;
  assert.equal(hot.compareParticlePlansHot(w.planA,w.planB),hot.P011_HOT_PLAN_MISMATCH);
  w.planB.lineage[1]=2;assert.equal(hot.compareParticlePlansHot(w.planA,w.planB),0);
  w.planA.predictedVx[0]=w.planB.predictedVx[0]=NaN;
  assert.equal(hot.compareParticlePlansHot(w.planA,w.planB),hot.P011_HOT_PLAN_MISMATCH);
  w.planA.requested=w.planB.requested;
  assert.equal(hot.collectParticlePlanInto(w,w.stepEpoch,w.planA),hot.P011_HOT_INVALID_BUFFER);
});
test('S1 exact Q/N boundaries, stale epoch and overflow leave canonical unchanged',()=>{
  const w=hot.configureHot({...config,requestCapacity:8}), f=field();f.waterTop.fill(400);
  for(let i=0;i<8;i++)w.pool.allocate({mass:1,x:20.5+i,y:300,vx:0,vy:0,phase:0,sourceKind:0,sourceId:1});
  assert.equal(hot.beginStepHot(w,inputFor(f)),0);const before=w.pool.snapshot();
  assert.equal(hot.collectParticlePlanInto(w,w.stepEpoch,w.planA),0);
  assert.equal(w.planA.motionCount,8);assert.equal(w.planA.requestCount,0);
  w.motionCapacity=7;
  assert.equal(hot.collectParticlePlanInto(w,w.stepEpoch,w.planB),hot.P011_HOT_MOTION_OVERFLOW);
  assert.equal(w.planB.valid,0);assert.deepEqual(w.pool.snapshot(),before);
  assert.equal(hot.collectParticlePlanInto(w,w.stepEpoch-1,w.planA),hot.P011_HOT_STALE_HANDLE);
  w.frozenBh.fill(400);w.motionCapacity=8;
  assert.equal(hot.collectParticlePlanInto(w,w.stepEpoch,w.planA),0);assert.equal(w.planA.requestCount,8);
  w.requestCapacity=7;
  assert.equal(hot.collectParticlePlanInto(w,w.stepEpoch,w.planB),hot.P011_HOT_PLAN_OVERFLOW);
  assert.deepEqual(w.pool.snapshot(),before);
});
test('S1 duplicate particle ids and invalid state fail without canonical writes',()=>{
  const w=hot.configureHot(config),f=field();f.waterTop.fill(400);
  for(let i=0;i<2;i++)w.pool.allocate({mass:1,x:20.5+i,y:300,vx:0,vy:0,phase:0,sourceKind:0,sourceId:1});
  assert.equal(hot.beginStepHot(w,inputFor(f)),0);w.frozenPool.pId[1]=w.frozenPool.pId[0];
  const before=w.pool.snapshot();assert.equal(hot.collectParticlePlanInto(w,w.stepEpoch,w.planA),hot.P011_HOT_INVALID_INPUT);
  assert.deepEqual(w.pool.snapshot(),before);
});
console.log(JSON.stringify({ suite: 'run_p011_particle_hot_units', passed, failed: 0 }));
