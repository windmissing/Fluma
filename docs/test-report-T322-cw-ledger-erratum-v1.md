# T322 明水原账与 commit 标签勘误（离线，结束待验收）

2026-10-30，tester 离线。按 architecture-T315 用 T310 原档（manifest/critical/frames，不改原件、不重采 ROI）作源账复核与标签勘误。

## 一、两窗有效时间逐位核验（T315 表全部成立）

| 窗 | rawNS a→b | 有效秒 |
|---|---|---:|
| 1 | 2734444379322947 → 2734444390443514 | **0.011120567** |
| 2 | 2734444390443514 → 2734444412682284 | **0.022238770** |

- `w1.end == w2.start`（两窗连续无隙）；合计 **0.033359337 s 有效时钟**，与两笔 admitted（0.011120567+0.022238770）及全局 W 终值逐位相符
- `rateWater=1`（签包 CLEAR_WATER_RUN_CONFIG_V1）——**T310 报告"8units/s"说法勘误**（与签包源/帧 token 配置不符）
- STOP row17 `reason=GAP` 紧随第二笔 TERMINAL（row16）、同 gesture/input generation、无第三笔 ENQUEUE/UP——**"30s 长按正常合并"勘误**：源在第二笔后因时钟 GAP 提前停止，30s 墙钟 ≠ 30s 有效注水（pointer 之后物理按住不自动恢复是当前源码行为；GAP 外因无跨线程原始时间线，不作唯一归因）

## 二、header/native commit 标签反序勘误（writer 键名错，值正确）

- **header 语义正确**（按原 Engine 合同）：`header[5]=586`＝frame900 时点 physicsCommit、`header[6]=2`＝第二笔终态 sourceCommit；token 字段 [4..11]=586,2,900,584… 与 header 同序；critical TERMINAL 行 `sourceCommit=2` 一致
- **native 样本键名互换实锤**：`sourceCommit=586`（实为 physics 值）、`physicsCommit=2`（实为 source 值）——**native writer 用错键名**，数值本身与 header 语义一致。按卡不判定为"因果等式"证据；接线修正须先回 architect 修文档后派 developer（tester 不改原档）
- T310 报告正文未引用 native 反序键名（只引 nativeWater/ledgerWater），该点无需报告勘误，但 writer 键名缺陷如实登记

## 三、能证边界（不升 PASS）

- **保留**：两笔终态→sourceCommit2/physicsCommit586→同 token FBO 水面窄证（层4 `22252bff` 非零、层2/3 背景）成立
- **未证**：body 实际可见、实际 swap 呈现、有效长按观感——按卡不升 PASS；当前 W≈33ms 有效注水不能裁定 30s 连续注水时 body 仍薄；sigma/tint/shader 不先调（designer 定标）
- 无相同 ROI 重采

## 四、结论

T322 **结束待验收**：两窗原 NS/有效秒/STOP GAP 逐位复核 + 两处报告说法勘误（8units/s、30s 合并）+ native writer 键名互换实锤（值正确、标签错）。全部原档未改。

证据：`test/reports/t322/erratum_summary.json`。
