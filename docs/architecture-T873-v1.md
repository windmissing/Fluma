# T873 · G13 候选 QUANTUM fence 超期首次归因

2026-11-23 · architect · P3 · Windows PC · 离线复核

**已定位到候选 QUANTUM 提交后，退役等待仍未看到 fence signal。** T868 的五个 QUANTUM 都进入此失败边，原始 `VkResult=2 / VK_TIMEOUT`。日志只保第二段退役等待的返回，缺第一段业务轮询何时停止、是否先撤权、GPU 分段耗时。UHD 630 在本工件及观测方式下未交任何已接受 QUANTUM。具体瓶颈、替代 PC 能否按时通过、数值精度均待实测。 [T868 原始报告](../test/reports/t868/report.md) · [逐轮哈希和复算](architecture-T873-evidence-v1.json)

## 八轮按实际返回和接受边沿登记

256 字 Completion 和归档按原字重读。T868 现场 PE `78ee42e9…` 与 `924cea58` 交接匹配，八份原日志固定于 `0dc3761f`。对照每个实际角色的独立 SPIR-V 和 PE 内唯一原字节，全部匹配。F02/F03/F05/F05-isozero 以日志 `fixtureConfigSHA`/`fixtureStateSHA` 对上输入封套与实际 state 原字，公共 `configSHA`/`seedSHA` 字段仍是 BOOT 预设标识。实测设备为 Intel UHD 630、同一 Vulkan device/driver UUID、queue family 0。八轮 OP65 与 OP66 均合格。F02/F03 各 16 个实际角色，其余各 17 个。各角色的真实 QueueSubmit、Wait 和双轮 OP66 与同 owner 绑定。OP66 覆盖相应角色资格，业务 QUANTUM 双 scope 需要它自己的尾及槽原件。 [T833 两版工件](../test/reports/t833/t861-candidate-handoff.md) · [T868 字段与工件](../test/reports/t868/summary.json)

| 轮次 | SOURCE 实测 | QUANTUM 或退出 |
| --- | --- | --- |
| REST | fence=0，acceptedRoot 非空，有原档 | 850 派发计划已提交，`RETIRING_FAILED_SUBMISSION` → `vkWaitForFences.T833_RETIRE`，`2` |
| F02 | 该入口直接以已核初态开始 | `PRIVATE_PROJECT`，同一失败边 `2`，无私有投影结果原件 |
| F03 | 该入口直接以已核初态开始 | `PC_SPLIT_CANDIDATE`，同一失败边 `2`，无首步、迭代振幅或收敛值 |
| F05、F05-isozero | 两轮各 fence=0、acceptedRoot 非空、各有原档 | 两轮均同一失败边 `2`，无闭盒场数值结果 |
| revoke-before 首试 | `T833_REAL_PULSE_ABSENT`，无根计划、无提交 | 候选初始脉冲窗口未放行，未触发待测撤权注入 |
| revoke-before2 | 16 槽已录制，`REVOKED_BEFORE_SUBMIT`，无根提交或归档 | 正确终止，未开 QUANTUM |
| revoke-after | 实际 SOURCE submit 后撤权，fence=0，有 1,530,368 B 原档，`acceptedRoot=null` | 旧候选已退，未开 QUANTUM。该档只作撤权证据 |

T868 共三笔 SOURCE 真接受及一份 SOURCE 撤权原档。四个 SOURCE 的 fence 均返回 0。五个失败 QUANTUM 都实际走到提交后的等待。每条 `deadlineNs-grantNs=250,000,000`，并有 `RETIRING_FAILED_SUBMISSION`。`Check(vkQueueSubmit)` 已通过，`inFlight` 为真，尾文件不存在。最后 `FAILED` 由 `RunBoot` 收尾后交给窗口线程，析构通过 `vkDeviceWaitIdle` 清理，原件未记其原返回和耗时。现场终态的 `stage=STEADY_OP66` 是资格阶段残留字段，`detail` 和失败的根计划才表明位置。V1 以 `git show 924cea58:entry/src/main/cpp/gpu_resident/g13vulkan_boot/t833_candidate.inc` 核对。 [候选实现现行 V2](../entry/src/main/cpp/gpu_resident/g13vulkan_boot/t833_candidate.inc) · [退役/输出](../entry/src/main/cpp/gpu_resident/g13vulkan_boot/boot.cpp)

## 有证等待与仍缺的原因

V1 在 grant 后创建并录制本根命令，先检查 `readPulse`、当前窗口和 deadline，再提交。随后在 `attempt.current() && now<deadline && readPulse()` 成立时按 2 ms 请求轮询 fence。循环退出时，旧代码保 `timely=(首段最后返回==VK_SUCCESS)`，其余非 device-lost 路径再以 250 ms **请求**等待同一 fence 退役。字段 `t833QuantumFence` 被后一段返回覆盖。真实 `2` 表明那次退役等待没有等到 signal，Vulkan timeout 的粒度由实现决定。请求值加到 grant 上无法得到现场完成时刻。原件缺首段最后一次调用、业务截止点、实际提交/退役 QPC、`readPulse` 状态和 GPU 时间戳。窗口撤权、UI pulse 停顿/观察跳拍、实际等待到 deadline 和它们与设备积压的组合都需要新增事件区分。`T833_REAL_PULSE_ABSENT` 另属等首批 pulse 的 250 ms 初始化窗口，不能用来证明其余五个根在同处失败。 [Vulkan fence 等待](https://docs.vulkan.org/refpages/latest/refpages/source/vkWaitForFences.html)

失败 QUANTUM 每根录制 850 个 compute dispatch、850 条逐派发 compute barrier 和 35 次 GPU copy。已经记录的是结构和范围，执行到第几个 pass 未知。中途 14 槽各两次 latch/首错 copy（合计 2,352 B）并插入 compute↔transfer 依赖，末尾还录制 1,024 B 尾及六整段，V1 共 1,532,720 B 搬移计划。这些证据指令全部位于同一候选 fence 内。源码级计数亦显示 850 次 pipeline bind 和 820 次 descriptor bind，其中按真实角色/集合变更计只需分别 160 和 6 次。336 个原逻辑槽属于 `PRESSURE_V`/`VISC_BLOCK`，实际 active、收敛 NOOP 和各组耗时待采。静态计数提供性能排查线索。 [计数及角色表](architecture-T873-evidence-v1.json) · [候选绑定](../pc/vulkan/root_plan.h) · [逻辑计划](architecture-T607-g13-dispatch-v1.json)

`readPulse` 的 freshness 来自观察到的 latest-only QPC，可能遇到真实 UI gap 或跨过中间 pulse。它和 M1 T867 的具体时钟谓词各自出具原错。先复用有序采证/收达事件的手法，T868 未留下可将两者定为同一根因的记录。当前 `99994ce2…` series PE 已补提交/fence QPC 和 30 步、V2 场导出。其总体 fence 字段仍在退役之后记录，需要补细分首段。该新版另算工件，本次八轮均为 `78ee42e9…`。 [T867 时钟证据分流](architecture-T867-v1.md) · [V2 交接与 ABI](../test/reports/t833/t861-candidate-handoff.md)

## Developer 可并行实施的测量和最小优化

1. **锁存第一退出原因并保实际退役。** 按 root nonce/ticket/device/queue 记录 `grant → recordBegin/End → submitEnter/Exit(VkResult) → BUSINESS_WAIT first/last → stop → RETIRE_WAIT enter/exit(VkResult) → tail map/check → ownerAccept`。对业务停止按实际谓词记 `WINDOW_REVOKED`、`PULSE_MISSING/STALE/GAP/REVERSED`、`GRANT_DEADLINE`、`FENCE_RESULT`，并保最近有效 pulse 原序号、QPC、age/gap、窗口代、真实 requested timeout、deadline 和每次等待的实际开始结束。短轮询实参取 `min(2 ms,剩余期限)`，越界或失权后的等待只退役。信号、device-lost、隔离和队列清理另有终态，保实际 `VkResult` 及对应 fence。对每个已完成根再在短锁内核 scope 与当前 QPC `<=deadline` 后接受。250 ms 和 33,333,333 ns 保原值。复用定容记录，写文件在安全点。
2. **按实际硬件分时测量。** 用 host QPC 分清 grant→录制→submit、submit→业务退出、退役、tail 检查和冷文件。设备队列支持 timestamp 时，保实际 `timestampValidBits`、`timestampPeriod` 和查询可用位，记录上传完成、ACTUAL/REPLAY 及压力/黏性组、尾复制以前、证据复制以后等少量点。GPU ticks 只在自身时域核分段，host QPC 用来判期限。各 query 在实际完成可安全读取后保存，始终保早期失败记录。该记录可测 host、等待、求解和复制各占多少，再决定相应优化。 [Vulkan 时间戳查询](https://docs.vulkan.org/spec/latest/chapters/queries.html)
3. **最小 A/B 优先移出整段取证。** A 冻结全证据路径。B 保原 850 顺序、原 compute barrier、每槽 latch/首错和 1,024 B 热尾，业务 fence 内只保热尾加 2,352 B 槽证据。接受后拿实际该根 bank/pack/CTRL 的同代 pin，在下一根复用前独立 cold copy 并记录关联的 fence 后导出 V2。暂时没有安全读窗口就登记该根未采。这样先隔离六/七整段搬移成本。若中间 transfer 同步是瓶颈，再把每槽原字留在有界、不复用的 GPU scratch 后一次取回，需等价验证所有 14 槽和首错，禁止跳过 latch 检查。主/证据路线的优先级和超期保持如实记录。
4. **减少冗余绑定，保数值与顺序。** 同一 command buffer、pipeline layout、descriptor identity 未变化时复用当前绑定。按实物句柄验证静态的 `850→160` 和 `820→6`，push 及 16/820 逻辑时序照原执行。组内且资源范围无冲突才按原 ACTUAL/REPLAY 语义考虑合并屏障。先用真实分段数据判断是 host 录制、低优先级 pulse 或 GPU 算子占主导。若压力/黏性组仍为实际热点，另作等价的局部执行优化，用 F02/F03/F05 原字和独立数值参考复测。836 后续融合另取证。所有候选保旧容量、守恒、事务、数值和 GPU shader 画面，禁止 Canvas 回退。

## 可选替代 PC 与验收

- 有合格的独显 Windows PC 时，可用原 `78ee42e9…` Release PE、同三组冻结 Config/State、同 Win32 pulse 来源及同日志模式，跑同工件的 F02/F03/F05 与两个撤权正控。记录完整 argv、GPU/driver/device UUID、同类 Vulkan 能力与 FloatControls、实际角色 SHA、QPC、真实原件及退出。优先 F02 诊断和 F03 单步，F05 用原家族。若历史执行参数有缺，按同一明确 argv 建新的成对对照。例如两端都使用 `--g13-candidate <新的档案基名> --g13-fixture F03 <F03-config128.bin> <F03-state.bin> --g13-trial 1`。目标用户的独显性能实测按已定交接口径。替代 PC 首根若在原 250 ms 内通过，只说明该工件在该适配域可运行。V1 与 V2 各用对应 SHA 和 parser，开发机上的分因及采证可独立推进。 [冻结输入](../test/reports/t868/fixtures/fixtures.json) · [T868 原工件](../test/reports/t868/report.md)
- T874 可先用既有 3 份 SOURCE 正档和 1 份撤权 SOURCE 原档验证 V1 封套、摘要及接受身份，V2 parser 沿 T833/T866 交接。业务证据要实际非零 QUANTUM，匹配 `ROOT_INIT → ACTUAL → REPLAY → COMPARE → PACK/TAIL`、本 scope 的真实 fence/owner acceptance、冷快照与字段。F02 走 PRIVATE_PROJECT 专用投影判据，保持 `acceptedRoot=null`。F03 首根后按 series 30 个**已接受**根量振幅及收敛。F05 正式 FULL 收实际速度/压力/黏性，与 T631/T721/T866 原阈值逐项比对。先定位真实工程超期和撤权，再判断实测数值是否需要算法迭代。生产 16/820 逻辑映射、850 候选发布和目标 14 的 `GridView` 前置沿原合同。 [T866 实际验收](../test/reports/t866/handoff.md) · [T828 数值合同](architecture-T828-v1.md)

Developer 实施时序边沿、取证隔离与绑定优化。Tester 承接离线字段/档案检查。Effector 在现有或可用 Windows PC 实跑，architect 按首个可证失败判工程或设计。T873 已完成八轮原件核对与修订/采证分流，本次未使用 GPU 运行，不能据结构计数提升目标 13 的验收阶段。
