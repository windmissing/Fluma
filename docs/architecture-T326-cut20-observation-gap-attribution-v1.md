# T326 · 观察注入缺档首次归因（cut20 三行与 checker 语义）v1

日期：2026-10-30。责任：architect。任务卡：滴答 `6ae45280e4b0f69bc573fc81`。
状态：**ROOT_CAUSE_IDENTIFIED / NOT_A_SERIALIZATION_DEFECT / FIX_IS_OPTIONAL**。
依据：`test-report-T319-tx-cut-checker-v1.md`、`test/reports/t319/checker_summary.json`、T304 原档、`architecture-T314`（cut 字典）、`.codex/local-test/t250-before-v30/entry/src/main/ets/physics/tx/{TxBeforeFault.ets,TxGpuFixture.ets}`、`.codex/local-test/t250-before-v30/entry/src/main/ets/physics/{WaterSandField.ets,P011NaturalObservation.ets}`。纯归因零生产代码。

## 0. 结论先行

**observationRows=[] 是注入设计的必然结果，不是序列化/采集缺陷**——BEFORE 边注入点在 `finishSubstep`（唯一 sink 调用）**之前**抛出异常，sink 永不执行，观察行按构造缺失。**不需要 developer 补序列化**（T319 报告「交 developer 小改补三行序列化」的建议被本卡推翻），缺证边界如实保留。

## 1. 缺失三行的产生机制（代码路径级证据）

cut20（ENDPOINT_FIXED_CLOCK，观察提交档）的注入与观察链（t250-before-v30 源码）：

```
TxGpuFixture.ets :21   observationRows=[]；setNaturalObservationProbe(P011NaturalObservationProbe(128, cb))
                       └─ cb = (record) => observationRows.push(JSON.stringify(record))
WaterSandField.ets :4146-4155（每子步 audit 后、finishSubstep 前）：
  bf20.begin(20, rootId, scope, {stamp})        ← BEFORE_INPUT 行在此记录（TxBeforeFault.rows）
  stamp20.key.runId='T250_BAD_OBSERVATION_KEY';  ← 注入：篡改 stamp 的 runId
  bf20.applied(20, {stamp})                     ← INJECTED_READSET 行在此记录
  probe.stampEndpointObservation(...)            ← :4148 实际调用
    └─ P011NaturalObservation.ets :744-746：runId 不匹配 → throw ENDPOINT_COMMIT_KEY_MISMATCH
  finally: bf20.end('P011NaturalObservationProbe…','EXCEPTION:…') ← ORIGINAL_CHECK_RETURN 行在此记录
WaterSandField.ets :4193  probe.finishSubstep(after, budgets, audit)   ← 异常已抛出，永不执行
  └─ P011NaturalObservation.ets :1084  this.sink({...})              ← 唯一 observationRows 来源，永不执行
```

**三行语义澄清**（T319/卡面表述的修正）：
- BEFORE_INPUT / INJECTED_READSET / ORIGINAL_CHECK_RETURN 三行是 **TxBeforeFault.rows**（beforeEvidence.rows，注入边界的输入/注入/返回记录）——**T319 已确认这三行在档**（「beforeEvidence.hit=true 三档全部成立」+ rootRows 的 STEP3 EXCEPTION 行）；
- **缺失的是 observationRows**（natural observation probe 的子步记录，另一个数组）——T319 报告把「三原行未入归档」表述为 BEFORE_INPUT/INJECTED_READSET/ORIGINAL_CHECK_RETURN 缺失，**实际缺失的是 observationRows 数组为空**，三行 beforeEvidence 记录本身在档（T319 §二「rootRows SOURCE1→SOURCE2→STEP3 EXCEPTION」与「beforeEvidence.hit=true」均已核）。
- 卡面「BEFORE_INPUT/INJECTED_READSET/ORIGINAL_CHECK_RETURN 三行真实存在位置」的答案：**存在于 beforeEvidence.rows**（TxBeforeFault.ets :4/:5/:6 的 record 调用），**不在 observationRows**——T319 摘要把两者混淆。

## 2. 为什么 observationRows 为空是「按构造」

`P011NaturalObservationProbe.stampEndpointObservation`（:744-746）的 key 校验 throw 是**注入的预期故障**（cut20 的设计目标就是验证「观察提交键不匹配时的物理/回滚行为」）。throw 发生在 `finishSubstep`（:4193）之前 → probe 的 `this.sink(...)`（:1084）永不调用 → fixture 的 cb 永不执行 → observationRows 保持空数组。

**这是时序必然**：自然观察记录的是「子步成功提交后的完整状态」；注入的故障恰在提交前炸开。**不存在能同时保留「注入异常」与「该子步观察行」的序列化补丁**——除非伪造观察行（违反不虚补旧档红线）或在异常路径补记一条「失败子步观察」（新语义，见 §4 可选项）。

## 3. 缺证边界（保缺证，不虚补）

- **保留在档**（T319 已核，本卡维持）：beforeEvidence 三行（BEFORE_INPUT/INJECTED_READSET/ORIGINAL_CHECK_RETURN with EXCEPTION）、rootRows 全序（SOURCE1 19.2 → SOURCE2 6.4 → STEP3 EXCEPTION:ENDPOINT_COMMIT_KEY_MISMATCH）、snapshot 30 词 identical（物理回滚）+ totalUsed 0→5.0（观察/endpoint 提交保留）——**cut20 档的注入验证目标已完整达成**；
- **不可恢复项**：注入子步的 natural observation 行（本应含该子步的 settle/percolate/lift/预算/审计摘要）——**永久缺失**，因为该子步的观察输出在物理上从未产生；
- **checker 语义**（卡面「checker 语义」核）：T319 checker 对 cut20 的判定字段 `missing_rows` 是如实披露不是误判——但**建议字样修正**：从「BEFORE_INPUT/INJECTED_READSET/ORIGINAL_CHECK_RETURN not in archive」改为「observationRows empty (injected fault precedes finishSubstep sink）」——**报告层勘误，不改档**（沿 T322 勘误先例）；
- **不重跑 20 矩阵**（卡面红线）：现有档案充分；若未来需要「注入子步的观察行」这一新证据形态，是**新增注入档**（如 cut20b：BEFORE 边不改 stamp 而在 finishSubstep 后改），属 designer 侧新用例设计，本卡不预派。

## 4. 可选项（不推荐，仅登记）

若 manager 认为注入故障子步的观察行有独立价值：在 `finishSubstep` 的 catch/finally 路径补一条**故障观察行**（`{substepFailed:true, errorCode, partial state}`）——这是新语义（观察「失败」而非「成功提交」），需要：
1. designer 定义故障观察 schema；
2. developer 改 probe（新增 failSubstep 方法）；
3. 不回填旧档（新档新语义）。
**本卡不推荐**：cut20 的验证目标（注入→异常→回滚→前缀保留）已由 in-flight 证据闭合，故障观察行的边际价值低。

## 5. 路由

- **不派 developer**（T319 的「补序列化」建议撤销——缺失按构造、无序列化可补）；
- **tester 报告勘误**（T319 checker_summary 与报告 §二 cut20 的 missing_rows 表述，§3 修正字样）——下次 T319 链报告修订时带上，不单独开卡；
- **不阻 GPU**（卡面红线确认）；hit=true 不当全通过的边界维持（T319 §四原判）。

## 6. 变更记录

- 2026-10-30 v1：初版。observationRows 缺失归因为注入时序必然（stampEndpointObservation throw 在 finishSubstep/sink 之前）；T319「三行未入档」表述勘误（三行在 beforeEvidence.rows 在档，缺的是 observationRows）；撤销 developer 补序列化建议；故障观察行可选项登记不推荐；缺证边界保留。
