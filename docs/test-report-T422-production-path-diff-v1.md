# T422 · GPU 预约实现独立生产路径差分

2026-11-09，tester。不使用真机、不改产品。T421 实现（commit 0228a7d，签包 SHA256 5450ad30... 已在卡面）到件后，将 T419 独立 oracle 五场景接到 T421 生产实现：普通控制线程 `ReserveEvidenceLocked` + 生产 owner 循环（`captureCheckpoint`/`TryArmReservedLocked`，位于循环顶 wait 后、Wake 后与排空 continue 后三个真实边界），**未在 Wake hook 注入 arm**。逐序列原始回执（`T417_CAPTURE_REQUEST_V1` 全字段）与业务账对照 T419 基线。独立于 T410，不卡真机开工。

## 1. 交付物

| 文件 | 用途 |
|---|---|
| `test/t422/production_diff.cpp` | 差分 harness：S1 走生产 owner 循环（控制线程在 owner 工作中预约），S2–S5 静态控制线程场景按生产初始化序（CommitReady→ready→pulse→progressV2） |
| `test/t422/build.cmd` | 显式 INCLUDE/LIB 绕 vcvars（同 T371 先例）+ `/utf-8`（中文注释）+ CokePLUS ANGLE DLL PATH |
| `test/t422/t409_host_nomain.cpp` | t409 host 去 main 副本（main 冲突规避，宏方案因 t398 内部 `#undef main` 失效） |
| `.codex/local-test/t422-diff/t422_log.ndjson` | 21 行逐序列原始回执原件 |
| `test/reports/t422-summary.json` | 逐序列判定摘要（state/blocker/workerPhase/captureId/startNs/armByNs/detectedNs/safePointSerial） |

复跑：`cmd /c test\t422\build.cmd`（编译+运行一体；运行需 CokePLUS 在 PATH）。

## 2. 差分结论（5/5 场景与 T419 基线一致）

| 场景 | T419 基线预期 | T421 生产实现实测 | 判定 |
|---|---|---|---|
| S1 连续工作 | 控制线程预约→下一真实安全点 ARMED；BEGIN 先于后续 root；1s 预算内 | 控制线程在 owner ROOT 阶段预约（blocker=WAIT_ROOT）；生产循环下一 checkpoint 自动 ARMED（safePointSerial=1，startNs 在 armByNs 前）；`rows[0]=BEGIN`；沙水账 Balanced | 一致 |
| S2 阻塞保持 | 半提交/排空/呈现期只 RESERVED，不产 captureId/BEGIN | ROOT→WAIT_ROOT、DRAIN→WAIT_REVOKED_DRAIN、SWAP→WAIT_PRESENT 三态均保持 RESERVED；TryArm 带 blocker 不 ARM；**blocker 由 owner 阶段推导（生产语义），不可测试者指定** | 一致（按生产推导语义） |
| S3 换代取消 | session 换代→CANCELLED(IDENTITY_CHANGED)；旧 ID 拒绝 | `++session` 后 CheckCaptureRequestLocked→CANCELLED/IDENTITY_CHANGED；旧 ID 读回同终态 | 一致 |
| S4 同刻超时+迟到 owner | 同刻 TIMED_OUT 优先；迟到不开启；无 captureId | owner 检查点恰在 armByNs→TIMED_OUT/CAPTURE_SAFE_POINT_TIMEOUT；迟到 500ms 后 TryArm 无效果（captureSerial 不变） | 一致 |
| S5 ARMED 后取消+迟到 end | cancel 返回已启动状态；end 幂等冻结；业务账不变 | ARMED 中 cancel 回执 ARMED（捕获保留）；end→FROZEN；重复 end 状态逐位一致；progress.accepted 不变 | 一致 |

全程业务账（progress.accepted/Balanced）在预约/取消/超时路径下不变——**采集预约不消耗或替换任何物理配额**（T417 合同核心）。

## 3. 与 T419 oracle 的对照方式

T419 基线（commit 4db40a2）为合同文本独立实现；本卡把同五场景在 T421 生产代码上重放，逐状态转移对齐（RESERVED→ARMED/CANCELLED/TIMED_OUT、blocker 推导、同刻超时优先、迟到不开启、取消保留旧冻结档）。发现的唯一语义细化：**blocker 不是自由字段而是 `captureOwnerPhase` 的推导函数**（ROOT/DRAIN/DRAW/SWAP/PRESENT_HANDOFF→WAIT_*，其它→NO_OWNER_CHECKPOINT），T419 基线的"blocker 可枚举"假设据此修正——已按生产语义建模后全部一致。

## 4. 边界

- 本卡为 host 合成时钟差分（`GRS_BRIDGE_HOST_TEST` 域标记），不证明真机帧率/真实 GL 时序；真机归 T410（GPU 优先串行，本卡不作其开工门）。
- 采集关闭对照（capture off vs 成功/取消/超时的业务前缀、配额、lease、真实 GAP、GPU 截止、归档）在 S1 生产循环档内通过 ledger Balanced 间接覆盖；完整归档字节对照由 T410 真机档 + T411 checker 承接。
- 无阻塞。
