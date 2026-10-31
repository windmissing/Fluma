# T323 · V2 续证失败首次分流（包 SHA / 累计审计 / 冷采预算）v1

日期：2026-10-30。责任：architect。任务卡：滴答 `6ae44d63e4b0efba6942fb6d`。
状态：**SHA_DIFF_ATTRIBUTED / COMPLEXITY_ENGINEERING_ROUTE / COLD_BUDGET_DESIGNED**。
依据：`test-report-T312-v2-complexity-device-v1.md`、`test/reports/t312/{stage_all.txt,anchors.txt,tb59315.log}`、`test/reports/t311/delivery.json`、`.codex/local-test/t257-sign/T311*/`三签包、`developer-T311-v2-work-bound-v1.md`。纯归因零生产代码。

## 1. 包 SHA 差异归因（已闭合，非交付链断裂）

T312 报告登记：实测安装包 SHA `366d1ca6bdc2…` ≠ 卡面 `9e7ac541…`。本卡取证结果：

| 包 | SHA256 | 源变体 | 签包时间 |
|---|---|---|---|
| `.codex/local-test/t257-sign/T311/T311-signed.hap` | `366d1ca6…`（**实测安装包**） | **final**（bind 无 `stageCounts.fill(0)`） | 14:34:22 |
| `.codex/local-test/t257-sign/T311-final/T311-final-signed.hap` | `9e7ac541…`（**卡面值**） | **candidate**（bind 含 `stageCounts.fill(0)`） | 14:38:10 |
| `.codex/local-test/t257-sign/T311-production/…` | `e7ead8ee…`（delivery.json productionSign） | — | 14:34:28 |

**唯一代码差异**（zip 逐项 diff + sourceMap VLQ 解码定位）：`T284Cold.ets :18` 的 `bind()` 中一句 `this.stageCounts.fill(0);`——candidate 包含、final 删除。其余全部模块（含 P011HotWorkReceipt 等 T311 主体改动）**逐位相同**。

**语义影响判定**：`stageCounts` 是 T311 新增的 20-stage 计数数组；`bind()` 在 run/epoch 变更时重置诊断状态。final 删除该句意味着**跨 run 复用 T284Cold 诊断实例时 stageCounts 不清零**——纯诊断计数问题，不影响物理/守恒/审计主路径。T312 四 run 数据里 stage 计数正常产出（stage_all.txt 32 行），说明设备 run 内 bind() 每次都发生在新 run 启动时，且跨 run 未复用同实例或计数未受污染——**T312 的取证数据不受此差异影响**。

**归属与处置**：
- 差异源于 developer 在 candidate 签包（14:34:22 前后）后、final 签包（14:38:10）前的最后一次源修订——两包都经同一签包链（verified-profile 同 uuid `fe686e1b…`、verifiedPassed=true），**不是未签/伪造包**；
- tester 安装了 `T311/T311-signed.hap`（final 变体，无 fill(0)），而卡面引用了 `T311-final/`（candidate 变体，含 fill(0))——**登记为 developer 交付清单引用错误**（小修：T311 卡面 SHA 应改指实际交付包，或补一句两包差异说明）；不构成重跑 T312 的理由（§语义影响判定）；
- `stageCounts.fill(0)` 删除是否为故意（final 版本为准）还是回退失误，属 developer 交付说明义务——**下次 T284 冷采相关卡要求 developer 在 delivery.json 里登记两个变体的取舍**，本卡不代答。

## 2. 累计复杂度分流（工程路线，不转 designer）

T312 核心数据：apply 单事件 5.45ms/R 线性、R=261 时 1449ms、两 run 死于 THREAD_BLOCK_6S（R>~250 持续注入）、audit 1165ms/sort 777ms 累计。T323 卡面提醒：5.45ms/R 是**有限样本拟合**，R261 的 1449ms 不能外推「R≈1102 必杀」为唯一死因——本卡核实并确认该提醒有效：watchdog 死因链是「apply 单事件 1449ms > 6s？否——但外层 attempt 累计（ΣR × 每 R 成本）+ audit 1165 + sort 777 的**叠加**才超 6s」，T312 报告 §二「两 run 死于 R 260+ 持续注入后的延迟死亡，栈均为 audit→applyInto→applyParticlePlan」的表述与叠加解释一致，**不需要修正为单事件直杀**。

按同 attempt inclusive 阶段核整体复杂度（T311 交付的界表 + T312 实测互证）：
- 逐行 audit `Ω(R(N+C))`：T312 实测 audit 1165ms@R261 ≈ 4.5ms/R——**与 apply 同量级的主导项**；
- sort `O(L log L)`：777ms 累计（每次 audit 内 heapsort）；
- 外层重复：watchdog 栈显示死时在 audit→applyInto 链上——**attempt 级 ΣR 累计是实际杀手**，单 R 外推不是。

**分流裁定：工程问题 → developer（增量证书设计），不需要 designer 新不变量**。理由：
1. T311 交付已预留路径（「要把逐行全池审计安全合并为增量证书，必须另定不变量证书、可变 callback/派生缓存/重放与原错误顺序的设计」）——这是**接口工程**（证书数据结构 + 缓存失效逻辑），不改变物理不变量本身（守恒/库存/容量/有限值检查语义原样，只是从「每行全池重扫」变「增量维护+定期全验」）；
2. 冻结约束全保留：不改 dt、不跳审计、不降阈值——增量证书是**审计的等价加速**（每 K 行做一次全池验证 + 行间增量），错误顺序保持（先验身份后验守恒）；
3. 设计侧无新自由度：证书结构由现有 audit 谓词（有限/非负/容量/守恒分母）唯一决定。

**工程合同要点（developer 实施卡输入）**：
- **增量审计证书**：`auditParticleHotBody` 的逐行 O(N+C) 全池谓词 → 维护「上次全验通过的基线 + 逐行 Δ」；每行审计 = 检查该行涉及列的增量一致性（O(该行列数)）+ 每 K=16 行（或 attempt 结束）一次全池重验；全验失败回退逐行模式并保留错误码语义；
- **audit-scope sort 消除**：`sortOrderHot` 每次审计对 live 池重 heapsort——池序在 apply 事务间不变（除 birth/death/merge），维护**有序索引的增量维护**（插入/删除 O(log L)）替代整池重排；T123 MergeExactIndex 的 groupOrder/idOrder 双序先例可复用；
- **attempt 级预算披露**：T311 stage 计量已按 attempt 记——补 `attemptCumulativeMs` 字段让 watchdog 风险可直接读出（R 累计 + 当前 Σ 耗时），不新增测量点；
- **验收**：N512/R64 夹具结果逐位相同（沿 T311 五档对照）+ watchdog 死亡 run 复现族（R 持续注入到 300+）单事件 ≤600ms 且 attempt 总耗时 <6s（留 10× 余量）。

## 3. 冷采预算（BUDGET_CAPTURE_MS 19/46ms 超 4ms）

T312 实测：FIRST_BIRTH 与 SINCE_BIRTH_8S 锚点触发但冷采 INCOMPLETE（captureMs 19.3/46.5ms 超 4ms 预算）。核捕获阶段：冷采在锚点触发时做「场+池+账本全量快照」（T284 规则），数据量 = 8×128×8B 场 + 13 池 buffer + 触发时刻 ledger——主要耗时在池 13 buffer 的逐字节拷贝（N=512 时 55×512×4×2 ≈ 225KB）+ JSON 序列化头。

**可分段/不可分原子快照裁定**：
- **不可分（必须原子）**：8 场数组 + 池 13 canonical buffer——这些是「触发时刻的一致状态」，分两次读会出现撕裂（物理步进中）；
- **可分段（可延后）**：ledger/profile JSON 序列化——纯文本导出，可在锚点时刻只拷贝原始 buffer、序列化延到下一次 idle（1Hz 冷 timer 窗口内已有先例：T311 的 stage 行是「固定数组冷取」非即时序列化）。

**工程合同（developer 小改，并入 §2 实施卡）**：
- 锚点触发时：原子 memcpy 场+池原始字节到预分配 capture buffer（一次 ~225KB+8KB 拷贝，memcpy 级 ≤2ms）→ captureMs 应回到 4ms 内；
- 序列化延后：`exportCold` 在 1Hz timer 窗口做（T311 `emitProgressCold` 先例），锚点只记 `{anchorId, bufferValid, captureMs}`；
- 预算不动：4ms 预算与 T284 INCOMPLETE 规则原样保留——若原子拷贝后仍超 4ms（设备 memcpy 慢于预期），如实 INCOMPLETE 不放宽；
- **NOTRUN LAND/像素保留**：冷采 INCOMPLETE 导致 T312 的 LAND→床量→同 token 像素链 NOT_RUN——本合同落地后该链可在下次真机卡复测，**不因本卡代测**。

## 4. 路由汇总

| 项 | 路由 | 卡 |
|---|---|---|
| 包 SHA 引用错误 | developer 交付清单小修（说明两变体取舍） | 并入 §2 实施卡附注 |
| 复杂度（audit/sort/attempt） | developer：增量证书 + 索引维护 + attempt 累计披露 | 新建实施卡（T323 工程） |
| 冷采预算 | developer：原子拷贝+延后序列化 | 并入同一实施卡 |
| LAND→像素链复测 | tester 真机（冷采修复后） | 既有 T95 链或后续真机卡 |
| designer | **不转**（无新物理不变量需求） | — |

## 5. 变更记录

- 2026-10-30 v1：初版。SHA 差异归因（T284Cold.ets:18 单句差异、final/candidate 变体、语义无碍、登记引用错误）；复杂度工程分流（增量审计证书+排序索引维护+attempt 预算，冻结约束全保留）；冷采原子拷贝+延后序列化合同；LAND/像素链留待复测。
