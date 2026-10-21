# P-011 观测③ 扩批前语义裁决 v1（ramp cell 归属 + H8 段尾截断）

- **日期**：2026-10-21；责任：tester。**扩批 34 runs（6ad62e0b）的硬前置**。
- **数据源**：pilot v4 clean run `p011-run-1792575546108-3`（`test/artifacts/p011-v3pilot4-20261021/run-173859/ledger-trimmed.ndjson`，6780 条：platform 3660 / ramp 3120；schedule A=18.7°、v=0.5°/s、理论时长 195.5s、实测跨度 240.16s）。
- **依据**：协议 §5.2 bracket 资格原文、§3 leg 预注册表、architecture §26.4；pilot-v4 报告 §3 两项移交。

## 裁决 1：ramp 内转换的 cell 归属

### 1.1 裁定：ramp leg 是 eligible segment；cell_sign 按该 leg 的预注册符号归属

**① ramp leg 是否 eligible？——是。** 协议 §5.2 bracket 资格第二行原文："same preregistered **ramp leg** OR same eligible stable-platform segment"——ramp leg 与稳定平台段是**并列的合格 segment 类型**，且 §5.1 明示"合法、完整提交的正常受控 ramp 允许 rate 非零"。checker 注释"v3 allows ramp legs as eligible segments"是正确意图，但实现中 `cell_sign = state["sign"] if state["stable"] else None` 把非 stable 平台的 ramp 段全部置 None，`if cell_sign is None: continue` 在**排除计数之前**跳出——70 个 ramp 转换未归属四 cell 也未留排除原因。**裁定为实现缺陷**（与影子审计 SHADOW_COVERED 基于 ramp leg bracket 系统性分歧），非有意保守。

**② cell_sign 归属规则**：ramp 段内转换按**该 leg 的预注册符号**归属 ±cell，符号来源=预注册 schedule binding（协议 §3.1 leg 表：L1..L4=正程 `+`，L5..L8=负程 `−`；A≤29.9<30 保证 leg 内符号恒定）。**不用实测 accepted 符号**（避免 leg 内跨零点附近抖动导致同 leg 转换分裂进两个 cell），实测符号与预注册符号不符的记录按现有排除链处理（新码 `RAMP_SIGN_MISMATCH`，预留；v4 数据实测 L1..L4 全 +、L5..L8 全 −，零失配）。零角附近（L4/L8 回零段）accepted=0 的 neutral 转换仍归 neutral 单列，不入四 cell。

**③ 实现**：`cell_sign` 判定改为——stable 平台段维持现状（accepted 符号）；ramp 段取 `RAMP_LEG_SIGN[legId]`（L1/L2/L3/L4→`+`，L5/L6/L7/L8→`−`，由预注册常量冻结）；两者皆缺→None（neutral），且 ramp 缺 legId 的记录在排除计数中记 `SEGMENT_IDENTITY_MISSING`（不再静默 continue）。

### 1.2 v4 数据影响面

ramp leg 记录 3120 条（L1 708 / L2 148 / L3 144 / L4 606 / L5 608 / L6 148 / L7 146 / L8 612）；pilot-v4 报告 88 个真实转换中 70 个在 ramp 内（含影子审计主激励 L2/H2 start、L3/L7 stop）。修订后这些转换进入 realTransitions 分母并按 §5.2 全资格链评估——**扩批 bracket 产出路径打通**。

## 裁决 2：H8 段尾截断

### 2.1 裁定：checker 按播放器结束时刻截断；实现走 manifest 注册的 schedule 理论时长 + 最后命令 dispatch 时间

**背景**：播放器 `tick()` 派发最后一条命令后 stateValue→`'finished'`（P011SchedulePlayer.ets:281），但该转换**未落账到 ledger**（field 无 scheduleFinishedAtMs 字段）；segment 标记保持 H8，采集窗口尾部 44.66s 自由演化（240.16s 实测 − 195.5s 理论）计入 H8——2445 条 H8 记录中 2297 条在正式 hold 之外。

**实现选择**：**checker 侧判定，不加 ledger 字段**（零 schema 变更、不改播放器/field 代码）。判定方法：

1. `scheduleEndMs = firstCommandDispatchMs + schedule理论时长`，其中理论时长 `T = 9×5100ms + Σ(leg 命令数×100ms)`（由 manifest 的 scheduleSha256 对应的 CSV 或 A/v 参数计算：`T = 9×5.1 + 4×A/v` 秒）。
2. `firstCommandDispatchMs` 取 ledger 中 `commandSequence==1` 首条子步的 `physicsTime`（ms 纪元）。
3. `physicsTime > scheduleEndMs + 容差`（容差=末命令 dispatch 漂移上界，取 200ms 与 scheduleTiming P95 的较大者）的记录标记 `postSchedule=true`：**不参与状态样本、不参与 bracket、不参与平台窗口**；checker 输出 `postScheduleRecords` 计数（诊断保留，不删数据）。
4. 扩批 runner 侧同步改进（建议，非本裁决强制）：采集窗口长度从固定 240s 改为 `T + 5s` 收尾缓冲，从源头减少尾部数据。

**不选择 ledger 加标记的原因**：改 schema/播放器违反"checker 判定域"分工（§26.4 决策 2 同款理由）；理论时长由 manifest 冻结的 scheduleSha256 唯一确定，checker 可独立复算，无需运行时标记。

### 2.2 v4 数据影响面

截断后 H8 有效 hold 记录 ≈148 条（2445−2297），H8 stop×1（尾部转换）落入 postSchedule 区——**不进入估计**；截断不影响 L1..L7 与 H0..H7 数据。

## 附带核查：platformReachability overall=FAIL 是否阻断扩批

**不阻断，无需修订 schedule。** 依据：

1. 协议 §3.1 明确"A 严格小于 30°，由 §4 确定"——A=18.7° 时 ±30 平台**按设计不存在**；platformReachability 的 ±30 项=0 是 schedule 参数的直接后果，非执行缺陷。
2. checker 内 `platformReachability.note` 已声明"diagnostic only; v3 does not require platforms for start events"，且 `overall` 判定（`INVALID` 条件=硬闸门失败/协议元数据错误/身份失配）**不含 reachability**——pilot-v4 overall=PILOT_REPORT 正确。
3. **但 verdict 语义有歧义**：`reach_pass` 要求 ±15/±30 四项全非零，A<30 的 run 必然 FAIL——该 FAIL 与"诊断项"note 自相矛盾。修订：`reach_pass` 判定域改为**schedule 实际包含的平台**（由 A 值推导：A≥30 才要求 ±30 项），±30 缺项在 A<30 时记 `NOT_IN_SCHEDULE` 而非 FAIL。这不放宽任何闸门（±30 从不是 v3 start 事件的必要条件），只消除恒假判定。

## 交付与验证

- checker 修订：`test/p011_obs3_v3_protocol_check.py`（RAMP_LEG_SIGN 常量 + cell_sign 归属分支 + postSchedule 截断 + reach_pass 判定域修正）。
- 单测：ramp start/stop 归 ±cell、neutral 零角不入四 cell、缺 legId 记 SEGMENT_IDENTITY_MISSING、postSchedule 截断边界、A<30 时 ±30 记 NOT_IN_SCHEDULE。
- v4 数据复跑验证（修订后 ramp 转换进入分母、H8 尾部被截断）。
- **不自行放宽任何硬闸门/统计门槛**；两项裁决均为"实现对齐协议原文"性质。

## 复跑中发现并修复的第三个实现缺陷（edge 身份前置检查）

v4 数据复跑首轮 real=0，逐层排查发现**第三个缺陷**：Pass 2 的 edge 身份检查要求 `finite(donor) and finite(receiver)`，但 ledger schema v3 中 donor/receiver 为可选字段（v4 全部 88 个转换的 donor/receiver=None，仅带 interfaceIndex）——转换在 cell 归属之前被静默丢弃。修订：interfaceIndex 为主身份键（协议 §5.1"同一边用 interfaceIndex"）；donor/receiver 存在时才执行相邻性与方向性一致性校验。

## v4 数据复跑结果（修订后，`run-173859/v3-check-post-ruling.json`）

| cell | realTransitions | eligibleBrackets | capped | 主要排除 |
|---|---|---|---|---|
| start/+ | 8 | 0 | 5 | PREV_ENDPOINT_NOT_ELIGIBLE×8、BUDGET×8 |
| start/− | 20 | 1 | 17 | PREV_ENDPOINT×18、BUDGET×19、TOO_WIDE×1、SCHEDULE_INTERVAL×1 |
| stop/+ | 16 | 0 | 13 | PREV_ENDPOINT×16、NONMONOTONE×16、POST_SCHEDULE×1 |
| stop/− | 27 | 0 | 24 | PREV_ENDPOINT×27、NONMONOTONE×26、TOO_WIDE×1 |

- **realTransitions 0→71**（8+20+16+27；其余 17 个为 postSchedule/neutral 排除在案）——ramp/H2/H6 转换全部进入分母，"影子有覆盖、checker 全零"分歧消除。
- **postSchedule excluded=2316**（H8 尾部 44.66s 自由演化被截断，诊断保留）。
- eligibleBrackets=1（start/− 首个合格 bracket）——单 run 全资格链通过数低属预期（v4 是单 pilot，34 runs 扩批后按统计门槛累计）。
- platformReachability verdict **FAIL→PASS**（±30 记 NOT_IN_SCHEDULE，判定域修正生效）。
- 单测 **49/49**（新增裁决套件：ramp leg 符号归属、符号失配排除、postSchedule 块）；runner 回归 **79/79**。
