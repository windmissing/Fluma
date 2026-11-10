# T459 · GPU 采集开关事务与 lease 独立对照（HOST_SCRIPTED_OFFLINE）

2026-11-10，tester，P5。不使用真机、不改产品。按 T441 §7 证据边界补正执行：同输入、同脚本时钟 capture-off / capture-on 两臂生产路径配对，避免复跑 T422 预约五场景，不代替 T410 真机。标 **HOST_SCRIPTED_OFFLINE**（冻结档回执 `evidenceSource=HOST_SCRIPTED_OFFLINE`，`clock=SCRIPTED_OFFLINE_STAMP_NS`）。

## 交付

| 文件 | 内容 |
|---|---|
| `test/t459/capture_toggle_diff.cpp` + `build.cmd` | 两臂差分器：接 T422/T409 host（生产 App loop + ReserveEvidenceLocked/TryArmReservedLocked 生产接线）。capture-on 臂在 `captureCommittedEpoch` 置位（transport Create 完成、Ready 提交后）立即预约，owner 首个安全点 ARM；accepted≥6 显式 END |
| `.codex/local-test/t459/t459_log.ndjson`（已归档至 `test/reports/t459/`） | 两臂原始回执与逐项判定 |
| `test/reports/t459/t459-summary-timeout.json` | lease 段守护降级终态 |

## 结果（最终 run，commit 见 git log）

### A/B/D 段（15 判定，除 quotaCarry 外全过）

| 判定组 | 结果 |
|---|---|
| acceptedCount / generatedQuanta / revokedDroppedQuanta / overloadDroppedQuanta / clearedCarry / pendingEnd / activeRemainingEnd | 7/7 **PAIR_OK** |
| completion256×8 roots | **逐字节一致** |
| quotaCarry | PAIR_DIVERGE off=436000000 on=460000000（根因见下） |
| 无观测扰动（accepted/generated 全等） | **NO_OBSERVATION_DISTURBANCE**（T441 §7 核心判据成立） |
| GAP | 两臂脚本时钟均未触发 CLOCK_GAP |
| deadline 单列 | capture durationNs=2000000000、armBudgetNs=1000000000，独立于业务 GPU deadline 250000000 |

### capture-on 臂事务（已行使至 FROZEN）

- **RESERVED**：requestId=1，requestNs=1000000，armByNs=1001000000（1s 诊断预算），blocker 推导正常。
- **ARMED**：owner 首个安全点开启，captureId=1，startNs=1000000。
- **END**：显式 EXPLICIT_END 冻结，FROZEN 档 196 行事件，evidenceSource=HOST_SCRIPTED_OFFLINE。
- **采集窗口**：begin=0，end=6，业务总计 8——显式 END 只冻结到触发时刻，END 后 quit 余量内接受的 7..8 不在档内，是生产正确语义（PREFIX_WINDOW_OK）。
- 两臂业务量与 completion 逐字节一致：**采集开关不改变业务账与接受序列（无观测扰动成立）**。

### quotaCarry 分歧根因（定性：离线环境噪声，非产品缺陷）

carry 数学：`scaled=carry+120*nativeGap; carry=scaled%1e9`（progress_clock_v2.h Pulse）。ready 由 `TryReadyV2Locked` 在 transport Create（含 SwiftShader kernel JIT，wall 时长）返回后置位；baseline 建立前的 pulse 不产 quota。两臂 JIT wall 时长不同（真实环境噪声，业务时钟为脚本时钟），baseline 前脉冲数不同，`120*nativeGap` 累积差落在 carry 残差。三轮 run 的 carry 值各自漂移（460/712、436/424、436/460）而业务量恒定一致，证实为噪声。真机上 capture 预约路径不触碰 progress/carry（`evidence_capture_request.inc` 无 carry 引用，结构性排除）。

### C 段 lease 代际（NOT_MEASURED，180s 硬守护触发）

独立 EGL/SwiftShader transport 的 lease Acquire→release fence→Retire→STALE 序列超过 180s 守护（`gpu.Create` 16MiB arena + kernel JIT 在 SwiftShader 上无界），守护线程按预注册降级写 `t459-summary-timeout.json` 并 `_exit(3)`，**不冒 PASS**。lease 生产语义已有 T434 full_host 实证（LEASE_ACQUIRE_OK / RELEASE_FENCE_OK / RETIRE_GENERATION_STALE_OK 于 T436 四档逐字节差分内），本卡不重复消耗离线 JIT 时间。

## 未完成项（如实）

1. **lease 代际独立序列 NOT_MEASURED**（守护超时降级，见上）。
2. quotaCarry 等值未达（已定性为环境噪声；如需等值须消除 JIT 相位差，属测试环境工程而非产品验证）。

## 结论

- **无观测扰动（T441 §7 核心判据）在业务账、completion 逐字节与 ARMED→FROZEN 全链路成立**。
- RESERVED→ARMED→END→FROZEN 事务语义在脚本时钟下完整闭环，采集窗口语义正确。
- lease 段 NOT_MEASURED 如实登记；T410 真机独立执行，本卡不作为其前置。
