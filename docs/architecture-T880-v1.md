# T880 · 目标 7 第二源在帧边界过期

2026-11-23 · architect · P4 · Windows PC 原件复核与工程合同

**T872 仍持续打开帧边界。run5 的第二次水按住期间有 196 次有效 QUANTUM，跨 50 个 frameSerial。200 个已入队的水 offer 中，196 个在 owner 取件时过期，4 个在撤权时结束。另有 343 次提交因队满返回。** run3 的第二 hold 重现 FIFO 过期，随后才失焦与撤权。run2、run4 则是整根 deadline 先退出，第二次 Down 没进入 offer 队列。首归因为宿主 SOURCE 调度，沙水算法本身尚无同场景输出可判。 [T876 原报告](../test/reports/t876/report.md) · [五轮复算](architecture-T880-evidence-v1.json)

## 原件与可达条件

按 `f70ed1c9` 比对五份原件，SHA 按工作区物理字节登记，Git 检出换行差异在比较时统一。实物 PE 校验匹配 T872 交接，宿主源码按 `9101958c`。原 OFFER 行在 `actionId` 后多一个引号，复算仅在内存应用 T496 的定点解析修复，逐轮校验 `run/geometry/offer` 的入队和唯一 terminal。计数包括 `M1_SOURCE_WSI` 中首个确实 queued 的 QUANTUM，不重复计入退出收据。 [T872 工件](../test/reports/t872/handoff.md) · [T496 解析](../test/t496/check_pc_d1.py)

| T876 原件 | 全程有效 QUANTUM | 第二 Down 期间 QUANTUM / frameSerial 数 | 第二材料入队 / 队满 → terminal | 最大 producer 拍间隔 |
| --- | ---: | ---: | --- | ---: |
| run1 沙→水 | 213 | 63 / 17 | 66 / 88 → 61 过期、1 `Rejected`、4 撤权 | 32.073 ms |
| run2 水→沙 | 5 | 0 / 0 | 0 / 0，第二 Down 晚于 `ROOT_DEADLINE` | 无第二 hold 回执 |
| run3 沙→水 | 199 | 152 / 39 | 154 / 234 → 152 过期、2 撤权，末尾 Cancel、Focus Lost 与 `CLOCK_GAP` | 32.196 ms |
| run4 沙→水 | 5 | 0 / 0 | 0 / 0，第二 Down 晚于 `ROOT_DEADLINE` | 无第二 hold 回执 |
| run5 沙→水 | 280 | 196 / 50 | 200 / 343 → 196 过期、4 撤权 | 31.489 ms |

1. `Pulse` 只往 `pending` 加配额，最多 4。`Claim` 只在 `activeRemaining==0` 进入，将当前 `pending` 冻结给新 grant。接受每个 QUANTUM 时 `AcceptQuantum` 减 1，续帧递增的 Completion frameSerial 和 ordinal 证实边界存在。连续成功时，每发一份 grant 最多 4 个接受 QUANTUM 就回到边界。这个上界按已接受的 QUANTUM 算，实际等待仍受 fence、WSI 和许可约束。 [时钟配额实现](../entry/src/main/cpp/gpu_resident/m1/progress_clock_v2.h) · [QUANTUM 接受](../entry/src/main/cpp/gpu_resident/g13vulkan_boot/m1_quantum.inc)
2. `M1Ingress::Take` 按最早 seq 取待处理项，`takeFresh` 每边界最多清 4 槽，并要求 `pulseTicks>pulseStartTicks` 和 `0≤now−pulseNs≤33,333,333 ns`。再次取得 fresh 并保持授权后，交互分支还需要同 hold 的两次被选拍，其中首次只建锚。两条被选拍相隔过大则设置 `expiredHold`，该 hold 此后不再进入 SOURCE。两次被选拍间距跟随帧边界，可能远大于实际连续 producer 拍间距。run1 的一个 `Rejected` 只证明有项通过 `takeFresh`，原档缺少该项后续条件的明细。run3、run5 的第二 hold 没有 `Rejected` 或 `Accepted`，应先修边界取得有效输入，再消除错误的边界抽样锚定。 [四槽入口](../pc/vulkan/m1_ingress.h) · [SOURCE 继续发布](../entry/src/main/cpp/gpu_resident/g13vulkan_boot/m1_source.inc#L629)
3. run5 的第 221、228 行见 frame 15/ordinal 4 → 16/1。按前一帧真实 Present 退出这个年龄下界，四槽 96–99 已分别老化 85.362、68.886、53.941、38.116 ms。最新两拍 100、101 已被队满拒绝。第 239、247 行的下一次边界同样清掉 102–105，最年轻也有 105.685 ms。这个复算独立证明旧项占满有效边界。run3 末端原时钟明确报 `CLOCK_GAP`，第二 hold 的 producer 记录在 Cancel 前持续，撤权与该缺拍均不足以解释之前 152 个已过期项。run2、run4 各仅一处真实根期限拒绝，日志各写两条失败/退出边沿，fence 原码为 2。 [边界、终态行号与 QPC](architecture-T880-evidence-v1.json)

## Developer 修订合同

1. **按真实 producer 相邻拍形成单项预约。** 在 Win32 同序的真实 callback 和授权采样点，以当前 run/owner/context/surface/geometry/session、hold 和材料建立相邻拍游标。新 Down 后以第一条真实合格拍为 baseline，从后续严格递增且间隔不超 33.333 ms 的实际相邻拍形成 `previous/current/actualDeltaNs`。UI/native 两条时钟的真实间隙、序号、反序及输出缺项沿 T872 判。长真实间隙按原许可失效且重新建立会话与新 Down，禁止用跨越间隙的累计量。`M1_D1` 显式 action offer 沿其具名 action lane，保留原四槽和终态。
2. **让最新有效预约在下一次合法边界优先。** 每个窗口当前授权 hold 预留一个定容 waiter。owner 尚未认领时，后续同 hold 的已验证相邻区间可替换它，被替换/超龄项分别给明确终态。已认领项不可改，最多保一个正在消费和一个待取的预约。禁止四个旧项挡住刚收到的一拍。`activeRemaining==0` 时，在新纯 QUANTUM grant 前按原 owner、clock、root、hold、材料、单调 watermark、fresh 和空事件范围检查，取一次合格区间再走原 SOURCE。取件与空槽裁决需短锁原子，可与脉冲/撤权并发。若该边界无合格预约或撤权，原 QUANTUM 可继续。失败前已授予 QUANTUM 不截断、不补领、不重复接受。条件持续成立时，认领有界为最多 4 个已接受 QUANTUM 后的下一合法边界。跨线程由 owner 消费时重新核终点 freshness，等待期间不得回填旧拍。预约间未消耗的区间不得并成大笔注入，区间消费一次且不重叠入账，记录明确的跳过量和终态。
3. **保持接受、时间与物理合同。** SOURCE 仍是零物理时长，量按实际合格相邻拍，保原 command32、offer 生产者 SHA、Completion256、root、事件精确前缀、库存与拒绝无副作用。用原提交前/尾接受前授权检查；Up、换材料、失焦、换代均撤预约，不接受旧件，已提交根按 T872 tombstone 与实际退役。原 120 Hz、四份配额、33,333,333 ns 与 250 ms 保留。场景根、库存在旧根已完整接受时仍同场景接续。所有传输、计算与 shader 出图沿 GPU 驻留路径，窗口故障按既有明确 FAULT 或同代 STALE，禁止 Canvas 回退。

## 复测判据

- Tester 离线：同一 hold 按原 QPC 模拟 10–32 ms 实际拍和 33–160 ms owner 边界，覆盖连续 4 QUANTUM、FIFO 满、拍更新与边界竞态、旧 hold 已认领、真实缺拍/反序、失焦、设备失效和整根 deadline。以相同输入证明旧实现回放 96–99 过期，新实现认领最新合法的相邻真实区间，量子与库存守恒，替换或撤销均只有一个终态。每边界保存 `active/pending`、选入的拍 ID 与间隔、年龄、拒绝子原因、尚未被消费的预约及原提交和终态。
- Effector 在 PC 复跑沙→水、水→沙和切换中原 hold 撤权。每个正控在同 run、owner、context、geometry 中交付两种非零 SOURCE 的 producer/command/Completion、water/sand 同一后续根上的非零库存、原守恒与同 lease 的实际业务着色像素和必要事件。两个独立场景各自产出、信息像素 `0xff525355` 及 WSI 退役只按各自口径登记。离线已复跑 T852 的 4 组合成正控和 15 项负控，真实同场景沙水留待新版 PC 原件。 [双源判据](../test/reports/t852/handoff.md) · [物理与画面合同](architecture-T832-v1.md)
