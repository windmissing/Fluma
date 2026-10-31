# T329 · T324 Amendment 1 实施交付（Pump 多批循环 + UINT32_MAX 修复 + submitPumpWallNs）

2026-10-31，developer；依据 `architecture-T324-gpu-partial-submit-contract-v1.md` §7 Amendment 1 + `architecture-T328-batch-gap-ratio-ruling-v1.md`（SINGLE_RECOMMENDATION 实施规格）。不使用真机；真机验证归 T330。

## 四件实施

1. **Pump 循环**（`m1/run.cpp` submitting 分支）：单轮 Pump 内 `while` 循环调 ContinueSubmit——(i) OK→inflight 退出；(ii) OK_SUBMITTING 且累计 wall < `kPumpBudgetNs`(8,000,000ns) → 继续下一批；(iii) 预算尽 → 返回 OK 下轮续（per-Pump 语义保留）；(iv) BUSY/错误现行语义。首批仍在 SubmitQuantum 内（INV-7）。行号较裁定稿漂移（run.cpp submitting 分支现位于 :22-47），按语义对位实施。
2. **UINT32_MAX 修复**（T328 移交裁定 2）：`full_transport.cpp` SubmitQuantum lambda 尾补显式 `observed.End(uint32_t(result))`——首批 SubmitExit code=16（OK_SUBMITTING）与续批一致，失败路径获真实码；与 SubmitSource/ContinueSubmit/Poll 三函数对称；wall 计量零变化。
3. **submitPumpWallNs 遥测**：`full_transport.h/.cpp` 新字段+getter（SubmitQuantum 每根重置并计首批，ContinueSubmit 逐批累加 lastBatchWallNs；Create 复位）；bridge `UiJsonLocked` 输出（经 App 缓存字段 `submitPumpWallNsCached`——gpu 是 loop 线程局部，status JSON 在锁内读缓存；刷新点在每轮 Pump 后，M1-only 守卫）。41 字典语义不变（字段走 status 非 timeline stage）。
4. **非机回归三新用例**：
   - **N1**（partial_host CASE_N1，真实 GL 两配置）：Amendment-1 循环合同直接执行——SubmitQuantum 后的受预算约束 ContinueSubmit 循环封尾。实测 192×64：14 批 continuation 循环 **1.97ms 封尾**（≤8ms）；512×128：16 批 **2.2ms**；随后 Poll 接受 physics=2。注：submitPumpWallNs（含 SubmitQuantum 内首批 staging+dispatch）在 PC SwiftShader 下 13ms——首批超预算是 PC 软渲染特性（真机实测首批 1.7ms），循环断言用 loop 专属墙钟（Run 视角正确）。
   - **N2**（run_contract 替身，慢批 9ms>预算）：每 Pump 恰一批（continues==2 两轮）后预算截断返回 OK、无跳批、快批尾段在预算内封尾。
   - **N3**（run_contract 替身）：循环中途取消——BATCH_ACTIVE 取消语义不变（cancels==1、IDLE、active 清除）、nonce 不二次消耗（begins==1）。

不变式自检：INV-5 每批 SubmitEnter/Exit 遥测照常（41 字典 carry 逐批递增，partial_host CASE1 trace 逐位断言仍在）；INV-6 GAP 检查在 PumpEnd 后不绕过（单轮 ≤8ms 预算保证检查频率）；INV-7 首批在 SubmitQuantum 内（代码注释+断言双重体现）。

## 证据（设备 NOT_RUN；全部归档 test/reports/t329/）

| 套件 | 结果 |
|---|---|
| partial_host ×{192×64, 512×128}（真实 GL PC ANGLE） | **26/26 ×2**（原 19 + N1 新 7：loop 封尾/批数/墙钟/预算/Poll 接受） |
| run_contract（替身） | **12/12**（Amendment-1 语义重写 Case1 + N2 预算截断 + N3 循环中取消） |
| bridge 十模式 | **11/11**（M0 路径零回归） |
| app 全链 | **6/6**（accepted=2/sourceCommit=1/physicsCommit=2/CURRENT/token 相等） |
| cold units | **23/23** |
| 生产 hvigor | BuildNativeWithNinja（3.2s 实跑）+ PackageHap 绿；SignHap 00303116 已知（**无签包**，T330 的包由执行 tester 按 T151 流程补签——T328 移交裁定 1） |

## 修改文件

| 文件 | 用途 |
|---|---|
| `entry/src/main/cpp/gpu_resident/m1/run.cpp/.h` | submitting 分支循环化 + kPumpBudgetNs |
| `entry/src/main/cpp/gpu_resident/m1/full_transport.cpp/.h` | SubmitQuantum 显式 End(16)、submitPumpWallNs 字段/累加/重置 |
| `entry/src/main/cpp/gpu_resident/app_bridge.cpp` | status JSON 追加 submitPumpWallNs（App 缓存 + M1 守卫刷新） |
| `test/t324/partial_host.cpp` | N1 用例（真实 GL 循环封尾） |
| `test/t324/run_contract.cpp` | Amendment-1 语义 Case1 重写 + N2/N3 |
| `test/t324/app_gl.cpp` | revision 改 T329 |
| `test/reports/t329/*` | 六套证据归档 |

## 边界

- 不改冻结阈值/GAP 判定/物理语义；本卡使 root 在窗内封尾，不动 33ms 窗。
- 真机 J1-J5 判据（单 Pump 封尾/FenceEnter 首达/accepted/首批 code16/GAP 反证）归 T330。
