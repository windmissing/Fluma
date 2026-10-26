# P011/P051 T78 双场采集修复后复测效果报告 v1

日期：2026-10-26。责任：effector。任务卡：`6adeedcee4b0127832ab780e`。技术路线：SandCanvas 双场页（rendermode 4 + p011particle 1 + p011capture 1），基线 `114a7ee`（T56 scene + T66 收尾），HAP s144 `af049753…`。  
状态：**PRODUCT_FAIL / MAIN_THREAD_FREEZE_IN_PARTICLE_POOL_LIFT_MERGE / HARD_GATE_TRIGGERED / CAPTURE_CLOSED_UNREACHABLE**。  
对比上一版（T44 采集 BLOCKED：无可见沙、缺 CLOSED）：本轮**新增致命硬门槛——三臂注入操作全部触发主线程卡死**，CLOSED 与 manifest 不可达；双流收尾修复（T66）未能被验证。

## 0. 环境与执行

| 项 | 值 |
|---|---|
| commit / 树指纹 | `114a7ee` / `403b08514017b326`（与 T82 同包 s144） |
| 设备 / 窗口 | VYG-AL00 `62T0225B10010020`；16:19:36–16:22:46 |
| 执行 | `tools/t78_dualfield_retest.ps1`（R0 冒烟 / R1 前台按钮 finish / R2 Want finish / R3 落床时间序列） |
| 产物 | `test/artifacts/t78-dualfield-retest-20261026/`（六 dump、六截图、两份 appfreeze 堆栈、console log） |

## 1. 主判：主线程卡死（硬门槛，一票否决）

三臂（R1/R2/R3）在 **SandCanvas 双场页 + pan 注入（swipe）** 后全部发生 `THREAD_BLOCK_3S`→`THREAD_BLOCK_6S`→`APP_INPUT_BLOCK`，进程被系统终止（R1-settled 截图已是桌面）：

| 臂 | 输入 | 卡死时点 | 证据 |
|---|---|---|---|
| R1 | shake=1 + 3×swipe | 首次 swipe 后（16:20:15 shake_lift 之后，16:20:21 NotifyAppFault） | WS 行止于 16:20:15；appfreeze log `…162023555.log` |
| R2 | shake=0.6 + 1×swipe | swipe 后（16:21:20 最后 WS 行 → freeze） | appfreeze log `…162128662.log` |
| R3 | shake=0 + 2×swipe | 第二次 swipe 后（WS_METRIC 止于 16:22:02，faultlog @16:22:29） | 23 条 metric 在卡死前正常产出 |

**卡死堆栈（appfreeze 两份日志，多帧采样同栈）**：

```text
mergeOldestHot / collectParticlePlanInto (P011ParticleHot.ets:908/718)
  ← editPoolHot / preparePatchHot / applyInto (P011ParticleM2Sink.ets:230/427)
  ← applyParticlePlanInto ← seed (P011ParticleM2Field.ets:88/99/128)
  ← liftByShake (WaterSandField.ets:6816/6743/6822)
  ← applyMotionChannels (WaterSandField.ets:4466)
  ← stepWithTiltTrace ← step ← frame (SandCanvas.ets:2179) ← onFrame
```

定性：**双场 step 的 shake-lift 粒子池路径（lift→seed→pool MERGE/collect）在主线程死循环或超长计算**。R3 shake=0 也同栈卡死（liftByShake 由 applyMotionChannels 调用，疑似未按 shake 幅度短路，或粒子池 merge 与注入事务组合触发）。T82 的 EmptySandDemo 干场页（同包同构建）大量 step 无卡死——差异在 SandCanvas 双场 route + shake lift 通道 + 128 列高度场组合。

## 2. 附带发现：pan 注入 0 准入（卡死前）

R3 卡死前收到 503 条 `pan_update`（429 条 accepted=true，gate 通过），但 `gesture_sand` 收据事件 0 条、`sourceEpoch=0`——注入事务从未产出收据（或 sink 未达）。R1/R2 同样 0 条。此点**不能**归因为 routeGestureSand 未执行（accepted=true 证明进入）；与卡死同源于双场路径异常，合并移交，不单独定论。

## 3. 未达成项（因卡死不可达）

- 双流 CLOSED（前台按钮与 Want finish 两路均未走到——R1 click finish 时进程已死，R2 finish Want 发给已死进程）。
- manifest COMPLETE 校验（两文件未产出）。
- 可见沙判定：R1-shaking/R2-settled/R3 截图均在卡死或桌面态，无有效帧证据。
- 落床时间序列：R3 的 23 条 WS_METRIC 只覆盖基线高度场沉降（deposited 770→928 增长），粒子注入未发生，序列对"粒子落床"无判据力。

## 4. 有效正证据（卡死前）

- 双场页可达：`particle_route {"ok":true,"active":true}`（三臂一致）；`P011_OBS_STORE` 正常产出（T66 身份 `P011_COMMIT_IDENTITY_V2`、storeGeneration=1、两文件路径齐备）；`p013_scene` 事件 `state=CURRENT, swap=true, staleSwap=false`（T56 scene 帧提交回执正常）。
- 基线高度场沉降正常（R3 deposited 770→928，suspensionFraction 0.85→0.82 平滑收敛，drift 有限）。
- R0 冒烟臂（无注入）12s 完全正常。

## 5. 判定与移交

| 项 | 判定 |
|---|---|
| 双场产品可见沙+双流 CLOSED（T78 主目标） | **FAIL（硬门槛：主线程卡死三连发）** |
| T66 收尾修复验证 | UNPROVEN（进程死亡先于 finish） |
| 卡死根因 | 交 **developer 归因修复**（P011ParticleHot mergeOldestHot/collectParticlePlanInto 死循环嫌疑 + liftByShake 在 shake=0 的短路缺失嫌疑；两嫌疑均列） |
| pan 注入 0 收据 | 与卡死合并移交，不单独归因 |

**建议 manager**：T78 结果回执交归因（新建或复用 developer 卡），修复完成前 T57（G5 真机双测）不应开工——同页同操作会稳定复现卡死；T82 已验证的 EmptySandDemo 干场线不受此阻塞。

## 6. 边界声明

- 产品代码零修改；两份 appfreeze 原始堆栈已入库可复核。
- 主观分档不适用（硬门槛触发，一票否决；不计 0 分凑平均，直接 NOT PASS）。
- 滴答评论与完成登记留 manager；清单 T78 行状态未改（仍"未开始"，接单登记已超出本轮有限授权范围，交 manager 补登）。
