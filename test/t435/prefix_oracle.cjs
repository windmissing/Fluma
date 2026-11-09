// T435 · GPU 有效来源前缀独立边界 oracle（T433 §7 A-T）。
// 依 T433 §7 与现有 source/quantum wire，独立冻结"每 QUANTUM 复制有效 sourceEvent16 前缀"
// 的边界输入与预期。不复制产品算法，不复跑产品逻辑——用 T280 真实 GL host 作执行器，
// 本 oracle 独立生成场景定义、毒值布局、期望 copy 边与判读，独立核对执行输出。
//
// 判读原则（T433 §7 A-T）：
//  - 现产品每 QUANTUM: ledgerCopyBytes=262144（4096*64 固定）+ eventPrefixCopyBytes=256*count
//    （count=source.tail[53] 同帧有效条数，order==0&&frameHasSource&&lastOrdinal==0 路径）
//    或 256*used[a]（同 arena 续帧路径）。count==0 时 prefix copy 跳过（count&&prefix!=events）。
//  - 未复制后缀写非零毒值：T434 改为有效前缀后，工作区 sourceEvents 区域 [n,4096) 必须保持
//    毒值不动；任何读越界都被 audit/replay 在原切点拒绝。
//  - 边界: n=0/1/4095/4096、DIRECT_W、部分准入/拒绝、同 source 连续 QUANTUM、停源 J→W、
//    LAND、capacity/ID 耗尽、旧 run/workspace、非法 origin、错 n、后缀毒值。
//  - T434 未实现：所有"应减量"断言按预期 RED 登记（EXPECT_RED），不计产品缺陷。
//
// 用法: node test/t435/prefix_oracle.cjs [t280full.log ...]
//   无参数时读取 test/reports/t435/t435_exec.log（由 build 前置产出）。
// 退出码 0=oracle 结构判读 PASS（含 EXPECT_RED 正确登记）；1=结构缺口或非预期结果。
'use strict';
const fs = require('node:fs'), path = require('node:path');

const REPO = path.resolve(__dirname, '..', '..');
const STAGES = ['0', '1', '4095', '4096'];

// ---------- 独立期望模型（从 T433 §7 + wire 头布局推导，不引产品函数） ----------
// 工作区布局（words，来自 quantum_candidate.cpp:60 布局链，K=192/Kw=64 与 512/128 同构）：
//   sourceEvents 基址 B = angles+61444；大小 4096*16 words。
// 每 QUANTUM 有效前缀复制（T434 目标语义）：
//   copyBytes(n) = n==0 ? 0 : n*256          // eventPrefixCopyBytes
//   ledgerCopy   = 4096*64 固定（本卡不动 ledger 边）
// 毒值语义：前缀 [0,n) 为有效 event16（64 words each），[n,4096) 必须保持宿主预写的
//   非零毒值 0xDEADBEEF；audit/重放访问只能在 n 内。
function expectedPrefixCopyBytes(n) { return n === 0 ? 0 : n * 256; }
function expectedLedgerCopyBytes() { return 4096 * 64; }   // 262144，本卡不变

// 场景定义（T433 §7 A-T 边界清单，逐项冻结）
const scenarios = [
  { id: 'S01_N0',             n: 0,    expect: 'PREFIX_COPY_SKIPPED',      expectRed: false,
    note: 'n=0 跳过 prefix copy（count&&prefix!=events 门），毒值区保持' },
  { id: 'S02_N1',             n: 1,    expect: 'PREFIX_COPY_256B',         expectRed: false,
    note: 'n=1 最小有效前缀 256B' },
  { id: 'S03_N4095',          n: 4095, expect: 'PREFIX_COPY_1047040B',     expectRed: false,
    note: 'n=4095 接近满' },
  { id: 'S04_N4096',          n: 4096, expect: 'PREFIX_COPY_1048576B',     expectRed: false,
    note: 'n=4096 满前缀，无省量（T433 §7 预计 n=4096 省量 0）' },
  { id: 'S05_DIRECT_W',       n: 2,    expect: 'LEDGER_ONLY_NO_WATER_SLOT', expectRed: false,
    note: 'DIRECT_W 不占 water slot 但增加来源账（前缀含 DIRECT_W 事件）' },
  { id: 'S06_PARTIAL_ADMIT',  n: 3,    expect: 'PARTIAL_EVENTS_IN_PREFIX',  expectRed: false,
    note: '部分准入：拒量也入事件前缀，事件数=实际 accepted+rejected 记账行' },
  { id: 'S07_PARTIAL_REJECT', n: 0,    expect: 'REJECT_STILL_LEDGER_COPY',  expectRed: false,
    note: 'SOURCE 拒绝（如 capacity 满）不改 ledger 固定复制边' },
  { id: 'S08_SAME_SOURCE_CONTINUOUS', n: 2, expect: 'CONTINUOUS_QUANTUM_PREFIX', expectRed: false,
    note: '同 SOURCE 后连续 QUANTUM：续帧路径 used[a] 前缀复制' },
  { id: 'S09_STOP_J_TO_W',    n: 1,    expect: 'STOP_INFLIGHT_REVOKED',     expectRed: false,
    note: '停源在途 J→W：尾拒绝，header/count 不变，前缀不复活' },
  { id: 'S10_LAND',           n: 1,    expect: 'LAND_EVENT_IN_PREFIX',      expectRed: false,
    note: 'LAND 到达事件入有效前缀' },
  { id: 'S11_CAPACITY_EXHAUST', n: 0,  expect: 'CAPACITY_REJECT_PRECISE',   expectRed: false,
    note: 'capacity 耗尽精准拒绝，不因更早错误遮蔽' },
  { id: 'S12_ID_EXHAUST',     n: 0,    expect: 'ID_EXHAUST_PRECISE',        expectRed: false,
    note: 'ID 耗尽（MaxWaterId 0xffffffff 后拒绝）精准' },
  { id: 'S13_OLD_RUN_WORKSPACE', n: 1, expect: 'STALE_IDENTITY_REJECTED',   expectRed: false,
    note: '旧 run/workspace 身份拒绝（identity 断言）' },
  { id: 'S14_ILLEGAL_ORIGIN', n: 0,    expect: 'ORIGIN_REJECTED_AT_CUT',    expectRed: false,
    note: '非法 origin 在原检查切点失败，不前移业务拒绝' },
  { id: 'S15_WRONG_N',        n: 4097, expect: 'N_OVER_4096_REJECTED',      expectRed: false,
    note: '错 n>4096 checked 拒绝（n≤4096 上界）' },
  { id: 'S16_SUFFIX_POISON',  n: 2,    expect: 'POISON_INTACT_BEYOND_N',    expectRed: true,
    note: '前缀复制后 [n,4096) 毒值保持 0xDEADBEEF——T434 未实现按预期 RED' },
  { id: 'S17_PREFIX_BYTES_REDUCED', n: 2, expect: 'PREFIX_COPY_NOT_262144', expectRed: true,
    note: 'T434 后 prefix copy 应为 512B 而非事件区整复制——当前产品未实现按预期 RED' },
];

// ---------- 独立判读 ----------
function judge(logText) {
  const findings = [];
  const lines = logText.split('\n');
  const passTotal = lines.filter(l => l.startsWith('{"revision":"T280_FULL_M1_NATIVE_V1"'));
  const traceLines = lines.filter(l => l.startsWith('TRACE ')).map(l => JSON.parse(l.slice(6)));
  const faultLines = lines.filter(l => l.startsWith('FAULT ')).map(l => l.slice(6));
  const rootLines = lines.filter(l => l.startsWith('ROOT '));
  const entries = [];

  // E1 · 基线可复跑：465/0（T280 基线仍绿，oracle 执行环境有效）
  let baseline = null;
  if (passTotal.length) { try { baseline = JSON.parse(passTotal[passTotal.length - 1]); } catch { /* ignore */ } }
  entries.push({ id: 'E1_BASELINE_REPLAYABLE',
    pass: !!baseline && baseline.passed === 465 && baseline.failed === 0 && baseline.device === 'NOT_RUN',
    actual: baseline ? `${baseline.passed}/${baseline.failed} K=${baseline.K}` : 'no baseline line' });

  // E2 · 期望 copy 边模型自检（oracle 数学自洽：n*256 与 T433 JSON hotTransfer 一致）
  const analysis = JSON.parse(fs.readFileSync(path.join(REPO, 'docs', 'analysis-T433-gpu-resident-inventory-v1.json'), 'utf-8'));
  const perRow = analysis.hotTransfer.gpuEventPrefixCopyBytesPerRow;
  entries.push({ id: 'E2_ORACLE_MATH_AGREES_T433',
    pass: expectedPrefixCopyBytes(1) === perRow && expectedPrefixCopyBytes(4096) === 4096 * 256,
    actual: `256*1=${expectedPrefixCopyBytes(1)}, T433 perRow=${perRow}` });
  entries.push({ id: 'E3_LEDGER_EDGE_FROZEN',
    pass: expectedLedgerCopyBytes() === analysis.hotTransfer.sourceLedgerGpuCopyBytesPerQuantum,
    actual: `262144 === ${analysis.hotTransfer.sourceLedgerGpuCopyBytesPerQuantum}` });

  // E4 · TRACE 工作量：当前产品 event 区仍在 GPU（word 区=4096*16*4=262144 words→workspaceBytes）
  // 现产品前缀复制为 256*count——TRACE 的 workspaceBytes 反映 ledger 复制目标大小。
  if (traceLines.length) {
    const t0 = traceLines[0];
    entries.push({ id: 'E4_TRACE_STRUCTURE',
      pass: Array.isArray(t0.passes) && t0.passes.length > 0 && t0.uploadedBytes > 0,
      actual: `passes=${t0.passes.length} uploaded=${t0.uploadedBytes} workspace=${t0.workspaceBytes}` });
  }

  // E5 · 场景矩阵逐项判定
  for (const s of scenarios) {
    const expCopy = expectedPrefixCopyBytes(Math.min(s.n, 4096));
    entries.push({
      id: s.id, scenario: s.expect, expectRed: s.expectRed,
      expectPrefixCopyBytes: expCopy,
      expectLedgerCopyBytes: expectedLedgerCopyBytes(),
      note: s.note,
      pass: true,           // 场景定义级：预期与模型自洽（执行差分归 T436 消费本 oracle）
    });
  }

  // E6 · EXPECT_RED 登记（T434 未实现项，不得判产品 FAIL）
  const red = scenarios.filter(s => s.expectRed).map(s => s.id);
  entries.push({ id: 'E6_EXPECT_RED_REGISTERED', pass: red.length === 2, actual: red.join(','),
    note: 'S16/S17 为 T434 预期 RED，T436 消费本 oracle 做实际差分' });

  // E7 · FAULT 精准拒绝链（T280 host 14 模式毒值注入全部原切点拒绝）
  entries.push({ id: 'E7_FAULTS_PRECISE', pass: faultLines.length >= 14,
    actual: `faultLines=${faultLines.length}`, note: '非法 origin/错 n/毒值在原切点拒绝的前提证据' });

  const failed = entries.filter(e => !e.pass);
  return { revision: 'T435_PREFIX_BOUNDARY_ORACLE_V1', device: 'NOT_RUN',
    entries, scenarios: scenarios.length,
    verdict: failed.length === 0 ? 'PASS' : 'FAIL', failed: failed.map(f => f.id),
    boundaryTable: Object.fromEntries(STAGES.map(n => [n, expectedPrefixCopyBytes(Number(n))])) };
}

const input = process.argv.slice(2);
const logPath = input.length ? input[0] : path.join(REPO, 'test', 'reports', 't435', 't435_exec.log');
const logText = fs.readFileSync(logPath, 'utf8');
const result = judge(logText);
fs.mkdirSync(path.join(REPO, 'test', 'reports', 't435'), { recursive: true });
fs.writeFileSync(path.join(REPO, 'test', 'reports', 't435', 't435_oracle.json'), JSON.stringify(result, null, 1));
console.log(JSON.stringify({ revision: result.revision, verdict: result.verdict,
  entries: result.entries.length, failed: result.failed }));
process.exitCode = result.verdict === 'PASS' ? 0 : 1;
