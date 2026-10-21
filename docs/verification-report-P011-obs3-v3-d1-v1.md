# P-011 观测③ v3 核验报告（五项清单 + D-1 整改核验）

- **日期**：2026-10-21。执行：tester。
- **被测构建**：commit `742efa6`（[P-011] Observation-3 D-1 remediation: legacy bit-exact restore + explicit correction route isolation）。**原核验结论已废弃**（被测构建行为漂移），本报告全部在 742efa6 上重跑。
- **前置确认**：architect D-1 复核交付、manager 审核通过，D-1 `RESOLVED`；本任务解锁。checker 用例已提前就位（55d18ae）。
- **边界遵守**：不改物理、不降门槛、不授权采集；核验跑在 Node/checker 层与既有合成 evidence 上。

## 核验项 1：执行身份 —— **PASS**

- checker 能区分 configured/executed：`p011_obs3_v3_protocol_check.py` 的 `executed_identity_ok()` 要求 `executedThresholds` 携带 `thetaStartDeg/thetaStopDeg/slopeStart/slopeStop` **加** `stateMachineVersion/sourceCommit/sourceFileSha256/buildSha256` 四项实现身份；仅复制 manifest 到日志不满足。身份不一致或缺失 → `EXECUTION_THRESHOLD_IDENTITY_MISMATCH` 且各 cell 携带该码（合成验证实测：缺 executedThresholds 的 ledger 四 cell 全部命中该码，估计 null）。
- 候选阈值实际驱动状态机（真实代码路径验证）：候选 run（armed 26/20 + natural route）运行 10 步，ledger `context.executedThresholds` 非空且 `thetaStartDeg=26 / thetaStopDeg=20` 与配置一致，实现身份字段齐备（stateMachineVersion=routeId 绑定、sourceCommit=742efa6、双 SHA-256 非空）——v2 的 `EXECUTION_THRESHOLD_IDENTITY_MISMATCH` 根因（thresholds 未进状态机）确认已修。
- decision 字段落位：`relaxHysteresis.edges[*]` 每边携带 `decisionG/decisionR/decisionAlpha/decisionOrdinal/thresholdUsed{branch,angleDeg,comparator}/transitionReason`，实测 70/70 边字段完整（采样边：alpha=0、reason=HOLD_INACTIVE、threshold=start@26 comparator=>=）。

## 核验项 2：相名/完整性 —— **PASS**

- v3 新序列相位映射：`P011ScheduleV3.ets` 实现九平台 H0..H8 + 八 leg L1..L8、5100ms hold、100ms 命令步距、半开区间归属（终点命令归下一平台）、CSV 契约（列序/无 BOM/LF/末行 LF/禁科学计数法）；单测 `p011_scheduleV3_generationAndCsvContract` 等在 runner 76/76 内绿。
- 九相执行证据完整：v3 checker 对合成九相 ledger 按 ±15/±30 相分别输出 `platformReachability.perPhase`（stableRecords/firstStableMs/accepted 范围/plannerLimitedStableRecords）；第 9 回零段提交记录由 `baseline`/`*Return` 相位名承载（5e60f2d 映射 + GAP-6 增补的 v2 九相名），无提交记录的相位如实报 `PLATFORM_UNREACHED`/`PLATFORM_UNSTABLE`，不伪装。

## 核验项 3：硬安全 —— **PASS**

- 全部 MUST-PASS 原样生效：tan30/planning、逐 writer guard、local/bottom/total 预算、守恒、库存、容量、有限值、遥测对齐、边身份、stop 语义由既有 safety checker（`p011_slip_formal_check.py` schemaVersion 2 + `p011_v210_natural_observation_check.py` contract）承载，v3 通道零豁免、零复制、零弱化（GAP-6 增补九相名未动 `SKIP_TAN30_PHASES`）。
- 回归实测：safety checker 对 v1 合成 evidence 判定不变（同 FAIL 语义、schemaVersion 2）；runner 76/76 全绿（含 4 个 D-1 用例、4 个 v2/v3 闸门通道回归）。
- BLOCKED attempt 可审计（真实代码路径验证）：preexisting violation 场景 step 返回 `ok=false errorCode=5`，ledger 落账 `attemptId=p011-run-…-a1`、`commitStatus='blocked'`、`rollbackReason='PREEXISTING_INFEASIBLE_STATE'`、planner telemetry（thetaRequested/thetaAccepted）在账——被拒 attempt 可审计但不算观测样本。

## 核验项 4：新 ramp 资格过滤 —— **PASS**

- 预注册 leg（八 leg 表）、bracket ≤0.5°（宽事件 `BRACKET_TOO_WIDE` 不切分）、首转折规则（每 `(runId, segmentId, fixedEdge)` 时间序首个）、3 点单调窗口（不向前搜索）全部在 `p011_obs3_v3_protocol_check.py` 实现（§5.2 公式逐条落地），决策残差字段（decisionG/R/Alpha 非空）在核验项 1 实测 70/70。
- 合成单测 `test_p011_obs3_v3_protocol_check.py` **38/38 绿**（742efa6 重跑），覆盖宽度 0.5° 边界值两侧、单调/非单调、首转折取舍、每 run ≤3 cap、覆盖矩阵结构。

## 核验项 5：输出规则 —— **PASS**

- checker 分支齐全（codeCatalog 实测 12 码）：`RIGHT_CENSORED_AT_TAN30`（v3 新定义四条件）、`STOP_TRANSITION_UNOBSERVED`（九细分）、`START_OUTSIDE_ELIGIBLE_WINDOW`、`PLATFORM_UNREACHED/UNSTABLE`、`TELEMETRY_INCOMPLETE`、`RUN_INVALID_HARD_GATE`、`EXECUTION_THRESHOLD_IDENTITY_MISMATCH`、`EXECUTION_ENVELOPE_UNRESOLVED`、`SHADOW_PREDICTED_CENSOR`、`BRACKET_TOO_WIDE`、`NONMONOTONE_LOCAL_WINDOW`；`INCOMPLETE` 由 `pilotCoverage=INCOMPLETE` + cell deficit 表达；全部不产点估计，`thresholdVerdict=NOT_EVALUATED` 恒定。
- 身份失败传播实测：缺 executedThresholds 的 ledger → 四 cell 携带 `EXECUTION_THRESHOLD_IDENTITY_MISMATCH + TELEMETRY_INCOMPLETE`，overall=INVALID。

## 核验项 6：D-1 整改核验 —— **PASS（D-1b 遥测按裁决定位如实记录，不阻断）**

- **D1-C1 LEGACY 轨迹对比 PASS**：742efa6 上重跑，240/240 ok、零 BLOCKED、14 个 5e60f2d 冻结锚点全过（含 step25/step214 两个历史分歧点）——LEGACY 位精确已恢复。
- **D1-C2 双条件资格门分流 PASS**：四场景（legacy 默认/候选无 route/显式 correction route/route 绑定但机器 off）路由全正确；legacy 路线 D-1 缺陷签名（requested>0 且 allowed=0）零命中；correction 路线 slot 实际运行。
- **D1-C3（D-1b）如实记录 RED**：`correctionRequested/Granted/Rejected/StalledCount` 尚未进 `P011NaturalObservationAudit`（代码只有私有 `correctionStalledCount`，audit 接口无此四字段），checker 判 `RED_BASELINE`。按裁决第 3 条此为 correction 路线效果报告的放行前置，**不阻断本核验与 v3 tester 链路解锁**；转绿前 correction 效果报告不具备放行资格。
- manager/architect 既有证据复核一致：Node 81/81（verify_d1_final.json 零失败）、d1_check_verify.json（C1/C2 PASS）与本报告独立重跑结论相同。

## 结论

六项核验全部 PASS（其中 D-1b 遥测按裁决定位如实记录 RED，不阻断）。观测③ v3 tester 链路解锁条件达成；pilot 启动待 manager 批准。遗留：D-1b 遥测四字段落 audit（developer，correction 路线放行前置）；本报告不追认任何历史 run，不发布阈值估计。

**证据清单**：`test/artifacts/p011-obs3-d1/d1-verification-evidence-742efa6.json`（本轮重跑）、`d1-verification-result-742efa6.json`、`test/reports/verify_d1_final.json`（81/81）、`test/reports/d1_check_verify.json`（C1/C2 PASS）、`test/artifacts/p011-slip-formal-v1/v3verify-exec-identity.json`（身份/相名/输出码）、runner 76/76（76 用例含 4 D-1 用例）、v3 合成单测 38/38。
