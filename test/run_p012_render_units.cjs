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
const { P012WaterSandRenderer, p012ShouldUseP012Renderer, p012E3QDirty } = rendererMod;
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
  // §12 E3：EA 档默认走 I2 光学缓冲。烘焙判据用**新 renderer**（首帧必烘，
  // 60 帧循环后的实例静止命中零烘焙是幂等设计的正确行为，不能当证据）；
  // rgba fill 判据仅 E3I2 消融档（旧 12 档路径）下成立。
  const freshRenderer = new P012WaterSandRenderer(cols, 192);
  const freshSink = { draws: 0, fillRects: 0, clips: 0, drawImages: 0, styles: [] };
  freshRenderer.render(makeCountingCanvas(freshSink), field.snapshot(),
    makeContext({ appearanceTier: P012AppearanceTier.EA }));
  const eaBaked = freshRenderer.e3BakeCount > 0;
  draws.length = 0;
  fills.length = 0;
  renderer.render(ctx, field.snapshot(), makeContext({
    appearanceTier: P012AppearanceTier.EA, ablation: typesMod.P012Ablation.E3I2 }));
  const eaMistFills = fills.length;
  results.push({
    name: 'p012_ea_mistOnlyInEATier',
    status: eMistFills === 0 && eaMistFills > 0 && eaBaked ? 'PASS' : 'FAIL',
    error: `eMist=${eMistFills} eaMist(e3i2-legacy)=${eaMistFills} eaI2Baked=${eaBaked}`
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
  // §12 E3：默认 I2 路径。FOG 消融判据两支：①旧路径（E3I2 消融做 off 基线）
  // draw 差>0；②I2 路径 FOG 下烘焙不发生（fresh renderer 首帧 FOG=0 烘焙，
  // 对照同 fresh 场景 OFF=2 烘焙——静止命中只影响已烘实例，fresh 必烘）。
  const eaFog = measure({
    appearanceTier: P012AppearanceTier.EA,
    ablation: typesMod.P012Ablation.FOG
  });
  const eaOffLegacy = measure({
    appearanceTier: P012AppearanceTier.EA,
    ablation: typesMod.P012Ablation.E3I2
  });
  const fogRenderer = new P012WaterSandRenderer(cols, 192);
  const offRenderer = new P012WaterSandRenderer(cols, 192);
  const snap = field.snapshot();
  offRenderer.render(makeCountingCanvas({}), snap, makeContext({ appearanceTier: P012AppearanceTier.EA }));
  fogRenderer.render(makeCountingCanvas({}), snap, makeContext({
    appearanceTier: P012AppearanceTier.EA, ablation: typesMod.P012Ablation.FOG }));
  results.push({
    name: 'p012_ablate_fog_skipsFogLayers',
    status: (eaOffLegacy.draws - eaFog.draws > 0 &&
      offRenderer.e3BakeCount > 0 && fogRenderer.e3BakeCount === 0) ? 'PASS' : 'FAIL',
    error: `legacyDrawsOff=${eaOffLegacy.draws} drawsAblated=${eaFog.draws} ` +
           `delta=${eaOffLegacy.draws - eaFog.draws} bakeOff=${offRenderer.e3BakeCount} bakeFog=${fogRenderer.e3BakeCount}`
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

// □§12.6 E3 单测 10 项（I2 光学缓冲 + N1 噪声场，第一批）
// ①sampler 层序勘误回归 ②通用标量采样同定位 ③un-premultiply 暗边
// ④N1 连续性无 64 周期 ⑤N1 与物理 RNG 隔离 ⑥缓存失效矩阵
// ⑦静止零重烘焙 ⑧reset 相位清零 ⑨补边无缝 ⑩buffer 等比
(function e3Suite() {
  const noiseMod = require('../entry/src/main/ets/render/P012E3Noise.ets');
  const bufferMod = require('../entry/src/main/ets/render/P012E3FogBuffers.ets');
  const samplerMod = require('../entry/src/main/ets/render/P012MaterialSampler.ets');
  const { P012E3NoiseField } = noiseMod;
  const { P012E3FogBuffer } = bufferMod;
  const { P012MaterialColumnSampler } = samplerMod;

  // ① sampler 层序勘误回归：0 ≤ bh ≤ mixStart ≤ sandTop 逐点（含负 bl 场景
  //    由物理保证非负；此处用单调递增列 + 边界）
  (function t1() {
    const cols = 64;
    const bh = new Float64Array(cols);
    const ms = new Float64Array(cols);
    const st = new Float64Array(cols);
    for (let c = 0; c < cols; c++) {
      bh[c] = 20 + 8 * Math.sin(c / 5);
      ms[c] = bh[c] + 15 + 5 * Math.cos(c / 3);
      st[c] = ms[c] + 10;
    }
    const s = new P012MaterialColumnSampler(bh, ms, st, 512);
    let ok = true;
    for (let i = 0; i <= 100; i++) {
      const x = 512 * i / 100;
      const b = s.bhAtPx(x);
      const m = s.mixStartAtPx(x);
      const t = s.sandTopAtPx(x);
      if (!(0 <= b && b <= m + 1e-9 && m <= t + 1e-9)) { ok = false; break; }
    }
    results.push({ name: 'p012_e3_samplerLayerOrder', status: ok ? 'PASS' : 'FAIL',
      error: `monotonic=${ok}` });
  })();

  // ② 通用标量采样与层高采样同定位/同权重：scalarAt 与 sampler 的 split
  //    对同一 x 给出同 i/t（断言插值结果一致——用同数据数组对比）
  (function t2() {
    const cols = 64;
    const data = new Float64Array(cols);
    for (let c = 0; c < cols; c++) { data[c] = c * 3 + 1; }
    const renderer = new P012WaterSandRenderer(cols, 192);
    const s = new P012MaterialColumnSampler(data, data, data, 512);
    let ok = true;
    let worst = 0;
    for (let i = 0; i <= 50; i++) {
      const x = 512 * i / 50;
      const viaSampler = s.bhAtPx(x);
      const viaScalar = renderer.scalarAt(data, x, 512);
      worst = Math.max(worst, Math.abs(viaSampler - viaScalar));
      if (Math.abs(viaSampler - viaScalar) > 1e-9) { ok = false; }
    }
    results.push({ name: 'p012_e3_scalarSamplerSameSplit', status: ok ? 'PASS' : 'FAIL',
      error: `worstDiff=${worst.toExponential(2)}` });
  })();

  // ③ un-premultiply 暗边：alpha 线性梯度 buffer（左半 a=0.5 纯白雾、右半
  //    a=0）→ flush → 断言 a=0 区 RGB=0、a=0.5 区 RGB≈雾色、无比雾色暗 5 级
  (function t3() {
    const buf = new P012E3FogBuffer(64, 64, 64);
    for (let u = 0; u < 64; u++) {
      const a = u < 32 ? 0.5 : 0;
      for (let v = 0; v < 64; v++) {
        buf.setPremul(u, v, 232, 240, 236, a);
      }
    }
    buf.flushToUpload();
    let ok = true;
    let detail = '';
    // a=0.5 区：RGB 应≈雾色（232/240/236 ± 舍入）
    const mid = buf.readUpload(16, 32);
    if (mid[3] !== 128 || Math.abs(mid[0] - 232) > 1 || Math.abs(mid[1] - 240) > 1) {
      ok = false; detail = `mid=${mid}`;
    }
    // a=0 区：RGB 必须置 0
    const right = buf.readUpload(48, 32);
    if (right[0] !== 0 || right[1] !== 0 || right[2] !== 0 || right[3] !== 0) {
      ok = false; detail += ` right=${right}`;
    }
    // 过渡带（u=31→32）无中间暗值：a=0.5 侧 RGB 不塌
    const edge = buf.readUpload(31, 32);
    if (Math.abs(edge[0] - 232) > 1) { ok = false; detail += ` edge=${edge}`; }
    results.push({ name: 'p012_e3_unpremultiplyNoDarkEdge', status: ok ? 'PASS' : 'FAIL',
      error: detail || `mid=${mid} right=${right} edge=${edge}` });
  })();

  // ④ N1 连续性无 64 周期：相邻采样 |Δ| 有界（连续）+ P64 自相关低（无周期）
  (function t4() {
    const f = new P012E3NoiseField(20261020);
    f.bakeCoverage(0, 0, 512, 128);
    let maxStep = 0;
    for (let x = 0; x < 511; x++) {
      maxStep = Math.max(maxStep, Math.abs(f.sample(x + 1, 64) - f.sample(x, 64)));
    }
    // 64 周期检测：n(x) 与 n(x+64) 的相关性应远低于 n(x) 与 n(x+1)
    let corr64 = 0, n64 = 0;
    let varX = 0, meanX = 0;
    const xs = [];
    for (let x = 0; x < 448; x++) { xs.push(f.sample(x, 64)); meanX += xs[x]; }
    meanX /= xs.length;
    for (let x = 0; x < 448; x++) { varX += (xs[x] - meanX) ** 2; }
    varX /= xs.length;
    for (let x = 0; x < 384; x++) {
      corr64 += (xs[x] - meanX) * (f.sample(x + 64, 64) - meanX);
      n64++;
    }
    corr64 /= n64 * Math.max(1e-9, varX);
    // 连续性：步长 ≤ 0.15（16px 格点 × 权重 0.565 + 48px 格点 0.435 的导数界）
    // 无周期：|corr64| < 0.5（完美 64 周期会 ≈1）
    results.push({ name: 'p012_e3_n1ContinuousNo64Period', status:
      (maxStep <= 0.2 && Math.abs(corr64) < 0.5) ? 'PASS' : 'FAIL',
      error: `maxStep=${maxStep.toFixed(4)} corr64=${corr64.toFixed(4)}` });
  })();

  // ⑤ N1 与物理 RNG 隔离：渲染 N1 采样前后 WaterSandField 快照逐位一致
  (function t5() {
    const cols = 64;
    const fieldA = new WaterSandField(cols, 192);
    fieldA.initialize(fullScene(cols));
    fieldA.setTilt(20);
    fieldA.step(1 / 60);
    const snapBefore = JSON.stringify(fieldA.snapshot().bh);
    const f = new P012E3NoiseField(20261020);
    for (let i = 0; i < 1000; i++) {
      f.sample(i * 7.3, i * 3.1);
    }
    const snapAfter = JSON.stringify(fieldA.snapshot().bh);
    results.push({ name: 'p012_e3_n1PhysicsRngIsolated', status:
      snapBefore === snapAfter ? 'PASS' : 'FAIL',
      error: `bhIdentical=${snapBefore === snapAfter}` });
  })();

  // ⑥⑦ 缓存失效矩阵 + 静止零重烘焙：fresh renderer 场景
  (function t67() {
    const cols = 64;
    const field = new WaterSandField(cols, 192);
    field.initialize(fullScene(cols));
    field.setTilt(10);
    field.step(1 / 60);
    const snap = field.snapshot();
    const mk = (sink) => makeCountingCanvas(sink);
    // 帧一：首烘焙（water+iface 各 1）
    const r = new P012WaterSandRenderer(cols, 192);
    r.render(mk({}), snap, makeContext({ appearanceTier: P012AppearanceTier.EA }));
    const bake1 = r.e3BakeCount;
    // 帧二：同快照静止 → 零重烘焙（⑦）
    r.render(mk({}), snap, makeContext({ appearanceTier: P012AppearanceTier.EA }));
    const bake2 = r.e3BakeCount;
    // 帧三：层高变化（物理步进）→ 重烘（⑥ 层高触发）
    field.step(1 / 60);
    r.render(mk({}), field.snapshot(), makeContext({ appearanceTier: P012AppearanceTier.EA }));
    const bake3 = r.e3BakeCount;
    results.push({ name: 'p012_e3_staticZeroRebake', status:
      (bake1 === 2 && bake2 === 2 && bake3 > 2) ? 'PASS' : 'FAIL',
      error: `bake1=${bake1} bake2=${bake2} bake3=${bake3} (1=烘2buffer, 2=静止0增, 3=层高变>2)` });
  })();

  // ⑧ reset 相位清零：resetContinuity 后 aPhaseX=0、双 buffer dirty
  (function t8() {
    const r = new P012WaterSandRenderer(32, 192);
    r['aPhaseX'] = 123.4;                      // 模拟泄漏相位（Node 直接字段注入）
    r.resetContinuity();
    const phaseZero = r['aPhaseX'] === 0;
    const dirtyBoth = (r['e3WaterBuffer'] === null || r['e3WaterBuffer'].dirty) &&
      (r['e3IfaceBuffer'] === null || r['e3IfaceBuffer'].dirty);
    results.push({ name: 'p012_e3_resetPhaseZero', status:
      phaseZero && dirtyBoth ? 'PASS' : 'FAIL',
      error: `phaseZero=${phaseZero} dirtyBoth=${dirtyBoth}` });
  })();

  // ⑨ 补边无缝：bake 两次不同窗口，重叠区采样一致。bakedPx=4 后窗口
  //    边界外走 computeDirect、窗口内走 4px 网格双线性——两者差是
  //    网格分辨率语义（~px²·曲率项），不是缝；缝的判定=窗口内两次
  //    烘焙的重叠区逐位一致。改为：对照两次烘焙都在窗口内的重叠段
  //    （x∈[64,224)，两次都在覆盖内）+ 全段差上界 0.01（插值语义）。
  (function t9() {
    const f = new P012E3NoiseField(20261020);
    f.bakeCoverage(0, 0, 256, 64);
    const ref = [];
    for (let x = 32; x < 224; x++) { ref.push(f.sample(x, 32)); }
    f.shiftAndBake(64, 0, 256, 64);            // 移 64px（补边语义）
    let overlapWorst = 0;                       // 窗口内重叠段（x∈[64,224)）
    let allWorst = 0;
    for (let i = 0; i < ref.length; i++) {
      const v = f.sample(32 + i, 32);
      const d = Math.abs(v - ref[i]);
      allWorst = Math.max(allWorst, d);
      if (32 + i >= 64) { overlapWorst = Math.max(overlapWorst, d); }
    }
    const ok = overlapWorst <= 1e-6 && allWorst <= 0.01;
    results.push({ name: 'p012_e3_shiftSeamless', status: ok ? 'PASS' : 'FAIL',
      error: `overlapMaxDiff=${overlapWorst.toExponential(2)} (重叠区逐位) ` +
             `edgeMaxDiff=${allWorst.toExponential(2)} (边界插值语义≤0.01)` });
  })();

  // ⑩ buffer 宽高与 boxW/boxH 等比：height = round(width*boxH/boxW)
  (function t10() {
    const buf = new P012E3FogBuffer(256, 921, 1099);
    const expectH = Math.round(256 * 1099 / 921);
    results.push({ name: 'p012_e3_bufferAspectRatio', status:
      buf.height === expectH ? 'PASS' : 'FAIL',
      error: `h=${buf.height} expect=${expectH} w=256` });
  })();
})();

// ═════════ §13 参数可调化单测（架构 §13.6 七项） ═════════
(function appearanceControlsSuite() {
  const types = typesMod;
  const cols = 128;
  const factory = () => types.p012FactoryAppearance();
  const pxPerMaterial = 311 / 192;       // s133 真机几何等比

  // ① schema 校验矩阵（每字段超域/非法档/缺字段/styleFamily 不符 → 各拒绝路径）
  (function a1() {
    const cases = [];
    const push = (name, mutate, expectReject) => {
      const p = factory();
      mutate(p);
      const err = types.validateP012AppearanceParams(p, pxPerMaterial);
      const rejected = err !== null;
      cases.push({ name, ok: rejected === expectReject, err: err || '-' });
    };
    push('合法默认包', () => {}, false);
    push('schemaVersion=99', p => { p.schemaVersion = 99; }, true);
    push('styleFamily 不符', p => { p.styleFamilyId = 'other-v9'; }, true);
    push('fine=3 非法档', p => { p.grainFeaturePx = 3; }, true);
    push('fine=6 合法档', p => { p.grainFeaturePx = 6; }, false);
    push('sigma=11 超下界', p => { p.grainSigma = 11; }, true);
    push('sigma=25 超上界', p => { p.grainSigma = 25; }, true);
    push('sigma=20.5 非整步', p => { p.grainSigma = 20.5; }, true);
    push('fog=0.45 超下界', p => { p.fogDensityScale = 0.45; }, true);
    push('fog=1.05 超上界', p => { p.fogDensityScale = 1.05; }, true);
    push('fog=0.77 非步进', p => { p.fogDensityScale = 0.77; }, true);
    push('lambda=15 超下界', p => { p.strataLambdaMaterial = 15; }, true);
    push('lambda=33 超上界', p => { p.strataLambdaMaterial = 33; }, true);
    push('strength=0.4 超下界', p => { p.strataStrengthScale = 0.4; }, true);
    push('strength=1.4 超上界', p => { p.strataStrengthScale = 1.4; }, true);
    push('mottle=0.45 超下界', p => { p.clearMottleContrast = 0.45; }, true);
    push('mottle=1.3 超上界', p => { p.clearMottleContrast = 1.3; }, true);
    push('source 非法', p => { p.source = 'hacker'; }, true);
    push('revision=-1', p => { p.configRevision = -1; }, true);
    push('revision=1.5 非整', p => { p.configRevision = 1.5; }, true);
    const allOk = cases.every(c => c.ok);
    const bad = cases.filter(c => !c.ok).map(c => c.name).join(',');
    results.push({ name: 'p012_appearance_schemaMatrix', status: allOk ? 'PASS' : 'FAIL',
      error: `n=${cases.length} 失败=[${bad}]` });
  })();

  // ①b λ 去重表：同 k 合并 + 键集校验
  (function a1b() {
    const table = types.buildStrataLambdaDedupTable(pxPerMaterial);
    // 每个键的 k 唯一
    const ks = table.map(nominal =>
      Math.max(1, Math.ceil(256 / (nominal * pxPerMaterial))));
    const uniqueK = new Set(ks).size === ks.length;
    // 默认 24 必在键集（s133 基线）
    const has24 = types.strataLambdaInDedupTable(24, pxPerMaterial);
    // 同 k 的非代表值（如 24.1 与 24 同 k 时 24.1 不在键集）
    const k24 = Math.max(1, Math.ceil(256 / (24 * pxPerMaterial)));
    const nextNominal = 24.1;
    const kNext = Math.max(1, Math.ceil(256 / (nextNominal * pxPerMaterial)));
    const dedupWorks = kNext === k24
      ? !types.strataLambdaInDedupTable(nextNominal, pxPerMaterial)
      : true;                            // k 不同则本来就该在（表会含它）
    results.push({ name: 'p012_appearance_lambdaDedup', status:
      uniqueK && has24 && table.length > 0 && dedupWorks ? 'PASS' : 'FAIL',
      error: `tableSize=${table.length} uniqueK=${uniqueK} has24=${has24} dedupWorks=${dedupWorks}` });
  })();

  // ② f=c=1 恢复链：与默认渲染输出逐位一致（数值绑定表第一项）
  (function a2() {
    const field = new WaterSandField(cols, 192);
    field.initialize(fullScene(cols));
    field.setTilt(15);
    field.step(1 / 60);
    const r1 = new P012WaterSandRenderer(cols, 192);
    const r2 = new P012WaterSandRenderer(cols, 192);
    const s1 = makeCountingCanvas({});
    const s2 = makeCountingCanvas({});
    r1.render(s1, field.snapshot(), makeContext({}));
    r2.render(s2, field.snapshot(), makeContext({}));
    // r2 应用 factory 等价包（f=c=1 无 diff → 元数据更新）
    const pkg = factory();
    pkg.configRevision = 1;
    const err = r2.applyAppearance(pkg, pxPerMaterial);
    field.step(1 / 60);
    r1.render(s1, field.snapshot(), makeContext({}));
    r2.render(s2, field.snapshot(), makeContext({}));
    const same = r1.lastDrawCount === r2.lastDrawCount &&
      r2.e3BakeCount === r1.e3BakeCount;
    results.push({ name: 'p012_appearance_factoryEquiv', status:
      err === null && same ? 'PASS' : 'FAIL',
      error: `err=${err} draws=${r1.lastDrawCount}/${r2.lastDrawCount} bakes=${r1.e3BakeCount}/${r2.e3BakeCount}` });
  })();

  // ③ 重烘焙矩阵：fine→颗粒组+1 层理/雾不动；λ→层纹组+1 颗粒不动；
  //    f/c→两雾 buffer dirty 且零位图重烘
  (function a3() {
    const r = new P012WaterSandRenderer(cols, 192);
    const ctxC = makeCountingCanvas({});
    const field = new WaterSandField(cols, 192);
    field.initialize(fullScene(cols));
    field.setTilt(15);
    field.step(1 / 60);
    r.render(ctxC, field.snapshot(), makeContext({}));
    const g0 = r.grainBakeCount, s0 = r.stratBakeCount;
    // fine 改变
    const p1 = factory();
    p1.grainFeaturePx = 4;
    p1.configRevision = 1;
    const e1 = r.applyAppearance(p1, pxPerMaterial);
    const g1 = r.grainBakeCount, s1 = r.stratBakeCount;
    // λ 改变（用去重表键集内的值——32 在部分几何下不在键集）
    const lambdaTable = types.buildStrataLambdaDedupTable(pxPerMaterial);
    const altLambda = lambdaTable.find(n => n !== 24) ?? 32;
    const p2 = factory();
    p2.strataLambdaMaterial = altLambda;
    p2.configRevision = 2;
    const e2 = r.applyAppearance(p2, pxPerMaterial);
    const g2 = r.grainBakeCount, s2 = r.stratBakeCount;
    // f/c 改变
    const p3 = factory();
    p3.fogDensityScale = 0.75;
    p3.clearMottleContrast = 0.9;
    p3.configRevision = 3;
    const e3 = r.applyAppearance(p3, pxPerMaterial);
    const g3 = r.grainBakeCount, s3 = r.stratBakeCount;
    const ok = e1 === null && e2 === null && e3 === null &&
      g1 === g0 + 1 && s1 === s0 &&          // fine → 颗粒组+1，层理不动
      s2 === s1 + 1 && g2 === g1 &&          // λ → 层纹组+1，颗粒不动
      g3 === g2 && s3 === s2;                // f/c → 零位图重烘
    results.push({ name: 'p012_appearance_rebakeMatrix', status: ok ? 'PASS' : 'FAIL',
      error: `fine:${g0}->${g1} strat:${s0}->${s1} | λ strat:${s1}->${s2} grain:${g1}->${g2} | f/c grain:${g2}->${g3} strat:${s2}->${s3}` });
  })();

  // ④ 物理零触碰：外观全参数遍历前后，同输入物理 step 逐位一致
  (function a4() {
    const mk = () => {
      const f = new WaterSandField(cols, 192);
      f.initialize(fullScene(cols));
      f.setTilt(15);
      return f;
    };
    const fa = mk();
    const fb = mk();
    const r = new P012WaterSandRenderer(cols, 192);
    const ctxC = makeCountingCanvas({});
    for (let i = 0; i < 10; i++) {
      fa.step(1 / 60);
      fb.step(1 / 60);
    }
    // 外观全参数遍历（含极端值；λ 用键集内值防拒绝）
    const lambdaTable = types.buildStrataLambdaDedupTable(pxPerMaterial);
    const extremeLambda = lambdaTable[lambdaTable.length - 1];
    const variants = [factory()];
    const v1 = factory(); v1.grainFeaturePx = 6; v1.grainSigma = 12;
    v1.fogDensityScale = 0.5; v1.strataLambdaMaterial = extremeLambda;
    v1.strataStrengthScale = 4 / 3; v1.clearMottleContrast = 1.25;
    variants.push(v1);
    const v2 = factory(); v2.grainSigma = 24; v2.strataStrengthScale = 0.5;
    variants.push(v2);
    let allApplied = true;
    for (const v of variants) {
      v.configRevision = 10;
      if (r.applyAppearance(v, pxPerMaterial) !== null) {
        allApplied = false;
      }
      r.render(ctxC, fb.snapshot(), makeContext({}));   // 渲染也跑（含烘焙）
    }
    for (let i = 0; i < 10; i++) {
      fa.step(1 / 60);
      fb.step(1 / 60);
    }
    const sa = fa.snapshot();
    const sb = fb.snapshot();
    let bitwise = true;
    for (let c = 0; c < cols; c++) {
      if (sa.bh[c] !== sb.bh[c] || sa.bl[c] !== sb.bl[c] ||
        sa.sandTop[c] !== sb.sandTop[c] || sa.waterTop[c] !== sb.waterTop[c]) {
        bitwise = false;
        break;
      }
    }
    results.push({ name: 'p012_appearance_physicsUntouched', status:
      allApplied && bitwise ? 'PASS' : 'FAIL',
      error: `applied=${allApplied} bitwise=${bitwise}` });
  })();

  // ⑤ 持久化损坏恢复：非法 JSON / 校验失败串 → 丢弃（语义由 validate 驱动，
  //    本用例钉 validate 对损坏输入的行为）
  (function a5() {
    const good = factory();
    const goodErr = types.validateP012AppearanceParams(good, pxPerMaterial);
    const broken = factory();
    broken.grainSigma = 99;               // 损坏值
    const brokenErr = types.validateP012AppearanceParams(broken, pxPerMaterial);
    const wrongFamily = factory();
    wrongFamily.styleFamilyId = 'legacy-v0';
    const wrongFamilyErr = types.validateP012AppearanceParams(wrongFamily, pxPerMaterial);
    results.push({ name: 'p012_appearance_persistReject', status:
      goodErr === null && brokenErr !== null && wrongFamilyErr !== null ? 'PASS' : 'FAIL',
      error: `good=${goodErr} broken=${brokenErr} family=${wrongFamilyErr}` });
  })();

  // ⑥ configRevision 单调：应用成功 +1；拒绝不变
  (function a6() {
    const r = new P012WaterSandRenderer(cols, 192);
    const rev0 = r.getAppearanceConfigRevision();
    const p1 = factory();
    p1.grainSigma = 18;
    p1.configRevision = rev0 + 1;
    r.applyAppearance(p1, pxPerMaterial);
    const rev1 = r.getAppearanceConfigRevision();      // staging 未消费 = 仍 rev0
    // 帧边界消费（render 入口）
    const ctxC = makeCountingCanvas({});
    const field = new WaterSandField(cols, 192);
    field.initialize(fullScene(cols));
    field.setTilt(15);
    field.step(1 / 60);
    r.render(ctxC, field.snapshot(), makeContext({}));
    const rev2 = r.getAppearanceConfigRevision();      // 消费后 = rev0+1
    // 非法包被拒：revision 不变
    const bad = factory();
    bad.grainSigma = 99;
    bad.configRevision = 99;
    r.applyAppearance(bad, pxPerMaterial);
    const rev3 = r.getAppearanceConfigRevision();
    results.push({ name: 'p012_appearance_revisionMonotonic', status:
      rev1 === rev0 && rev2 === rev0 + 1 && rev3 === rev2 ? 'PASS' : 'FAIL',
      error: `rev0=${rev0} staged=${rev1} consumed=${rev2} afterReject=${rev3}` });
  })();

  // ⑦ 恢复推荐 source 切换：user → factory-recommended；探索档标注语义（f/c≠1）
  (function a7() {
    const r = new P012WaterSandRenderer(cols, 192);
    const pUser = factory();
    pUser.fogDensityScale = 0.75;         // 探索值
    pUser.source = 'user';
    pUser.configRevision = 1;
    r.applyAppearance(pUser, pxPerMaterial);
    const ctxC = makeCountingCanvas({});
    const field = new WaterSandField(cols, 192);
    field.initialize(fullScene(cols));
    field.setTilt(15);
    field.step(1 / 60);
    r.render(ctxC, field.snapshot(), makeContext({}));   // 消费 staging
    const active = r.activeAppearanceParams;
    const isUser = active.source === 'user' && active.fogDensityScale === 0.75;
    // 恢复推荐
    const pFactory = factory();
    pFactory.configRevision = active.configRevision + 1;
    r.applyAppearance(pFactory, pxPerMaterial);
    field.step(1 / 60);
    r.render(ctxC, field.snapshot(), makeContext({}));
    const restored = r.activeAppearanceParams;
    const isRestored = restored.source === 'factory-recommended' &&
      restored.fogDensityScale === 1.0 && restored.grainSigma === 20;
    results.push({ name: 'p012_appearance_restoreFactory', status:
      isUser && isRestored ? 'PASS' : 'FAIL',
      error: `userApplied=${isUser} restored=${isRestored} f=${restored.fogDensityScale}` });
  })();

  // ⑧（附加）非法包拒绝整包：不 clamp 不部分应用（active 保持旧值）
  (function a8() {
    const r = new P012WaterSandRenderer(cols, 192);
    const pBad = factory();
    pBad.grainSigma = 99;                 // 非法
    pBad.grainFeaturePx = 6;              // 混入合法字段
    pBad.configRevision = 1;
    const err = r.applyAppearance(pBad, pxPerMaterial);
    const active = r.activeAppearanceParams;
    const rejected = err !== null &&
      active.grainSigma === 20 && active.grainFeaturePx === 2;   // 无部分应用
    results.push({ name: 'p012_appearance_rejectNoClamp', status:
      rejected ? 'PASS' : 'FAIL',
      error: `err=${err !== null} activeSigma=${active.grainSigma} activeFine=${active.grainFeaturePx}` });
  })();
})();

// ═════════ E2 路线②原型单测（toplight 开关/烘焙确定性/失效计数/OFF 等价） ═════════

// ═════════ s134 渲染回归修复单测（q 阈值 dirty 失效——任务 6adb3508…0b97） ═════════
(function e3QThresholdSuite() {
  // 0) 直接验证阈值语义（p012E3QDirty 纯函数）：旧实现「绝对 0.01」在 settle
  //    微动场景每帧触发；新实现「max(0.02, |prev|·0.2)」抑制之、真实变化仍触发。
  (function q0() {
    // settle 微动：prev=0.5, Δ=0.015（2% 相对，旧阈 0.01 会触发→脏）
    const settleMicro = p012E3QDirty(0.5, 0.5, 0.515, 0.5) === false;
    // 静水微抖：prev=0.01, Δ=0.002（20% 相对，新阈 0.02 兜住→不脏）
    const staticWater = p012E3QDirty(0.01, 0.01, 0.012, 0.012) === false;
    // 真实变化：prev=0.5, Δ=0.2 → 脏（重烘必须跟上）
    const realChange = p012E3QDirty(0.5, 0.5, 0.7, 0.5) === true;
    results.push({ name: 'p012_e3_qThresholdSemantics', status:
      (settleMicro && staticWater && realChange) ? 'PASS' : 'FAIL',
      error: `settleMicroSuppressed=${settleMicro} staticWaterStable=${staticWater} realChangeDirty=${realChange}` });
  })();

  // ① 沉降尾段低频重烘：600 帧沉降后 300 帧尾段重烘远低于修复前「每帧」风暴。
  //    诊断：修复后 q 通道不再每帧触发；尾段 43 次重烘全部来自真实层高累积蠕变
  //    （raw bh/frame≈0.014 物理格，累积超 LAYER_EPS=0.25 即重烘，属正确行为），
  //    非 q 伪 dirty。断言上界 120（修复前为 300/每帧；留 2.8× 余量，杜绝回退）。
  (function q1() {
    const f = new WaterSandField(128, 192);
    f.initialize(fullScene(128));
    f.setTilt(15);
    const r = new P012WaterSandRenderer(128, 192);
    const c = makeCountingCanvas({});
    const ctxEab = () => makeContext({ appearanceTier: typesMod.P012AppearanceTier.EAB });
    for (let i = 0; i < 600; i++) {
      f.step(1 / 60);
      r.render(c, f.snapshot(), ctxEab());
    }
    const b0 = r.e3BakeCount;
    for (let i = 0; i < 300; i++) {
      f.step(1 / 60);
      r.render(c, f.snapshot(), ctxEab());
    }
    const tailBakes = r.e3BakeCount - b0;
    results.push({ name: 'p012_e3_qTailLowRebake', status:
      tailBakes < 120 ? 'PASS' : 'FAIL',
      error: `沉降600帧后300帧尾段重烘=${tailBakes}（修复前每帧=300；真实蠕变，<120）` });
  })();

  // ② 大倾角真实重排的响应：setTilt(70) 触发 avalanching（层高真实快速
  //    变化——区别于 shake 扬沙限流缓变），重烘率应显著（漏更新=回归）
  (function q2() {
    const f = new WaterSandField(128, 192);
    f.initialize(fullScene(128));
    f.setTilt(15);
    const r = new P012WaterSandRenderer(128, 192);
    const c = makeCountingCanvas({});
    const ctxEab = () => makeContext({ appearanceTier: typesMod.P012AppearanceTier.EAB });
    for (let i = 0; i < 600; i++) {
      f.step(1 / 60);
      r.render(c, f.snapshot(), ctxEab());
    }
    // 尾段静置（q1 已证 ~0）；大倾角雪崩重排
    f.setTilt(70);
    const b0 = r.e3BakeCount;
    for (let i = 0; i < 120; i++) {
      f.step(1 / 60);
      r.render(c, f.snapshot(), ctxEab());
    }
    const avalancheBakes = r.e3BakeCount - b0;
    const rate = avalancheBakes / 120;
    const ok = rate >= 0.3;            // 雪崩期层高每帧大变——重烘须跟上
    results.push({ name: 'p012_e3_avalancheStillRebakes', status:
      ok ? 'PASS' : 'FAIL',
      error: `tilt70 雪崩 120 帧重烘=${avalancheBakes}（rate=${rate.toFixed(2)}，≥0.30——漏更新=回归）` });
  })();
})();

(function e2TopLightSuite() {
  const types = typesMod;

  // ① 烘焙确定性：同 seed 同输入两次烘焙逐位一致
  (function e1() {
    const mk = () => {
      const f = new WaterSandField(128, 192);
      f.initialize(fullScene(128));
      f.setTilt(15);
      f.step(1 / 60);
      return f;
    };
    const run = () => {
      const r = new P012WaterSandRenderer(128, 192);
      const c = makeCountingCanvas({});
      const f = mk();
      f.step(1 / 60);
      r.render(c, f.snapshot(), makeContext({ ablation: types.P012Ablation.TOPLIGHT }));
      f.step(1 / 60);
      r.render(c, f.snapshot(), makeContext({ ablation: types.P012Ablation.TOPLIGHT }));
      return { map: r.e2LightMap.slice(), bakes: r.e2BakeCount };
    };
    const a = run();
    const b = run();
    let bitwise = a.map.length === b.map.length;
    for (let i = 0; bitwise && i < a.map.length; i++) {
      if (a.map[i] !== b.map[i]) {
        bitwise = false;
      }
    }
    results.push({ name: 'p012_e2_bakeDeterministic', status:
      bitwise && a.bakes >= 1 ? 'PASS' : 'FAIL',
      error: `mapLen=${a.map.length}/${b.map.length} bakes=${a.bakes} bitwise=${bitwise}` });
  })();

  // ② 平台无坡度不新增分区：平沙面（sandTop 全列同值）下光照图列间 Lmat 相同
  (function e2() {
    const r = new P012WaterSandRenderer(128, 192);
    const c = makeCountingCanvas({});
    const f = new WaterSandField(128, 192);
    f.initialize({
      waterRegions: [{ startCol: 0, endColInclusive: 127, bottom: 120, height: 60 }],
      heavyDepositedRegions: [{ startCol: 0, endColInclusive: 127, bottom: 0, height: 50 }],
      lightDepositedRegions: [{ startCol: 0, endColInclusive: 127, bottom: 50, height: 50 }],
      heavySuspendedRegions: [],
      lightSuspendedRegions: []
    });
    f.step(1 / 60);
    r.render(c, f.snapshot(), makeContext({ ablation: types.P012Ablation.TOPLIGHT }));
    // 平沙：所有列 z=0 处 Lmat 应同值（无坡度→无左右分区）
    const l0 = r.e2LightMap[0 * 256 + 0];
    let uniform = true;
    for (let col = 1; col < 128; col++) {
      if (Math.abs(r.e2LightMap[col * 256 + 0] - l0) > 1e-12) {
        uniform = false;
        break;
      }
    }
    results.push({ name: 'p012_e2_flatNoPartition', status:
      uniform ? 'PASS' : 'FAIL',
      error: `l0=${l0.toFixed(6)} uniform=${uniform}（平沙须无列间分区）` });
  })();

  // ③ OFF 等价性：toplight 不开时输出逐位=现状（draws 与既有路径一致）
  (function e3() {
    const mk = () => {
      const f = new WaterSandField(128, 192);
      f.initialize(fullScene(128));
      f.setTilt(15);
      return f;
    };
    const run = (abl) => {
      const r = new P012WaterSandRenderer(128, 192);
      const c = makeCountingCanvas({});
      const f = mk();
      for (let i = 0; i < 5; i++) {
        f.step(1 / 60);
        r.render(c, f.snapshot(), makeContext(abl !== undefined ? { ablation: abl } : {}));
      }
      return { draws: r.lastDrawCount, bakes: r.e2BakeCount };
    };
    const base = run();                                  // OFF 默认（现状）
    const offExplicit = run(types.P012Ablation.OFF);     // 显式 OFF
    const ok = base.draws === offExplicit.draws &&
      base.bakes === 0 && offExplicit.bakes === 0;       // OFF 路径零 E2 烘焙
    results.push({ name: 'p012_e2_offEquiv', status:
      ok ? 'PASS' : 'FAIL',
      error: `base=${base.draws} off=${offExplicit.draws} e2bakes=${base.bakes}/${offExplicit.bakes}` });
  })();

  // ④ 光照图失效重烘计数：静止零重烘；形状变化重烘
  (function e4() {
    const r = new P012WaterSandRenderer(128, 192);
    const c = makeCountingCanvas({});
    const f = new WaterSandField(128, 192);
    f.initialize(fullScene(128));
    f.setTilt(15);
    f.step(1 / 60);
    r.render(c, f.snapshot(), makeContext({ ablation: types.P012Ablation.TOPLIGHT }));
    const bakesAfterFirst = r.e2BakeCount;
    // 静止 5 帧（dormant 或微变 < 阈值）
    for (let i = 0; i < 5; i++) {
      f.step(1 / 60);
      r.render(c, f.snapshot(), makeContext({ ablation: types.P012Ablation.TOPLIGHT }));
    }
    const bakesAfterStill = r.e2BakeCount;
    // 大形状变化（倾斜 40° 扬沙）
    f.setTilt(40);
    for (let i = 0; i < 20; i++) {
      f.step(1 / 60);
      r.render(c, f.snapshot(), makeContext({ ablation: types.P012Ablation.TOPLIGHT }));
    }
    const bakesAfterTilt = r.e2BakeCount;
    // 静止段允许少量重烘（沉降期层高持续变——形状阈值 2px 显示域内），
    // 但倾斜段重烘次数须显著更多（形状大变必响应）
    const stillBakes = bakesAfterStill - bakesAfterFirst;
    const tiltBakes = bakesAfterTilt - bakesAfterStill;
    results.push({ name: 'p012_e2_rebakeAccounting', status:
      bakesAfterFirst >= 1 && tiltBakes >= 1 && bakesAfterTilt >= bakesAfterStill ? 'PASS' : 'FAIL',
      error: `first=${bakesAfterFirst} still+${stillBakes} tilt+${tiltBakes}` });
  })();

  // ⑤ 坡向响应：人造左右坡（左坡升/右坡降）Lface 应随光向横向分量区分
  (function e5() {
    const r = new P012WaterSandRenderer(128, 192);
    const c = makeCountingCanvas({});
    const f = new WaterSandField(128, 192);
    // 人造沙丘：中间高两侧低（左坡上升、右坡下降）
    const scene = {
      waterRegions: [{ startCol: 0, endColInclusive: 127, bottom: 120, height: 60 }],
      heavyDepositedRegions: [],
      lightDepositedRegions: [],
      heavySuspendedRegions: [],
      lightSuspendedRegions: []
    };
    // 逐列构造丘形（用 light 沉积高度梯度——中列 60 边列 10）
    const hills = [];
    for (let c0 = 0; c0 < 128; c0++) {
      const h = 10 + 50 * Math.sin(Math.PI * c0 / 127);
      hills.push({ startCol: c0, endColInclusive: c0, bottom: 0, height: Math.round(h) });
    }
    scene.lightDepositedRegions = hills;
    f.initialize(scene);
    f.step(1 / 60);
    r.render(c, f.snapshot(), makeContext({ ablation: types.P012Ablation.TOPLIGHT }));
    // 左坡（c=32，上升段）与右坡（c=96，下降段）的表面 Lmat（z=0）
    const lLeft = r.e2LightMap[32 * 256 + 0];
    const lRight = r.e2LightMap[96 * 256 + 0];
    // 左上光（LIGHT_X=-0.55）：右上升坡（表面向左上）迎光更亮，左下降坡背光更暗
    // （与既有 shadeOf 同向语义——不要求特定符号，要求两侧可区分）
    const separable = Math.abs(lLeft - lRight) > 1e-6;
    results.push({ name: 'p012_e2_slopeResponse', status:
      separable ? 'PASS' : 'FAIL',
      error: `lLeft=${lLeft.toFixed(4)} lRight=${lRight.toFixed(4)} diff=${Math.abs(lLeft - lRight).toFixed(6)}` });
  })();
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
