# P011/P013 T129 精确索引设备稳定性复测效果报告 v1

日期：2026-10-27。责任：effector。任务卡：`6adf424be4b0e6ce82cdad2c`。技术路线：SandCanvas 双场页（rendermode 4 + p011particle 1 + p011capture 1），基线 `a2e47d0`（T123 精确索引 `469ee37` + T128 差分 6/6/287 样本），HAP s148 `4c4aa66b…`。  
状态：**PRODUCT_FAIL / FREEZE_REPRODUCED_2_ARMS / FREEZE_POINT_MIGRATED_TO_AUDIT_SORT / SHAKE_ONLY_TRIGGER / HARD_GATE_TRIGGERED**。  
对比 T94（merge O(N²) 卡死）：**T123 索引已生效（mergeOldestHot 不再出现在卡死栈中），但卡死点转移到 `finishStepHot` 的 audit 排序链（`siftDown/sortOrderHot`）**；且触发条件比 T94 更早——**纯 shake=1 冷启后数秒、无需 swipe**。

## 0. 环境与执行

| 项 | 值 |
|---|---|
| commit | `a2e47d0`（T123 索引 + T133 + T128 差分；entry/src 树指纹 `f8832885f6ed7696`） |
| HAP | `entry-default-s148-signed.hap`，SHA256 `4c4aa66bee8e310ea774f9c33bb56983eb4fc275afe824b61566ca977bb8917e` |
| 设备 / 窗口 | VYG-AL00 `62T0225B10010020`；07:21:00–07:24:49 |
| 执行 | `tools/t129_index_stability.ps1` 五臂（T94 同序列）；**freeze 判读按 T124 勘误采用 PID 隔离** |
| 产物 | `test/artifacts/t129-index-stability-20261027/`（两份 appfreeze 堆栈、五臂 dump、截图、console） |

## 1. 主判：两臂复发，栈显示卡死点已迁移

| 臂 | 输入 | PID | 结果 |
|---|---|---|---|
| R0 | 无注入 15s | 30471 | 存活（0 freeze） |
| R1 | shake=1（+3×swipe 未及发出） | 31390 | **FREEZE @冷启后 4.8s（07:21:48.853），早于 swipe1（07:21:52）** |
| R2 | shake=0.6（+1×swipe 未及发出） | 32663 | **FREEZE @init 后 12.8s（07:23:07.010），早于 swipe** |
| R3 | shake=0 + 2×swipe + 30s | 32755 | **存活**（1460 ws 行、0 freeze、全程跑完） |
| R4 | finish | — | CLOSED 出现（records=264，R3 的 store；与前两轮一致），两文件拉取 0B（脚本 Get-Item 时序缺陷，见 §4） |

**R1 freeze 堆栈（PID 31390 匹配）**：
```text
siftDown (P011ParticleHot.ets:662)
  ← sortOrderHot (:684)  ← auditParticleHot (:1099)
  ← finishStepHot (:598) ← seed (P011ParticleM2Field.ets:177)
  ← liftByShake (WaterSandField) ← applyMotionChannels ← step
```
**mergeOldestHot / P011MergeExactIndex 不在栈中**——T123 索引自身未卡死；卡死点转移到同一 seed 事务的 **finishStepHot→auditParticleHot→sortActive/sortOrderHot**（全池 heapsort）。

## 2. 机理观察（供 architect/developer 分流）

1. **触发条件收窄**：R1/R2 在 **shake=1 冷启后 4.8s/12.8s、swipe 之前**卡死——纯 shake lift 路径即可触发，与手势注入无关；R3（shake=0+2 swipe+30s）存活——与 T94 的"swipe 触发"表象相比，本卡证明 **shake 幅度>0 是触发必要条件之一**（R3 无 shake 存活）。
2. **堆排序本身 O(N log N)（N=192 约千次比较/子步）不应卡死**——嫌疑集中在：audit 链被**每子步反复执行**（seed→finishStepHot 每帧多次？λ 重试放大？）或 `siftDown` 在特定数据（如大量同 pId/同 Y 粒子）下退化；`sortActive` 的校验分支（return -1）与堆排序的交互也需查。栈中 `state=R`（运行态）而非锁等待，指向**计算量或死循环**而非死锁。
3. **T123 索引有效性正面证据**：freeze 栈无 merge 函数——T128 的 O(N log N) 上界在设备上成立；旧卡死点确已被索引消除。
4. **R4 CLOSED 正常**（records=264 status=COMPLETE，R3 store）——T66 收尾在本卡存活臂上再次工作；两文件拉取失败是脚本在 CLOSED 后未等待即 Get-Item 的时序缺陷（R3 ledger 仍在设备 cache，如需可补拉），非产品问题。

## 3. 判定表

| # | 判据 | 结果 |
|---|---|---|
| A1 三臂不冻结 | **FAIL**（R1/R2 freeze，shake-only 触发；R3 存活） |
| A2 真实准入 | R3 存活但 ledger 待补拉；R1/R2 进程死前收据状态未及判读 |
| A3 可见沙 | R1/R2 冻结前帧未及出有效证据；R3 截图 3 张待人工审看 |
| B1 双流 CLOSED | CLOSED 事件出现（R3 store, 264/0 COMPLETE）；文件补拉待做 |
| D1 code5 | 未再现（先卡死于 audit 排序） |
| E1 帧健康 | 冻结前正常 |

**综合：NOT PASS（硬门槛触发）**。

## 4. 移交（developer/architect）

1. **卡死点已迁移**：`finishStepHot→auditParticleHot→sortOrderHot/siftDown`（P011ParticleHot:662/684/1099/598）——排查方向：①audit 链的每子步调用次数（λ 重试/preview 放大是否让它每帧多次全池排序）；②heapsort 在重复键（同 pId 残留/同 Y）下的行为；③`sortActive` 校验失败路径与 finishStepHot 的重试交互。
2. **shake=1 是触发必要条件**（本卡 R3 对照）——与 T94"swipe 触发"结论合并后，触发矩阵为：shake>0 或 gesture seed 任一 + 池活粒子；修复须同时压测两入口。
3. T97/T128 的 host 有界重放需补 audit 排序链负载（此前只覆盖 merge），否则同类盲区会重现。
4. T57 继续阻塞；T95 阻塞不变（T146 FIELD_COLUMN 线独立）。

## 5. 边界声明

- 产品代码零修改；s148 隔离构建（CompileArkTS 已跑，树指纹 `f8832885f6ed7696`）。
- 两份 appfreeze 原始堆栈已入库；R1/R2 freeze 判读经 PID 隔离（31390/32663 与 freeze log pid 逐一对上，执行 T124 勘误教训）。
- 滴答 T129 卡评论已发；清单行状态留 manager。
