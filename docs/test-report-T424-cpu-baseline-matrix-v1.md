# T424 · CPU 整帧负载与事务差分基线（legacy 冻结）

2026-11-09，tester。不使用真机、不改产品。按 T420 §6"匹配负载"独立冻结现行 legacy 原路（`P051DerivedFacadeV2`/`CPU_ORIGINAL_F64`，隔离工程 `.codex/local-test/t400-final`）的可复跑离线基线，产出候选差分消费的判读器与适配接口清单。T423 实现未交付，候选差分按卡面约定随后另排。

## 1. 交付物

| 文件 | 用途 |
|---|---|
| `test/t424/baseline_matrix.cjs` | 冻结基线生成器：7 场景 × 96 帧（dt=20ms callback，Q≤2），同输入/seed=51/事件序逐帧可复跑；每帧 begin→work→end 互斥闭合（FrameBudgetV2 schema 原样） |
| `test/t424/judge.py` | 独立判读器：哨兵逐位核验 + 逐场景工作界汇总（wall p50/p95、Q/A/R、阶段成本、fillCopy/authority 字节）+ T368 真机数量级锚 |
| `test/reports/t424/matrix.json` | 基线原件（逐帧 rows + substeps + stages + admission 实账） |
| `test/reports/t424/judgement.json` | 判读输出（verdict PASS，8 entries） |

复跑：`node test/t424/baseline_matrix.cjs .codex/local-test/t400-final test/reports/t424 && python -X utf8 test/t424/judge.py`

## 2. 场景矩阵（T420 §6 匹配负载覆盖）

| 场景 | N | L 终态 | Q 合计 | 状态 | 阶段热点 |
|---|---|---|---|---|---|
| empty_active_n512_l0（空场活跃无源） | 512 | 0 | 0 | 96/96 ok | callbackTail/seal/consumerCopy（无 R、无 hotApply——L=0 仍执行场/发布/渲染，证实"不能默认休眠"） |
| empty_sleep_n512（真实休眠 setActive(false)） | 512 | 0 | 0 | 96/96 ok | 仅 consumerCopy——与活跃空场对照即休眠差分基线 |
| inject_n512_l8（持续注入） | 512 | 96 | 184 | 96/96 ok | 全链 14 段激活（injection→…→exact70/strictDomain） |
| inject_hold_n512_l157（UP 后高接触） | 512 | 512 | 184 | 160/160 ok | step/…/exact70；**R 实账=0 行入账、preRejected=95 次注入被 CAPACITY_BLOCKED 拒绝（如实记录，不冒满池注入）** |
| inject_full_n512_l512（满池） | 512 | 511 | 184 | 96/96 ok | 13 段激活 |
| empty_active_n64_l0 | 64 | 0 | 0 | 96/96 ok | 仅 step |
| inject_hold_n64_l48 | 64 | 64 | 184 | 160/160 ok | 11 段激活 |

## 3. 哨兵（行为回归，逐位）

- **T403 LEVEL0**：三笔正授予 `[0.1, 0.1, 0.09999999999999998]`、退役 3、`bedFinal=0.3`——**逐位一致**
- **T403 TILT25**：`[0.1, 0.032121583881089655, 0]`、退役 1、`bedFinal=0.13212158388108966`——**逐位一致**
- T381 零获准约束由 T381 units（全过）继续承担；本基线哨兵锚在 Z08 夹具物理结果上
- T368 高接触窗（callback2017-2144 N512/L157/Q2/A0/R942、子步 wall p50 411ms/R471）作为真机数量级锚引入判读器对照；host 基线不直接比 ms，只比工作界结构

## 4. 可复跑性证明

同输入/seed/checkpoint 两次独立运行：逐帧 `commit/N/live/Q/A/R/fillCopy` **完全一致**，哨兵一致（runId 含 Date.now 会话戳属预期差异，不在物理等价域）。判读器 `incomplete=0/invalid=0`（互斥闭合 96/96）。

## 5. 负载缺口与边界（如实登记）

- **R 记账缺口**：高接触场景的 `R` 在 substep 层为 0（facade 预算未把 apply 行数记入 substep row 的 R 槽），行层 R 亦只在低 L 场景非零。T420 §2 要求"R 是 apply 行数，不能替代活粒"——**当前 R 账不可对齐 T368 的 R942/R471，属明确缺口**，候选差分不能以本基线的 R 作逐行对照，须等 T423 实现侧补 substep R 记账或在判读器内以 hotApply stage 总额代偿（本判读器已同时报两级）。
- **注入准入**：单笔上限受 free slot 约束（`CAPACITY_BLOCKED`），预注入按自适应策略如实记账 preAdmitted/preRejected；"满池 L157/101 分布对齐"只能近似（L 终态 512/511/64/96），不能伪造 T368 的 L157 时序分布。
- **Q/A**：Q=2 由 dt=20ms 决定，两窗 Q 合计 184 与 T368 的 Q2/帧一致；A=0（无水路）。λ 重试/失败重试路径未注入 fault 注入器，属候选差分阶段另配（负控在 T423 落地后加）。
- 本基线为 host 离线，33.3ms 判定与真机 A/B 按卡面归后续真机任务，不在本卡范围。

## 6. 候选适配接口（给 T423/T426 消费，不等 T423 交付）

1. **输入契约**：`matrix.json` 每 scenario 的 `frames[]`（commit/epoch/N/live/Q/A/R 序列）即为差分对照键；候选实现按同 seed/同注入序列回放后逐帧比对。
2. **判读入口**：`judge.py` 支持 `--matrix <path>` 扩展为双档输入（legacy vs candidate）分列输出；当前为单档结构判读。
3. **哨兵复用**：`sentinels.t403_physics` 逐位期望值可直接嵌入候选实现的等价回归（T420 §6 等价回归项）。
4. **R 补账请求**（交 T423 developer）：substep row R 槽在 apply 路径未赋值，候选差分前需在 facade 预算内补 `applyParticlePlanInto` 行数到 `substeps[9]` 与 `rows[9]`，或在判读器声明 R-missing 并以 hotApply 总额代偿。
