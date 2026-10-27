# T150 FIELD_COLUMN 诊断 V2 独立测试（tester 报告 v1）

日期：2026-10-27。角色：tester。不使用真机。目标：T149 e3ee81f（`P011FieldColumnEvidenceV2.ets` + `P011ParticleHot.beginStepHot` 捕获接线 + `WaterSandField` 观察点/透传）。
执行器：tester 自有 `test/p150_field_v2_independent.cjs`（独立 oracle 构造，不镜像开发 13 例；raw bits 全部由测试端从原 buffer 字节独立重算比对）。结果 `test/reports/t150/p150_results.json`。

## 结论

**12/12 PASS。T149 FIELD_COLUMN 诊断 V2 独立定点验证成立：原位型捕获、首错冻结、观测无副作用、来源标记区分全部实测通过；未发现缺陷。** 开发 13/13 复跑通过仅作交叉参照；只测 V2 定点，未重跑全物理。

## 独立验证覆盖

| 用例 | 实测 | 结论 |
|---|---|---|
| V1 八非有限 | 8 个数组各自独立置非有限（Infinity/带 payload NaN 交替），每者仅置自身 bit0..7，rawBits 与测试端从 buffer 字节重算逐 hex 一致（`7ff0000000000000`/`fff8000000000001`） | PASS |
| V2 七严格负 + 不触发 | −MIN_VALUE 各置 bit8..14 且 rawBits=`8000000000000001`；**−0 库存与有限负 surface（−1）均不触发原谓词**（beginCollect 返回 0） | PASS |
| V3 sum 溢出 | bh=bl=1e308 → 左结合 (bh+bl)+mh+ml 溢出，**恰置 bit15**，sumBits 三段与测试端独立重算逐 hex 一致，其余分量有限 | PASS |
| V4 [-EPS,0) 合同差异 | bh=−5e-10：audit 容忍（无失败）而 Hot 拒绝并携带精确负 raw bits——两合同并存，**未修改任何判据** | PASS |
| V5 首列/多因 | 首个非法列 (4) 胜过后续 (9)；同列负+非有限组合出双 mask 位 | PASS |
| V6 NaN payload 冻结 | 自写 NaN 位型 `7ff8123456789abc` 捕获后 cleanup（bh 置 0+rollback）evidence 中 bits 不变；13 份 pool canonical buffer hex 长度完整 | PASS |
| V7 窗口/环 | last-legal/first-illegal 各 8192B 快照；后续再失败 firstIllegal 不被替换；35 次观察→32 环、dropped=3、首行 S3；首观察已非法=PREEXISTING_IN_WINDOW | PASS |
| V8 trace 透传 | FIELD_COLUMN 来源 driftMeasured=false/driftSource=NOT_MEASURED/rollbackExecuted=false；evidence 冻结——后续第二失败不覆盖 | PASS |
| V9 audit 来源区分 | drift 路径 driftMeasured=true/driftSource=AUDIT_GET_DRIFT——与 FIELD_COLUMN 默认零显式区分 | PASS |
| V10 无副作用 | 诊断关：evidence=null（零分配）；开：观察后 field 8 数组与 pool snapshot 与关诊断孪生场 **deepEqual 逐值一致** | PASS |
| V11 分片传输 | export JSON 经 997B 限界分片重组，rawBits/badMask/8192B hex 逐位不变 | PASS |
| V12 身份字段 | 观察行 stage/runId/poolEpoch/preview 填充完整 | PASS |

## 边界与声明

- 观察点接线（GESTURE_BEFORE/AFTER、BACKUP、APPLY_END、AUDIT_ACCEPTED_BEFORE_COMMIT 等 8 处）为只读调用，V10 双场对照证实无物理 guard/audit/rollback/派生更新增加。
- FNV32 digest 的 `digestCoverage='FNV32_FIELD7_PARTIAL_POOL_CORRELATION_ONLY'` 标记与实现一致——仅相关性，不作完整状态 SHA 消费。
- `surfaceIsSandTopScratch` 由场端补充判定实测为 true（sandTopScratch 绑定身份正确）。
- 真实设备链、签名包身份归 T151；本卡未使用真机。
- 无产品/阈值/判据变更；原 V1 证据链不受影响。

交 manager 验收。
