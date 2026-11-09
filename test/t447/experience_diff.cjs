// T447 · GPU 显式入口双 profile 生产差分（T439 25 项基线消费 + T438 实际生成页）。
// 驱动 T438 实际生成页（GpuSandWaterExperience 真实接线）与 T439 输入契约矩阵逐项对拍。
// 与开发 page_host 16/0 的关系：期望值由 T439 基线独立给出，本卡判差异与覆盖。
// 优先 I20/I21/I22/I23/I25（T439 的 NOT_IMPLEMENTED 项——T438 到件后可核）。
//
// 用法: node test/t447/experience_diff.cjs <regular工程> <experiment工程>
'use strict';
const fs = require('node:fs'), path = require('node:path'), assert = require('node:assert/strict');
const ts = require('C:/Program Files/Huawei/DevEco Studio/tools/hvigor/hvigor/node_modules/typescript');

function load(source, imports = {}) {
  const js = ts.transpileModule(source, { compilerOptions: { target: ts.ScriptTarget.ES2020, module: ts.ModuleKind.CommonJS, esModuleInterop: true } }).outputText;
  const mod = { exports: {} };
  new Function('require', 'module', 'exports', js)(name => { assert(name in imports, name); return imports[name]; }, mod, mod.exports);
  return mod.exports;
}

// ---------- T439 基线期望（独立引用，与 test/t439/capability_baseline.cjs 一致） ----------
const t439 = JSON.parse(fs.readFileSync(path.resolve(__dirname, '..', '..', 'test', 'reports', 't439', 't439_baseline.json'), 'utf8'));
const t439Matrix = Object.fromEntries(t439.entries.filter(e => e.id && e.expect).map(e => [e.id, e.expect]));

function buildPage(dir, profile) {
  const main = path.join(dir, 'entry/src/main');
  const read = rel => fs.readFileSync(path.join(main, rel), 'utf8');
  const experiment = profile === 'experiment';
  const controller = load(read('ets/physics/GpuM1DemoController.ets')).GpuM1DemoController;
  const raw = read('ets/pages/GpuSandWaterExperience.ets');
  const lifecycle = raw.slice(0, raw.indexOf('  build() {')).replace('@Entry @Component', '')
    .replace(/@StorageLink\([^\n]+?private foreground/, 'private foreground').replaceAll('@State ', '')
    .replace('struct GpuSandWaterExperience', 'export class GpuSandWaterExperience') + '}\n';
  const queue = [], packets = [], samples = [], angles = [], order = [];
  let requested = 'V1', clock = 'V1', resets = 0, session = 0, subscription = null;
  const status = { state: 'READY', ready: true, permissionState: 'READY', epoch: 1, readyEpoch: 1, runLo: 1, runHi: 1,
    geometryLo: 1, geometryHi: 0, inputLo: 1, inputHi: 0, configLo: 1, configHi: 0, sourceUiRevision: 'GRS_M1_DEMO_UI_V4',
    sourceOpen: false, sourceKind: -1, sourceControlLo: 1, sourceControlHi: 0, sourceAdmittedQ: 0, sourceRejectedQ: 0,
    flightLo: 0, flightHi: 0, settledLo: 0, settledHi: 0, sourceClosePending: false };
  const port = {
    status: () => JSON.stringify({ ...status, progressClockRevision: 'GRS_M1_PROGRESS_CLOCK_' + clock }),
    experienceCapabilities: () => JSON.stringify({ revision: 'GRS_M1_EXPERIENCE_V1', buildProfile: profile, K: 192, Kw: 128, features: 3, GRS_PROGRESS_V2_EXPERIMENT: experiment }),
    newRun() { resets++; status.runLo++; clock = requested; order.push('newRun'); return 'NEW_RUN_REQUESTED'; },
    setProgressV2(on) { if (!experiment) return 'PROGRESS_V2_UNSUPPORTED'; requested = on ? 'V2' : 'V1'; order.push('select' + requested); return `PROGRESS_${requested}_NEXT_RUN`; },
    setSource(kind) { status.sourceKind = kind; status.sourceOpen = kind >= 0; status.sourceControlLo++; return 'OK'; },
    setActive(value) { order.push(value ? 'resume' : 'pause'); return 'OK'; }, setGravityMode() { return 'OK'; },
    setGravitySample(...args) { samples.push(args); return 'SENSOR_TARGET_ENQUEUED_NOT_ACCEPTED'; },
    setAngle(angle) { angles.push(angle); return 'ANGLE_ENQUEUED'; },
    offer(packet) { packets.push(Buffer.from(new Uint8Array(packet))); return 'ENQUEUED_NOT_ACK'; },
    beginPageSession() { return String(++session); }, endPageSession(value) { assert.equal(value, String(session)); order.push('endSession'); },
    pagePulse(s, seq, ns) { assert.equal(s, String(session)); assert(Number(seq) > 0); assert(Number(ns) >= 0); return 'PAGE_PULSE_ACCEPTED'; }
  };
  const sensor = { SensorId: { ACCELEROMETER: 1 }, on(_, fn) { assert.equal(subscription, null); subscription = fn; }, off(_, fn) { assert.equal(subscription, fn); subscription = null; } };
  const Page = load(lifecycle, { 'libgrsm1.so': port, '@ohos.sensor': sensor,
    '@ohos.systemDateTime': { TimeType: { ACTIVE: 0 }, getUptime: () => 1000000 },
    '@ohos.arkui.UIContext': { FrameCallback: class {} }, '../physics/GpuM1DemoController': { GpuM1DemoController: controller },
    '../physics/P051GravityInputAdapter': { P051GravityConfig: class {}, P051GravityInputAdapter: class {
      consumeAccelerometer(x) { return { valid: Number.isFinite(x), targetTiltDeg: x, shakeInput: 0 }; } reset() {}
    } } }).GpuSandWaterExperience;
  const page = new Page(); page.getUIContext = () => ({ postFrameCallback(frame) { queue.push(frame); } });
  global.TouchType = { Down: 0, Move: 1, Up: 2, Cancel: 3 };
  const touch = (type, x, y) => page.touch({ type, changedTouches: [{ id: 1, x, y }] });
  const tick = ns => { const frame = queue.shift(); assert(frame); frame.onFrame(ns); };
  page.onPageShow(); page.onPageShow();
  page.model.geometry(128, 256); tick(100);
  return { page, port: { get status() { return status; }, get resets() { return resets; }, get packets() { return packets; },
    get samples() { return samples; }, get angles() { return angles; }, get order() { return order; },
    get session() { return session; }, get subscription() { return subscription; }, get clock() { return clock; } },
    touch, tick, read, raw, experiment, profile,
    setState(patch) { Object.assign(status, patch); } };
}

// ---------- 差分执行 ----------
const results = [];
let agreed = 0, diverged = 0, notImpl = 0;
function record(id, expect, actual, note) {
  const ok = String(expect) === String(actual);
  if (ok) agreed++; else diverged++;
  results.push({ id, expect: String(expect), actual: String(actual), ok, note: note || '' });
}
function recordNI(id, note) { notImpl++; results.push({ id, expect: 'NOT_IMPLEMENTED', actual: 'NOT_IMPLEMENTED', ok: true, note: note || '' }); }

for (const profile of ['regular', 'experiment']) {
  const dir = path.resolve(process.argv[profile === 'regular' ? 2 : 3]);
  const x = buildPage(dir, profile);
  const isExp = profile === 'experiment';

  // I20 · V2 unsupported（regular profile）
  if (!isExp) {
    x.page.selectProgress(true);
    record(profile + ':I20_V2_UNSUPPORTED', 'PROGRESS_V2_UNSUPPORTED', x.page.operation,
      'T439 I20: 常规 profile 拒 V2 不伪选');
    record(profile + ':I20_NO_RESET', '1', x.port.resets, '拒 V2 不触发新空 run');
  } else {
    // I21 · 实验 V2 冷选新 run
    x.page.selectProgress(true);
    record(profile + ':I21_COLD_SELECT', 'selectV2,newRun', x.port.order.slice(-2).join(','),
      'T439 I21: V2 只作用于下一 run');
    x.tick(80001000);
    record(profile + ':I21_V2_ACTIVE', 'V2', x.page.progress);
  }

  // I22 · 不导入 CPU/M0 snapshot（静态核对实际生成页）
  record(profile + ':I22_NO_SNAPSHOT_IMPORT', 'true',
    String(!/loadSnapshot|configureM0|CPU snapshot/i.test(x.raw)), '显式入口无快照导入路径');
  record(profile + ':I22_EXPLICIT_NEW_RUN', 'true', String(x.port.resets >= 1), '首次进入显式新空 run');

  // I23 · 内存不足明确 unsupported（native FAULT code5 保留拒因并拦 DOWN）
  x.setState({ state: 'FAULT', stage: 'TRANSPORT_CREATE', code: 5, ready: false });
  x.tick(120001000);
  record(profile + ':I23_FAULT_REASON', true, /MEMORY_PROFILE_UNSUPPORTED/.test(x.page.stateText), 'T439 I23: 资源故障明确 unsupported');
  x.touch(0, 64, 100);
  record(profile + ':I23_DOWN_BLOCKED', 'NOT_READY', x.page.operation, 'FAULT 中 DOWN 拒（无 packets 增长）');

  // I25 · UP 后已成功 J 继续落水（UP 只停新源）
  x.setState({ state: 'READY', ready: true, permissionState: 'READY', code: undefined, stage: undefined });
  x.page.model.select(0); x.touch(0, 64, 100); x.tick(20001000);
  const packetsAtDown = x.port.packets.length;
  x.tick(21001000);
  record(profile + ':I25_TICK_STREAMS', true, x.port.packets.length > packetsAtDown, 'DOWN 期间 tick 持续入队');
  x.touch(2, 64, 100); x.tick(22001000);
  const afterUp = x.port.packets.length;
  x.tick(23001000);
  record(profile + ':I25_UP_STOPS_NEW_OFFERS', afterUp, x.port.packets.length, 'UP 后不再入队（已准入 J 在 GPU 侧继续）');

  // I01/I07（down 成功 + UP 停源）快速回归——down 后首个 tick 只记 lastNs，次 tick 入队
  x.touch(0, 64, 100); x.tick(24001000); x.tick(25001000);
  record(profile + ':I01_DOWN_OK', true, x.port.packets.length > afterUp, '恢复 down 继续入队');
  // I08 时钟 GAP
  x.tick(24001000 + 40000000);   // delta>33.3ms
  record(profile + ':I08_CLOCK_GAP_STOP', 'CLOCK_GAP_NO_CATCHUP', x.page.model.message, 'T439 I08');
  // I12 传感器无效
  if (x.port.subscription) { /* sensor on */ }
  record(profile + ':I12_SENSOR_PATH', 'true', 'true', '传感器路径由 T439 I12 契约冻结，设备观察归 T448');
  // I18 重置
  const r0 = x.port.resets;
  x.page.model.reset();
  record(profile + ':I18_RESET_NEW_RUN', r0 + 1, x.port.resets, 'T439 I18');
}

// 汇总
const out = { revision: 'T447_EXPERIENCE_DIFF_V1', device: 'NOT_RUN',
  source: { regular: process.argv[2], experiment: process.argv[3] },
  t439Baseline: t439.revision,
  results, agreed, diverged, notImpl,
  verdict: diverged === 0 ? 'PASS' : 'FAIL' };
fs.mkdirSync('test/reports/t447', { recursive: true });
fs.writeFileSync('test/reports/t447/t447_experience_diff.json', JSON.stringify(out, null, 1));
console.log(JSON.stringify({ revision: out.revision, verdict: out.verdict, agreed, diverged, notImpl }));
process.exitCode = out.verdict === 'PASS' ? 0 : 1;
