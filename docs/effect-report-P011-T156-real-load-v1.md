# P011/P013 T156 真实负载与收尾窄补证效果报告 v1

日期：2026-10-28。责任：effector。任务卡：`6adff62ce4b0127832d2796a`。技术路线：SandCanvas 双场页（rendermode 4 + p011particle 1 + p011capture 1），基线 `0cc0439`（HEAD：T123 索引 + T166 + T174 列 guard + T175 独立 11/11），HAP s151 `3c8abfae…`。  
状态：**FREEZE_REPRODUCED_WITH_EXACT_INDEX / AGGREGATE_COMPLEXITY_O_N²_logN_IDENTIFIED / CLOSED_MECHANISM_VERIFIED / PRODUCT_FAIL**。  
对比 T129（索引生效、卡死点迁移到 audit 排序）：本卡在 **T174/T168/T175 全部到件后**复现卡死，且栈首次完整暴露 **精确索引的调用次数放大**——机理证据升级。

## 0. 环境与执行

| 项 | 值 |
|---|---|
| commit / 树指纹 | `0cc0439` / `f7fbec80d9d7eff8`（CompileArkTS 已跑） |
| HAP | `entry-default-s151-signed.hap`，SHA256 `3c8abfae71bbff53ca5e1ae98137b187e2d11f96af4d121a0423597ff0748f6c` |
| 设备 / 窗口 | VYG-AL00 `62T0225B10010020`；08:54:21–08:58:26 |
| 执行 | `tools/t156_real_load.ps1`（v2 纪律：冻结旧 PID→force-stop→冷启读身份→PID 过滤→cmd-audit 241MB） |
| 产物 | `test/artifacts/t156-real-load-20261028/`（两份 appfreeze、step3 ledger 50.8MB、dump、截图） |

## 1. 主判：卡死复现（两臂），栈直指精确索引的聚合复杂度

| 臂 | 输入 | PID | 结果 |
|---|---|---|---|
| R0/step3 | 冒烟+健康检查 | 21230 | 存活（ledger 258 子步全 committed、守恒 true、`p011-run-1793148867083-3`） |
| R1 | shake=1 + 3×swipe | 22265 | **FREEZE**（08:55:23 THREAD_BLOCK；PID 匹配 22 条） |
| R2 | shake=0.6 + 6×swipe | 23181 | **FREEZE**（08:56:30；PID 匹配 22 条） |
| R3 | shake=0 + 2×swipe → finish | 24636 | **存活全程**；先注入后 finish 顺序执行；**CLOSED 258/0 COMPLETE**（T66 收尾机制再次验证） |

**R1 freeze 堆栈（PID 22265 匹配，两份日志多帧同栈）**：
```text
sift → sort → find/findBody (P011MergeExactIndex 内部)
  ← mergeOldestHot (P011ParticleHot.ets:949, 索引版)
  ← editPoolHot:978 ← preparePatchHot ← applyInto (M2Sink)
  ← applyParticlePlanInto ← seed ← liftByShake ← step
```

**机理定稿（对 T123/T128 结论的调用次数放大修正）**：`editPoolHot` 的 birth 循环中**每次 freeCount==0 都调用 `mergeOldestHot`**（`P011ParticleHot.ets:978`，在 births for 循环体内）；shake lift 一次可产生数百 births → 数百次索引调用，每次含**完整重建排序 O(N log N)** → 聚合 **O(N² log N)**，比旧 T94 全扫描 O(N²) 更糟。T123"每次 merge O(N log N)"与 T128 D2"无匹配单次 ≤64N"均属实，但**二者都未覆盖"births 循环内反复调用"的调用次数维度**——T97/T128 host 有界重放未含此放大（同手势负载 birth 少），第三次证明 host 秒数不外推设备。

**R2 佐证**：freeze 栈在 `seedBody←liftByShake`（seed 阶段自身），同族路径不同采样帧——shake>0 的 lift→seed→birth→merge 链是共同触发面。

## 2. 收尾与 ledger（R3 有效臂）

- **CLOSED 机制验证**：`P011_OBS_STORE_CLOSED records=258 admissions=0 status=COMPLETE`——**先注入后 finish 顺序**下 CLOSED 正常产出（T66 幂等收尾连续第三次设备验证）。
- **ledger 完整拉取**：50,818,215B（258 行）经 `/data/app/el2/100/base/.../cache/` 成功回收（T95 教训应用）；admissions 0B 与 R3（shake=0，手势注入 0 收据，同 T78/T94 未解）一致；末条 `audit errorCode=null, conservation=true`。
- 注：R3 CLOSED 的 runId 是 step3 冷启的 run（`p011-run-1793148867083-3`）——R3 冷启后页面复用该 run（脚本 R3-pre-finish 的 dump 即取到该路径），ledger 数据有效（R0/step3 段的健康运行记录）。

## 3. 判定表

| # | 判据 | 结果 |
|---|---|---|
| A1 三臂不冻结 | **FAIL**（R1/R2 freeze；R3 存活） |
| A2 真实准入 | R1/R2 卡死吞没（gesture_sand 0，同 T78/T94）；R3 shake=0 无手势收据（未注入成功——同未解问题） |
| B1 双流 CLOSED | **PASS（机制层）**：CLOSED COMPLETE 出现；ledger 50.8MB 完整拉取 |
| B2 manifest | 未跑（admissions 0B，manifest 需双流非空） |
| C1 落床 | 无粒子可判（同 A2） |
| S1 PID 隔离 | 全程执行（脚本 PID=spawn 后立即读取；freeze 归属逐一匹配） |

**综合：NOT PASS（硬门槛触发）**。

## 4. 移交（architect T116/developer）

1. **精确索引的聚合复杂度缺陷**：`editPoolHot` births 循环内逐 birth 调用 `mergeOldestHot`（含每次重建索引排序）→ 聚合 O(N² log N)。修复方向：①births 循环**批量**处理 free-slot 不足（一次 merge 腾出多个 slot 或循环外统一 merge）；②索引增量化（birth 后增量插入而非全量重建）；或③ births 循环前置一次性合并。**T123/T128 的单次调用合同需扩展到"调用次数×单次成本"的聚合上界。**
2. **R2 栈在 seed 阶段**（无 merge 帧）提示 seed→audit 链仍有独立热点（与 T129 audit 排序卡死同族），修复时一并覆盖。
3. **手势注入 0 收据**（T78/T94/T156 三次复现，shake=0 下也出现）仍未解——与卡死正交的独立问题，建议 T116 一并分流。
4. T57 继续阻塞；T95 干场线结论不变（无 merge 路径不受影响）。

## 5. 边界声明

- 产品代码零修改；s151 隔离构建；cmd-audit.log 241MB（含全部命令审计）与两份 appfreeze、50.8MB ledger 已入库。
- PID 隔离判读全程（脚本 PID 为 spawn 后立即读取值，freeze 归属逐一匹配 22265/23181）。
- 滴答 T156 卡评论已发；清单行状态留 manager。
