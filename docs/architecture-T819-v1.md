# T819 · RX SOURCE 命令过期与根内时序

2026-11-22 · 目标20 · P5 · architect · 离线首次归因

**T816 首拒 `PUMP_ENTRY / COMMAND_EXPIRED` 正确。** root 已运行 **249.202 ms**，距 250 ms 的根期限还有 **0.798 ms**，命令 receipt 已达 **250.931 ms**。真实 PagePulse 此刻仅 **4.580 ms**。执行 fence 已创建，5 次执行 wait 全为 TIMEOUT，直到退役才收到同一 fence 的信号。实际阻断仍是提交和队列完成太慢，改命令时间戳无法使这根通过。 [T816 原报告](test-report-T816-v1.md) · [逐段复算](architecture-T819-evidence-v1.json) · [T807 原归因](architecture-T807-v1.md)

## 原件和首拒

T816 未签 `80ac0d6e…` → 已签 `52bc48e1…`。14 个 payload 逐字一致，`libgrs_g9rx.so` SHA `cf12eb71…`，BuildId `ecc2b1195ee9b411de892f422d258276370521e6` 与 T810 回执和侧车一致。同票 **1**，PID **58229** / start **8593780**，owner TID **58436**，run/owner/context/nonce/plan 均 **1**，根 source geometry 为 **2**。首拒 expected/actual scope 相同，clock/lifecycle 分别为 **1/5**。`STOP` 和后续 extent 更新均发生在首拒之后。归档 25,829 B / 34,337 B 与报告 SHA 前缀 `29a759a2 / d36dba45` 一致。本轮归档无独立的 native `SAVED` 日志，因此设备端写出摘要仍以报告为准。 [签包](../test/reports/t257/T816-rx.json) · [T810 编译回执](<D:/hmprojects/HelloWorld-T771/docs/receipt-T810-compile-v1.json>) · [terminal](../test/reports/t491/t816/sidecar-terminal.json) · [retirement](../test/reports/t491/t816/sidecar-retirement.json)

T765 带各自编译回执复读。T816 两份分别为 `REJECT_OBSERVED / REJECT_RETIRED`，同为 `COMMAND_EXPIRED / PUMP_ENTRY`，基础 trace `truncated=false`。T802 v3 两份复读分别为原 `DEADLINE / PRE_FENCE_CREATE` 的拒绝和退役。T785 离线回归通过。T816 分段层独立记录退役 wait 前 8 条和末条，`dropped=55` 是分段固定槽裁剪，原 transport 仍完整保 63 次轮询边。不能以分段槽推算 63 次的完整 owner CPU。

| 同根原值 | T816 | T802 v3 |
| --- | ---: | ---: |
| receipt → begin，ms | 1.729062 | 1.431875 |
| 根起时两道年龄/期限共同允许的最长区间，ms | 248.270938 | 248.568125 |
| 首拒距根起，ms | 249.202344 | 294.081562 |
| 首拒时根期限余量，ms | **+0.797656** | **−44.081562** |
| 首拒时命令 receipt age，ms | **250.931406** | **295.513437** |
| 首拒时真实 pulse age，ms | 4.579844 | 13.346562 |
| 最后通过 guard / pulse age | `PRE_FENCE_WAIT`，2.492032 ms | `PRE_FLUSH`，2.345000 ms |

T816 的 `QUEUE`、`FREEZE` 和 guard 回指同一个原 receipt `86279375512813`。命令年龄边界为 `86279625512813`，根期限为 `86279627241875`，首拒读时 `86279626444219` 落在两者之间。按整数原件复算，修正 T816 摘要所写的 28.2 ms 根期限余量。pulse #16 用实际 native receipt `86279621864375`，16 条原 pulse 最大 native gap **17.411 ms**，原上限 **33.333333 ms**。`SourceTimeGuard` 先判 now > root deadline，继而判真实 pulse 和 command receipt。T802 在首拒时已同时跨过两道 250 ms 边界，记录顺序使 DEADLINE 为首拒。T816 的根前 `ENQUEUED→APPLIED` 是 **318.725 s**，后续 claim 和根内分段独立计时。 [guard 源码](<D:/hmprojects/HelloWorld-T771/entry/src/main/cpp/gpu_resident/g9rx/source_guard.h#L19>) · [原 Offer 冻结](<D:/hmprojects/HelloWorld-T771/entry/src/main/cpp/gpu_resident/g9rx/controller.cpp#L439>)

## 根内 CPU、wall 和 fence

| T816 实际调用 | 相对根起的入口→出口，ms | wall / owner CPU |
| --- | ---: | ---: |
| BASE copy `1→2`，591,872 B | 2.345→2.788 | 0.444/0.441 ms |
| op8/op9 dispatch | 4.006→4.058，含两笔调用及间隔 | 16.250/15.000 μs 与 12.031/12.188 μs |
| 首笔 arena copy `7→8`，12,767,232 B | **4.073→138.904** | **134.831/134.055 ms** |
| 第二笔 arena copy `7→9`，12,767,232 B | 138.935→138.969 | 34.375/34.531 μs |
| cursor 10 submit flush | 138.981→138.984 | 3.437/3.281 μs |
| op10/op11 dispatch | 139.016→139.105，含两笔调用及间隔 | 66.875/67.032 μs 与 7.656/7.812 μs |
| cursor 12 submit flush | **139.115→241.183** | **102.068/101.712 ms** |
| 执行 fence 创建 / fence 后 flush | 241.207→241.240 | 28.281/28.437 μs 与 2.187/2.187 μs |
| 5 次执行 wait / 首拒 | 241.245→247.121 / 249.202 | 五次合计约 **27 μs wall / 26 μs CPU**，全是 `37147` |

三笔 copy 的两个 offset 均为 0，12 dispatch、12 dispatch barrier、3 copy barrier，同代实际 map/typed 均为空。首笔 arena copy 和末批 submit flush 合计 **236.899 ms wall / 235.767 ms owner CPU**，约耗掉 receipt 可用根窗口的 **95%**。T802 对应 wall 为 **287.861 ms**，工件和根不同，差值只能比较本轮形态。T816 无 GPU 时钟；高 wall 和高 owner CPU 指向各 API 在 owner 上的实际代价，无法将成本转算为 shader 执行或 GPU 搬运。

执行 fence handle 1 创建并由原 owner 复用退役，无新退役 fence。原 transport 的末次 TIMEOUT **API 入口**和首次 `37146` **API 出口**给出相对 root 的 **386.010–388.252 ms** 区间。最早下界已越命令有效边界 **137.740 ms**。这与小幅缩短排队或多轮询几次的量级不同。commit/quanta 仍为 0、FIFO 保 1；无当前 typed 结果、有效 Q20、发布或 Swap。

## 工程修订和验法

1. **保持原时钟与命令身份。** 可在原收据和同根 trace 显式记录 `receiptExpiryNs = receiptNs + 250,000,000` 和 `effectiveLimitNs = min(rootDeadlineNs, receiptExpiryNs)`，以原单调 clock 安全计算，供观测及现有 guard 下的唤醒上界使用。原 guard 继续按 deadline、pulse、receipt 的顺序采样，保持 `>` 的边界语义、同 owner/permit 以及不可变 receipt。禁止重签旧 command、用 PagePulse 或 owner wake 更新 receipt、延长窗口或绕开失效。失败仍撤权并由原 owner 真退役。重复真机根沿原一轮 Configure 和真正新 offer 开新作用域，维持原 FIFO/意图去重，不将过期快照复用为新根。
2. **先做可归因的提交对照。** T810 原顺序已证两处高 CPU/wall 调用。可按 T807/T792 做显式 RX 研究臂，保 cursor 10 的 flush、原三笔 copy 和全部 barrier。尾部同根 guard 后插一个覆盖完整批次的执行 fence，只做一次最终 flush，实际出口后按当前时钟重验 permit/pulse/receipt。若 create、flush、post-guard 出错，按原队列所有权与本 fence 走真实退役。空 fence 且已有提交沿原独立退役。默认路径与研究臂用同工件和匹配的实际设备条件比对，留 T810 全部时钟、对象范围、单次调用及全链时间。观察重成本是否移到 fence、末次 flush 或其后的 wait。若仍被 prior op8/op9 或公共提交支配，优先用分离前缀配真实同队列 fence 或可用非阻塞 GPU 时间追原 producer，诊断不计产品 PASS。尚无依据改 copy 范围或直接启用复制候选。
3. **轮询诊断降级为辅助。** T816 实际执行 wait 域已到达，但 5 次 wait 合计只有约 27 μs，先保原 owner/wake；若后续发现足够的 owner 调度或 poll CPU 压力，再单独评估节拍轮询。Tester 保留 receipt 门槛前/相等/后一纳秒、与根期限同时越线的首拒优先级、真实 pulse、不可刷新原 receipt、迟到 signal 仅退役、取消/空 fence/错代和未完成 typed 的负控。Effector 比较原 B/候选从真实 receipt、claim 到实际及时 signal、1024 B WORK 原字、map/unmap、typed oracle 和原子提交，成功仍须 **209,715 Q20、commit +1、quanta 不增、FIFO 消费一次**。冷编译、暖缓存和 XComponent+shader 首个有效 Swap 各有独立收据。GPU 驻留、CPU 停投。 [T792 工程与收益合同](architecture-T792-v1.md) · [T736 原 oracle](architecture-T736-v1.md)

本卡离线完成工件、原件和 reader 核验，未使用真机。
