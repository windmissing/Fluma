# T419 · GPU 采集安全点离线反例基线

2026-11-09，tester。不使用真机、不改产品代码。按 `docs/architecture-T417-gpu-capture-reservation-v1.md` 合同文本独立构建可重放边界序列与预期，交 architect 消费。

## 1. 交付物

| 文件 | 用途 |
|---|---|
| `test/t419/safepoint_baseline.py` | 独立 oracle：Owner 循环仿真（WorkInterval 覆盖段 + 循环顶安全检查点）+ 预约状态机（按 T417 合同逐条实现，不引用 entry/ 产品代码） |
| `test/reports/t419/t419_baseline.json` | 五场景预期序列 + T409 Wake-hook 前置对照结论 |

运行：`python -X utf8 test/t419/safepoint_baseline.py`（自测 PASS，退出码 0）；`--trace S1..S5` 打印单场景事件序列。

## 2. 五场景基线结论

| 场景 | 覆盖 | 预期（基线） |
|---|---|---|
| S1 连续工作 | 迭代中途/两迭代间预约；队列非空安全点；owner 在 wait；GAP 后排空中；V2 稳定暂停 | 预约→RESERVED；下一真实安全点 TryArm→ARMED（captureId+startNs+BEGIN 同锁）；GAP 排空中只 RESERVED（blocker=`WAIT_REVOKED_DRAIN`），排空完成才 ARM；BEGIN 先于后续 root 事件；1s 预算内必 ARM |
| S2 旧 jobLive | 终态旧元数据残留；真实在途 root；HasFlight 缓存假阴性（半提交） | 残留 jobLive 不阻塞安全点（按真实完成识别），且采集入口不清位；在途 root→RESERVED（`WAIT_ROOT`）；**缓存 captureRootBusy=false 但快照发现半提交→仍 RESERVED**（T417：HasFlight/SubmitPhase::IDLE 单独成立都不充分） |
| S3 预约后换 run | 换 run/epoch/session、旧 ID、上一冻结档 | 换代→CANCELLED（IDENTITY_CHANGED）；旧 ID 一切操作 `STALE_CAPTURE_REQUEST`；上一冻结档在 ARMED 前保留可导出；新 run 同配置可重新预约（requestId 递增不复用） |
| S4 超时 | 无安全点超 1s、迟到 owner、同刻、查询不刷新 | 监督线程 `TIMED_OUT/CAPTURE_SAFE_POINT_TIMEOUT`（detectedNs/lastBlocker 留档）；**owner 迟到不得开启旧预约**（无 captureId/无 BEGIN）；安全点与 armByNs 同刻→TIMED_OUT 优先；查询不刷新等待期 |
| S5 迟到结束 | 迟到 end、ARMED 后 cancel | 迟到 end 幂等返回冻结终态、端点不被改写；ARMED 后 cancel 拒绝（由 end 显式冻结） |

## 3. T409 实际 arm 对照（脚本 idle 与 UI 跨循环窗口，只读源码结论）

- T409 host 用例（`test/t409/app_host.cpp`）在 `BridgeStage::Wake` hook 内直接 `BeginEvidenceLocked` 并断言 ARMED。Wake 检查点位于循环顶 wait 块之后、`WorkInterval` 构造之前；上一迭代完整走完（含 retire）后 `captureWorkBusy` 已清 false，且 hook 自查 `!evidence && !captureRootBusy && !jobLive`——这是"已知安全边界"，脚本形态下成立。
- **生产缺口**：裸 Wake 不持有 T417 要求的安全快照（Run 无提交/在途/隔离轮询、transport 无半提交/fence、drainingRevoked 完成、本 root 交接结束、safePointSerial/workerPhase 字段）。修订实现不得以裸 Wake 位置充当安全点，须出示快照字段。
- UI 跨循环窗口：pulse 线程每 8ms 推进时钟并 PagePulse；`captureWorkBusy` 析构后为 false 的窗口同时可能发生控制换代（`seen!=epoch continue`），T409 脚本未覆盖换代与预约竞争。

## 4. 边界

- 本基线为合成层独立实现（对照 T417 合同文本），不是产品代码；当前产品（T409 签包）无预约接口，五场景对现实现整体为**预期 RED 登记**，不作为缺陷、不要求开发修复。
- 未定修复语义只记录基线；真机判读前置：修订包安全点必须可出示快照字段，超时回执须含真实检测时刻与 lastBlocker。
- 无阻塞。
