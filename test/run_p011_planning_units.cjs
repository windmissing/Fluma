// P-011 §20.7 planning-pass acceptance unit runner (Node 1:1 channel).
// Executes the real ArkTS product code through the bundled TypeScript
// compiler — same channel as run_p011_input_units.cjs — and emits the extra
// evidence fields required by test/p011_planning_pass_check.py:
//   * previewCalls / passCount on the directed exhaustion case (□1)
//   * blockedSemantics section with run-level invalid evidence (□7)
//   * p011_planningPass_verifyConsistencyChannel (□6 channel existence)
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
const planning = require('../entry/src/main/ets/physics/P011TiltPlanningPass.ets');
const { WaterSandField, defaultWaterSandParams } = physics;

const H = Math.tan(30 * Math.PI / 180);

function fullLoadScene(cols) {
  return {
    waterRegions: [{ startCol: 0, endColInclusive: cols - 1, bottom: 120, height: 60 }],
    heavyDepositedRegions: [{ startCol: 0, endColInclusive: cols - 1, bottom: 0, height: 50 }],
    lightDepositedRegions: [{ startCol: 0, endColInclusive: cols - 1, bottom: 50, height: 50 }],
    heavySuspendedRegions: [{ startCol: 0, endColInclusive: cols - 1, bottom: 10, height: 10 }],
    lightSuspendedRegions: [{ startCol: 0, endColInclusive: cols - 1, bottom: 10, height: 10 }]
  };
}

// □1 directed exhaustion with counting delegate.
(function directedExhaustion() {
  let previewCalls = 0;
  const g0 = new Float64Array([0, 0, 2.5, 0, 0]);
  const delegate = {
    previewAt: (_theta) => { previewCalls += 1; return []; },
    sandTopSnapshot: () => {
      const copy = new Float64Array(g0.length + 1);
      copy[0] = 0;
      for (let c = 0; c < g0.length; c++) copy[c + 1] = copy[c] + g0[c];
      return copy;
    },
    preexistingViolation: () => false
  };
  const service = new planning.P011TiltPlanningPassService();
  service.attach(delegate);
  const input = {
    runId: 'check-run', substepId: 1, dt: 1 / 60, thetaBefore: 0, targetTilt: 12,
    maxTiltOmega: 720, sandTopBefore: new Float64Array(6),
    budgetsBefore: {
      localRemaining: [0, 0, 0, 0, 0, 0], bottomRemaining: 0, totalRemaining: 0,
      channelRateCapRemaining: { settle: 0, percolate: 0, bottomCreep: 0, relax: 0, lift: 0, suspensionDiffusion: 0 }
    },
    channelStateBefore: { sliding: [false, false, false, false, false] }
  };
  const preview = service.createPreview(input, H);
  const r = preview.result;
  const entry = {
    name: 'p011_planningPass_directedThreeCandidateExhaustion',
    status: (r.status === 'blocked' && r.passCount <= 2 && previewCalls <= 3) ? 'PASS' : 'FAIL',
    error: `status=${r.status} passCount=${r.passCount} previewCalls=${previewCalls}`,
    previewCalls: previewCalls,
    passCount: r.passCount
  };
  results.push(entry);
  service.detach();
})();

// □6 preview/commit consistency channel: the exported comparator must reject
// mismatches (NaN, -0, grant divergence) and accept exact matches.
(function consistencyChannel() {
  const basePlan = (granted) => ({
    channelId: 'settle',
    requested: { requested: granted },
    plannedGrant: { granted, clipped: 0, rejected: 0 },
    deltaSandTopByColumn: new Float64Array([granted, 0]),
    budgetUse: { local: granted, bottom: 0, total: granted, channelRateCap: 0 },
    admissibility: 'admissible'
  });
  const preview = {
    thetaRequested: 5, thetaAccepted: 5, feasibleThetaMin: 0, feasibleThetaMax: 10,
    limitingEdge: null, plans: [basePlan(1)], passCount: 1, fallbackUsed: false,
    status: 'accepted', blockedReason: null
  };
  const ok = planning.p011PreviewCommitConsistent(preview, 5, [basePlan(1)]);
  const mismatch = planning.p011PreviewCommitConsistent(preview, 5, [basePlan(2)]);
  // -0 vs 0 normalization: preview side carries -0, commit side carries 0.
  const previewMinusZero = {
    thetaRequested: 5, thetaAccepted: -0, feasibleThetaMin: 0, feasibleThetaMax: 10,
    limitingEdge: null, plans: [basePlan(-0)], passCount: 1, fallbackUsed: false,
    status: 'accepted', blockedReason: null
  };
  const minusZeroOk = planning.p011PreviewCommitConsistent(previewMinusZero, 0, [basePlan(0)]);
  const nanBad = planning.p011PreviewCommitConsistent(preview, Number.NaN, [basePlan(1)]);
  const pass = ok === true && mismatch === false && minusZeroOk === true && nanBad === false;
  results.push({
    name: 'p011_planningPass_verifyConsistencyChannel',
    status: pass ? 'PASS' : 'FAIL',
    error: `ok=${ok} mismatch=${mismatch} minusZero=${minusZeroOk} nan=${nanBad}`
  });
})();

// □7 blocked semantics through the full field: force a preexisting violation
// (steep injected hump beyond tan30), then verify step() reports ok=false
// with the planning error code and evidence telemetry, without freezing.
(function blockedSemantics() {
  const field = new WaterSandField(8, 192);
  const params = defaultWaterSandParams();
  params.heavyDiffusion = 0; params.lightDiffusion = 0; params.waterLevelRate = 0.001;
  field.configure(params);
  field.initialize(fullLoadScene(8));
  field.inject({
    phase: 1 /* WaterSandPhase.HEAVY */, amount: 60, startCol: 6, endColInclusive: 7,
    state: 0 /* WaterSandInjectionState.DEPOSITED */
  });
  field.setTilt(0);
  const r = field.step(1 / 60);
  const t = field.getPlanningTelemetry();
  global.__blockedSemantics = {
    status: t.status,
    blockedReason: t.blockedReason,
    passCount: t.passCount,
    okFalse: r.ok === false,
    errorCode: r.errorCode
  };
})();

// □2/□3 preview purity + single commit + □5 tie-break through WaterSandFieldTest.
require('../entry/src/test/WaterSandFieldTest.ets').default();

const output = {
  execution: 'actual-product-ets-via-bundled-typescript',
  results,
  blockedSemantics: global.__blockedSemantics,
  passed: results.filter(r => r.status === 'PASS').length,
  failed: results.filter(r => r.status === 'FAIL').length
};
const outputPath = process.argv[2];
if (outputPath) {
  fs.mkdirSync(path.dirname(outputPath), { recursive: true });
  fs.writeFileSync(outputPath, JSON.stringify(output, null, 2));
}
console.log(JSON.stringify(output, null, 2));
process.exitCode = output.failed ? 1 : 0;
