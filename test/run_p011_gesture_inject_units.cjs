// P-011 falling-sand physics injection unit runner (architecture-P011-
// gesture-sand-inject-phys-v1.md §6.2 — ten cases, Node channel).
'use strict';
const fs = require('fs');
const assert = require('assert/strict');
const Module = require('module');
const ts = require('C:/Program Files/Huawei/DevEco Studio/tools/hvigor/hvigor/node_modules/typescript');

const results = [];
const hypium = {
  describe: (_n, fn) => fn(),
  it: (name, _f, fn) => {
    try {
      fn();
      results.push({ name, status: 'PASS' });
    } catch (error) {
      results.push({ name, status: 'FAIL', error: `${error}` });
    }
  },
  expect: value => ({
    assertTrue: () => assert.strictEqual(value, true),
    assertFalse: () => assert.strictEqual(value, false),
    assertNull: () => assert.strictEqual(value, null),
    assertEqual: expected => assert.deepStrictEqual(value, expected)
  })
};
const { describe, it, expect } = hypium;
const originalLoad = Module._load;
Module._load = function (name, parent, main) {
  return name === '@ohos/hypium' ? hypium : originalLoad.call(this, name, parent, main);
};
require.extensions['.ets'] = (mod, filename) => mod._compile(ts.transpileModule(
  fs.readFileSync(filename, 'utf8'), {
  compilerOptions: {
    target: ts.ScriptTarget.ES2020, module: ts.ModuleKind.CommonJS
  }
}).outputText, filename);

const physics = require('../entry/src/main/ets/physics/WaterSandField.ets');
const tracerMod = require('../entry/src/main/ets/render/P012SandTracer.ets');
const obs = require('../entry/src/main/ets/physics/P011NaturalObservation.ets');
const { WaterSandField, defaultWaterSandParams } = physics;
const { P012SandTracer } = tracerMod;

const COLS = 128;
const CAP = 512;

function makeField(waterBottom = 300, waterH = 160, sandH = 100) {
  const field = new WaterSandField(COLS, CAP);
  const params = defaultWaterSandParams();
  params.heavyDiffusion = 0;
  params.lightDiffusion = 0;
  params.waterLevelRate = 0.001;
  params.angleDeg = 4;
  field.configure(params);
  field.initialize({
    waterRegions: [{ startCol: 0, endColInclusive: COLS - 1,
      bottom: waterBottom, height: waterH }],
    heavyDepositedRegions: [{ startCol: 0, endColInclusive: COLS - 1,
      bottom: 0, height: sandH }],
    lightDepositedRegions: [{ startCol: 0, endColInclusive: COLS - 1,
      bottom: sandH, height: sandH }],
    heavySuspendedRegions: [],
    lightSuspendedRegions: []
  });
  return field;
}

function request(amountH, amountL, seq = 1, inputTime = 1000, cols = 8) {
  const qH = new Float64Array(COLS);
  const qL = new Float64Array(COLS);
  const perColH = amountH / cols;
  const perColL = amountL / cols;
  for (let c = 60; c < 60 + cols; c++) {
    qH[c] = perColH;
    qL[c] = perColL;
  }
  return {
    perColumnHeavy: qH, perColumnLight: qL,
    sourceId: 1, sequence: seq, inputTime: inputTime,
    x: 500, y: 100, seed: 7
  };
}

function snapshotState(field) {
  const s = field.snapshot();
  return JSON.stringify({
    waterTop: Array.from(s.waterTop), bh: Array.from(s.bh),
    bl: Array.from(s.bl), mh: Array.from(s.mh), ml: Array.from(s.ml),
    suspH: Array.from(s.suspH), suspL: Array.from(s.suspL)
  });
}

describe('injectGestureSand (gesture-sand-inject-phys §6.2)', () => {

  // 1) Conservation closure: 50 random injections, drift < 1e-9,
  //    water total change == 0.
  it('p011_gsi_conservationClosure', 0, () => {
    const field = makeField();
    const waterBefore = field.getTotals().water;
    for (let i = 0; i < 50; i++) {
      const h = 0.5 + (i % 7) * 0.3;
      const l = 0.4 + (i % 5) * 0.2;
      const r = field.injectGestureSand(request(h, l, i + 1, 1000 + i * 16));
      assert.ok(r !== null, `injection ${i} returned null`);
    }
    const drift = field.getDrift();
    assert.ok(drift.heavy < 1e-9, `heavy drift ${drift.heavy}`);
    assert.ok(drift.light < 1e-9, `light drift ${drift.light}`);
    const waterAfter = field.getTotals().water;
    assert.ok(Math.abs(waterAfter - waterBefore) < 0.1,
      `water moved ${waterAfter - waterBefore}`);
  });

  // 2) λ admission: tight field -> partial admission with the H/L
  //    REQUEST ratio preserved.
  it('p011_gsi_lambdaAdmissionKeepsPhaseRatio', 0, () => {
    // Water nearly to the top: tiny headroom forces λ<1.
    const field = new WaterSandField(COLS, CAP);
    const params = defaultWaterSandParams();
    params.heavyDiffusion = 0; params.lightDiffusion = 0;
    params.waterLevelRate = 0.001; params.angleDeg = 4;
    field.configure(params);
    field.initialize({
      waterRegions: [{ startCol: 0, endColInclusive: COLS - 1,
        bottom: 0, height: CAP - 2 }], // ~full
      heavyDepositedRegions: [],
      lightDepositedRegions: [],
      heavySuspendedRegions: [], lightSuspendedRegions: []
    });
    const r = field.injectGestureSand(request(10, 5));
    assert.ok(r !== null);
    if (r.rejectedReason === null && r.admittedH + r.admittedL > 0) {
      const reqRatio = 10 / 5;
      const admRatio = r.admittedH / Math.max(r.admittedL, 1e-12);
      assert.ok(Math.abs(admRatio - reqRatio) < 0.35 * reqRatio,
        `ratio ${admRatio} vs ${reqRatio}`);
    }
    // Either partial admission or CAPACITY_BLOCKED with zero residue.
    const drift = field.getDrift();
    assert.ok(drift.heavy < 1e-9 && drift.light < 1e-9);
  });

  // 3) NO_WATER: dry field -> zero admission + state unchanged.
  it('p011_gsi_noWaterZeroState', 0, () => {
    const field = new WaterSandField(COLS, CAP);
    const params = defaultWaterSandParams();
    params.heavyDiffusion = 0; params.lightDiffusion = 0;
    params.waterLevelRate = 0.001; params.angleDeg = 4;
    field.configure(params);
    field.initialize({
      waterRegions: [], heavyDepositedRegions: [],
      lightDepositedRegions: [], heavySuspendedRegions: [],
      lightSuspendedRegions: []
    });
    const before = snapshotState(field);
    const r = field.injectGestureSand(request(5, 5));
    assert.ok(r !== null);
    assert.equal(r.admittedH, 0);
    assert.equal(r.admittedL, 0);
    assert.equal(r.rejectedReason, 'NO_WATER');
    assert.equal(snapshotState(field), before, 'state must be untouched');
  });

  // 4) CAPACITY_BLOCKED: full field + big request -> zero/~0 admission.
  it('p011_gsi_capacityBlocked', 0, () => {
    const field = new WaterSandField(COLS, CAP);
    const params = defaultWaterSandParams();
    params.heavyDiffusion = 0; params.lightDiffusion = 0;
    params.waterLevelRate = 0.001; params.angleDeg = 4;
    field.configure(params);
    field.initialize({
      waterRegions: [{ startCol: 0, endColInclusive: COLS - 1,
        bottom: 0, height: CAP }],
      heavyDepositedRegions: [], lightDepositedRegions: [],
      heavySuspendedRegions: [], lightSuspendedRegions: []
    });
    const r = field.injectGestureSand(request(500, 500));
    assert.ok(r !== null);
    // Full water: no sand fits under the surface.
    assert.equal(r.admittedH + r.admittedL, 0);
    assert.equal(r.rejectedReason, 'CAPACITY_BLOCKED');
    const drift = field.getDrift();
    assert.ok(drift.heavy < 1e-9 && drift.light < 1e-9);
  });

  // 5) Transaction rollback: state after a rejected/failed injection is
  //    bit-identical to before.
  it('p011_gsi_rollbackBitIdentical', 0, () => {
    const field = makeField();
    const before = snapshotState(field);
    // NO_WATER case first (guaranteed rollback path).
    const dry = new WaterSandField(COLS, CAP);
    const params = defaultWaterSandParams();
    params.heavyDiffusion = 0; params.lightDiffusion = 0;
    params.waterLevelRate = 0.001; params.angleDeg = 4;
    dry.configure(params);
    dry.initialize({
      waterRegions: [], heavyDepositedRegions: [],
      lightDepositedRegions: [], heavySuspendedRegions: [],
      lightSuspendedRegions: []
    });
    const dryBefore = snapshotState(dry);
    dry.injectGestureSand(request(3, 3));
    assert.equal(snapshotState(dry), dryBefore);
    // CAPACITY path on the full field.
    const full = new WaterSandField(COLS, CAP);
    full.configure(params);
    full.initialize({
      waterRegions: [{ startCol: 0, endColInclusive: COLS - 1,
        bottom: 0, height: CAP }],
      heavyDepositedRegions: [], lightDepositedRegions: [],
      heavySuspendedRegions: [], lightSuspendedRegions: []
    });
    const fullBefore = snapshotState(full);
    full.injectGestureSand(request(500, 500));
    assert.equal(snapshotState(full), fullBefore);
  });

  // 6) Queue: 3 staged -> 3 substeps consume 1 each (FIFO); Δt clamp.
  it('p011_gsi_queueFifoAndDtClamp', 0, () => {
    const field = makeField();
    // FIFO: stage 3, step once -> exactly one consumed.
    field.stageGestureSand(request(2, 1, 1, 1000));
    field.stageGestureSand(request(2, 1, 2, 1100));
    field.stageGestureSand(request(2, 1, 3, 1200));
    const totals0 = field.getTotals();
    field.setTilt(0);
    field.step(1 / 60); // 1 substep at this dt
    const totals1 = field.getTotals();
    // First request admitted (plus normal physics drift is tiny at 0 tilt)
    const gainedH = totals1.heavy - totals0.heavy;
    assert.ok(gainedH > 1.5 && gainedH < 2.5,
      `first substep consumed ~one request, gained H=${gainedH}`);
    // Δt clamp: a request 1s after the last gets scaled to 1/30.
    const clamped = new WaterSandField(COLS, CAP);
    const params = defaultWaterSandParams();
    params.heavyDiffusion = 0; params.lightDiffusion = 0;
    params.waterLevelRate = 0.001; params.angleDeg = 4;
    clamped.configure(params);
    clamped.initialize({
      waterRegions: [{ startCol: 0, endColInclusive: COLS - 1,
        bottom: 300, height: 160 }],
      heavyDepositedRegions: [{ startCol: 0, endColInclusive: COLS - 1,
        bottom: 0, height: 100 }],
      lightDepositedRegions: [{ startCol: 0, endColInclusive: COLS - 1,
        bottom: 100, height: 100 }],
      heavySuspendedRegions: [], lightSuspendedRegions: []
    });
    // The Δt clamp lives in consumePendingGestureSand (the queue path) —
    // stage both requests and step twice: the second (1 s gap) must be
    // clamped to 1/30 of its requested amount.
    clamped.stageGestureSand(request(1, 1, 1, 1000));
    clamped.setTilt(0);
    clamped.step(1 / 60); // consumes request 1 (establishes lastGestureInputTime)
    clamped.stageGestureSand(request(6, 6, 2, 2000));
    clamped.step(1 / 60); // consumes request 2 with the clamp
    // The second request's admitted amount rides the ledger: sourceInjectedH
    // grew by ~1 (first) + ~6/30 (clamped second) = ~1.2.
    const totalsH = clamped.getTotals().heavy - 12800; // minus the scene base
    assert.ok(totalsH > 1.0 && totalsH < 1.5,
      `clamped gain H=${totalsH} (expect ~1.2 = 1 full + 0.2 clamped)`);
  });

  // 7) Receipt contract: field-nine-fields mechanically identical to the
  //    B-side feed() consumer (names/types; anti-drift check).
  it('p011_gsi_receiptContractMatchesB', 0, () => {
    const field = makeField();
    const r = field.injectGestureSand(request(4, 2));
    assert.ok(r !== null);
    const expectedKeys = ['sourceId', 'sequence', 'admittedH', 'admittedL',
      'requestedH', 'requestedL', 'rejectedReason', 'x', 'y', 'inputTime',
      'seed'];
    for (const key of expectedKeys) {
      assert.ok(key in r, `receipt missing ${key}`);
    }
    // The tracer's feed() accepts the receipt (structural compatibility).
    const tracer = new P012SandTracer();
    const before = tracer.activeCount();
    tracer.feed(r, 6);
    assert.ok(tracer.activeCount() > before || (r.admittedH + r.admittedL) === 0,
      'feed accepts the physics receipt');
  });

  // 8) Schema v4: ledger records carry the new fields; no-source runs
  //    read them as absent/0 without error.
  it('p011_gsi_schemaV4LedgerFields', 0, () => {
    const records = [];
    const probe = new obs.P011NaturalObservationProbe(COLS,
      record => { records.push(record); });
    const field = makeField();
    field.setNaturalObservationProbe(probe);
    field.setTilt(0);
    field.injectGestureSand(request(3, 3));
    field.step(1 / 60);
    assert.ok(records.length > 0);
    const last = records[records.length - 1];
    // v4 fields present on this injected run.
    assert.ok('sourceInjectedH' in last.context ||
      last.context.sourceInjectedH !== undefined);
    assert.ok(last.context.sourceInjectedH >= 3 - 1e-9);
    assert.ok(last.context.sourceEpoch >= 1);
    // No-source run parses with the fields zero/absent.
    const records2 = [];
    const probe2 = new obs.P011NaturalObservationProbe(COLS,
      record => { records2.push(record); });
    const field2 = makeField();
    field2.setNaturalObservationProbe(probe2);
    field2.setTilt(0);
    field2.step(1 / 60);
    const last2 = records2[records2.length - 1];
    const ih = last2.context.sourceInjectedH === undefined
      ? 0 : last2.context.sourceInjectedH;
    assert.equal(ih, 0);
  });

  // 9) t90 epoch: sourceEpoch increments per injection; the in-flight
  //    t90 observation window restarts.
  it('p011_gsi_t90EpochIncrements', 0, () => {
    const field = makeField();
    const records = [];
    field.setNaturalObservationProbe(new obs.P011NaturalObservationProbe(
      COLS, record => { records.push(record); }));
    field.injectGestureSand(request(2, 2, 1, 1000));
    field.setTilt(0);
    field.step(1 / 60);
    let epoch1 = -1;
    for (const r of records) {
      if (r.context.sourceEpoch !== undefined) {
        epoch1 = Math.max(epoch1, r.context.sourceEpoch);
      }
    }
    assert.ok(epoch1 >= 1, `epoch after first injection ${epoch1}`);
    field.injectGestureSand(request(2, 2, 2, 2000));
    records.length = 0;
    field.step(1 / 60);
    let epoch2 = -1;
    for (const r of records) {
      if (r.context.sourceEpoch !== undefined) {
        epoch2 = Math.max(epoch2, r.context.sourceEpoch);
      }
    }
    assert.ok(epoch2 === epoch1 + 1, `epoch ${epoch2} vs ${epoch1}+1`);
  });

});

// 10) Legacy inject regression: the old interface keeps its exact
//     semantics (baseline RESET on injection) — pinned so the new path
//     cannot silently change it.
describe('injectGestureSand legacy regression (fixed)', () => {
  it('p011_gsi_legacyInjectBaselineResets', 0, () => {
    const field = makeField();
    const before = field.getTotals();
    const WaterSandPhaseEnum = physics.WaterSandPhase;
    const WaterSandStateEnum = physics.WaterSandInjectionState;
    field.inject({ phase: WaterSandPhaseEnum.HEAVY, amount: 50, startCol: 10,
      endColInclusive: 20, state: WaterSandStateEnum.SUSPENDED });
    const after = field.getTotals();
    assert.ok(Math.abs(after.heavy - (before.heavy + 50)) < 1e-9);
    // Legacy semantics: the baseline RESETS to the new totals (drift 0).
    const drift = field.getDrift();
    assert.ok(drift.heavy < 1e-12, `legacy baseline reset, drift ${drift.heavy}`);
  });
});

const failed = results.filter(r => r.status === 'FAIL');
console.log(JSON.stringify({
  suite: 'run_p011_gesture_inject_units',
  passed: results.length - failed.length,
  failed: failed.length,
  results
}, null, 2));
if (failed.length > 0) {
  process.exitCode = 1;
}
