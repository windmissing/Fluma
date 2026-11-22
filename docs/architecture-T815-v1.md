# T815 · DM 前缀样本分母与复制成本裁定

2026-11-22 · 目标20 · P5 · architect · 离线复核

**T809 的原件是五种模式各一次，共一轮。** 归档只有一组可作初筛的 B 和四前缀，按 T799 严格口径已证完整配对为 **0/5**。本轮 `COPIES−OP9` 原始 fence 夹逼差 **4.425–6.829 ms**，与 T804 历史一轮 **6.108–8.592 ms** 方向一致。两轮都没有支持 ≥20 ms 的 copy 专属成本，约 90–98 ms 的 owner 开销又出现在没有 arena copy 的 OP9 fence 创建里。**保持 `COPY_COST_UNCONFIRMED`，本轮可以停止复制候选专用补组，设备时间优先测 copy 前 compare/replay 和公共提交。** [T809 原报告](test-report-T809-v1.md) · [逐根复算和 reader 结果](architecture-T815-evidence-v1.json) · [T799 原门槛](architecture-T799-v1.md)

## 样本身份及实际分母

- 五份 `execution.jsonl` 各只含一个 `ATTEMPT_IDENTITY`、一个 `SOURCE_ROOT`、一个终态和一套时序。四个前缀各有独立 `GPU_SEGMENTS` 及原 `DIAGNOSTIC_PREFIX STOP`，B 的标准原件止于 `SOURCE_TIMING`。五个 PID/start tick 与 scope 各异，原单调时间支持 B→OP8→OP9→COPIES→OP11 的执行顺序。Git 原交付和归档只有这一轮。T809 原报告的“5/5”准确覆盖五个模式形态，不能用作组间方差。 [T809 归档汇总](../test/reports/t491/t809/v1-five-interleaved.json)
- 五份原件都是 K192 / Maleoon 935F，实际 `arm=B`、无显式程序 cache 配置，roleSet、SOURCE、参数及布局 SHA 一致。设备侧实际映射 driver `09befda4…`、BOOT DSO `962b06ec…` 与本机 T797 回执和实物匹配。已复核 driver、HAP、包内 DSO 与配套 libc++ 的 SHA/长度，以及 HAP 内 DSO 与分发副本逐字相同。报告声称同 K192/S40 并核在位 MD5，归档缺实际 argv/退出壳记录、冻结输入文件及设备完整字节部署收据。T773 五份均为 `DEPLOYED_BYTES_UNOBSERVED`。原 `inputSHA` 含本根 scope、nonce 和时间，`seedSHA` 含写入 state 的运行身份，差异符合源码接线，二者不能直接用作跨进程恒定的文件 SHA。 [T797 交接](<D:/hmprojects/HelloWorld-T771/docs/receipt-T797-compile-v1.json>) · [Input128 写入边](<D:/hmprojects/HelloWorld-T771/entry/src/main/cpp/gpu_resident/g9dm/input_gate.h#L14>) · [seed 注入边](<D:/hmprojects/HelloWorld-T771/entry/src/main/cpp/gpu_resident/g9dm/transport.cpp#L376>)
- B 到 cursor 12 首拒 `PUMP_ENTRY_GUARD / DEADLINE`。原执行 fence 最后观测 `37147`，归退役复用同一 handle 后首次 `37146`。四个截点均按 `WOULD_BLOCK` 停止，进程诊断退出 0，执行 fence 未尝试，退役各自新建 fence 且 `DRAINED`。冻结的根期限均为 250 ms。五臂均未进入 map/typed，commit/quanta=0、FIFO 留 1。晚信号只覆盖退役。前缀 GPU clock 为 `NOT_REQUESTED`，T809 无新增可用 GPU timestamp。
- **分母**。模式形态 **5/5**，归档有 **1 轮**可复算，本轮设备/输入回执齐全的 T799 配对数 **0**。若能找回原部署字节、输入和实际运行回执，T809 至多补记 **1 组**。T804 无可核的同窗 B，归历史截点重复。T801 的五次完整 B 归超期重复，不能和两轮前缀拼出五组。未取得合格组间 max−min 或方差。 [T806 建组规则](architecture-T806-v1.md#续采合同) · [T801 历史原件核验](architecture-T803-v1.md)

## 用实际 wait API 夹逼

| 原件 | PID / start tick | 游标 / dispatch / dispatch barrier / copy barrier | 根 wall / owner CPU，ms | 新建退役 fence wall / CPU，ms | TIMEOUT 入口至 signal 出口，距根开始 ms |
| --- | --- | --- | ---: | ---: | ---: |
| [B](../test/reports/t491/t809/x-B.jsonl) | 29008 / 8159014 | 12 / 12 / 12 / 3 | 250.184 / 188.450 | 复用执行 fence | 348.455–349.851 |
| [OP8](../test/reports/t491/t809/x-op8.jsonl) | 32008 / 8212679 | 9 / 9 / 9 / 1 | 1.360 / 1.313 | 97.159 / 96.626 | 282.504–283.700 |
| [OP9](../test/reports/t491/t809/x-op9.jsonl) | 35460 / 8269614 | 10 / 10 / 10 / 1 | 1.175 / 1.179 | 98.119 / 97.970 | 320.100–321.298 |
| [COPIES](../test/reports/t491/t809/x-copies.jsonl) | 39012 / 8322804 | 10 / 10 / 10 / 3 | 91.821 / 91.792 | 0.035 / 0.035 | 325.722–326.929 |
| [OP11](../test/reports/t491/t809/x-op11.jsonl) | 42328 / 8376165 | 12 / 12 / 12 / 3 | 95.938 / 95.892 | 90.412 / 90.350 | 348.278–349.424 |

根 wall 从 `SOURCE_ROOT.rootBeginNs` 计到 `SOURCE_TIMING.root` 出口，与 T773 同定义，owner CPU 按该段的线程时钟。各 fence 下界取最后实际 `37147` 的 API **入口**，上界取首次实际 `37146` 的 API **出口**。以各自原根起点为零。差值为 `[L_after−U_before, U_after−L_before]`，不以 wait/poll 次数乘间隔。按原整数纳秒计算如下。

| 根间差 | 整数原值，ns | 原始差区间，ms | 解释 |
| --- | --- | ---: | --- |
| OP9 − OP8 | `[36,399,530, 38,793,749]` | 36.400–38.794 | 优先检查 op8 replay 排队到 op9 compare 的提交和队列工作。 |
| COPIES − OP9 | `[325722188−321297656, 326928750−320099687]` | **4.425–6.829** | 两截点差，含跨进程、提交顺序、观测与先前排队开销。 |
| OP11 − COPIES | `[21,349,063, 23,702,187]` | 21.349–23.702 | 保留 op10/11 pack/seal 与尾部提交的调查方向。 |
| B − OP11 | `[-969,687, 1,572,812]` | −0.970–1.573 | 执行 fence 和退役 fence 重排后，两个原根完成时间区间重叠。 |

COPIES 的两笔 arena 各为 **12,767,232 B**。首笔 API **90.454 ms wall / 90.423 ms owner CPU**，第二笔约 **30.469/30.937 μs**。OP9 未提交这两笔却在退役 fence 创建花去 **98.119/97.970 ms**。B 的首笔 API **93.113/93.032 ms**，末批首个 submit flush **79.540/79.541 ms**。四个前缀独立诊断 flush wall 均约 2.5–3.75 μs。观测支持重开销会落到不同 owner 调用，无法从单个 API wall 给 copy 或 GPU kernel 定责。原根 250 ms 已超出，B 到实际退役 signal 约 **349.851 ms**。两轮 copy 截点差在描述上共有 **4.425–8.592 ms** 包络，端点跨度 **4.168 ms**。这是跨历史非配对的观测跨度，T799 的有效组波动和已校正的专属成本下界仍为空。即使暂用两轮的原始上界作快速筛查，也未到 20 ms。 [T804 复算](architecture-T806-evidence-v1.json) · [B/C 提交迁移](architecture-T792-v1.md)

## 何时再开复制候选

1. **当前停专用补组。** 保留 T799 的实现合同与原事务路径。本轮不因名义分母继续花五组冷配置时间，不实施双目的复制。下一次已授权的 DM 取证优先沿 T792/T797 聚焦 copy 前的 op8/op9 及公共提交，对照 B 的真实 root→fence 和同窗队列端点。若设备 GPU clock 只有既有 op8 单对或出现 disjoint，继续使用原前缀与实际 API 原件。必要时在独立不计分研究臂复核区段，避免生产根插同步读 query 或附加 fence。现有数据只给排查优先级，具体 producer 或公共 driver 阶段需用进一步证据分开。
2. **只在出现可归因的新证据时重开。** 若后续同队列的完整可用 timestamp，或经控制提交差异的前缀加 CPU/wall 交叉证据，显示 copy 及其特有依赖可能稳定 ≥20 ms，再按 T806 精确命令补齐同窗五个有效组。每组各跑 `--timing=B` 及 `--timing=B --prefix=op8|op9|copies|op11` 的四个单独命令，模式轮换，唯一进程和输出目录。恢复 T809 原回执并满足同窗匹配时可计 1 组、至少补 4 组。否则安排五个完整新组，历史前缀只作旁证。交设备映射路径、三件套 SHA/字节、冻结 `config.u32/settings.u32/seed.u32`、生成 `source-command.u32` 原件及 SHA、原 argv/RC、原终态全行、设备负载和各组剔除原因。只有 copy 专属成本保守下界 **≥20 ms** 且 **≥2× max(有效组间范围, 采样分辨率)** 才放行研究臂。
3. 保同根 **250,000,000 ns**、真实 guard/pulse/receipt、原 oracle、GPU 驻留及原退役。T809 的 B 和前缀均无有效 SOURCE。产品首个有效 Swap 仍沿 XComponent+shader 独立取证。 [现有命令与采集要求](architecture-T806-v1.md#续采合同)

离线已复跑 T773 五份，独立核对身份、时间戳和夹逼公式。未使用真机。
