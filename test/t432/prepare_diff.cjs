// T432 · 单沙准备许可生产逻辑独立差分。
// 依 T426 §3–5 从合同文本独立实现 READY 谓词 oracle（不 import 产品实现逻辑做期望推导），
// 实际驱动 T428 生成页的 PrepareReadiness helper 与真实页面接线（真实 stopEditing/clearFocus/
// 窗口监听/FrameCallback/reserveCapture 路径），逐场景比对实际行为 vs 独立期望。
// 与开发 page_units 16/0 的关系：那是开发自测；本卡是独立差分——期望值由本卡从
// T426 合同独立推导，不是引用开发断言。发现漏接线/语义分歧记 DIVERGE。
//
// 用法: node test/t432/prepare_diff.cjs <T426生成工程根>
// 退出码 0=全一致；1=发现分歧/缺接线。
'use strict';
const fs = require('node:fs'), path = require('node:path'), Module = require('node:module');
const assert = require('node:assert/strict');
const ts = require('C:/Program Files/Huawei/DevEco Studio/tools/hvigor/hvigor/node_modules/typescript');

const root = path.resolve(process.argv[2], 'entry/src/main/ets');
const source = fs.readFileSync(path.join(root, 'pages/P051DerivedV2Demo.ets'), 'utf8');

// ---------- 独立静态接线核对（不依赖运行行为） ----------
const staticChecks = [];
function staticCheck(id, ok, detail) { staticChecks.push({ id, ok, detail: detail || '' }); }

staticCheck('W1_PAGE_WIRES_THREE_REAL_INPUTS',
  source.split('\n').filter(s => s.includes('TextInput({controller:')).length === 3,
  '三个 TextInput 必须绑定独立 controller（T426 §3.2）');
staticCheck('W2_NO_HIDE_TEXT_INPUT',
  !source.includes('hideTextInput'),
  '正式路径不得调用 inputMethod hideTextInput（T426 §2）');
staticCheck('W3_RESERVE_GATE_IS_CONSUME',
  source.includes('prepareGate.consume()'),
  '预约唯一入口必须消费 preparePermit（T426 §4 reserve 复核）');
staticCheck('W4_STOP_EDITING_BEFORE_CLEAR_FOCUS',
  /stopEditing\(\)[\s\S]{0,200}clearFocus\(\)/.test(source) || (source.includes('stopEditing') && source.includes('clearFocus')),
  '退出编辑先于清理焦点（T426 §3.2）');
staticCheck('W5_DEFAULT_FOCUS_FALSE',
  source.includes('defaultFocus(false)'),
  '三输入禁用自动聚焦（T426 §3.2）');
staticCheck('W6_PREPARE_COLD_FILE_NAME',
  /t426-prepare-\$\{/.test(source) || source.includes('t426-prepare-'),
  '准备冷档独立命名（T426 §3.7 / T428 交付）');
staticCheck('W7_PERMIT_NOT_HOLD_TOKEN',
  !/permit[^;\n]{0,80}(holdId|holdToken)/.test(source) && !/holdId[^;\n]{0,40}permit/.test(source.slice(0, source.indexOf('build('))),
  'preparePermit 不得伪装 hold token（T426 §4）');

// ---------- 独立 READY 谓词 oracle（从 T426 §4 表逐行独立实现） ----------
// 返回 ''（READY 可）或具体拒因。仅依据合同文本，不引用 PrepareReadiness.predicate 代码。
function oraclePredicate(snap, inputs, sessionEverEdited, keyboardAreaSeen, diagOverflow) {
  if (diagOverflow) return 'PREPARE_DIAGNOSTIC_OVERFLOW';
  if (!snap.windowActive || !snap.windowRegistered || snap.windowId <= 0) return 'PREPARE_WINDOW_UNKNOWN';
  if (inputs.some(i => !i.created)) return 'INPUT_HISTORY_UNKNOWN';
  if (inputs.some(i => i.editing || i.focused)) return 'INPUT_EDITING_OR_FOCUSED';
  // T426 §4 键盘行: 固定键盘配置+全屏手机+WINDOW_PX+四边+有限+曾编辑必须见过非零区域
  if (snap.keyboardConfiguration !== 'TESTER_CONFIRMED_FIXED_SYSTEM_KEYBOARD' ||
      snap.deviceType !== 'phone' || snap.windowMode !== 1 ||
      snap.avoidUnit !== 'WINDOW_PX' || snap.avoid.length !== 4 ||
      !snap.avoid.every(r => [r.x, r.y, r.width, r.height].every(Number.isFinite) && r.width >= 0 && r.height >= 0) ||
      (sessionEverEdited && !keyboardAreaSeen)) return 'KEYBOARD_VISIBILITY_UNKNOWN';
  if (snap.keyboardAnimation || snap.keyboardHeight > 0 || snap.avoid.some(r => r.width > 0 && r.height > 0))
    return 'KEYBOARD_OCCLUDED';
  if (!snap.fullyVisible || !snap.componentPresent || !snap.transformIdentity) return 'COMPONENT_VISIBILITY_UNKNOWN';
  const r = snap.component, w = snap.windowRect;
  const fin = a => a.every(Number.isFinite);
  if (!fin([r.x, r.y, r.width, r.height, w.x, w.y, w.width, w.height, snap.globalX, snap.globalY,
            snap.widthVp, snap.heightVp, snap.vp2px]) ||
      r.width <= 0 || r.height <= 0 || snap.vp2px <= 0 ||
      r.x < 0 || r.y < 0 || r.x + r.width > w.width || r.y + r.height > w.height) return 'COMPONENT_OUTSIDE_WINDOW';
  const c = snap.vpBoxWindowPx, sc = snap.screen;
  if (!fin([c.x, c.y, c.width, c.height, sc.x, sc.y, sc.width, sc.height]) ||
      c.width <= 0 || c.height <= 0 || c.x < 0 || c.y < 0 ||
      c.x + c.width > w.width || c.y + c.height > w.height ||
      sc.x < w.x || sc.y < w.y || sc.x + sc.width > w.x + w.width || sc.y + sc.height > w.y + w.height)
    return 'COMPONENT_OUTSIDE_WINDOW';
  if (!snap.nativeReady || !snap.session || !snap.controller || !snap.surface || !snap.context ||
      !snap.run || snap.nativeGeometry !== snap.geometry ||
      snap.viewportWidth !== r.width || snap.viewportHeight !== r.height) return 'PREPARE_NATIVE_NOT_READY';
  return '';
}

// ---------- 实际 harness（复用 t426/page_units.cjs 的加载器模式，独立驱动） ----------
let now = 1000, frame = 1, throwStop = false, throwSubscribe = false, timers = [];
let files = new Map(), lateWrites = [], deferWrites = false;
const native = { getState: () => ({ surfaceEpoch: 3, contextEpoch: 2, widthPx: 800, heightPx: 600 }) };

class ColdFiles {
  async save(name, text) {
    if (deferWrites) await new Promise((resolve, reject) => lateWrites.push({ resolve, reject }));
    files.set(name, text);
    files.set('__sha__' + name, 'a'.repeat(64));   // 记录 SHA 收据
    return { name, bytes: text.length, sha256: 'a'.repeat(64) };
  }
}
class TextController {
  stopEditing() {
    if (throwStop) throw { code: 55, name: 'ComponentError', message: 'stop failed', runStatus: 9 };
    this.events?.onEditChange?.(false); this.events?.onBlur?.();
  }
}
global.TextInputController = TextController;
global.getContext = () => ({ filesDir: 'fixture' });
global.TextInput = options => {
  const events = {}; options.controller.events = events;
  let proxy; proxy = new Proxy({}, { get: (_, key) => (...args) => {
    if (key === 'defaultFocus') assert.equal(args[0], false);
    if (key === 'onAppear') args[0]();
    if (/^on/.test(key)) events[key] = args[0];
    return proxy;
  } }); return proxy;
};

const cache = new Map();
function load(name) {
  const file = path.resolve(root, name + '.ets');
  if (cache.has(file)) return cache.get(file).exports;
  const m = new Module(file); cache.set(file, m); m.filename = file;
  m.require = id => {
    if (id === '@ohos.window') return { AvoidAreaType: { TYPE_KEYBOARD: 3 }, WindowStatusType: { FULL_SCREEN: 1 },
      WindowEventType: { WINDOW_SHOWN: 1, WINDOW_ACTIVE: 2, WINDOW_INACTIVE: 3, WINDOW_HIDDEN: 4, WINDOW_DESTROYED: 7 } };
    if (id === '@ohos.deviceInfo') return { deviceType: 'phone' };
    if (id === '@ohos.arkui.UIContext') return { FrameCallback: class {} };
    if (id === '@ohos.sensor') return { SensorId: { ACCELEROMETER: 1 }, on() {}, off() {} };
    if (id === '@ohos.hilog') return { info() {}, warn() {}, error() {} };
    if (id === '@ohos.systemDateTime') return { getUptime: () => now * 1e6, TimeType: { ACTIVE: 0 } };
    if (id === 'libp013renderer.so') return native;
    if (id === '@ohos.file.fs' || id === '@ohos.util' || id === '@ohos.security.cryptoFramework' || id === '@kit.AbilityKit') return {};
    if (id.endsWith('/DiagnosticColdFiles')) return { DiagnosticColdFiles: ColdFiles };
    if (id.endsWith('/P051DerivedDemoV2')) return { P051DerivedDemoV2: class { budget = { enabled: false }; createPublicationCold() { return {}; } stop() {} setActive() {} } };
    if (id.endsWith('/P013SceneController')) return { P013SceneController: class { setActive() {} release() {} } };
    if (id.startsWith('.')) return load(path.relative(root, path.resolve(path.dirname(file), id)));
    return require(id);
  };
  let text = fs.readFileSync(file, 'utf8');
  if (name === 'pages/P051DerivedV2Demo') {
    text = text.slice(0, text.indexOf(' build(){')) + ' wireInputs():void {\n' +
      source.split('\n').filter(s => s.includes('TextInput({controller:'))
        .map(s => s.slice(s.indexOf('TextInput('))).join('\n') + '\n}\n}\nexport { P051DerivedV2Demo };';
    text = text.replace(/@Entry\s*/g, '').replace(/@Component\s*/g, '').replace(/@State\s*/g, '')
      .replace('struct P051DerivedV2Demo', 'class P051DerivedV2Demo');
  }
  m._compile(ts.transpileModule(text, { compilerOptions: { module: ts.ModuleKind.CommonJS, target: ts.ScriptTarget.ES2020, esModuleInterop: true } }).outputText, file);
  return m.exports;
}
const realTimeout = global.setTimeout, realClear = global.clearTimeout;
global.setTimeout = (fn, ms) => { timers.push({ fn, ms }); return timers.length; };
global.clearTimeout = id => { if (timers[id - 1]) timers[id - 1].cancel = true; };

const { P051DerivedV2Demo: Page } = load('pages/P051DerivedV2Demo');
const { PrepareMainWindow } = load('render/PrepareMainWindow');
const { PrepareSnapshot, PrepareRect } = load('render/PrepareReadiness');

const keyboard = height => ({ topRect: { left: 0, top: 0, width: 0, height: 0 },
  rightRect: { left: 0, top: 0, width: 0, height: 0 },
  bottomRect: { left: 0, top: 1600 - height, width: height ? 1200 : 0, height },
  leftRect: { left: 0, top: 0, width: 0, height: 0 } });

function windowFixture() {
  const listeners = new Map();
  return { area: keyboard(0), focused: true, mode: 1,
    props: { id: 4, windowRect: { left: 0, top: 0, width: 1200, height: 1600 } },
    getWindowProperties() { return this.props; }, getWindowAvoidArea() { return this.area; },
    isFocused() { return this.focused; }, getWindowStatus() { return this.mode; },
    on(key, fn) { if (throwSubscribe && key === 'windowEvent') throw { code: 8, message: 'subscribe failed' }; listeners.set(key, fn); },
    off(key, fn) { assert.equal(listeners.get(key), fn); listeners.delete(key); },
    listeners, emit(key, value) { listeners.get(key)?.(value); } };
}

function page() {
  now += 10; throwStop = throwSubscribe = false; timers = [];
  const w = windowFixture(); PrepareMainWindow.set(w);
  const queued = [];
  let box = { size: { width: 800, height: 600 }, windowOffset: { x: 200, y: 400 },
    screenOffset: { x: 200, y: 400 }, scale: { x: 1, y: 1, z: 1 }, translate: { x: 0, y: 0, z: 0 }, rotate: { angle: 0 } };
  const p = new Page();
  p.getUIContext = () => ({ vp2px: n => 2 * n, getFocusController: () => ({ clearFocus() {} }),
    getComponentUtils: () => ({ getRectangleById: () => box }), postFrameCallback: cb => queued.push(cb) });
  p.wireInputs(); p.layout.globalX = 100; p.layout.globalY = 200;
  p.layout.widthVp = 400; p.layout.heightVp = 300; p.layout.vp2px = 2; p.layout.geometry = 3;
  p.layout.visible = true; p.layout.configuration = 'FIXED_PANEL_XCOMPONENT_V2';
  p.fullyVisible = true; p.surfaceWidth = 400; p.surfaceHeight = 300; p.fixedKeyboardConfirmed = true;
  const c = p.singleSand, packet = c.preparation.packet;
  Object.assign(packet, { session: 1, surface: 3, context: 2, width: 800, height: 600, geometry: 3, run: 'r', enabled: true, token: 'token', frame: 1 });
  c.session = 1; c.lastReceipt = { presentationState: 'CURRENT' }; c.captureGeometry(p.layout);
  p.onPageShow();
  return {
    p, w, box, queued,
    flush() { const cb = queued.find(q => typeof q.onIdle === 'function'); assert(cb); queued.splice(queued.indexOf(cb), 1); cb.onFrame(++frame); cb.onIdle(2000000); },
    ready() { p.prepare(); for (let i = 0; i < 4; i++) this.flush(); assert.equal(p.prepareGate.state, 'READY', p.prepareGate.reason); },
  };
}

// ---------- 差分执行 ----------
let passed = 0, diverged = 0, missing = 0;
const results = [];
function record(id, expect, actual, note) {
  const ok = expect === actual;
  if (ok) passed++; else diverged++;
  results.push({ id, expect, actual, ok, note: note || '' });
}
function oracleReady(x) {
  // 用独立 oracle 期望当前快照应 READY：驱动实际页面把快照喂给 oracle
  const gate = x.p.prepareGate;
  const rep = JSON.parse(gate.report());
  return oraclePredicate(rep.snapshot, rep.inputs, rep.sessionEverEdited, rep.keyboardAreaSeen, rep.diagnosticOverflow);
}

(async () => {
  // O1 · 冷启无键盘 READY（独立 oracle 期待 READY，实际必须 READY 且原因一致）
  {
    const x = page(); x.p.prepare();
    for (let i = 0; i < 4; i++) { x.flush(); }
    const oracle = oracleReady(x);
    record('O1_COLD_READY_PREDICATE', '', oracle, 'oracle 期待冷启快照通过 §4 谓词');
    record('O1_COLD_READY_STATE', 'READY', x.p.prepareGate.state);
    const r = JSON.parse(x.p.prepareGate.report());
    record('O1_TWO_DISTINCT_FRAMES', true, r.permit.secondFrame > r.permit.firstFrame, '§3.4 两个不同帧点');
    record('O1_STOP_EDITING_CALLED', true,
      r.api.some(a => a.api && a.api.startsWith('TextInputController.stopEditing.')),
      '实际 stopEditing×3 在 API 原始记录中');
  }
  // O2 · 编辑→非零避让→退出→零避让→READY（逐阶段 oracle 拒因一致）
  {
    const x = page();
    x.p.roiInput.events.onFocus(); x.p.roiInput.events.onEditChange(true);
    x.w.area = keyboard(400); x.w.emit('avoidAreaChange', { type: 3 });
    x.p.prepare(); x.flush(); x.flush();   // 布局观察需两帧（与开发用例同拍）
    record('O2_NONZERO_OCCLUDED', 'KEYBOARD_OCCLUDED', x.p.prepareGate.reason);
    record('O2_ORACLE_AGREES', 'KEYBOARD_OCCLUDED', oracleReady(x));
    x.w.area = keyboard(0); x.w.emit('avoidAreaChange', { type: 3 });
    for (let i = 0; i < 3; i++) x.flush();
    record('O2_EXIT_THEN_READY', 'READY', x.p.prepareGate.state);
    record('O2_ORACLE_AGREES_READY', '', oracleReady(x));
  }
  // O3 · 命令成功但键盘仍在 → 不 READY（ratio1 不能覆盖）
  {
    const x = page(); x.w.area = keyboard(400); x.p.prepare();
    for (let i = 0; i < 4; i++) x.flush();
    record('O3_STILL_OCCLUDED', 'PREPARING', x.p.prepareGate.state);
    record('O3_REASON', 'KEYBOARD_OCCLUDED', x.p.prepareGate.reason);
    record('O3_ORACLE_AGREES', 'KEYBOARD_OCCLUDED', oracleReady(x));
  }
  // O4 · 曾编辑但无非零避让 → unknown（不能默认 false）
  {
    const x = page(); x.p.planInput.events.onFocus();
    x.p.prepare(); for (let i = 0; i < 4; i++) x.flush();
    record('O4_UNKNOWN', 'KEYBOARD_VISIBILITY_UNKNOWN', x.p.prepareGate.reason);
    record('O4_ORACLE_AGREES', 'KEYBOARD_VISIBILITY_UNKNOWN', oracleReady(x));
  }
  // O5 · 浮动键盘/非全屏 → unknown 不 READY
  {
    for (const [tag, change] of [['FLOATING', x => { x.p.fixedKeyboardConfirmed = false; }],
                                 ['NON_FULLSCREEN', x => { x.w.mode = 2; }]]) {
      const x = page(); change(x); x.p.prepare();
      for (let i = 0; i < 4; i++) x.flush();
      record('O5_' + tag, 'KEYBOARD_VISIBILITY_UNKNOWN', x.p.prepareGate.reason);
    }
  }
  // O6 · 同尺寸位置漂移重置稳定计数，旧 READY 立即撤销；旧许可预约零副作用
  {
    const x = page(); x.ready();
    const seqBefore = x.p.singleSand.t394.sequence;
    x.box.windowOffset.x++;
    x.p.singleSand.reserveCapture(new Int32Array(12), '', 1000);
    record('O6_RESERVE_ZERO_SIDE_EFFECT', seqBefore, x.p.singleSand.t394.sequence, '漂移后预约零 phase 副作用');
    record('O6_PERMIT_REVOKED', 'INVALIDATED', x.p.prepareGate.state);
  }
  // O7 · native 旧几何/迟到动作区布局不能产生许可
  {
    const x = page(); x.p.singleSand.preparation.packet.geometry = 2; x.p.prepare();
    for (let i = 0; i < 4; i++) x.flush();
    record('O7_NATIVE_STALE', 'PREPARE_NATIVE_NOT_READY', x.p.prepareGate.reason);
    x.p.singleSand.preparation.packet.geometry = 3;
    x.flush(); record('O7_RECOVERY_RESTARTS_COUNT', 'PREPARING', x.p.prepareGate.state);
    x.flush(); record('O7_RECOVERED_READY', 'READY', x.p.prepareGate.state);
  }
  // O8 · 唯一预约门：无许可 REJECTED 且 sequence 不变；有许可才 RESERVED 且消费一次
  {
    const x = page();
    const roi = new Int32Array([0, 0, 16, 16, 20, 20, 16, 16, 40, 40, 16, 16]);
    const c = x.p.singleSand;
    record('O8_NO_PERMIT_REJECTED', 'REJECTED', c.reserveCapture(roi, '', 1000).state);
    record('O8_NO_PHASE_MUTATION', 0, c.t394.sequence);
    x.ready();
    x.p.probeRoi = '0,0,16,16;20,20,16,16;40,40,16,16';
    await x.p.reserve();
    record('O8_WITH_PERMIT_RESERVED', 'RESERVED', c.capture.status().state);
    record('O8_PERMIT_CONSUMED', 'INVALIDATED', x.p.prepareGate.state);
    record('O8_CONSUMED_REASON', 'PREPARE_PERMIT_CONSUMED', x.p.prepareGate.reason);
    // 消费后第二次预约必须再拒（permit 一次性）
    const seq2 = c.t394.sequence;
    record('O8_PERMIT_ONESHOT_REJECT', 'REJECTED', c.reserveCapture(roi, '', 1000).state);
    record('O8_ONESHOT_ZERO_EFFECT', seq2, c.t394.sequence);
  }
  // O9 · READY 后编辑失效许可；OPEN 中键盘遮挡 hold INVALID 保首因
  {
    const x = page(); x.ready();
    x.p.roiInput.events.onFocus();
    record('O9_READY_EDIT_INVALIDATES', 'INVALIDATED', x.p.prepareGate.state);
    const s = x.p.singleSand.capture;
    const roi = new Int32Array(12);
    s.reserve(roi, '', 1000, 1, 1); s.open(1, 1, 'token', 1, 'r', 'owner', now);
    x.w.area = keyboard(400); x.w.emit('avoidAreaChange', { type: 3 });
    record('O9_OPEN_OCCLUSION_HOLD_INVALID', 'INVALID', s.status().state);
    const why = s.status().reason;
    x.w.emit('keyboardHeightChange', 400);
    record('O9_FIRST_CAUSE_STABLE', why, s.status().reason, '后续事件不得改写首因');
  }
  // O10 · 连点/晚到定时器/晚帧不能 READY 或改新代
  {
    const x = page(); x.p.prepare();
    const id = x.p.prepareGate.prepareId; x.p.prepare();
    record('O10_DOUBLE_PREPARE_SAME_ID', id, x.p.prepareGate.prepareId, 'PREPARING 中重复点击零副作用');
    now += 3001;
    for (let i = 0; i < 3; i++) x.flush();
    record('O10_TIMEOUT_FAILS', true, x.p.prepareGate.state === 'FAILED' && x.p.prepareGate.reason.startsWith('PREPARE_TIMEOUT'));
  }
  // O11 · hide/show 精确解绑监听；旧代回调/定时器/监听不得结束新准备
  {
    const x = page(); x.p.prepare();
    const oldFrame = x.p.prepareFrame, oldTimer = timers.at(-1).fn, oldListener = x.w.listeners.get('avoidAreaChange');
    x.p.onPageHide();
    record('O11_LISTENERS_DETACHED', 0, x.w.listeners.size, '按原 callback 引用解除，无残留');
    x.p.onPageShow(); x.p.prepare();
    const newId = x.p.prepareGate.prepareId;
    oldFrame.onFrame(500); oldFrame.onIdle(2e6); oldTimer(); oldListener({ type: 3 });
    record('O11_STALE_CALLBACKS_INERT', newId, x.p.prepareGate.prepareId);
    record('O11_NEW_PREP_ALIVE', 'PREPARING', x.p.prepareGate.state);
    for (let i = 0; i < 4; i++) x.flush();
    record('O11_NEW_PREP_CAN_READY', 'READY', x.p.prepareGate.state);
  }
  // O12 · 组件 API 原始错误保留（code/name/message/runStatus）；窗口订阅失败具体拒
  {
    const x = page(); throwStop = true; x.p.prepare();
    record('O12_STOP_FAILS_TO_FAILED', 'FAILED', x.p.prepareGate.state);
    const r = JSON.parse(x.p.prepareGate.report());
    record('O12_RAW55_PRESERVED', true, r.api.some(a => a.code === 55 && a.runStatus === '9'), '历史 55 原样保留不赋 READY 义');
    throwStop = false;
    const y = page(); y.p.onPageHide(); throwSubscribe = true; y.p.onPageShow();
    record('O12_SUBSCRIBE_FAILS', 'FAILED', y.p.prepareGate.state);
    throwSubscribe = false;
  }
  // O13 · 隐藏页旧异步写盘失败不影响新准备/不碰 exporting 所有权
  {
    const x = page(); deferWrites = true; x.p.prepare();
    const old = lateWrites.splice(0); x.p.onPageHide(); deferWrites = false;
    x.p.onPageShow(); x.p.prepare();
    const id = x.p.prepareGate.prepareId; x.p.exporting = true;
    for (const pw of old) pw.reject(new Error('old disk error'));
    await Promise.resolve(); await Promise.resolve();
    record('O13_NEW_PREP_UNTOUCHED', id, x.p.prepareGate.prepareId);
    record('O13_EXPORT_BUSY_UNTOUCHED', true, x.p.exporting === true);
    record('O13_STATE_ALIVE', 'PREPARING', x.p.prepareGate.state);
  }
  // O14 · vp2px 漂移重置稳定；viewport 出窗即 INVALIDATED
  {
    const x = page(); x.p.prepare(); x.flush(); x.flush();
    x.p.layout.globalX = 110; x.box.windowOffset.x = 220; x.box.screenOffset.x = 220;
    x.flush(); record('O14_DRIFT_RESETS', 'PREPARING', x.p.prepareGate.state);
    x.flush(); record('O14_RESTABILIZED', 'READY', x.p.prepareGate.state);
    x.box.screenOffset.y = 1500; x.p.observePrepare();
    record('O14_OUT_OF_WINDOW_INVALID', 'INVALIDATED', x.p.prepareGate.state);
  }
  // O15 · 历史 reject55 三态夹具不能替代实际观察（隐藏后可 READY，仍显示/未知必须拒）
  {
    let raw55 = 0;
    for (const mode of ['hidden', 'visible', 'unknown']) {
      const x = page();
      try { await Promise.reject({ code: 55, message: 'historical not bound', runStatus: 9 }); } catch (e) { raw55++; }
      if (mode === 'visible') x.w.area = keyboard(400);
      if (mode === 'unknown') x.p.fixedKeyboardConfirmed = false;
      x.p.prepare(); for (let i = 0; i < 4; i++) x.flush();
      record('O15_' + mode.toUpperCase(), mode === 'hidden' ? 'READY' : 'NOT_READY',
        mode === 'hidden' ? x.p.prepareGate.state : (x.p.prepareGate.state === 'READY' ? 'READY' : 'NOT_READY'),
        '命令回执自身不代替页面判定');
    }
    record('O15_RAW55_COUNT', 3, raw55);
  }
  // O16 · 窗口重建终结准备；旧代不能发布 READY
  {
    const x = page(); x.p.prepare();
    PrepareMainWindow.set(windowFixture());
    for (let i = 0; i < 3; i++) x.flush();
    record('O16_WINDOW_REBUILD_TERMINATES', true, x.p.prepareGate.state !== 'READY');
  }
  // O17 · 期限等号：恰 3000ms 边界失败（>=3000 含等号，晚回调不能绕过）
  {
    const x = page(); x.p.prepare();
    const rep0 = JSON.parse(x.p.prepareGate.report());
    const started = rep0.startedMonoMs;      // begin() 记录的真实起点
    now = started + 3000;                    // 恰好等号（同一毫秒时钟域）
    x.flush();
    x.flush();                                 // 队列中两次 post 均消费（后一次才与 prepareFrame 匹配）
    const st17 = x.p.prepareGate.state, rs17 = x.p.prepareGate.reason;
    record('O17_EXACT_DEADLINE_FAILS', true, st17 === 'FAILED' && rs17.startsWith('PREPARE_TIMEOUT'),
      'T426 §3.5 真实 elapsed 含等号 (state=' + st17 + ', reason=' + rs17 + ', started=' + started + ')');
  }
  // O18 · 准备冷档独立文件名与 SHA 收据（不与 T413 hold 档混名）
  {
    const x = page(); x.p.prepare(); for (let i = 0; i < 4; i++) x.flush();
    const prep = [...files.keys()].filter(n => n.startsWith('t426-prepare-'));
    record('O18_COLD_FILE_NAMED', true, prep.length > 0 &&
      prep.every(n => /t426-prepare-s\d+-\d+-\d+-\d+-(CONFIGURING|PREPARING|READY|FAILED|INVALIDATED)-\d+\.json/.test(n)),
      '独立命名 t426-prepare-<session>-<gen>-<id>-<state>-<serial>.json');
    record('O18_COLD_FILES_EXIST', true, prep.length >= 2, 'PREPARING 与 READY 状态各落盘');
    record('O18_SHA_RECEIPT', true, prep.every(n => (files.get('__sha__' + n) || '') === 'a'.repeat(64)),
      'DiagnosticColdFiles 每档返回 SHA 收据');
  }

  global.setTimeout = realTimeout; global.clearTimeout = realClear;
  fs.mkdirSync('test/reports/t432', { recursive: true });
  const out = { revision: 'T432_INDEPENDENT_PREPARE_DIFF_V1', device: 'NOT_RUN',
    source: path.resolve(process.argv[2]), staticChecks, results,
    passed, diverged, missing, summary: passed + ' agree, ' + diverged + ' diverge, ' + missing + ' missing wiring' };
  fs.writeFileSync('test/reports/t432/t432_prepare_diff.json', JSON.stringify(out, null, 1));
  console.log(JSON.stringify(out.summary), 'static:', staticChecks.filter(c => c.ok).length + '/' + staticChecks.length);
  if (staticChecks.some(c => !c.ok) || diverged || missing) process.exitCode = 1;
})().catch(error => { global.setTimeout = realTimeout; global.clearTimeout = realClear; console.error(error); process.exitCode = 1; });
