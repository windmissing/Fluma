# T807 · RX SOURCE 在建执行 fence 前超期

2026-11-22 · 目标20 · P5 · architect · 离线归因

**第一笔 arena copy 与末批首次 `glFlush` 合计 287.861 ms，超过整根 250 ms。** 最后一次通过 guard 是 `PRE_FLUSH`，排在重 flush 出口之后的 `PRE_FENCE_CREATE` 首拒 `DEADLINE`。没有创建执行 fence，T768 的 wait 轮询优化触及不到这一根。下一步用 RX 自身工件把 driver 提交与 copy 依赖分开，T792 的改序仅作有条件的配对研究。 [原报告](../test/reports/t491/t802/v3-bootsource-sidecar.json) · [完整复算值](architecture-T807-evidence-v1.json)

## 工件与原件

T802 v3 实际采用 T805 纠正后的 T798 BOOT_SOURCE。未签 `da5ec9b4…` → 已签 `f39bcb3a…`，14 个入包 payload 逐字匹配，剥离 DSO `3891c1e1…`，GNU BuildId `f96d365d162691cdfd7ebfc86d1be291b635d6cc` 与配置日志相符。ticket 1、PID 57798/start 7635555、owner TID 57963、run/context 1/1、nonce 1、plan 1 同根。T765 带真实 T798 回执读两份原件，分别返回 `REJECT_OBSERVED` 与 `REJECT_RETIRED`，均 `DEADLINE / PRE_FENCE_CREATE / truncated=false`。T785 当前回归（含晚 fence、越权 map/commit、无原 pulse 和退役缺信号）通过。

| 侧车 | 原件 SHA 前缀 / bytes | native SAVED SHA 前缀 / bytes |
| --- | --- | --- |
| [terminal](../test/reports/t491/t802/sidecar-terminal.json) | `c8a61f0e…` / 12642 | `a6e0e331…` / 12641 |
| [retirement](../test/reports/t491/t802/sidecar-retirement.json) | `18cba1eb…` / 20598 | `e4747456…` / 20597 |

归档两个 CRLF 都只在文件末尾，内存中将末尾替换为 LF 后，长度和 SHA 与 writer 的 `SAVED` 逐字节重算匹配，原始归档保留。 [签名回执](../test/reports/t257/T802-v3.json) · [原配置及侧车写入日志](../test/reports/t491/t802/hl6.txt#L3006) · [T798 编译回执](<D:/hmprojects/HelloWorld-T771/docs/receipt-T798-compile-v1.json>)

## 拒绝与退役

| 原边 | 相对根起点及耗时 |
| --- | --- |
| 根前编译 → APPLIED | `ENQUEUED→APPLIED` **318.169 s**。BOOT compile/link 约 **0.183/0.310 s**。SOURCE compile/link **85.102/232.279 s**，各自成功，之后才建根。APPLIED 比根开始早约 **4.33 ms**。 |
| 三笔 copy | BASE **0.682 ms**。op9 的 `7→8` 在根起 **5.761 ms** 进入，wall **176.461 ms**。随后 `7→9` **56.406 μs**。原件证明对象号及 API wall，copy range 和 owner CPU 本次无原边。 |
| 两批 flush | cursor 10 的中间 flush **2.344 μs**。cursor 12 的末批 flush 在根起 **182.669 ms** 进入，wall **111.400 ms**。12 dispatch、12 dispatch barrier、3 copy barrier。 |
| 最后通过、首拒 | `PRE_FLUSH` 的 pulse #12 原生 age **2.345 ms**。这次通过到首拒 **111.414 ms**。flush 出口到首拒 **12.812 μs**。首拒距期限 **44.082 ms**，pulse #18 原生 age **13.347 ms**，18 个同根 pulse 最大间隔 **18.397 ms**，均在原 **33.333 ms** 上限内。命令 receipt 此刻 age **295.513 ms**，已跨过独立门槛。原 guard 先判 deadline，记录的唯一首拒仍是 DEADLINE。 |
| 真实退役 | 原执行 fence 和 wait 都为空，owner 独立创建退役 fence（包住 `glFenceSync` 和一次 `glFlush`，合计 **25.313 μs**）。63 次实际 wait，末次 `37146` 后释放。同根相对 deadline 的保守 signal 窗为 **181.428–183.654 ms**。 |

退役信号只确认旧资源可安全释放。两侧车都只有第一次拒绝，提交为 0、quanta 为 0、FIFO 为 1，无 map/typed；因此也没有可报价的 SOURCE Q20 或首 Swap。配置数分钟与本次根超期各有独立原时钟。两次长调用由 owner 串行进入，可能包含前序工作和 driver 提交，现有 API wall 不能指定 GPU 搬运或驱动编译是根内主因。

## 工程交接

- **T768 暂不试作。** 首轮执行 wait 尚未发生。原 250 ms 根、真实 PagePulse 及命令 receipt、同根 guard、取消与退役契约保持。T768 可在后续确实进入执行 wait 且测出 owner 轮询/调度压力时独立评估。
- **先给 RX 加低扰动分段。** Developer 复用 T798 固定槽并对齐 T792/T797 的字段。记录 op8、op9、两笔 arena copy 的实际对象/offset/bytes、barrier、两个 flush、fence create 和真实 wait 的单调时钟及 owner thread CPU，外加各边同根 guard 与 pulse receipt。固定容量并显式报 dropped，禁止根内读回、同步取 GPU query 或额外 finish。若有可靠的同队列 GPU markers，等原资源安全退役后再取值，缺端点保空。原版 B 先作对照。若重开销明显随前序或共享提交移动，优先追其 producer/driver 路径。仅 copy 特有成本能独立站住时再审 RX 自身复制候选。
- **T792 改序研究臂的触发条件。** 待实际同窗 RX B 复现，且原双阶段证据表明末批 pre-fence flush 有可比提交开销，才和 B 交错对照。研究臂保 cursor 10 原 flush 及尾部所有 op/barrier，把 cursor 12 的 guard 放在实际执行 fence 之前，在 fence 后只发一次末批 flush。post-guard 在该调用实际返回后重新核验，失败就沿原同队列 fence 退役。缺 fence、GL/EGL 错、取消、超期均沿原证据/退役。若单次调用变短但重成本转进 fence 或信号未提前，回到提交分段。候选需保原算法、绑定和原尾、独立退役及 stale 呈现，最终以同根实际及时 signal、WORK 原 1024 B、typed/oracle、admitted **209,715 Q20**、commit +1 和 FIFO 恰消费一次判效果。GPU 驻留，CPU 停投，首个有效 Swap 单独以 XComponent+shader 记录。 [T768](architecture-T768-v1.md) · [T792](architecture-T792-v1.md) · [T736](architecture-T736-v1.md)

本卡只读原件、核包和复跑离线 reader，未使用真机。
