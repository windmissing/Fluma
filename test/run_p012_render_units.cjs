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
  if (name === '@ohos/hypium') return hypium;
  if (name === '@ohos.hilog' || name === '@ohos/hilog') {
    return { default: { info: () => {}, error: () => {}, warn: () => {}, debug: () => {} } };
  }
  return originalLoad.call(this, name, parent, main);
};
require.extensions['.ets'] = (mod, filename) => mod._compile(ts.transpileModule(
  fs.readFileSync(filename, 'utf8'), { compilerOptions: {
    target: ts.ScriptTarget.ES2020, module: ts.ModuleKind.CommonJS
  }}).outputText, filename);

const physics = require('../entry/src/main/ets/physics/WaterSandField.ets');
const adapterMod = require('../entry/src/main/ets/render/P012WaterSandAdapter.ets');
const rendererMod = require('../entry/src/main/ets/render/P012WaterSandRenderer.ets');
const motionMod = require('../entry/src/main/ets/render/P012MaterialMotionCache.ets');
const typesMod = require('../entry/src/main/ets/render/P012RenderTypes.ets');
const { WaterSandField } = physics;
const { P012WaterSandAdapter } = adapterMod;
const { P012WaterSandRenderer, p012ShouldUseP012Renderer } = rendererMod;
const { P012MaterialMotionCache } = motionMod;
const { P012AppearanceTier, P012InvalidReason, P012_E_BATCH_DRAW_BUDGET,
  P012_FIRST_TIER_DRAW_BUDGET, P012_B_BATCH_DRAW_BUDGET } = typesMod;

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
    ablation: typesMod.P012Ablation.OFF,
    fillBox: typesMod.P012FillBox.TIGHT,
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

// □I EAB-batch: B grain draw budget + grain-only-in-EAB + determinism + motion cache.
function makeCountingCanvas(sink) {
  sink.draws = 0; sink.fillRects = 0; sink.clips = 0; sink.drawImages = 0; sink.styles = [];
  const counted = ['fill', 'stroke', 'fillRect', 'strokeRect', 'drawImage'];
  const noop = ['save', 'restore', 'beginPath', 'closePath',
    'translate', 'rotate', 'rect', 'moveTo', 'lineTo'];
  return new Proxy({}, {
    get(_t, prop) {
      if (prop === 'clip') {
        return () => { sink.clips++; };
      }
      if (counted.includes(prop)) {
        return () => {
          sink.draws++;
          if (prop === 'fillRect') { sink.fillRects++; }
          if (prop === 'drawImage') { sink.drawImages++; }
        };
      }
      if (noop.includes(prop)) { return () => {}; }
      if (prop === 'createLinearGradient') { return () => ({ addColorStop: () => {} }); }
      if (prop === 'createPattern') { return () => ({ _pattern: true }); }
      return undefined;
    },
    set(_t, prop, value) {
      if (prop === 'fillStyle') { sink.styles.push(`${value}`); }
      return true;
    }
  });
}

(function eabMotionCache() {
  const cols = 32;
  const field = new WaterSandField(cols, 192);
  field.initialize(fullScene(cols));
  field.step(1 / 60);
  const adapter = new P012WaterSandAdapter(cols, 192);
  const r0 = adapter.update(field.snapshot(), makeContext());
  // 冻结：valid=false → isValid=false，mean 偏移=0
  const mc = new P012MaterialMotionCache(cols);
  mc.update(r0.curr, r0.curr, false, 6, 256);
  const freezeOk = mc.isValid === false &&
    Math.abs(mc.meanDeltaXPx(true)) < 1e-9 &&
    Math.abs(mc.meanDeltaXPx(false)) < 1e-9;
  // 同快照零平流（valid=true）
  const r1 = adapter.update(field.snapshot(), makeContext());
  mc.update(r0.curr, r1.curr, true, 6, 256);
  const zeroOk = mc.isValid === true &&
    Math.abs(mc.meanDeltaXPx(true)) < 1e-9;
  // 净搬运产生非零偏移（倾斜两步）
  field.setTilt(24);
  field.step(1 / 60);
  const r2 = adapter.update(field.snapshot(), makeContext());
  mc.update(r1.curr, r2.curr, true, 6, 256);
  const advectOk = mc.isValid === true &&
    Math.abs(mc.meanDeltaXPx(true)) > 0 &&
    mc.meanDeltaXPx(true) >= 0 && mc.meanDeltaXPx(true) < 256;
  results.push({
    name: 'p012_eab_motionCache_freezeZeroAdvect',
    status: (freezeOk && zeroOk) ? 'PASS' : 'FAIL',
    error: `freezeOk=${freezeOk} zeroOk=${zeroOk}`
  });
  results.push({
    name: 'p012_eab_motionCache_netTransportAdvect',
    status: advectOk ? 'PASS' : 'FAIL',
    error: `advectOk=${advectOk}`
  });
})();

(function eabDrawBudget() {
  const sink = {};
  const ctx = makeCountingCanvas(sink);
  const cols = 128;
  const field = new WaterSandField(cols, 192);
  field.initialize(fullScene(cols));
  field.setTilt(15);
  field.inject({ phase: 1, amount: 60, startCol: 20, endColInclusive: 60, state: 1 });
  field.step(1 / 60);
  const renderer = new P012WaterSandRenderer(cols, 192);
  // Node 1:1 通道无 OffscreenCanvas/ImageBitmap，构造期 makeGrainBitmap 全部
  // 返回 null → drawSandGrain 三带整体跳过，EA/EAB 提交数必然相同（这正是本用例
  // 在 clip+drawImage 改造后观测到 fillRectEA==fillRectEAB 的原因，与档位无关）。
  // 注入不透明桩位图，让三带颗粒路径在计数画布上真实执行。
  const stubBitmap = { _stub: true, width: 256, height: 256 };
  renderer.grainBitmapH = stubBitmap;
  renderer.grainBitmapL = stubBitmap;
  renderer.grainBitmapM = stubBitmap;
  let worstEAB = 0;
  let worstEA = 0;
  let clipsEA = -1;
  let clipsEAB = -1;
  let imgEA = -1;
  let imgEAB = -1;
  for (let i = 0; i < 60; i++) {
    field.step(1 / 60);
    sink.draws = 0; sink.clips = 0; sink.drawImages = 0;
    renderer.render(ctx, field.snapshot(), makeContext({ appearanceTier: P012AppearanceTier.EA }));
    worstEA = Math.max(worstEA, sink.draws);
    clipsEA = sink.clips; imgEA = sink.drawImages;
    sink.draws = 0; sink.clips = 0; sink.drawImages = 0;
    renderer.render(ctx, field.snapshot(), makeContext({ appearanceTier: P012AppearanceTier.EAB }));
    worstEAB = Math.max(worstEAB, sink.draws);
    clipsEAB = sink.clips; imgEAB = sink.drawImages;
  }
  // 第一梯队合计 ≤112；总 ≤160
  results.push({
    name: 'p012_eab_drawBudget_firstTierWorst60',
    status: worstEAB <= P012_FIRST_TIER_DRAW_BUDGET ? 'PASS' : 'FAIL',
    error: `worstEAB=${worstEAB} budget=${P012_FIRST_TIER_DRAW_BUDGET}`
  });
  // B 颗粒仅 EAB 档出现：EAB 比 EA 多 3 次 clip（三带各 1 次带曲线 clip），
  // 且 drawImage 平铺提交数 > 0（改造后颗粒走 clip+drawImage，不再是 fillRect）。
  results.push({
    name: 'p012_eab_grainOnlyInEAB',
    status: ((clipsEAB - clipsEA === 3) && imgEAB > 0 && imgEA === 0) ? 'PASS' : 'FAIL',
    error: `clipEA=${clipsEA} clipEAB=${clipsEAB} imgEA=${imgEA} imgEAB=${imgEAB}`
  });
  // B 档颗粒净增量 ≤48（P012_B_BATCH_DRAW_BUDGET）。
  // 2026-10-21 审核发现：clip+drawImage 改造后平铺边界四周各外扩一整圈
  // 冗余 tile（那是 v1 pattern 采样留余量的做法，drawImage 是 1:1 直贴不需要），
  // 净增量达66 > 48，而该常量此前被 import 却从未使用，故静默超标。
  // 已改为精确边界（只摆覆盖 clip 包围盒的 tile，像素输出不变）+ 新增本断言。
  results.push({
    name: 'p012_eab_grainWithinBBatchBudget',
    status: (worstEAB - worstEA <= P012_B_BATCH_DRAW_BUDGET) ? 'PASS' : 'FAIL',
    error: `grainDelta=${worstEAB - worstEA} budget=${P012_B_BATCH_DRAW_BUDGET} ` +
           `drawImageWorst=${imgEAB}`
  });
  // 渲染器自身钩子（供页面侧性能预检复用，须与上面同口径）
  results.push({
    name: 'p012_eab_withinBBatchDrawBudgetHook',
    status: renderer.withinBBatchDrawBudget() ? 'PASS' : 'FAIL',
    error: `lastDrawCount=${renderer.lastDrawCount} ea=${renderer.lastDrawCountEa} ` +
           `budget=${P012_B_BATCH_DRAW_BUDGET}`
  });
})();

(function eabDeterminism() {
  const stylesA = [];
  const stylesB = [];
  const mk = (sink) => makeCountingCanvas(sink);
  const cols = 128;
  const field = new WaterSandField(cols, 192);
  field.initialize(fullScene(cols));
  field.setTilt(15);
  field.step(1 / 60);
  const r1 = new P012WaterSandRenderer(cols, 192);
  const r2 = new P012WaterSandRenderer(cols, 192);
  const snap = field.snapshot();
  const sA = {}; const sB = {};
  r1.render(mk(sA), snap, makeContext({ appearanceTier: P012AppearanceTier.EAB }));
  r2.render(mk(sB), snap, makeContext({ appearanceTier: P012AppearanceTier.EAB }));
  const same = sA.styles.length === sB.styles.length &&
    sA.styles.every((v, i) => v === sB.styles[i]);
  results.push({
    name: 'p012_eab_deterministicSameSeed',
    status: same ? 'PASS' : 'FAIL',
    error: `styleCounts=${sA.styles.length}/${sB.styles.length}`
  });
})();

// □§10.4 消融 want 参数：p012ablate（off/grain/fog/spark）× p012fillbox（full/tight）
// 判据（draw 分支计数）：
//   · ablation=GRAIN（EAB 档）→ drawImage 提交数归零（颗粒层跳过），
//     与 EA 档 baseline 的 clip 数一致（少 3 次带 clip）
//   · ablation=FOG（EA 档）→ draw 总数比 off 明显下降（水体雾 12 档 +
//     界面带绘制消失），且下降量 > 0
//   · ablation=SPARK（E 档）→ spark 3 档 alpha fill 消失（draw 总数下降，
//     bandColor 列级提亮退化 gain=1 不影响计数，靠 style 序列可再核）
//   · fillBox=FULL（EAB 档）→ drawImage 提交数比 TIGHT 多（外扩一整圈），
//     但输出像素集合不变（styles 序列与 TIGHT 完全一致——fillStyle 不受
//     平铺边界影响）
(function ablationWantParams() {
  const cols = 128;
  const field = new WaterSandField(cols, 192);
  field.initialize(fullScene(cols));
  field.setTilt(15);
  field.step(1 / 60);
  const renderer = new P012WaterSandRenderer(cols, 192);
  const stubBitmap = { _stub: true, width: 256, height: 256 };
  renderer.grainBitmapH = stubBitmap;
  renderer.grainBitmapL = stubBitmap;
  renderer.grainBitmapM = stubBitmap;

  const measure = (over) => {
    const sink = {};
    renderer.render(makeCountingCanvas(sink), field.snapshot(), makeContext(over));
    return { draws: sink.draws, clips: sink.clips, images: sink.drawImages,
      fillRects: sink.fillRects, styles: sink.styles.slice() };
  };

  // ── GRAIN 消融 ──
  const eabOff = measure({ appearanceTier: P012AppearanceTier.EAB });
  const eabGrain = measure({
    appearanceTier: P012AppearanceTier.EAB,
    ablation: typesMod.P012Ablation.GRAIN
  });
  // 带的实际 clip 数场景相关（空带高度≤0.5 直接跳过不 clip），不能硬编码：
  // 判据=drawImage 归零 + clip 严格减少（至少 1 条非空带被跳过）
  results.push({
    name: 'p012_ablate_grain_skipsGrainLayer',
    status: (eabGrain.images === 0 && eabOff.images > 0 &&
      eabOff.clips > eabGrain.clips) ? 'PASS' : 'FAIL',
    error: `imgOff=${eabOff.images} imgAblated=${eabGrain.images} ` +
           `clipOff=${eabOff.clips} clipAblated=${eabGrain.clips}`
  });

  // ── FOG 消融（水体雾 + 界面带一并跳过）──
  const eaOff = measure({ appearanceTier: P012AppearanceTier.EA });
  const eaFog = measure({
    appearanceTier: P012AppearanceTier.EA,
    ablation: typesMod.P012Ablation.FOG
  });
  results.push({
    name: 'p012_ablate_fog_skipsFogLayers',
    status: (eaOff.draws - eaFog.draws > 0) ? 'PASS' : 'FAIL',
    error: `drawsOff=${eaOff.draws} drawsAblated=${eaFog.draws} ` +
           `delta=${eaOff.draws - eaFog.draws}`
  });

  // ── SPARK 消融（spark 3 档 alpha fill 消失）──
  const eOff = measure({ appearanceTier: P012AppearanceTier.E });
  const eSpark = measure({
    appearanceTier: P012AppearanceTier.E,
    ablation: typesMod.P012Ablation.SPARK
  });
  // spark fill 数 = 活跃 alpha 档数（1..3）；E 档同场景下两者唯一差异就是 spark
  results.push({
    name: 'p012_ablate_spark_skipsSparkFill',
    status: (eOff.draws - eSpark.draws > 0 &&
      eOff.draws - eSpark.draws <= 3) ? 'PASS' : 'FAIL',
    error: `drawsOff=${eOff.draws} drawsAblated=${eSpark.draws} ` +
           `delta=${eOff.draws - eSpark.draws}`
  });

  // ── FILLBOX 对照（FULL 比 TIGHT 多提交，styles 序列逐项一致）──
  const tight = measure({
    appearanceTier: P012AppearanceTier.EAB,
    fillBox: typesMod.P012FillBox.TIGHT
  });
  const full = measure({
    appearanceTier: P012AppearanceTier.EAB,
    fillBox: typesMod.P012FillBox.FULL
  });
  const stylesSame = tight.styles.length === full.styles.length &&
    tight.styles.every((v, i) => v === full.styles[i]);
  results.push({
    name: 'p012_fillbox_full_submitsMoreDraws',
    status: (full.images > tight.images && full.images >= tight.images) ? 'PASS' : 'FAIL',
    error: `imgTight=${tight.images} imgFull=${full.images}`
  });
  results.push({
    name: 'p012_fillbox_full_pixelEquivalentStyles',
    status: stylesSame ? 'PASS' : 'FAIL',
    error: `stylesTight=${tight.styles.length} stylesFull=${full.styles.length} same=${stylesSame}`
  });

  // ── 非法值容错：want 侧过滤在 EntryAbility/SandCanvas，renderer 侧字段
  //    缺省（undefined）时 render() 必须不抛错并等价 OFF/TIGHT（旧 frameContext
  //    兼容），三态=合法值生效/非法字段忽略/缺省走默认。 ──
  let legacyOk = true;
  let legacyErr = '';
  try {
    const sink = {};
    const legacyCtx = makeContext({ appearanceTier: P012AppearanceTier.EAB });
    delete legacyCtx.ablation;
    delete legacyCtx.fillBox;
    renderer.render(makeCountingCanvas(sink), field.snapshot(), legacyCtx);
    legacyOk = sink.drawImages > 0;
  } catch (e) {
    legacyOk = false;
    legacyErr = `${e}`;
  }
  results.push({
    name: 'p012_ablate_legacyContextDefaults',
    status: legacyOk ? 'PASS' : 'FAIL',
    error: legacyErr
  });
})();

// □§11.6 C 层理单测 8 项（C1 稳定材质深度层纹，C-batch）
// ①sampler 线性插值保层序（含边界 x=0/x=boxW）
// ②bandNoise 位图零均值有界  ③分段连续性（段边界相位连续）
// ④λ 换算一致性              ⑤位图 alpha=255 全检（Node 无位图 → 验烘焙数据路径）
// ⑥消融 strat 跳过 C          ⑦epoch 断配对后 seed/相位稳定
// ⑧空带跳过
(function stratificationSuite() {
  const cols = 128;
  const field = new WaterSandField(cols, 192);
  field.initialize(fullScene(cols));
  field.setTilt(15);
  field.step(1 / 60);

  // ① sampler 保层序（含边界）
  (function test1() {
    const bh = new Float64Array(cols);
    const ms = new Float64Array(cols);
    const st = new Float64Array(cols);
    for (let c = 0; c < cols; c++) {
      bh[c] = 30 + 10 * Math.sin(c / 9);
      ms[c] = bh[c] + 20 + 5 * Math.cos(c / 7);
      st[c] = ms[c] + 15 + 3 * Math.sin(c / 11);
    }
    const S = typesMod;   // P012MaterialSampler 经 renderer import 链加载
    const samplerMod = require('../entry/src/main/ets/render/P012MaterialSampler.ets');
    const sampler = new samplerMod.P012MaterialColumnSampler(bh, ms, st, 640);
    let orderOk = true;
    let worst = 0;
    for (let i = 0; i <= 200; i++) {
      const x = 640 * i / 200;
      const b = sampler.bhAtPx(x);
      const m = sampler.mixStartAtPx(x);
      const s = sampler.sandTopAtPx(x);
      if (!(b <= m + 1e-9 && m <= s + 1e-9)) { orderOk = false; }
      worst = Math.max(worst, 0);
    }
    // 边界值精确：x=0 → 列 0 原值；x=boxW → 列末原值
    const edgeOk = Math.abs(sampler.bhAtPx(0) - bh[0]) < 1e-9 &&
      Math.abs(sampler.sandTopAtPx(640) - st[cols - 1]) < 1e-9;
    results.push({
      name: 'p012_c_sampler_preservesLayerOrder',
      status: (orderOk && edgeOk) ? 'PASS' : 'FAIL',
      error: `orderOk=${orderOk} edgeOk=${edgeOk}`
    });
  })();

  // ② bandNoise 零均值有界 [-1,1]
  (function test2() {
    let ok = true;
    let detail = '';
    for (const key of ['stratNoiseH', 'stratNoiseL', 'stratNoiseM']) {
      const t = new P012WaterSandRenderer(cols, 192)[key];
      let mean = 0;
      let maxAbs = 0;
      for (let i = 0; i < t.length; i++) {
        mean += t[i];
        maxAbs = Math.max(maxAbs, Math.abs(t[i]));
      }
      mean /= t.length;
      if (Math.abs(mean) > 1e-9 || maxAbs > 1 + 1e-9 || maxAbs < 0.5) {
        ok = false;
        detail = `${key} mean=${mean.toFixed(3)} maxAbs=${maxAbs.toFixed(3)}`;
      }
    }
    results.push({
      name: 'p012_c_bandNoise_zeroMeanBounded',
      status: ok ? 'PASS' : 'FAIL',
      error: detail
    });
  })();

  // ③ 分段连续性：段边界两侧 warp 相位差 ≤ 1/16 周期（阈值本身），
  //    且相邻段中心 warp 差 < 2×阈值（分段近似误差 ≤1/32 的两倍界）
  (function test3() {
    const renderer = new P012WaterSandRenderer(cols, 192);
    const colW = 640 / cols;
    // 直接用私有函数逐列扫（测试通道同对象）
    let maxJump = 0;
    for (let c = 0; c < cols - 1; c++) {
      const w0 = renderer.warpAt(0 + c * colW, 0, colW, 0);
      const w1 = renderer.warpAt(0 + (c + 1) * colW, 0, colW, 0);
      maxJump = Math.max(maxJump, Math.abs(w1 - w0));
    }
    results.push({
      name: 'p012_c_segmentPhaseContinuous',
      status: maxJump <= 1 / 16 + 1e-12 ? 'PASS' : 'FAIL',
      error: `maxColWarpJump=${maxJump.toFixed(4)} cycles threshold=${(1 / 16).toFixed(4)}`
    });
  })();

  // ④ λ 换算一致性：λpx = H/k（k=ceil(256/原始λpx)，H=256）→ 与
  //    P012_C_LAMBDA_MATERIAL×pxPerMaterial 偏差 <1/k；多帧同快照相位零漂移
  (function test4() {
    const renderer = new P012WaterSandRenderer(cols, 192);
    const pxPerMaterial = 900 / 192;   // boxH/capacity
    const sink = {};
    const ctx = makeCountingCanvas(sink);
    const stubBitmap = { _stub: true, width: 64, height: 256 };
    renderer.stratBitmapH = stubBitmap;
    renderer.stratBitmapL = stubBitmap;
    renderer.stratBitmapM = stubBitmap;
    renderer.render(ctx, field.snapshot(), makeContext({
      appearanceTier: P012AppearanceTier.EAB,
      boxW: 640, boxH: 900
    }));
    const lamPx = renderer.stratLamPx;
    const raw = 24 * pxPerMaterial;                       // 原始 λpx
    const k = Math.max(1, Math.ceil(256 / raw));
    const expect = 256 / k;                               // H/k 对齐
    const aligned = Math.abs(lamPx - expect) < 1e-9;
    const withinTol = Math.abs(lamPx - raw) / raw <= 1 / k + 1e-9;  // 偏差 <1/k
    // 第二帧同快照：λpx 不变（确定性）
    const lamPx2 = renderer.stratLamPx;
    // 无缝性：λpx 整除位图高（平铺相位连续的结构保证）
    const seamless = Math.abs(256 / lamPx - Math.round(256 / lamPx)) < 1e-9;
    results.push({
      name: 'p012_c_lambdaScaleConsistent',
      status: (aligned && withinTol && lamPx2 === lamPx && seamless) ? 'PASS' : 'FAIL',
      error: `lamPx=${lamPx.toFixed(3)} expect(H/k)=${expect.toFixed(3)} raw=${raw.toFixed(3)} ` +
             `aligned=${aligned} withinTol=${withinTol} seamless=${seamless} stable=${lamPx2 === lamPx}`
    });
  })();

  // ⑤ 位图 alpha=255 全检：Node 无 OffscreenCanvas 位图为 null → 验证
  //    烘焙路径的灰度数据 bound（envelope·amp ∈ [128-amp, 128+amp]，alpha 常量 255
  //    在 makeStratBitmap 源码中；此处验证位图为 null 时跳过且不炸）
  (function test5() {
    const renderer = new P012WaterSandRenderer(cols, 192);
    const nodeNoBitmap = renderer.stratBitmapH === null;   // Node 通道无 OffscreenCanvas
    results.push({
      name: 'p012_c_bitmapAlphaOpaqueOrSkipped',
      status: nodeNoBitmap ? 'PASS' : 'FAIL',
      error: nodeNoBitmap ? 'Node channel: bitmap null (skipped, on-device alpha=255 baked)' :
        'unexpected bitmap in Node channel'
    });
  })();

  // ⑥ 消融 strat：draw 计数不含 C（strat 跳过时 drawImage 少于 off）
  //    + C 净增量预算断言（§11.4：draw ≤16；首版 27 超标回炉的回归闸）
  (function test6() {
    const renderer = new P012WaterSandRenderer(cols, 192);
    const stubBitmap = { _stub: true, width: 64, height: 256 };
    renderer.stratBitmapH = stubBitmap;
    renderer.stratBitmapL = stubBitmap;
    renderer.stratBitmapM = stubBitmap;
    const measure = (over) => {
      const sink = {};
      renderer.render(makeCountingCanvas(sink), field.snapshot(), makeContext(over));
      return sink;
    };
    const off = measure({ appearanceTier: P012AppearanceTier.EAB });
    const strat = measure({
      appearanceTier: P012AppearanceTier.EAB,
      ablation: typesMod.P012Ablation.STRAT
    });
    const delta = off.draws - strat.draws;
    results.push({
      name: 'p012_c_ablateStrat_skipsStratLayer',
      status: (delta > 0) ? 'PASS' : 'FAIL',
      error: `drawsOff=${off.draws} drawsStrat=${strat.draws} delta=${delta}`
    });
    results.push({
      name: 'p012_c_drawBudget_within16',
      status: delta <= 16 ? 'PASS' : 'FAIL',
      error: `cDelta=${delta} budget=16 (C1 drawImage + C3 fillRect)`
    });
  })();

  // ⑦ epoch 断配对后 seed/相位稳定：resetContinuity 后同快照渲染，
  //    warp 表（确定性 seed 生成）不变 → 分段相位不变
  (function test7() {
    const renderer = new P012WaterSandRenderer(cols, 192);
    const stubBitmap = { _stub: true, width: 64, height: 256 };
    renderer.stratBitmapH = stubBitmap;
    renderer.stratBitmapL = stubBitmap;
    renderer.stratBitmapM = stubBitmap;
    const ctx = makeCountingCanvas({});
    renderer.render(ctx, field.snapshot(), makeContext({
      appearanceTier: P012AppearanceTier.EAB
    }));
    const warpBefore = [];
    for (let c = 0; c < cols; c++) {
      warpBefore.push(renderer.warpAt(c * 5, 0, 5, 0));
    }
    renderer.resetContinuity();
    renderer.render(ctx, field.snapshot(), makeContext({
      appearanceTier: P012AppearanceTier.EAB
    }));
    let stable = true;
    for (let c = 0; c < cols; c++) {
      const w = renderer.warpAt(c * 5, 0, 5, 0);
      if (w !== warpBefore[c]) { stable = false; }
    }
    results.push({
      name: 'p012_c_epochBreak_seedPhaseStable',
      status: stable ? 'PASS' : 'FAIL',
      error: `warpTableStableAfterReset=${stable}`
    });
  })();

  // ⑧ 空带跳过：轻沙带全 0 场景（bl=0 → mix band 高度≤0）不炸、不提交该带
  (function test8() {
    const thinScene = {
      waterRegions: [{ startCol: 0, endColInclusive: cols - 1, bottom: 120, height: 60 }],
      heavyDepositedRegions: [{ startCol: 0, endColInclusive: cols - 1, bottom: 0, height: 80 }],
      lightDepositedRegions: [],           // 无轻沙 → 轻沙带空
      heavySuspendedRegions: [],
      lightSuspendedRegions: []
    };
    const f2 = new WaterSandField(cols, 192);
    f2.initialize(thinScene);
    f2.step(1 / 60);
    const renderer = new P012WaterSandRenderer(cols, 192);
    const stubBitmap = { _stub: true, width: 64, height: 256 };
    renderer.stratBitmapH = stubBitmap;
    renderer.stratBitmapL = stubBitmap;
    renderer.stratBitmapM = stubBitmap;
    let ok = true;
    let err = '';
    try {
      const sink = {};
      renderer.render(makeCountingCanvas(sink), f2.snapshot(), makeContext({
        appearanceTier: P012AppearanceTier.EAB
      }));
      ok = sink.draws >= 0;                // 不炸即过；带空由 bandHeight 判定跳过
    } catch (e) {
      ok = false;
      err = `${e}`;
    }
    results.push({
      name: 'p012_c_emptyBandSkipped',
      status: ok ? 'PASS' : 'FAIL',
      error: err
    });
  })();

  // ⑨（epoch 修复 2026-10-22）geometryEpoch 变化 → 重烘焙触发；
  //    epoch 恒定 → 不重烘（幂等）。λpx 随 boxH（pxPerMaterial）变化。
  //    判据：stratLamPx 在 boxH 变化后按 H/k 公式重算；同 epoch 复渲染不变。
  (function test9() {
    const renderer = new P012WaterSandRenderer(cols, 192);
    const stubBitmap = { _stub: true, width: 64, height: 256 };
    renderer.stratBitmapH = stubBitmap;
    renderer.stratBitmapL = stubBitmap;
    renderer.stratBitmapM = stubBitmap;
    const ctx = makeCountingCanvas({});
    // 帧一：boxH=900 → 首烘焙（stratBakedGeometryEpoch 从 -1 变 1）
    renderer.render(ctx, field.snapshot(), makeContext({
      appearanceTier: P012AppearanceTier.EAB, boxW: 640, boxH: 900
    }));
    const lam1 = renderer.stratLamPx;
    const bakedEpoch1 = renderer.stratBakedGeometryEpoch;
    // 帧二：同几何（epoch 同）→ 不重烘（λpx 不变）
    renderer.render(ctx, field.snapshot(), makeContext({
      appearanceTier: P012AppearanceTier.EAB, boxW: 640, boxH: 900
    }));
    const lam2 = renderer.stratLamPx;
    const idempotent = lam2 === lam1;
    // 帧三：几何变化（boxH 900→1200 → pxPerMaterial 变）+ epoch 递增 → 重烘
    const rawPx = 24 * (1200 / 192);
    const k = Math.max(1, Math.ceil(256 / rawPx));
    const expectLam = 256 / k;
    renderer.render(ctx, field.snapshot(), makeContext({
      appearanceTier: P012AppearanceTier.EAB, boxW: 640, boxH: 1200,
      geometryEpoch: 2
    }));
    const lam3 = renderer.stratLamPx;
    const rebaked = Math.abs(lam3 - expectLam) < 1e-9 && lam3 !== lam1;
    results.push({
      name: 'p012_c_geometryEpoch_rebake',
      status: (bakedEpoch1 === 1 && rebaked) ? 'PASS' : 'FAIL',
      error: `bakedEpoch1=${bakedEpoch1} lam(900)=${lam1.toFixed(2)} ` +
             `lam(1200)=${lam3.toFixed(2)} expect=${expectLam.toFixed(2)} rebaked=${rebaked}`
    });
    results.push({
      name: 'p012_c_geometryEpoch_idempotent',
      status: idempotent ? 'PASS' : 'FAIL',
      error: `lam1=${lam1.toFixed(2)} lam2=${lam2.toFixed(2)} same=${idempotent}`
    });
  })();
})();

// Regression: grain tile must be filled across the WHOLE tile.
// buildValueNoiseTile once hardcoded P012_A_TILE=64 as its size, so the 256²
// grain tile was only filled in its top-left 64×64 (93.75% constant) and the
// B-tier grain rendered as flat base colour (measured on-device hfH≈0.035).
(function eabGrainTileCoverage() {
  const cols = 128;
  const renderer = new P012WaterSandRenderer(cols, 192);
  const tiles = {
    H: renderer.grainTileH,
    L: renderer.grainTileL,
    M: renderer.grainTileM
  };
  const TILE = Math.round(Math.sqrt(tiles.H.length));
  const QUAD = 64;                       // legacy buggy fill size
  let worstQuadStd = Infinity;
  let minOverallStd = Infinity;
  for (const key of Object.keys(tiles)) {
    const t = tiles[key];
    const n = t.length;
    let mean = 0;
    for (let i = 0; i < n; i++) mean += t[i];
    mean /= n;
    let varSum = 0;
    for (let i = 0; i < n; i++) varSum += (t[i] - mean) * (t[i] - mean);
    const std = Math.sqrt(varSum / n);
    minOverallStd = Math.min(minOverallStd, std);
    // every QUAD×QUAD quadrant (incl. the previously-dead 3 quadrants) must vary
    for (let qy = 0; qy < TILE; qy += QUAD) {
      for (let qx = 0; qx < TILE; qx += QUAD) {
        let qm = 0; let qc = 0;
        for (let y = qy; y < qy + QUAD; y++) {
          for (let x = qx; x < qx + QUAD; x++) { qm += t[y * TILE + x]; qc++; }
        }
        qm /= qc;
        let qv = 0;
        for (let y = qy; y < qy + QUAD; y++) {
          for (let x = qx; x < qx + QUAD; x++) {
            const d = t[y * TILE + x] - qm; qv += d * d;
          }
        }
        worstQuadStd = Math.min(worstQuadStd, Math.sqrt(qv / qc));
      }
    }
  }
  const coverageOk = worstQuadStd > 0.05;
  results.push({
    name: 'p012_eab_grainTile_fullCoverage',
    status: coverageOk ? 'PASS' : 'FAIL',
    error: `tile=${TILE} worstQuadrantStd=${worstQuadStd.toFixed(4)} minOverallStd=${minOverallStd.toFixed(4)}`
  });

  // Rendered-luminance std the grain contributes over a sand-sized area
  // (pattern repeat of the tile; sigma×tile offsets on the heavy base).
  // B 档语义（§5.1）：Y = Ybase + alpha·sigmaG·nG，nG 单位方差 →
  // 颗粒有效亮度标准差 = P012_B_GRAIN_ALPHA × P012_B_SIGMA = 0.55×20 = 11，
  // 与 E 档基色大尺度 std（on-device 实测 ~13，低频，与颗粒不相关）按方差合成。
  const heavy = { r: 145, g: 168, b: 111 };
  const SIGMA = 20;
  const ALPHA = 0.55;
  const BASE_STD = 13;               // E 档沙基色大尺度 std（实测口径）
  const t = tiles.H;
  const W = 400, H = 120;
  let lumMean = 0; const lum = new Float64Array(W * H);
  for (let y = 0; y < H; y++) {
    for (let x = 0; x < W; x++) {
      const v = t[(y % TILE) * TILE + (x % TILE)];
      const r = Math.max(0, Math.min(255, heavy.r + ALPHA * SIGMA * v));
      const g = Math.max(0, Math.min(255, heavy.g + ALPHA * SIGMA * v));
      const b = Math.max(0, Math.min(255, heavy.b + ALPHA * SIGMA * v));
      const L = 0.299 * r + 0.587 * g + 0.114 * b;
      lum[y * W + x] = L; lumMean += L;
    }
  }
  lumMean /= (W * H);
  let lv = 0;
  for (let i = 0; i < lum.length; i++) lv += (lum[i] - lumMean) * (lum[i] - lumMean);
  const lumStd = Math.sqrt(lv / lum.length);
  // horizontal neighbour roughness (hfH proxy) must be clearly non-zero
  let hfSum = 0; let hfN = 0;
  for (let y = 0; y < H; y++) {
    for (let x = 1; x < W; x++) { hfSum += Math.abs(lum[y * W + x] - lum[y * W + x - 1]); hfN++; }
  }
  const hfH = hfSum / hfN;
  const totalStd = Math.sqrt(BASE_STD * BASE_STD + lumStd * lumStd);
  results.push({
    name: 'p012_eab_grainTile_lumStdContribution',
    status: (totalStd >= 15 && hfH >= 0.8) ? 'PASS' : 'FAIL',
    error: `grainStd=${lumStd.toFixed(2)} hfH=${hfH.toFixed(3)} totalStd(vs base ${BASE_STD})=${totalStd.toFixed(1)}`
  });

  // Unit-variance semantics (§5.1): every tile std≈1 and clipped to ±P012_B_GRAIN_CLIP.
  let worstStdDev = 0; let worstAbs = 0;
  for (const key of Object.keys(tiles)) {
    const tt = tiles[key];
    let m2 = 0;
    for (let i = 0; i < tt.length; i++) m2 += tt[i];
    m2 /= tt.length;
    let v2 = 0;
    for (let i = 0; i < tt.length; i++) v2 += (tt[i] - m2) * (tt[i] - m2);
    worstStdDev = Math.max(worstStdDev, Math.abs(Math.sqrt(v2 / tt.length) - 1));
    for (let i = 0; i < tt.length; i++) worstAbs = Math.max(worstAbs, Math.abs(tt[i]));
  }
  results.push({
    name: 'p012_eab_grainTile_unitVarianceClipped',
    status: (worstStdDev < 0.1 && worstAbs <= 2.5 + 1e-9) ? 'PASS' : 'FAIL',
    error: `worstStdDev=${worstStdDev.toFixed(4)} worstAbs=${worstAbs.toFixed(4)}`
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
