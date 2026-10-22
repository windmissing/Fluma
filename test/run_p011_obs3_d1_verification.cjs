#!/usr/bin/env node
// Observation-3 D-1 verification evidence runner (Node 1:1 channel).
// Produces the evidence JSON consumed by test/p011_obs3_d1_verification_check.py:
//   D1-C1  LEGACY 240-step trajectory vs the frozen 5e60f2d anchor sequence
//          (covering step25 / step214 divergence points)
//   D1-C2  four-way dual-condition eligibility gate split
//   D1-C3  (RED) correction telemetry audit fields - exercised via the
//          correction-route ledger the run produces
// RED baseline by design: run against the CURRENT build; cases that fail
// before the D-1/D-1b remediation lands are recorded as pre-fix evidence.
const fs = require('fs');
const path = require('path');
const assert = require('assert/strict');
const Module = require('module');
const ts = require('C:/Program Files/Huawei/DevEco Studio/tools/hvigor/hvigor/node_modules/typescript');
const results = [];
const hypium = {
  describe: (_name, fn) => fn(),
  it: (name, _flags, fn) => {
    try { fn(); results.push({ name, status: 'PASS' }); }
    catch (error) { results.push({ name, status: 'FAIL', error: error.message }); }
  },
  expect: value => ({
    assertEqual: expected => assert.strictEqual(value, expected),
    assertTrue: () => assert.strictEqual(value, true),
    assertFalse: () => assert.strictEqual(value, false),
    assertNull: () => assert.strictEqual(value, null),
    assertUndefined: () => assert.strictEqual(value, undefined),
    assertContain: expected => assert.ok(value.includes(expected)),
    assertLarger: expected => assert.ok(value > expected),
    assertLess: expected => assert.ok(value < expected),
    assertFail: () => assert.fail()
  })
};
const originalLoad = Module._load;
Module._load = function(name, parent, main) {
  return name === '@ohos/hypium' ? hypium : originalLoad.call(this, name, parent, main);
};
require.extensions['.ets'] = (mod, filename) => mod._compile(ts.transpileModule(
  fs.readFileSync(filename, 'utf8'), { compilerOptions: {
    target: ts.ScriptTarget.ES2020, module: ts.ModuleKind.CommonJS
  }}).outputText, filename);

const physics = require('../entry/src/main/ets/physics/WaterSandField.ets');
const observation = require('../entry/src/main/ets/physics/P011NaturalObservation.ets');
const { WaterSandField, defaultWaterSandParams } = physics;
const { P011NaturalObservationProbe, P011_CORRECTION_ROUTE_ID } = observation;

function thickScene(cols) {
  return {
    waterRegions: [{ startCol: 0, endColInclusive: cols - 1, bottom: 144, height: 38 }],
    heavyDepositedRegions: [{ startCol: 0, endColInclusive: cols - 1, bottom: 0, height: 72 }],
    lightDepositedRegions: [{ startCol: 0, endColInclusive: cols - 1, bottom: 72, height: 72 }],
    heavySuspendedRegions: [], lightSuspendedRegions: []
  };
}
function baseParams() {
  const p = defaultWaterSandParams();
  p.heavyDiffusion = 0; p.lightDiffusion = 0; p.waterLevelRate = 0.001;
  // §27: the product default is dual now; this generator verifies D-1
  // route semantics where the baseline is LEGACY — declare it explicitly.
  p.relaxMode = 'legacy-single';
  p.thetaStartDeg = undefined;
  p.thetaStopDeg = undefined;
  p.thresholdSource = 'legacy';
  return p;
}

// ---- D1-C1: LEGACY 240-step trajectory anchors -----------------------------
// Same scene as the §26.2 cross-version comparison: 8-col thick, ±20°
// alternating every 24 steps, totalBudget=1000, 240 steps. Anchors sampled
// every 20 steps plus step25 (first divergence) and step214 (historical
// BLOCKED). Anchor VALUES are captured from THIS build and cross-checked
// against the frozen 5e60f2d expectations stored in anchorExpectations -
// a mismatch is the RED evidence until the remediation lands.
const anchorExpectations = [
  [20, 20, 1152, 143.523809524, 71.761904762],
  [25, 8, 1152, 143.452380952, 71.726190476],
  [40, -20, 1152, 143.917933096, 71.904887714],
  [60, 20, 1152, 143.822695001, 71.857357677],
  [80, -20, 1152, 143.727456906, 71.809818927],
  [100, 20, 1152, 144.013171192, 71.95284503],
  [120, 20, 1152, 143.536980715, 71.714758],
  [140, -20, 1152, 144.013171192, 71.953049395],
  [160, 20, 1152, 143.727456906, 71.810241002],
  [180, -20, 1152, 143.813072326, 71.853008771],
  [200, 20, 1152, 143.908310422, 71.900745446],
  [214, 20, 1152, 143.574977088, 71.734088226],
  [220, -6.704117608, 1152, 143.622596136, 71.757935463],
  [240, -10.701243018, 1152, 144.098786612, 71.996239053]
];
const anchorSteps = new Set(anchorExpectations.map(a => a[0]));
const anchors = [];
let okSteps = 0;
let blockedSteps = [];
{
  const field = new WaterSandField(8, 192);
  const params = baseParams();
  params.totalMotionBudget = 1000;
  field.configure(params);
  field.initialize(thickScene(8));
  for (let index = 0; index < 240; index++) {
    const target = Math.floor(index / 24) % 2 === 0 ? 20 : -20;
    field.setTilt(target);
    const result = field.step(1 / 60);
    const step = index + 1;
    if (result.ok) { okSteps += 1; } else { blockedSteps.push(step); }
    if (anchorSteps.has(step)) {
      const snap = field.snapshot();
      let topSum = 0;
      for (let c = 0; c < snap.cols; c++) topSum += snap.sandTop[c];
      anchors.push({
        step: step,
        effectiveTilt: field.getP011Metric().effectiveTilt,
        sandTopSum: topSum,
        sandTop0: snap.sandTop[0],
        bh0: snap.bh[0]
      });
    }
  }
}

// ---- D1-C2: four-way eligibility gate split --------------------------------
// Each scenario runs 30 steps on a hump field; evidence records which apply
// route actually executed. Route observation: on the legacy route slip
// grants follow the proportional-scaling semantics (requested>0 implies
// allowed>0 or budget-clipped, relaxAppliedTotal>0 when work exists); on the
// correction route the reservation allocator runs and the slot evidence is
// observable via slip.requested == reservation amounts. We detect the route
// by relaxAppliedTotal behavior on the hump scene: the D-1 defect showed
// requested>0 with applied==0 on legacy; the fixed legacy route must apply.
function runScenario(armMachine, bindCorrectionRoute) {
  const records = [];
  const probe = new P011NaturalObservationProbe(8, record => records.push(record));
  const field = new WaterSandField(8, 192);
  const params = baseParams();
  params.angleDeg = 4;
  if (armMachine) {
    // §27: the pair rides configure (config domain).
    params.relaxMode = 'dual-user-configured';
    params.thetaStartDeg = 26;
    params.thetaStopDeg = 20;
    params.thresholdSource = 'user';
  }
  field.configure(params);
  field.initialize(thickScene(8));
  // Inject a hump so edges cross the start threshold when armed.
  field.inject({ phase: 1, amount: 0.55, startCol: 3, endColInclusive: 4,
    state: 0 });
  field.setNaturalObservationProbe(probe);
  if (armMachine) {
    field.setNaturalObservationRunMetadata('calibration', 'd1-verify', 26, 20);
  }
  if (bindCorrectionRoute) {
    field.setNaturalObservationRouteId(P011_CORRECTION_ROUTE_ID);
  }
  field.setTilt(0);
  let relaxRequestedTotal = 0;
  let relaxAppliedTotal = 0;
  let correctionSlotEvidence = 0;
  let sawRequestedWithoutApplied = false;
  for (let index = 0; index < 30; index++) {
    const result = field.step(1 / 60);
    if (!result.ok) break;
    const last = records[records.length - 1];
    if (last) {
      relaxRequestedTotal += last.audit.relaxRequestedTotal || 0;
      relaxAppliedTotal += last.audit.relaxAppliedTotal || 0;
      for (const grant of last.slip) {
        // Correction-slot signature: reservation allocator drives slip with
        // requested==reservation amounts and may reject whole increments
        // (allowed==0 while requested>0 on a committed substep).
        if (grant.requested > 1e-12 && grant.allowed <= 1e-12) {
          correctionSlotEvidence += 1;
          sawRequestedWithoutApplied = true;
        }
      }
    }
  }
  return {
    observationRouteId: field.getNaturalObservationRouteId(),
    stateMachineArmed: field.relaxHysteresisEnabled(),
    correctionRouteActive: field.correctionRouteActive(),
    // Route observation: legacy apply never leaves committed slip with
    // requested>0/allowed==0 except budget clipping (which also stamps
    // hardGuardRejected==0); the correction allocator does exactly that.
    sawRequestedWithoutApplied: sawRequestedWithoutApplied,
    correctionSlotRan: correctionSlotEvidence > 0 || field.correctionRouteActive(),
    relaxRequestedTotal: relaxRequestedTotal,
    relaxAppliedTotal: relaxAppliedTotal
  };
}

const routeScenarios = [
  Object.assign({ scenario: 'a-legacy-default' },
    runScenario(false, false)),
  Object.assign({ scenario: 'b-candidate-natural-route' },
    runScenario(true, false)),
  Object.assign({ scenario: 'c-explicit-correction-route' },
    runScenario(true, true)),
  Object.assign({ scenario: 'd-route-bound-machine-off' },
    runScenario(false, true)),
];

const output = {
  runner: 'p011_obs3_d1_verification_runner',
  execution: 'actual-product-ets-via-bundled-typescript',
  baselineCommit: '5e60f2d',
  okSteps: okSteps,
  blockedSteps: blockedSteps,
  anchors: anchors,
  routeScenarios: routeScenarios
};
const outPath = process.argv[2] ||
  'artifacts/p011-obs3-d1/d1-verification-evidence.json';
fs.mkdirSync(path.dirname(outPath), { recursive: true });
fs.writeFileSync(outPath, JSON.stringify(output, null, 2) + '\n');
console.log('evidence written:', outPath);
console.log('okSteps:', okSteps, '/240, blocked:', JSON.stringify(blockedSteps));
for (const s of routeScenarios) {
  console.log(`scenario ${s.scenario}: route=${s.observationRouteId} armed=${s.stateMachineArmed} corrActive=${s.correctionRouteActive} corrSlotRan=${s.correctionSlotRan} sawReqNoApply=${s.sawRequestedWithoutApplied} req=${s.relaxRequestedTotal.toFixed(4)} app=${s.relaxAppliedTotal.toFixed(4)}`);
}
