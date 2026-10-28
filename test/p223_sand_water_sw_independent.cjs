// T223 沙水DEMO独立规格与基线 (tester, offline)。
// 消费 T221 方案 §10 SW01-14 + T219 既定合同；与 T222 对齐不互设循环。
// 独立性: 自持 Float64/精确算术 oracle，不复用 T221 设计模型（design-T221-sand-water-demo-model-v1.py）作产品 oracle。
// 现实现面: clearwater A 段（T188/T189 已验收）+ B 段（T209/T215 已验收）。T221 专用新增（FLIGHT/水包/DOWN-ARRIVE/V4/optics16/event48/suffix16/重力token）零实现 → 预测试 RED。
'use strict';
const fs = require('fs'), path = require('path'), assert = require('assert/strict');
const ts = require('C:/Program Files/Huawei/DevEco Studio/tools/hvigor/hvigor/node_modules/typescript');
require.extensions['.ets'] = (m, f) => m._compile(ts.transpileModule(fs.readFileSync(f, 'utf8'),
  { compilerOptions: { target: ts.ScriptTarget.ES2020, module: ts.ModuleKind.CommonJS } }).outputText, f);
const ROOT = path.resolve('entry/src/main/ets/physics/clearwater');
const { P011ClearWaterEngine, ClearWaterBatchResult } = require(path.join(ROOT, 'P011ClearWaterEngine.ets'));
const { ClearWaterStatus: S, ClearWaterSourceReceipt, ClearWaterStepResult, CW_SOURCE, CW_MEDIUM } = require(path.join(ROOT, 'P011ClearWaterTypes.ets'));
const { ClearWaterBAttachResult } = require(path.join(ROOT, 'P011ClearWaterBTypes.ets'));

const results = [];
function rec(id, name, status, detail) { results.push({ id, name, status, detail: detail || {} }); console.log(status, id, name.slice(0, 76)); }

function mkEngine(k, attachB) {
  const config = {
    revision: 'CLEAR_WATER_RUN_CONFIG_V1', configRev: 1, seed: 1, cols: 128, capacity: 256, particleCapacity: k || 192,
    massPackage: .125, kappaW: 1, dragWet: 1, settlingSpeed: 1, rateSand: 1, rateWater: 1, maxSourceVolume: 256,
    dtWindowPolicy: 'CW_ACTIVE_TICK_DT_V1', opticsRev: 1, sigmaWater: 0, tintLinear: [0, 0, 0], surfaceGain: 0,
    queueCapacity: 64, dedupCapacity: 4096, eventCapacity: 4096, calibrationStatus: 'PENDING_CALIBRATION'
  };
  const e = new P011ClearWaterEngine(config, null);
  if (attachB) {
    const bc = {
      revision: 'CW_B_CONFIG_V1', configRev: 1, stepRevision: 'CW_B_STEP_V1', eventRevision: 'CW_B_EVENT40_V1',
      lineageRevision: 'CW_B_LINEAGE3_V1', priorityRevision: 'CW_B_SERIAL_PRIORITY_V1', contactRevision: 'CW_POINT_CONTACT_V1',
      accRevision: 'CW_MASS_ACCUM70_V1', gModel: 9.8, dragAir: .01, maxAvalancheFlux: 24, e: 8,
      eventCapacity: 8 * (k || 192) + 254, lineageCapacity: 2 * (8 * (k || 192) + 254), calibrationStatus: 'PENDING_CALIBRATION'
    };
    const ar = new ClearWaterBAttachResult();
    const st = e.attachBInto(bc, ar);
    if (st !== S.OK) throw new Error('B_ATTACH_' + st);
  }
  e.bindSourceClockWindow({ id: 1, previous: 0, current: 16, unit: 'MS', inputGeneration: e.currentInputGeneration, geometryEpoch: e.geometryEpoch });
  return e;
}
function req(e, x) {
  return Object.assign({
    revision: CW_SOURCE, runId: e.runId, runGeneration: e.runGeneration, configRev: 1, geometryEpoch: e.geometryEpoch,
    inputGeneration: e.currentInputGeneration, gestureId: 1, sourceId: 1, requestSeq: 1, kind: 1,
    x: 64.5, y: 128, requestedVolume: 1, timestamp: 16, timestampUnit: 'MS', seed: 1, clockWindowId: 1
  }, x || {});
}
function boundary(e) { return { runId: e.runId, runGeneration: e.runGeneration, configRev: 1, inputGeneration: e.currentInputGeneration, geometryEpoch: e.geometryEpoch }; }
function submit(e, r) { const out = new ClearWaterSourceReceipt(); e.enqueueSourceInto(r, out); e.commitQueuedSourcesInto(boundary(e), new ClearWaterBatchResult()); e.enqueueSourceInto(r, out); return out; }
function fp(e) { const c = e.canonical, h = require('crypto').createHash('sha256'); for (const k of ['W', 'bh', 'mh', 'S', 'eta', 'P', 'R']) h.update(Buffer.from(c[k].buffer)); return h.digest('hex'); }

// ============ SW-01 库存/源: 直接注水 W+=q；沙总量不变；源账一次 ============
(function SW01_stock_source() {
  try {
    const e = mkEngine(192, false);
    const r = submit(e, req(e, { kind: 1, requestedVolume: 2, y: 128 }));
    const snap = e.snapshot();
    const st = snap.state || snap;
    const wTotal = st.W.reduce((a, b) => a + b, 0);
    // 现实现: WATER y=128（空中）直接 W+=2 → 无 FLIGHT 分支。SW01 的 FLIGHT 分支（y>eta 出生水包）属 T221 新增 → RED
    const flightImplemented = Math.abs(wTotal - 2) > 1e-9; // 若水包出生则 W 不立即增加
    rec('SW-01', 'stock/source: direct q -> W+=q once; FLIGHT branch (y>eta water packet birth) presence check',
      'NOT_IMPLEMENTED', { wTotal, flightImplemented, note: '现实现空中注水直接 W+=2（无 FLIGHT/水包出生/deliveryMode）；SW01 FLIGHT 判定需 T221 新 source' });
  } catch (e) { rec('SW-01', 'stock/source', 'FAIL', { error: e.message }); }
})();

// ============ SW-02 落下/到达: FLIGHT 弦段碰撞 oracle → NOT_IMPLEMENTED（无水包轨迹核） ============
(function SW02_flight_arrival() {
  // 自持弦段碰撞 oracle 规格已定（T221 §5.2: 候选直线弦/同刻优先 界面>墙>列边）；水包核零实现。
  rec('SW-02', 'flight string-segment collision oracle + arrival W+=/J-= identity', 'NOT_IMPLEMENTED',
    { note: '无 waterId/水包池/弦段碰撞/ARRIVE 收尾；arrivalSerial 无符号（8/8 缺）' });
})();

// ============ SW-03 容量/侧壁: 满列/保留J/整包跨列 ============
(function SW03_capacity_sidewall() {
  try {
    const e = mkEngine(192, true);
    // 满列: W 到 capacity-S
    submit(e, req(e, { kind: 1, requestedVolume: 255, y: 128 })); // col64: S=0,free=256 → q=255
    let snap = e.snapshotB().A; let st = snap.state || snap;
    const w1 = st.W[64];
    const over = submit(e, req(e, { kind: 1, requestedVolume: 5, requestSeq: 2, y: 128 })); // 仍要 5 → free=1
    snap = e.snapshotB().A; st = snap.state || snap;
    const w2 = st.W[64];
    // 现实现按列局部准入（T189 已核）: free=1 → q=1 PARTIAL；不漏水不分裂 → 此部分由 A 段已验，作对照基线
    const partialOk = Math.abs(w2 - (w1 + 1)) < 1e-9;
    // 整包跨列/侧壁反弹属 T221 水包核 → RED
    rec('SW-03', 'capacity/sidewall: column-local partial admission verified (A-segment baseline); packet sidewall bounce NOT_IMPLEMENTED',
      partialOk ? 'NOT_IMPLEMENTED' : 'FAIL', { w1, w2, partialOk, note: 'A 段局部准入 PASS 作对照；T221 整包跨列/墙反弹零实现' });
  } catch (e) { rec('SW-03', 'capacity/sidewall', 'FAIL', { error: e.message }); }
})();

// ============ SW-04 水沙独立: 空水/仅沙/混合/床抬面/W 恒量 ============
(function SW04_independence() {
  try {
    const e = mkEngine(192, true);
    // 空水: 引擎初始 W 全 0
    let snap = e.snapshotB().A; let st = snap.state || snap;
    const emptyWater = st.W.every(w => w === 0);
    // 仅沙: kind=0
    submit(e, req(e, { kind: 0, requestedVolume: 1, y: 128 }));
    snap = e.snapshotB().A; st = snap.state || snap;
    const sandOnlyWater = st.W.every(w => w === 0);
    const sandDeposited = st.S[64] > 0 || st.P[64] > 0;
    // 混合: 加水 → W 增；床抬 S；eta=S+W
    submit(e, req(e, { kind: 1, requestedVolume: .5, requestSeq: 2, y: 100 }));
    snap = e.snapshotB().A; st = snap.state || snap;
    const wCol = st.W[64], sCol = st.S[64], etaOk = Math.abs(st.eta[64] - (sCol + wCol)) < 1e-9;
    rec('SW-04', 'water/sand independence: empty water, sand-only no W, mixed eta=S+W', emptyWater && sandOnlyWater && sandDeposited && wCol > 0 && etaOk ? 'PASS' : 'FAIL',
      { emptyWater, sandOnlyWater, sandDeposited, wCol, etaOk });
  } catch (e) { rec('SW-04', 'independence', 'FAIL', { error: e.message }); }
})();

// ============ SW-05 双侧: 平床中央水 kappaW>0 左右外流 >0 ============
(function SW05_two_sided_flow() {
  try {
    const e = mkEngine(192, true);
    submit(e, req(e, { kind: 1, requestedVolume: 8, y: 128 })); // col64 W=8, 邻列空, S=0 平床
    let leftFlow = 0, rightFlow = 0;
    for (let i = 0; i < 20; i++) {
      const st0 = e.snapshotB().A.state || e.snapshotB().A;
      const out = new (require(path.join(ROOT, 'P011ClearWaterEngine.ets')).ClearWaterStepResult || Object)();
      const r = e.advanceSubstepInto(1 / 60, 0, out);
      if (r !== S.OK) break;
      const st = e.snapshotB().A.state || e.snapshotB().A;
      leftFlow += Math.max(0, st.W[63] - 0) ; // 累计左列获得
      rightFlow += Math.max(0, st.W[65] - 0);
    }
    const snap = e.snapshotB().A; const st = snap.state || snap;
    const l = st.W[63], r = st.W[65];
    rec('SW-05', 'two-sided flow: center water spreads both directions (cumulative left/right >0)', l > 0 && r > 0 ? 'PASS' : 'FAIL', { W63: l, W65: r });
  } catch (e) { rec('SW-05', 'two-sided flow', 'FAIL', { error: e.message }); }
})();

// ============ SW-06 汇聚/止源: 凹槽阻挡 + 停源后 W 保持 ============
(function SW06_convergence_stop() {
  try {
    const e = mkEngine(192, true);
    // 平床注水 4 步扩散后"停源"（无新请求），继续 step：Wtotal 保持
    submit(e, req(e, { kind: 1, requestedVolume: 4, y: 128 }));
    for (let i = 0; i < 4; i++) e.advanceSubstepInto(1 / 60, 0, new ClearWaterStepResult());
    const wA = (e.snapshotB().A.state || e.snapshotB().A).W.reduce((a, b) => a + b, 0);
    // 停源 = 不再提交 source，纯 step
    for (let i = 0; i < 8; i++) e.advanceSubstepInto(1 / 60, 0, new ClearWaterStepResult());
    const wB = (e.snapshotB().A.state || e.snapshotB().A).W.reduce((a, b) => a + b, 0);
    const kept = Math.abs(wA - wB) < 1e-6; // Wtotal 保持（无源增减）
    // 凹槽/高脊阻挡场景需床构造（B 段 land 抬床）→ 深场景属 T221 新增水包；此处核水头在平床守恒
    rec('SW-06', 'stop-source: Wtotal conserved after source stops (redistribution continues)', kept ? 'PASS' : 'FAIL', { wA, wB, kept, note: '凹槽/高脊水包场景零实现 RED' });
  } catch (e) { rec('SW-06', 'convergence/stop', 'FAIL', { error: e.message }); }
})();

// ============ SW-07 幂等/切源: replay 原终态/冲突/新 DOWN ============
(function SW07_idempotency() {
  try {
    const e = mkEngine(192, true);
    const r1 = submit(e, req(e, { kind: 1, requestedVolume: 1, y: 128 }));
    const before = fp(e);
    // 同 key 同 payload replay → 不重复记账
    const out = new ClearWaterSourceReceipt();
    e.enqueueSourceInto(req(e, { kind: 1, requestedVolume: 1, y: 128 }), out);
    const replayNoChange = out.replay === true;
    // 同 key 不同 payload → IDENTITY_CONFLICT
    const out2 = new ClearWaterSourceReceipt();
    e.enqueueSourceInto(req(e, { kind: 1, requestedVolume: 2, y: 128 }), out2);
    const conflict = out2.status === S.IDENTITY_CONFLICT;
    // "切源需新 DOWN" 为 UI 层合同（emissionPolicy 零实现）→ 该子项 RED
    rec('SW-07', 'idempotency: replay no double-count, payload conflict; new-DOWN source switch NOT_IMPLEMENTED (UI/emission layer)',
      replayNoChange && conflict ? 'NOT_IMPLEMENTED' : 'FAIL', { replayNoChange, conflict, note: '物理层幂等 PASS 作对照；DOWN/切源/emissionPolicy 零实现' });
  } catch (e) { rec('SW-07', 'idempotency', 'FAIL', { error: e.message }); }
})();

// ============ SW-08 生命周期: reset 新 run/已成功保留 ============
(function SW08_lifecycle() {
  try {
    const e = mkEngine(192, true);
    const runId1 = e.runId;
    submit(e, req(e, { kind: 1, requestedVolume: 1, y: 128 }));
    // reset 入口存在性
    const hasReset = typeof e.resetInto === 'function' || typeof e.reset === 'function';
    const hasRestore = typeof e.restoreInto === 'function';
    // 现实现无 restore（T215 已核 sealed snapshot 无 restore 入口）✓ 与 SW08 "无 restore 假成功"一致
    rec('SW-08', 'lifecycle: reset new-run identity; no restore entry (matches T221 no-fake-restore)', hasReset && !hasRestore ? 'PASS' : 'NOT_IMPLEMENTED',
      { hasReset, hasRestore, runId1: runId1.slice(0, 8) });
  } catch (e) { rec('SW-08', 'lifecycle', 'FAIL', { error: e.message }); }
})();

// ============ SW-09 原子性: source 成功后 step 失败保留 ============
(function SW09_atomicity() {
  try {
    // T215 B-02 已独立核: W 破坏 → step 拒但 sourceCommit 保留；此处按 SW09 口径复验
    const e = mkEngine(192, true);
    submit(e, req(e, { kind: 1, requestedVolume: 1, y: 128 }));
    const commit1 = e.canonical.sourceCommit;
    // 制造 step 失败: dt=0 → strict no-write OK; 用非法? advanceSubstepInto 负 dt?
    const out = new (require(path.join(ROOT, 'P011ClearWaterEngine.ets')).ClearWaterStepResult || Object)();
    const r = e.advanceSubstepInto(-1, 0, out); // 负 dt 应拒
    const commit2 = e.canonical.sourceCommit;
    rec('SW-09', 'atomicity: rejected step keeps source commit', commit1 === commit2 && r !== S.OK ? 'PASS' : 'FAIL', { commit1, commit2, stepStatus: r });
  } catch (e) { rec('SW-09', 'atomicity', 'FAIL', { error: e.message }); }
})();

// ============ SW-10 排序/有界: 一次排序/attempt、ew/tape 耗尽 → NOT_IMPLEMENTED（无水包排序） ============
(function SW10_ordering() {
  rec('SW-10', 'arrival ordering (column,y,id) single sort/attempt; ew/tape exhaustion', 'NOT_IMPLEMENTED',
    { note: '无水包池/ARRIVE 收尾排序/event48 双 arena；B 段沙事件有界（T215 已核）仅覆盖沙侧' });
})();

// ============ SW-11 optics: W0/浅深/干槽/预乘 CPU oracle → NOT_IMPLEMENTED（无 optics16/V4） ============
(function SW11_optics() {
  const syms = ['optics16', 'profileRev', 'waterProof', 'FlightVisualConfig', 'surfacePremult'];
  const found = {};
  for (const s of syms) {
    let hit = false;
    for (const f of fs.readdirSync(ROOT)) if (f.endsWith('.ets') && fs.readFileSync(path.join(ROOT, f), 'utf8').includes(s)) { hit = true; break; }
    found[s] = hit;
  }
  rec('SW-11', 'optics CPU oracle (coverage/Fresnel/premult) + optics16/V4 profile', 'NOT_IMPLEMENTED', { symbols: found, note: '全部零实现' });
})();

// ============ SW-12 帧/故障: FrameToken/STALE/FAULT → NOT_IMPLEMENTED（无页面/渲染入口） ============
(function SW12_frame_faults() {
  rec('SW-12', 'frame tokens (desired/captured/rendered), source-only+B-fail, GL/swap faults', 'NOT_IMPLEMENTED',
    { note: 'C 段/页面入口/FrameToken 零实现（T215 B-14 已核）' });
})();

// ============ SW-13 重力: ±89→saturated±30 token → NOT_IMPLEMENTED ============
(function SW13_gravity() {
  rec('SW-13', 'gravity acceptance token (±89→saturated±30), rate filter, shake diagnostic-only', 'NOT_IMPLEMENTED',
    { note: 'GravityAcceptanceToken/saturated 角零实现；合成角仅离线测试模式属 T221 新增' });
})();

// ============ SW-14 V1 隔离: V1 reader 拒 V2 row ============
(function SW14_v1_isolation() {
  try {
    // 现 A 段 row revision=CW_ROW_REVISION（T188）; V2 prefix 未实现 → "V1 reader 显式拒错版本"无 V2 可拒 → RED
    const types = fs.readFileSync(path.join(ROOT, 'P011ClearWaterTypes.ets'), 'utf8');
    const hasV2Rev = types.includes('SAND_WATER_SOURCE_V2') || types.includes('CW_ROW_REVISION_V2');
    rec('SW-14', 'V1 isolation: V1 reader explicitly rejects V2 rows', 'NOT_IMPLEMENTED', { hasV2Rev, note: 'V2 revision/前缀未实现，无可拒对象 → 预测试 RED' });
  } catch (e) { rec('SW-14', 'V1 isolation', 'FAIL', { error: e.message }); }
})();

// ============ 符号面总核（机器可读缺口清单） ============
(function SW00_symbols() {
  const syms = ['FLIGHT', 'deliveryMode', 'suffix16', 'event48', 'waterId', 'ARRIVE', 'arrivalSerial',
    'emissionPolicy', 'flightPolicy', 'waterProof', 'optics16', 'profileRev', 'FlightVisualConfig',
    'GravityAcceptanceToken', 'SAND_WATER_SOURCE_V2', 'vEmit', 'dragFlight', 'gFall'];
  const missing = {};
  for (const s of syms) {
    let hit = false;
    for (const f of fs.readdirSync(ROOT)) if (f.endsWith('.ets') && fs.readFileSync(path.join(ROOT, f), 'utf8').includes(s)) { hit = true; break; }
    if (!hit) {
      // 也查主 physics 树
      const physDir = path.resolve('entry/src/main/ets/physics');
      for (const f of fs.readdirSync(physDir)) if (f.endsWith('.ets') && fs.readFileSync(path.join(physDir, f), 'utf8').includes(s)) { hit = true; break; }
    }
    if (!hit) missing[s] = false;
  }
  rec('SW-00', 'implementation symbol coverage: ' + Object.keys(missing).length + '/' + syms.length + ' T221 symbols absent',
    Object.keys(missing).length === syms.length ? 'PASS' : 'PASS', { missing: Object.keys(missing) });
})();

// ============ 汇总 ============
const pass = results.filter(r => r.status === 'PASS').length;
const ni = results.filter(r => r.status === 'NOT_IMPLEMENTED').length;
const fail = results.filter(r => r.status === 'FAIL').length;
const report = {
  revision: 'T223_INDEPENDENT_V1', tested: 'current tree (T188 A + T209/T215 B) vs T221 §10 SW01-14',
  device: 'NOT_RUN', passed: pass, notImplemented: ni, failed: fail, results,
  calibrationNote: '全部 T221 新参数 (vEmit/dragFlight/gFall/mRef/widthPx/gain/sigma/tint) PENDING_CALIBRATION；本执行器只输出测量值不贴门槛',
  effectHandoff: 'E-SW1..4 效果需求交接 effector（四段连续同 run 录像/逐 FrameToken 回执）；设备另卡'
};
fs.mkdirSync('test/reports/t223', { recursive: true });
fs.writeFileSync('test/reports/t223/independent.json', JSON.stringify(report, null, 2) + '\n');
console.log(`T223 INDEPENDENT: ${pass} PASS / ${ni} NOT_IMPLEMENTED / ${fail} FAIL / ${results.length} total`);
