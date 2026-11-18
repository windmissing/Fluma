# T593 v1 · 真实 typed SOURCE 与同序 Claim 续证（T597-fixed 包）

日期：2026-11-18，tester，真机（011124264R000337）。
被测：T597-fixed-signed.hap（T597 a7ba790 修复：witness 绑定真实 root，SHA
20bad4993786c3edf5e6d478767b0f28d49f54084b6c892ab290f788dcf87550），bm clean 后冷启。
证据：`test/reports/t491/t593a/`（execution.json 309KB sha 0e163a0a…+manifest）。

## 操作链（T585 口径）

冷启 → M2 体验页 → 新空场（建 CURRENT）→ 切水 → **按住 150–250ms×3 跨 pulse 序列**
（uitest swipe 200/250ms 长按）→ 填 T597-fixed 签包 SHA → 导出稳定快照。

## 结果

| 域 | 实测 |
|---|---|
| witness ledger | SourceClaimWitnessV1 / 184B / designatedEnqueueSeq=0 / dropped=0 / **rows=[]** |
| inputStages | 2 行：serial1=**NATIVE_ACTION_API_REACHED**（code9）、serial2=**CONTROLLER_ACTION_REPLY event=SOURCE_SELECT**（code6）——UI 水选择已接线、typed 层未触发 |
| typed SOURCE | typedType/typedLength/sourceCommit 字段**不在档**（本轮触摸未产生 typed enqueue） |
| Claim/ROOT_STARTED/TERMINAL | **无**（空 Q accepted=0，不冒非空 SOURCE） |
| timeline | INCOMPLETE（诚实截断） |

## 判定

- **T597 修复包下 witness ledger 通道健康**（witnessBytes 184>176 旧版，绑定真实
  root 的结构在档）； SOURCE_SELECT 控制器动作到 native API 均正常
- **真实 typed SOURCE 事件仍未产生**：uitest 长按/滑动手势不进入 M2 页 typed
  端点（与 T510 v4、T546 §5 同域一致）——**同 seq enqueue→FIFO Claim→SOURCE
  completion 采证 NOT_MEASURED**，空账如实保持（rows=[] 不当链 PASS）
- T510 已证 Down→Offer（18.634ms/5×ENQUEUED_NOT_ACK）不重复
- **接口缺口回 architect**：真实手指按住跨 pulse 的 typed SOURCE 事件只能由真人
  操作产生（uitest 不行）——需要 effector 域真人按住取证，或 developer 提供
  typed 事件注入测试端点（合成 SOURCE 被卡面禁止，不采用）
