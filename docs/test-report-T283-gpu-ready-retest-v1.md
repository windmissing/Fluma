# T283 GPU READY 修复真机续证 · 报告 v1

- 角色：tester（真机 VYG-AL00 62T0225B10010020）· 日期：2026-10-30 · 状态：**BLOCKED（T282 READY 原子性真机验证 PASS；K192 整步上屏链被挂起-恢复 delta 环境阻断，同 T273；回 T269 architect）**
- 被测包：**T282-signed.hap**（SHA `21804317436c0fb0e212f24a6c7c098eed97e2336b602c04f540892348d2039e`，源 T282-ready-atomic）
- 页面：`pages/GpuResidentM0`（`--ps grsm0 true`）；pid 50339 会话；证据 `test/reports/t283/`（a–y 快照 20+、t283z.jpeg、suspend.txt）

## T282 修复真机验证（新字段/新行为逐项）

| 验证项 | 结果 | 证据 |
|---|---|---|
| `permissionState`/`readyEpoch`/`staleQueuedRejected` 新字段上真机 | ✅ | 全部快照含三字段，值随状态机变化 |
| READY 原子提交（CommitReadyLocked） | ✅ | `runLo` 0→11 逐次递增；每次 rebuild 后 permissionState 曾到达 READY（由随后 `PAUSED_CLOCK` 反证——REVOKE 只能从 READY/CLOCK_GAP 路径来，STATE_LOST 值不再出现于已提交 run） |
| 一致权限（permissionState 与 ready/readyEpoch 同步） | ✅ | PAUSED_CLOCK 时 ready=false、readyEpoch=0、permissionState=PAUSED_CLOCK 三者一致；无"ready=true 但 permission 非 READY"的组合 |
| CURRENT 与 READY 分列 | ✅（结构） | permissionState 独立于呈现状态；CURRENT 未达成（见阻断） |
| CLOCK_GAP 保原序列（不猜 Doze 唯一因） | ✅ | advanceCode=6、plannedQuanta 保留、PAUSED_CLOCK→NEW_RUN_REQUIRED 显式 |
| 无 FAULT、无旧 READY 复用 | ✅ | 全会话零 FAULT；epoch 每次显式递增（54+ 次 newRun 全部生效） |

## 阻断：READY→CLOCK_GAP 极速循环，K192 整步无法推进

**事实链**（本轮 60+ 次观察）：每次 `New empty run` → rebuild → **READY 提交成功**（runLo++、phase=READY）→ 下一 8ms tick 判定 `delta>33,333,333ns` → **立即 CLOCK_GAP 撤权**（PAUSED_CLOCK/NEW_RUN_REQUIRED）。20 次连续 0.5s 间隔快照 100% PAUSED_CLOCK、0 次 READY 捕获——READY 窗口 <0.5s。

**根因（与 T273 相同环境因子）**：USB 调试+熄屏场景下进程/线程被周期性挂起，恢复后首个 wall delta 必超 33.3ms。45 连发设备端脚本（wakeup+newRun+spawn，无主机往返）亦未能建立稳定 8ms tick 流。

**尝试记录**：①power-shell timeout 900000+wakeup；②主机 0.2–0.5s 级高频快照；③设备本地 45 连发 newRun+spawn；④Home 键→重进触发 onPageShow（排除 active=false 假设——重进后 runLo 继续递增证明 active 正常）；⑤单 shell 三连击。均无法阻止挂起-恢复 delta。

## NOT_RUN（如实）

- 同一 K192 run source terminal→quantum/completion256→lease→CURRENT 同 token 像素：NOT_RUN（READY 窗口 <0.5s，spawn 无法在窗口内被消费；acceptedCount 恒 0——**acceptedCount=0 不能证保库存**，按卡如实登记）。
- K512/完整 fault 矩阵/收益：NOT_RUN（先最小闭环）。

## 与 T273/T282 报告关系

- T273 的 Doze 阻断结论**被本轮强化**（同因子再现于 T282 修复后）。
- T282 离线 18/0 与真机表现一致部分：READY 原子性/撤权/字段——无矛盾；未覆盖的仅是"挂起-恢复 delta 频发"场景，属环境而非离线测试缺陷。

## 继续条件

亮屏人工保持前台（或挂起豁免），READY 稳定 ≥2s 后 spawn→8s 内可完成整步链。复现：装 T282 → `aa start --ps grsm0 true` → New empty run → 观察 permissionState 在 READY/PAUSED_CLOCK 间翻转（runLo 递增、readyEpoch 恒 0 于 PAUSED_CLOCK 期）。
