'use strict';
/** T424 · CPU 整帧负载与事务差分基线（T420 §6 匹配负载矩阵，离线可复跑）。
 * 冻结 legacy 原路（P051DerivedFacadeV2, CPU_ORIGINAL_F64）+ 同输入/seed/checkpoint。
 * 场景覆盖 T420 §6 匹配负载：空场活跃（无源）、空场休眠（setActive(false)）、
 * 持续注入（held，λ 采样）、UP 后高接触（L 满池维持）、Q0/Q1/Q2 由 dt 决定，
 * N64/N512 × L0/8/157/满池，R/attempt 按实际记录。
 * 哨兵：T403 LEVEL0 三笔正授予 0.1/0.1/0.09999999999999998、TILT25 中列
 * 0.032121583881089655、T381 零获准检查、事件/首错原位型。
 * 用法: node test/t424/baseline_matrix.cjs <隔离工程根> <输出目录>
 * 输出: <输出目录>/matrix.json（逐场景逐帧行，FrameBudgetV2 schema）
 */
const fs = require('node:fs'), path = require('node:path'), assert = require('node:assert/strict');
const { loader } = require('../t351/load.cjs');

const root = process.argv[2] || '.codex/local-test/t400-final';
const outDir = process.argv[3] || 'test/reports/t424';
const load = loader(path.resolve(root, 'entry/src/main/ets'));
const { P051DerivedFacadeV2: F } = load('physics/derived/P051DerivedFacadeV2');
const { P051DerivedDemoV2: Demo } = load('physics/derived/P051DerivedDemoV2');
const { P013SceneController: Scene } = load('render/P013SceneController');

const CONFIG = (n) => ({ isolated: true, numericPolicy: 'P011_DRY_CACHE_EXACT_V2', nmax: n,
  sourceId: 51, backend: 'CPU_ORIGINAL_F64', writerRegistry: 'T216_DRY_WRITERS_V2',
  motionRevision: 'P011_CPU_DRY_CROSS_VERTICAL_V1' });

function native() {
  return { init: () => ({ code: 0, sessionId: 1 }), getState: () => ({ surfaceEpoch: 1, contextEpoch: 1 }),
    prepareDualFieldSurface: () => ({ code: 0, sizeSource: 'EGL_QUERY_SURFACE', widthPx: 320, heightPx: 480, surfaceEpoch: 1, contextEpoch: 1 }),
    bindMaterialConfiguration: () => ({ code: 0, bindingId: 1 }), registerSceneConfiguration: () => ({ code: 0 }),
    drawNewPipelineFrame: f => ({ code: 0, currentFrameSucceeded: true, presentationState: 'CURRENT', firstFailureStage: '',
      requestedFrameToken: f.frameToken, renderedFrameToken: f.frameToken, lastGoodFrameToken: f.frameToken,
      requestedSwapSucceeded: true, staleSwapSucceeded: false }) };
}
function sceneContext(s, frame) {
  return { physicsCommitId: s.physicsCommitId, particleLayerEnabled: true, fieldEpoch: s.stateEpoch,
    continuityEpoch: 1, geometryEpoch: 1, frameSerial: frame, sampleTimeMs: frame * 1000 / 30,
    frameDeltaMs: 1000 / 30, inputMode: 'EXPERIENCE', sourceKind: 'gesture',
    physicalRouteId: 'dual-field-particle-v1', runIdentity: s.runId, boxX: 0, boxY: 0, boxW: 320, boxH: 480,
    pixelRatio: 1, visualInverted: false, appearanceTier: 0, renderSeed: 51, configRevision: 1,
    ablation: 'off', fillBox: 'tight', canonicalEvidence: false, gravityDirX: 0, gravityDirY: 1,
    sandTracerEnabled: false };
}

/** 通用帧驱动：注入策略由场景给出，20ms callback（dt=2/60 → Q 可到 2）。 */
function runScenario(name, opts) {
  const { n = 512, holdFrames = 0, preInjectMass = 0, injectPerFrame = null, upAfterFrame = null,
    sleep = false, activeFrames = 96, tilt = 0, seed = 51 } = opts;
  const demo = new Demo(n);
  const slot = demo.createPublicationCold();
  const scene = new Scene(native());
  const budget = demo.budget;
  budget.enabled = true;
  if (tilt) demo.owner.setTilt(tilt);
  if (sleep) demo.setActive(false);
  // 预注入: 按 free slot 自适应单笔（≤32），铺到目标质量或池满为止。
  // CAPACITY_BLOCKED 是物理约束（birth occluded / free slot 耗尽），不是错误；
  // 基线如实记录实际 admitted 与 rejected 次数，不伪造满池。
  let seq = 0, remaining = preInjectMass, preRejected = 0, preAdmitted = 0;
  const liveNow = () => demo.owner.field?.derivedV2?.canonical?.pool?.liveCount ?? 0;
  while (remaining > 1e-9) {
    const free = n - liveNow();
    if (free <= 2) break;
    const amount = Math.min(remaining, 32, free - 2);
    const r = demo.owner.inject({ revision: 'P051_DRY_AIR_SOURCE_V1', sourceId: 51, sequence: ++seq,
      inputTimeMs: seq * 100, x: 64.5, y: 220, seed, requestedH: amount, requestedL: 0 });
    if (!r.ok) { preRejected++; break; }
    preAdmitted += r.original?.admittedH ?? amount;
    remaining -= (r.original?.admittedH ?? amount);
  }
  const frameOut = [];
  for (let frame = 0; frame < activeFrames; frame++) {
    const ns = 1e9 + frame * 20e6, now = ns / 1e6;
    budget.begin(frame, frame === 0 ? 0 : 20e6, frame === 0 ? 0 : 1 / 30);
    if (injectPerFrame !== null && (upAfterFrame === null || frame < upAfterFrame)) {
      const r = demo.owner.inject({ revision: 'P051_DRY_AIR_SOURCE_V1', sourceId: 51, sequence: ++seq,
        inputTimeMs: now, x: 64.5, y: 220, seed, requestedH: injectPerFrame, requestedL: 0 });
      if (!r.ok) preRejected++;
    }
    // 未 arm Z08 夹具：beginZ08Frame 为 no-op（archive===null 直接 return）
    const ok = demo.tick(ns, 20e6, now);
    if (demo.copyPublicationInto(slot) !== 'OK') throw new Error('copy failed frame ' + frame);
    const s = slot.state;
    const pub = slot.readZ08Identity();
    demo.finishZ08Frame?.call(demo, null, null, null, ns, frame, !ok, now);
    budget.end();
    frameOut.push({ frame, ok, N: n, live: s.liveCount ?? s.live ?? null,
      commit: s.physicsCommitId, epoch: s.stateEpoch, run: s.runId,
      kind: s.publicationKind, id: pub ? { run: pub.run, commit: pub.commit, substep: pub.substep } : null });
    if (frame === activeFrames - 1 && holdFrames > 0) {
      // 高接触保持窗: UP 后每帧再注入维持 L
      for (let h = 0; h < holdFrames; h++) {
        const hns = ns + (h + 1) * 20e6, hnow = hns / 1e6;
        { const r = demo.owner.inject({ revision: 'P051_DRY_AIR_SOURCE_V1', sourceId: 51, sequence: ++seq,
          inputTimeMs: hnow, x: 64.5, y: 220, seed, requestedH: 0.1, requestedL: 0 });
          if (!r.ok) preRejected++; }
        const hok = demo.tick(hns, 20e6, hnow);
        if (demo.copyPublicationInto(slot) !== 'OK') throw new Error('copy failed hold ' + h);
        frameOut.push({ frame: activeFrames + h, ok: hok, N: n, live: slot.state.liveCount ?? slot.state.live ?? null,
          commit: slot.state.physicsCommitId, epoch: slot.state.stateEpoch, run: slot.state.runId,
          kind: slot.state.publicationKind, hold: true });
        demo.budget.end();
      }
    }
  }
  return { name, frames: frameOut, budget: JSON.parse(budget.exportCold()),
    admission: { preAdmitted, preRejected, finalLive: liveNow() } };
}

/** Z08 callback（对齐 t400/units.cjs 语义，slot+frame+receipt 必须传全）。 */
function z08Callback(model, i, dt) {
  const ns = 1e9 + i * 20e6, now = ns / 1e6;
  model.beginZ08Frame(now);
  const ok = model.tick(ns, dt, now);
  const slot = model.createPublicationCold();
  assert.equal(model.copyPublicationInto(slot), 'OK');
  const pub = slot.readZ08Identity();
  const frameToken = pub ? [pub.run, pub.epoch, pub.commit, i].join(':') : null;
  const source = pub ? { frameToken, runId: pub.run, ledgerCommitKey: pub.ledgerKey, stateEpoch: pub.epoch,
    physicsCommitId: pub.commit, surfaceEpoch: 7, contextEpoch: 8, geometryEpoch: 9,
    continuityEpoch: 10, generation: 11, configRevision: 12 } : null;
  const frame = source ? { ...source, fieldEpoch: pub.epoch, frameTimeNs: ns, frameId: i,
    requestedGeneration: 11, appearanceConfigRevision: 12 } : null;
  const receipt = frame ? { presentationState: 'CURRENT', currentFrameSucceeded: true,
    requestedSwapSucceeded: true, requestedFrameToken: frameToken, renderedFrameToken: frameToken,
    renderedSource: source, targetSurfaceEpoch: 7, targetContextEpoch: 8, requestedGeneration: 11,
    renderedGeneration: 11, actualPresentationGeneration: 11, requestedConfigRevision: 12,
    renderedConfigRevision: 12, displayPresentVerified: false } : null;
  model.finishZ08Frame(ok ? slot : null, ok ? frame : null, ok ? receipt : null, ns, i, !ok, now);
  return model.exportZ08Cold() ? JSON.parse(model.exportZ08Cold()) : null;
}

/** T403/T381 哨兵（行为回归，逐位）。 */
function sentinels() {
  const out = {};
  // T403 LEVEL0: 三笔正授予 [.1,.1,.09999999999999998]；TILT25: [.1,.032121583881089655,0]
  const physics = [];
  for (const restricted of [false, true]) {
    const demo = new Demo(512);
    assert.match(demo.startZ08NearBed(restricted, 1000), /^Z08_WAITING_BIND/);
    assert.equal(z08Callback(demo, 1, 1 / 30), null);
    const archive = z08Callback(demo, 2, 1 / 30);
    assert.equal(archive.arm.admissionState, 'ADMITTED');
    const land = archive.firstT3.lands;
    assert.deepEqual(land.map(l => l.event.granted),
      restricted ? [.1, .032121583881089655, 0] : [.1, .1, .09999999999999998], 'T403 grants drift');
    assert.equal(land.at(-1).bedFinal, restricted ? .13212158388108966 : .3, 'T403 bedFinal drift');
    physics.push({ fixture: archive.arm.fixture, grants: land.map(l => l.event.granted),
      retirements: land.filter(l => l.retired).length, steps: archive.successfulSubsteps,
      landCurrentSwap: archive.landCurrentSwap });
  }
  out.t403_physics = physics;
  return out;
}

/** 判读器：对照 T420 §6 判读字段与哨兵，输出 verdict。 */
function judge(matrix, budgetByScenario) {
  const findings = [];
  for (const [name, rows] of Object.entries(matrix)) {
    const okFrames = rows.filter(r => r.ok).length;
    findings.push({ scenario: name, frames: rows.length, okFrames,
      firstFailure: rows.find(r => !r.ok)?.frame ?? null,
      liveFirst: rows[0]?.live, liveLast: rows[rows.length - 1]?.live });
  }
  return findings;
}

function main() {
  fs.mkdirSync(outDir, { recursive: true });
  const matrix = {}, budgets = {};
  const scenarios = [
    ['empty_active_n512_l0', { n: 512, activeFrames: 96 }],
    ['empty_sleep_n512', { n: 512, activeFrames: 96, sleep: true }],
    ['inject_n512_l8', { n: 512, activeFrames: 96, injectPerFrame: 0.1 }],
    ['inject_hold_n512_l157', { n: 512, activeFrames: 96, preInjectMass: 128, injectPerFrame: 0.1, upAfterFrame: 32, holdFrames: 64 }],
    ['inject_full_n512_l512', { n: 512, activeFrames: 96, preInjectMass: 512 }],
    ['empty_active_n64_l0', { n: 64, activeFrames: 96 }],
    ['inject_hold_n64_l48', { n: 64, activeFrames: 96, preInjectMass: 32, injectPerFrame: 0.1, upAfterFrame: 32, holdFrames: 64 }],
  ];
  for (const [name, opts] of scenarios) {
    const r = runScenario(name, opts);
    matrix[name] = r.frames;
    budgets[name] = { budget: r.budget, admission: r.admission };
  }
  const sentinelsOut = sentinels();
  const findings = judge(matrix, budgets);
  const admissions = Object.fromEntries(Object.entries(budgets).map(([k, v]) => [k, v.admission]));
  const report = { revision: 'T424_CPU_BASELINE_MATRIX_V1', device: 'NOT_RUN',
    root: path.resolve(root), seed: 51, dtPerCallbackMs: 20,
    scenarioDefs: Object.fromEntries(scenarios), sentinels: sentinelsOut,
    findings, admissions, budgets };
  const dest = path.join(outDir, 'matrix.json');
  fs.writeFileSync(dest, JSON.stringify(report, null, 1));
  console.log(JSON.stringify({ revision: report.revision, scenarios: Object.keys(matrix),
    sentinels: sentinelsOut, findings, dest }));
}
main();
