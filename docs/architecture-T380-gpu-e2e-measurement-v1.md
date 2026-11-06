# T380 · GPU M1 端到端收益计量合同 v1

2026-11-06，architect。不使用真机。GPU compute 为主线 P5。T376 先按 T341 W1–W5 验预热和双源闭环，T379 独立判读冷时间线可并行。本合同供闭环完成后的收益卡消费。本文核合同、包交付和源码接口，设备收益待测。

## 1. 基线和先决事实

| 证据 | 可消费范围 |
|---|---|
| T330/T337b | 末批 fence 和首批 flush 曾分别有约37–41/33–35ms 阻塞，部分 run GAP 取消且 accepted=0。它们是提交与调度证据，未形成端到端收益分母。 |
| T341/T324 Amendment 3 与 T375 | 真实 pass0 工作组+flush 在 READY/首 anchor 前，stage42 与 `warmupWallNs` 记原值。T375 签包本地 SHA256 实核 `b63d6ec9843cee97d352259f8ed886d377ad2914cbbd23c58e70f94212eb0fe1`。host GLES 预热约43.66s 归 host pbuffer，不外推真机。 |
| T264 与旧 GPU T164 | T264 的 M0 真机受桥接阻断，完整成果不可测。T164 是较早三路线和包，保留各自结论，不能移植成 M1 加速比。 |
| T347/T352/T361 | CPU V2 空场约12fps，停源负载约1.3fps。T361 对 18399de8… 包的追回 ON 预算明确 128 帧中位 767.742ms、hotApply 634ms。该档是历史慢窗，实时 A/B 须重新采同输入。 |
| T368/T374 | 该 run 约1.2fps，hotApply 占帧内85.107%。冷档形态与报告 T366 包不符，签包绑定与内部六段缺证，不能充当修订后 CPU 基线。 |

GPU M1 使用 Q20、固定 1/120 成功量子、分段碰撞和水包/沉积水，CPU V2 使用干场 F64 与页面 `dt` 上限和最多2子步，倾角/源包/预算与显示也有实际差异。两边都支持沙的共同任务不意味着数值轨迹、提交数或像素逐位一致。冻结配对域前，确认 GPU 的 settle/flight 指标、CPU 的 AIR/CONTACT_WAIT 和实际 LAND 字段含义。保持已存在的完整校验、事务和原有时钟语义。

## 2. 同一任务的可比边界

**A0 空场和冷启动**。同设备、同容器域128列×容量256、同 native 像素宽高/盒、方向、刷新目标、窗口生命周期。记录起始 stock=0、首个可见完整 scene 的同等要求。分别测新进程冷启、同进程新 run、预注册空闲时长后的重启。不得将重复 swap 或没有真实工作的新 CURRENT 当有效物理吞吐。

**A1 沙单源交集**。初始干空场，GPU 令 W/J=0，双方使用相同容器像素盒、注入点的归一位置与 material 坐标、单 H、相同容量档 K192/K512，accepted tilt=0 为首个公共夹具，shake=0。传入相同有序 offered 记录 `(fixtureSeq, sourceKind, intendedTime, x, y, amount, seed/configId)`，优先用 Q20 可准确表达的量和位置，再在双方验证实际接受值。CPU 和 GPU 的运行内部 request id 可不同，通过 fixtureSeq 绑定映射。若 normal touch 将同脚本转换成不同 offered 记录，保留为“同手势整页比较”。要做“同 offered 工作量比较”，使用同脚本驱动原 source admission 的隔离诊断入口，并验证相同语义的输入仍经原准入，不把 UI 事件重复当成准入成功。最终性能结论需说明采用的入口。端到端产品测试还要单列原真实触摸入口的延迟。

预注册阶段（推荐起点为同一空场的三段）为无源短窗、固定输入窗、明确 UP 后的停源窗。源坐标/高度与时间表、每窗 offered 质量/个数、目标成功物理时长、最小有效可见样本、墙钟超时、排除规则在开始前写入 manifest。至少一个空场和一个持续同负载的高活跃窗，记录每个边界 live、实际粒径/包数、出生/退役/等待量和 GPU 的水包数。双方分别跑到同一**成功物理时长/同语义结果锚点**和同一**墙钟上限**，输出两种分母，超时保留实际进展与未完成结果。CPU 用成功 substepDt 求和，GPU 用完成且 accepted 的 `quantumSerial` 增量×1/120，排除 SOURCE-only root 及预热，处理 reset/epoch。不能用 callback、commit、root 或 swap 数乘固定 dt 代替。

**共同输出判据**。同等功能至少包含接受/拒绝量与原因、分相守恒/容量/无非法状态、成功模拟时间进展、真实粒子与床的同 token CURRENT、选定的出生/离床下降/LAND/床增等任务锚点。结果比较定义在共同材料单位与合法容器坐标上。容许 Q20 与 F64 表示差异，轨迹或床量的业务容差须由相应设计合同预先给出，缺容差就按锚点分项与实际差值登记。相同视觉目标、分辨率、完整图层与像素格式是整页收益比较前置。各路线 shader/材质若没有同等光学档，拆成 simulation-through-lease 与各自 full-scene 成本，并用双方显示质量说明。CPU 受阻停滞历史证据按 T353/T377，若测试配对有一边未达到指定锚点，分类 `FUNCTION_NOT_COMPARABLE`，记录各自结果和局部段，不给统一 GPU/CPU 倍率。

**A2 GPU M1 双源**。真实沙水 source→accepted→completion→GPU lease→同 token scene→swap 单独测试，记录 W/J 分账、拒量、沉积及后续交互。CPU V2 干场缺等同双源功能，给 GPU M1 自身容量、时延和吞吐；CPU 列 `UNSUPPORTED`。不得以仅 H 场景的 CPU 时间作为双源加速比分母。未来如接其它 CPU 沙水路线，另建配对合同及版本。

## 3. 三条时间线和分母

全程保存原生 monotonic ns 和原始来源名。GPU `STEADY_NS` 事件以同一 owner epoch 的 ns 配对。CPU `rawCallbackIntervalNs` 和 `Date.now()` wall 是不同域，不能直接相减得到分段时序。若要把 UI 入队/触摸和 native 时间放在同一条链，使用同一单调时间接口或 cold 采集 `(UI before, native timestamp, UI after)` 映射与误差界。若误差大于被测量级或映射缺失，报告各域局部时间和端到端锚点缺证。禁止用 UTC 文件时间拼亚毫秒延迟。U64 从十进制或高低词按整数读出。

| 层 | 采集点和计算 | 判据边界 |
|---|---|---|
| 冷启动和首帧 | 从 launch/reset 接收到创建、程序/资源、stage42 预热、READY、AnchorSet、第一笔有效源接受、首有效物理 CURRENT SwapEnd。分别报 init→READY、真实预热 wall、READY→first useful swap、整体 launch/reset→first。CPU 同采初始化和首 frame。 | stage42 绑定同 run/epoch/surface/TID，root=0，恰一对并与 `warmupWallNs` 一致。首热 SubmitEnter/Exit按 T341 W2，分批 flush按 W3。预热在热预算/GAP 外仍计用户冷启动。保留无税或复税的实际结果，GAP 窗与8ms不改。 |
| 稳态 CPU/提交/工作 | CPU 帧原始 callback 间隔、成功 Q/A、互斥 wall/step/hotApply/pack/nativeDraw。GPU 从 Offer/Consume、Pump/Submit per root/batch、Dispatch、Fence 创建、Poll、completion/map/校验/accepted，到 lease draw、SwapBegin/End。按同线程嵌套关系扣子段，跨线程按各自区间或 critical-path，重叠段禁止相加。 | Submit/DispatchExit 是 CPU 返回。fence 创建耗时也是 CPU wall。首信号观测只界定提交→ready latency，并受排队、flush 和 poll 频率影响。root GAP、取消、退队、未完成及重复 swap 单独计数，保留墙钟成本。 |
| GPU 真异步执行 | 可用设备 timer-query 才采 GPU 时间。针对一条 GL stream 的实际 dispatch 段及 render 段打 query，限定 ring/采样率，延迟轮询可用结果，记录 disjoint/支持与关联 root。跨 Pump/上下文的分段单独记录，不能把两次 query 间空闲/其他工作全计为本 root kernel。 | 无支持、disjoint、超时或对照显示明显扰动时按 `GPU_EXEC_NOT_MEASURED`，仍可报告提交、fence观测及端到端 wall。禁止 glFinish、同步读 query、增设强制 fence/flush以获得“可测”。 |
| 用户端到端 | 同 fixtureSeq 到 accepted/首次相应内容的完整 swap，另报 UP 后首推进/首 LAND 或可解释等待。区分 accepted 而未 render、render candidate 与实际成功 swap。 | `requested==renderedToken`、generation 与 commit 对齐，且交换成功才计新帧。系统实际呈现仅在可关联的显示证据可用时确认。`displayPresentVerified=false` 或未采系统显示则标 `SWAP_CONFIRMED / DISPLAY_NOT_MEASURED`。 |

每窗同时报 `成功模拟秒/墙钟秒`、唯一新 CURRENT 帧/墙钟秒、accepted 质量与拒量/墙钟秒、成功根/量子和各阶段 P50/P95。根计数、子步和量子只用于本路线工作量解释。吞吐均以整窗 wall 为分母，包含失败、GAP 与无进展的间隙，另列有效样本数。延迟统计只对完整配对样本计算，并单列超时/未配对比例，禁止剔除失败后宣全窗通过。重复 CURRENT 同 commit/内容只作为显示重复率。收益仅在同窗口、同任务与输出判据成立时给 `CPU ms / GPU ms` 或吞吐反比，附逐 run 统计及范围。冷启动、首有效帧、稳态收益各报各的，不加总分位数。

## 4. 复用接口、缺口和实现边界

1. **CPU**。复用 `P051DerivedV2Demo`、`P011FrameBudgetV2`（最近128帧/256子步）、`P051DerivedFacadeV2` 和 `DiagnosticColdFiles`。锁定实际签包及入口，给成功 elapsed/step receipt、source sequence→admission、frame→requested/actual 和 accepted mass。现有 legacy 预算可能无子步或路径身份，按自带 schema；T367 的累计 elapsed 和诊断锚点可复用，避免热态全池导出。若 CPU ON/OFF 需比较，最小单调间隔/事件层在两臂一致，详细阶段只在 ON 样本采。
2. **GPU**。复用 `GpuResidentM1.ets`、`GpuM1DemoController` 的 `status()/offer()`、`app_bridge.cpp` 的 `timeline(cursor)/diagnostics()`、completion256 与 scene receipt、`GpuBridgeTimelineCold`、`DiagnosticColdFiles`。原 timeline `T295_STEADY_TRACE_V1`、stage42 字典和4096行原件保留，T379 独立解析。`status` 是最新采样，`diagnostics` completion 前32条会 drop，不能用末状态补造逐帧证据。`RootSubmit/Poll` 与 `FenceExit` 时点必须按实际语义使用，fence 创建与已见 signal 另标。现有 `SwapEnd` 和 `DrawEnd` 有 status 但长窗需有按 root 到 full token 的固定关联记录。必要新增单独冷 schema 的定容收据，键含 `(backend,build,run,epoch,surface,context,geometry,input,rootNonce,sourceCommit,physicsCommit,publicationGeneration,renderFrame,requestedToken,renderedToken)`。另列 source seq、accepted quantum delta、queue wait、fence ready observed、map bytes/time、lease/result 与 present/clock。大整数保持原值，热路径不拼接 JSON。
3. **导出与扰动**。既有冷时间线在计量窗外导出稳定前缀，并按 manifest 的文件 SHA、row serial、stageRevision、epoch/owner 检查 `COMPLETE` 与缺页。增长/覆写/取消登记原错，只用覆盖闭合的分段，不将4096行当全 run。长窗采用定容 run 汇总与抽样短窗原始行，拒绝静默截断。签包 SHA 来自真实装包回执与签名清单，UI 输入值仅用于关联。比较模式分 `MINIMAL` 与 `DETAIL_SAMPLED`，同输入分别跑，配对可见结果及成本；读回、冷 hash/导出、timer query、截图与观测日志单列耗时和 bytes。测量窗内不主动导出/拉档，不在普通绘制加入 readback。像素证据在独立同 token 锚点采，记录设备和 target 层。
4. **资源**。报每窗实际 N/K/Kw、accepted/rejected quanta、dispatch/barrier/fence/map 计数、CPU/GPU 已知 buffer+target bytes、peak 和 driver `NOT_MEASURED`；GAP 后资源重建计入生命周期。GPU 只在摘要/诊断边界读回，不为计量热路径搬全池。保留背景和前景的 XComponent+shader 完整展示，故障只报 FAULT 或完整旧 token STALE。

## 5. 独立验点与接力

- tester 先消费 T376 同包 W1–W5 结果及 T379 工具。在正式收益试验前核冷预热/热首批/完整同 token，区分 “提交通过、已交换、显示实证”。如果 W 门失败，报告该路线当前不可比，性能数字仅保留能实测的分段。T380 工程稿及任何计量补口不作为 T376 前置。
- developer 只在实际接口缺口处新增版本化采样收据、同负载回放和时钟关联。离线校验采样开关状态等价、原事件/时钟/nonce/rollback 语义、无额外同步等待、导出覆盖与丢样。设备验证另卡。
- tester 固定 HAP SHA、设备/系统/刷新档/电源/温度起始条件、分辨率及输入脚本，交替 CPU/GPU 次序，每档至少三次独立 run，保留每次原始回执。窗口与双源功能对照先判，之后才报告收益。独立负控覆盖错 run/token、缺 stage42、重复 swap、SOURCE 冒 quantum、GPU query disjoint、截断与缺 clock 关联，应明确被拒或 NOT_MEASURED。任何定量误差源、半窗或观测扰动均附原证据，不降低 W1–W5、守恒或同 token 约束。

T380 已完成计量合同。GPU 真机闭环完成后由 manager 派收益取证，性能失败回 architect 首分流。
