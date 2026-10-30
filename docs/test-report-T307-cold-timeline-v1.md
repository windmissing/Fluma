# T307 GPU 冷时间线定点真机续证（真机，结束待验收）

2026-10-30，tester 真机（VYG-AL00 62T0225B10010020）。包：T306-signed.hap，SHA256 `56a3edef2f355e6bd52128c121cff6b790ca89fb6cf2811bcf0bdd930dbdbf0b`（实测一致；显式输入通道=页面 TextInput，非运行时自验）。

## 一、冷导出链路真机验证

- **M0**（pid 37404）：首次导出无 SHA → `INCOMPLETE Error: PACKAGE_SHA_REQUIRED`（fail-closed 正确）；输入外部签包 SHA 后导出 **COMPLETE / STABLE_PREFIX_FULL_TRACE**：14 页 × 4 行 = **56 行**，serial 1→56 连续、missing=0、duplicates=0、part0.ndjson 18010B SHA 独立核对一致。
- **M1**（pid 40537）：诊断区展开导出，第一次 `INCOMPLETE Error: GREW_DURING_EXPORT`（worker 仍在增长——如实 INCOMPLETE 不冒 COMPLETE）；静置后重导 **COMPLETE**：16 页 = **63 行**，SHA 独立核对一致。
- MANIFEST 字段核：stageRevision=T295_STAGE23_V1（24 项字典）、packageSha256 显式外部输入、pid=真实 process.pid、identity.module/route 对应实际导入；`OPERATOR_SIGNED_ARTIFACT_MANIFEST_EXTERNAL_VERIFY_REQUIRED` 固定标记在案。

## 二、首帧分账（原 ns，TID 分列，不钳值）

### M0（worker TID 394084247696 / UI 线程 TID 391046875480）

| 阶段 | 原 ns | 分账 |
|---|---|---|
| InitBegin(epoch2,flags=3) | 2733015714484078 | 首次 init（无 window） |
| ReadyRejected(epoch2,flags=3) | 2733056439569078 | 正确拒绝（缺 window/allowed） |
| Invalidate ×3（UI 线程） | — | 跨 TID 分组可见 |
| Ready(epoch4,flags=7) | 2733085221216266 | **READY 提交**（active+window+allowed） |
| FirstTick(delta=1) | 2733085221218766 | ready→first **2.5µs** |
| AnchorSet | 2733085221222828 | first→anchor **4.1µs** |
| RootSubmit | 2733085425242203 | anchor→submit **204.0ms**（首 root GPU 提交） |
| PauseBegin/End(delta=**195907500**) | 2733085433339703 | submit→pause 8.1ms 后 CLOCK_GAP，**delta=195.9ms 原值** |

### M1（同结构）

Ready(epoch4,flags=7) → FirstTick **47.5µs** → AnchorSet **4.5µs** → RootSubmit **87.9ms** → PauseEnd(delta=**78688906**)。两次独立冷启 CLOCK_GAP 原 ns 不同（195.9ms / 78.7ms）= 真实挂起恢复 wall 间隔，非钳值、非补跑。

### 等待/工作分账

M0/M1 全部 WaitEnter→Wake 配对时长 **~8.08–8.13ms**（Fixed120Clock 8ms tick 正常等待）；无异常长 wait、无调度异常（pause 后不再重复取证，保首次 PauseBegin/End）。

## 三、结论

- **READY→CLOCK_GAP 极速循环的真因时序首次可见**：Ready→RootSubmit 后 8ms 内即 Pause（delta 78–196ms 原值）——首 root GPU 提交期间（87–204ms）wall 被挂起/切换，恢复后 delta 即超阈值。与 T283/T296/T301 的外部观察一致，现分账到 stage 级。
- 冷导出工具链真机 PASS（COMPLETE/INCOMPLETE 语义正确、SHA/serial/单调全过）。
- source terminal→quantum→lease→同 token 像素 NOT_RUN（CLOCK_GAP 后需新 run，未推进；accepted=0 不冒 PASS）。

证据：`test/reports/t307/`（M0/M1 manifest ×3 + part0 ×3 + 布局快照 + cold 日志）。
