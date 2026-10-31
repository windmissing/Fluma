# T332 零分配工具收口三项（离线，完成）

2026-10-31，tester 离线。依据 `docs/architecture-T325-zero-alloc-closing-ruling-v1.md`（§3 工具收口三项）+ manager 2026-10-31 裁决（Date.now 保留、PROOF_GAP 改判接受的成本）。红线遵守：不碰路径 1（方舟运行时原语摘要工具）、不做无据白名单、不改 ZERO 断言、静态 FAIL 维持（NOT_ZERO_ALLOCATION/164 不变）。

## 第 ① 项：39 显式位点热路径可达性标注（T162 §2.1 判据复用）

产出：`test/reports/t332/t332_site39_hotpath_disposition.json`（39/39 逐条，HOT 附调用链）。

| 标注 | 数量 | 说明 |
|---|---|---|
| **HOT** | **6** | ① `TxPairAdjAvl.extraI:91` 数组字面量[8 引用]——调用链 copyIndex←TxWholeTransactionCore.copy(:109/:121)←txD1.copy←copyPoolHot(P011ParticleHot:898)←preparePatchHot(:1101/1111)←M2Sink.applyInto(:232)←applyParticlePlanIntoBody←**每步 apply**；② `DerivedMutationTokenV2.mint:19`——begin(:112)←derivedV2.begin←mergePairHot(:970)/editPoolHot←每步 apply 事件；③-⑥ `editPoolHot 'ROW:'+kind`（:1024/1052/1061/1070）字符串拼接——editPoolHot←preparePatchHot(:1105)←applyInto←**每步 apply 全事件（kind 1/2/3/4/6）** |
| COLD_FAILURE_ONLY | 32 | `throw txError as Error`（29，catch 重抛——正常热路径不进 catch）+ `new Error(...)` 协议违约路径（3：DERIVED_UNREGISTERED_COPY/DERIVED_UNSUPPORTED_RAW_IMPORT/DERIVED_FIELD_MARK） |
| COLD_EXPORT | 1 | `checkLong:122 new Uint8Array`——VIEW_CONSTRUCT（T162 已判），仅 span≥100ms 且 longCaptured=false 单次触发 |
| CTOR | 0 | 无类构造器位点 |

**结论**：T320 遗留的「39 位点正常热路径可达性仍未证」闭合——**6 处 HOT 位点实证在每步 apply 正常流执行**（不是可达性问题，而是真实存在的热路径分配位点），零分配形式门（NOT_ZERO_ALLOCATION/164）的 FAIL 在这些位点上**有据维持**，不放宽。

## 第 ② 项：净 18 的逐条 reason 对账表（闭合 4+37−32=9≠18 缺口）

产出：`test/reports/t332/t332_net18_reconciliation.json`（36 行逐条：27 新增 + 9 消失，净 18）。

- 对账口径 = T316 原文口径（file/owner/text/reason 多重集），对 `test/reports/t311/static-{before,current}.json(.gz)` 复现（两文件逐位一致校验通过）。
- **可复现结果：新增 27、消失 9、净 18**——T316 手写的「28/10」与可复现口径差 1 条双边（净 18 恒成立不受影响；候选原因=一条镜像位移行被双侧各计 1，如实披露在 JSON `t316_handwritten` 字段）。
- reason 级完整分解（闭合 T320 的 9 条缺口）：**净 18 = 4（Date.now：begin/end × UNRESOLVED_CALL/UNKNOWN_CALL_OR_ALIAS_ESCAPE，T311 新热时钟原语 PROOF_GAP，manager 裁决保留）+ 23（CONTEXT_LIMIT_FAIL_CLOSED 新增：receipt.begin/end/syncHotContext/seal/storage 校验链接线扩大调用图）− 9（CONTEXT_LIMIT 消失：txD1.syncContext→syncHotContext 替换、TxWorkTrace.leave 内联合并）**。4+23−9=18 闭合。
- 全部 36 条逐条附 reason 归因（证明缺口/工具未实现层，无一条为真实新增 `new`——与 T320 结论一致）。

## 第 ③ 项：旧 2 内建效应并入独立分母

- `P011PoolProgressDiagnostic.checkLong:122 new Uint8Array(buffer)`（两层内建效应唯一位点）沿 T162 §2.1 判 **VIEW_CONSTRUCT**——已并入第 ① 项标注表的独立分母（COLD_EXPORT 行，附触发条件：span≥100ms 长窗单次捕获）。
- 不计入主门禁分母（T162 原语义），ArkTS 运行时是否为视图构造分配头对象仍未实测（T162 原注保留，不臆测）。

## 结论

- 三项工具收口产出齐：39 位点标注（HOT 6/COLD_FAILURE_ONLY 32/COLD_EXPORT 1）、净 18 对账表（36 行闭合，4+23−9=18）、checkLong VIEW_CONSTRUCT 独立分母。
- **静态门维持 FAIL**：`NOT_ZERO_ALLOCATION/164` 不变——6 处 HOT 位点为真实热路径分配证据，FAIL 有据；Date.now PROOF_GAP 按 manager 10-31 裁决保留为接受的成本。
- 不阻 GPU（T316/T325 原判维持）。

证据：`test/reports/t332/{t332_site39_hotpath_disposition.json, t332_net18_reconciliation.json}`；输入 manifest `test/reports/t311/static-{before,current}.json(.gz)`。
