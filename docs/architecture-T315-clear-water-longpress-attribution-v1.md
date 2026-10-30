# T315 · 明水长按源量与同帧像素链首次归因 v1

2026-10-30，architect；不使用真机。依据 T310 报告、`test/reports/t310/{manifest.json,cw-critical-*-critical-0.json,cw-critical-*-frames.json,cold_end.txt}`、T309 签包生成源与 T305 工程合同。仅裁定隔离 `cwdiag=true` 的 C F64 参考线；不得冒充 GPU M1 实现或目标 8 体验达成。

## 源账与有效时间

critical 18/16384、`criticalDropped=0`、低优先 skipped3055 独立记账和段文件 SHA/END 均成立，故本 run 的关键事件窗口完整。run `cw-run-1793341689224-1-1` 的 DOWN 后只有两次 CLOCK_WINDOW/ENQUEUE/BATCH/TERMINAL：

| 序号 | 原 NS 前→后 | 有效秒 | requested/admitted | 源 commit / 全局 W 后 |
|---|---|---:|---:|---:|
| 1 | 2734444379322947→2734444390443514 | 0.011120567 | 0.011120567 | 1 / 0.011120567 |
| 2 | 2734444390443514→2734444412682284 | 0.022238770 | 0.022238770 | 2 / 0.033359337 |

两窗合计 **0.033359337 秒有效时钟**，与两笔水量逐位相符。T309 本签包的 `CLEAR_WATER_RUN_CONFIG_V1` 原配置 `rateWater=1`，`ClearWaterSourceController.sample()` 用 `rateWater × clock.seconds`；T310 报告称“8 units/s 下长按合并”与签包源/帧 token 配置不符，应由 tester 勘误。`ClearWaterFrameClock.tick()` 对大于 1/30s 的间隔返回 `GAP`，`SourceController.tick()` 随即 `stop('GAP')`，清 pointer 并 reset clock；critical 第 17 行在第二笔终态后正是 `STOP GAP`、同 gesture/input generation，没有第三笔 ENQUEUE 或 UP。由此可确定本轮**源在第二笔后因时钟 GAP 提前停止**，所谓 30s 墙钟长按不能换算为 30s 有效注水，也不能称长按请求被正常合并为两笔。GAP 的外因（页面/设备调度、输入事件、长 callback）没有原始跨线程时间线，不作唯一归因；pointer 之后即使仍物理按住也不会自动恢复，是当前源码行为。

这不是水量准入失败：两笔均从 PENDING 到 ACCEPTED，admitted=requested，源 commit 1→2，W 0→0.033359337；也没有容量裁剪证据。工程合同须由 developer 修订：在 C 参考线把有效长按定义为连续 pointer 身份、时钟域与窗口、GAP/STOP/UP 的显式终态，提供固定且有界的 GAP 前后 rawNS/delta、pointer 状态、sample/commit 计数和用户可见停源反馈；若产品要求长墙钟按住且恢复后继续供水，先把重新 DOWN/重新授权或恢复手势的策略交 designer 定义，不能悄悄补跑丢失的 dt、跨 GAP 续旧 generation 或放宽 1/30s 时钟上限。tester 使用现有原档先修报告分母；是否需要设备最小补证，仅针对 STOP 前后触摸/调度与显示状态，不无限重采相同 ROI。

## 同 run 帧与像素能证到哪

frame900–902 的封存帧在第二笔终态之后，原 `P011ClearWaterEngine` 规定 header[5]=**physicsCommit**、header[6]=**sourceCommit**；三帧均为 586/2，后者等于第二笔 terminal sourceCommit2。`frameData`、ledger/profile 的 W 为 0.033359337；三帧 requested/rendered/LastGood token 各帧相等，`currentFrameSucceeded=1`、槽 verify=1。native 三层九个样本同 token、`rawGL=0`、`nativeWater=ledgerWater=0.033359337`。ROI `(1172,0,4×4)` 对应本 run 实量水列127；层4 有 `22252bff` 与背景 `12161fff` 的非零字节差，层2/3 保持背景。故“源两笔终态→后继提交2/物理586→同帧新 shader 水面 FBO 原像素”窄链成立，body 可见和实际呈现 swap 仍没有独立完整证据。原 native 样本的 `sourceCommit=586, physicsCommit=2` 字段顺序与 header 合同相反，不能拿这两个**标签**直接做因果等式；需 tester 对照原 token/header 和 native writer 接线勘误，保留原值不改档。

当前 W 只相当于约 33ms、rateWater1 的有效注水，不能据此裁定 30s 连续注水时 body 仍薄、也不能先调 sigma/tint/shader。后续 tester 只在真实有效长按/明确 stop 原因、后继 sourceCommit/physicsCommit、同 token slot/native ROI 与 swap 状态齐全时更新端到端结论；effector 再评观感，设计定标归 designer。全部前背景仅新 XComponent+shader；失败 FAULT 或完整旧 token 显式 STALE，禁止 Canvas 上屏/回退。GPU M1 落水/侧扩散等效果仍沿 GPU 驻留优先，不以 C F64 参考像素作 M1 PASS。
