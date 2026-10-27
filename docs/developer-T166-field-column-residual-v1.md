# T166 FIELD_COLUMN 残差口径工程修复

日期：2026-10-27。任务：6ae02ee1e4b017ab34116ed9。依据 T146 §5 F1 与任务正文；不使用真机。

## 物理修改：waterTop 检查对齐 audit

唯一物理修改是 `P011ParticleHot.ets` 的 beginStepHotBody FIELD_COLUMN 判据：
waterTop 原先 `<0` 拒绝，现在 `<-1e-9` 拒绝，等于 `-EPS` 允许，与 WaterSandField.audit 使用的 EPS=1e-9 相同。
非有限值仍拒绝；bh/bl/mh/ml/suspH/suspL 六分量仍严格 `<0` 拒绝；surface 有限、左结合和有限、库存/容量/预算/守恒/λ/η 等既有判据不动。
这明确改变了 waterTop 的负值拒绝边界，不能表述为“全部阈值未变”。

未 clamp 或改写缓存，未启用 dyadic：`-2^-55` 仍以原 bits 留在 field 和池列缓存中；本次消除预览与 audit 的 waterTop 判据不一致，未根除 credit/debit 舍入残差。

## 工具修改：导出优先顺序

`P011FieldColumnEvidenceV2.exportEvidence` 把 failure/stages 移至快照 hex 前；不更改 schema revision、完整两份 8×128×8B 场快照、13 组池字节、首错锁存、32 条窗口、只读及分片合同。
该改动仅调整导出优先顺序。failure 自身也含完整池字节；固定字节上限仍可能截断，不能保证任意长度 payload JSON 完整、不能宣称“详表必达”。

诊断原始符号 scan 刻意保持 `<0`：允许的 waterTop 微负残差仍可在 stages 中显示 legal=false / LEGAL_TO_INVALID，并保留原 bits；此为原始符号观测，**不等于 Hot FIELD_COLUMN 拒绝**。没有 Hot 失败时 status=NO_FAILURE、failure=null，不能把诊断 raw sign 与新的物理谓词混用。

## 自验

`node test/run_t166_field_column_residual_units.cjs`：9/9。

- C1 X=0.44466802978515624 / C2 X=0.4449276657104492：credit 四个 .1 加尾包、同序 debit 均精确重构 `bc80000000000000`（-2^-55）；重构搬运后池/场，beginCollect 和 audit(true) 通过；rollback 不改变原 bits、场/池/预算/来源。
- `-EPS`、区间内部、最小微负、±0 接受；`-1.0000000000000003e-9`、更负值、NaN/±Inf 拒绝，错误列/原始 bits 留证。
- 六个其它分量即使 -MIN_VALUE 仍拒绝；观测开关物理状态一致；完整导出可分片复原、首错不覆盖。
- T149 原严格 waterTop 用例按批准 F1 修订为 `<-EPS` 反例，保留六分量精确拒绝和原 bits 断言；未删除失败断言。

T146 J3 规定回归全部通过：Hot 11、M2 13、patch 12、sink 17、T149 13、T85 7、T123 155 cases；另 T154 8 与 dry admission 回归通过。逐套日志及退出码见 `test/reports/t166/`。

主工程 hvigor assembleHap：Native、CompileArkTS、PackageHap 通过；SignHap 仍为既有 00303116（storePassword/keyPassword 小于32字符），未改签名配置。构建日志 build.txt、产物身份 build-manifest.json。

T167 负责独立边界/残差回归，T95 负责设备两臂 J2、LAND/手持；本报告不提供设备恢复或端到端产品 PASS 结论。生成隔离 GPU 包会继续带自己的 source manifest；T165 已归档的 unsigned 包不因主工程随后提交而悄然替换。
