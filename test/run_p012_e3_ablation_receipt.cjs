// P-012 E3 std 超界 A/B 归因回执（freeze-blocker §2 五项证据）
// 全部 Node 通道：同 s132 构建（HEAD 代码）、同初态/seed/几何/输入、同帧配对
// 证据①缓冲失效账 ②像素链账 ③64 vs 256 宽对照（首帧强制重烘）
// ④强制重烘对照（dirty 策略 × bakedPx） ⑤s117 复算口径归档说明
const fs = require('fs');
const Module = require('module');
const ts = require('C:/Program Files/Huawei/DevEco Studio/tools/hvigor/hvigor/node_modules/typescript');
const hypium = {
  describe: (_n, f) => f(), it: (n, _f, fn) => { try { fn(); } catch (e) { /* ignore */ } },
  expect: v => ({ assertEqual: () => {}, assertTrue: () => {} })
};
const _load = Module._load;
Module._load = function (name, parent, main) {
  if (name === '@ohos/hypium') return hypium;
  if (name === '@ohos.hilog' || name === '@ohos/hilog') return { default: { info: () => {}, error: () => {}, warn: () => {}, debug: () => {} } };
  return _load.call(this, name, parent, main);
};
require.extensions['.ets'] = (mod, filename) => mod._compile(ts.transpileModule(
  fs.readFileSync(filename, 'utf8'), { compilerOptions: { target: ts.ScriptTarget.ES2020, module: ts.ModuleKind.CommonJS } }).outputText, filename);

const physics = require('../entry/src/main/ets/physics/WaterSandField.ets');
const rendererMod = require('../entry/src/main/ets/render/P012WaterSandRenderer.ets');
const typesMod = require('../entry/src/main/ets/render/P012RenderTypes.ets');
const { WaterSandField } = physics;
const { P012WaterSandRenderer, P012_APPEARANCE_ROUTE_ID } = rendererMod;
const { P012AppearanceTier, P012Ablation, P012FillBox } = typesMod;

// ── 复测场景重建（run_p012_e3_recheck 的 want 链：seed=20261020/thick/tier=3）──
const cols = 128, capacity = 192;
function makeField() {
  const f = new WaterSandField(cols, capacity);
  f.initialize({
    waterRegions: [{ startCol: 0, endColInclusive: cols - 1, bottom: 120, height: 60 }],
    heavyDepositedRegions: [{ startCol: 0, endColInclusive: cols - 1, bottom: 0, height: 50 }],
    lightDepositedRegions: [{ startCol: 0, endColInclusive: cols - 1, bottom: 50, height: 50 }],
    heavySuspendedRegions: [{ startCol: 0, endColInclusive: cols - 1, bottom: 110, height: 10 }],
    lightSuspendedRegions: [{ startCol: 0, endColInclusive: cols - 1, bottom: 110, height: 10 }]
  });
  return f;
}
// 真机几何（s132 epoch 日志 box 263x311@51,215 的等比 Node 复现——用屏幕大几何
// 会导致 buffer 高度不同；回执用真机同比例 boxW=263 boxH=311 保持 64 宽下
// buffer 高 78 与真机一致）
const GEO = { boxX: 51, boxY: 215, boxW: 263, boxH: 311 };
const DT = 1 / 60;

let serial = 0;
function mkCtx(ablation) {
  return {
    fieldEpoch: 1, continuityEpoch: 1, geometryEpoch: 1,
    frameSerial: ++serial, sampleTimeMs: 1000 + serial * 16.7, frameDeltaMs: 16.7,
    inputMode: 'TEST', sourceKind: 'script', physicalRouteId: 'r', runIdentity: 'run-1',
    boxX: GEO.boxX, boxY: GEO.boxY, boxW: GEO.boxW, boxH: GEO.boxH,
    pixelRatio: 1, visualInverted: false,
    appearanceTier: P012AppearanceTier.EAB, renderSeed: 20261020, configRevision: 1,
    ablation, fillBox: P012FillBox.TIGHT, canonicalEvidence: false
  };
}

// 计数 canvas：只统计（fill/fillRect/stroke/drawImage/clip），不真画
function makeCountingCanvas(sink) {
  return new Proxy({}, {
    get(_t, prop) {
      if (prop === 'clip') return () => { sink.clips++; };
      if (['fill', 'stroke', 'fillRect', 'strokeRect', 'drawImage'].includes(prop)) {
        return (...a) => {
          sink.draws++;
          if (prop === 'fillRect') {
            // 收集雾矩形（fog RGB 232,240,236）做纯 Node std 复算
            sink.rects.push([a[0], a[1], a[2], a[3]]);
          }
        };
      }
      if (['save', 'restore', 'beginPath', 'closePath', 'translate', 'rotate', 'rect', 'moveTo', 'lineTo'].includes(prop)) return () => {};
      if (prop === 'createLinearGradient') return () => ({ addColorStop: () => {} });
      return undefined;
    },
    set(_t, p, v) {
      if (p === 'fillStyle') sink.styles.push(String(v));
      return true;
    }
  });
}

// 模拟「静态段」轨迹：t=0 起步 → 静置 settle（复测 static-0 截图≈冷启后 25s ≈ 1500 帧）
// 但 Node 上跑 1500 帧太慢；WaterSandField 静置会 dormant（substeps=0 层高冻结）——
// 关键窗口是沉降期（层高持续微变）。跑 300 帧（5s 沉降中段）+ 记录失效账。
function runTrajectory(renderer, ctx, opts) {
  const field = opts.field;
  const account = {
    // dirty 请求计数（按 e3Invalidate 决策路径分列）
    dirtyByLayer: 0, dirtyByQ: 0, dirtyByPhase: 0, dirtyByGeometry: 0, cleanHits: 0,
    // LAYER_EPS 拒绝账：每帧实际层高偏差 max|cur-prev|（prev=上帧快照，无条件更新）
    epsRejectedMaxDev: [],      // 每帧 [maxDev, frameIdx]
    // N1 账
    n1BakeRequested: 0, n1ShortCircuited: 0, n1Baked: 0,
    // buffer 年龄
    waterBakeAtFrame: [], ifaceBakeAtFrame: [],
    // 帧时
    frameMs: []
  };
  const n = cols;
  let prevSmooth = null;
  let prevQ = null;
  let lastWaterBake = -1, lastIfaceBake = -1;
  // monkey-patch 计数（不改生产代码——包一层）
  const origBakeWater = renderer.e3BakeWaterBuffer.bind(renderer);
  const origBakeIface = renderer.e3BakeIfaceBuffer.bind(renderer);
  let frameIdx = 0;
  renderer.e3BakeWaterBuffer = function (...a) {
    origBakeWater(...a);
    lastWaterBake = frameIdx;
    account.waterBakeAtFrame.push(frameIdx);
  };
  renderer.e3BakeIfaceBuffer = function (...a) {
    origBakeIface(...a);
    lastIfaceBake = frameIdx;
    account.ifaceBakeAtFrame.push(frameIdx);
  };
  // N1 计数（e3Noise.shiftAndBake 请求 → 内部短路或实烘）
  const noise = renderer.e3Noise;
  if (noise !== null && noise !== undefined) {
    const origShift = noise.shiftAndBake.bind(noise);
    let n1Requested = 0, n1Baked = 0;
    noise.shiftAndBake = function (x0, y0, w, h) {
      n1Requested++;
      const before = noise.baked !== null;
      origShift(x0, y0, w, h);
      // 短路判定：同窗口时 baked 引用不变（无重烘）。用近似：记录 bakedW 是否变化
      n1Baked++;
      account.n1BakeRequested = n1Requested;
      account.n1Baked = n1Baked;
      account.n1ShortCircuited = n1Requested - n1BakedReal(noise, x0, w, h);
    };
    // 用窗口变化判定实烘
    let lastWinKey = null;
    function n1BakedReal(nz, x0, w, h) {
      const key = `${Math.round(x0 * 2)}_${Math.round(w)}`;
      const changed = lastWinKey !== null && key !== lastWinKey;
      lastWinKey = key;
      return changed ? 1 : 0;
    }
  }
  const t0 = Date.now();
  for (let i = 0; i < opts.frames; i++) {
    frameIdx = i;
    field.step(DT);
    // 失效账复算（与 e3Invalidate 同语义）：层高/q/phase
    const view = field.snapshot();
    // 读 renderer 平滑层（render 内部 buildDisplayProfiles 后的值在下帧 prev）
    const snap = renderer.render.length; // noop
    const ctxObj = mkCtx(opts.ablation !== undefined ? opts.ablation : P012Ablation.OFF);
    const tFrame = Date.now();
    renderer.render(ctx, view, ctxObj);
    account.frameMs.push(Date.now() - tFrame);
    // 层高偏差账：从 field 快照取原始层高（未经平滑），对照上帧
    if (prevSmooth !== null) {
      let maxDev = 0;
      for (let c = 0; c < n; c++) {
        maxDev = Math.max(maxDev, Math.abs(view.bh[c] - prevSmooth.bh[c]));
      }
      account.epsRejectedMaxDev.push([maxDev.toFixed(4), i]);
    }
    if (prevSmooth === null) {
      prevSmooth = { bh: new Float64Array(n) };
    }
    for (let c = 0; c < n; c++) prevSmooth.bh[c] = view.bh[c];
  }
  account.bakeCountWater = account.waterBakeAtFrame.length;
  account.bakeCountIface = account.ifaceBakeAtFrame.length;
  account.waterBufferAgeAtEnd = account.waterBakeAtFrame.length > 0
    ? (opts.frames - 1 - account.waterBakeAtFrame[account.waterBakeAtFrame.length - 1]) : -1;
  account.totalMs = Date.now() - t0;
  return account;
}

// ════════ 证据③+④ 主实验矩阵 ════════
// 变量：BUFFER_W（64/128/256）× dirty 策略（现行 LAYER_EPS=0.25 vs 强制每帧重烘）
//       × N1 bakedPx（2 vs 1）——首帧必重烘（新 renderer）
// 固定：同 field 轨迹（同 seed 同初态同输入）、segU/segV 上屏参数按构建现值
function runCase(bufW, forceRebake, bakedPx, frames) {
  // 动态改常量：P012_E3_BUFFER_W 是模块级 const——通过 renderer 构造后
  // 重建 buffer 的期望值不好注入。改用直接改 renderer 私有字段 +
  // e3Invalidate 的重建检测路径：先按默认建（64），再手动替换 buffer。
  const field = makeField();
  field.setTilt(15);        // 与单测/复测轨迹一致的初始倾斜
  field.step(DT);
  const renderer = new P012WaterSandRenderer(cols, capacity);
  const sink = { draws: 0, clips: 0, styles: [], rects: [] };
  const ctx = makeCountingCanvas(sink);
  // 首帧渲染（触发 buffer 创建 + 首烘）
  field.step(DT);
  renderer.render(ctx, field.snapshot(), mkCtx(P012Ablation.OFF));
  // 替换 buffer 为目标宽度（P012E3FogBuffer 构造签名 (width, boxW, boxH)）
  const BufCtor = renderer.e3WaterBuffer.constructor;
  const scaleH = Math.round(bufW * GEO.boxH / GEO.boxW);
  const newW = new BufCtor(bufW, GEO.boxW, GEO.boxH);
  const newI = new BufCtor(bufW, GEO.boxW, GEO.boxH);
  renderer.e3WaterBuffer = newW;
  renderer.e3IfaceBuffer = newI;
  // N1 bakedPx：e3Noise.bakedPx 可改（bakeCoverage 用 BAKE_PX 常量——
  // 需要重 bake 一次目标步长）。BAKE_PX 是类静态 readonly，monkey-patch：
  if (bakedPx !== 2) {
    Object.defineProperty(renderer.e3Noise.constructor, 'BAKE_PX', { value: bakedPx, configurable: true, writable: true });
    // 强制重 bake 一次（清空 baked → 下次 shiftAndBake 必烘目标步长）
    renderer.e3Noise.baked = null;
  }
  if (forceRebake) {
    // 强制每帧重烘：每帧 render 前把 prev 快照置 NaN（e3Invalidate 对 NaN 恒 dirty）
    // 简化注入：monkey-patch e3Invalidate 返回恒 true
    renderer.e3Invalidate = function () { return [true, true]; };
  }
  // 跑轨迹并收集「最终帧」的雾 std（用 buffer premul alpha 的空间 std——
  // 与设备 pxStd 同语义的近似：I2 alpha 分布的空间变异性）
  const framesN = frames || 60;
  let lastAccount = null;
  for (let ph = 0; ph < 3; ph++) {
    // 轨迹三段：静置（settle）→ 慢倾（tilt）→ 回正（return）——复测同结构
    const tilt = ph === 0 ? 15 : ph === 1 ? 40 : 0;
    field.setTilt(tilt);
    for (let i = 0; i < framesN; i++) {
      field.step(DT);
      renderer.render(ctx, field.snapshot(), mkCtx(P012Ablation.OFF));
      sink.rects.length = 0;
    }
  }
  // 读最终 buffer alpha 分布（water buffer，水体雾主体）
  const wb = renderer.e3WaterBuffer;
  const alphas = [];
  let capCount = 0, total = 0, sum = 0;
  for (let v = 0; v < wb.bufferHeight; v++) {
    for (let u = 0; u < wb.bufferWidth; u++) {
      const a = wb.readPremulAlpha(u, v);
      total++;
      sum += a;
      if (a > 0) alphas.push(a);
      if (a >= 0.779) capCount++;     // alpha≈0.78 cap 占比
    }
  }
  const mean = total > 0 ? sum / total : 0;
  let varSum = 0, nA = alphas.length || 1;
  for (const a of alphas) varSum += (a - mean) * (a - mean);
  const std = Math.sqrt(varSum / nA);
  return {
    bufW, forceRebake, bakedPx,
    waterAlpha: {
      mean: +mean.toFixed(4), std: +std.toFixed(4),
      capRatio: +(capCount / total).toFixed(4),
      nonzeroRatio: +(alphas.length / total).toFixed(4)
    },
    e3BakeCount: renderer.e3BakeCount,
    lastDrawCount: renderer.lastDrawCount
  };
}

// ════════ 证据① 缓冲失效账（现行默认 64+EPS 0.25+bakedPx2，全轨迹）════════
console.log('═══ 证据① 缓冲失效账（默认 s132 配置，settle 300 帧）═══');
{
  const field = makeField();
  field.setTilt(15);
  field.step(DT);
  const renderer = new P012WaterSandRenderer(cols, capacity);
  const sink = { draws: 0, clips: 0, styles: [], rects: [] };
  const ctx = makeCountingCanvas(sink);
  // 包 account 的轨迹跑法（复用 runTrajectory 的计数 monkey-patch）
  const acc = runTrajectory(renderer, ctx, { field, frames: 300 });
  // LAYER_EPS 拒绝账：层高偏差 <0.25 的帧 = 被拒（现行快照无条件更新 → 小增量不累计）
  let rejected = 0, accepted = 0;
  let rejectedMaxDev = 0;
  for (const [dev] of acc.epsRejectedMaxDev) {
    const d = parseFloat(dev);
    if (d < 0.25) { rejected++; rejectedMaxDev = Math.max(rejectedMaxDev, d); }
    else accepted++;
  }
  // 「被拒时累计层高偏差」：从上一次被接受帧起累计（无条件快照更新 → 偏差不累计，
  // 每帧只看与上帧差——这正是 B 疑点的账面证据）
  console.log(`  bake(water)=${acc.bakeCountWater} bake(iface)=${acc.bakeCountIface}`);
  console.log(`  water 重烘帧: [${acc.waterBakeAtFrame.slice(0, 12).join(',')}${acc.waterBakeAtFrame.length > 12 ? ',…' : ''}]`);
  console.log(`  EPS 拒绝: ${rejected}/${acc.epsRejectedMaxDev.length} 帧（层高帧间偏差<0.25），接受 ${accepted}`);
  console.log(`  被拒帧最大帧间偏差: ${rejectedMaxDev.toFixed(4)} 物理格`);
  console.log(`  N1: 请求=${acc.n1BakeRequested} 实烘=${acc.n1Baked} 短路=${acc.n1ShortCircuited}`);
  console.log(`  buffer 末龄（距最后一次 water 烘的帧数）: ${acc.waterBufferAgeAtEnd}`);
  console.log(`  300 帧总耗时 ${acc.totalMs}ms（Node）`);
}

// ════════ 证据② 像素链账（默认配置，末帧快照 hash + alpha 分布）════════
console.log('\n═══ 证据② 像素链账（默认 64/EPS0.25/bakedPx2，末帧）═══');
{
  const r = runCase(64, false, 2, 20);
  console.log(JSON.stringify(r, null, 1));
}

// ════════ 证据③ 同快照分辨对照（首帧强制重烘，64 vs 128 vs 256）════════
console.log('\n═══ 证据③ 64 vs 128 vs 256（首帧强制重烘+同输入轨迹）═══');
{
  const sizes = [64, 128, 256];
  const results = [];
  for (const w of sizes) {
    const t0 = Date.now();
    const r = runCase(w, true, 2, 40);
    r.caseMs = Date.now() - t0;
    results.push(r);
    console.log(`  W=${w}: alpha.mean=${r.waterAlpha.mean} alpha.std=${r.waterAlpha.std} ` +
      `cap=${r.waterAlpha.capRatio} nonzero=${r.waterAlpha.nonzeroRatio} bakes=${r.e3BakeCount} (${r.caseMs}ms)`);
  }
  // std 对比表：64 vs 256 的 alpha std 差 = 采样面贡献
  const s64 = results[0].waterAlpha.std, s256 = results[2].waterAlpha.std;
  console.log(`  → alpha.std 64→256 变化: ${s64} → ${s256}（${((s256 / s64 - 1) * 100).toFixed(1)}%）`);
}

// ════════ 证据④ 强制重烘对照（64 固定，现行 vs 强制；bakedPx 2 vs 1）════════
console.log('\n═══ 证据④ dirty 策略 × bakedPx 对照（64 宽固定）═══');
{
  const cases = [
    { label: '现行(EPS0.25+bakedPx2)', force: false, px: 2 },
    { label: '强制重烘+bakedPx2', force: true, px: 2 },
    { label: '现行+bakedPx1', force: false, px: 1 },
    { label: '强制重烘+bakedPx1', force: true, px: 1 }
  ];
  for (const c of cases) {
    const t0 = Date.now();
    const r = runCase(64, c.force, c.px, 40);
    console.log(`  ${c.label}: alpha.std=${r.waterAlpha.std} mean=${r.waterAlpha.mean} ` +
      `bakes=${r.e3BakeCount} (${Date.now() - t0}ms)`);
  }
}

// ════════ 证据⑤ s117 12.44/13.71 复算口径 ════════
console.log('\n═══ 证据⑤ s117 复算口径（已在 analysis-recheck-cleanROI-v1.json 归档）═══');
console.log('  s117 ON=12.44 OFF=13.71（追算 cleanROI rows 945-1160，见 blocker §1）');
console.log('  本回执全部 Node 通道 alpha std 是 I2 计算面分布，与设备 pxStd（Y 通道）');
console.log('  量纲不同；设备侧回界判定由 effector 修复构建重采执行（blocker §4 顺序）');
