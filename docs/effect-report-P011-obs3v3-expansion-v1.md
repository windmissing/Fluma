# P-011 观测③ v3 扩批报告：34 runs 完成（第一批全 PILOT_REPORT / 0 硬闸门失败）

- **日期**：2026-10-21（采集 12:00–14:22，真机 62T0225B10010020）
- **执行**：effector
- **任务**：观测③·补采至协议下限 ≥34 runs（任务 6ad62e0b，manager 19:1x 解锁）
- **被测构建**：c869cda（`entry-default-s106-signed.hap` SHA256 `e9f3f928…b6a9`，与 pilot 同构建；want 携带 schedule+metadata+身份三参数）
- **schedule**：A=18.7° / 0.5°/s（SHA256 `7b59b2e4…bd46`，与 pilot 完全一致，未中途换）

## 1. 采集总览

| 项 | 值 |
|---|---|
| 完成 runs | **34/34**（p011-v3expansion-run-001..034，runId `p011-run-1792581…-3` 系） |
| 分组 | **fit 24 + holdout 10**（随机化 manifest `calibration/randomization-manifest-obs3v3-expansion-v1.json` SHA256 `38bdd9fd…13f4`；seed=20261019，Fisher–Yates on plannedRunId，pilot 槽位永久排除） |
| overall | **34/34 = PILOT_REPORT**（无 INVALID） |
| runInvalidHardGate | **34/34 = false**（零硬闸门失败，未重试未调参） |
| executionThresholdIdentity | 34/34 ok=true（身份三字段全部落账且与 runner 绑定一致） |
| scheduleTiming | breached 合计 25/50456 命令（每 run 0–3 条，均在容忍带边界；无追赶/伪造） |
| telemetryIncompleteRecords | 全 0 |

采集 runner：`tools/run_p011_v3expansion.ps1`（每 run 独立目录、HAP/schedule/随机化三摘要闸门、check-manifest 随 run 生成、checker 自动调用）。

## 2. 观测产出（checker 汇总）

| cell | realTransitions | eligibleBrackets |
|---|---|---|
| start/+ | 260 | **0** |
| start/− | 651 | **14**（14 个不同 run 各 1 条） |
| stop/+ | 549 | **0** |
| stop/− | 903 | **0** |

- 全批 2363 个真实转换进入分母（pre-expansion ruling 的 ramp 归属修复生效——v4 单 run 时 checker 只认平台内 18 个）
- postSchedule 截断生效（H8 尾部自由演化已排除）；platformReachability 全 PASS（±30 记 NOT_IN_SCHEDULE）
- 状态样本远超门槛（每 cell 百万级边样本；§7 的 ≥30 状态样本/≥10 run 门槛在状态样本维度大幅超标）

## 3. 需 manager/designer 注意的事实

1. **eligibleBrackets=14，全部集中在 start/−**；stop/± 与 start/+ 的合格 bracket 为 0。原因（按 checker 排除链）：stop 转换在 v3 语义下要求 `prev_after==active_before` 的相邻连续子步对，而 ±A 平台尾部/回程 ramp 的状态翻转多数伴随 PREV_ENDPOINT_NOT_ELIGIBLE / NONMONOTONE 排除——与影子审计"ramp leg 激励可产出 bracket"的预测存在产出率差距。
2. **协议 §7 的 20 事件/cell 门槛**：start/− 14 条未达 20；其余三 cell 为 0。**观测③的双角标定（thetaStart/thetaStop 估计）在本批数据上不可执行**——这不是执行缺陷，是单候选 pair/单速率档在当前激励结构下的产出上限。
3. 按任务完成标准"补齐至协议下限"已达成（34 runs 全套交付）；后续路径（是否调整激励/速率档/候选、或按 §5.3 统计口径重新评估）属 designer/manager 裁决域，effector 不越权。

## 4. 交付

- 34 × (ledger.ndjson + ledger-trimmed.ndjson + hilog.txt + check-manifest.json + v3-summary.json) 于 `test/artifacts/p011-v3expansion-20261021/p011-v3expansion-run-001..034/`
- 随机化 manifest：`calibration/randomization-manifest-obs3v3-expansion-v1.json`（SHA256 `38bdd9fd83e588d513f4…`）
- 汇总脚本：`test/verify_expansion_hashes.py`；runner：`tools/run_p011_v3expansion.ps1`

*红线自查：A=18.7 与 schedule 摘要全程未变；每 run want 参数与 pilot 完全一致；零重试零调参；分组由冻结 manifest 承载（run 前 fit/holdout 已定，未按观测结果换组）。*
