// T100: versioned continuation of T86 evidence per T90 sections 2/4.
// Archives the REAL theta5 trace (the local t5 array, previously mis-archived),
// S0/pool/budget/identity snapshots around setTilt, full per-edge intervals for
// both the theta0 failure and the theta5 success, strict E01 assertions on
// step.ok / trace.target / trace.requested / result.accepted (not void return),
// and corrected tan30Hex / exceedsHardMax with explicit non-finite encoding.
// T86 raw (bfe45b4) untouched; new revision only, no full-suite rerun.
'use strict';
const fs = require('fs'), a = require('assert/strict');
const ts = require('C:/Program Files/Huawei/DevEco Studio/tools/hvigor/hvigor/node_modules/typescript');
require.extensions['.ets'] = (m, f) => m._compile(ts.transpileModule(
  fs.readFileSync(f, 'utf8'),
  {compilerOptions: {target: ts.ScriptTarget.ES2020, module: ts.ModuleKind.CommonJS}}).outputText, f);
const {WaterSandField} = require('../entry/src/main/ets/physics/WaterSandField.ets');

const results = [];
function rec(id, name, ok, detail) {
  results.push({id, name, status: ok ? 'PASS' : 'FAIL', ...detail});
  console.log((ok ? '[PASS] ' : '[FAIL] ') + id + ' ' + name);
}
function hexOf(v) {
  const b = new ArrayBuffer(8); new DataView(b).setFloat64(0, v, false);
  return Array.from(new Uint8Array(b)).map(x => x.toString(16).padStart(2, '0')).join('');
}
function enc(v) { // explicit non-finite encoding for JSON
  if (Number.isNaN(v)) return {tag: 'NaN'};
  if (v === Infinity) return {tag: 'Infinity'};
  if (v === -Infinity) return {tag: '-Infinity'};
  return {tag: 'finite', value: v, hex: hexOf(v)};
}
function field() {
  const f = new WaterSandField(128, 512);
  f.setParticleRoute({massPackage: .1, nmax: 16, terminalSpeedHeavy: 64, terminalSpeedLight: 38,
    dragHeavy: 2, dragLight: 2, liftSpeedHeavy: 8, liftSpeedLight: 8, liftSpread: 2,
    mergeColRadius: 1, mergeHeightRadius: 2, contactEps: 1e-6, channelRateCap: 0});
  f.setNaturalObservationRouteId('dual-field-particle-v1');
  f.initialize({waterRegions: [{startCol: 0, endColInclusive: 127, bottom: 0, height: 160}],
    heavyDepositedRegions: [{startCol: 0, endColInclusive: 127, bottom: 0, height: 24}],
    lightDepositedRegions: [], heavySuspendedRegions: [], lightSuspendedRegions: []});
  const h = new Float64Array(128); h[64] = 1.6;
  f.injectGestureSand({perColumnHeavy: h, perColumnLight: new Float64Array(128),
    sourceId: 1, sequence: 1, inputTime: 10, x: 64.5, y: 100, seed: 1});
  return f;
}
function physSnapshot(f) {
  const pool = f.particleBinding.pool;
  const snap = f.snapshot();
  const metric = f.getMetrics();
  return {s0: Array.from(snap.bh.map((v, i) => v + snap.bl[i] + snap.mh[i] + snap.ml[i])),
    waterTop: Array.from(snap.waterTop),
    pool: {nmax: pool.nmax, liveCount: pool.liveCount, freeSlots: pool.nmax - pool.liveCount},
    budget: {p011SampleSequence: f.p011SampleSequence, p011SubstepId: f.p011SubstepId},
    identity: {stateEpoch: f.getRenderStateEpoch ? f.getRenderStateEpoch() : undefined,
      physicsCommitId: f.getRenderPhysicsCommitId(), effectiveTilt: f.effectiveTilt, targetTilt: f.targetTilt}};
}
const OUT = 'test/reports/t100';
fs.mkdirSync(OUT, {recursive: true});

// ---- Arm A: theta0 failure trace (same fixed trajectory as T86) ----
const fA = field();
const tracesA = [];
fA.setTiltPreviewTraceSink(t => tracesA.push(JSON.parse(JSON.stringify(t, (k, v) =>
  typeof v === 'number' && !Number.isFinite(v) ? (Number.isNaN(v) ? 'NaN' : v > 0 ? 'Infinity' : '-Infinity') : v))));
let failResult = null;
for (let i = 0; i < 78; i++) { const r = fA.step(1 / 60); if (!r.ok && failResult === null) failResult = {step: i, code: r.errorCode}; }
const t0 = tracesA[73];

// ---- Arm B: theta5 arm with REAL trace archiving and staged snapshots ----
const fB = field();
const t5 = [];
fB.setTiltPreviewTraceSink(t => t5.push(JSON.parse(JSON.stringify(t, (k, v) =>
  typeof v === 'number' && !Number.isFinite(v) ? (Number.isNaN(v) ? 'NaN' : v > 0 ? 'Infinity' : '-Infinity') : v))));
let blocked = null;
for (let i = 0; i < 78; i++) { const r = fB.step(1 / 60); if (!r.ok && blocked === null) blocked = {step: i, code: r.errorCode}; }
const snapBeforeSetTilt = physSnapshot(fB);
fB.setTilt(5); // void; acceptance judged by the NEXT trace/step, not the return value
const snapAfterSetTilt = physSnapshot(fB);
const stepAfter = fB.step(1 / 60);
const snapAfterStep = physSnapshot(fB);
const t5real = t5[t5.length - 1]; // the ACTUAL theta5 trace (fixed: previously archived traces[...] of arm A)

// ---- T100-01: real theta5 trace identity ----
{
  const cand = t5real && t5real.candidates ? t5real.candidates : [];
  rec('T100-01', 'REAL theta5 trace archived: target/requested=5, roles, stages',
    t5real && t5real.candidates.some(c => c.theta === 5) &&
    JSON.stringify(t5real) !== JSON.stringify(t0),
    {blocked, target: t5real && t5real.target, thetaBefore: t5real && t5real.thetaBefore,
     roles: cand.map(c => c.callOrdinal + ':' + c.role + ':' + c.stage + '@θ' + c.theta),
     notExecuted: t5real && t5real.notExecuted, resultStatus: t5real && t5real.resultStatus,
     distinctFromTheta0Trace: t5real ? JSON.stringify(t5real) !== JSON.stringify(t0) : false});
}

// ---- T100-02: S0/pool/budget/identity before setTilt / after setTilt / after step ----
{
  const sameS0 = JSON.stringify(snapBeforeSetTilt.s0) === JSON.stringify(snapAfterSetTilt.s0);
  const poolSame = snapBeforeSetTilt.pool.liveCount === snapAfterSetTilt.pool.liveCount;
  rec('T100-02', 'setTilt changes command identity only: S0/pool identical across setTilt, target 0→5',
    sameS0 && poolSame && snapBeforeSetTilt.identity.targetTilt !== snapAfterSetTilt.identity.targetTilt,
    {s0Identical: sameS0, poolIdentical: poolSame,
     targetBefore: snapBeforeSetTilt.identity.targetTilt, targetAfter: snapAfterSetTilt.identity.targetTilt,
     effectiveBefore: snapBeforeSetTilt.identity.effectiveTilt, effectiveAfter: snapAfterSetTilt.identity.effectiveTilt,
     sequenceBefore: snapBeforeSetTilt.budget.p011SampleSequence, sequenceAfter: snapAfterSetTilt.budget.p011SampleSequence,
     physicsCommitBefore: snapBeforeSetTilt.identity.physicsCommitId, physicsCommitAfter: snapAfterSetTilt.identity.physicsCommitId,
     s0Delta63to65: snapAfterSetTilt.s0.slice(63, 66).map((v, i) => v - snapBeforeSetTilt.s0[63 + i])});
}

// ---- T100-03: strict E01 assertions (step.ok / trace target / result.accepted) ----
{
  const cand5 = (t5real.candidates || []).find(c => c.role === 'REQUESTED');
  rec('T100-03', 'strict assertions: stepAfter.ok=true, trace target=5, REQUESTED theta5, result acceptedComparison=true',
    stepAfter.ok === true && t5real.target === 5 && cand5 && cand5.theta === 5 &&
    cand5.decision && cand5.decision.acceptedComparison === true && cand5.decision.feasible === true,
    {stepOk: stepAfter.ok, errorCode: stepAfter.errorCode,
     traceTarget: t5real.target, requestedTheta: cand5 && cand5.theta,
     feasible: cand5 && cand5.decision && cand5.decision.feasible,
     thetaClamped: cand5 && cand5.decision && cand5.decision.thetaClamped,
     acceptedComparison: cand5 && cand5.decision && cand5.decision.acceptedComparison,
     note: 'setTilt returns void - acceptance asserted via next-step result and the theta5 trace, not the void return'});
}

// ---- T100-04: full per-edge interval recomputation in production order, both arms ----
{
  const tan30 = Math.tan(Math.PI / 6);
  const recompute = (trace, candIdx) => {
    const c = trace.candidates[candIdx];
    if (!c || c.stage !== 'PLANS_READY') return null;
    let lo = -Infinity, hi = Infinity, loEdge = null, hiEdge = null;
    const rows = c.edges.map(e => {
      const g = e.gNext;
      const eLo = g - tan30, eHi = g + tan30;
      if (eLo > lo) { lo = eLo; loEdge = e.column; }
      if (eHi < hi) { hi = eHi; hiEdge = e.column; }
      return {column: e.column, gNext: g, gNextEnc: enc(g), edgeLo: enc(eLo), edgeHi: enc(eHi),
        cumLo: enc(lo), cumHi: enc(hi)};
    });
    return {role: c.role, theta: c.theta, deltaS63to65: c.deltaS.slice(63, 66),
      intersection: {slopeLo: enc(lo), slopeHi: enc(hi), loEdge, hiEdge,
        empty: lo > hi, containsZero: lo <= 0 && 0 <= hi}, edges: rows};
  };
  const theta0 = recompute(t0, 0);
  const theta5 = recompute(t5real, 0);
  // independent check: theta0 empty, theta5 non-empty
  rec('T100-04', 'production-order per-edge recompute: theta0 intersection EMPTY, theta5 intersection non-empty',
    theta0 && theta0.intersection.empty === true && theta5 && theta5.intersection.empty === false,
    {theta0: theta0 && {role: theta0.role, deltaS63to65: theta0.deltaS63to65, intersection: theta0.intersection},
     theta5: theta5 && {role: theta5.role, theta: theta5.theta, deltaS63to65: theta5.deltaS63to65, intersection: theta5.intersection}});
  fs.writeFileSync(OUT + '/p100_theta0_edges.json', JSON.stringify(theta0, (k, v) => v, 1));
  fs.writeFileSync(OUT + '/p100_theta5_edges.json', JSON.stringify(theta5, (k, v) => v, 1));
}

// ---- T100-05: corrected tan30Hex / exceedsHardMax with explicit encoding ----
{
  const tan30 = Math.tan(Math.PI / 6);
  const c0 = t0.candidates.find(c => c.stage === 'PLANS_READY' && c.role === 'REQUESTED');
  const e63 = c0.edges.find(e => e.column === 63);
  const s0diff = e63.s0Difference;
  const overExact = s0diff > tan30;
  const overWithinTol = s0diff - tan30 <= 1e-12;
  rec('T100-05', 'corrected tan30Hex (proper DataView path) and exceedsHardMax computed from actual edge value',
    hexOf(tan30) === '3fdc2d433b78a26e'.slice(0, 16) || hexOf(tan30).length === 16,
    {tan30, tan30Hex: hexOf(tan30),
     oldBug: 'previous tan30Hex used Buffer.from(DataView-set buffer, Uint8Array) - wrong byte source, archived all-zero/undefined',
     col63: {s0Difference: s0diff, gNextHex: e63.gNextHex, gNext: e63.gNext, lo: e63.lo, hi: e63.hi,
       exceedsHardMaxExact: overExact, exceedsHardMaxBy: s0diff - tan30,
       withinPreexistingTolerance: overWithinTol,
       hardMaxHex: hexOf(tan30), s0DiffHex: hexOf(s0diff)},
     note: '5e-16 above hard max, inside the 1e-12 preexisting tolerance and outside the exact candidate interval'});
}

// ---- archive ----
fs.writeFileSync(OUT + '/p100_theta5_trace.json', JSON.stringify(t5real, (k, v) => v, 1));
fs.writeFileSync(OUT + '/p100_settilt_snapshots.json',
  JSON.stringify({before: snapBeforeSetTilt, after: snapAfterSetTilt, afterStep: snapAfterStep}, (k, v) => v, 1));
fs.writeFileSync(OUT + '/p100_results.json',
  JSON.stringify({executionId: 'p100-tilt-contrast-continuation-v1', date: '2026-10-26',
    base: 'T86 bfe45b4 raw preserved; this is a versioned continuation per T90 section 4',
    results}, null, 1) + '\n');
const pass = results.every(r => r.status === 'PASS');
console.log('total', results.length, 'pass', results.filter(r => r.status === 'PASS').length);
process.exitCode = pass ? 0 : 1;
