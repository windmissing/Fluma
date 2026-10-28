# T186 首错 V2 独立测试报告

日期：2026-10-28。任务：6ae0b709e4b0f11bdf1e2214（tester，离线）。消费：T185 1f762cc（`developer-T185-first-physics-failure-v2.md`），按 T181 §4.3 场景规格。被测 commit：1f762cc。

## 执行器与独立点

`test/p186_first_failure_independent.cjs`（10 用例）。独立性：按 T181 §4.3 场景逐条独立构造，不镜像 T185 开发自测结构——新增开发自测未覆盖的独立断言：导出独立副本可变性（外部改写不污染）、抛异常 trace 不掩盖物理失败、交错生命周期 100 stop 边界、诊断捕获不改物理 totals、导出确定性、身份组成语义核（installTime 不在 V2 身份字段）。

## 结果：10/10 PASS（原始 JSON：test/reports/t186/independent.json）

| # | 场景（T181 §4.3） | 结果 | 关键证据 |
|---|---|---|---|
| 1 | 非 5 失败原码 1..6/null 逐值 + code5 | PASS | rawCode 逐字记录（null 不填 0）；sticky 后 step 调用数不变（只计一次不重跑）；repeatCount 关联同一 firstFailureId |
| 2 | 导出为独立副本 | PASS | 外部改写 rawCode/firstFailureId 后二次导出仍原值 |
| 3 | 缺 trace / 抛异常 trace | PASS | 两者均 stage=null、m2Code=null、traceStatus=NOT_CAPTURED（未知不填 0）；诊断异常不掩盖物理失败 |
| 4 | reset 后新 run | PASS | previous 冻结旧导出（firstFailureId/lastGoodCommit=42 保留）；current 清空；新 run runId 不同、modelGeneration 单调增、lastGoodCommit=null |
| 5 | 交错生命周期 | PASS | 100 次 page stop：lifecycle 封顶 8、dropped>0、repeatCount=100、全部行关联同一首错 |
| 6 | 成功准入后 step 失败 | PASS | sourceSequence/Epoch/externalAdmissionCount After=Before+1（准入不回滚） |
| 7 | 交错进程身份 | PASS | 两实例 firstFailureId 不同（runId:generation:frame 组成，各自 runId） |
| 8 | 同 installTime 不同包 | PASS | V2 身份字段由 runId/generation/frame 组成，导出结构中无 installTime 字段——身份不依赖 bundle 级 installTime，不同包各自 runId/generation 天然区分 |
| 9 | 注错不暴露产品恢复 | PASS | 捕获前后 field totals JSON 相等 |
| 10 | 导出异常隔离 | PASS | 物理结果不受影响，多次导出一致 |

## 基线

T185 开发自测 11/11 独立复跑 PASS（本会话确认）。

## 边界

- 设备日志截断归 T187 真机（本卡不含）。
- 隔离夹具注错仅测试侧，未添加产品自动恢复接口。
- 失败上报：无。无阻断。
