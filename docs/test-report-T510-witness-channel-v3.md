# T510 v3 · SourceClaimWitness 通道真机验证（T579b-witness 包，T575 消费）

日期：2026-11-18，tester，真机（011124264R000337）。
被测：T579b-witness-signed.hap（t575-witness-tree，T575 SourceClaimWitnessV1）。
证据：`test/reports/t491/t510c/`。

## Witness 出档通道真机验证 PASS（T575 消费验证）

M0 页（--ps grsm0）：**TextInput 填外部签包 SHA256**（T579b-witness hapSha=
3095547c…，从 delivery.json wave2 读取）→ 导出原生时间线 → **COMPLETE**：

- `gpu-timeline-libgrsm0.so-54179-2-1794970875575-manifest.json`（6820B）：
  state=COMPLETE / reason=STABLE_PREFIX_FULL_TRACE / rows=173（serial 1..173 连续，
  missing=0 duplicates=0）/ pages=44 / files 2 part 各带 sha256
- part0/part1 ndjson：173 行真实 timeline 行（stage 索引按 T324_STAGE42_V1 映射：
  DispatchEnter/Exit×59、WaitEnter/Wake×7、InitBegin/Invalidate/Program*/Resource*/…）
- RootSubmit×1 在档；PACKAGE_SHA_REQUIRED 门（首次无 SHA → INCOMPLETE）按合同拒绝
  并在填入外部签包 SHA 后放行——**外部签包清单校验链真机生效**

## T510 核心域（Down→Offer→G1 Claim 同 seq 链）现场

- M0 Spawn sand 域：statusAfter.lastOfferCode=**NONE**、napiStage=GET_CB_INFO、
  typedType=-1（typed 输入未到达）——M0 Launch 域无触摸 Offer 行（符合域划分）
- M1/M2 体验页真实触摸链：M1 页 STALE（CLOCK_GAP，run 0:1）下触摸不产生 root；
  真实 Down→Offer 域需在 **M2 体验页**（T452_EXPERIMENT_V3）进行——该页触摸+
  witness ledger（executionDiagnosticsV1 sourceClaimWitness 字段）为下一轮
  attempt 的取证路径（T575 witness ledger 已接线，T579b-witness 包就绪）
- 与 T510 v2 原件链：既有 Down→首 NATIVE_OFFER 18.634ms 证据保持有效；本轮新增
  witness 通道验证+M0 域对照（lastOfferCode 域可读性确认）

## 判定

- T575 witness 通道：**真机消费验证 PASS**（SHA 门+COMPLETE 出档+行完整性）
- T510 精确补证域：witness 通道就绪；同 seq Down→Offer→Claim 链取证转 M2 体验页
  （下一 attempt：M2 触摸→导出启动诊断→sourceClaimWitness 字段解析）
- 保留：T510 v2 的 18.634ms/5×ENQUEUED_NOT_ACK 已证域
