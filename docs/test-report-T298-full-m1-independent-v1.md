# T298 GPU M1 整步接受独立验证（离线，结束待验收）

2026-10-30，tester 离线（PC ANGLE/SwiftShader，GL 身份如实标注，device=NOT_RUN）。被测：T280 FullM1 commit 交付（组合 SHA `6c476dcf…`）。

## 一、独立复跑（直接执行交付二进制，非复用报告 json）

| 项 | 分母 | 结果 |
|---|---|---|
| full.exe K192/W64 | 465 | **465/0** |
| full.exe K192/W128 | 465 | **465/0** |
| full.exe K512/W64 | 465 | **465/0** |
| full.exe K512/W128 | 465 | **465/0** |
| 合计 | 1860 | **1860/0** |
| replay（完整 event 重放+整数 oracle）四档 | 38×4 | **152/0 PASS** |
| SOURCE 兼容回归（t240 transport.exe 重跑） | 562 | **562/0** |

覆盖（依 full trace 独立可见）：SOURCE→QUANTUM→后续 SOURCE、J→W、water MAX/ID_EXHAUSTED/DIRECT_W（source-only 0 哨兵兼容与 FullM1 MAX 策略边界在交付文档与 full 断言内分层明确）、池/容量拒绝真实分母、同 frame 多 quantum、lease 背压/旧 generation 拒绝、正常尾包逐字段篡改/长度/GL 拒绝、Stop 迟到保前缀。

## 二、污染反例独立抽核

full trace 中 **14 个 FAULT 行**（14 类 GPU 事件/候选/ledger/pub/tail 污染反例）全部 `status=9`（拒接受）且 `audit<1023`（未通过全相审核）——**无一例假接受**；成功前缀在反例后保留（前缀 root 的 `POLL… audit=1023` 行继续出现）。completion256 尾封三 SHA/nonce/代/长度/预算校验在 full.exe 内部断言覆盖，replay 152/0 独立复核重放语义。

## 三、边界（如实）

-tester 复核主要采用**独立执行 + trace 行独立判读**；completion256 字段级/双相总量等语义期望未逐项另建第二实现（开发 1860/0 复跑不冒独立——本报告以"独立复跑一致 + 反例独立判读"为准）。
- 稳态 buffer payload（23.7/25.8/55.6/57.8MB）与 dispatch 数（113/127/122/136）与交付一致（trace CREATE 行核对）。
- T281 scene/设备收益另卡，不作前置、不阻 T281。

## 四、结论

T298 **结束待验收**：1860/0 + 152/0 + 562/0 独立复跑全过，14 反例拒接受语义独立可见，无 FAIL、无需回 architect。

证据：`test/reports/t298/`（full_recheck ×4、source_recheck、recheck_summary.json）。
