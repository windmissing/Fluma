# T40 O-2 口径补充与独立验证回执（tester）

- 任务卡：`6adda513e4b0e6ce82af399e`（【T40·O-2口径补充与独立验证】tester：合法相等、独立计数来源和故障反例）
- 日期：2026-10-25；角色边界：客观测试；**不使用真机**；不改判据、不改协议、不宣布生效
- 被测实现：T39 commit `2f864f0`（`fix(p011): derive admission count from committed receipt records`），验证时工作区 = HEAD `2ef21f9` + 并行未提交改动（仅渲染发布相关，见 §5）
- 依据：T04 原卡本轮 manager 裁决 + `docs/architecture-P011-endpoint-fields-revision-v1.md` §9（05e2c68）
- 结论：**PASS**（30/30 项 MATCH，未发现口径偏离或假 PASS 路径）

## 1. 口径补充登记（适用补充版本：本文件 v1）

按 manager 裁决与 architect §9.2/§9.3，登记以下口径，作为 T11 取证与后续 revision 的适用依据：

| 项 | 补充口径 |
|---|---|
| 合法相等 | 正常 run 中 `externalAdmissionCount == sourceEpoch` 是**合法不变量**，不是缺陷 |
| 独立含义 | “独立”指**来源/核对链独立**：count 由成功准入 receipt 记录累加器维护，epoch 由物理成功准入边界维护；两者不由同一变量复制 |
| 旧任务文字 | 原卡“健康 run 必须不等”为任务文字冲突，已由 manager 撤销；**禁止**制造不一致来“证明可触发” |
| 检测语义 | `EXTERNAL_ADMISSION_COUNT_EPOCH_MISMATCH` 保留原语义：漏记/重复/错配/传输损坏时可检出；正常相等不使检测失效 |
| 协议正文 | 已发布 endpoint 协议 v1 **零改动**；本补充不新增加协议字段、不起 errata revision（计数对象与语义未变，变的只是实现来源，属实现层事项，由本登记覆盖） |
| 估计对象 | 不改 E=VH0+VL0+IH+IL、M=U+P、5%、末 1s 窗、双算、sourceEpoch 静置门 |

## 2. 独立验证方法与结果（30/30）

执行器（tester 侧，独立编写，未复用 T39 开发自测代码）：

- `test/run_t40_external_admission_independent.cjs`：驱动真实 ArkTS `WaterSandField`，24 项探针
- `test/t40_external_admission_verification.py`：把真实 writer 上下文送入**交付 checker**（只读），6 组判定
- 证据：`test/reports/t40-external-admission-independent.json`（`allMatch=true`）

### 2.1 独立计数来源（A 组，5 项）

| 探针 | 结果 |
|---|---|
| A1 从原始成功记录独立重算 count（去重 admissionTransactionId、admittedH+L>0 过滤）与 count/epoch/observedTransactions/IH/IL 合计一致 | MATCH（3 次准入：count=epoch=3，admittedH/L 合计与 sourceInjected 一致） |
| A2 破坏 epoch（=99）后 count 与 writer 值不变（2） | MATCH——count 不读时派生自 epoch |
| A3 破坏 count（=0 / =2）后 epoch 不变（2 / 1） | MATCH——epoch 不派生自 count |
| A4 消费端篡改导出记录副本/snapshot 不影响权威证据 | MATCH |
| A5 同一请求内容重放为两笔不同成功事务（id 1,2），exportLast 重放不重计 | MATCH |

### 2.2 合法相等与不增项（B 组，7 项）

两相多列单事务恰计 1 次（小数质量仍一次）；零请求/容量拒绝/λ 耗尽回滚/NO_WATER 不计且场状态逐值不变；失败 λ 试算后成功只计最终 receipt；shake/step/invert/snapshot 读取与导出不计；健康上下文 count=epoch=1 送入 checker 得 **PASS、无硬闸门失败**。

### 2.3 reset/restore 边界（C 组，6 项）

scene reset 成组归零（count/epoch/IH/IL/lastRecord/新 runId）；restore 成组恢复（count=2、epoch=2、IH/IL、初值）且 `rawHistoryStatus=SUMMARY_ONLY`（检查点不是原始流），恢复后可继续累计（3）；restore 为深拷贝（事后篡改快照不泄漏）；legacy 缺 `externalSourceState` → count=null 且后续准入不回填、显式 reset 后才归 0；epoch 不一致的检查点在改场前被拒（`MISMATCH`，waterTop 逐值不变）；count=null 伴随非 null 质量 → `INVALID_EXTERNAL_SOURCE_SNAPSHOT`。

### 2.4 披露状态与原子性（D 组，6 项）

无 sink=`SUMMARY_ONLY`、有 sink=`STREAMED`；导出失败→`INCOMPLETE`（exportFailures=1）且计数不变；prepared 记录单次使用（重复 commit 抛 `DUPLICATE`）；零质量/负/NaN 记录抛 `INVALID_EXTERNAL_ADMISSION_RECORD`；`sourceEpoch=MAX_SAFE_INTEGER` 时注入抛 `SOURCE_EPOCH_OVERFLOW` 且在发布前原子回滚（场逐值不变、count=0）。

### 2.5 故障反例入 checker（Python 6 组）

| 场景 | count / epoch | checker 判定 |
|---|---|---|
| 健康 | 1 / 1 | **PASS**（无任何 EXTERNAL_ADMISSION 硬失败） |
| epoch 损坏 | 2 / 99 | EXTERNAL_ADMISSION_COUNT_EPOCH_MISMATCH → INVALID_LEDGER |
| 漏记 | 0 / 2 | 同上 → INVALID_LEDGER |
| 重复发布 | 2 / 1 | 同上 → INVALID_LEDGER |
| 传输损坏（健康+1） | 2 / 1 | 同上 → INVALID_LEDGER |
| bool / 负数 / 小数 | — | INVALID_EXTERNAL_ADMISSION_COUNT → INVALID_LEDGER |
| 字段缺失 / legacy null | — | missingFields `context.externalAdmissionCount`，状态 NOT_IMPLEMENTED（**不默认填 epoch 或 0**） |

## 3. 与 T39 开发自测的边界

本验证为 tester 独立编写（探针按 §9.2/§9.3 契约文本与 manager 裁决重写，不复用开发自测代码），两者结论一致（T39：node 13/13、python 4/4；T40：node 24/24、python 6/6）。T40 额外独立覆盖：原始记录重算 count（A1）、双向非派生（A2/A3）、快照深拷贝时序（C3，修正了“快照即恢复真值”与“别名泄漏”的语义区分）、null+质量非法快照（C6）、零/负/NaN 记录与 epoch 溢出原子性（D4/D5）、类型损坏三类（bool/负/小数）、字段缺失与默认填值排查。

## 4. T11 取证资格结论（供 T11 使用）

- **原始事件导出开关可用**：`WaterSandField.setExternalAdmissionEventSink` 已在生产类暴露（`P011ExternalAdmissions.exportLast` 同步导出已提交证据副本）；T11 设备 run **必须在准入发生前绑定 sink 并保持全程绑定**，导出记录含 `runId/admissionTransactionId/sourceId/sequence/inputTime/admittedH/admittedL/cumulativeCount`，身份可溯。
- **记录完整性资格判据**（T11 回执须披露）：`STREAMED`=原始记录完整可得（唯一可独立重算状态）；`SUMMARY_ONLY`=仅汇总（未开流）；`INCOMPLETE`=有导出失败（exportFailures>0）；`UNKNOWN`/count=null=legacy 恢复缺证据。只有 `STREAMED` 支持“从原始记录重算 count”的独立核对；其余状态按缺失披露，**不得**用 epoch 补造。
- **资格结论**：O-2 实现满足 T11 取证的数据前置；T11 仍需自行完成设备 run 与同账本 peak 双报，本卡不代跑、不预先宣布 endpoint/peak 生效。

## 5. 版本与哈希登记

| 对象 | 值 |
|---|---|
| endpoint 协议 SHA-256 | `ffe337d73a4ee78b21a48543025468238cb3607d68b386de7b2097fba5148837`（与验收记录一致，工作区无改动） |
| `P011ExternalAdmissions.ets` | blob `dad22ea0e18f33a9f3f139488704a59596838e32`（2f864f0 与 HEAD 相同） |
| `P011NaturalObservation.ets` | blob `2ed4b77bb459178bde91ddf54fe3bf9ce0e28faa`（2f864f0 与 HEAD 相同） |
| `WaterSandField.ets` | 2f864f0=`838634366987ae4b762c6e68904258c959201729`；HEAD=`fcdf34bc0cbd426334022fb40d532d9903aa7a10`（并行会话渲染发布改动，** admission/epoch/inject 逻辑无差异**，工作区另有 38+/7- 未提交渲染 hunk，均不影响本结论） |
| 本补充版本 | `tester-T40-o2-supplementary-v1`（叠加式登记，不回改任何已发布文件） |

## 6. 观察项（informational，不改判定）

- O-1：restore 对快照中 `lastRecord.admittedH/L` 不与其 `admittedH/L` 合计交叉校验（`validateSource` 只校验 id/cumulative/质量>0 与总量对 IH/IL）；快照完整性依赖外部 state hash。属快照传输层稳健性改进建议，不构成 §9.2 成组恢复契约违反（count/epoch/IH/IL 均已交叉校验）。
- O-2：`SUMMARY_ONLY`/`UNKNOWN` 状态下 checker 只能核对汇总值，无法重算；T11 若未全程开流，独立核对资格降级为披露项（已在 §4 登记判据）。

## 7. 复现命令

```bash
node test/run_t40_external_admission_independent.cjs   # 24/24
python test/t40_external_admission_verification.py     # 6/6（含 checker 只读消费）
python -m unittest test.test_p011_external_admissions  # T39 交付套件 4/4（对照）
```

- 2026-10-25，v1：首次发布。口径补充登记 + T39 实现独立验证（30/30）+ T11 资格结论。纯离线；未改协议/判据/实现；未用真机；未宣布 endpoint/peak 生效。
