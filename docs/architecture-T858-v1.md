# T858 · Windows PC 异步 SOURCE、归档与资源退役

2026-11-23 · architect · P4 · Windows x64 Vulkan · 离线工程合同

**首个可看预览走现有 M1 干沙窗口。** T853 Release PE `9bdbcf7d…` 的固定 SOURCE 已接受 `131072 Q20`。同一 PE 的交互初始化已完成角表 fence、16 个非空描述符集合及全部更新，并抵达 `M1_POINTER_READY`。该轮在按下前由夹具关闭。旧 T840 的同根绿色诊断帧已复跑。真正的干沙窗口已经有从 QUANTUM publication 读活动粒子和床高的 shader，待 T856 实际按住、取得非空 SOURCE→首个 QUANTUM/REPLAY 与同根业务像素和 Present。T856 成功一轮即可提供首个可见干沙预览，T858 的异步工程、T848 冷档校验、32 轮资源验证和 G13 水线均可并行。稳定 30 FPS、松手后可见后续运动及落床累积按各自真实帧序列登记。 [T853 新 PE 与证据](../test/reports/t853/handoff.md) · [T840 诊断对照](../test/reports/t840/v2-m1-rerun.json) · [干沙 shader](../pc/vulkan/m1_dry_scene.frag)

## 现有接口与接续顺序

| 原件 | PC 接缝与复用边界 |
| --- | --- |
| T830/T853 | Win32 UI 在短锁内记录 `PointerSample`，独立 owner 取 pulse、持有 `ProgressClockV2`，构造实际 `SourceCommand32`，运行 SOURCE/QUANTUM 和 WSI。`acceptPointer` 以 geometry/hold 核验最终接受。当前只有 held 输入接缝及显式单次 `--m1-cold`。缺通用 native 外线程提交、可异步 `Reserve/Read` 的归档入口和逐资源退役台账。 |
| T496/T684/T703 与 T692 | 复用真实外线程输入的锁外交接、52 动作、动作/原命令/接受/真实排空判据、忙点预约、完整日志及独立原字重放。旧 GL C0/C1/C2、短 D2 和 92 根窄对照沿原证据有效域留存。Windows Vulkan 用真实接口和新来源映射。 |
| T504/T500 | 复用同源核验、32 轮矩阵、对象世代、两次 warm 后同检查点比较和原故障/并发读取判据。原静态 10/10 仅覆盖旧四档公式和 host 布局，PC Vulkan 重算实际请求与并存量。 |

Developer 可先按本合同实现接口、边界和 compile；tester 可先移植现有判据，核真实 PE、ABI 和来源。**启动业务 D1/D2 与 32 轮的输入门是 T856 有效业务根原件和新的真实异步入口。** T856 的普通 `--m1-window` 多帧跑法适合拍可见预览。`--m1-cold` 在首个 QUANTUM 后同步读回、写盘并改走单次呈现，单独跑一轮给 T848，保该进程自己的 JSONL 和 Present。 [T830 保持窗口交接](../test/reports/t830/m1-held-handoff.md) · [T496 设施](../test/t496/CASES.md) · [T504 静态结果](test-report-T504-resource-lifecycle-v1.md)

## 外线程提交，原 Vulkan owner 消费

1. 在现有 Win32→owner 边界提供同一个有界 `OfferSource` 接缝。Win32 输入适配器和 tester 的外线程调用该接缝，复用同一权限检查和队列。入参用冻结的窗口/geometry/input 代、来源、实际 hold/sample 序号、原客户区坐标和转换后坐标、UI pulse/sample 原时标。owner 给 caller 的只读快照含 `run, owner, device/context, surface, geometry, config, permission, pageSession`。调用者带回该范围和单调 `offerSeq`。producer 只提交不可变请求或取消，owner 从实际授权和样本构造并上传原 32 LE U32 命令。存 producer 原输入及 SHA、实际构造的 128 B 和 SHA 两份，保两者及同一 SOURCE root 的关联。只有实际上载并接受的 `Completion256` 才是接纳回执。
2. 首版为 4 个固定 offer 槽，满时具名背压。活槽及近期回执内重复同键同字节返回原回执，冲突拒绝。回执窗口沿定容存储，低于最后消费水位的已淘汰序号退为过期，不能再次消费。待消费、消费中和终态按单向状态记录，实验完整记录进入同 sink。`Up/Cancel/捕获丢失/新 geometry/关闭` 经同一短锁更新权限，优先失效旧待消费请求，不排在已满队列后。owner 在真实消费、提交前及尾接受时核范围、hold 和有效 pulse。SOURCE 只使用当前 `PlanGrantV2`。Up 停止持续注沙，已获批 QUANTUM 在其原许可和窗口有效时继续，resize/窗口撤权使该代未接受候选按 fence 退役。
3. UI、调度和归档 worker 只做固定槽读写、通知和 CPU 字节处理。所有 Vulkan 录制、提交、fence、map/invalidate、publication 与 WSI 都由原 owner 执行。拷贝必要快照后先释放协调锁再调真实入口，不从外线程访问 owner 私有 `SourceRootOwner`，不跨 `wait/present` 持 UI 锁。现 `Window::Input`/READY 输出有锁内 `Emit`，接异步入口时移动 I/O 到锁外或既有独立 sink，保原事件与 READY 的线性化顺序和单调收据。
4. 沿用 `ProgressClockV2` 250 ms 根 deadline、33,333,333 ns gap、120 Hz 物理钟和 quota/carry 守恒式。脚本声明时域、Win32/QPC 原 ticks/频率及 owner 实际 pulse 接收/permit 时域分别入档。按原 1024 B 热尾判定真实 `Poll` 的原子接受，采原 event 范围、ACTUAL/REPLAY 与同代发布，输入和采集都不能补 pulse、改 deadline 或伪造 `Ready/Allowed`。 [Win32 主循环](../pc/vulkan/main.cpp) · [SOURCE 实际命令](../entry/src/main/cpp/gpu_resident/g13vulkan_boot/m1_source.inc#L215) · [时钟及配额](../entry/src/main/cpp/gpu_resident/m1/progress_clock_v2.h)

## D1 业务无扰动与原字对照

- 就绪后跑独立 OFF/ON 两进程。两臂同 PE、硬件/driver、三 SPIR-V、布局与 pipeline/cache 配置、种子、动作及 owner gate 目标。沿 T684 固定 48 pulse、2 真实 SOURCE、2 capture control。当前 K192/Kw128 PC dry 采用单独版本的两个干沙 offer 及明确非零、负控输入，旧砂/水双源场景保原动作和未测项。OFF 的 capture control 只有同切点回执，ON 走真实预约。独立 producer 在约定的 owner 阶段/序号到达后从锁外提交，记录实际 busy 点。禁止用 Wake 注入补原外线程正控。复用 240s 资源、120s 动作、30s drain 与 420s 外层交集，父进程在 360s 发结束请求，保剩余清理段。送完的每个 action 必须有实际消费或拒绝终态。退出和计数归零还需真实提交、fence、lease 与许可均已收尾的 drain 收据。
- 主 OFF/ON 的采集只用固定 trace、原 1024 B Completion 及身份/事件边界。两臂均保持既有热路、同观测 sink 和无全 bank/event 下载。必须有真实外线程 SOURCE 正接纳、已接受非空 QUANTUM、完成的 REPLAY 和实际 publication 消费。逐根比较输入、有效质量/账本/carry、结果、原根顺序、首错、尾和 bank/event/发布身份。两臂原时标、原 Completion 和原命令不覆写。跨进程若随机/时钟身份不同，只依逐字段明确的实际 run/nonce/frame/permit/世代映射比对，任何非身份字差异、缺边沿或不可证明时域等价均保差异。无确切同域证据的比较结果只登记到相应已证层，不报逐位 PASS。
- 用另两条同动作的证据重放臂执行 T692 式 owner 私有读取。读取点为实际接受 fence 后、下一写者复用前，按真实 backing/slot/代取 bank、实际 SOURCE 或 QUANTUM arena 的 `[0, eventEnd×256)`、原 Completion，并保 `eventStart/eventEnd/capacity`、实际拷贝范围、map/invalidate/完成返回及读取额外耗时。以根、提交、动作等完整谱系回配各自主臂。Vulkan 使用确切 buffer/range 与 compute→transfer→host 证据，保 fence 和 barrier，读取分支禁补主臂缺失记录。T684 每臂 32 MiB 信用仍分 12/16/4 MiB 用于 API/原字/索引，超限保存前缀、实际分区余额和第一缺项，结果 `INCOMPLETE`。 [原 D1 动作和退役](architecture-T494-external-source-reservation-triage-v1.md) · [T684 预算及双层见证](architecture-T684-v1.md) · [原字接缝](developer-T692-v1.md) · [跨时域锚](architecture-T703-v1.md)

## D2 忙点预约、异步生产与冷档谱系

1. 从独立协调线程以同一范围调用实际 `Reserve`/`Read`/`Cancel`。以真实 SOURCE/QUANTUM 在途、Graphics 已提交或 Present 已交接的明确边沿作为忙点，记录实际未返回调用和 request ID、请求/检测时刻、blocker、owner 阶段及 `safePointSerial`。相同配置的重复预约保同 ID，冲突、存储未备或忙位不足返回具名拒绝。`RESERVED` 起的 ARM 上限沿原 1,000,000,000 ns，到期或撤销后不再 ARM。owner 在本代实际安全点，确认已接受且对应已提交 fence 已退、无待复用 copy 和对应 draw/lease 风险、原件范围/代仍有效、存储 credit 已有后才 `ARMED→BEGIN`。保完整 `END→FROZEN`、或 CANCELLED/TIMED_OUT/FAILED 等请求与归档终态；失败记录首错、最后阶段、已产生部分及清理终态。D2 主对照的 ON 只记录既有热路径原件和固定页。 [原预约实义](../entry/src/main/cpp/gpu_resident/evidence_capture_request.inc) · [忙点及 FROZEN 判据](../test/t496/check_capture.py)
2. 真正全量冷档由独立冷请求触发。预约时只锁选中根的条件，提交 copy 前按实际 acceptedRoot、bank、publication slot/generation、arena 和 submission/fence 核对；`Acquire` 只 pin publication，单独保护 bank/event backing 到 capture copy 安全点。首版在明确冷采或隔离重放中由 owner 在覆盖前提交 copy，copy 结束前保必要 lease，按同队列正确屏障顺序处理后续写者，不能从另一线程 map。已有后续业务写且安全窗未取得时，报具名未采并归还 credit。主 OFF/ON 无附加全量 copy/fence。冷路径的 staging、fence、等待时间和 I/O 单列；独立 writer 只在 CPU 字节不可变且已绑定身份后接手，按旧同目录临时文件、flush、原子落盘语义处理重复路径、取消、写失败和残留。活的 Vulkan 资源及 generation 不能交给 writer。
3. 首版最多一个 GPU staging copy 和一枚当前 capture fence、两个有界 native payload 槽（填充/写入中也计），每槽 payload ≤16 MiB，文件封套另有 72 B。只设一个 writer 和一份 `.pending` 文件。复用同一块受 credit 的不可变载荷支持 `Begin/Page/End`，一个 reader，60 s 到期，仅已封页可读。页含 request/capture、root 和代、PE/模块集、cursor/count、长度/SHA，End/过期撤销读权。已完成字节可作为旧代历史文件，过期 request 或 owner 已撤权的未封候选不可新发有效 FROZEN。外部进程观测到超时只记录未完成项。 [T830 冷档写入](../entry/src/main/cpp/gpu_resident/g13vulkan_boot/m1_quantum.inc#L275) · [T500 payload/快照寿命](architecture-T500-gpu-resident-resource-lifecycle-v1.md)

### T848 reader 修订前置

`M1PCOLD1` 保 8 段原协议、顺序、长度、SHA 和首个真实 SOURCE→QUANTUM 范围。K192/Kw128 最大原 payload 为 11,986,304 B，最大 GPU copy 为 11,985,152 B，分别由 state 29,696 B、publication 31,744 B、46,264 个 Q event 槽和 193 个 SOURCE event 槽复算。版本化 PC 归档的公开异步请求、忙点和页终态记录在 JSONL/sidecar，未获 T848 独立支持的后续任意根归档不套首根文件解释。 [旧封套与交接](../test/reports/t830/m1-interactive-handoff.md) · [T848 原 reader](../test/t848/check.py)

本轮用原 C++ `ProgressClockV2` 离线复核，第一次 pulse 仅建立基线，初次 SOURCE grant 的 QUANTUM 配额为 0。后续 pulse 先积累 `pending`，不改变该 SOURCE 的 `activeRemaining`。`RunM1Quantum` 因此取得下一 grant 并令 `frameSerial+1`，第一 QUANTUM 仍从同一已接受 SOURCE bank 开始，`eventStart` 为 0。沿现有 T848 fixture 构造下一帧/下一 permit、正确 arena 和 hash 的测试件，当前 reader 实际拒 `ROOT_LINEAGE`。尚无 T856 真实冷档可据此回填通过。 [SOURCE 初次领用](../entry/src/main/cpp/gpu_resident/g13vulkan_boot/m1_source.inc#L202) · [QUANTUM 同帧条件](../entry/src/main/cpp/gpu_resident/g13vulkan_boot/m1_quantum.inc#L143)

T848 tester 的最小修订是在现行共享范围、nonce、owner/fence、源及物理提交、bank 转移、发布代和 Present/退役门之内，区分两条真实分支。**同帧**要求同 permit、同 frame/arena，实际 `q.eventStart==s.eventEnd`，QUANTUM event 区前缀逐字等于该 SOURCE events。**下一帧**要求首个 ordinal、同 SOURCE 提交及直接 bank 谱系，`q.frame==s.frame+1`、`q.control==s.control+1`、实际 arena 等于相应 frame 奇偶、`q.eventStart==0`，SOURCE events 仍单独完整保存。两条都检查序号及 `eventEnd<=capacity`、根和段 SHA、真实 owner 接受及同代同根 WSI。原合成同帧 fixture 的非零 SOURCE 当前没有拷入 Q prefix，须修正以配合严格验证。分别测同帧/下一帧正例、无 prefix/错 frame/错 permit/错 arena/旧代负控，再验 T856 同进程真档。该 reader 分流不阻塞普通窗口第一帧。

## T504 的 PC 32 轮迁移和产品退役

首个矩阵只登记可运行的 **Windows Vulkan M1 dry K192/Kw128**，每轮真实 SOURCE、非空 QUANTUM、业务 publication 消费后再关账。故障轮在注入前须已有可核业务正控，并在故障及完整退役后用新代再取真实业务根。固定 1–2 轮热身，3–10、11–18、19–26 各按相同顺序覆盖八个切点。27–32 轮做 A→B→A 缩放、相同尺寸复用、零/非法 extent 与复原、重复 Reserve 和读占用、TTL/旧代拒绝、挂起写清理及最后关闭。该矩阵沿 T500 独立长序列思路，历史两路线×四档另按实有工件覆盖，缺项明确 `NOT_RUN`。 [T500 原矩阵](architecture-T500-gpu-resident-resource-lifecycle-v1.md)

| 八个固定切点 | 注入与判据 |
| --- | --- |
| 未封 compute batch | resize/取消在真正提交前夺权；旧录制无 submit，无后到接受。 |
| compute fence 待回 | 撤权后保旧资源到同代 fence/实际 device-lost 收据，late tail 不进入 owner 接受。 |
| 已接受未 Draw | 旧根保历史，只可作显式 STALE 旧整帧；新代窗口无旧根 CURRENT。 |
| Present 前后 | 分开记撤权与原 owner 的 Present 交接边沿。撤权先取得则取消未提交呈现；已交接的在途 Present 保存图像和原结果，跨代结果只退役。 |
| publication lease 持有 | 同代真实 graphics fence 前 slot 不因 Up/resize 被覆写；强制失去设备记录 LOST 及 quarantine 销账，不能据 `Release` 布尔报成功。 |
| 冷 staging / copy | 超时及撤销不再 ARM。已提交 copy 退役前 staging、选中 backing 和 credit 均受保护。 |
| cache / 创建 I/O | 当前 PE 只有在实际启用 pipeline cache 时才测 Vulkan 缓存读写及故障，记录实际创建参数。无此入口的轮保相应 NOT_IMPLEMENTED，先完成其它资源子项；不拿旧 ANGLE Store 数字冒 PC 通过。 |
| 归档导出 | 慢写、写失败、重复目标和关闭并发。已提交文件不被半档覆盖，按归档 ID 对账挂起临时文件和 writer credit，旧范围 reader 请求拒绝。 |

资源账在每轮 `READY/受理/提交/accept/copy/draw/present/revoke/retire/reopen` 保存 `{run, owner, device/context, surface, geometry, pool/swapchain generation, objectSerial, buffer/range 或 image index, 请求 bytes, allocation bytes, live/pending/retired, actual VkResult}`。按对象及持有者把 compute descriptor/buffer/command pool/fence、Graphics 的 publication consumer/attachment、WSI 的 swapchain image/view、capture staging、native 槽/reader/writer 和临时文件各归同一检查点。实际 graphics fence 后退 GPU 读 lease。现版不传 Present wait semaphore，`vkDeviceWaitIdle` 和销毁调用是应用侧收尾证据，展示引擎何时释放 backing 另记未知。若实际启用 present fence，保同 swapchain/image 的 signal 原件。同图像重获并等待信号只覆盖该 image 的前一次呈现，旧 swapchain 在重建时按 T824 退役。旧代 compute/device 未退前不再建第二套完整业务资源。保窗口 owner 完成边沿到 UI `DestroyWindow` 及下一代创建。`VK_ERROR_DEVICE_LOST`、退役 API 未返回和外部 watchdog kill 各留实际原件，分别登记 LOST / RETIRE_PENDING / 未完成。 [现有 WSI 路径](../pc/vulkan/wsi.cpp#L341) · [T824 退役合同](architecture-T824-v1.md) · [Vulkan Present 资源的寿命](https://docs.vulkan.org/guide/latest/swapchain_semaphore_reuse.html)

过关以同 checkpoint 可解释的 live/retired 和高水位、旧代无新接受或 CURRENT、已知对象在原 fence 后收敛为准。稳定保留的固定快照、有限历史收据和 cache 分账；driver backing 与 OS RSS 只按可观测范围记录。Tester 先复用 T836 动作/代际、T848 冷档、T496 动作/D2/比较和 T504 静态账。T858 本轮复跑了 T836 的 8 个合成检查、T848 原 fixture 的 10 项拒绝、T496 的 7 项与 T504 10/10 静态账，另核 T853 PE/三 SPIR-V/三份 JSONL SHA。业务 D1/D2、全量冷档和长序列待真实运行。全程 GPU 驻留和 shader 渲染，禁止 Canvas。 [T836 夹具](../test/t836/README.md) · [T848 判据](../test/t848/check.py) · [T504 静态复算](../test/t504/static_ledger.py)
