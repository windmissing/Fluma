# T65 T44 失败客观补证 v1

日期：2026-10-26。责任：tester。任务：[6adea6ec…712c](https://dida365.com/webapp/#p/6ac8a7e3e9adb60000000419/tasks/6adea6ece4b017ab33f5712c)。状态：**EVIDENCE_ONLY / NOT_A_FIX / ROOT_CAUSE_BELONGS_TO_T64 / DEVICE_FACTS_PARTIALLY_ABSENT**。

不使用真机；不改产品、判据。消费 T44 归档原始数据（`test/artifacts/t44-dualfield-product-run-20261026/`：R0/R1 双份 NDJSON、admissions、actions、日志）与隔离版本身份（HEAD `d941e7f`、tree `7836d441`，T44 报告§身份），离线复算唯一键、重复 sequence、准入累加、route 来源与生命周期计数；总体根因由 T64 负责。

## 0. 证据文件

| 文件 | 内容 |
|---|---|
| `test/reports/p065_t44_evidence.json` | NDJSON/admissions 结构化复算结果（本稿各表数字的唯一来源） |
| `test/p065_store_lifecycle_units.cjs` + `test/reports/p065_store_lifecycle_units.json` | 真实 FileStore（未改字节）+ worker 协议桩生命周期夹具，4/4 PASS |
| T44 归档原始文件 | 全部保留未去重；复算只读 |

## 1. 唯一键复算（F44-IDENTITY 第一层）

R1 最终流 283 条记录：

| 键 | 唯一性 |
|---|---|
| 全身份元组 (sequence, frameId, substepId, attemptId, physicsTime, committedOrdinal) | **283/283 唯一** |
| substepId | 283/283 唯一 |
| attemptId | 283/283 唯一 |
| frameId | 142 个（每帧 2 子步，dt=1/60 钳位，正常） |
| sequence 单独作键 | **不唯一**（267 出现 7 次） |

**结论：连接键必须是全元组（或 attemptId / (frameId,substepId)）；sequence 单独不是唯一键。** 这与 endpoint 协议"连接键=(runId,sequence,frameId,substepId)"一致——协议口径下键仍唯一，只是 sequence 单独复述会产生重复。

## 2. R1 重复 sequence 逐笔解剖

- 重复范围：**仅 sequence=267，共 7 条**，分布在 frame 134–137（每帧 2 子步），substepId 267–273 连续无空洞，attemptId a267–a273 逐笔独立，committedOrdinal 267–273 逐笔递增。
- 机制（代码面）：`context.sequence = p011SampleSequence+1`（beginNaturalObservation 捕获）；`p011SampleSequence` 仅在 `getP011Metric` 内自增（step 环 hook / noop 遥测）。frame 134–137 期间计数器冻结 6 个子步（墙钟 19654→23568ms ≈3.9s 停滞，落在 swipe4/5 窗口），frame 137 s2 恢复自增。
- **生产侧而非传输侧**：R1 初拉与终拉前 282 行**逐字节相同**（初拉第 283 行为截断 JSON，长度 287974 字符，`Expecting ',' delimiter`），重复在两份拉取中都存在——不是导出重发或传输损坏。
- **离线不可判定项**：设备上 hook 是否被跳过无逐帧遥测（无 hook-fire/序列自增记录），**不能区分**"step 环 hook 被跳过（如 testMode 分支未走 hook 路径的帧）"与"其它冻结原因"；交 T64，需要逐帧 hook 遥测或设备复现。
- 对照：R0 954 条 0 重复（全程单子步帧）；WS_METRIC 采样行（R1-full 48 条）同样出现 267×3（substepId 268/270/272），与 NDJSON 一致，排除单流伪象。

## 3. stateBefore/stateAfter 链与准入

- **链断裂恰好 27 处，与 externalAdmissionCount 递增步一一对应（集合相等）**；每处 bh/bl/mh/ml/suspH/suspL 跨步质量差最大 **1.78e-15**（浮点零），差异全部在 waterTop（外源注入把悬浮质量转水柱）。账本链本身无缺失、无重复提交、无回滚孔洞。
- **准入 27 笔按原顺序逐笔 `+=` 累加 = sourceInjectedH/L 逐位一致**（19.731137432098386，差 0.0）；transactionId/cumulativeCount 1..27 连续；evidence 块 count=27、rawHistoryStatus=STREAMED、externalAdmissionCount==sourceEpoch==27（健康不变量）。T44 报告的 3.55e-15 差异是**求和方式**（补偿求和/次序）产物，不是账本差异。
- R0 无准入（0 行，文件为空哈希 e3b0c442…），与 R0 无触摸一致。

## 4. store 元数据 route 来源（F44-IDENTITY 第二层）

`P011_OBS_STORE` 打印的 `observationRouteId="natural-relax-hysteresis-v1"` 来自 **SandCanvas 硬编码模块常量 `P011_NATURAL_OBSERVATION_ROUTE_ID`**（SandCanvas.ets:1152），不是运行时路由；NDJSON context 的 `observationRouteId="dual-field-particle-v1"` 与 `particleRouteId` 才是实际路由。**元数据与账本的 route 差异是日志源写错常量，不是路由激活差异**——修复属一行日志源更正（归 T66 范围由 T64 裁定），且不影响任何账本判定。

## 5. 收尾/CLOSED 生命周期夹具（F44-C）

真实 `FileStore`（字节未改）+ worker 协议桩（镜像真实 worker 的 init/ready/append/written/admission/close/closed 与计数语义；桩为合成，已标注）：

| 用例 | 场景 | 结果 |
|---|---|---|
| A01 | 283 记录+27 准入后 close() | **PASS**：close resolve，CLOSED 回执计数可复现（283/27） |
| B01 | host/worker 计数失配（2 vs 1） | **PASS**：OBS_STORE_INCOMPLETE、close reject、**无 CLOSED** |
| C01 | closing 期间 append | **PASS**：OBS_STORE_APPEND_AFTER_CLOSE（fail-closed，对物理调用方不抛异常）、close reject |
| D01 | 双重 close() | **PASS**：幂等同一 promise |

设备侧解释候选（T64 分流）：T44 日志中既无 CLOSED 也无 P011_OBS_ERROR。**若 close() 从未被调用**（Back 只触发 onPageHide——它不关 store；只有 aboutToDisappear 关），则无回执是预期行为；**若 close() 已调用但 worker 死亡/消息丢失**，B01 分支的守卫同样只留 OBS_ERROR 而无 CLOSED。两种可能离线不可区分（无生命周期遥测）。

## 6. 渲染身份链（F44-VISIBLE）

T44 日志中**不存在** `p013_dual_frame` 回执、renderer 选择、draw/swap、fallback 拒因或帧 token 字段；页面也未显示 NONUNIFORM_SCALE/INVALID_PARTICLE。**渲染身份链无法从 T44 归档恢复**——无沙画面的渲染分支归因需要设备采集（逐帧 renderer 决策 + draw/swap 回执），交 manager 建带真机标签的采集卡；不以 T52 的两码外推本基线。

## 7. 交 T64 的缺口清单（设备独有，如实缺失）

1. 逐帧 hook 触发/序列自增遥测（区分 sequence 冻结的两种机制）；
2. T44 页面 renderer 选择/draw-swap/fallback/frameToken 逐帧回执（无沙根因）；
3. Back/Home 是否到达 aboutToDisappear 的生命周期事件记录；
4. worker 侧 close 时 written 计数（区分"close 未调用"与"worker 失败"）。

## 8. 不涉及项

- 不改产品/判据/协议；不宣称 T44 任一 FAIL 项已修复；
- 不把合成夹具当设备通过；复算不去重、不修补坏行（R1 初拉截断行原样保留并记录）；
- 不重复 T44 已交付的 prefix 分析；总体根因与修复路由归 T64，实施归 T66。

## 9. 变更记录

- 2026-10-26，v1：首次发布。唯一键 283/283 全元组唯一、sequence 冻结机制解剖（6 子步/3 帧/墙钟停滞，两拉取逐字节同前缀）、链断裂 27 处恰为准入、准入逐位一致（3.55e-15 为求和方式产物）、route 差异定位到日志常量、生命周期守卫 4/4 复现、渲染链不可恢复如实声明；四项设备缺口交 T64。
