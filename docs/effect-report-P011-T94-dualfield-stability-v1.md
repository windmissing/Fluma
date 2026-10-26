# P011/P013 T94 双场卡死修复后复测效果报告 v1

日期：2026-10-26。责任：effector。任务卡：`6adf1e5ee4b06ee357420099`。技术路线：SandCanvas 双场页（rendermode 4 + p011particle 1 + p011capture 1），基线 `52a0700`（T96 池优化 + T97 独立 7/7 PASS），HAP s146 `6350fc8b…`。  
状态：**PRODUCT_FAIL / FREEZE_REPRODUCED_IDENTICAL_STACK / T96_OPTIMIZATION_INSUFFICIENT_ON_DEVICE / HARD_GATE_TRIGGERED**。  
对比 T78（同栈卡死）：**栈完全相同，T96 优化未消除设备卡死**；额外澄清——卡死与 shake 无关，手势注入事务本身即可触发。

## 0. 环境与执行

| 项 | 值 |
|---|---|
| commit | `52a0700`（entry/src 与 c0c58c3 零差异，T97 仅加 test/；modules.abc sha256 `165f5868…` 与 s145 逐字节一致，代码等价已验） |
| 设备 / 窗口 | VYG-AL00 `62T0225B10010020`；19:19:41–19:23:25 |
| 执行 | `tools/t94_dualfield_stability.ps1` 五臂（R0 冒烟 / R1=T78 R1 序列 / R2=T78 R2 序列 / R3=T78 R3 序列+25s 落床 / R4 finish） |
| 产物 | `test/artifacts/t94-dualfield-stability-20261026/`（两份 appfreeze 堆栈、R3 完整 ledger 51.9MB、截图、dump） |

## 1. 主判：卡死三连复现，栈与 T78 逐帧一致

| 臂 | 输入 | 结果 |
|---|---|---|
| R0 | 无注入 15s | **存活**（零 freeze 关键词） |
| R1 | shake=1 + 3×swipe | **FREEZE**（19:20:30 THREAD_BLOCK_3S，swipe 1 后数秒） |
| R2 | shake=0.6 + 1×swipe | **FREEZE**（19:21:51） |
| R3 | shake=0 + 2×swipe | **FREEZE**（19:22:51）——**零摇晃也卡死** |
| R4 | finish | CLOSED 出现（records=264，R3 的 store），但**进程已死**，两文件拉取 0B（随后手动拉到 R3 ledger 51.9MB 前缀） |

**freeze 堆栈（两份 appfreeze 日志多帧采样，与 T78 完全同栈）**：
`mergeOldestHot (P011ParticleHot.ets:929:69 内层 j 循环) ← editPoolHot ← preparePatchHot ← applyInto (M2Sink) ← applyParticlePlanInto ← seed (M2Field) ← liftByShake (WaterSandField:6816) ← applyMotionChannels:4466 ← step`

## 2. 机理定稿：mergeOldestHot 设备端 O(N²) 超时

- **触发**：`liftByShake→seed`（shake lift 或手势注入事务）后 `editPoolHot` 调 `mergeOldestHot`；外层 `while` 找不到匹配时 `afterId=pId[a]` 继续，最坏 `nmax²` 次内层候选检查（capacity=192 → 36,864 次/调用；N512 满池 209 万次）。
- **T97 有界重放的边界暴露**：host Node 2.1s 完成 209 万检查——**设备 ArkTS 解释执行慢约一个数量级以上**，同负载超 3s 阈值 → THREAD_BLOCK。T97 §3"host 有限负载不能当作设备不冻结证明"的警告被本卡证实。
- **shake 无关性（新证据）**：R3 shake=0 且 0 收据（ledger sourceEpoch=0、264 行全 committed 无粒子）仍同栈卡死——**手势注入事务的 seed 路径本身就进 merge 全扫**；与 T92"shake-lift 池合并"假设相比，触发面更宽：**任何 seed（shake lift 或 gesture 注入）+ 池内活粒子即可能触发**。
- **准入 0 收据（同 T78 未解）**：R1/R3 `gesture_sand` 收据 0、ledger sourceEpoch=0——注入事务在 seed/merge 阶段卡死，收据从未产出；T82 干场页（EmptySandDemo，无 merge 路径竞争）正常，差异进一步收窄到 SandCanvas 双场页 seed→merge 链。

## 3. 判定表

| # | 判据 | 结果 |
|---|---|---|
| A1 三臂不冻结 | **FAIL**（3/3 臂 freeze，栈同 T78） |
| A2 真实准入 | **FAIL**（0 收据，注入事务被卡死吞没） |
| A3 可见沙 | FAIL（R1-R3 截图均为冻结前最后一帧或桌面） |
| B1 双流 CLOSED | **部分**：CLOSED 事件出现（R4，records=264 status=COMPLETE——T66 幂等收尾在进程死后仍发出），但两文件拉取需绕权限（已手动补拉 ledger 51.9MB；admissions 0B=0 准入一致） |
| C1 落床序列 | 无粒子可判（准入 0） |
| D1 code5 | 未再现（本卡先卡死于 merge，未到规划阶段） |
| E1 帧健康 | 冻结前正常（R3 23 条 WS_METRIC 覆盖 10s，deposited 770→928 基线沉降平滑） |

**综合：NOT PASS（硬门槛触发）**。T96 离线等价正确（T97 7/7），但优化只缩常数、未消 O(N²)，设备端不达标。

## 4. 移交（developer）

1. **mergeOldestHot 外层 while 的复杂度必须消除**（不是再缩常数）：候选按 `(pPhase,pSource,pSourceId,floor(pX))` 分桶或索引化，或 merge 改为批量一次性配对（排序后线性扫描）；目标=单次调用 O(nmax log nmax) 以内。
2. **手势注入事务路径同样要走该 merge**——修复合入后须同时覆盖 shake lift 与 gesture seed 两入口（R3 证据）。
3. **T97 重放方法论修正建议**：后续有界重放应乘设备/host 速度系数（或直接在设备上跑有界子步计数），host 秒数不具设备外推力。
4. T57 继续阻塞（同页）；T95 阻塞（T109 线独立）；T78 保持 FAIL 不改判。

## 5. 边界声明

- 产品代码零修改；两份 appfreeze 原始堆栈与 R3 ledger（51.9MB，264 子步全 committed 守恒 true）已入库可复核。
- 滴答 T94 卡评论已发；清单行状态留 manager。
