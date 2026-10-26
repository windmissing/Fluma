# test-report-T76-独立复核（上传合同/采集收尾/被动诊断）v1

日期：2026-10-26。责任：tester。任务：[6adeedbc…72a4](https://dida365.com/webapp/#p/6ac8a7e3e9adb60000000419/tasks/6adeedbce4b0127832ab72a4)。路线：`P076_DEV_DELIVERY_REVIEW_V1`。被测代码 HEAD：`59e64af`（含 T55 `b4561a2`、T72 `0c86c47`、T75 `dfa9877`、T66 `545f45e`）。状态：**OFFLINE_OBJECTIVE_PASS（11/11）/ SCREEN_DEVICE_NOT_COVERED**。

不使用真机；仅新 XComponent+shader 管线视角，无 Canvas 上屏/回退路径参与（T76-75-03 反向验证：Canvas 回调零调用）。独立执行器 `test/p076_dev_delivery_verification.cjs`，证据 `test/reports/p076_dev_delivery_verification.json`；**不镜像、不重跑 T75/T66/T72 开发自测作为唯一证据**。

## 1. T75 上传合同（4/4）

| 用例 | 内容 | 结果 |
|---|---|---|
| T76-75-01 | 绑定目标：fresh 绑定 code=0 且**仅成功时缓存**；同 revision 异 digest → 402，缓存 receipt 不被覆盖 | PASS |
| T76-75-02 | 非法 budget（0/-1/1.5/65537/NaN）结构化 400/byteBudget/零上传，逐笔原样转发 native（5 笔全达），active/pending 代号自 receipt 取得 | PASS |
| T76-75-03 | fallback 抛 `NEWPIPE_FAULT_NO_CANVAS`；drawLegacyFull/clearForeground/hideEntireGles **三个 Canvas 回调 0 调用** | PASS |
| T76-75-04 | 归档 native 转录重解析：≥12 个 PASS 标记、0 个 FAIL 行；**完整行预算数学独立复算**（65536B = 64 行 grain×1024B；strata 全量 768 行×256B） | PASS |

## 2. T66 采集收尾（3/4）

| 用例 | 内容 | 结果 |
|---|---|---|
| T76-66-01 | `P011_COMMIT_IDENTITY_V2`：hook 有无 + 每帧 7 次额外 `getP011Metric()` → attempt 键序列逐项相同且严格单调；场数组逐字节相同；重复 getter 不推进 sequence 且与末条 attempt 一致 | PASS |
| T76-66-02 | store（对抗性 worker 桩，全新消息时序）：跳写 sequence → FAILED 且 close 拒绝；CLOSED 需双流 written 严格相等；finish 幂等复用 promise、requestId 不符拒绝；计数短缺 → FAILED 无 CLOSED；metadata 冻结真实 route | PASS |
| T76-66-03 | manifest 工具（真实 `tools/p011_capture_manifest.py` 子进程）：严格 fence（state/route/计数/末条身份）→ COMPLETE；重复 raw 行 → PREFIX_OR_MISMATCH（rawRows=2 独立复核）；writtenAdmissions 短缺 → PREFIX_OR_MISMATCH；bytes/sha256/rawRows 独立重算一致 | PASS |

## 3. T72 被动诊断（3/4）

| 用例 | 内容 | 结果 |
|---|---|---|
| T76-72-01 | 诊断开/关两真实模型同触摸时序：bh/bl/mh/ml/suspH/suspL/waterTop、池 pMass、sourceEpoch **逐值一致**；事件覆盖 touch/tick(stale)/stale 停触/dry.receipt/up 停止；chunk <1800 字符 | PASS |
| T76-72-02 | ring 溢出：1000 条 → dropped=744 显式、存活 256 条、首条 sequence=745（缺口可由 sequence 检出）；每次 drain ≤2 行 | PASS |
| T76-72-03 | dry/wet sink 区分：真实干场准入成功后 **wet gesture sink 0 调用**——「sink 无回调」不能被误判为零准入的反向证据成立 | PASS |

## 4. T55/T75 入口覆盖交叉核对（1/1）

`napi_bridge.cpp` 导出表确认 `bindMaterialConfiguration`/`pumpMaterialUploads`/`stageMaterialBundle` 三入口；T75 core 测试覆盖 Stage/Pump/Activate/CanActivate/Restore/ClearSession/BindConfiguration。**覆盖缺口（记录，不改判）**：READY pending 上的绑定 402 拒绝仅在隔離第二 uploader 实例上验证；同实例 activate→rebind 时序由 bridge 测试覆盖。

## 5. 离线不可运行缺口（如实列明，交设备卡）

1. 真实 GPU 上传/fsync 时序（T75 native 为合成 GPU 边界）；
2. 设备生命周期：Want finish 路径、UI attempt 边界、真实 worker 文件 IO（T66）；
3. 诊断行在屏幕/hilog 传输层的呈现（T72）。

## 6. 结论与移交

- **11/11 PASS、0 FAIL**：三卡合同语义与开发报告声明一致，未发现需 designer 归因的缺陷；
- 执行器修正记录：T76-72-02 首版对 ring 存活行数的断言有误（预期 2，实际 256 存活），按实现语义修正期望——属测试侧修正，非判据放宽；manifest 子进程 PATH python 解析问题加绝对解释器回退——脚本自身修正；
- T74 设备卡可消费本稿：T72 诊断行格式（首条 sequence=745 检缺、dry.receipt/wet sink 区分）与 T66 CLOSED fence 判据已独立复核；
- 未改产品/判据；不使用真机；不宣称 T44 FAIL/BLOCKED 产品结论改变（完整场景仍等 T56）。

## 7. 变更记录

- 2026-10-26，v1：首次发布。11/11 PASS；T75 合同 4 例、T66 3 例（含真实 manifest 工具子进程）、T72 3 例、T55/T75 入口交叉核对 1 例；三项离线不可运行缺口列明。
