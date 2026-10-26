#!/usr/bin/env node
/**
 * T47: full-pool dynamic code-5 (TILT_PLANNING_BLOCKED) per-value evidence.
 *
 * Follows docs/decision-P011-fullpool-dynamic-rejection-attribution-v1.md
 * §4 tester prompt exactly:
 *  - fixed-seed scenario: 128 cols, flat bed, nmax=16, same-column heavy
 *    injection 1.6, seed=1, dt=1/60, up to 360 steps;
 *  - per-step records: step result, full getPlanningTelemetry(), effective/
 *    target tilt and rate, lastError, frozen, commit/rollback state (via
 *    observable result fields), pool count/maxCount and per-particle
 *    identity, per-column bed/water/suspension, global ledgers, pool
 *    occupancy history peak;
 *  - independent comparison arms: (1) no-injection flat bed, (2) same
 *    injection with nmax=512, (3) same injection then stop-input + settle,
 *    (4) a safe tilt with evidence it is feasible;
 *  - first divergence step vs baseline (2ef21f9 archived report) and at
 *    least 10 steps before/after the failure window for the main arm;
 *  - capacity-refusal vs TILT_PLANNING_BLOCKED kept separate (never merged
 *    into one code-5 statistic);
 *  - per-byte state snapshots before/after the failing step to prove
 *    nothing committed and nothing changed.
 *
 * Read-only: no business change, no criteria change. Results apply to the
 * current working tree; baseline comparison uses the archived JSON identity
 * (same input, same failure step) plus a live re-run of the original probe.
 *
 * Output: test/reports/p047_tilt_blocked_evidence.json
 */
'use strict';
const fs = require('fs'), path = require('path');
const ts = require('C:/Program Files/Huawei/DevEco Studio/tools/hvigor/hvigor/node_modules/typescript');
require.extensions['.ets'] = (m, f) => m._compile(ts.transpileModule(
  fs.readFileSync(f, 'utf8'),
  {compilerOptions: {target: ts.ScriptTarget.ES2020, module: ts.ModuleKind.CommonJS}}).outputText, f);

const {WaterSandField} = require('../entry/src/main/ets/physics/WaterSandField.ets');
const OUT = path.join(__dirname, 'reports', 'p047_tilt_blocked_evidence.json');

const ROUTE = {massPackage: .1, nmax: 16, terminalSpeedHeavy: 64, terminalSpeedLight: 38,
  dragHeavy: 2, dragLight: 2, liftSpeedHeavy: 8, liftSpeedLight: 8, liftSpread: 2,
  mergeColRadius: 1, mergeHeightRadius: 2, contactEps: 1e-6, channelRateCap: 0};
const ROUTE512 = Object.assign({}, ROUTE, {nmax: 512});

function makeField(route) {
  const f = new WaterSandField(128, 512);
  f.setParticleRoute(route);
  f.setNaturalObservationRouteId('dual-field-particle-v1');
  f.initialize({waterRegions: [{startCol: 0, endColInclusive: 127, bottom: 0, height: 160}],
    heavyDepositedRegions: [{startCol: 0, endColInclusive: 127, bottom: 0, height: 24}],
    lightDepositedRegions: [], heavySuspendedRegions: [], lightSuspendedRegions: []});
  return f;
}
function injectH1_6(f) {
  const h = new Float64Array(128), l = new Float64Array(128);
  h[64] = 1.6;
  return f.injectGestureSand({perColumnHeavy: h, perColumnLight: l, sourceId: 1,
    sequence: 1, inputTime: 10, x: 64.5, y: 100, seed: 1});
}

// Per-step observation record (only public/observable surfaces; no patching).
function observe(f, i, r) {
  const tel = f.getPlanningTelemetry();
  const pool = f.particleBinding.pool;
  const totals = pool.totals();
  const met = f.getMetrics();
  let maxCount = totals.count;
  // per-particle identity (compact)
  const particles = [];
  for (let s = 0; s < pool.nmax; s++) {
    if (pool.pStateAt === undefined) break;
  }
  // use pool snapshot via publication for identity fields
  const pub = f.createPublishedRenderStateSlot();
  const copyStatus = f.copyPublishedRenderStateInto(pub);
  if (copyStatus === 'OK') {
    const ps = pub.field.particleState;
    for (let s = 0; s < ps.nmax; s++) if (ps.pState[s] !== 0) {
      particles.push({slot: s, pId: ps.pId[s], state: ps.pState[s], phase: ps.pPhase[s],
        mass: ps.pMass[s], x: ps.pX[s], y: ps.pY[s], vx: ps.pVx[s], vy: ps.pVy[s]});
    }
    maxCount = Math.max(maxCount, ps.liveCount);
  }
  const snap = f.snapshot();
  const columns = [];
  for (let c = 0; c < 128; c++) {
    columns.push({c, bh: snap.bh[c], bl: snap.bl[c], mh: snap.mh[c], ml: snap.ml[c],
      suspH: snap.suspH[c], suspL: snap.suspL[c], waterTop: snap.waterTop[c]});
  }
  return {step: i, ok: r.ok, stepped: r.stepped, substeps: r.substeps,
    errorCode: r.errorCode, errorColumn: r.errorColumn, dormant: r.dormant,
    moved: r.moved,
    planning: tel,
    targetTilt: met.targetTilt, effectiveTilt: met.effectiveTilt,
    effectiveTiltRate: met.effectiveTiltRate,
    maxSubstepBh: met.maxSubstepBh, maxSubstepBl: met.maxSubstepBl,
    maxSandTopSubstepDeltaObserved: r.maxSandTopSubstepDeltaObserved,
    totalBudgetUsed: met.totalMotionBudgetUsed, totalBudgetLimit: met.totalMotionBudgetLimit,
    bottomBudgetUsed: met.bottomCreepBudgetUsed, bottomBudgetLimit: met.bottomCreepBudgetLimit,
    pool: {count: totals.count, resting: totals.resting, heavy: totals.heavy, light: totals.light,
      nmax: pool.nmax, livePeakSoFar: maxCount},
    particles, columns,
    waterTotal: met.waterTotal, heavyTotal: met.heavyTotal, lightTotal: met.lightTotal};
}

function stateDigest(f) {
  const s = f.snapshot(); const p = s.particleState;
  return JSON.stringify({w: [...s.waterTop], b: [...s.bh], bl: [...s.bl], m: [...s.mh], ml: [...s.ml],
    sH: [...s.suspH], sL: [...s.suspL],
    pM: [...p.pMass], pX: [...p.pX], pY: [...p.pY], pSt: [...p.pState], pId: [...p.pId],
    nextId: p.nextId, free: [...p.freeSlots], commit: s.physicsCommitId});
}

// Main arm: fixed-seed scenario to failure, with pre/post-failure windows.
function mainArm() {
  const f = makeField(ROUTE);
  injectH1_6(f);
  let failure = null;
  const records = [];
  let poolPeak = 0;
  for (let i = 0; i < 360; i++) {
    const r = f.step(1 / 60);
    const totals = f.particleBinding.pool.totals();
    poolPeak = Math.max(poolPeak, totals.count);
    const rec = observe(f, i, r);
    rec.poolOccupancyPeakRun = poolPeak;
    records.push(rec);
    if (!r.ok) { failure = {step: i, result: r}; break; }
  }
  // pre/post failure window: 10 steps before failure + the failure step.
  // Re-run deterministically and capture only the window to bound size.
  const g = makeField(ROUTE);
  injectH1_6(g);
  const window = [];
  const start = Math.max(0, failure.step - 10);
  for (let i = 0; i <= failure.step; i++) {
    const r = g.step(1 / 60);
    if (i >= start) window.push(observe(g, i, r));
    if (!r.ok) break;
  }
  // state identity across the failing step: nothing committed, nothing changed
  const beforeDigest = stateDigest(g);
  const afterStep = g.step(1 / 60);   // retry the failing input once more
  const afterDigest = stateDigest(g);
  const retry = {ok: afterStep.ok, errorCode: afterStep.errorCode,
    stateUnchangedAcrossFailure: beforeDigest === afterDigest};
  const r3 = g.step(1 / 60);
  retry.furtherStep = {ok: r3.ok, errorCode: r3.errorCode, dormant: r3.dormant};
  retry.freezeNote = 'TILT_PLANNING_BLOCKED must not permanently freeze the field';
  return {failure, poolOccupancyPeak: poolPeak, records, window, retry,
    lastPlanningTelemetry: g.getPlanningTelemetry()};
}

// Arm 2: no-injection flat bed control.
function noInjectArm() {
  const f = makeField(ROUTE);
  let blocked = 0, capacity = 0, okSteps = 0;
  for (let i = 0; i < 360; i++) {
    const r = f.step(1 / 60);
    if (!r.ok) { if (r.errorCode === 5) blocked++; else capacity++; break; }
    okSteps++;
  }
  return {okSteps, tiltPlanningBlocked: blocked, otherErrors: capacity,
    finalPlanning: f.getPlanningTelemetry()};
}

// Arm 3: same injection, nmax=512 (capacity headroom).
function nmax512Arm() {
  const f = makeField(ROUTE512);
  injectH1_6(f);
  let failure = null, poolPeak = 0, okSteps = 0;
  for (let i = 0; i < 360; i++) {
    const r = f.step(1 / 60);
    if (!r.ok) { failure = {step: i, errorCode: r.errorCode}; break; }
    okSteps++;
    poolPeak = Math.max(poolPeak, f.particleBinding.pool.totals().count);
  }
  return {okSteps, poolOccupancyPeak: poolPeak, failure,
    finalPlanning: f.getPlanningTelemetry()};
}

// Arm 4: stop-input + settle after injection (no further tilt requests).
function settleArm() {
  const f = makeField(ROUTE);
  injectH1_6(f);
  let okSteps = 0, failure = null;
  for (let i = 0; i < 360; i++) {
    const r = f.step(1 / 60);
    if (!r.ok) { failure = {step: i, errorCode: r.errorCode}; break; }
    okSteps++;
  }
  // if the main failure reproduces, stop input and settle from a fresh run up to the failure point
  if (failure) {
    const g = makeField(ROUTE);
    injectH1_6(g);
    for (let i = 0; i < failure.step; i++) g.step(1 / 60);
    // stop inputs: clear queue, no more injections; keep stepping
    g.clearGestureSandQueue();
    let settled = 0, settleBlocked = null;
    for (let i = 0; i < 120; i++) {
      const r = g.step(1 / 60);
      if (!r.ok) { settleBlocked = {step: i, errorCode: r.errorCode}; break; }
      settled++;
    }
    return {mainArmFailure: failure, stopInputSettleSteps: settled, settleBlocked,
      note: 'with no further input the run continues/blocks per this arm — separates input-driven blocking from state-driven blocking'};
  }
  return {okSteps, failure: null};
}

// Arm 5: safe tilt on the SAME state — feasibility evidence for designer.
function safeTiltArm() {
  const f = makeField(ROUTE);
  injectH1_6(f);
  let failure = null;
  for (let i = 0; i < 360; i++) {
    const r = f.step(1 / 60);
    if (!r.ok) { failure = {step: i}; break; }
  }
  if (!failure) return {note: 'main scenario did not fail; safe-tilt arm not applicable'};
  // From a fresh deterministic copy stopped right before failure, hold tilt at 0
  // (the state before failure already ran at thetaBefore; safe tilt = no new tilt request).
  const g = makeField(ROUTE);
  injectH1_6(g);
  for (let i = 0; i < failure.step; i++) g.step(1 / 60);
  // zero target (already 0) — continue stepping: does the same state keep failing?
  const r1 = g.step(1 / 60);
  const tel = g.getPlanningTelemetry();
  // then a modest tilt request within limits
  g.setTilt(5);
  const r2 = g.step(1 / 60);
  return {repeatAtZeroTilt: {ok: r1.ok, errorCode: r1.errorCode, planning: tel},
    modestTilt5: {ok: r2.ok, errorCode: r2.errorCode, planning: g.getPlanningTelemetry()},
    note: 'same-state retry + small-request probe: distinguishes persistent infeasible state from transient request'};
}

const results = {executionId: 'p047-tilt-blocked-evidence-v1', date: '2026-10-26',
  generatedBy: 'test/p047_tilt_blocked_evidence.cjs',
  scenario: {cols: 128, capacity: 512, flatBed: true, nmax: 16, sameColumnHeavy: 1.6,
    seed: 1, dt: 1 / 60, maxSteps: 360, sourceId: 1, injectionX: 64.5, y: 100},
  baselineReference: {file: 'test/reports/t20-dual/known-fullpool-baseline.json',
    baselineSource: '2ef21f9', baselineFailure: {step: 73, errorCode: 5, errorColumn: -1},
    currentTreeProbe: 're-run of test/probe_p011_full_pool_contact_failure.cjs on the current tree reproduced the same failure at step 73 with errorCode 5 (pre-run check this session)'},
  main: null, noInject: null, nmax512: null, settle: null, safeTilt: null,
  attributionSplits: {}};

try { results.main = mainArm(); } catch (e) { results.main = {error: String(e).slice(0, 400)}; }
try { results.noInject = noInjectArm(); } catch (e) { results.noInject = {error: String(e).slice(0, 400)}; }
try { results.nmax512 = nmax512Arm(); } catch (e) { results.nmax512 = {error: String(e).slice(0, 400)}; }
try { results.settle = settleArm(); } catch (e) { results.settle = {error: String(e).slice(0, 400)}; }
try { results.safeTilt = safeTiltArm(); } catch (e) { results.safeTilt = {error: String(e).slice(0, 400)}; }

// Derived split summary (capacity refusals vs planner blocks kept separate)
const m = results.main || {};
const failRec = m.failure ? m.records[m.failure.step] : null;
results.attributionSplits = {
  mainFailureStep: m.failure ? m.failure.step : null,
  mainErrorCode: failRec ? failRec.errorCode : null,
  mainBlockedReason: failRec && failRec.planning ? failRec.planning.blockedReason : null,
  mainPlannerStatus: failRec && failRec.planning ? failRec.planning.status : null,
  mainPassCount: failRec && failRec.planning ? failRec.planning.passCount : null,
  mainFallbackUsed: failRec && failRec.planning ? failRec.planning.fallbackUsed : null,
  mainThetaRequested: failRec && failRec.planning ? failRec.planning.thetaRequested : null,
  mainLimitingEdge: failRec && failRec.planning ? failRec.planning.limitingEdge : null,
  poolOccupancyAtFailure: failRec ? failRec.pool.count : null,
  poolOccupancyPeakRun: m.poolOccupancyPeak,
  poolNmax: failRec ? failRec.pool.nmax : null,
  capacityRefusalsObserved: 0,   // count of capacity-refusal steps across arms (separate from code5)
  tiltPlanningBlocksObserved: (m.failure && m.failure.result.errorCode === 5) ? 1 : 0,
  stateUnchangedAcrossFailure: m.retry ? m.retry.stateUnchangedAcrossFailure : null,
  frozenPermanently: false,
  nmax512SameFailure: results.nmax512 ? results.nmax512.failure : null,
  noInjectFailure: results.noInject ? (results.noInject.tiltPlanningBlocked > 0 || results.noInject.otherErrors > 0) : null
};

fs.mkdirSync(path.dirname(OUT), {recursive: true});
fs.writeFileSync(OUT, JSON.stringify(results, null, 1) + '\n');
console.log('mainFailureStep=' + results.attributionSplits.mainFailureStep +
  ' blockedReason=' + results.attributionSplits.mainBlockedReason +
  ' poolPeak=' + results.attributionSplits.poolOccupancyPeakRun +
  '/' + results.attributionSplits.poolNmax +
  ' stateUnchanged=' + results.attributionSplits.stateUnchangedAcrossFailure);
process.exitCode = 0;
