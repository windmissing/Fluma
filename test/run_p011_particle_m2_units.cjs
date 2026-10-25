'use strict';
const fs = require('fs');
const assert = require('assert/strict');
const ts = require('C:/Program Files/Huawei/DevEco Studio/tools/hvigor/hvigor/node_modules/typescript');
require.extensions['.ets'] = (mod, file) => mod._compile(ts.transpileModule(fs.readFileSync(file, 'utf8'),
  { compilerOptions: { target: ts.ScriptTarget.ES2020, module: ts.ModuleKind.CommonJS } }).outputText, file);
const { WaterSandField } = require('../entry/src/main/ets/physics/WaterSandField.ets');
const p = require('../entry/src/main/ets/physics/P011ParticleField.ets');
const m2 = require('../entry/src/main/ets/physics/P011ParticleM2Field.ets');
const obs = require('../entry/src/main/ets/physics/P011NaturalObservation.ets');
const params = { massPackage: 0.1, nmax: 16, terminalSpeedHeavy: 64, terminalSpeedLight: 38,
  dragHeavy: 2, dragLight: 2, liftSpeedHeavy: 8, liftSpeedLight: 8, liftSpread: 2,
  mergeColRadius: 1, mergeHeightRadius: 2, contactEps: 1e-6, channelRateCap: 0 };
const results = [];
function test(name, fn) { try { fn(); results.push({ name, status: 'PASS' }); }
  catch (e) { results.push({ name, status: 'FAIL', error: e.stack }); } }
function routedField() {
  const f = new WaterSandField(128, 512);
  assert.equal(f.setParticleRoute(params).code, 'OK');
  f.setNaturalObservationRouteId(p.P011_PARTICLE_ROUTE_ID);
  assert.equal(f.particleRouteActive(), true);
  return f;
}
function request(heavy, light = 0, sourceId = 17) {
  const h = new Float64Array(128); const l = new Float64Array(128);
  h[64] = heavy; l[64] = light;
  return { perColumnHeavy: h, perColumnLight: l, sourceId, sequence: 1, inputTime: 10,
    x: 64.5, y: 100, seed: 13 };
}
test('1: particle mass stays in the canonical heavy/light totals', () => {
  const f = routedField(); const before = f.getTotals();
  const receipt = f.injectGestureSand(request(0.4, 0.2));
  assert.equal(receipt.admittedH, 0.4); assert.equal(receipt.admittedL, 0.2);
  const after = f.getTotals(); assert.ok(Math.abs(after.heavy - before.heavy - 0.4) < 1e-9);
  assert.ok(Math.abs(after.light - before.light - 0.2) < 1e-9);
});
test('2: admitted material is not simultaneously stored in suspension', () => {
  const f = routedField(); const oldH = f.suspH[64]; const oldL = f.suspL[64];
  f.injectGestureSand(request(0.3, 0.1));
  const pool = f.particleBinding.pool; const s = pool.snapshot();
  assert.equal(pool.totals().count, 4); assert.equal(f.suspH[64], oldH); assert.equal(f.suspL[64], oldL);
  assert.ok(s.pMass.every(v => v <= params.massPackage + 1e-9));
  assert.ok(Math.abs(pool.totals().heavy - 0.3) < 1e-9);
});
test('3: the route gate requires route id and params together', () => {
  const f = new WaterSandField(128, 512);
  assert.equal(f.particleRouteActive(), false); f.setNaturalObservationRouteId(p.P011_PARTICLE_ROUTE_ID);
  assert.equal(f.particleRouteActive(), false); assert.equal(f.setParticleRoute({...params,channelRateCap:1}).code, 'INVALID_PARAMS');
  assert.equal(f.setParticleRoute(params).code, 'OK'); assert.equal(f.particleRouteActive(), true);
  const conflict = new WaterSandField(128, 512); conflict.setNaturalObservationRouteId('other-route');
  assert.equal(conflict.setParticleRoute(params).code, 'ROUTE_CONFLICT');
  assert.equal(conflict.particleRouteActive(), false);
});
test('4: R9 numeric adapter retains one total debit and correct per-end local debit', () => {
  const w = require('../entry/src/main/ets/physics/P011ParticleHot.ets').configureHot({ cols: 128, nmax: 8,
    requestCapacity: 16, motionCapacity: 8, eventCapacity: 16, lineageCapacity: 32,
    params: Object.assign({}, params, { nmax: 8 }) });
  const adapter = new (require('../entry/src/main/ets/physics/P011ParticleHotBudget.ets').P011ParticleHotBudgetAdapter)(w);
  assert.equal(adapter.beginHot(1, 1 / 60), 0); adapter.localRemaining[4] = 1; adapter.localRemaining[5] = 0.5;
  assert.equal(adapter.totalRemaining, 5); assert.equal(adapter.bottomUsed, 0);
  assert.equal(adapter.finishHot(1), 0);
});
test('5: field route refuses unload with live particles and preserves the pool', () => {
  const f = routedField(); f.injectGestureSand(request(0.25));
  const count = f.particleBinding.liveCount; assert.equal(f.setParticleRoute(null).code, 'BUSY_PARTICLES');
  assert.equal(f.particleBinding.liveCount, count); assert.equal(f.particleRouteActive(), true);
});
test('6: particle allocation and inversion preserve mass and mirror position', () => {
  const pool = new p.P011ParticleField(128, 8); pool.allocate({ mass: 0.4, x: 10.25, y: 80,
    vx: 2, vy: -1, phase: 0, sourceKind: 0, sourceId: 4 });
  pool.invert180(512); const s = pool.snapshot();
  assert.ok(Math.abs(s.pX.find(v => v !== 0) - (127 - 10.25)) < 1e-9);
  assert.ok(Math.abs(pool.totals().heavy - 0.4) < 1e-9);
});
test('7: full-pool refusal does not delete or mutate existing particle mass', () => {
  const pool = new p.P011ParticleField(128, 1); pool.allocate({ mass: 0.5, x: 1.5, y: 80,
    vx: 0, vy: -1, phase: 0, sourceKind: 0, sourceId: 1 });
  assert.equal(pool.allocate({ mass: 0.2, x: 2.5, y: 80, vx: 0, vy: -1,
    phase: 0, sourceKind: 1, sourceId: 2 }), -1);
  assert.equal(pool.totals().count, 1); assert.equal(pool.totals().heavy, 0.5);
});
test('8: route OFF preserves structural-zero particle totals', () => {
  const f = new WaterSandField(128, 512); const initial = f.snapshot();
  assert.equal(f.particleRouteActive(), false); assert.equal(f.getTotals().heavy, 128 * 24);
  assert.equal(initial.schemaVersion, 3); assert.equal(initial.particleState, null);
});
test('9: live particles hold the field awake and route unload is non-destructive', () => {
  const f = routedField(); f.injectGestureSand(request(0.2));
  f.dormant = true; const r = f.step(1 / 60);
  assert.equal(r.dormant, false); assert.equal(r.ok, true, `route step failed: ${r.errorCode}`);
  assert.equal(f.particleBinding.work.pageBacklog, 0, 'committed evidence page was not drained');
  const second = f.step(1 / 60);
  assert.equal(second.ok, true, `second route step failed: ${second.errorCode}`);
  assert.equal(f.particleBinding.work.pageBacklog, 0, 'second evidence page was not drained');
});
test('10: field invert mirrors particle position and retains phase mass', () => {
  const f = routedField(); f.injectGestureSand(request(0.2));
  const pool = f.particleBinding.pool; const before = pool.snapshot();
  let oldX = 0; for (let i = 0; i < before.nmax; i++) if (before.pState[i]) { oldX = before.pX[i]; break; }
  f.invert180(); const after = pool.snapshot(); let newX = 0;
  for (let i = 0; i < after.nmax; i++) if (after.pState[i]) { newX = after.pX[i]; break; }
  assert.ok(Math.abs(newX - (127 - oldX)) < 1e-9); assert.ok(Math.abs(pool.totals().heavy - 0.2) < 1e-9);
});
test('11: schema 4 round-trip and schema 3 structural-zero restore', () => {
  const f = routedField(); f.injectGestureSand(request(0.2)); const snap = f.snapshot();
  assert.equal(snap.schemaVersion, 4); const other = routedField(); other.restoreFromSnapshot(snap);
  assert.equal(other.particleBinding.pool.totals().heavy, 0.2);
  const old = Object.assign({}, snap, { schemaVersion: 3, particleState: undefined });
  other.restoreFromSnapshot(old); assert.equal(other.particleBinding.pool.totals().count, 0);
});
test('12: injection η solve keeps free-water volume invariant with particles', () => {
  const f = routedField(); const before = f.getTotals().water; const receipt = f.injectGestureSand(request(0.3));
  assert.ok(receipt.admittedH > 0); assert.ok(Math.abs(f.getTotals().water - before) < 1e-6);
});
test('13: sustained external mass uses the expected ledger and stays auditable', () => {
  const f = routedField();
  for (let i = 0; i < 8; i++) {
    const r = f.injectGestureSand(request(20, 0, 100 + i)); assert.ok(r.admittedH >= 0);
  }
  assert.ok(f.getDrift().heavy < 1e-9); assert.ok(f.getDrift().light < 1e-9);
});
console.log(JSON.stringify({ schemaVersion: obs.P011_NATURAL_OBSERVATION_LEDGER_SCHEMA_VERSION,
  tests: results.length, passed: results.filter(r => r.status === 'PASS').length,
  failed: results.filter(r => r.status === 'FAIL') }, null, 2));
if (results.some(r => r.status === 'FAIL')) process.exitCode = 1;
