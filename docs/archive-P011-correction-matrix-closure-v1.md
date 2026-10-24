# P-011 correction 矩阵归档：全链闭环文档 v1

日期：2026-10-24。责任：tester。状态：**ARCHIVED / CANDIDATE_NOT_PROMOTED（维持候选，归档关闭）**。
性质：本文件是 correction 线的**归档整理**，不改写任何历史数据、结论或阈值；全部数值以既有回执与 commit 为准，本文只做索引、串联与影响评估。

裁决依据：manager 终审登记（滴答任务 6ad9ea60，2026-10-23 17:32 评论全文，见 §5.2 引用）。

## 1. 结论头

**correction（`relax-correction-weighted-v1` 路线）维持候选：`CANDIDATE / NOT_PROMOTED`。**

- 代码**保留归档**，`routeId` 显式隔离（D-1 裁决第 2 条：`relaxHysteresisThresholds !== null` 是必要条件，显式 `observationRouteId` 绑定才是充分条件）——非 correction 的 LEGACY/默认链路逐位不变。
- correction **不进入默认链路**，不作为任何新批次的方法基底。
- 审计结论接手方 = 方案 3（悬浮平流 advection，机制设计已验收），按「**无 correction 快车道**」基底推进实现排期（architecture-P011-advection-v1.md）。
- 观测③标定链以裸 26/20（factory-recommended，非标定值）机制继续，不受本归档影响。

## 2. 全链时间线

| 时间 | 事件 | 证据 |
|---|---|---|
| 2026-10-21 | **D-1 发现**：correction apply 越权激活（候选阈值武装即隐式走 slot，LEGACY 路径行为漂移）。manager 裁决 Option A（资格门）+ 显式路线隔离；D-1b 遥测四字段登记 RED（by design，不阻断） | `docs/priority.md`「2026-10-21 D-1 裁决」；TC-P011-OBS3-D1 用例（test-cases.md §11.10） |
| 2026-10-21 | D-1 整改核验：LEGACY 240 步轨迹 14 锚点逐值恢复；四场景 gate/路由分流 PASS；D1-C3（D-1b 遥测）如实 RED | commit `742efa6`；`docs/verification-report-P011-obs3-v3-d1-v1.md` |
| 2026-10-22 夜 | **conditioning 通道验证**：通道接通（CND_ 前缀 65s 偏移）、前缀在做事（maxΔS≈0.65）；**Q3 不成立**：G1/G2/G0 三档共用同一命令序列 → `STRATUM_FACTOR_NOT_EXCITED`；用户裁决照常跑满 108run、按「27 可交换 block / 单一 working state」登记 | `docs/cnd-verify-receipt-P011-v1.md` |
| 2026-10-22 18:50 | 终审任务链建立（采集→tester 分析→designer 建议→manager 终审） | 滴答 6ad9ea60 |
| 2026-10-22 晚 → 10-23 16:48 | **正式 108run 采集**：第 1 run 因哨兵判据比引擎严（无 EPS 容差）误触发停批，按「不删失败 run」保留 `-partial-<ts>` 并重采；哨兵对齐引擎容差（tan30+1e-9）后续续跑 | stop-receipt §3/§4；cnd-verify §9 |
| 2026-10-23 16:24 | manager 裁决 `EARLY_STOPPED_AT_USER_DISCRETION`（用户指令「把你的脚本停下」）；commit `c79d447` 落 STOP_BATCH.now 优雅停机；16:48 停机于 **run 82/108** | stop-receipt；closure-receipt |
| 2026-10-23 17:06 | 用户确认「108run 收尾 = 封存交接」，不再续跑剩余 26 run | closure-receipt |
| 2026-10-23 | 封存：82 run 全哨兵通过，20 完整 block，`CLOSURE.json` 落盘，数据封存待 tester 提取 | `test/artifacts/p011-corr-matrix-formal-108-20261022/CLOSURE.json` |
| 2026-10-23 | **interim 精确复核**（commit `4402c5f`）：口径三疑点全解，12 block 全量重算 median −271.8%、CI 上界<0、12/12 全负；归因=correction 通道本身（非 D 特异） | `docs/interim-g1-review-P011-v1.md` |
| 2026-10-23 | **支持性分析管线 dry-run**（commit `950fd09`）：20 完整 block median −2.7280、CI [−2.7436,−2.6963]；G1/G2 漂移可忽略；金额闭合零违例；非劣门三件无炸 | `docs/dryrun-27block-pipeline-P011-v1.md` |
| 2026-10-23 17:32 | **manager 终审：维持候选**（用户裁决），correction 归档关闭 | 滴答 6ad9ea60 评论全文（§5.2） |

## 3. 数据摘要（封存口径）

来源：`corr-matrix-formal-108-closure-receipt-P011-v1.md` / `CLOSURE.json` / dryrun 报告。冻结计划 `calibration/p011-corr-matrix-formal-plan-v1.json`（master seed 20261022），构建 s133（`entry-default-s133-signed.hap`，SHA256 `b0c67920…d6f3`）。

| 项 | 值 |
|---|---|
| 完成 run | **82 / 108**（EARLY_STOPPED_AT_USER_DISCRETION） |
| 完整 block（ABCD 齐） | **20**：G1 b01–b12（12）+ G2 b01/b05/b06/b07/b08/b10/b11/b12（8） |
| 不完整 block | 1：G2/b04 仅 A、D（主比较 D vs A 可用 21 对；B/C 比较限 20 完整 block） |
| 未开始 | G0（3 block 全缺） |
| 变体计数 | A 21 / B 20 / C 20 / D 21 |
| 哨兵 | 82/82 全通过（身份、D-1b 闭合、前缀到位、硬安全、不变量在 tan30 内、runId 唯一） |
| J_E 主比较（D vs A） | 20 block median **−2.7280**，bootstrap median 95%CI **[−2.7436, −2.6963]**（上界<0，方向结论稳健） |
| interim（G1 12 block 精确口径） | median **−271.8%**，12/12 全负，0/12 达 10% 门槛 |
| 非劣门三件 | 帧时 387–392s 无差异；普通通道挤占无证据（reject 率 B 1.88%/C 2.03%/D 1.94%，budget excess e-14 量级）；输入可达无异常 |
| tan30 | 引擎闸门零违规（range=True 82/82）；**61/82 run 贴边界**（maxErr=0.5773502691896xx，全部 B/C/D 修正臂；A 组 0.4997–0.5010 余量≈0.077）——登记为观察项，未裁定正常饱和/余量告警 |
| 金额闭合 | per-substep 核对 20 block 全部 0 违例 |
| stratum | `STRATUM_FACTOR_NOT_EXCITED`：三档同分布，G2 稳健性 / G0 控制两条预注册判定作废；统计适用域=单一 working state |

**数据定性**（closure-receipt §2.5）：仅可用于**描述与区间估计**，不得写 VALIDATED / PASS；本归档遵守该口径，负信号登记为「维持候选的支持性数据」，不作为转正依据（转正分支本批未启用）。

## 4. 归因结论（研究备忘）

**J_E 抬升不是 D 变体（e²γ 加权算法）的缺陷，而是 correction 通道本身的机制后果**（interim 精确复核增量发现，B/C/D 全臂一致）：

- B / C / D 三臂 J_E 紧聚集（median 0.02984 / 0.02957 / 0.02967），A 单独远离（0.0078–0.0083）——**所有 correction 臂一致抬高 J_E ≈3.7×**，与加权指数 γ 无关。
- 超出量分解（b01）：D 的 excess² 中 **active 边 171.2（A 的 7.4×）**、inactive 边 74.6（A 的 1.7×）——搬运材料后，邻边/后续子步残差重新超过 stop 线，而 E 口径（无 active 遮罩 + stateAfter 计）把搬运副作用全量计入。
- 非劣门三件无异常，排除工程性故障（帧时/挤占/输入可达）归因。

**备忘（若未来重启 correction 研究，从这里开始）**：机制研究方向应为「slot-apply 与 E 累积的交互」——例如 slot 搬运的邻边效应是否应在 estimand 中归属、active 边复超是否可用提交前联合预演约束。**ESTIMAND 复议通道保留**（interim 报告已登记）：若 designer 认为 correction 臂不应承担搬运副作用，须走新 revision 复议估计量定义，不得临场改口径；转正复活条件见 §6.4。

## 5. EARLY_STOPPED 声明

### 5.1 事实

- 停机：2026-10-23 16:48，run **82/108**（计划 27 block，完成 20 完整 block + 1 不完整 block）。
- 原因：用户显式指令停机（「把你的脚本停下」），**非哨兵触发、非失败停批**；manager 16:24 裁决 `EARLY_STOPPED_AT_USER_DISCRETION`，设备释放给 effector 真机窗口；用户 17:06 确认收尾=封存，不续跑。
- 现场：后台任务 killed（运行 8h53m）；宿主机残留 hdc 孤儿进程与设备端 hilog 已清；设备在线未关机。

### 5.2 样本缩水影响评估（manager 终审登记 17:32 原文引用）

> 「④批经用户裁决 EARLY_STOPPED（82/108，~20 完整 block），方向结论稳健（剩余 run 无法翻转 CI 上界），样本缩水如实标注。」

即：26 个未采 run 按已观测效应量（median −2.73、stdev 0.07）不可能把 bootstrap CI 上界从 <0 翻过 0；剩余样本的价值在**精度**（CI 收窄）不在**方向**。本归档所有区间估计均按 82 run / 20 block 实际样本量陈述，不虚构 108 精度。

## 6. 后续衔接

### 6.1 方案 3 接棒（无 correction 快车道基底）

advection 通道工程设计已就位（`architecture-P011-advection-v1.md`，P-2/P-4/P-5/P-6 + 起沙 b 落点 + developer 任务书；P-1 前置已修 `8df1aae`）。排期按「correction 归档、无快车道」基底推进；b 阶段（起沙链）解锁等 P-3 manager 独立批准。

### 6.2 观测③标定

以裸 26/20 机制继续（factory-recommended 用户可配、非标定值）；correction 归档不改变标定链口径。

### 6.3 routeId 隔离保证

correction 代码保留归档但 routeId 显式隔离：非 correction 运行不触达 `applyCorrectionSlot`，LEGACY/默认链路逐位不变（D-1 整改 240 步 14 锚点轨迹等价为证）；S4 候选参数组 / sweep 二轮等并行工作互斥已预登记，不受本归档影响。

### 6.4 转正复活条件

correction 若未来重启转正评估，**必须重新预注册**（新 protocolRevision + 新采集批 + 新身份哨兵），不得沿用本批 82 run 数据直接转正；复活研究入口见 §4 归因备忘与 ESTIMAND 复议通道。

## 7. 红线遵守自查

- 归档=整理非改写：全部数值引自既有回执/commit/JSON，本文件未改任何历史数据、结论、阈值或判定。
- EARLY_STOPPED 全文如实（§5），样本缩水影响如实标注。
- ESTIMAND 复议通道保留说明（§4/§6.4）。
- 未追认任何历史 run；负信号按 closure-receipt 口径仅作支持性数据，不写 VALIDATED / PASS。
- 数据仍在封存目录（`test/artifacts/p011-corr-matrix-formal-108-20261022`，173.3 GiB trimmed 账本），未删除、未重采、未续跑。

## 8. 关联资产索引

| 类别 | 路径 |
|---|---|
| 本归档 | `docs/archive-P011-correction-matrix-closure-v1.md` |
| 停机现场 | `docs/corr-matrix-formal-108-stop-receipt-P011-v1.md` |
| 封存回执 | `docs/corr-matrix-formal-108-closure-receipt-P011-v1.md` |
| conditioning 验证 | `docs/cnd-verify-receipt-P011-v1.md` |
| 统计口径修订 | `docs/correction-matrix-conditioning-statistics-revision-P011-v1.md` |
| interim 精确复核 | `docs/interim-g1-review-P011-v1.md`（commit `4402c5f`） |
| 管线 dry-run | `docs/dryrun-27block-pipeline-P011-v1.md`（commit `950fd09`） |
| D-1 裁决 | `docs/priority.md`「2026-10-21」；TC-P011-OBS3-D1（`docs/test-cases.md` §11.10/§11.11） |
| 预注册 | `docs/preregistration-P011-correction-matrix-v1.md` |
| 封存数据 | `test/artifacts/p011-corr-matrix-formal-108-20261022/`（CLOSURE.json / batch-log.csv / 82 run 账本） |
| 终审登记 | 滴答任务 6ad9ea60 manager 评论（2026-10-23 17:32） |
| 接棒设计 | `docs/architecture-P011-advection-v1.md` |
