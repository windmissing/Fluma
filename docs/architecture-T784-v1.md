# T784 · DM 与 RX SOURCE 超期归因及修订路线

2026-11-21 · 目标 20 · P5 · architect · 离线复核。

**DM 实测为三次 A 基线复现。** T773 独立 reader 从三个原件均读出 `arm=A`。三轮都提交到第 12 个 dispatch，根期限内没有观察到执行 fence signal。第一笔 12.77 MB copy 加末批首次 flush 占用 148.8–157.6 ms。RX 的终态和已部署源码映射支持 live Pump 内的 `DEADLINE`，尚缺首拒位置、真实 pulse age 和耗时分段。两线继续沿用固定根期限与原提交条件。 [T775 报告](test-report-T775-v1.md) · [T776 报告](test-report-T776-v1.md) · [复核数据](architecture-T784-evidence-v1.json)

## DM · 原件与 arm 身份

`A.jsonl`、`B.jsonl`、`C.jsonl` 分属新的 PID 和 scope，均用 K192、相同 SOURCE SHA、布局和参数。每根的冻结输入和 seed 摘要单独记账。日志的 driver 映射 BuildId 为 `71b03f56…`，BOOT DSO 为 `86e14c1a…`，与 T771 编译回执联核。报告记载设备 MD5 比对，归档缺 T773 所需的完整部署回执，reader 返回 `DEPLOYED_BYTES_UNOBSERVED`。三份 `SOURCE_TIMING` 均为 `cadenceNs=0`、计划唤醒为空、`wake.count=0`，且依次执行 cursor 10 `SUBMIT_FLUSH`、cursor 12 `SUBMIT_FLUSH`、cursor 12 `EXEC_FLUSH`。T771 入口省略 `--timing` 时走 A，归档未保存设备实际 argv，选臂回退的具体原因待核。 [A 原件](../test/reports/t491/t775/A.jsonl) · [B 原件](../test/reports/t491/t775/B.jsonl) · [C 原件](../test/reports/t491/t775/C.jsonl) · [T771 入口](<D:/hmprojects/HelloWorld-T771/test/t670/harness.cpp#L45>)

| 原件文件名 | A | B | C |
| --- | ---: | ---: | ---: |
| PID / startTick | 22926 / 133816 | 24914 / 171040 | 27892 / 223054 |
| 实际 arm | A | A | A |
| 首笔 591,872 B copy，ms | 0.149 | 0.170 | 0.171 |
| 第一笔 12,767,232 B copy，ms | 61.269 | 85.781 | 83.874 |
| 第二笔 12,767,232 B copy，ms | 0.033 | 0.030 | 0.029 |
| 三次 flush，ms | 0.005 / 87.512 / 0.002 | 0.003 / 71.843 / 0.002 | 0.003 / 72.020 / 0.002 |
| 第一笔大 copy 加末批 `SUBMIT_FLUSH`，ms | 148.781 | 157.623 | 155.894 |
| 根起点到 `FenceSync` 返回，ms | 150.111 | 159.114 | 157.283 |
| 执行 fence poll 次数，均返回 TIMEOUT 37147 | 4480 | 4093 | 4138 |
| T773 根墙时 / owner thread CPU，ms | 250.018 / 245.621 | 250.014 / 249.635 | 250.015 / 249.763 |
| 同执行句柄在退役时返回 37146，距期限，ms | +47.613 | +86.980 | +85.713 |

根墙时以 `SOURCE_ROOT.rootBeginNs` 为起点，thread CPU 用 `SOURCE_TIMING.root` 区间。每次均有 12 dispatch、12 dispatch barrier、3 copy barrier。主耗时两段的 owner CPU 与墙时接近，GPU 时间字段为 `UNOBSERVED`。当前可以把约 60%–63% 的预算定位到这两个原生 API 调用，GPU 执行、驱动队列反压和调用内工作各占多少仍待实测。晚 signal 是退役检测时点。三个根都没有 SOURCE map、typed proof 或 admitted Q20，终态 `commit=0`、`quanta=0`、FIFO 保留 1 项。T775 提到的 BOOT 尾属于 BOOT 阶段。 [T773 判据](<D:/hmprojects/HelloWorld-T771/docs/test-report-T773-v1.md>) · [复核数据](architecture-T784-evidence-v1.json)

下表均以固定 deadline 为零。A/C 为 transport `PUMP_ENTRY_GUARD` 首次观察到 `DEADLINE`。B 文件中的 `PRE_EXEC_POLL_GUARD` 在期限前采时，之后 controller 持锁采时越过期限，具名原因为 `DEADLINE`。49 是 pulse 序号。A 最后一轮的 poll 还有一条跨边界的调用证据，poll 自身记录的 permit 采样为 −1719 ns，实际 wait 进入为 +1406 ns、返回 TIMEOUT 为 +4844 ns，根未进入 map 或发布。成功准入仍按后续实钟 guard 判定，重跑须分别保留这几个时点。

| 原件文件名 | 最近通过 guard | 首拒 | pulse age 的实测位置 |
| --- | --- | --- | --- |
| A | `PRE_EXEC_POLL` −3750 ns | `PUMP_ENTRY` +12344 ns | 最近通过 guard 4.104 ms |
| B | `PUMP_ENTRY` −6406 ns | `PRE_EXEC_POLL` −1093 ns，controller 持锁 `DEADLINE` +782 ns | controller 首拒 1.800 ms |
| C | `PRE_EXEC_POLL` −11562 ns | `PUMP_ENTRY` +4219 ns | 最近通过 guard 3.244 ms |

三轮真实诊断 heartbeat 最大接收间隔分别为 5.635、5.496、6.550 ms。A/C 首拒为 deadline 优先短路，首拒时的 pulse age 留空。DM 诊断 heartbeat 与 RX 产品 PagePulse 各沿原入口计时。

### DM 后续执行

1. **先补实际 B/C。** Effector 复用已核对的 T771 工件和输入，保存每臂完整 argv、实际设备 driver/DSO/libc++ 的 SHA、大小及映射路径。使用 `g9_dm_differential INPUT NEW_OUTPUT --timing=B`、`--timing=C`，若沿用 CACHE 则将缓存目录放于 arm 参数前。每次新 PID/start tick 和输出目录。同条件对照可以先复用已核验的 A。最终 reader 必须同时核对请求与原生执行身份，B 为 250 μs cadence 与双 flush，C 为同 cadence 与末批 fence 后单 flush。只有 root 原件的对应时序齐全才进入效果比较。
2. **尽早发现选臂不符。** 如果现有启动日志无法确认实际枚举，developer 在冷编译之前加一条可落盘的选臂记录，带 native parsed arm、cadence、是否使用默认值和进程身份；选臂工具将请求值与原生回显比较。Tester 保持原 reader 的序列检查，外层比较期望与实读 arm，并覆盖将实际 A 文件送入 B/C 的负控。保留一次命令错误的 stdout/stderr 和退出码即可定位部署脚本问题。
3. **按全链判断 B/C。** A→B 比较实际 wake、poll 次数、owner CPU、最后执行状态和根墙时。B→C 比较末批 flush、fence 创建、首次真实 signal 的位置及根墙时，避免把开销转移误记成节省。若实际 C 仍超期，优先隔离第一笔大 copy 前的依赖和末批提交成本。沿 T768 在可用时采 GPU/调度计时，或另取各自独立的 prefix fence，按受控实验记录。研究根的三笔 GPU copy、原 scope/buffer 范围、barrier 和 12-pass 依赖继续沿用。 [T768 隔离方案](architecture-T768-v1.md#工程修订)

## RX · 已证终态与缺证

T776 归档的 `boot-retire.txt` 包含 PID 34145、owner TID 34326、run/owner/context 1/1/1 的八条 RX BOOT 原事件。BuildId `4bdd76dd…` 与 T766/T763 相符。`FINISH_EXIT.raw=37146`、BOOT map/unmap、seal 6559303 和 `BOOT_AUDIT_OK ns=2960786477813` 均属于根前的 BOOT 证明。终态 `l2.json` 记录 `PUMP_FAILED code=6`。同一构建下，T763 的 live Pump `Denied()` 只在 `SourceTimeGuard(now > deadline)` 的 `Deadline` 映射到 6。UI 终态缺 PID/start/root key，结合已报签包和 BOOT 身份可定位到 Pump 根期限拒绝这个故障族，具体哪一条 guard 及其前序 GL 调用仍未知。 [BOOT 原 hilog](../test/reports/t491/t776/boot-retire.txt) · [终态原件](../test/reports/t491/t776/l2.json) · [RX 具名守卫](../entry/src/main/cpp/gpu_resident/g9rx/source_guard.h) · [返回映射](../entry/src/main/cpp/gpu_resident/g9rx/source_timeline.h) · [T772 同包联核](architecture-T772-v1.md)

T776 报告提到的 17 条 `chain.txt` 和 `g9rx-source-*.json` 未在归档找到，`post.txt` 为空。按报告转录的时间端点复算，SOURCE compile 为 **83.760030625 s**，link 为 **234.921729375 s**，`APPLIED` 为 `3279540338125 ns`；这三项只记作报告转录。现有原件无根起点、最近通过的 SOURCE guard、真实 pulse 及命令 receipt 的时钟戳，也无 SOURCE dispatch cursor、执行 fence 或退役 fence。`APPLIED` 属于配置成功点，编译和链接发生在 claim 前。pulse age 须用同根、同单调时钟的 guard `now - pulse` 复算。T776 的“分钟级脉龄”及 RX/DM 共因推断无相应原件支持。 [T776 报告与转录](test-report-T776-v1.md) · [原总结 JSON](../test/reports/t491/t776/v1-same-ticket-timing.json)

### RX 后续执行

1. **由 T785 先闭合侧车导出。** 在原位置有界保存不可覆盖的 `firstReject`、`recentPass` 与 terminal/retirement，再在热路径之外导出。以 PID/start、ticket、run/owner/context、generation、root/nonce 和 BuildId 串同票。原始 `rootBegin/deadline/guardNow`、真实 PagePulse 的序号与单调戳、命令 receipt、owner enter/lock、cursor/已提交数及前一调用起止时点都要保留。若发生 fence wait，分别保实际执行/退役句柄及 wait raw；若从未到达对应阶段，留空并写明原因。把 sidecar 落盘路径、长度、摘要或实际 I/O 错误交到已有的页面取证入口。保存和拉取结果可与终态核对，核不到时保留 UI/hilog 和原错误。
2. **取得一份同票真实入口原件后再选择优化。** Effector 采同签包身份、侧车、完整原 hilog 与 UI 终态。Tester 离线按 T765 及侧车契约复算失败边和年龄，BOOT 与 SOURCE 的 fence 各用原阶段证据。Architect 按首个实证耗时段分流：owner 唤醒/锁间隔为主则量测栈上的调度延迟；单个 copy/flush 跨线则对照该原调用与已提交前缀；执行等待为主才评估 DM 同类研究臂。若实采发现时钟、pulse 或 receipt 拒绝，分别看对应原 guard 和采样顺序。原件齐备前先做取证闭环。

## 共同放行边界

根期限固定 **250,000,000 ns**，真实 PagePulse freshness **33,333,333 ns**，命令 receipt 年龄和 claim/nonce 继续沿原契约。正证据要求当次同根执行 fence、WORK map/unmap、typed proof 和原 oracle 的原尾及事务结论。DM K192 正样本保 **1024 B** 原尾、**Q20=209715**、commit 加 1、quanta 不增、FIFO 恰好消费一次；RX 沿 T736 的本族原 oracle 和实测非零搬移。晚 signal 仅进入同根或同前缀的退役。编译、链接、暖加载、根内准入和首个有效 Swap 各按其真实材料计。GPU 驻留、CPU 停投，渲染仅用 XComponent+shader，禁止 Canvas 回退。 [T736 SOURCE 合同](architecture-T736-v1.md) · [T768 同根验收](architecture-T768-v1.md)

## 离线复核

从 T771 工件工作树复用 T773 reader `b41656c1`，给三个设备原件分别传 T771 编译回执。三次 reader 成功，均输出 `A / DEADLINE / execution UNSIGNALED / retirement SIGNALED / admittedQ20=null`。另外逐原件核对了 flush 顺序、guard/poll 边、两类 fence 和无提交终态，并从 RX 原始 hilog 反解了八个 BOOT 事件。T773 既有 T762 归档、15 个时序样本与 27 个证据负控复跑通过。T784 未使用真机。 [时间戳与归档 SHA256](architecture-T784-evidence-v1.json)

```powershell
foreach ($arm in 'A','B','C') {
  python D:/hmprojects/HelloWorld-T771/test/t761/read_source_timing.py `
    "test/reports/t491/t775/$arm.jsonl" `
    --compile D:/hmprojects/HelloWorld-T771/docs/receipt-T771-compile-v1.json
}
```
