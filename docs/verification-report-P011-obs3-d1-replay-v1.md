# P-011 观测③ D1 离线资格重放核验报告 v1

- **日期**：2026-10-22；责任：tester（1A 协同 architect）。对应任务【观测③·D1离线核验】（滴答 6ad8d38d）。
- **依据**：design-P011-obs3-calibration-data-path-v1.md §3.1（五项实现偏差）+ §4.1（1A 离线窗口伪代码）；manager D0 批准的数据路径方案（1A 优先）。
- **性质**：离线重放审计，零真机成本。**不产出阈值估计，不解封 holdout 残差，不改变观测③不足状态。**
- **输入**：
  - pilot v4 ledger：`test/artifacts/p011-v3pilot4-20261021/run-173859/ledger-trimmed.ndjson`（6780 条，公开可复用）
  - 扩批 34 runs：`test/artifacts/p011-v3expansion-20261021/p011-v3expansion-run-001..034/ledger-trimmed.ndjson`（各 ~4600 条）
  - schedule/role manifest：`calibration/randomization-manifest-obs3v3-expansion-v1.json`（fit=排列前 24 / holdout=后 10，冻结）
- **工具**：`test/p011_obs3_d1_eligibility_replay.py`（本次新写）+ `test/run_p011_d1_replay_34runs.py`（批处理）；SHA256 见 §6。

## 1. 逐端资格重放实现与五项偏差修复（任务步骤 1.1–1.2）

重放按 §4.1 伪代码逐端实现：每固定边保留紧前两条 raw decisions（含 invalid attempts 的 decision）、连续已提交子步、同预注册 segment、两端非零同符号 accepted、两端 planner/clip clear、timing clear（实际接受间隔，drift 仅遥测）、ramp 三点单调、执行阈值跨越、宽 ≤0.5°、`(run,segment,edge)` 首事件去重。五项偏差修复与验证：

| # | 偏差（designer §3.1） | 修复 | 修复后验证 |
|---|---|---|---|
| FIX-1 | 前端点被强制 stable（`prior_state['stable']`） | ramp bracket 前端点改为"同 segment 已提交 decision"要求，不再要求平台 | pilot stop/+ 16 笔旧码 `PREV_ENDPOINT_NOT_ELIGIBLE` 全部消除，替换为真实原因（§2） |
| FIX-2 | planner/clip clear 只查当前端点；helper 读 `guardClipped` 而 ledger 实际是 `hardGuardClipped` | 两端各自独立查 clear；字段名对齐 ledger（`hardGuardClipped/hardGuardRejected`）；req=0&granted=0 仍按真实 stop 不算限流 | pilot stop/- 27 笔中 22 笔现报 `PREV_BUDGET_GUARD_OR_STOCK_LIMITED`（designer §3.2 前端点 requested>granted 22 笔逐位一致） |
| FIX-3 | ramp cell 按 leg 符号归属但未逐端核验非零 accepted 符号 | 两端 accepted 符号必须等于 cell 符号且非零；零角=neutral | pilot 唯一 summary-eligible bracket（c=125 L5 substep 2522）被正确排除：前端点 2521 accepted=0 → `PREV_ENDPOINT_SIGN_NEUTRAL_OR_MISMATCH`，与 designer §3.3 结论一致 |
| FIX-4 | `perRunTransitions` 在资格判定前累加（状态/事件分母混用） | cap 只作用于状态样本计数；转换资格对每个真实转换评估，不因前序不合格事件耗尽额度 | 34 runs 重放分母=全部真实转换（start/+ 583 vs 旧 260——旧实现把 ramp/H2/H6 start 转换在 cap 下吞掉） |
| FIX-5 | PlatformWindow 只识别 0/±15/±30；Pass 1 把 epoch-ms 的 physicsTime 再乘 1000 | 平台名单加注册峰值 ±A（18.7）；时间归一化改为 \|t\|>1e10 视为 ms 不再缩放 | ±18.7 平台记录（102/100 条）现可入平台窗口；稳定窗口时长恢复真实 200ms 语义 |

修复同时消除误排与漏检：旧码 `PREV_ENDPOINT_NOT_ELIGIBLE` 的误排消除；新增 `PREV_ENDPOINT_SIGN_NEUTRAL_OR_MISMATCH`（漏检方向，designer §3.3 指出的零角端点类）。三点单调/≤0.5°/限制排除/零点隔离/安全门槛全部未动。

## 2. pilot 43 stop 逐笔判定表（任务步骤 1.3，与 designer §3.2 对照）

重放输出 43 行 stop 判定（stop/+ 16、stop/- 27），逐笔含三点 alpha/前端点 requested/granted/排除原因（`test/reports/d1_replay_pilot_v1.json` reasonRows）。聚合对照：

| 方向 | 笔数 | 三点非单调 | 前端点 requested>granted | 前端点位于 ramp | designer §3.2 |
|---|---:|---:|---:|---:|---|
| stop/+ | 16 | 16 | 16 | 16 | 16/16/16/16 ✅ |
| stop/- | 27 | 26 | 22 | 27 | 27/26/22/27 ✅ |

designer §3.2 三个可复查窗口逐位复现：

| 窗口 | designer 三点 alpha | 重放判定 | 一致性 |
|---|---|---|---|
| c=126 L2 1356/1357/1358 | 20.012237→20.041189→19.963696 | NONMONOTONE + PREV_CLIP（宽 0.077<0.5） | ✅（1358 是真实阈值 stop，q=0，未误标限流） |
| c=0 L5 2573/2574/2575 | 20.000643→20.044813→19.934257 | 仅 NONMONOTONE（前端点无裁剪） | ✅ |
| c=125 L5 2994/2995/2996 | 20.160531→20.134613→18.570048 | BRACKET_TOO_WIDE（1.56457°>0.5）+ PREV_CLIP | ✅（宽 1.564565 逐位一致） |

**结论（与 designer §3.2 相同）**：pilot 43 次 stop 在修复后资格链下仍全部无法入池（42 非单调 + 1 过宽），旧 checker 的 `PREV_ENDPOINT_NOT_ELIGIBLE` 是误排原因标签，不是真实资格障碍——真实障碍是三点非单调与限流，与"修复误排不自动产生合格 stop"的设计判断一致。

## 3. 扩批 34 run 资格重放差异表（任务步骤 2）

旧 checker 结果（34 份 `v3-summary.json` 聚合）保留；修复后重放（`test/reports/d1_replay_diff_summary.json`）：

| cell | 旧 real | 旧 elig | 新 raw | 新 elig（修复后） | 新 elig 中的首事件 bracket |
|---|---:|---:|---:|---:|---:|
| start/+ | 260 | 0 | 583 | 0 | 0 |
| start/- | 651 | 14 | 889 | 11 | 11（11 个不同 run 各 1） |
| stop/+ | 549 | 0 | 549 | 0 | 0 |
| stop/- | 903 | 0 | 903 | 0 | 0 |

### 3.1 分母修正（FIX-4 + FIX-1 生效）

- start/+ realTransitions 260→583：旧实现 cap（每 run 3）在资格判定前吞掉了 323 个 H2 平台 start 转换；修复后全部进入分母并逐笔评估。stop 两向分母不变（549/903——旧实现这些转换本就未被 cap 吞掉）。
- start/- 651→889：ramp L5/L6/L7/L8 start 转换进入分母。

### 3.2 逐原因交集表（原因可重叠，**不能相加还原事件数**）

34 runs 全部转换按命中原因集合分组（`∩`=同时命中）：

**start/+（583 笔，0 合格）**
| 原因集合 | 笔数 |
|---|---:|
| BUDGET_GUARD_OR_STOCK_LIMITED | 411 |
| BRACKET_TOO_WIDE ∩ BUDGET | 170 |
| BUDGET ∩ NONMONOTONE | 2 |

**start/-（889 笔，11 合格）**
| 原因集合 | 笔数 |
|---|---:|
| BUDGET_GUARD_OR_STOCK_LIMITED | 822 |
| BRACKET_TOO_WIDE ∩ BUDGET | 33 |
| BUDGET ∩ NONMONOTONE | 12 |
| （合格，无原因） | 11 |
| PREV_ENDPOINT_SIGN_NEUTRAL_OR_MISMATCH（仅此一项） | 10 |
| BUDGET ∩ PREV_NOT_SAME_SEGMENT | 1 |

**stop/+（549 笔，0 合格）**
| 原因集合 | 笔数 |
|---|---:|
| NONMONOTONE ∩ PREV_BUDGET | 531 |
| PREV_BUDGET 仅此一项 | 17 |
| NONMONOTONE ∩ PREV_BUDGET ∩ TIMING | 1 |

**stop/-（903 笔，0 合格）**
| 原因集合 | 笔数 |
|---|---:|
| NONMONOTONE ∩ PREV_BUDGET | 697 |
| NONMONOTONE 仅此一项 | 158 |
| BRACKET_TOO_WIDE ∩ PREV_BUDGET | 24 |
| TOO_WIDE ∩ NONMONOTONE ∩ PREV_BUDGET | 22 |
| NONMONOTONE ∩ PREV_BUDGET ∩ TIMING | 2 |

### 3.3 start/- 14→11 的逐笔差异（旧 checker vs 修复后）

以 `(run, segment, edge, substep)` 为键对照旧 `firstBrackets` 与新合格事件：

- **保留（交集）4 笔**：run-006/025/028/030（L5 edge 125，substep 2543/2528/2527/2527）。
- **剔除 10 笔**：run-003/011/014/015/021/024/026/027/033/034——全部是 **L5 leg 起步首个子步、前端点 accepted=0**（逐笔抽查证实：run-003 substep 2519 accepted=0 → 2520 start）。旧 checker 按 leg 符号归属 cell 未核验端点符号，把 neutral 前端点当作 bracket 端点（designer §3.3 预言的类别，10/10 全部命中该类）。
- **新增 7 笔**：run-002/005/016/018/022/029/032——旧实现被 `PREV_ENDPOINT_NOT_ELIGIBLE`（前端点强制 stable）误排，修复后凭合法 ramp 前端点入池（逐笔抽查证实 run-002 substep 2516 accepted=-0.05 非零、无裁剪）。
- 净变化 14−10+7=11 ✅。

### 3.4 timing 闸门影响

实际接受间隔闸门命中转换：stop/+ 1 笔、stop/- 2 笔、start/- 0 笔（与旧 checker 完全一致——timing 判据未动，命中命令 run 级 breach map 与 drift-consumption ruling 的口径一致）。scheduleDriftMs 仅遥测，无失格。

### 3.5 分组覆盖 QA（只给 designer 计数，holdout 残差未封存解封）

按冻结随机化排列（前 24 fit / 后 10 holdout）：

| 组 | cell | raw | 合格 | 有合格事件的 run 数 |
|---|---|---:|---:|---:|
| fit | start/+ | 410 | 0 | 0 |
| fit | start/- | 628 | 9 | 9 |
| fit | stop/+ | 386 | 0 | 0 |
| fit | stop/- | 636 | 0 | 0 |
| holdout | start/+ | 173 | 0 | 0 |
| holdout | start/- | 261 | 2 | 2 |
| holdout | stop/+ | 163 | 0 | 0 |
| holdout | stop/- | 267 | 0 | 0 |

对 §6 统计门槛的缺额（每方向/type ≥20 合格事件、≥5 run）：修复后 start/- 全批 11（fit 9 + holdout 2）——仍缺 9（且需 ≥5 fit run，现 9 达标；holdout 需独立 ≥20，现 2）；start/+ 与 stop/± 均为 0（各缺 20）。**本表仅为覆盖计数，未计算任何残差/阈值/CI。**

## 4. QA 结论

1. 五项偏差修复后，34 runs 的合格事件总数 14→11，**净减少**——旧 14 中 10 笔属零角前端点误入池（漏检方向修复），7 笔新事件来自 ramp 前端点误排消除（误排方向修复）。两个方向同时落实，无"只删不利规则"。
2. stop 两向在修复后依旧 0 合格，主导原因=三点非单调（1409/1452 笔命中）+前端点限流（PREV_BUDGET），与 designer §3.2 pilot 结论同构——**资格修复不改变"当前激励结构下 stop 无合格 bracket"的事实**，观测③不足状态维持。
3. timing 命中极少（3 笔转换）；非缺陷源。
4. 资料完整性：34/34 ledger+manifest+旧 summary 全部在档，无缺失字段阻断（edge 身份全走 interfaceIndex，donor/receiver 缺省符合 schema v3）。
5. 无资料补造；所有数字可由归档 ledger + 本报告工具复现。

## 5. 移交

- **designer**：§3.2 交集表 + §3.5 分组覆盖计数（fit/holdout 各自缺额），用于 1B 影子审计设计或路径裁决；holdout 残差继续封存（本报告未读取任何 holdout 拟合信息）。
- **manager**：确认 §3.1 偏差修复验收；观测③ `insufficientEvidence` 状态不变；是否进入 1B/路径 4 属下一步裁决。
- **工具**：`test/p011_obs3_d1_eligibility_replay.py`（重放器，`--ledger --manifest --old-summary --output [--no-details]`）；`test/run_p011_d1_replay_34runs.py`（34 run 批处理+差异汇总）。同一路径可服务后续 shadow 审计的资格函数（同资格函数复用）。

## 6. 制品与版本

| 制品 | SHA256（前 16 位） |
|---|---|
| `test/p011_obs3_d1_eligibility_replay.py` | 见 commit 记录 |
| `test/reports/d1_replay_pilot_v1.json`（pilot 重放+43 stop 明细） | — |
| `test/reports/d1_replay_diff_summary.json`（34 run 新旧差异汇总） | — |
| `test/reports/d1_replay_p011-v3expansion-run-NNN.json`（34 份无明细重放） | — |
| `test/reports/d1_replay_detail_p011-v3expansion-run-NNN.json`（34 份含明细重放） | — |
| 输入 checker（修复前基线）`test/p011_obs3_v3_protocol_check.py` SHA256 `521fc01872756b4c…`（designer §1 登记，未改动该文件） | — |

**红线自查**：未放宽任何门槛（三点单调/≤0.5°/限制排除/零点隔离/安全闸门全部原样）；未删不利规则（PREV 类原因全部保留并新增更严的端点符号核验）；误排与漏检同向消除；未产阈值估计；未解封 holdout 残差；离线零真机。
