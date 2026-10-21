# P-011 观测③ drift 消费口径判定记录 v1（tester）

- **日期**：2026-10-21；责任：tester。
- **依据**：v3 pilot v2 报告 §5 移交 + architecture-P011 §26.4 决策 2（drift 字段消费口径归 checker 判定域）；协议 §3.2「实际接受时间/间隔照实记录并执行既有输入时序闸门」。
- **数据源**：smoke run `p011-run-1792565357992-3`（诊断性，非 pilot），`test/artifacts/p011-v3pilot2-20261021/smoke-144911/ledger.ndjson`（5939 条记录、1513 条命令、1214 条有子步消化）。
- **性质**：判定记录 + checker 落地。不自行放宽/收紧协议——若需收紧须上报 manager。

## 1. 判定基准：实际接受间隔，非绝对 drift

协议 §3.2 原文是「**实际接受时间/间隔**照实记录并执行既有输入时序闸门」——闸门的直接对象是相邻命令的**实际接受间隔**，不是单条命令与 schedule 的绝对偏差。两个量的区别：

| 量 | 定义 | smoke 实测 | 属性 |
|---|---|---|---|
| `scheduleDriftMs` | `actualDispatchMs − scheduledOffsetMs`（单条，自播放起点累积） | P50=57 / P95=86 / max=99 | 含**会话起始常偏**（启动耗时 + 帧量化），不随时间恶化（5939 条内无增长趋势；被消化命令 P50=46、max=99 全程稳定） |
| 实际接受间隔 | 相邻已消化命令首子步 `physicsTime` 之差 | leg 内 P50=132ms / P95=141 / min=62 / max=168（对 100ms 理想：dev P50=32 / P95=39 / max=68） | **闸门判定对象**；反映真实消化节拍 |

**结论**：`SCHEDULE_DRIFT_EXCEEDED`（>50ms）标记 62.7% 是**绝对量的常偏表现**，不代表命令间隔失序。若以 marker 计失格，等于把启动常偏当作时序漂移惩罚——与「照实记录」语义不符。marker 保持**非失格判定**（§26.4：marker 非 error），checker 按间隔判定。

## 2. checker 消费口径（落地规则）

按 phaseKind 分域判定实际接受间隔（理想值不同）：

| 衔接域 | 理想间隔 | 失格条件（命令级，作用于该命令驱动的子步） |
|---|---|---|
| ramp→ramp（同 leg 相邻命令） | `COMMAND_STEP_MS = 100ms` | 间隔 `< 50ms`（半步距以下=疑似追赶/跳步伪造）或 `> 200ms`（两步距以上=节拍失控，子步消化停滞） |
| platform→ramp（hold 结束进入 leg） | `PLATFORM_HOLD_MS = 5100ms` | 间隔 `< 4000ms` 或 `> 6200ms`（hold 被「多等数秒」冒充或被截断） |
| 其余衔接（跨 leg/跨平台边界） | — | 不在此闸门判定（bracket 本就不跨边界，§3.2） |

**辅助完整性约束**（同一次落地）：

1. **慢帧合并不隔断 bracket**：命令序列跳变（如 104→106，299 处）只要 `substepId` 连续，bracket 链完好——bracket 是 attempt/子步层概念，命令层跳变不隔断（smoke 实测：跳变处 substepId 762→763→764 连续）。但被合并的两条命令目标差必须 ≤ 2×角步距（0.10°@0.5°/s）——超过则该命令对失格（新码 `SCHEDULE_COMMAND_GAP_EXCEEDED`）。
2. **角跨度完整性**：leg 内相邻消化命令 target 差 = k×角步距，k≤2 容许（k=2 即慢帧合并 1 帧）；k>2 失格。smoke 实测：leg 内 >1 步距跳变 0 处（全部恰为 1 条跳过）。
3. **drift 字段消费**：`scheduleDriftMs` 仅作为遥测记录与报告统计量（P50/P95/max 进 checker 输出 `scheduleDriftSummary`），**不参与任何失格判定**；`scheduleDriftExceeded` marker 不产生 censor code。
4. **dt 轨迹**：子步间隔 >40ms 占 47.5%（慢帧合并语义），dt 轨迹集合按协议 §4.1 由 architect/tester 绑定到正式采集配置——不在本判定内单设闸门，但 checker 输出 `dtSummary` 供绑定登记。

## 3. 判定结论：当前漂移水平无需收紧（不上报）

| 判定项 | 实测 | 闸门 | 结论 |
|---|---|---|---|
| leg 内间隔 | P50=132 / P95=141 / max=168 | [50, 200] | **全过**（P95 141 距上限 200 有余量；max 168 亦在内） |
| hold 时长 | P50=5076 / min=5072 / max=5124 | [4000, 6200] | **全过**（无「多等数秒冒充」，无截断） |
| 命令合并幅度 | leg 内跳变全部 = 1 条（0.10°） | k≤2 | **全过** |
| 追赶行为 | drift 无增长趋势；间隔 min=62 > 50 | 无跳步伪造 | **无** |

**P50=57ms 的绝对 drift 不构成收紧依据**：它是 `只记录不追赶` 语义下启动常偏+帧量化的稳态表现，命令间隔（闸门直接对象）全程在界。**不上报 manager 收紧请求**；若正式 pilot 出现间隔越界或 k>2 合并，按上述规则失格并如实记录。

## 4. checker 落地与单测

- `test/p011_obs3_v3_protocol_check.py`：新增 `SCHEDULE_TIMING` 常量区（间隔域/阈值/合并上限）、Pass 2 命令级时序检查（间隔越界 → 所涉子步的 bracket 候选失格，码 `SCHEDULE_INTERVAL_OUT_OF_BAND`；合并 k>2 → `SCHEDULE_COMMAND_GAP_EXCEEDED`）、输出 `scheduleDriftSummary`（P50/P95/max/标记占比，遥测不判定）与 `dtSummary`。
- 单测：`test_p011_obs3_v3_protocol_check.py` 增补间隔正反例（界内/过快/过慢/hold 域/合并 k=2 容许/k=3 失格）与「marker 不失格」断言。
- 本记录不修改协议、冻结附表、播放器实现或任何产品代码。
