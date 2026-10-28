# T179 材质首错诊断独立测试报告

日期：2026-10-28。任务：6ae090f6e9adb6000000080c（tester，离线）。消费：T178 64ca085（`developer-T178-material-rejection-v1.md`），按 T176 §5 场景规格。

## 执行器与独立点

`test/p179_material_independent.cjs`（11 用例）。独立性：SOURCE 谓词 7 组合独立枚举（不沿用开发抽查顺序）、LAYOUT 多维独立触发、捕获前缀/隔离快照用独立参考 hex 重算、read-only 断言用"失败时刻状态独立重构 + 后续变异排除"双判据、receipt 重附语义（last-writer-wins）如实核。

## 结果：11/11 PASS（原始 JSON：test/reports/t179/independent.json）

| # | 场景 | 结果 | 关键证据 |
|---|---|---|---|
| 1 | SOURCE 谓词枚举 | PASS | 7 组合位精确：top/water NaN（1/2）、bottom<0（16）、bottom>mix（32）、mix>top（64）、top>water（128）、water>capacity（256），stage 全 SOURCE_VALIDATION |
| 2 | LAYOUT 多维 | PASS | boxW=0/boxH=0/boxW=-5 全 MATERIAL_DATA_LAYOUT |
| 3 | F32 前缀捕获 | PASS | 有效前缀截断（capturedBytes>0）；快照 detached（源重填后 rawHex 不变） |
| 4 | read-only 源数组 | PASS | bh raw hex 等于失败时刻独立重构值（含 -1），后续源变异（77/88）不进入捕获 |
| 5 | firstFailure 不被覆盖 | PASS | 5 次错误 token attach 后首错 frameToken 不变、receipt 保持 null |
| 6 | 正确 token receipt | PASS | 全状态字段附着（code/presentationState/originalFailureStage/presentOldStatus=3/swapOldStatus=4）；重复 attach 为 last-writer-wins（实现语义如实记录） |
| 7 | 首成功/首失败不可变 | PASS | 20 次重复 → repeatCount=20、ring 8、dropped>0；新 run 清 current、runId 谱系保留 |
| 8 | 未执行语义 | PASS | presentOldStatus/swapOldStatus null 保留（不填 0） |
| 9 | 字节预算 | PASS | captureBytes≤28752 容量；captureTimeMs=null；missingReason 含 CAPTURE_TIMING_NOT_CAPTURED 与 PACKAGE_SOURCE_IDENTITY_EXTERNAL_MANIFEST |
| 10 | 未知 throw | PASS | UNEXPECTED_EXCEPTION + RangeError name/message 原样；诊断异常不翻成功 |
| 11 | P0/P1 场景存活 | PASS | 夹具 wet/dry 上下文可用（T56 窄回归另行复跑 5/5） |

## 语义记录（如实）

- `attachReceipt` 同 token 重复附着为 **last-writer-wins**（每次直接赋值）——非幂等；开发文档未承诺幂等，如实登记，不判缺陷。
- 捕获 bytes 冻结于**失败时刻**：失败后源 buffer 继续变异不污染已捕获证据（detached 快照语义成立）。

## 基线

T178 开发自测 17/17 复跑 PASS；T56 scene 单元 5/5 PASS。

## 边界

- 设备首错原 bits 归 T180 真机取证（本卡离线）。
- 失败上报：无。无阻断。
