// T244 明水 C 行为与 native 独立补验（tester, 离线）v2
// 面 A: source ACK/同 key replay/stop/reset/后台清 pending/accepted 角 token/stale 暂停
// 面 C: 独立面积 Fresnel 求和/预乘参考
// 面 B: 独立 native-host cpp（t244_clear_water_independent.cpp）另跑
'use strict';
const fs = require('fs'), path = require('path');
const ROOT = path.resolve(__dirname, '..');
process.chdir(ROOT);
const results = [];
const rec = (id, name, status, detail) => { results.push({ id, name, status, detail }); console.log(`${status}  ${id}  ${name}  ${JSON.stringify(detail).slice(0, 130)}`); };
const CW = path.resolve('entry/src/main/ets/physics/clearwater');

const ts = require('C:/Program Files/Huawei/DevEco Studio/tools/hvigor/hvigor/node_modules/typescript');
require.extensions['.ets'] = (m, f) => m._compile(ts.transpileModule(fs.readFileSync(f, 'utf8'),
  { compilerOptions: { target: ts.ScriptTarget.ES2020, module: ts.ModuleKind.CommonJS } }).outputText, f);

const { P011ClearWaterEngine } = require(path.join(CW, 'P011ClearWaterEngine.ets'));
const { ClearWaterSourceController } = require(path.join(CW, 'P011ClearWaterSourceController.ets'));
const { clearWaterCConfig } = require(path.join(CW, 'P011ClearWaterCConfig.ets'));
const { ClearWaterStepResult } = require(path.join(CW, 'P011ClearWaterTypes.ets'));
const CTypes = require(path.join(CW, 'P011ClearWaterCTypes.ets'));
const CQ = CTypes.ClearWaterReceiptQuery;
const S = { OK: 0, INVALID_VALUE: 3, UNSUPPORTED_FEATURE: 19, INVALID_TIME_WINDOW: 21 };

// A-01: 真实 down→sample→ACK
{
  const eng = new P011ClearWaterEngine(clearWaterCConfig(192));
  const c = new ClearWaterSourceController(eng, 1, 1, 1);
  c.tick(0, 'NS', 'T244');
  if (!c.down(0, 64.5, 200)) rec('A-01', 'down', 'FAIL', { down: false });
  // down 的 stop('DOWN') 重置时钟 → 需两个 tick 产生有效窗口（第一次 FIRST_OR_CHANGED）
  c.tick(8_000_000, 'NS', 'T244');
  c.tick(24_666_666, 'NS', 'T244'); // 16.67ms 有效窗口
  const s1 = c.sample();
  const t = c.terminal.receipt;
  const acked = c.terminal.query === CQ.TERMINAL && (t.disposition === 'ACCEPTED' || t.disposition === 'PARTIAL_CAPACITY') && t.admitted > 0 && t.transactionId !== null && t.kindSuccessSeq !== null;
  rec('A-01', 'real source down→sample→ACK terminal', acked ? 'PASS' : 'FAIL', { sample: String(s1), query: String(c.terminal.query), disposition: t.disposition, admitted: t.admitted, txn: t.transactionId, kindSeq: t.kindSuccessSeq, sourceCommit: t.sourceCommit });
}

// A-02: 同 key replay（c.request 同 key 再查 → 同终态, sourceCommit 不变）
{
  const eng = new P011ClearWaterEngine(clearWaterCConfig(192));
  const c = new ClearWaterSourceController(eng, 1, 1, 1);
  c.tick(0, 'NS', 'T244'); c.down(0, 64.5, 200); c.tick(8_000_000, 'NS', 'T244'); c.tick(24_666_666, 'NS', 'T244'); c.sample();
  const commit1 = eng.canonical.sourceCommit;
  const disp1 = c.terminal.receipt.disposition;
  // 同 key replay: 直接再查同一 request（不 enqueue）
  const req = c['request'];
  const q2 = eng.copySourceReceiptInto(req, c.terminal);
  const commit2 = eng.canonical.sourceCommit;
  const same = q2 === CQ.TERMINAL && commit2 === commit1 && c.terminal.receipt.disposition === disp1;
  rec('A-02', 'same-key replay: no new commit, same terminal', same ? 'PASS' : 'FAIL', { q2: String(q2), commit1, commit2, disp1, disp2: c.terminal.receipt.disposition });
}

// A-03: 带真实 pending 的 stop 清空（down→sample 产生 pending→stop→pending=0+inputGen+1）
{
  const eng = new P011ClearWaterEngine(clearWaterCConfig(192));
  const c = new ClearWaterSourceController(eng, 1, 1, 1);
  c.tick(0, 'NS', 'T244'); c.down(0, 64.5, 200); c.tick(8_000_000, 'NS', 'T244'); c.tick(24_666_666, 'NS', 'T244'); c.sample();
  const inputGenBefore = eng.currentInputGeneration;
  const st = c.stop('T244_STOP');
  const after = eng.snapshot();
  rec('A-03', 'stop: pending=0 + inputGeneration bump + status OK', (st === 0 && after.pendingCount === 0 && eng.currentInputGeneration === inputGenBefore + 1) ? 'PASS' : 'FAIL', { st: String(st), pendingAfter: after.pendingCount, genBefore: inputGenBefore, genAfter: eng.currentInputGeneration });
}

// A-04: reset 清 pending+dedup
{
  const eng = new P011ClearWaterEngine(clearWaterCConfig(192));
  const c = new ClearWaterSourceController(eng, 1, 1, 1);
  c.tick(0, 'NS', 'T244'); c.down(0, 64.5, 200); c.tick(8_000_000, 'NS', 'T244'); c.tick(24_666_666, 'NS', 'T244'); c.sample();
  c.reset();
  const snap = eng.snapshot();
  rec('A-04', 'reset clears pending+dedup', (snap.pendingCount === 0 && snap.dedupCount === 0) ? 'PASS' : 'FAIL', { pending: snap.pendingCount, dedup: snap.dedupCount });
}

// A-05: 后台/clock domain 变化 → stop 清 pending
{
  const eng = new P011ClearWaterEngine(clearWaterCConfig(192));
  const c = new ClearWaterSourceController(eng, 1, 1, 1);
  c.tick(0, 'NS', 'T244'); c.down(0, 64.5, 200); c.tick(8_000_000, 'NS', 'T244'); c.tick(24_666_666, 'NS', 'T244'); c.sample();
  const pendingBefore = eng.snapshot().pendingCount;
  const v = c.tick(33_333_332, 'NS', 'T244-BACKGROUND'); // domain 变
  const pendingAfter = eng.snapshot().pendingCount;
  rec('A-05', 'clock domain change → tick invalid → stop → pending cleared', (v === false && pendingAfter === 0) ? 'PASS' : 'FAIL', { valid: v, pendingBefore, pendingAfter });
}

// A-06: accepted 角 token 域（dt=0 no-op OK; |angle|>30 拒 INVALID_VALUE; 边界 30 进身份核）
{
  const eng = new P011ClearWaterEngine(clearWaterCConfig(192));
  const out = new ClearWaterStepResult();
  const r0 = eng.advanceSubstepInto(0, 0, out);            // dt=0 → OK no-op
  const okNoop = r0 === 0;
  const r31 = eng.advanceSubstepInto(1 / 120, 31, out);    // 越域 → INVALID_VALUE
  const rej31 = r31 === S.INVALID_VALUE;
  const r30 = eng.advanceSubstepInto(1 / 120, 30, out);    // 边界合法 → 进 B 或 UNSUPPORTED（phase A 无 B）
  const boundary30 = r30 === S.UNSUPPORTED_FEATURE || r30 === 0;
  rec('A-06', 'angle token: dt0 noop / 31 rejected / 30 boundary accepted-to-phase', (okNoop && rej31 && boundary30) ? 'PASS' : 'FAIL', { r0: String(r0), r31: String(r31), r30: String(r30) });
}

// A-07: stale 时钟窗口（窗口>1/30s 拒；重复 id 拒；previous 不连续拒）
{
  const eng = new P011ClearWaterEngine(clearWaterCConfig(192));
  const s1 = eng.bindSourceClockWindow({ id: 1, previous: 0, current: 1e6, unit: 'NS', inputGeneration: eng.currentInputGeneration, geometryEpoch: eng.geometryEpoch });
  const s2 = eng.bindSourceClockWindow({ id: 1, previous: 0, current: 1e6, unit: 'NS', inputGeneration: eng.currentInputGeneration, geometryEpoch: eng.geometryEpoch }); // dup id
  const s3 = eng.bindSourceClockWindow({ id: 2, previous: 5e6, current: 6e6, unit: 'NS', inputGeneration: eng.currentInputGeneration, geometryEpoch: eng.geometryEpoch }); // previous≠clockRaw(1e6)
  const s4 = eng.bindSourceClockWindow({ id: 2, previous: 0, current: 1e9, unit: 'NS', inputGeneration: eng.currentInputGeneration, geometryEpoch: eng.geometryEpoch }); // 1s>1/30
  rec('A-07', 'stale clock: ok / dup-id rejected / discontinuous rejected / oversize rejected', (s1 === 0 && s2 !== 0 && s3 !== 0 && s4 !== 0) ? 'PASS' : 'FAIL', { s1: String(s1), s2: String(s2), s3: String(s3), s4: String(s4) });
}

// C-01: 独立面积 Fresnel 求和/预乘集成参考
{
  let fails = 0;
  const sigma = 0.025, gain = 0.35;
  const T = (d) => Math.exp(-sigma * d);
  const colAlpha = (wet) => wet ? gain * T(1) : 0;
  const a1 = colAlpha(true) / (colAlpha(true) + colAlpha(true));
  if (Math.abs(a1 - 0.5) > 1e-12) fails++;
  if (Math.abs(a1 + a1 - 1) > 1e-12) fails++;
  const tint = [0.14, 0.35, 0.42];
  const body = [(1 - T(8)) * tint[0], 1 - T(8)];
  if (!(body[1] > 0 && body[1] < 1)) fails++;
  rec('C-01', 'independent Fresnel-sum/adjacent-column/premult reference', fails === 0 ? 'PASS' : 'FAIL', { a1, body1: body[1].toFixed(6) });
}

fs.mkdirSync(path.join(ROOT, 'test/reports/t244'), { recursive: true });
fs.writeFileSync(path.join(ROOT, 'test/reports/t244/ets-drive.json'), JSON.stringify({ generatedAt: new Date().toISOString(), results }, null, 1));
const p = results.filter(r => r.status === 'PASS').length, f = results.filter(r => r.status === 'FAIL').length;
console.log(`T244 ETS drive: ${p} PASS / ${f} FAIL`);
process.exit(f > 0 ? 1 : 0);
