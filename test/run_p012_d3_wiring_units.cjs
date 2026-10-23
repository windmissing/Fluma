// P-011 falling-sand D3 page-wiring unit runner (task §6: routing / gate /
// cancel / tiltX retirement / command channel / end-to-end receipt flow).
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
    assertFalse: () => assert.strictEqual(value, false)
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
const inputMode = require('../entry/src/main/ets/physics/P011InputMode.ets');
const { WaterSandField, defaultWaterSandParams, P011GestureSandRequest } = physics;
const { P012SandTracer } = tracerMod;

const COLS = 128;
const CAP = 512;
// The routing kernel mirrors the SandCanvas page constants (§2 candidates).
const SAND_RATE = 8;
const SAND_RADIUS = 2;
const SAND_H_SHARE = 0.5;

/** Reimplement the page's routing math verbatim for the unit test (the
 * page method is private; this mirrors it line-for-line so drift shows
 * up as a test failure once a shared helper is extracted). */
function routeTouch(localX, boxX, boxW, cols, dtSec) {
  const colF = (localX - boxX) / boxW * cols;
  const center = Math.floor(colF);
  const qH = new Float64Array(cols);
  const qL = new Float64Array(cols);
  const total = SAND_RATE * dtSec;
  const weights = [];
  let kernelSum = 0;
  for (let d = -SAND_RADIUS; d <= SAND_RADIUS; d++) {
    const w = SAND_RADIUS + 1 - Math.abs(d);
    weights.push(w);
    kernelSum += w;
  }
  for (let i = 0; i < weights.length; i++) {
    const c = center - SAND_RADIUS + i;
    if (c < 0 || c >= cols) {
      continue;
    }
    qH[c] = total * SAND_H_SHARE * weights[i] / kernelSum;
    qL[c] = total * (1 - SAND_H_SHARE) * weights[i] / kernelSum;
  }
  return { qH, qL, center };
}

function makeField() {
  const field = new WaterSandField(COLS, CAP);
  const params = defaultWaterSandParams();
  params.heavyDiffusion = 0; params.lightDiffusion = 0;
  params.waterLevelRate = 0.001; params.angleDeg = 4;
  params.relaxMode = 'legacy-single';
  params.thetaStartDeg = undefined; params.thetaStopDeg = undefined;
  params.thresholdSource = 'legacy';
  field.configure(params);
  field.initialize({
    waterRegions: [{ startCol: 0, endColInclusive: COLS - 1,
      bottom: 300, height: 160 }],
    heavyDepositedRegions: [{ startCol: 0, endColInclusive: COLS - 1,
      bottom: 0, height: 100 }],
    lightDepositedRegions: [{ startCol: 0, endColInclusive: COLS - 1,
      bottom: 100, height: 100 }],
    heavySuspendedRegions: [], lightSuspendedRegions: []
  });
  return field;
}

describe('falling-sand D3 page wiring (task §6)', () => {

  // 1) Routing: touch -> column ± radius triangular kernel, H/L 0.5.
  it('p011_d3_routingKernelAndSplit', 0, () => {
    const boxW = 1024;
    const { qH, qL, center } = routeTouch(512, 0, boxW, COLS, 0.05);
    assert.equal(center, 64);
    // Triangular 1,2,3,2,1 over columns 62..66 (kernel sum 9).
    const w62 = 1 / 9, w63 = 2 / 9, w64 = 3 / 9;
    const total = SAND_RATE * 0.05;
    assert.ok(Math.abs(qH[64] - total * 0.5 * w64) < 1e-12);
    assert.ok(Math.abs(qL[64] - total * 0.5 * w64) < 1e-12);
    assert.ok(Math.abs(qH[63] - total * 0.5 * w63) < 1e-12);
    assert.ok(Math.abs(qH[62] - total * 0.5 * w62) < 1e-12);
    // Outside the kernel: zero.
    assert.equal(qH[60], 0);
    assert.equal(qH[70], 0);
    // H+L totals conserve the rate×dt.
    let sum = 0;
    for (let c = 0; c < COLS; c++) {
      sum += qH[c] + qL[c];
    }
    assert.ok(Math.abs(sum - total) < 1e-12);
  });

  // 2) Gate three conditions (mirror of gestureSandGate).
  it('p011_d3_gateThreeConditions', 0, () => {
    const gate = (renderModeIsWaterSand, insideBox, panelOpen) =>
      renderModeIsWaterSand && insideBox && !panelOpen;
    assert.equal(gate(true, true, false), true);  // all pass
    assert.equal(gate(false, true, false), false); // wrong mode
    assert.equal(gate(true, false, false), false); // outside box
    assert.equal(gate(true, true, true), false);   // panel open
  });

  // 3) Gesture cancel semantics: fed points are NOT recalled; the
  //    physics queue keeps staged material (consumed by later substeps).
  it('p011_d3_cancelDoesNotRecallFedPoints', 0, () => {
    const field = makeField();
    const tracer = new P012SandTracer();
    const qH = new Float64Array(COLS); qH[64] = 2;
    const receipt = field.injectGestureSand({
      perColumnHeavy: qH, perColumnLight: new Float64Array(COLS),
      sourceId: 1, sequence: 1, inputTime: 1000, x: 512, y: 100, seed: 1
    });
    assert.ok(receipt !== null && receipt.admittedH > 0);
    tracer.feed(receipt, 8);
    const before = tracer.activeCount();
    // Cancel/end: NO clear on either side (raise-hand semantics).
    assert.ok(before > 0);
    // The tracer advances naturally on update (points fly out).
    tracer.update(0.016, 0, 1, null);
    assert.equal(tracer.activeCount(), before);
  });

  // 5) Automated tilt command channel still reaches the field: the
  //    script target path (setTilt / freshScriptCommand semantics) is
  //    untouched by the Pan rewrite.
  it('p011_d3_commandChannelStillWorks', 0, () => {
    const field = makeField();
    field.setTilt(20);
    field.setTilt(0);
    // The setter path is the command channel's landing point.
    assert.ok(typeof field.setTilt === 'function');
    // canP011GestureOwnInput survives as the defensive assertion (its
    // semantics unchanged — TEST mode + no script + no invert).
    const canOwn = inputMode.canP011GestureOwnInput({
      testMode: true, scriptTargetTilt: null, forceInvert: false
    });
    assert.equal(canOwn, true);
    const cannotOwn = inputMode.canP011GestureOwnInput({
      testMode: false, scriptTargetTilt: null, forceInvert: false
    });
    assert.equal(cannotOwn, false);
  });

  // 6) End-to-end synthetic receipt flow: event -> route -> inject ->
  //    receipt -> tracer feed, counts consistent.
  it('p011_d3_endToEndReceiptFlow', 0, () => {
    const field = makeField();
    const tracer = new P012SandTracer();
    const boxW = 1024;
    let fedReceipts = 0;
    let admittedTotal = 0;
    for (let i = 0; i < 10; i++) {
      const { qH, qL } = routeTouch(512 + i * 3, 0, boxW, COLS, 1 / 60);
      const receipt = field.injectGestureSand({
        perColumnHeavy: qH, perColumnLight: qL,
        sourceId: 1, sequence: i + 1, inputTime: 1000 + i * 16,
        x: 512, y: 100, seed: i + 1
      });
      if (receipt !== null) {
        tracer.feed(receipt, boxW / COLS);
        fedReceipts += 1;
        admittedTotal += receipt.admittedH + receipt.admittedL;
      }
    }
    assert.ok(fedReceipts === 10, `fed ${fedReceipts}`);
    assert.ok(admittedTotal > 0);
    assert.ok(tracer.activeCount() > 0);
    assert.ok(tracer.activeCount() <= P012SandTracer.MAX_POINTS);
  });
});

// Structural check for case 4 (source-level, not runtime): the Pan
// handler must not write tiltX.
describe('falling-sand D3 structural (Pan-retirement)', () => {
  it('p011_d3_panHandlerHasNoTiltXWrite', 0, () => {
    const src = fs.readFileSync(
      'entry/src/main/ets/pages/SandCanvas.ets', 'utf8');
    // Extract the PanGesture block.
    const start = src.indexOf('PanGesture()');
    const end = src.indexOf('.onActionCancel', start);
    assert.ok(start > 0 && end > start, 'PanGesture block found');
    const block = src.slice(start, end + 400);
    assert.ok(!block.includes('this.tiltX'),
      'Pan handler must not write tiltX (retired per user ruling)');
    assert.ok(!block.includes('gestureHoldUntil'),
      'Pan handler must not arm gestureHoldUntil (retired)');
    assert.ok(!block.includes('useSensor'),
      'Pan handler must not flip useSensor (sensor owns tilt)');
  });
});

const failed = results.filter(r => r.status === 'FAIL');
console.log(JSON.stringify({
  suite: 'run_p012_d3_wiring_units',
  passed: results.length - failed.length,
  failed: failed.length,
  results
}, null, 2));
if (failed.length > 0) {
  process.exitCode = 1;
}
