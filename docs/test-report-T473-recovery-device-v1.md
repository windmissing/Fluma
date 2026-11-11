# T473 · GPU 恢复事务真机独立验证（v1）

- 日期：2026-11-11
- 角色：tester（真机 62T0225B10010020）
- 结论：**核心恢复事务链真机达成**。T467-experience-final 包实际 V2 页，五份 T451_GPU_BOOT_PROBE_V1 探针（含 T464_RECOVERY_V1 recovery 只读对象）完整留痕。
- 证据：`test/reports/t473/`（探针 json ×5、截图/布局×4）
- 时域声明：全部时序为真机实录，不把软件 JIT wall 当设备期限；fps 不从字节推。

## 恢复事务链真机留痕（T464 合同逐项）

| 合同步骤 | 探针证据 |
| --- | --- |
| 真实 GAP → FAULTED_REVOKED | baseline：phase=FAULTED_REVOKED、reason=V2_PULSE_CLOCK_GAP、firstReason=WINDOW_OR_PAGE_CHANGED、resetSerial=1、retired=true（旧账保存） |
| 显式新空场（RESET_REQUESTED） | resetSerial 1→2、epoch 3→4、phase=CREATING_NEW、旧代 attempt(epoch3) 终态保留 terminal=True |
| 旧 root/lease 退休 | retired=true 在 CREATING_NEW 前；旧代与新代 attempt 代际隔离 |
| 资源完成 → 新 session | session 1→2 新建（旧 session 保留于旧代）；transitions：XCOMPONENT_UNWRAP→REGISTER→SIZE→WINDOW_REFERENCE→SURFACE_CREATED→UI_RESET |
| 真实 pulse → READY | 新代 attempt(epoch4) **readyEpoch=4==epoch READY 达成**、actualClock=GRS_M1_PROGRESS_CLOCK_V2 |
| GAP 复发撤权 | 随后 attempt4 终态 ff=V2_PULSE_CLOCK_GAP（设备 GPU 提交卡顿域，T451 归因一致） |

## 负控

1. **前后台**：transitions 留痕 `ACTIVE_BACKGROUND → ACTIVE_RESUME → UI_RESET`——窗口变更取消事务（resetSerial 2→3，页面显式再请求），session 归 0 待新代。无伪心跳续期。
2. **快速两 reset**：连点两次新空场 → resetSerial 3→5 每次严格 ++、epoch→7；旧代 attempt(5) 曾 READY=5 终态保留、attempt(6) 终态 ff=UI_RESET；当前 RESET_REQUESTED retired=false（第二次重新请求等待退休）——代际滚动正确无跳变。
3. **晚尾禁止接受 / root age 独立时域**：旧 token 晚尾 pulse→STALE_PAGE_SESSION 与 V2 pulse 域 39.08s vs V1 anchor 域 41.3ms 的独立分列已由 **T466**（同族设备窗口）取证，本卡引用不重复。

## 结论与移交

- T467 恢复事务真机行为与离线 T481 31/31 语义一致：RESET_REQUESTED→RETIRING→CREATING_NEW→资源交接→新 session→真实 pulse→READY，GAP 撤权、后台取消、快速两 reset 全部按 T464 合同。
- READY 达成后仍被设备域 CLOCK_GAP 撤（GPU 提交卡顿，与 T451/T410/T466 归因一致）——**恢复事务本身无缺陷**；设备 READY 窗口稳定化（coherent shader 修复）为 T410 双源/T437 收益配对的前置。
- 结果回 architect 分流；T410/T437 按原卡在设备 READY 窗口后续接。
