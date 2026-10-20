// P-012 E-batch acceptance unit runner (Node 1:1 channel).
// Executes the real ArkTS product code through the bundled TypeScript
// compiler — same channel as run_p011_planning_units.cjs — and adds the
// E-batch specific evidence:
//   * adapter conservation state machine (H/L independent, inject break,
//     same-snapshot zero advection, not-finite guard, frame gap, epoch)
//   * OFF-branch decision purity (canonical forced OFF / disposed renderer)
//   * E-tier draw budget constant
//   * mock-canvas draw counting for the E pass (<= 16 submissions)
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
const adapterMod = require('../entry/src/main/ets/render/P012WaterSandAdapter.ets');
const rendererMod = require('../entry/src/main/ets/render/P012WaterSandRenderer.ets');
const typesMod = require('../entry/src/main/ets/render/P012RenderTypes.ets');
const { WaterSandField } = physics;
const { P012WaterSandAdapter } = adapterMod;
const { P012WaterSandRenderer, p012ShouldUseP012Renderer } = rendererMod;
const { P012AppearanceTier, P012InvalidReason, P012_E_BATCH_DRAW_BUDGET } = typesMod;

function fullScene(cols) {
  return {
    waterRegions: [{ startCol: 0, endColInclusive: cols - 1, bottom: 120, height: 60 }],
    heavyDepositedRegions: [{ startCol: 0, endColInclusive: cols - 1, bottom: 0, height: 50 }],
    lightDepositedRegions: [{ startCol: 0, endColInclusive: cols - 1, bottom: 50, height: 50 }],
    heavySuspendedRegions: [{ startCol: 0, endColInclusive: cols - 1, bottom: 110, height: 10 }],
    lightSuspendedRegions: [{ startCol: 0, endColInclusive: cols - 1, bottom: 110, height: 10 }]
  };
}

let serial = 0;
function makeContext(over) {
  serial++;
  const ctx = {
    fieldEpoch: 1, continuityEpoch: 1, geometryEpoch: 1,
    frameSerial: serial, sampleTimeMs: 1000 + serial * 16.7, frameDeltaMs: 16.7,
    inputMode: 'TEST', sourceKind: 'script',
    physicalRouteId: 'test-route', runIdentity: 'run-1',
    boxX: 10, boxY: 20, boxW: 640, boxH: 900, pixelRatio: 1,
    visualInverted: false,
    appearanceTier: P012AppearanceTier.E,
    renderSeed: 20261020, configRevision: 1,
    canonicalEvidence: false
  };
  return Object.assign(ctx, over || {});
}

// ── E-batch directed evidence (beyond the hypium suite) ──

// □A flux proxy semantics: consecutive valid pairs over a tilting run must
// never report conservation failure (physics conserves; Kahan tolerance holds),
// and an inject must flip CONSERVATION_H exactly once then recover.
(function fluxProxySemantics() {
  const cols = 32;
  const field = new WaterSandField(cols, 192);
  field.initialize(fullScene(cols));
  const adapter = new P012WaterSandAdapter(cols, 192);
  adapter.update(field.snapshot(), makeContext());
  let conservationFails = 0;
  for (let i = 0; i < 120; i++) {
    field.setTilt(i % 40 < 20 ? 18 : -18);
    field.step(1 / 60);
    const r = adapter.update(field.snapshot(), makeContext());
    if (!r.pair.valid) {
      conservationFails++;
    }
  }
  results.push({
    name: 'p012_adapter_longRun_neverConservationFails',
    status: conservationFails === 0 ? 'PASS' : 'FAIL',
    error: `conservationFails=${conservationFails}`
  });
})();

// □B inject: H breaks (L stays conserved within the check), next frame recovers.
(function injectBreaksHOnly() {
  const cols = 32;
  const field = new WaterSandField(cols, 192);
  field.initialize(fullScene(cols));
  field.step(1 / 60);
  const adapter = new P012WaterSandAdapter(cols, 192);
  adapter.update(field.snapshot(), makeContext());
  field.inject({ phase: 1, amount: 40, startCol: 8, endColInclusive: 15, state: 0 });
  const r = adapter.update(field.snapshot(), makeContext());
  const ok = r.pair.valid === false &&
    r.pair.reason === P012InvalidReason.CONSERVATION_H &&
    r.conservation !== null && r.conservation.conservedL === true;
  field.step(1 / 60);
  const r2 = adapter.update(field.snapshot(), makeContext());
  results.push({
    name: 'p012_adapter_injectHOnlyBreakAndRecover',
    status: ok && r2.pair.valid === true ? 'PASS' : 'FAIL',
    error: `reason=${r.pair.reason} recovered=${r2.pair.valid}`
  });
})();

// □C L-inject must break CONSERVATION_L (H/L independent checks).
(function injectBreaksLOnly() {
  const cols = 32;
  const field = new WaterSandField(cols, 192);
  field.initialize(fullScene(cols));
  field.step(1 / 60);
  const adapter = new P012WaterSandAdapter(cols, 192);
  adapter.update(field.snapshot(), makeContext());
  field.inject({ phase: 2, amount: 40, startCol: 8, endColInclusive: 15, state: 0 });
  const r = adapter.update(field.snapshot(), makeContext());
  const ok = r.pair.valid === false &&
    r.pair.reason === P012InvalidReason.CONSERVATION_L &&
    r.conservation !== null && r.conservation.conservedH === true;
  results.push({
    name: 'p012_adapter_injectLOnlyBreak',
    status: ok ? 'PASS' : 'FAIL',
    error: `reason=${r.pair.reason}`
  });
})();

// □D mock canvas: E pass draw submissions must stay <= 16 for a tilted run.
(function drawBudgetMockCanvas() {
  const draws = [];
  const ctx = new Proxy({}, {
    get(_t, prop) {
      if (prop === 'fill' || prop === 'stroke' || prop === 'fillRect' || prop === 'strokeRect') {
        return () => { draws.push(prop); };
      }
      if (prop === 'save' || prop === 'restore' || prop === 'beginPath' ||
        prop === 'closePath' || prop === 'clip') {
        return () => {};
      }
      if (prop === 'moveTo' || prop === 'lineTo' || prop === 'translate' || prop === 'rotate' ||
        prop === 'rect') {
        return () => {};
      }
      if (prop === 'createLinearGradient') {
        return () => ({ addColorStop: () => {} });
      }
      return undefined;
    },
    set(_t, _prop, _value) { return true; }
  });
  const cols = 128;
  const field = new WaterSandField(cols, 192);
  field.initialize(fullScene(cols));
  field.setTilt(15);
  const renderer = new P012WaterSandRenderer(cols, 192);
  let worst = 0;
  for (let i = 0; i < 60; i++) {
    field.step(1 / 60);
    draws.length = 0;
    renderer.render(ctx, field.snapshot(), makeContext());
    worst = Math.max(worst, draws.length);
  }
  results.push({
    name: 'p012_renderer_drawBudget_worst60Frames',
    status: worst <= P012_E_BATCH_DRAW_BUDGET ? 'PASS' : 'FAIL',
    error: `worstDraws=${worst} budget=${P012_E_BATCH_DRAW_BUDGET}`
  });
  // determinism: same seed → same spark tile → same color strings on two renders
  const colors = [];
  const ctx2 = new Proxy({}, {
    get(_t, prop) {
      if (prop === 'fillStyle') { return undefined; }
      if (prop === 'fill' || prop === 'stroke' || prop === 'fillRect' || prop === 'strokeRect') {
        return () => {};
      }
      if (prop === 'save' || prop === 'restore' || prop === 'beginPath' ||
        prop === 'closePath' || prop === 'clip') {
        return () => {};
      }
      if (prop === 'moveTo' || prop === 'lineTo' || prop === 'translate' || prop === 'rotate' ||
        prop === 'rect') {
        return () => {};
      }
      if (prop === 'createLinearGradient') {
        return () => ({ addColorStop: () => {} });
      }
      return undefined;
    },
    set(_t, prop, value) { if (prop === 'fillStyle') { colors.push(`${value}`); } return true; }
  });
  const r1 = new P012WaterSandRenderer(cols, 192);
  const r2 = new P012WaterSandRenderer(cols, 192);
  const snap = field.snapshot();
  colors.length = 0;
  r1.render(ctx2, snap, makeContext());
  const a = colors.slice();
  colors.length = 0;
  r2.render(ctx2, snap, makeContext());
  const same = a.length === colors.length && a.every((v, i) => v === colors[i]);
  results.push({
    name: 'p012_renderer_deterministicSameSeed',
    status: same ? 'PASS' : 'FAIL',
    error: `styleCounts=${a.length}/${colors.length}`
  });
})();

// □E OFF-branch decision table (canonical/disposed/NULL/OFF/E/EA/EAB).
(function offBranchDecision() {
  const renderer = new P012WaterSandRenderer(16, 192);
  const cases = [
    [null, P012AppearanceTier.E, false, false],
    [renderer, P012AppearanceTier.OFF, false, false],
    [renderer, P012AppearanceTier.E, true, false],
    [renderer, P012AppearanceTier.E, false, true],
    [renderer, P012AppearanceTier.EA, false, true],
    [renderer, P012AppearanceTier.EAB, false, true]
  ];
  let allOk = true;
  for (const [r, tier, canon, want] of cases) {
    if (p012ShouldUseP012Renderer(r, tier, canon) !== want) {
      allOk = false;
    }
  }
  renderer.dispose();
  if (p012ShouldUseP012Renderer(renderer, P012AppearanceTier.E, false) !== false) {
    allOk = false;
  }
  results.push({
    name: 'p012_offBranchDecision_table',
    status: allOk ? 'PASS' : 'FAIL',
    error: 'decision table mismatch'
  });
})();

// □G EA-batch: A-tier mist layer draw budget + fog alpha bound + determinism.
(function eaBatchMistBudget() {
  const draws = [];
  const fills = [];
  const ctx = new Proxy({}, {
    get(_t, prop) {
      if (prop === 'fill' || prop === 'stroke' || prop === 'fillRect' || prop === 'strokeRect') {
        return () => { draws.push(prop); };
      }
      if (prop === 'save' || prop === 'restore' || prop === 'beginPath' ||
        prop === 'closePath' || prop === 'clip') {
        return () => {};
      }
      if (prop === 'moveTo' || prop === 'lineTo' || prop === 'translate' || prop === 'rotate' ||
        prop === 'rect') {
        return () => {};
      }
      if (prop === 'createLinearGradient') {
        return () => ({ addColorStop: () => {} });
      }
      return undefined;
    },
    set(_t, prop, value) {
      if (prop === 'fillStyle' && typeof value === 'string' && value.startsWith('rgba(232,240,236')) {
        fills.push(value);
      }
      return true;
    }
  });
  const cols = 128;
  const field = new WaterSandField(cols, 192);
  field.initialize(fullScene(cols));
  field.setTilt(15);
  // 悬浮注入：让水体雾 tau > 0（suspended state=1）
  field.inject({ phase: 1, amount: 60, startCol: 20, endColInclusive: 60, state: 1 });
  field.step(1 / 60);
  const renderer = new P012WaterSandRenderer(cols, 192);
  let worst = 0;
  let worstTotal = 0;
  let alphaOk = true;
  for (let i = 0; i < 60; i++) {
    field.step(1 / 60);
    draws.length = 0;
    fills.length = 0;
    renderer.render(ctx, field.snapshot(), makeContext({ appearanceTier: P012AppearanceTier.EA }));
    worst = Math.max(worst, draws.length - 0);
    // total draws include E pass; A mist uses rgba(232,240,236) fills only
    worstTotal = Math.max(worstTotal, draws.length);
    for (const f of fills) {
      const m = /rgba\(232,240,236,([0-9.]+)\)/.exec(f);
      // 水体雾上限 0.78（标定后）；界面带 0.55——两者都经此色提交，用 0.78 上界
      if (m && parseFloat(m[1]) > 0.78 + 1e-9) {
        alphaOk = false;
      }
    }
  }
  results.push({
    name: 'p012_ea_mistDrawBudget_worst60Frames',
    status: worstTotal <= 64 ? 'PASS' : 'FAIL',
    error: `worstTotalDraws=${worstTotal} (E+A budget 64)`
  });
  results.push({
    name: 'p012_ea_fogAlphaBound',
    status: alphaOk ? 'PASS' : 'FAIL',
    error: alphaOk ? '' : 'fog alpha exceeded 0.55'
  });
  // E tier regression: tier=E must not emit any mist fills
  draws.length = 0;
  fills.length = 0;
  renderer.render(ctx, field.snapshot(), makeContext({ appearanceTier: P012AppearanceTier.E }));
  const eMistFills = fills.length;
  renderer.render(ctx, field.snapshot(), makeContext({ appearanceTier: P012AppearanceTier.EA }));
  const eaMistFills = fills.length;
  results.push({
    name: 'p012_ea_mistOnlyInEATier',
    status: eMistFills === 0 && eaMistFills > 0 ? 'PASS' : 'FAIL',
    error: `eMist=${eMistFills} eaMist=${eaMistFills}`
  });
})();

// □H EA determinism: same seed two renderers → same fill sequences.
(function eaDeterminism() {
  const fillsA = [];
  const fillsB = [];
  const mk = (sink) => new Proxy({}, {
    get(_t, prop) {
      if (prop === 'fill' || prop === 'stroke' || prop === 'fillRect' || prop === 'strokeRect') {
        return () => {};
      }
      if (prop === 'save' || prop === 'restore' || prop === 'beginPath' ||
        prop === 'closePath' || prop === 'clip') {
        return () => {};
      }
      if (prop === 'moveTo' || prop === 'lineTo' || prop === 'translate' || prop === 'rotate' ||
        prop === 'rect') {
        return () => {};
      }
      if (prop === 'createLinearGradient') {
        return () => ({ addColorStop: () => {} });
      }
      return undefined;
    },
    set(_t, prop, value) { if (prop === 'fillStyle') { sink.push(`${value}`); } return true; }
  });
  const cols = 128;
  const field = new WaterSandField(cols, 192);
  field.initialize(fullScene(cols));
  field.setTilt(15);
  field.step(1 / 60);
  const r1 = new P012WaterSandRenderer(cols, 192);
  const r2 = new P012WaterSandRenderer(cols, 192);
  const snap = field.snapshot();
  r1.render(mk(fillsA), snap, makeContext({ appearanceTier: P012AppearanceTier.EA }));
  r2.render(mk(fillsB), snap, makeContext({ appearanceTier: P012AppearanceTier.EA }));
  const same = fillsA.length === fillsB.length &&
    fillsA.every((v, i) => v === fillsB[i]);
  results.push({
    name: 'p012_ea_deterministicSameSeed',
    status: same ? 'PASS' : 'FAIL',
    error: `fillCounts=${fillsA.length}/${fillsB.length}`
  });
})();

// □F adapter slot isolation: page snapshot mutation must not leak into slots.
(function slotIsolation() {
  const cols = 16;
  const field = new WaterSandField(cols, 192);
  field.initialize(fullScene(cols));
  const adapter = new P012WaterSandAdapter(cols, 192);
  const snap = field.snapshot();
  const r1 = adapter.update(snap, makeContext());
  const bh0Before = r1.curr !== null ? r1.curr.bhAt(0) : -1;
  snap.bh[0] += 999;
  const r2 = adapter.update(field.snapshot(), makeContext());
  const ok = bh0Before >= 0 && r2.pair.valid === true;
  results.push({
    name: 'p012_adapter_slotIsolation',
    status: ok ? 'PASS' : 'FAIL',
    error: `bh0Before=${bh0Before} pairValid=${r2.pair.valid}`
  });
})();

// Full hypium suite (state machine details incl. not-finite/frame-gap/epoch/serial).
require('../entry/src/test/P012WaterSandAdapterTest.ets').default();

const output = {
  execution: 'actual-product-ets-via-bundled-typescript',
  results,
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
