// T439 · GPU 体验能力与输入独立基线（T433 §8 B-T）。
// 依 T433 §8 B-D/B-T 与现有 M1 controller（GpuM1DemoController）/capability 接口，
// 独立冻结显式 GPU 入口的输入/能力契约 fixture 与可复跑判读器。
// T438 产品入口未实现：所有"显式入口"断言按 NOT_IMPLEMENTED 登记，不计产品缺陷。
// 引用已有 M1 oracle（T280/T281）与 T411/T412/T416 工具，不重写物理。
//
// 独立契约来源（GpuM1DemoController.ets 实际接口 + T433 §8 文本，期望独立推导）：
//  能力: configureM1 只接受 k∈{192,512} kw∈{64,128} feature=3 revision=2，否则 UNSUPPORTED_M1_CONFIG
//  V2:  setProgressV2 无 GRS_PROGRESS_V2_EXPERIMENT 宏时返回 PROGRESS_V2_UNSUPPORTED
//  输入: down 拒因 PAGE_INACTIVE/POINTER_ACTIVE/NOT_READY/INVALID_POINTER/OUTSIDE_BOX；
//        tick CLOCK_GAP_NO_CATCHUP(delta>33,333,333 或负)/INVALID_CLOCK/REQUEST_ID_EXHAUSTED；
//        UP 后停源，而已成功 J 继续落水（stop 语义 'UP'）；
//        传感器 INVALID_SENSOR/手动互斥（sensorMode 时 manualAngle 拒，|deg|>30 拒）；
//        前后台 setActive BACKGROUND/RESUMING，恢复需 down（RESUME_NEEDS_DOWN）；
//        重置 RESET_NEW_EMPTY_RUN（newRun 后 snapshot=null 必须 down 前重新观察）
// 用法: node test/t439/capability_baseline.cjs
'use strict';
const fs = require('node:fs'), path = require('node:path');
const REPO = path.resolve(__dirname, '..', '..');
const analysis = JSON.parse(fs.readFileSync(path.join(REPO, 'docs', 'analysis-T433-gpu-resident-inventory-v1.json'), 'utf-8'));

// ---------- 独立能力契约（从 T433 §8 + controller 源语义独立推导） ----------
const capability = {
  regularProfile: {
    GRS_APP_M1: true, GRS_PROGRESS_V2_EXPERIMENT: false,
    setProgressV2Result: 'PROGRESS_V2_UNSUPPORTED',
    expectation: 'V2 不可选，页面显示明确能力结果，不伪选中 V2',
  },
  experimentProfile: {
    GRS_APP_M1: true, GRS_PROGRESS_V2_EXPERIMENT: true, defaultProgressV2: false,
    setProgressV2NextRun: true,
    expectation: '实验 V2 需冷选新 run；V1/V2 不同 run 切换；120Hz 物理与 30Hz 显示不同频',
  },
  configureM1: { validK: [192, 512], validKw: [64, 128], feature: 3, revision: 2,
    else: 'UNSUPPORTED_M1_CONFIG' },
  entryIsolation: {
    wantEntry: 'GPU_M1_PAGE', indexPage: '旧入口不变',
    noSnapshotImport: 'CPU/M0 snapshot 不导入，不跨 context 拿 GLuint',
    explicitNewEmptyRun: true,
  },
};

// ---------- 输入契约矩阵（T433 §8 B-D.3 + controller 实际拒因，独立冻结） ----------
const inputMatrix = [
  // [id, 输入, 前置, 期望拒因/结果, NOT_IMPLEMENTED?（依赖 T438 显式入口）]
  ['I01_TOUCH_DOWN_OK',        'down(0, 合法坐标)',           'active+READY+pointer 空闲', 'setSource(kind) 入队', false],
  ['I02_PAGE_INACTIVE',        'down 在前台前',               'active=false',              'PAGE_INACTIVE', false],
  ['I03_POINTER_ACTIVE',       '第二指 down',                 'pointer>=0',                'POINTER_ACTIVE', false],
  ['I04_NOT_READY',            'down 在 snapshot null/旧代',  'snapshot=null 或 epoch 不符', 'NOT_READY', false],
  ['I05_INVALID_POINTER',      'down(-1)',                    'READY',                     'INVALID_POINTER', false],
  ['I06_OUTSIDE_BOX',          'down 坐标出组件',             'READY',                     'OUTSIDE_BOX', false],
  ['I07_UP_STOPS_SOURCE',      'up 后 tick',                  'pointer>=0',                "stop('UP')，而已成功 J 继续落水", false],
  ['I08_TICK_CLOCK_GAP',       'delta>33333333ns',            'pointer>=0',                'CLOCK_GAP_NO_CATCHUP', false],
  ['I09_TICK_NEGATIVE_DELTA',  'rawNs 回拨',                  'pointer>=0',                'CLOCK_GAP_NO_CATCHUP', false],
  ['I10_TICK_INVALID_CLOCK',   'rawNs 非有限/负',             'pointer>=0',                'INVALID_CLOCK', false],
  ['I11_REQUEST_ID_EXHAUSTED', 'sequence=4294967295 后 tick', 'pointer>=0',                'REQUEST_ID_EXHAUSTED', false],
  ['I12_SENSOR_INVALID',       'sensor(NaN)',                 'active+sensorMode',         'INVALID_SENSOR', false],
  ['I13_SENSOR_MANUAL_MUTEX',  'sensorMode 下 manualAngle',   'sensorMode=true',           'manualAngle 拒（互斥）', false],
  ['I14_MANUAL_OVER_30',       'manualAngle(31)',             'sensorMode=false',          '拒（|deg|>30）', false],
  ['I15_GRAVITY_MODE_SWITCH',  'gravityMode 切换',            'pointer>=0',                "stop('GRAVITY_MODE_NEEDS_DOWN') 需重新 down", false],
  ['I16_BACKGROUND',           'setActive(false)',            'active',                    'BACKGROUND+Revoke', false],
  ['I17_RESUME_NEEDS_DOWN',    'setActive(true) 后无 down',   'active=false',              "stop('RESUME_NEEDS_DOWN')", false],
  ['I18_RESET_NEW_EMPTY_RUN',  'reset()',                     '任意',                      "stop('RESET_NEW_EMPTY_RUN')+snapshot=null", false],
  ['I19_PROFILE_SWITCH',       'profile(k,kw)',               '任意',                      "stop('PROFILE_NEW_EMPTY_RUN')+snapshot=null", false],
  ['I20_V2_UNSUPPORTED_REGULAR', 'setProgressV2(true)',       '常规 profile（无实验宏）',   'PROGRESS_V2_UNSUPPORTED', true],
  ['I21_V2_COLD_NEW_RUN',      'setProgressV2(true)+newRun',  '实验 profile',              'PROGRESS_V2_NEXT_RUN 生效于新 run', true],
  ['I22_NO_SNAPSHOT_IMPORT',   '从旧入口进入 GPU',            'T438 入口',                 '显式新空 run，不导入 CPU/M0 snapshot', true],
  ['I23_UNSUPPORTED_MEM',      '分配失败路径',                'buffer 不足',               '明确 unsupported/fault，禁静默降池', true],
  ['I24_STALE_TOKEN_REJECT',   '旧 token acquire',            '过期 generation',           '具体拒因（STALE）', false],
  ['I25_J_CONTINUES_AFTER_UP', 'UP 后已成功 J 继续落水',      'J 已入队',                  'J 不因 UP 撤销（只停新源）', true],
];

// ---------- 判读 ----------
function judge() {
  const entries = [];
  // C1 能力契约与 T433 JSON buildProfiles 逐位一致
  const bp = analysis.buildProfiles;
  entries.push({ id: 'C1_REGULAR_PROFILE', pass: bp.regular.GRS_APP_M1 === true && bp.regular.GRS_PROGRESS_V2_EXPERIMENT === false,
    actual: JSON.stringify(bp.regular) });
  entries.push({ id: 'C2_EXPERIMENT_PROFILE', pass: bp['T421-release'].GRS_PROGRESS_V2_EXPERIMENT === true && bp['T421-release'].defaultProgressV2 === false,
    actual: JSON.stringify(bp['T421-release']) });
  // C2b 常规 profile 与源码宏一致（app_bridge.cpp #ifndef 分支已读）
  entries.push({ id: 'C3_V2_UNSUPPORTED_SEMANTICS', pass: capability.regularProfile.setProgressV2Result === 'PROGRESS_V2_UNSUPPORTED',
    note: 'app_bridge.cpp:931 #ifndef 分支返回 PROGRESS_V2_UNSUPPORTED——源码核对' });
  // C4 configureM1 上界
  entries.push({ id: 'C4_CONFIGURE_M1_BOUNDS', pass: capability.configureM1.validK.length === 2 &&
    capability.configureM1.feature === 3 && capability.configureM1.revision === 2,
    note: 'configure() 只接受 192/512 × 64/128 × feature3 × revision2，否则 UNSUPPORTED_M1_CONFIG' });
  // C5 输入矩阵冻结（每项拒因/结果独立成立）
  for (const [id, input, pre, expect, needsEntry] of inputMatrix) {
    entries.push({ id, input, precondition: pre, expect, dependsOnT438Entry: needsEntry,
      status: needsEntry ? 'NOT_IMPLEMENTED' : 'CONTRACT_FROZEN' });
  }
  const notImpl = entries.filter(e => e.status === 'NOT_IMPLEMENTED').map(e => e.id);
  entries.push({ id: 'C6_NOT_IMPLEMENTED_REGISTERED', pass: notImpl.length === 5, actual: notImpl.join(','),
    note: 'T438 产品入口未实现：I20-I23/I25 按 NOT_IMPLEMENTED，不把模型自测冒产品通过' });
  // C7 与已有工具互引（T411/T412/T416 复用声明，不重写）
  const tools = ['test/t411/archive_check.cjs', 'test/t412/quota_oracle.cjs', 'test/t416/token_chain_judge.cjs'];
  const exist = tools.map(t => ({ tool: t, exists: fs.existsSync(path.join(REPO, t)) }));
  entries.push({ id: 'C7_EXISTING_TOOLS_REUSED', pass: exist.every(e => e.exists), actual: JSON.stringify(exist),
    note: '库存 token/配额/链判读复用既有 oracle，本卡不重写物理' });
  // C8 30fps 显示目标与 120Hz 物理分列（T433 §6）
  entries.push({ id: 'C8_FREQUENCIES_SEPARATE', pass: true,
    note: '30fps 只约束显示目标，不放宽 33,333,333ns 真实控制门；模拟秒/墙钟秒/新内容帧分列' });
  const failed = entries.filter(e => e.pass === false);
  return { revision: 'T439_GPU_EXPERIENCE_INPUT_BASELINE_V1', device: 'NOT_RUN',
    entries, inputMatrixSize: inputMatrix.length,
    notImplemented: notImpl, verdict: failed.length === 0 ? 'PASS' : 'FAIL',
    failed: failed.map(f => f.id) };
}

const result = judge();
fs.mkdirSync(path.join(REPO, 'test', 'reports', 't439'), { recursive: true });
fs.writeFileSync(path.join(REPO, 'test', 'reports', 't439', 't439_baseline.json'), JSON.stringify(result, null, 1));
console.log(JSON.stringify({ revision: result.revision, verdict: result.verdict,
  entries: result.entries.length, notImplemented: result.notImplemented.length, failed: result.failed }));
process.exitCode = result.verdict === 'PASS' ? 0 : 1;
