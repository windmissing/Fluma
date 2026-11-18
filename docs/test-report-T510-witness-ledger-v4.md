# T510 v4 · M2 体验页 witness ledger 首证（T579b-witness 包）

日期：2026-11-18，tester，真机（011124264R000337）。
被测：T579b-witness-signed.hap。
证据：`test/reports/t491/t510d/`。

## 决定性：sourceClaimWitness 字段真机首证

操作链：冷启 → M2 体验页 → 新空场（建 run）→ 水滑动触摸 ×4 → TextInput 填签包 SHA
→ **导出稳定快照** → `gpu-snapshot-64805-1794972133427-execution.json`（558KB）：

- **`sourceClaimWitness` 字段在档**（T575 接线真机验证 PASS）：
  revision=SourceClaimWitnessV1 / witnessBytes=176 / designatedEnqueueSeq=0 /
  dropped=0 / rows=[]（空账本——本轮触摸未产生 SOURCE typed enqueue）
- timeline 快照（libgrsm1）：4096 行 / 1024 页 / missing=0 dup=0 / **NATIVE_DROPPED
  INCOMPLETE**（缓冲满 4096 上限——host 侧如实截断，无丢行断言伪造）；其中
  RootSubmit×6、acceptedCount=**1187**、Submit/Flush/QuotaBalance 域全在档
- inputStages（T537_INPUT_STAGE_V1）：3 行——serial1/3=**NATIVE_ACTION_API_REACHED**
  （code9）、serial2=**CONTROLLER_ACTION_REPLY "ACTION_RECORDED"**（code6，
  event=NEW_RUN_RETURN）；counts[5]=1/counts[8]=2

## 现场判读

- 触摸滑动（uitest swipe）走 UI 手势路径，**未产生 SOURCE typed 输入**（typedType=-1、
  准入 Q 0 一致）——真实水滴请求需要页面级手势绑定到 typed 端点（GpuSandWaterExperience
  的触摸→SOURCE 输入映射），uitest swipe 可能不触发该绑定（T546 §5 同域）
- witness ledger 通道（捕获 ENQUEUED_NOT_ACK 入队+FrontLocked claim）已接线且结构
  正确——一旦产生真实 SOURCE enqueue，designatedEnqueueSeq/rows 将填充
- 空场 run 的 launch/accepted 域（1187 accepted）与 SOURCE 域分离清晰

## 判定

- **T575 SourceClaimWitnessV1 真机接线验证 PASS**（字段/结构/通道三层）
- **T492/T495 timeline 深度链真机 PASS**（4096 行深度+INCOMPLETE 诚实截断）
- T510 同 seq Down→Offer→Claim 链：**通道全通、待真实 typed SOURCE 事件**
  （需 UI 手势→typed 端点映射验证——归 T572 adapter 域或 M2 页手势绑定核查）
- 对 T553 §2 的推进：witness 窄接口已交已验，"缺同 seq原件"的设施域正式关闭
