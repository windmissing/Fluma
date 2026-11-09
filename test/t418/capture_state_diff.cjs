// T418 · T414 实际 controller（T413Capture）状态差分场景执行器。
// 对隔离生成工程的 render/T413Capture.ets 逐场景执行，输出 JSON 供 diff_units.py 与
// T415 oracle 对照。不引用工作区产品、不使用真机。
// 用法: node test/t418/capture_state_diff.cjs <隔离工程根>
const fs = require('node:fs'), path = require('node:path'), Module = require('node:module');
const ts = require('C:/Program Files/Huawei/DevEco Studio/tools/hvigor/hvigor/node_modules/typescript');

const root = path.resolve(process.argv[2], 'entry/src/main/ets');
const cache = new Map();
function load(name) {
  const file = path.resolve(root, name + '.ets');
  if (cache.has(file)) return cache.get(file).exports;
  const js = ts.transpileModule(fs.readFileSync(file, 'utf8'), {
    compilerOptions: { module: ts.ModuleKind.CommonJS, target: ts.ScriptTarget.ES2020, esModuleInterop: true }
  }).outputText;
  const m = new Module(file); m.paths = [path.dirname(file)];
  const stub = new Module(file + '-stub');
  stub.exports = { p013NowMs: () => nowMs };
  m.require = id => id === './P013RenderController' ? stub.exports : require(id);
  m._compile(js, file);
  cache.set(file, m.exports);
  return m.exports;
}
let nowMs = 1000;

const { T413Capture, CaptureGeometry } = load('render/T413Capture');
const SHA = 'a'.repeat(64), SHA2 = 'b'.repeat(64);
const results = [];

function freshCapture(maxNowAdvance = 0) {
  nowMs = 1000;
  return new T413Capture();
}
function geometryOk(c) {
  const g = new CaptureGeometry();
  g.globalX = 10; g.globalY = 400; g.widthVp = 1000; g.heightVp = 1000;
  g.vp2px = 1; g.geometry = 1; g.visible = true; g.configuration = 'FIXED_PANEL_XCOMPONENT_V2';
  c.setGeometry(g);
}
const ROI = [0, 0, 16, 16, 100, 100, 16, 16, 200, 200, 16, 16];

// 场景 1: 全生命周期正例 reserve→open→ack(OPEN 内)→seal→(savedTerminal)→release
{
  const c = freshCapture(); geometryOk(c);
  const canReserve = c.canReserve(new Int32Array(ROI), 300000, 1000, 1000);
  const r1 = c.reserve(new Int32Array(ROI), '', 300000, 1, 1);
  const attemptId = c.status().attemptId;       // 实际生成的 attemptId（session-a1）
  const open0 = c.status();           // RESERVED
  c.open(1, 1, 'tk1', 1, 'run1', 'owner1', nowMs);
  const open1 = c.status();           // HOLD_OPEN
  const a1 = c.ack(attemptId, 1, SHA);
  const a2 = c.ack('old', 1, SHA);    // 旧 attempt（在正确 ack 之后）
  const s1 = c.seal(c.status().holdId);
  nowMs += 50;
  const s2 = c.seal(JSON.parse(c.terminal()).hold.holdId);   // 重复同 id
  const term = c.terminal();
  const termHold = JSON.parse(term).hold;
  c.savedTerminal(attemptId, termHold.holdId, SHA2);
  const termBeforeRelease = c.terminal();
  const r = c.release(attemptId, termHold.holdId);
  const termRetry = c.terminal();   // release 后状态 RELEASED: terminal() 返回 ''（产品语义）
  results.push({
    scenario: 'S1_FULL_LIFECYCLE',
    canReserve, attemptId, reserveState: r1.state, beforeState: open0.state, openState: open1.state,
    holdId: open1.holdId, requestId: open1.requestId,
    ackCorrect: a1.state, ackCorrectArchive: c.status().archiveState,
    ackOldAttempt: a2.state, ackOldAttemptArchive: c.status().archiveState,
    sealState: s1.state, sealRetryState: s2.state,
    terminalImmutableAfterRetry: termBeforeRelease === term,
    terminalAfterReleaseEmpty: termRetry === '',
    savedTerminal: 'done', releaseState: r.state,
    startMonoMs: termHold.startMonoMs, endMonoMs: termHold.endMonoMs,
  });
}
// 场景 2: OPEN 中新预约 → controller 层 canReserve 先拒 HOLD_ACTIVE 且零副作用。
// 注: controller.reserveCapture 以 canReserve 开头（HOLD_ACTIVE 先拒），T413Capture.reserve
// 本身不重复查 state——直调 reserve 是越权用法，controller 合同保证不会发生。
{
  const c = freshCapture(); geometryOk(c);
  c.reserve(new Int32Array(ROI), '', 300000, 1, 1);
  c.open(1, 1, 'tk1', 1, 'run1', 'owner1', nowMs);
  const before = c.status();               // HoldStatus 对象
  const can2 = c.canReserve(new Int32Array(ROI), 300000, 1000, 1000);
  // 模拟 controller 语义: canReserve 非空即拒绝，不调 reserve
  const reservedViaController = can2 === '' ;
  if (reservedViaController) c.reserve(new Int32Array(ROI), '', 300000, 2, 2);
  const after = c.status();
  results.push({
    scenario: 'S2_RESERVE_DURING_OPEN', canReserve2: can2,
    controllerPath: reservedViaController ? 'RESERVED' : 'REJECTED_HOLD_ACTIVE',
    statusUnchanged: JSON.stringify(after) === JSON.stringify(before),
    requestPhaseUnchanged: after.requestId === before.requestId &&
      after.phaseSequence === before.phaseSequence && after.actionSerial === before.actionSerial,
  });
}
// 场景 3: 旧 ack 三类（attempt 不匹配 / request 不匹配 / 空 SHA），在 OPEN 且未正确 ack 时
{
  const c = freshCapture(); geometryOk(c);
  c.reserve(new Int32Array(ROI), '', 300000, 7, 7);
  const attemptId = c.status().attemptId;
  c.open(7, 7, 'tk1', 1, 'run1', 'owner1', nowMs);
  const a1 = c.ack('wrong', 7, SHA);
  const a2 = c.ack(attemptId, 6, SHA);
  const a3 = c.ack(attemptId, 7, '');
  results.push({
    scenario: 'S3_OLD_ACK', attemptMismatch: a1.state + ':' + a1.reason,
    requestMismatch: a2.state + ':' + a2.reason, emptySha: a3.state + ':' + a3.reason,
    archiveStillPending: c.status().archiveState === 'NATIVE_ARCHIVE_PENDING',
  });
}
// 场景 4: 同 id seal 幂等 / 错 id seal / 未 OPEN seal
{
  const c = freshCapture(); geometryOk(c);
  c.reserve(new Int32Array(ROI), '', 300000, 1, 1);
  c.open(1, 1, 'tk1', 1, 'run1', 'owner1', nowMs);
  const s1 = c.seal(c.status().holdId);
  const term1 = c.terminal();
  nowMs += 10;
  const s2 = c.seal(JSON.parse(term1).hold.holdId);
  const s3 = c.seal('other-hold-id');
  const termAfter = c.terminal();
  results.push({
    scenario: 'S4_SEAL_IDEMPOTENCE', sealState: s1.state, sealRetry: s2.state,
    sealWrongId: s3.state + ':' + s3.reason,
    terminalImmutable: c.terminal() === term1,
    endMonoMsUnchanged: JSON.parse(termAfter).hold.endMonoMs === JSON.parse(term1).hold.endMonoMs,
  });
}
// 场景 5: 真实 elapsed 到期（status 驱动，无定时器）→ EXPIRED，seal 拒绝，终态冻结
{
  const c = freshCapture(); geometryOk(c);
  c.reserve(new Int32Array(ROI), '', 100, 1, 1);
  c.open(1, 1, 'tk1', 1, 'run1', 'owner1', nowMs);
  nowMs += 200;                        // 超 maxHoldMs=100
  const st = c.status();               // status() 触发到期
  const s = c.seal(st.holdId);
  const term1 = c.terminal();
  nowMs += 100;
  const term2 = c.terminal();
  results.push({
    scenario: 'S5_EXPIRY', stateAfterExpiry: st.state, reason: st.reason,
    sealState: s.state + ':' + s.reason, terminalFrozen: term1 === term2,
    terminalValid: JSON.parse(term1).valid,
  });
}
// 场景 6: 重复 cancel 保留首因；错 attempt cancel 零副作用
{
  const c = freshCapture(); geometryOk(c);
  c.reserve(new Int32Array(ROI), '', 300000, 1, 1);
  const attemptId = c.status().attemptId;
  c.open(1, 1, 'tk1', 1, 'run1', 'owner1', nowMs);
  const c1 = c.cancel(attemptId, 'KEYBOARD');
  const statusAfterCancel1 = c.status();   // 首因记在 status().reason
  const first = c.terminal() || '';
  const c2 = c.cancel(attemptId, 'MOUSE');
  const c3 = c.cancel('wrong', 'MOUSE');
  const c4 = c.cancel(attemptId, 'MOUSE'); // 已 INVALID: 重复取消回执同终态
  results.push({
    scenario: 'S6_CANCEL_REPEAT', cancel1: c1.state, statusReasonAfterCancel1: statusAfterCancel1.reason,
    cancel2: c2.state, cancelRepeatAfterInvalid: c4.state, cancelWrong: c3.state,
    firstCauseStable: (c.terminal() || '') === first,
    terminalFirstFailure: first ? JSON.parse(first).firstFailure : null,
    statusReasonStable: c.status().reason === statusAfterCancel1.reason,
  });
}
// 场景 7: 未归档 SEALED 不能 release；归档后 release 成功
{
  const c = freshCapture(); geometryOk(c);
  c.reserve(new Int32Array(ROI), '', 300000, 1, 1);
  const attemptId = c.status().attemptId;
  c.open(1, 1, 'tk1', 1, 'run1', 'owner1', nowMs);
  const holdId = c.status().holdId;
  c.seal(holdId);
  const rel1 = c.release(attemptId, holdId);
  c.ack(attemptId, 1, SHA);
  const rel2 = c.release(attemptId, holdId);   // 还缺 savedTerminal
  c.savedTerminal(attemptId, holdId, SHA2);
  const rel3 = c.release(attemptId, holdId);
  results.push({
    scenario: 'S7_RELEASE_GATES', releaseNoArchive: rel1.state + ':' + rel1.reason,
    releaseNoTerminal: rel2.state + ':' + rel2.reason, releaseOk: rel3.state,
  });
}
// 场景 8: geometry 变化 → INVALID + 首因；SEALED 后失效保留首 seal 不可作为成功释放
{
  const c = freshCapture(); geometryOk(c);
  c.reserve(new Int32Array(ROI), '', 300000, 1, 1);
  c.open(1, 1, 'tk1', 1, 'run1', 'owner1', nowMs);
  const g2 = new CaptureGeometry();
  g2.globalX = 11; g2.globalY = 400; g2.widthVp = 1000; g2.heightVp = 1000;
  g2.vp2px = 1; g2.geometry = 1; g2.visible = true; g2.configuration = 'FIXED_PANEL_XCOMPONENT_V2';
  c.setGeometry(g2);
  const st = c.status();
  const s = c.seal(st.holdId);
  results.push({
    scenario: 'S8_GEOMETRY_CHANGE', state: st.state, reason: st.reason,
    sealAfterInvalidate: s.state + ':' + s.reason,
  });
}
console.log(JSON.stringify({ revision: 'T418_CAPTURE_STATE_DIFF_V1', device: 'NOT_RUN', results }));
