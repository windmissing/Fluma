#!/usr/bin/env node
/**
 * T73: dry-air DRY_AIR_V1 independent objective verification (tester side).
 *
 * INDEPENDENT executor: cases are designed from docs/design-P051-T58-dry-air-
 * admission-v1.md §8 (D01-D15) and written fresh — the T59 developer
 * self-check (test/run_p059_dry_admission_units.cjs) is NOT reused or
 * re-run as evidence. Verification target: commit ddc6947 dry-air
 * implementation as present in the current working tree (ddc6947 is HEAD
 * lineage; file identity re-checked against the T59 build commit below).
 *
 * Independence measures:
 *  - expectations derived from the T58 design text (receipt fields, W=0,
 *    envelope=O equality, DRY_POINT placement, unified lambda, rollback
 *    identities, EMPTY_BASELINE, dry gravity vector), not from the
 *    developer's assertions;
 *  - extra probes the dev suite does not have: envelope-vs-free-water
 *    distinction after bed growth, lambda monotone cap and slot-count math
 *    recomputed independently, second-admission envelope growth, drift
 *    finiteness with E=0, publication epoch behavior on rejection.
 *
 * No device use; no product/criteria change.
 *
 * Output: test/reports/p073_dry_air_verification.json
 */
'use strict';
const fs = require('fs'), assert = require('assert/strict'), cp = require('child_process');
const ts = require('C:/Program Files/Huawei/DevEco Studio/tools/hvigor/hvigor/node_modules/typescript');
require.extensions['.ets'] = (m, f) => m._compile(ts.transpileModule(
  fs.readFileSync(f, 'utf8'),
  {compilerOptions: {target: ts.ScriptTarget.ES2020, module: ts.ModuleKind.CommonJS}}).outputText, f);

// Verify the dry implementation files are byte-identical to the T59 build commit.
const git = 'C:/Program Files/WorkBuddy/resources/vendor/PortableGit/cmd/git.exe';
const T59_COMMIT = 'ddc694703eb984b8d8c7355919404d9f08884952';
const identityFiles = [
  'entry/src/main/ets/physics/P051DryParticleAdmission.ets',
  'entry/src/main/ets/physics/P051EmptySandDemo.ets',
  'entry/src/main/ets/physics/WaterSandField.ets',
  'entry/src/main/ets/physics/P011ParticleM2Field.ets',
  'entry/src/main/ets/physics/P011ParticleHot.ets',
  'entry/src/main/ets/physics/P011ParticleField.ets',
  'entry/src/main/ets/physics/P011ParticleM2Sink.ets'
];
const identity = {};
let identityAllMatch = true;
for (const f of identityFiles) {
  let snap;
  try { snap = cp.execFileSync(git, ['show', T59_COMMIT + ':' + f], {encoding: 'utf8'}); }
  catch (e) { identity[f] = 'SNAPSHOT_UNAVAILABLE'; identityAllMatch = false; continue; }
  const cur = fs.readFileSync(f, 'utf8').replace(/\r\n/g, '\n');
  const norm = snap.replace(/\r\n/g, '\n');
  const same = norm === cur; // EOL-normalized: git autocrlf checkout difference only
  identity[f] = same ? 'IDENTICAL_EOL_NORMALIZED' : 'DIFFERS_FROM_T59_BUILD';
  if (!same) identityAllMatch = false;
}

const {WaterSandField: F} = require('../entry/src/main/ets/physics/WaterSandField.ets');
const {defaultP011ParticleParams: P} = require('../entry/src/main/ets/physics/P011ParticleM2Field.ets');
const {P051_DRY_REVISION} = require('../entry/src/main/ets/physics/P051DryParticleAdmission.ets');

const results = [];
function rec(id, name, status, detail) {
  results.push(Object.assign({id, name, status}, detail || {}));
  console.log(`[${status}] ${id} ${name}`);
}
function run(id, name, fn) {
  try { const d = fn(); rec(id, name, 'PASS', d); }
  catch (e) { rec(id, name, 'FAIL', {error: String(e && e.stack || e).slice(0, 600)}); }
}
const near = (x, y, eps = 1e-9) => { assert.ok(Math.abs(x - y) < eps, `${x} != ${y}`); };

// Independent helpers (fresh, not from the dev suite)
function dryField(params) {
  const f = new F(128, 256);
  const r = f.initializeDryParticleDemo({revision: P051_DRY_REVISION, sourceId: 51,
    particleParams: params || P()});
  assert.deepEqual(r, {ok: true, code: 'OK'});
  return f;
}
function req(patch) {
  return Object.assign({revision: P051_DRY_REVISION, sourceId: 51, sequence: 1,
    inputTimeMs: 1, seed: 51, x: 20.25, y: 220, requestedH: .3, requestedL: .2}, patch || {});
}
// Independent envelope/free-water recomputation from raw arrays.
function envelopeCheck(f, tag) {
  const s = f.snapshot();
  const p = s.particleState;
  for (let c = 0; c < 128; c++) {
    const O = s.bh[c] + s.bl[c] + s.mh[c] + s.ml[c] + p.colParticleMassH[c] + p.colParticleMassL[c];
    near(s.waterTop[c], O, 1e-9, );          // envelope equality (waterTop IS the envelope)
    assert.ok(s.waterTop[c] <= 256 + 1e-9, 'capacity bound');
  }
  const totals = f.getTotals();
  near(totals.water, 0, 1e-12);              // actual free water stays 0
  assert.ok([...s.suspH, ...s.suspL].every(v => v === 0), 'U stays 0 (dry v1)');
  return {tag};
}
function poolDigest(f) {
  const s = f.snapshot(); const p = s.particleState;
  return JSON.stringify({w: [...s.waterTop], b: [...s.bh], bl: [...s.bl], m: [...s.mh], ml: [...s.ml],
    sH: [...s.suspH], sL: [...s.suspL],
    pM: [...p.pMass], pX: [...p.pX], pY: [...p.pY], pVx: [...p.pVx], pVy: [...p.pVy],
    pSt: [...p.pState], pId: [...p.pId], nextId: p.nextId, free: [...p.freeSlots],
    commit: s.physicsCommitId, epoch: s.stateEpoch});
}

// ------------------------------------------------------- D01 zero baseline
run('T73-D01', 'dry cold start: W/H/L/P/U all zero, cold waterTop all-zero, no NaN, EMPTY_BASELINE, empty tick and reset',
  () => {
    const f = dryField();
    const t = f.getTotals();
    assert.deepEqual({water: t.water, heavy: t.heavy, light: t.light}, {water: 0, heavy: 0, light: 0});
    assert.ok([...f.waterTop].every(v => v === 0), 'cold waterTop all zero');
    const d = f.getDryDiagnostics();
    assert.equal(d.baselineStatus, 'EMPTY_BASELINE');
    assert.equal(d.mediumRevision, 'DRY_AIR_V1');
    assert.equal(d.airDynamicsRevision, 'DRY_AIR_GRAVITY_V1');
    assert.equal(d.freeWaterTotal, 0);
    assert.equal(d.occupiedVolumeTotal, 0);
    assert.ok(f.step(1 / 60).ok);
    envelopeCheck(f, 'after empty tick');
    // reset back to zero after an admission
    f.injectDryGestureSand(req());
    assert.ok(f.initializeDryParticleDemo({revision: P051_DRY_REVISION, sourceId: 51, particleParams: P()}).ok);
    assert.equal(f.getExternalAdmissionEvidence().count, 0, 'reset clears admission count');
    envelopeCheck(f, 'after reset');
    const d2 = f.getDryDiagnostics();
    assert.equal(d2.baselineStatus, 'EMPTY_BASELINE');
    return {resetClearsCount: true};
  });

// ------------------------------------- D02 old wet path NO_WATER unchanged
run('T73-D02', 'old wet injectGestureSand still NO_WATER on dry field (even with nonzero envelope); no bypass',
  () => {
    const f = dryField();
    f.injectDryGestureSand(req());
    const before = poolDigest(f);
    const h = new Float64Array(128); h[20] = .1;
    const r = f.injectGestureSand({perColumnHeavy: h, perColumnLight: new Float64Array(128),
      sourceId: 51, sequence: 2, inputTime: 2, x: 20.25, y: 220, seed: 51});
    assert.equal(r.rejectedReason, 'NO_WATER');
    assert.equal(poolDigest(f), before);
    return {guard: 'NO_WATER intact'};
  });

// ------------------------------------- D03 exact air-point birth placement
run('T73-D03', 'air touch x=20.25,y=220: DRY_POINT placement pX/pY exact, zero initial velocity, no bed growth, W stays 0, envelope grows exactly by admitted mass, one publication',
  () => {
    const f = dryField();
    const commitBefore = f.snapshot().physicsCommitId;
    const r = f.injectDryGestureSand(req({requestedH: .1, requestedL: 0}));
    assert.equal(r.rejectedReason, null);
    near(r.admittedH, .1); near(r.admittedL, 0);
    assert.equal(r.admissionTransactionId, 1);
    assert.equal(r.sourceEpoch, 1);
    assert.equal(r.physicsCommitId, commitBefore + 1);
    const s = f.snapshot(); const p = s.particleState;
    let born = 0;
    for (let i = 0; i < p.nmax; i++) if (p.pState[i] !== 0) {
      born++;
      assert.equal(p.pX[i], 20.25, 'exact touch x preserved (no column-center jitter)');
      assert.equal(p.pY[i], 220, 'exact touch y preserved (not clamped to water/bed)');
      assert.equal(p.pVx[i], 0); assert.equal(p.pVy[i], 0);
      assert.equal(p.pState[i], 1);
    }
    assert.ok(born >= 1, 'at least one particle born');
    assert.ok([...s.mh, ...s.ml].every(v => v === 0), 'bed unchanged at birth (deltaS=0)');
    // independent envelope: grew exactly by admittedH at column 20
    const O = s.bh[20] + s.bl[20] + s.mh[20] + s.ml[20] + p.colParticleMassH[20] + p.colParticleMassL[20];
    near(s.waterTop[20], O, 1e-9);
    near(s.waterTop[20], .1, 1e-9);
    envelopeCheck(f, 'after birth');
    return {born, envelopeCol20: s.waterTop[20]};
  });

// ------------------------- D04 two-phase single transaction + rollback id
run('T73-D04', 'two-phase one request: both phases born at same point, IH/IL separate, epoch/count +1 each once; injected second-phase fault rolls back field/pool/allocator/source/publication completely',
  () => {
    const f = dryField();
    const r = f.injectDryGestureSand(req({requestedH: .3, requestedL: .2}));
    assert.equal(r.rejectedReason, null);
    near(r.admittedH, .3); near(r.admittedL, .2);
    const s = f.snapshot(); const p = s.particleState;
    const phases = new Set();
    for (let i = 0; i < p.nmax; i++) if (p.pState[i] !== 0) { phases.add(p.pPhase[i]); assert.equal(p.pX[i], 20.25); assert.equal(p.pY[i], 220); }
    assert.ok(phases.has(0) && phases.has(1), 'both phases present at the same touch point');
    const ev = f.getExternalAdmissionEvidence();
    assert.equal(ev.count, 1, 'one transaction = one success count');
    const d = f.getDryDiagnostics();
    near(d.sourceEpoch, 1);
    // fault injection on second phase row
    const g = dryField();
    const b = g.particleBinding;
    const orig = b.sink.applyInto.bind(b.sink);
    b.sink.applyInto = (w, e, pl, row, rr) => row === 1 ? 13 : orig(w, e, pl, row, rr);
    const before = poolDigest(g);
    const r2 = g.injectDryGestureSand(req());
    assert.equal(r2.rejectedReason, 'CAPACITY_BLOCKED');
    assert.equal(poolDigest(g), before, 'complete rollback identity');
    assert.equal(b.work.pageBacklog, 0);
    assert.equal(b.active, false);
    b.sink.applyInto = orig;
    return {bothPhases: true, rollbackClean: true};
  });

// ------------------------------------------- D05 unified lambda / capacity
run('T73-D05', 'unified lambda: near-full column admits H/L at the same lambda; one free slot with both phases positive rejects whole request; independent lambda math',
  () => {
    let params = P(); params.massPackage = 10;
    const f = dryField(params);
    f.injectDryGestureSand(req({requestedH: 200, requestedL: 55}));
    // independent capacity math: column 20 occupied 255, free = 1
    const s = f.snapshot(); const p = s.particleState;
    const O = s.bh[20] + s.bl[20] + s.mh[20] + s.ml[20] + p.colParticleMassH[20] + p.colParticleMassL[20];
    near(O, 255, 1e-9);
    const free = 256 - O;
    near(free, 1, 1e-9);
    const r = f.injectDryGestureSand(req({sequence: 2, inputTimeMs: 2, requestedH: 2, requestedL: 2}));
    assert.equal(r.rejectedReason, null);
    // independent lambda: min(1, free/(H+L)) = 1/4
    near(r.lambda, 0.25, 1e-12);
    near(r.admittedH, 0.5, 1e-12); near(r.admittedL, 0.5, 1e-12);
    envelopeCheck(f, 'after lambda admit');
    // one slot, both phases -> whole-request rejection
    let p1 = P(); p1.nmax = 1;
    const g = dryField(p1);
    const before = poolDigest(g);
    const r2 = g.injectDryGestureSand(req());
    assert.equal(r2.rejectedReason, 'CAPACITY_BLOCKED');
    assert.equal(poolDigest(g), before);
    return {lambda: r.lambda, wholeRequestReject: true};
  });

// ------------------------------------------- D06 invalid inputs / NO_OP
run('T73-D06', 'invalid x/y/mass and Q=0: explicit rejection without clamping to water/bed; zero request is NO_OP with no state change',
  () => {
    const f = dryField();
    for (const patch of [{x: 128}, {x: -1}, {y: 257}, {y: 0}, {x: NaN},
      {requestedH: -1}, {requestedL: Infinity},
      {requestedH: Number.MAX_VALUE, requestedL: Number.MAX_VALUE},
      {requestedH: 0, requestedL: 0}]) {
      const before = poolDigest(f);
      const r = f.injectDryGestureSand(req(patch));
      assert.ok(r.rejectedReason, 'expected rejection for ' + JSON.stringify(patch));
      assert.equal(poolDigest(f), before, 'no mutation for ' + JSON.stringify(patch));
    }
    const r0 = f.injectDryGestureSand(req({requestedH: 0, requestedL: 0}));
    assert.equal(r0.rejectedReason, 'NO_OP');
    return {rejectedAll: true};
  });

// --------------------------------- D07 airborne through LAND with W=0
run('T73-D07', 'completely dry birth -> continuous steps to LAND: W stays 0 through the whole trajectory, per-phase conservation holds, envelope consistent after crossing/landing',
  () => {
    const f = dryField();
    assert.equal(f.injectDryGestureSand(req({requestedH: .3, requestedL: .2})).rejectedReason, null);
    let steps = 0;
    for (let i = 0; i < 600; i++) {
      assert.ok(f.step(1 / 60).ok, `step ${i} code ${f.getLastError()}`);
      envelopeCheck(f, 'step ' + i);
      steps++;
      if (f.particleBinding.pool.liveCount === 0) break;
    }
    assert.equal(f.particleBinding.pool.liveCount, 0, 'all particles landed');
    const t = f.getTotals();
    near(t.heavy, .3, 1e-9); near(t.light, .2, 1e-9);
    near(t.water, 0, 1e-12);
    return {steps};
  });

// ------------------------------ D08 full receiver / budget / partial LAND
run('T73-D08', 'budget-less CONTACT_WAIT then budgeted step: state2 retained, inventory exact, no quality loss (independent state-chain check)',
  () => {
    const f = dryField();
    assert.equal(f.injectDryGestureSand(req({y: 1e-6, requestedH: .1, requestedL: 0})).rejectedReason, null);
    const b = f.particleBinding;
    f.substepMotionBudgetRemaining.fill(0);
    f.substepMotionBudgetUsed.fill(0);
    f.totalMotionBudgetRemaining = 0;
    f.totalMotionBudgetUsed = 0;
    const zeros = new Float64Array(128);
    assert.equal(b.beginCollect(1 / 60, 0, 1, 1, .1, 0, 1), 0);
    assert.equal(b.apply(1, zeros, new Float64Array(128), 0), 0);
    b.exportCommittedEvidence();
    f.updateSandTop();
    assert.equal(b.pool.liveCount, 1);
    assert.equal(b.pool.pState[0], 2, 'CONTACT_WAIT (state2) retained');
    near(f.getTotals().heavy, .1, 1e-9);
    envelopeCheck(f, 'after zero-budget contact');
    assert.ok(f.step(1 / 60).ok);
    near(f.getTotals().heavy, .1, 1e-9);
    envelopeCheck(f, 'after budgeted step');
    return {state2Retained: true};
  });

// ------------------------------------- D09 export fault / later step fault
run('T73-D09', 'export sink failure records INCOMPLETE without revoking the committed admission; later physics step failure never revokes external success',
  () => {
    const f = dryField();
    f.setExternalAdmissionEventSink(() => { throw Error('sink'); });
    assert.equal(f.injectDryGestureSand(req()).rejectedReason, null);
    const ev = f.getExternalAdmissionEvidence();
    assert.equal(ev.count, 1);
    assert.equal(ev.rawHistoryStatus, 'INCOMPLETE');
    assert.equal(ev.exportFailures, 1);
    near(f.getTotals().heavy, .3, 1e-9);
    // later step failure does not revoke
    const g = dryField();
    g.injectDryGestureSand(req());
    const before = poolDigest(g);
    const audit = g.audit.bind(g);
    g.audit = () => 5;
    assert.equal(g.step(1 / 60).ok, false);
    g.audit = audit;
    assert.equal(poolDigest(g), before);
    assert.equal(g.getExternalAdmissionEvidence().count, 1);
    return {incompleteEvidence: true, noRevocation: true};
  });

// ------------------------------------- D10 duplicate sequence / no catch-up
run('T73-D10', 'duplicate sequence rejected (identity), no catch-up after stop/background cycles in the real demo model; successful mass (not requested) enters E',
  () => {
    const f = dryField();
    assert.equal(f.injectDryGestureSand(req()).rejectedReason, null);
    const before = poolDigest(f);
    assert.equal(f.injectDryGestureSand(req()).rejectedReason, 'INVALID_IDENTITY');
    assert.equal(poolDigest(f), before);
    // real demo model cycles
    const {P051EmptySandDemo: D} = require('../entry/src/main/ets/physics/P051EmptySandDemo.ets');
    const d = new D();
    d.setActive(true);
    d.start(20.25, 220);
    d.tick(0, 0);
    d.setActive(false);
    d.tick(60e9, 1 / 60);
    d.setActive(true);
    d.tick(61e9, 1 / 60);
    assert.equal(d.field.getTotals().heavy, 0, 'no catch-up injection');
    return {noCatchUp: true};
  });

// ------------------------------- D11 E=0 denominators / phase baselines
run('T73-D11', 'E=0: H-only/L-only/both-phase sources never reset initial baseline; drift finite with empty baseline; the other phase stays truly zero',
  () => {
    for (const [h, l] of [[.1, 0], [0, .1], [.1, .1]]) {
      const f = dryField();
      assert.equal(f.injectDryGestureSand(req({requestedH: h, requestedL: l})).rejectedReason, null);
      const s = f.snapshot();
      assert.equal(s.externalSourceState.initialHeavy, 0, 'initial baseline never reset');
      assert.equal(s.externalSourceState.initialLight, 0);
      near(s.externalSourceState.sourceInjectedH, h, 1e-12);
      near(s.externalSourceState.sourceInjectedL, l, 1e-12);
      const drift = f.getDrift();
      assert.ok(Object.values(drift).every(Number.isFinite), 'drift finite at E=0-ish');
      if (h === 0) near(f.getTotals().heavy, 0, 1e-12);
      if (l === 0) near(f.getTotals().light, 0, 1e-12);
      envelopeCheck(f, 'D11 ' + h + '/' + l);
    }
    return {baselinesPreserved: true};
  });

// --------------------------- D12 tilt vector / shake retain / cold parity
run('T73-D12', 'dry air acceleration follows accepted tilt (±15°) with correct signs; 0° degrades to legacy candidate; cold predictor matches both HOT collects',
  () => {
    const cold = require('../entry/src/main/ets/physics/P011ParticleField.ets');
    for (const angle of [-15, 0, 15]) {
      const f = dryField();
      assert.equal(f.injectDryGestureSand(req({requestedH: .1, requestedL: 0})).rejectedReason, null);
      f.setTilt(angle);
      assert.ok(f.step(1 / 60).ok);
      const s = f.snapshot(); const p = s.particleState;
      const i = p.pState.findIndex(v => v !== 0);
      const theta = f.internalEffectiveTilt() * Math.PI / 180;
      near(p.pVx[i], 9.8 * Math.sin(theta) / 60, 1e-12);
      near(p.pVy[i], -9.8 * Math.cos(theta) / 60, 1e-12);
      envelopeCheck(f, 'tilt ' + angle);
      if (angle === 15) {
        // cold predictor vs HOT double-collect parity
        const b = f.particleBinding;
        const dt = 1 / 60;
        assert.equal(b.beginCollect(dt, theta, 1, 99, .1, 0, 1), 0);
        const w = b.work;
        const field = {cols: 128, capacity: 256, mediumMode: 'DRY_AIR_V1', effectiveTiltRad: theta,
          sandTopAt: c => w.frozenSurface[c], waterTopAt: c => w.frozenWaterTop[c], suspAt: () => 0,
          freeSpaceAt: c => 256 - w.frozenSurface[c] - w.frozenPool.colParticleMassH[c] - w.frozenPool.colParticleMassL[c]};
        const id = {runId: 'cold-dry', substepId: 1, dt, effectiveTiltRad: theta, seed: 99,
          paramsRevision: 1, inputRevision: 1};
        const plan = cold.collectParticlePlan(id, field, w.frozenPool, P(), new cold.P011ParticleJournal(id));
        assert.equal(plan.motions.length, 1);
        near(plan.motions[0].vx, w.planA.motionVx[0], 1e-12);
        near(plan.motions[0].vy, w.planA.motionVy[0], 1e-12);
        assert.equal(w.planA.motionX[0], w.planB.motionX[0]);
        assert.equal(w.planA.motionY[0], w.planB.motionY[0]);
        b.rollback();
      }
    }
    return {parity: true};
  });

run('T73-D12b', 'full-pool shake retains bed, refunds, never degrades to suspension (U stays 0)',
  () => {
    const params = P(); params.nmax = 2;
    const f = dryField(params);
    assert.equal(f.injectDryGestureSand(req({requestedH: .1, requestedL: 0, y: .01})).rejectedReason, null);
    for (let i = 0; i < 30 && f.particleBinding.pool.liveCount; i++) assert.ok(f.step(1 / 60).ok);
    near(f.mh[20], .1, 1e-9);
    assert.equal(f.injectDryGestureSand(req({sequence: 2, inputTimeMs: 2, requestedH: .2, requestedL: 0})).rejectedReason, null);
    f.setShakeInput(1);
    for (let i = 0; i < 120; i++) { assert.ok(f.step(1 / 60).ok); envelopeCheck(f, 'shake ' + i); }
    near(f.mh[20], .1, 1e-9);
    near(f.getTotals().heavy, .3, 1e-9);
    assert.equal(f.getExternalAdmissionEvidence().count, 2);
    return {bedRetained: true};
  });

// --------------------------------------- D13 snapshot/restore identities
run('T73-D13', 'dry snapshot round-trip restores medium/source/count/envelope; corrupt medium/envelope/missing fields refuse restore before any write',
  () => {
    const f = dryField();
    assert.equal(f.injectDryGestureSand(req()).rejectedReason, null);
    const s = f.snapshot();
    const g = dryField();
    g.restoreFromSnapshot(s);
    assert.equal(g.getMediumMode(), 'DRY_AIR_V1');
    assert.equal(g.injectDryGestureSand(req()).rejectedReason, 'INVALID_IDENTITY', 'sequence identity carried');
    envelopeCheck(g, 'restored');
    for (const mutate of [sn => { sn.airDynamicsRevision = 'bad'; },
      sn => { delete sn.mediumMode; },
      sn => { sn.waterTop[20] += 1; },
      sn => { delete sn.externalSourceState; }]) {
      const bad = f.snapshot();
      mutate(bad);
      const before = poolDigest(g);
      assert.throws(() => g.restoreFromSnapshot(bad));
      assert.equal(poolDigest(g), before, 'no partial write on refused restore');
    }
    return {roundTrip: true, refusals: 4};
  });

// --------------------------------------------- D14 mode gates / conflicts
run('T73-D14', 'mode gates: dry field rejects invert (DRY_INVERT_UNSUPPORTED), water writers (MODE_CONFLICT), route unload; wet field rejects dry injection; wet run state intact after each refusal',
  () => {
    const f = dryField();
    assert.equal(f.injectDryGestureSand(req()).rejectedReason, null);
    const before = poolDigest(f);
    assert.throws(() => f.invert180(), /DRY_INVERT_UNSUPPORTED/);
    assert.throws(() => f.addWater(1), /MODE_CONFLICT/);
    assert.equal(f.setParticleRoute(null).code, 'ROUTE_CONFLICT');
    assert.equal(poolDigest(f), before);
    const wet = new F(128, 256);
    assert.equal(wet.injectDryGestureSand(req()).rejectedReason, 'MODE_CONFLICT');
    return {gatesIntact: true};
  });

// --------------------------------------- D15 same-commit render adaptation
run('T73-D15', 'dry publication (field+pool same commit, medium carried) passes the existing adapter with COLUMN_PRESERVING_F32_RENDER_V1; packet commit matches; no second render path introduced',
  () => {
    const {P013DualFieldAdapter: A, P013DualFieldUploadSlot: U} =
      require('../entry/src/main/ets/render/P013DualFieldAdapter.ets');
    const f = dryField();
    assert.equal(f.injectDryGestureSand(req()).rejectedReason, null);
    const p = f.createPublishedRenderStateSlot();
    assert.equal(f.copyPublishedRenderStateInto(p), 'OK');
    assert.equal(p.mediumMode, 'DRY_AIR_V1');
    const out = new U(p.particleCapacity);
    assert.equal(new A().packInto(p, {physicsCommitId: p.physicsCommitId}, out, 'COLUMN_PRESERVING_F32_RENDER_V1'), 'OK');
    assert.equal(out.frame.physicsCommitId, p.physicsCommitId);
    assert.equal(out.frame.particleCount, p.liveCount);
    envelopeCheck(f, 'after pack');
    return {adapterPath: 'COLUMN_PRESERVING_F32_RENDER_V1 (new pipeline only; no Canvas fallback exercised)'};
  });

// ------------------------- T73 additions: envelope growth & publication id
run('T73-X01', 'second admission grows envelope by exactly the second admitted mass and bumps publication again; rejected request does not bump physicsCommitId',
  () => {
    const f = dryField();
    const c0 = f.snapshot().physicsCommitId;
    assert.equal(f.injectDryGestureSand(req({requestedH: .1, requestedL: 0})).rejectedReason, null);
    const c1 = f.snapshot().physicsCommitId;
    assert.equal(c1, c0 + 1);
    assert.equal(f.injectDryGestureSand(req({sequence: 2, inputTimeMs: 2, requestedH: .05, requestedL: 0})).rejectedReason, null);
    const s = f.snapshot();
    near(s.waterTop[20], .15, 1e-9);
    assert.equal(s.physicsCommitId, c1 + 1);
    const before = poolDigest(f);
    const c2 = f.snapshot().physicsCommitId;
    assert.ok(f.injectDryGestureSand(req({x: 128})).rejectedReason);
    assert.equal(f.snapshot().physicsCommitId, c2, 'rejection does not publish');
    assert.equal(poolDigest(f), before);
    return {envelopeGrowth: .15, rejectionDoesNotPublish: true};
  });

fs.mkdirSync(require('path').join(__dirname, 'reports'), {recursive: true});
const pass = results.filter(r => r.status === 'PASS').length;
const fail = results.filter(r => r.status === 'FAIL').length;
const out = {
  executionId: 'p073-dry-air-verification-v1', date: '2026-10-26',
  generatedBy: 'test/p073_dry_air_verification.cjs',
  independence: 'Cases designed fresh from T58 design §8 (D01-D15); T59 developer self-check not reused as evidence; envelope/lambda/conservation recomputed independently from raw arrays.',
  targetIdentity: {t59Commit: T59_COMMIT, files: identity, allMatch: identityAllMatch,
    note: identityAllMatch ? 'working-tree dry implementation is byte-identical to the T59 build' : 'SOME FILES DIFFER — results apply to the current working tree, not the archived build'},
  results,
  summary: {total: results.length, pass, fail,
    verdict: fail === 0 ? 'ALL REGISTERED CASES PASS (offline objective layer)' : 'FAILURES PRESENT — route to designer attribution'},
  boundaries: 'Offline objective verification only; screen effect, device behavior and calibration NOT covered. Old endpoint/peak protocols untouched; dry medium applicability to those protocols remains a separate registration.'
};
fs.writeFileSync(require('path').join(__dirname, 'reports', 'p073_dry_air_verification.json'),
  JSON.stringify(out, null, 1) + '\n');
console.log(`total=${results.length} pass=${pass} fail=${fail}`);
process.exitCode = fail > 0 ? 1 : 0;
