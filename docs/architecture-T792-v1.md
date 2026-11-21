# T792 · DM SOURCE 真实 A/B/C 归因与 GPU 提交修订

2026-11-21 · 目标 20 · P5 · architect · 离线原件复核。

**B 的定时轮询减少约 70.5 ms owner CPU，C 将约 74 ms 的提交开销移进了 fence 创建。** 三个研究根均在原 250 ms 内没有观察到可供准入的执行 signal，且未进入 map 或 typed commit。下一轮沿 B 的低 CPU 轮询取证，定位第一笔 arena copy 和末批提交的时间归属。 [逐阶段复核数据](architecture-T792-evidence-v1.json)

## 原件及实际路径

T773 `b41656c1` 独立复判 T775 的 `A.jsonl` 及 T788 的 `B.jsonl/C.jsonl`，实际 native arm 分别为 A/B/C。三个根使用不同 PID/start/scope，均为 K192/S40、相同参数、布局及 SOURCE SHA，实际 driver BuildId `71b03f56…`、BOOT DSO BuildId `86e14c1a…`。reader 均为 `DEPLOYED_BYTES_UNOBSERVED`，本地编译收据及设备映射 BuildId 已有，缺完整部署字节回执。A 与 B/C 来自两批采证，单个样本的差值用于定位，批次效应仍待交错比较。 [A 原件](../test/reports/t491/t775/A.jsonl) · [B 原件](../test/reports/t491/t788/B.jsonl) · [C 原件](../test/reports/t491/t788/C.jsonl) · [T773 reader](<D:/hmprojects/HelloWorld-T771/test/t761/read_source_timing.py>)

| 实测 | A | B | C |
| --- | ---: | ---: | ---: |
| cadence / wake 次数 | 0 / 0 | 250 μs / 252 | 250 μs / 246 |
| 执行 poll 次数 | 4,480 | 252 | 246 |
| 三笔 copy 墙时，ms（state / arena1 / arena2） | 0.149 / 61.269 / 0.033 | 0.165 / 76.879 / 0.030 | 0.180 / 81.110 / 0.028 |
| `SUBMIT_FLUSH` cursor 10，ms | 0.0045 | 0.0036 | 0.0034 |
| `SUBMIT_FLUSH` cursor 12，ms | 87.512 | 74.366 | 省略 |
| 执行 `FenceSync` API 墙时，ms | 0.020 | 0.022 | 74.156 |
| `EXEC_FLUSH` cursor 12，ms | 0.0022 | 0.0019 | 0.0022 |
| 根起点→执行 fence 返回，ms | 150.111 | 152.636 | 156.635 |
| T773 根墙时 / owner CPU，ms | 250.018 / 245.621 | 250.175 / 175.077 | 250.151 / 177.029 |
| `PUMP_ENTRY_GUARD` 首拒距期限，μs | +12.344 | +143.438 | +129.218 |
| 同执行句柄在退役首次观测 signal，根起后 ms | 297.613 | 336.187 | 337.963 |

B/C 的 owner CPU 比 A 低 **28.7% / 27.9%**，执行 poll 分别少 **94.4% / 94.5%**。C 末批在 `FenceSync` 内耗时 **74.156 ms**，其中 owner thread CPU **74.055 ms**，接近 B 末批首次 flush 的 **74.366 ms**。两者最终 `EXEC_FLUSH` 都在微秒量级。第一笔 12,767,232 B arena copy 本身的 owner CPU **61.108 / 76.806 / 80.938 ms**，另一次等长 copy 约 30 μs。第一笔大 copy 加各臂末批重调用合计 **148.781 / 151.245 / 155.266 ms**，约占根期限 60%–62%。现有 GPU clock 未观测，调用内 CPU、异步 GPU 工作、排队和实际搬运之间的因果份额仍未知。

三臂都完成 12 dispatch、12 dispatch barrier、3 GPU copy barrier。最后一次执行 wait 原返回 **37147**，退役复用同根执行句柄后实际返回 **37146**，这只是首次观察到晚 signal 的时刻。最后实际 wait 返回相对 deadline 分别为 **+4.844 / −87.656 / −101.094 μs**。B/C wake overshoot 的 reader 最大已观测值为 **374.688 / 158.437 μs**。最后通过 guard 时真实诊断 heartbeat 年龄为 **4.104 / 3.788 / 0.516 ms**，三个首拒均是 deadline，A/B/C 的 SOURCE WORK 未 map，最终 commit/quanta=0、FIFO=1、admittedQ20 留空。采样说明这两项调度改动均未使当前样本赶上期限。T788 的“窗口太小”与“需要放宽”的因果判断尚无证据。 [T788 原报告](test-report-T788-v1.md) · [T784 基线归因](architecture-T784-v1.md)

## 下一轮工程合同

1. **沿 B 取低扰动成本剖面。** 复用 T771 的 250 μs 有界 poll、既有中间 flush 和末批双 flush 作为下一轮研究对照。C 保留作开销迁移对照。默认产品配置、根限额和 `8 ms / 10 pass` 提交分批保持原口径。以现有 `SourceTiming` 固定槽保第一笔 arena copy 前后的 op8 replay、op9 compare、两次 copy 的对象/范围和两段 barrier、op10 pack、op11 seal、最后提交、实际 fence wait、map/unmap 和 commit。原始 API wall/thread CPU 继续分记。保冷前置的 build/cache 独立时间域。
2. **先确定优化落点。** 设备支持可用 GPU timestamp 才取同队列按区段结果、完整 clock domain 和 disjoint/不可用状态，延后提取，避免根内阻塞读 query。否则在可重放的独立非计分诊断运行沿同物理输入取四个 prefix，分别在 op8 后、op9 碰 copy 前、两笔 arena copy 后、op11 后插一个真实队列 fence，保真实排队、flush、wait 和无 commit 终态。每次只有该 prefix，专用缓冲及进程，保持原撤权和安全退役。prefix 干扰及固定前缀以外的差值不计正式根收益。effector 保 A/B/C 或后续候选的交错新进程、完整设备 SHA/映射及系统竞争状态。
3. **成本证实后再落实现。** 若主要开销来自 copy 之前的 compare/replay 排队，优先剖出具体 pass 和依赖内核，改善可并行读写或队列等待，逐字保原 replay、比较、事件和 sealing。若受控证据定位为两次 copy/提交边，可做显式**研究臂**，用 GPU 复制内核从同一来源向两个互不别名的目的 arena 写满原范围，或在已有合格后端下用同队列预录 copy 命令。两者均保 `ACTIVE_EVENTS→SELECTED_EVENTS`、`ACTIVE_EVENTS→AUTHORITY_EVENTS` 的完整原字、绑定代号及 compare→copy→pack/seal 可见性。老 B 继续作对照。新增 GPU 提交的实际 op/dispatch 和原 12 个 SOURCE 算法槽分账，须独立版本化同步/接线证据，禁止填假 copy 计数。首版禁止缩范围、共用可变目的对象或改原 SOURCE shader 算法。新 shader 的完整编译输入最多 49,152 B，GLES 与 Vulkan 的实际同步、编译和缓存独立取证。仅在现有 Vulkan 同 oracle 资格成立时走该后端对照。 [原复制和绑定](<D:/hmprojects/HelloWorld-T771/entry/src/main/cpp/gpu_resident/g9dm/transport.cpp#L870>) · [T736 原 12 槽](architecture-T736-v1.md) · [T768 prefix 归因](architecture-T768-v1.md)

通过条件是同根 250,000,000 ns 内观察实际执行 signal，后续真实时钟和 scope/pulse/receipt guard 仍成立，取得 WORK 原尾 **1024 B**、map/unmap、typed proof、Q20 **209,715**、commit+1、quanta 不增、FIFO 恰好消费一次、原 replay 和事件 oracle 全部成立。每次晚 signal 只退役。Tester 核旧 B 与候选原字、错绑定或部分写、超时/取消及 stale scope 反例。研发修订不延长窗口、不合成 pulse、不把预算前搬到配置阶段执行真实根工作。GPU 驻留、CPU 停投，实际呈现只用 XComponent+shader。T792 未使用真机。 [T736 原 oracle](architecture-T736-v1.md#原-oracle-与观察结果)
