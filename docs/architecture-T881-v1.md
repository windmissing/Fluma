# T881 · PC D1 四臂退役与时域等价首次归因

2026-11-23 · architect · P4 · Windows PC 原件复核与工程合同

**四臂都取得 WSI 退役，只有 REPLAY_OFF 的业务 drain 为 true。四臂的 SOURCE 7 虽然都接受，真实相邻拍间隔已产生四种 Q20 接受量。** `DRAIN` 和 `CLOCK_EQUIVALENCE_UNPROVEN` 均有原始依据。优先修宿主 QUANTUM 隔离退役与收据、分离 D1 的固定对照输入和实际时钟证据，再交独立 PC 四臂复采。T869 的忙点、热页和同根归档继续按已证范围入账。 [T496 独立复核](../test/reports/t496/pc-offline-join.md) · [四臂复算](architecture-T881-evidence-v1.json)

## 四臂实证

使用 `c512bbca` 主批的 `off2/on2/replay-off2/replay-on2.jsonl`，T860 交付 PE 的实物 SHA 匹配，宿主代码钉 `9e10003d`。四臂每臂 52 个唯一有序 action，17–769 ms 为声明相对时间，另有每次实际 QPC 换算时间。每臂 4 条 OFFER JSON 行的 `actionId` 整数后多一个引号，原字与修补行号单存。`source 7` 由实际 pulse 8 投递，以 pulse 6 为间隔起点。`source 29` 由 pulse 30 投递，与 pulse 28 的实际差只有 15.008–16.062 ms，四臂均 `Expired`、root=0。此终态出自 owner 的 `takeFresh`，该版本具体子条件仍需要记录实际取件年龄或 start tick，现有结果不支持将它归到材料接受或尾拒。 [T860 交接](../test/reports/t860/handoff.md) · [T875 原日志](../test/reports/t875/report.md)

| 主臂 | SOURCE 7 的实际 Δ ns | SOURCE 7 接受 Q20 | WSI 退役码 | 业务 drain / inFlight | 最后拒绝 QUANTUM 的 fence 码 |
| --- | ---: | ---: | ---: | --- | --- |
| OFF | 14,571,100 | 122,231 | 0 | false / 0 | 0 |
| ON | 16,076,100 | 134,856 | 0 | false / 0 | 0 |
| REPLAY_OFF | 16,079,800 | 134,887 | 0 | true / 0 | 无该失败 |
| REPLAY_ON | 16,031,500 | 134,481 | 0 | false / 1 | 2 |

### 生产、脚本与证据分流

- **生产侧退役缺陷。** T860 的 QUANTUM fence/permit 首拒调用 `root.Stop()`，将已提交 owner 置 tombstone/`Quarantined`。WSI 退出后只释放 publication、撤时钟并取快照，缺该根的 owner 退役。OFF/ON 末次 fence 已返回 0 且 no offer/lease/permit/inFlight，源码和最后拒绝闭合到被隔离 owner 尚未退役。REPLAY_ON 最后样本的 fence 为 `VK_TIMEOUT=2`，`inFlight=1`，缺随后同 fence 最终信号或设备失效证明。原件无法判定该根后来何时 signal。`frame.retire==0` 证明对应 WSI 边沿，`m1D1Drain.retireVkResult` 在原码只抄它。返回前强制 `clock.Revoke` 也会清 `pending/activeRemaining`，因此还需撤销账而非只检查清零。 [T860 QUANTUM 等待](../entry/src/main/cpp/gpu_resident/g13vulkan_boot/m1_quantum.inc) · [Owner Poll/Stop](../entry/src/main/cpp/gpu_resident/m1/source_owner.cpp)
- **生产侧序列化缺陷。** `Window::DrainSourceTrace` 在整数 `actionId` 后输出了字符串结束引号。以原 JSONL 修正版/兼容版标记区分。T496 的固定位置内存修复可复核历史端到端 join，不能充作新版格式验收。T872 基线也带同一缺陷，T880 的 5 轮复算碰到它。 [原日志解析](../test/t496/check_pc_d1.py#L33) · [Win32 入口](../pc/vulkan/main.cpp)
- **脚本时域不等与证据缺口。** `Window::RunD1` 用 `sleep_until` 发实际 QPC pulse，再用真实 `ns-previousNs` 制造 SOURCE 工件。四个原 Completion 精确等于 `floor(8 × Δns × 2^20 / 10^9)`，就地解释 12,656 Q20 的最大差，写入量差已发生在热页或银行对照前。仅靠相同的动作表、READY 后重置时间原点或 `DELIVERED` 不能证明实际拍被 owner 接受。末 action 立即发 `WM_CLOSE` 混合正常停止与 window 撤权，且 ON 首批有 `CLOCK_GAP/PERMIT_REVOKED`，REPLAY_ON 主批 `NOT_BUSY/NO_PAGE`。不能据此估计捕获的因果影响。T496 适配器人为设 `clockEquivalent=false`，是给缺证的门标志，独立的实际间隔和原 Q20差已证明等输入不成立。 [T865 实际门](../test/t865/check.py#L63) · [独立连接](../test/t496/check_pc_d1.py#L159)
- **保留已过判据。** T875 的 27 份 REPLAY `M1PROOT1` 与各自进程和根原字对齐。T869 runA 的首根 `M1PCOLD1` 两页 94,336 B，runB 的后继忙点 `M1PROOT1` 62,792 B，runC 的 256 原字热页和 nonce 均通过相应检查。主 ON 的热页也有同根原件。这些原件的证明范围为各自忙点、快照和归档。D1 业务 drain 和跨臂时域/原字等价继续待证。 [T869 原报告](../test/reports/t869/report.md) · [T496 机器收据](../test/reports/t496/pc-offline-join.json)

## Developer 修订与 Tester 最小复采

1. **先安全退役与原始收据。** 复用 T872 已交付的有序脉冲、实际双时标和拒绝根退役路径，接入 D1 尾段。动作表结束发一次 `END_INTAKE`，封住本次 action 和新 reservation，确认最后接收序号。owner 在实测有界窗口内清理已提交工作、结束/撤销每个 offer 和 capture request，按截止账销账剩余待发配额。接受根沿原 Completion/发布合同，已 tombstone 的根先等该实际 fence 后以 `Poll(Signaled,nullptr…)` 退役，禁止迟到结果成为 accepted。真实 `DEVICE_LOST` 走状态丢失，fence 仍未就绪留隔离与 `DRAIN_TIMEOUT`，均没有 PASS。保旧资源/lease 到退役完成，分记 owner fence、WSI retire、writer/reader/pin/credit 手续的实证。所有 `offers/inFlight/lease/pending/active/pin/writer` 都闭合、owner Idle、最终 trace loss=0 后，输出唯一完整 drain，随后关闭窗口。动作执行失败同样取完整退役，但失败动作保原码。普通窗口交互和业务期限沿原路径，33,333,333 ns、250 ms、120 Hz、守恒、shader 渲染和 GPU 驻留保原合同，禁止 Canvas 回退。
2. **同一输入合同下核真实时域。** 冻结同一新版 PE、三 SPIR-V、配置、初态、窗口和 52 个 action，所有 arms 以真实 READY 作一次相对锚定。记录每个 action 声明 ns/QPC、producer callback 与 native receipt QPC、owner 消费和接受/撤销、相邻真实 Δ、SOURCE 量、grant/carry/overload/accepted/revoked 与每根按序对应的 pulse ID。先复采 OFF×2，且做一次 `END_INTAKE` 时 fence 已 signal、一次仍 pending 的离线对照。用严格解析检查 OFFER 输入唯一终态，独立 bridge 只将真实 owner 接受映射为 `PULSE ACCEPTED`，并确认窗口关闭前 drain。两次 OFF 若真实输入已不同，按首对失配保原字，四臂留 `CLOCK_EQUIVALENCE_UNPROVEN`。如需消除四次 `sleep_until` 的 SOURCE 抖动，推荐 D1 测试专用共用 producer 流，预先固定同一份真实测得的相邻拍间隔和 SOURCE 参数，用同一锚协议在各 arm 实时送入，另外记录每个实际送达、owner 领取、真实 watchdog/fence 与时域偏差。只有实测可以复算等价时才登记同真实时域。固定输入诊断组可单独重放相同逻辑轨迹、帮定位原字差异，实时时钟和 freshness 始终用真实 QPC，原始实测门的缺项继续留缺。保 T865 的作用域，SOURCE 29 再过期要留实际年龄/许可/截止阶段。 [T865 D1/D2 交接](../test/reports/t865/handoff.md)
3. **四臂原字。** 实时合同与 drain 均闭合后，由 PC 取证角色依次采 OFF、ON、REPLAY_OFF、REPLAY_ON。记录 ON 的真实 Busy 命中及 1,024 B 页，OFF 两臂保空操作且无热路附加 readback。`NOT_BUSY/NO_PAGE` 继续作为合法终态，额外按同输入取得相应 Busy 命中轮再验证捕获效应。保存每臂实际 producer128、SOURCE command32、Completion256、顺序和区间，REPLAY 两臂保同根实际 bank/event copy fence、invalidate、backing 和字节预算。T865 只归一已核真实身份字段，其余字、Q20、前缀和偏序按原值比较。先保首个 mismatch、原行及 SHA，再分流；通过业务退役或单次 archive 不单独宣告四臂 PASS。离线已复跑 T496 检查和 T865 12 项负控，原件的严格门按表继续记缺。 [T496 可执行检查](../test/t496/check_pc_d1.py) · [T865 负控](../test/t865/selftest.py)
