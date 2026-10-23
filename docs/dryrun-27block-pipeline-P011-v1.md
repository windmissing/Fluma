# P-011 27-block 正式统计管线 dry-run 报告 v1

- **日期**：2026-10-23；责任：tester。对应任务【P-011·27block管线dry-run】（滴答 6adb1443）。
- **性质**：**DRY_RUN / NOT_FOR_DECISION**——全部数值为管线预演输出，不构成任何判定；不改变采集；不修改冻结锚点与统计口径（revision 数值原样）。
- **数据**：当前批 ~82 run（batch-log 82 行）：**G1 12 完整 block + G2 8 完整 block = 20 完整 block**（80 run）；1 个不完整 block（G2-b04 缺 B，跳过并计数）；1 个 partial 目录（G1-b04-D 旧哨兵误判遗留，revision §3 已裁决不进 27 block）。
- **工具**：`tools/analyze_p011_corr_matrix_27block.py`（新，正式统计管线实现，批结束直接复用）；结果 `test/reports/dryrun_27block_pipeline_v1.json`。
- **口径**：correction-matrix-conditioning-statistics-revision-P011-v1 全条——单一 conditioned working state、配对 block bootstrap N=10000 seed=20261022、中位 gain ≥10% 且 CI 下界 >0 门槛原值、J_E 精确口径（interim 4402c5f 验收版）。

## 1. 管线预演结果（20 完整 block）

| 量 | dry-run 值 |
|---|---|
| gain median | **−2.7280（−272.8%）** |
| gain 范围 | [−2.9305, −2.6297] |
| gain stdev | 0.0714 |
| bootstrap median 95%CI | **[−2.7436, −2.6963]**（上界 <0） |
| 门槛（≥10% 且 CI 下界 >0） | 距离遥远（方向性预演，非判定） |

**G1/G2 标签漂移诊断（仅描述性）**：G1 12 block gain median −2.7184 vs G2 8 block −2.7280——**两批标签几乎无漂移**（差 0.01）；J_E,A 中位 G1 0.008068 vs G2 0.008092（差 0.3%）；帧时同量级。池化交换性初步成立（正式判定在全量 27 block 上重做）。

## 2. 管线问题清单与修复（dry-run 发现→当场修复）

| # | 问题 | 处置 |
|---|---|---|
| P-1 | executedThresholds 字段在部分 run 的 ledger 顶层缺失（context 级存在）——interim 精确重算器首跑 KeyError | 修复：`record.executedThresholds or context.executedThresholds or 默认` 双级回退（interim 已修，正式脚本继承） |
| P-2 | P 型指标窗反转未处理（E3 任务发现同类）——本管线无 P 指标，但 slopeStop 缺失路径同源 | 已含回退默认值；正式脚本断言 slopeStop 唯一性（输出 slopeStopValues 供核对） |
| P-3 | 批结束触发器未定义 | 本报告 §5：108 run 完成判据 = batch-log.csv 数据行 ≥108（当前 82）或 effector 回执；触发后直接 `python tools/analyze_p011_corr_matrix_27block.py`（脚本自动发现全量 block）出终版 JSON，去除 DRY_RUN 标注交 designer |
| P-4 | 金额闭合核对粒度 | 已实现 per-substep `correctionRequested == granted + rejected` 核对：**20 block 全部 0 违例** |
| P-5 | tan30 贴边诊断 | 已实现 `|maxErr−H|≤1e-9` 契约内饱和标志：**60/80 run 贴边、3,351 子步、maxErr 0.577350**——按 revision §3 登记为契约内边界饱和，非越界；无 range=False ✅ |
| P-6 | 非劣门三件 | 已实现：帧时 G1 387–390s / G2 390–392s（无差异）；correction 拒率 B 1.88%/C 2.03%/D 1.94%（同级）；budget excess 子步 33,968（量级 e-14）——无单项炸掉 ✅ |

## 3. 与 interim 复核的一致性

- interim（G1 12 block 精确）：median −2.7184；dry-run 同 12 block 复算 median **−2.7184 逐位一致**（管线等价性确认——正式脚本与 interim 重算器同口径同实现）。
- 加上 G2 8 block 后 median −2.7280、CI [−2.7436, −2.6963]——负信号在扩容后稳定，`INTERIM_NOT_SUPPORTED_SIGNAL` 方向维持（仍非判定）。

## 4. 正式批预期行为（全 27 block）

- G0 3 block（未采）+ G2 剩余 4 block 完成后：27 block 池化，脚本自动纳入。
- 不完整块（若有）：跳过并计数，正式批不补合成；若整批 <27 block，按 revision §1 须 manager 批准样本量修订，不得用已采块直接判 PASS。
- B−A/C−B/D−C 辅助对照与 FACTOR_NOT_EXCITED 检查（有竞争候选数/R_limit<Σu）在正式脚本中按 revision §2 表逐项输出。

## 5. 批结束触发器（本报告登记）

1. **判据**：`batch-log.csv` 数据行 ≥108（当前 82）或 effector 108-run 回执任一先到。
2. **动作**：直接运行 `python tools/analyze_p011_corr_matrix_27block.py`（预期分钟级，20 block 47 min → 27 block 预计 ~65 min）→ 终版 JSON（自动含全量 block）→ 去除 DRY_RUN 标注 → 交 designer 判定。
3. **红线**：正式判定前不对采集端泄露任何中间数值；触发判据本身不改变采集顺序。

## 6. 红线自查

DRY_RUN/NOT_FOR_DECISION 全程标注 ✅；dry-run 结果不影响采血 ✅；不停批 ✅；冻结锚点与统计口径零修改（预注册数值原样，门槛复述未改）✅；纯离线 ✅；不完整块跳过+计数（无合成）✅；漂移诊断仅描述性（未删块/未重分层）✅。

## 7. 制品

- `tools/analyze_p011_corr_matrix_27block.py`（正式统计管线，批结束直接复用）
- `test/reports/dryrun_27block_pipeline_v1.json`（dry-run 全量数值）
- 本报告：`docs/dryrun-27block-pipeline-P011-v1.md`
