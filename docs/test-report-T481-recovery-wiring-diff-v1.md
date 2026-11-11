# T481 · GPU 显式恢复生产接线独立差分（v1）

- 日期：2026-11-11
- 角色：tester（离线卡，不使用真机）
- 结论：**PASS 31/31**。消费 T467 真实生成 bridge（`t467-unit` 生成目录 app_bridge.cpp + recovery_v3.h），断言全部从 T451v2/T464 合同独立推导，不复用开发 control_host 23 项自测作判据。
- 代码：`test/t481/recovery_diff.cpp`、`test/t481/build.cmd`
- 证据：`test/reports/t481/t481-recovery-diff.json`（域标注 HOST_SCRIPTED_OFFLINE）
- 域区分：GL 全链走现有 ANGLE/SDK shim 离线设施；期限/GAP 时序为注入控制时钟，不当作设备期限/fps 结论（真机归 T473）。

## 覆盖范围（T451v2/T464 合同 → 31 项）

| 组 | 覆盖 |
| --- | --- |
| R1–R6 恢复事务链 | 显式新空场 → open=false/permit 撤/retiring+retired 登记；资源交接前 session 不建立；资源完成后未 READY；session 身份门（错 run/旧 serial 拒）；合法 token；首 pulse 建 baseline（READY 达成、generated=0）；第二 pulse 起 quota 且账平衡 |
| R7–R8 GAP 撤权 | 33.3ms GAP → FAULTED_REVOKED；旧 token pulse → STALE_PAGE_SESSION ×3 |
| R9 快速两 reset | resetSerial 严格递增；旧 token 新代拒绝；新代 1 pulse READY |
| R10 窗口/后台取消 | 后台 permit 卸下；后台 session 请求取消；回前台不自动 READY（无伪心跳/旧 permit 续期） |
| R11 capture RUN_RESET 终态 | RESERVED→CANCELLED/RUN_RESET；ARMED→冻结/RUN_RESET；reason 终态一次定型不被后续 reset 改写 |
| R12 250ms 期限 | 冻结不缩短/延续期限；恰到期接受、+1ns 拒绝（无宽限）；reset 杀旧 permit |
| R13 代际隔离 | 旧 epoch 资源交接、旧 serial session 双拒绝；session 溢出拒绝并置 requiresReset |
| R14 冷 clock 选择 | V1/V2 冷选各自 READY，token 递增 |
| R15 事务 tombstone | Request→RETIRING；退休完成前资源交接拒绝；512 事件环 600 溢出 drop=88；首错锁存 |

## 过程发现（差分器自身，非产品缺陷）

1. **生成目录版本差**：`.codex/local-test/t467-dev` 的 invalidate capture 撤销段为无条件 `WINDOW_OR_RUN_CHANGED`，而 control_host 实际使用的 `t467-unit` 为 `replace ? WINDOW_OR_RUN_CHANGED : RUN_RESET`——显式新空场（replace=false）下 ARMED 采集冻结 reason=RUN_RESET。初版误用 t467-dev 导致 R11B 误判，换 t467-unit 后一致。**该两目录 bridge 代码差异本身登记为观察项**（t467-dev 疑为中间版本产物）。
2. **退休两步是显式调用**：`Retiring(epoch)`/`Retired(epoch)` 由 GL owner 显式推进（断言调用有副作用），差分器 Resources 前置补齐后事务链贯通。
3. 首 pulse baseline 语义：首拍 ACCEPTED 且即达 READY（readyEpoch==epoch）但 generated=0，第二拍起产 quota。

## 缺口如实

- 两页 frame/页面 helper 12 项为 T467 page_units 范围，本卡未重复（复用已证范围）。
- 真实 GAP 域、250ms 设备成本、真机恢复时序归 **T473**（设备串行队列）。
