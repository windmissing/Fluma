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
console.log(JSON.stringify({ suite: 'run_p011_particle_hot_units', passed, failed: 0 }));
