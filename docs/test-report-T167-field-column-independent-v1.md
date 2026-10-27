# T167 FIELD_COLUMN 修复独立测试报告

日期：2026-10-27。任务：6ae02ee1e4b0127832e18707（tester，离线）。被测 commit：**1f8e16c**（T166 工程修复）。当前树 HEAD：50ebbff。
消费：`docs/developer-T166-field-column-residual-v1.md`、T146 §5 F1 归因、`git show 1f8e16c` 实际 patch。

## 执行器与独立点

`test/p167_field_column_independent.cjs`（不镜像 T166 自测断言）：

1. **独立边界 oracle**：`nextafter` 由 DataView 位操作自算（EPS_DOWN/EPS_UP），拒绝分类由独立谓词函数驱动，不读实现常量推断期望。
2. **独立派生残差**：除 C1/C2 冻结值外另构造 D1/D2（不同分母与合成 x），验证 credit/debit 顺序记账残差语义的一般性。
3. **双通道逐值核对**：每个边界值同时核 audit(true) 与 Hot beginCollect 返回码，并对接受路径核 raw bits 保留与回滚恒等。

## 结果：46/46 PASS（原始 JSON：test/reports/t167/independent.json）

### A. 残差修复效果（4/4）

| 臂 | x | 残差（独立记账重构） | bits | audit(true) | Hot | 残差原 bits 保留 | 回滚恒等 |
|---|---|---|---|---|---|---|---|
| C1 | 0.44466802978515624 | -4.85722573273506e-17 | bc8c000000000000 | null | 接受 | 是 | 是 |
| C2 | 0.4449276657104492 | -2.0816681711721685e-17 | bc78000000000000 | null | 接受 | 是 | 是 |
| D1（独立派生） | 0.6449276657104492 | +1.1102230246251565e-16 | 3ca0000000000000 | null | 接受 | 是 | 是 |
| D2（独立派生） | 0.34466802978515626 | 0 | 0000000000000000 | null | 接受 | 是 | 是 |

注：独立记账重构得到的 C1/C2 残差 bits（bc8c…/bc78…）与 T146 披露的 -2^-55（bc80000000000000）不同——顺序加/减的中间舍入路径依赖操作数分组，属正常现象；关键判据是：残差为微负、落在 [-EPS,0)、Hot 与 audit 一致接受、原 bits 全程保留、fieldColumnEvidenceV2.failure=null、回滚后字节级恒等。四臂全部满足。

### B. waterTop 拒绝边界（11/11，Hot+audit 双通道）

| 值 | 期望 | audit(true) | Hot | 结论 |
|---|---|---|---|---|
| -1e-9（恰 -EPS） | 接受 | null | 0 | PASS |
| -9.999999999999999e-10（(−EPS,0) 内部，nextafter oracle） | 接受 | null | 0 | PASS |
| -1.0000000000000003e-9（nextBelow(-EPS)） | 拒绝 | 非 null | 3/FIELD_COLUMN | PASS |
| -5e-10 / -2^-55 / -MIN_VALUE / -0 / +0 | 接受 | null | 0 | PASS（原 bits 保留，回滚恒等） |
| NaN / ±Inf | 拒绝 | 非 null | 3/FIELD_COLUMN | PASS（badMask 非有限位、rawBits 精确、字节快照不变） |

首个被拒列证据 column=48、rawBits 为原值十六进制、field/pool/预算/commit 字节级不变。

### C. 六分量严格拒绝（21/21）

bh/bl/mh/ml/suspH/suspL 在 -MIN_VALUE、-5e-10（audit [-EPS,0) 容差内）、-1e-300 下 Hot 全部拒绝（code=3、firstIndex=4、badMask 负值位、rawBits 精确、字节不变）；-0 全部接受。
关键不对称语义成立：audit 容忍 [-EPS,0)，Hot 对六分量仍严格 `<0` 拒绝——与 T166 报告"仅 waterTop 对齐 EPS"一致。
夹具注记：suspH/suspL 容差用例须用 wet 夹具（dry 演示 mediumMode 要求 susp 严格 0，负值先触发 DRY 分支而非 NEGATIVE_VALUE，audit code=3 实测确认）；首轮 6 个 FAIL 为本执行器夹具假设错误，修正夹具后全过，无产品缺陷。

### D. 首错列与跨列竞争（2/2）

bh col4（早） vs waterTop col10（晚）→ column=4；同组件 waterTop col3（早） vs col9（晚）→ column=3 且 rawBits 为早列值。

### E. raw sign 诊断与 Hot 接受并存（1/1）

waterTop=-2^-55 注入后：Hot 接受（code=0）、exportEvidence status=NO_FAILURE、failure=null、windowStatus=LEGAL_TO_INVALID、末 stage firstInvalidColumn=48 且 legal=false、firstIllegalBytes 读回恰为 -2^-55、状态字节不变。**"raw stages legal=false 与 Hot 接受及 NO_FAILURE 并存"语义成立**——raw 符号诊断不构成产品 FIELD_COLUMN 失败。

### F. 导出优先顺序/分片/首错锁存（1/1）

failure/stages 键位于 lastLegalBytesHex 之前；完整导出 512B 分片 round-trip 逐字节一致；二次不同错误（suspH col2 NaN）不覆盖首错（同对象引用、column 仍 48）；两快照各 16384 hex 字符；poolCanonicalBytesHex 13 组且 poolStorageValid=true。

### G. 观测开关等价（1/1）

观测开/关两实例（含 -2^-55 注入、observe、begin/rollback 两轮、再 observe）后 field 八数组/pool/source/预算/commit 字节级全等。注：窗口因先 observe 后注入故报 PREEXISTING_IN_WINDOW（首 stage 已非法），属窗口语义而非等价性缺陷。

### H. 源码身份与 patch 范围（2/2）

- `WATER_TOP_AUDIT_EPS=1e-9` 与 audit `EPS=1e-9` 一致；Hot 谓词 `f.waterTop[col] < -WATER_TOP_AUDIT_EPS`；Hot 中 `f.waterTop[col] < 0 ||` 已不存在。
- `git show 1f8e16c --name-only` 产品源码改动仅 P011ParticleHot.ets + P011FieldColumnEvidenceV2.ets 两个文件，与 T166 报告范围声明一致。

## 相关回归（J3 规定范围，全部在本树实跑）

| 套件 | 结果 |
|---|---|
| T167 独立执行器 | 46/46 PASS |
| T166 自验 | 9/9 PASS |
| T149 证据单元（含 T166 修订后用例） | 13/13 PASS |
| Hot / patch / sink / M2 | 11 / 12 / 17 / 13 全 PASS |
| T85 tilt trace | 7/7 PASS |
| T123 merge index 差分 | 155 cases 0 failed |
| T154 pool progress | 8/8 PASS |
| p059 dry admission | 全 PASS |

## 结论

T166 1f8e16c 的物理修复（waterTop 拒绝阈值 -1e-9 对齐 audit）与工具修改（导出优先顺序）在独立执行下全部验证通过：C1/C2 残差经 Hot+audit 双通道接受且原 bits 保留；新拒绝边界恰在 nextBelow(-EPS)；六分量严格拒绝与 audit 容差不对称成立；raw 符号诊断与产品失败语义正确分离；观测无副作用；patch 范围与声明一致。**T167 无阻断结论成立**，设备两臂 J2 及 LAND/手持复测按流程归 T95。本报告不含设备恢复或端到端产品 PASS 结论。
