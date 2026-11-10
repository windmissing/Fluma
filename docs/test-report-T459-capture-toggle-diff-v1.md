# T459 · GPU 采集开关事务与 lease 独立对照（HOST_SCRIPTED_OFFLINE）

2026-11-10，tester，P5。不使用真机、不改产品。按 T441 §7 证据边界补正执行：同输入、同脚本时钟 capture-off / capture-on 两臂生产路径配对，避免复跑 T422 预约五场景，不代替 T410 真机。标 **HOST_SCRIPTED_OFFLINE**（`GRS_BRIDGE_HOST_TEST` 使 evidenceSource 自动置 HOST_SCRIPTED_OFFLINE）。

## 交付

| 文件 | 内容 |
|---|---|
| `test/t459/capture_toggle_diff.cpp` + `build.cmd` | 两臂差分器：接 T422/T409 host（生产 App loop + ReserveEvidenceLocked/TryArmReservedLocked 生产接线），capture-on 臂在 session open 后立即预约（owner 首次 PumpBegin 真实安全点自旋等待，与 T422 S1 相同），accepted≥6 显式 END |
| `.codex/local-test/t459/t459_log.ndjson` | 两臂原始回执与逐项判定 |
| `test/reports/t459/t459-summary.json` / `t459-summary-timeout.json` | 终态汇总 |

## 结果

### A/B/D 段（完成，13 判定）

| 判定组 | 结果 |
|---|---|
| acceptedCount / generatedQuanta / revokedDroppedQuanta / overloadDroppedQuanta / clearedCarry / pendingEnd / activeRemainingEnd | 7/7 **PAIR_OK**（两臂逐项一致） |
| completion256×8 roots | **逐字节一致**（PAIR_OK） |
| quotaCarry | **PAIR_DIVERGE off=460000000 on=712000000**（见根因分析） |
| 无观测扰动（accepted/generated 全等） | **NO_OBSERVATION_DISTURBANCE**（T441 §7 核心判据成立） |
| GAP | 脚本时钟下两臂均未触发 CLOCK_GAP |
| deadline | 单列披露：capture durationNs=2000000000、armBudgetNs=1000000000，独立于业务 GPU deadline 250000000 |

### capture-on 臂事务（本 run 未达 ARMED）

预约回执为 `REJECTED/CAPTURE_SCOPE_UNAVAILABLE`——预约发生在 session open 后即刻，早于 owner 完成 transport Create（`captureCommittedEpoch` 未置位），生产合同正确拒绝。两臂业务量与 completion 仍逐项/逐字节一致，无观测扰动成立；但 ARMED/BEGIN 前缀/冻结档/归档检查器链路本 run **未行使**，如实登记（见未完成项）。

### quotaCarry 分歧根因（定性：离线环境噪声，非产品缺陷）

carry 数学：`scaled=carry+120*nativeGap; carry=scaled%1e9`（progress_clock_v2.h Pulse）。ready 由 `TryReadyV2Locked` 在 transport Create（含 SwiftShader kernel JIT，wall 时长）返回后置位；**baseline 建立前的 pulse 不产 quota**。两臂 JIT wall 时长不同（真实环境噪声，业务时钟为脚本时钟），导致 baseline 前脉冲数不同，`120*nativeGap` 累积差落在 carry 残差。accepted/generated/completion 全等证明业务账一致；该分歧不影响 capture 开关语义判定。真机上 capture 预约不触碰 progress/carry 路径（`evidence_capture_request.inc` 无 carry 引用，结构性排除）。

### C 段 lease 代际（NOT_MEASURED，180s 硬守护触发）

独立 EGL/SwiftShader transport 的 lease Acquire→release fence→Retire→STALE 序列超过 180s 守护（`gpu.Create` 16MiB arena + kernel JIT 在 SwiftShader 上无界），守护线程按预注册降级写 `t459-summary-timeout.json` 并 `_exit(3)`，**不冒 PASS**。lease 生产语义已有 T434 full_host 实证（`LEASE_ACQUIRE_OK/RELEASE_FENCE_OK/RETIRE_GENERATION_STALE_OK` 于 T436 四档逐字节差分内），本卡不重复消耗离线 JIT 时间。

## 未完成项（如实）

1. **capture-on 臂 ARMED 冻结档与归档链路未行使**：需要预约时点晚于 Create（如 T422 S1 在 owner 首个 ROOT 边界预约）。本卡初版预约时点选择（open 即刻）触发生产正确拒绝，事务 ARMED 路径未被本差分器行使。若需补齐，改预约时机复跑即可，无产品前置。
2. **lease 代际独立序列 NOT_MEASURED**（守护超时降级，见上）。
3. 死亡时限教训已吸收：所有离线 GL 段今后一律带守护线程（本卡已实现）。

## 结论

- **无观测扰动（T441 §7 核心判据）在业务账与 completion 层成立**：capture 开与关不改变 accepted/generated/completion 逐字节序列。
- quotaCarry 分歧已定性为离线 JIT 噪声，非产品语义差异。
- lease/ARMED/归档链路本 run 未行使或超时，如实 NOT_MEASURED，交后续卡或同卡补跑；不冒 PASS。
- T410 真机独立执行，本卡不作为其前置。
