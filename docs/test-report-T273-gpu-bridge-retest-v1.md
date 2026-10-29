# T273 GPU 产品桥接修复真机复测 · 报告 v1

- 角色：tester（真机 VYG-AL00 62T0225B10010020）· 日期：2026-10-29 · 状态：**BLOCKED（桥接修复真机验证 PASS 到 offer 入队；quantum 推进链被设备 Doze 冻结阻断，回 T269 architect）**
- 被测包：**T272-signed.hap**（SHA `7a5644a311168f423aba1caf4b039cef89e7606e24229167987a5d0f481e7ed6`，源 `026722e`）
- 页面：`pages/GpuResidentM0`（`--ps grsm0 true`）；pid 9677 会话；证据 `test/reports/t273/`（17 项快照/截图/hilog）

## T264 两缺陷的修复真机验证结果

### 缺陷 A（napi typedarray 接缝）—— **修复真机确认 PASS**

- T264：`GRS_SOURCE INVALID_COMMAND`（每次 offer 被拒）。
- T272 后真机：`napiStage:"OK"`、`typedType:-1`（ArrayBuffer 路径，页面已改传 ArrayBuffer 128B）、`backingBytes:128`、**`lastOfferCode:"ENQUEUED_NOT_ACK"`**、`appQueueDepth:1`；hilog `GRS_SOURCE ENQUEUED_NOT_ACK`（20:41:56.724）。命令成功过 NAPI 桥并入 app FIFO。**offer 链修复生效。**

### 缺陷 B（delta 越限永久熔断）—— **按 T272 设计改为显式 CLOCK_GAP（真机行为验证）**

- T264：任何 delta>33.3ms → `failed=true` 永久熔断，状态 `FAULT/ADVANCE`。
- T272 后真机：delta>33.3ms → **`PAUSED_CLOCK / CLOCK_GAP / clockPhase=NEW_RUN_REQUIRED`**，advanceCode=6（Pause）、carryAfter=0、accepted 前缀保留（acceptedCount 恒可见）、cancelledQueued 计数、app/run FIFO 清空；newRun 按钮每按一次 runLo 递增（2→9，多次 newRun 全部生效）且无 FAULT。**永久熔断消除，NEW_RUN_REQUIRED 显式合同真机生效。**

## 阻断点：READY→source 终态→quantum 推进链无法在当前测试环境达成

**现象**：每次 newRun 后 rebuild（runLo 递增、surfaceEpoch 递增）发生，但 `ready=true`/`state=READY` 从未被观察到——rebuild 完成后的下一个 8ms tick 恒定触发 CLOCK_GAP（plannedQuanta 1–2 为 CLOCK_GAP 前最后一拍）。

**根因（进程级证据，非猜测）**：`ps -p 9677` 两次采样（间隔 5s）CPU TIME **完全不变**（00:04:36）——**进程被 Doze cgroup freezer 整体冻结**（hilog `SUSPEND_MANAGER ... DOZE has special:ERR_IN_STATE_FOREGROUND`，20:40:42 与 20:43:33 两条）。冻结解除后恢复执行的第一个 wall delta 必然远超 33.3ms → 立即 CLOCK_GAP → 循环。已尝试：power-shell timeout 900000、wakeup 循环、设备本地 90 轮保活脚本（wakeup+click 无主机往返）、aa start 前台拉起、hilog 后台写——均无法阻止 USB 调试+熄屏场景下的周期性进程冻结。`uitest dumpLayout` 读到的 status 是冻结前最后的 JS timer 快照，非实时。

**已达成与未达成**：

| T273 范围项 | 状态 |
|---|---|
| offer 实际 NAPI 分支/32word 身份 | **PASS**（napiStage OK、backingBytes 128、ENQUEUED_NOT_ACK） |
| app/run FIFO 入队 | **PASS**（appQueueDepth 1→0 消费、runQueueDepth 短暂 1） |
| source 产品终态 | NOT_RUN（source 在 FIFO 中即遇 CLOCK_GAP 取消，cancelledQueued 计入） |
| 成功 quantum/completion256 | NOT_RUN（completion 固定 32 份缓冲未见填充） |
| lease→同 token swap/像素 | NOT_RUN |
| CLOCK_GAP 行为（NEW_RUN_REQUIRED、保 accepted 前缀、不自动恢复） | **PASS（设计行为真机生效）** |
| 一次暂停恢复采 advance 原码/delta/carry/phase/epoch | **PASS**（advanceCode=6、deltaNs>33.3ms、carryAfter=0、phase=NEW_RUN_REQUIRED、epoch/runLo 逐次递增均有快照） |
| K512/末 ID/完整 fault 矩阵/收益 | NOT_RUN（按卡先最小闭环） |

## 复现与所需输入（交 T269 architect / 下次执行条件）

复现：装 T272 → `aa start --ps grsm0 true` → New empty run → Spawn sand →（若期间进程被冻结）状态 `PAUSED_CLOCK/CLOCK_GAP/NEW_RUN_REQUIRED`。
继续所需：**亮屏人工保持前台**（或关闭 Doze 对该应用的冻结——`aa start` 与 wakeup 均不足以对抗 cgroup freezer）。在有人工交互的稳定前台窗口内，从 READY 起 spawn 后 8s 内即可完成 source→quantum→completion→swap 链（T272 前 T264 时代该链卡在 offer，现已通）。

## 边界（如实）

- Node 17/0 与 transport double 14/0（T272 离线）未冒设备 PASS；本报告只列真机观测。
- 未钳 delta、未把自动恢复当已实现（卡约定）；CLOCK_GAP 后 accepted 前缀为 0（本轮无成功 accepted，"保留"语义以字段可见性为准）。
- offer "ENQUEUED_NOT_ACK 明确非源终态"——未声称 source ACK。
