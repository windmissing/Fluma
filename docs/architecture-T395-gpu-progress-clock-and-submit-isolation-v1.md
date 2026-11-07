# T395 · GPU 停顿迁移与提交时钟裁定

2026-11-07，architect，P5。不使用真机。此稿接 T387/T390。现有产品协议继续执行 T324/T328。下面的独立进度时钟是须显式裁决的隔离版本。

## 1. 从 T318 到 T390 的证据

| 原档 | 超窗落点 | 已证明的机制 |
|---|---|---|
| T318 → T328 | 续批之间约 8–9.9ms，批本体约 0.10–0.23ms | 每批一次 Pump 和固定等待相乘。改 Pump 内多批后，T330 在单 Pump 到末批。 |
| T330 → T337 | T330 末批 fence 创建 37.65/41.28ms；分批 flush 后 T337 首批尾约 33–35ms | 改等待位置能推进到不同阶段。提前 flush 可以改变停顿落点。 |
| T376 → T384 探针续测 | 预热后批1短、批2尾长。T384 直接测到非末批 `glFlush` 墙时 62.079/38.493ms，撤销时 accepted=0 | 两笔长段位于 GL 调用开始至返回之间。没有线程调度轨迹可拆 CPU 执行、驱动等待、GPU 背压和脱调度。 |
| T390 | B 策略有一轮批2尾 11.514ms，但同 root 批7尾 57.913ms；另一轮批2尾仅 0.046ms，批3尾 31.829ms | 两批冷准备改变部分热批墙时，仍无法保证首个 root 走完。全热档没有 stage43–51，T390 各笔只能归为批尾墙时，逐笔 `glFlush` 或 staging 耗时未测。 |

T390 复核 42 个 SHA 前缀、六份导出文件的 manifest SHA。去累计快照重叠，取 B3 的 1006 行完整前缀，按 `(epoch,run,surface,TID)` 和 stage52 的 carry 认策略。原报告及评论对六轮、A/B 数目和导出顺序的叙述互有冲突。原档实际有 **A×5、B×3** 共 8 个新 run，全为同进程 pid 44515、同 owner TID 44926，末六轮 e4–e9 交替 A/B。A1 manifest 实为 `INCOMPLETE/GREW_DURING_EXPORT`（声明 226 行、归档内有 227 行），其行后续被 B3 完整覆盖。B3 中八个 run 均有 Ready、实测 GAP 和 `Cancel(code=1)`，均无实际 fence/Poll、Draw/Swap、Offer/Consume；六份 statusAfter 采样 accepted=0。旧 stage23 只是桥层 active/Pump 标记。触摸操作无 native 源准入证据，原档无双源 token，K5 按各 manifest 分开登记。

| epoch/run | 策略 | 42 冷 wall ms | 热批最长尾 ms | anchor→GAP ms |
|---|---|---:|---|---:|
| 2/1 | A | 26.377 | 批2 44.315 | 46.689 |
| 3/2 | A | 33.305 | 批2 46.990 | 49.903 |
| 4/3 | A | 31.220 | 批2 48.990 | 50.809 |
| 5/4 | B | 90.655 | 批7 57.913 | 93.470 |
| 6/5 | A | 26.619 | 批2 44.893 | 46.246 |
| 7/6 | B | 77.300 | 批2 35.546，整批 44.359（只发 1 op） | 54.181 |
| 8/7 | A | 20.960 | 批2 29.154 | 38.342 |
| 9/8 | B | 65.711 | 批3 31.829 | 39.350 |

B 三轮冷链全完成，K192/Kw128、冷角度 0、cleanup 11,843,584B，42 在 Ready/首 anchor 前，属于冷启动成本。e9/r8 的批3末 op 已离 anchor 7.502ms，31.829ms 叠加后就越窗；e5/r4 批7末 op 时已离 anchor 35.538ms，说明单 Pump 的 8ms guard 不能替代总 deadline，当前实现还继续发了后续批。A/B 同进程顺序、热真实角度/空闲和源准入无完整配对，不能从某一对批2计算推广收益，也不能把自发 GAP 当注入的独立负控。支持 W4 产品 FAIL，T380 收益继续缺前置。

## 2. 根因与裁定

现行 `app_bridge.cpp` 在成功 `Advance` 时更新 anchor，活跃 root 期间继续用它判 33,333,333ns。anchor→Pump/驱动调用→Poll/map/audit→CPU 接受，共用前台循环的时间窗。更关键的是 `PumpControlled` 持 app mutex 执行整段 `Run::Pump`，阻塞 GL 调用期间，输入、状态读取和撤权也等这把锁。guard 在调用返回后检查，无法抢占 GL。当前 source/quantum 的 `RootOwner::Poll` 会在 `Run::Pump` 内真正改变 bank、accepted 和 publication，桥的 post-Pump GAP 检查发生在它之后。T390 未到 Poll；这是源码暴露的下一处竞态，需在后续首个成功 Poll 前补齐。EGL Swap 仍可产生独立长尾。

GLES 的 flush 和 fence 契约描述命令提交与完成语义，未提供本项目 8/33ms 的主机墙钟上界。越窗可能在多处复现。架构上须把 **前台控制连续性、GPU root 的排队/执行期限、成功物理时间** 分开计量和治理。三者共用 monotonic 时基以便归因，控制路径无需等待 GL。降低 GPU 尾延迟也需继续，否则产品即使偶尔接受也达不到实时吞吐。[Khronos OpenGL ES 3.2 规范 §2.3、§4.1](https://registry.khronos.org/OpenGL/specs/es/3.2/es_spec_3.2.pdf)

| 路线 | 决策 |
|---|---|
| 继续扩大冷准备或回退到末批一次 flush | 停止作为主路线。B 已有实测反例，T330 又证明停顿会在 fence 暴露。保留 T375 作为对照。 |
| 缩小或重排提交组、按剩余时间调度 | 保留为成本优化。先取真实分段，优先检查多余 staging flush、连续命令队列压力和 pass 数；在守恒与逐位 oracle 约束下，用同包单变量验证。调小批次可控制已发工作量，无法约束单个调用返回。 |
| GL owner 与控制隔离，显式分开进度合同 | **主路线**。先落可独立实现的 V1 加固，保持 33ms 产品语义。若要让前台健康时的 38–62ms root 还有机会正常接受，须按下节批准独立 V2，真实 GAP 仍撤权。 |
| 专用 compute/render 上下文或换显式提交后端 | 首轮不引入上下文迁移。若实测 GPU worker 或 Swap 尾停顿仍压低可见帧率，再测独立呈现通道的共享资源/fence、窗口代次与平台能力。显式后端能改进命令编排，仍要实测 host submit、资源驻留和呈现，不能用 API 名称承诺上界。[Khronos Vulkan 队列规范](https://docs.vulkan.org/spec/latest/chapters/devsandqueues.html) |

## 3. 先交付可实施的 V1

1. 保留现有 GL owner 线程、context 和 pass 序。把 `PumpControlled` 分成短控制段、无 app mutex 的 GL 段和短提交段。控制侧固定容量 mailbox，原入口接收前后台/窗口/输入撤销，冻结 `run,owner,surface,geometry,input generation,permission revision,root nonce,absolute deadline` 给 worker。撤销短锁内先使许可失效、撤队并可立即读取状态；阻塞的驱动调用由原 owner 回来后收尾。timeline 写入使用短临界区或现有定容 writer，禁止为记录 GL 重新把大锁套回调用。保留源队列上限和 offer/准入区别。
2. 保留原 anchor、真实 33,333,333ns 判据和 8ms 护栏。首次 batch、每个续批、fence、Poll、映射与 swap 的调用前后记录剩余预算及原 wall。跨过截止点停止新发工作；批内有安全检查点时尽早结束。事件驱动续批与有限 Poll 唤醒，保公平和原批顺序，去掉有可运行 continuation 时机械等待 8ms。没有可执行工作时仍等待；GPU BUSY 不自旋。任何单调用卡死时只承诺控制撤权，GL 资源等原 owner/设备确认安全后释放。
3. 拆 source 和 quantum 的完成路径。fence/1KiB map/原完成审核产出未接受结果。短提交段重新校验 epoch/permission 与真实截止点，原 `RootOwner` 接受、bank 切换、source ACK/physicsCommit、event prefix 和 publication 一起线性化。超窗/撤销结果只排空，不能更新接受计数、bank、source 账或 token。未封尾与有 fence 分开清理，已入队 GL 命令无法回滚，其写目标继续隔离。旧 LastGood 只作标明 STALE 的完整帧。Swap 调用前后都验证授权；已经进入驱动的交换不可撤回，迟到结果不得发布 CURRENT 或计正常呈现，单列 pending swap 竞态。前后景只走 XComponent+shader。

## 4. V2 时钟须先裁决的语义

若 V1 在代表设备上仍因真实 33ms root 截止反复零进展，建议批准隔离 V2 来消除耦合。保留 V1 作为对照。V2 唯一允许 GPU root 跨 33ms 的场景是 **独立控制证据表明前台连续、权限未撤销，并且 root 尚在显式进度期限内**。控制生产者与监督器须独立于 GL mutex 和 GL owner。以真实页面回调/前台生命周期建立控制心跳，验证单调序号、原时刻和接收延迟。真实控制断档 >33,333,333ns 仍 `CLOCK_GAP`、撤代、清队列、拒晚到完成。后台、窗口失效、输入过期按原语义即时撤权。定时器只检测断档，不能替缺失控制心跳续命。

每个 plan 用连续控制的原始纳秒形成固定 120Hz 配额，保持整数 carry 与每 plan ≤4 个量子，至多一个在途 root。成功物理时间只随接受的 quantum 增长，source-only、BUSY、取消均不增长。实验有界积欠最多 4 个尚未发出的量子，超出的整量子显式记 `overloadDroppedQuanta` 和原 wall；carry 按真实纳秒更新，切代按原始失效规则清空并记丢弃。逐窗对账 `生成=成功+在途/计划+待发+过载丢弃+撤销丢弃`，不重用旧输入或倍速补跑。需要维持严格墙钟追赶的产品若不允许丢配额，就在积欠超限时显式停为 `PROGRESS_OVERLOAD`，由 manager/designer 先裁定该取舍。

GPU root 用提交意图起算的独立绝对期限，提交批、Poll 或继续 BUSY 都不续期。仅隔离实验可预注册 250ms 上限，用来区分可恢复的 38–62ms 阻塞和无进展；超过则 `GPU_PROGRESS_TIMEOUT`、撤销，未授权写入任何产品上限。控制发生 GAP 时即使 GPU 期限仍有效也拒绝。有限迟到时保活的是控制和真实完成机会，墙钟延迟、队列年龄、丢量子和 STALE 占比原样对外。若产品要求硬性 33ms root 完成，V2 不满足，需缩减 GPU 工作量或对该设备/配置明确能力限制。

一次在途、每个 quantum 一个 root 的现行拓扑要持续跟上 120Hz，含 source 竞争的平均服务间隔必须压到约 8.33ms/quantum。若每个 root 都要 40–60ms，理想上限也仅约 17–25 quantum/s，V2 只能保正确退化。实机先做调用点墙时、对应线程调度和完成进度分账，再用同包比较 staging flush 策略、每段命令量、通过原 oracle 的 dispatch 缩减。对已转移到 CPU 等待、GPU 队列或独立 Swap 的尾部各自给收敛证据。发布门槛应同时使用成功模拟秒/墙钟秒和唯一新 CURRENT 帧/墙钟秒，保留退化率、过载和尾延迟。

## 5. 交接与复测

- developer 先做 §3 隔离包。离线注入 5/40/65ms 与持续不返回的提交替身，核状态/撤权及时且无 GL 大锁。注入刚好跨 33ms 的 stage、Poll 与 map、校验与提交竞态、Swap 前后换代、fence 迟到、满队列、旧回调与失败重建。合法在窗 root 位型、nonce、carry、计费、原账、lease/事件与 published token 同旧合同。旧档和 4096 冷导出继续可读。通过编译、自测后交签包。
- tester 以新包分别采探针开/关，固定设备、K/Kw、真实接受角/空闲、进程冷/热及 offered 脚本，按独立 run 冻结样本和超时。记录 app 输入延迟和帧回执、staging/dispatch/flush/fence/Poll/map/audit/swap 原值、same token 与取消，能取匹配 TID 的调度则分项报告。要有真正前台断档负控、真实双源准入、至少一次成功 quantum→同 token Draw/Swap；K5 每窗原档完整。设备达成稳定吞吐和可见帧门槛后才按 T380 做收益比较，首个 accepted 只证明功能链。
- manager/designer 裁定 §4 的进度语义与产品阈值后，才派隔离 V2 和对应真机。V2 验证须同时覆盖健康心跳+慢 GPU 可接受、真实 GAP+慢 GPU 迟到拒绝、GPU 超时、配额积欠和长时间正常运行。任何方案若在真实设备不能稳定交付，都按原诊断回流，禁止用扩大预热、重置 anchor 或去掉慢样本记通过。
