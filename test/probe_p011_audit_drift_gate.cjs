// Probe: does audit(true) at substep end use raw initial or expected
// (initial + sourceInjected) after a sustained gesture injection?
'use strict';
const fs = require('fs');
const Module = require('module');
const ts = require('C:/Program Files/Huawei/DevEco Studio/tools/hvigor/hvigor/node_modules/typescript');
const hypium = { describe: (_n, fn) => fn(), it: (name, _f, fn) => { try { fn(); results.push({ name, status: 'PASS' }); } catch (e) { results.push({ name, status: 'FAIL', error: `${e}` }); } }, expect: v => ({ assertTrue: () => {}, assertEqual: () => {} }) };
const results = [];
const { describe, it } = hypium;
const originalLoad = Module._load;
Module._load = function (name, parent, main) { return name === '@ohos/hypium' ? hypium : originalLoad.call(this, name, parent, main); };
require.extensions['.ets'] = (mod, filename) => mod._compile(ts.transpileModule(fs.readFileSync(filename, 'utf8'), { compilerOptions: { target: ts.ScriptTarget.ES2020, module: ts.ModuleKind.CommonJS } }).outputText, filename);
const { WaterSandField, defaultWaterSandParams } = require('../entry/src/main/ets/physics/WaterSandField.ets');
const COLS = 128, CAP = 512;
function makeField() {
  const field = new WaterSandField(COLS, CAP);
  const params = defaultWaterSandParams();
  params.heavyDiffusion = 0; params.lightDiffusion = 0;
  params.waterLevelRate = 0.001; params.angleDeg = 4;
  field.configure(params);
  field.initialize({
    waterRegions: [{ startCol: 0, endColInclusive: COLS - 1, bottom: 300, height: 160 }],
    heavyDepositedRegions: [{ startCol: 0, endColInclusive: COLS - 1, bottom: 0, height: 100 }],
    lightDepositedRegions: [{ startCol: 0, endColInclusive: COLS - 1, bottom: 100, height: 100 }],
    heavySuspendedRegions: [], lightSuspendedRegions: []
  });
  return field;
}
function request(amountH, amountL, seq, inputTime, cols = 8) {
  const qH = new Float64Array(COLS), qL = new Float64Array(COLS);
  for (let c = 60; c < 60 + cols; c++) { qH[c] = amountH / cols; qL[c] = amountL / cols; }
  return { perColumnHeavy: qH, perColumnLight: qL, sourceId: 1, sequence: seq, inputTime, x: 0, y: 0, seed: 1 };
}
describe('probe audit drift denominator', () => {
  it('sustained injection then step', 0, () => {
    const field = makeField();
    const initH = field.getTotals().heavy, initL = field.getTotals().light;
    console.log('initialHeavy', initH, 'initialLight', initL);
    // Sustained gesture: 8 unit/s x ~30s worth in 0.5-unit packets via step path
    let admittedTotal = 0;
    for (let i = 0; i < 60; i++) {
      const r = field.injectGestureSand(request(0.4, 0.4, i + 1, 1000 + i * 16));
      if (r) admittedTotal += r.admittedH + r.admittedL;
    }
    console.log('admittedTotal', admittedTotal, 'ratio', admittedTotal / initH);
    // Now run a step — audit(true) at substep end computes drift vs initial*
    const res = field.step(1 / 60);
    console.log('step result:', JSON.stringify(res).slice(0, 400));
    console.log('getDrift:', JSON.stringify(field.getDrift()));
    console.log('getMetrics errorCode-ish fields:', field.getMetrics().driftHeavy, field.getMetrics().driftLight);
  });
});

describe('probe freeze persistence', () => {
  it('second step after freeze', 0, () => {
    const field = makeField();
    for (let i = 0; i < 60; i++) field.injectGestureSand(request(0.4, 0.4, i + 1, 1000 + i * 16));
    const r1 = field.step(1 / 60);
    const r2 = field.step(1 / 60);
    console.log('first step ok:', r1.ok, 'err:', r1.errorCode, '| second step ok:', r2.ok, 'err:', r2.errorCode, 'stepped:', r2.stepped);
  });
});
