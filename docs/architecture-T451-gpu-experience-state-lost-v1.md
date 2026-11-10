# T451 · GPU 显式入口 STATE_LOST 首次归因与冷启动取证

2026-11-10，architect，P5。不使用真机。依据 T448 原卡、17 件归档、T438 双签包及 T421 对照，冻结下一版诊断合同。 [原件索引、SHA 与源码比对](analysis-T451-gpu-experience-state-lost-evidence-v1.json)

## 裁定

**regular 包至少三次已经走过 EGL 与 transport 创建及 V1 READY 提交。regular run 3 最早可证的阻断是 READY 之后的 V1 CLOCK_GAP。** 原件无法分辨该次 GAP 的具体触发点，也无法判定 experiment 的 run 0:0 停在窗口交接、创建中、创建失败后被覆盖，还是 READY 提交被旧代门拒绝。两包都未观察到有效内容帧。工程上已确定的缺口是状态缺少换代原因，首个失败切点无法跨换代回读。先交 developer 补最小冷探针，现场按证据分流。 [T448 报告](test-report-T448-device-experience-blocked-v1.md)

| 原件 | 可确定事实 | 仍缺的证据 |
|---|---|---|
| `l0` | 普通菜单存在显式入口 | 无 GPU 运行证据 |
| `l1/l2` | regular，实际 V1，STATE_LOST / run 0:0 | 无窗口/epoch、owner 建立或 READY 回执 |
| `l3/l5/l6/l7` | regular，实际 V1，STATE_LOST / run 0:2、0:3、0:4。l5/l6 原字节相同 | 显示的是最近一次已公开的 run。没有采样 epoch、readyEpoch 或换代原因，无法归属到正在创建的那一代 |
| `l4/s4` | regular V1，PAUSED_CLOCK CLOCK_GAP / run 0:3，UI 源账全零、Surface 区域黑 | V1 控制时间或首 root 调用的细切点和原始时戳。空账不能证明从未有提交意图 |
| `l8/l9/l10` | experiment profile 也显示**实际 V1**、STATE_LOST / run 0:0。l8/l9 原字节相同 | 未归档实际 V2 新 run，不能把两个 build profile 当 V1/V2 运行对照 |
| 全部体验布局 | XComponent 区域均为 `[0,688][1280,2458]`，传感器输入行在场 | ArkUI 布局与 onLoad 属页面层，不能单独证明 native window 引用、EGLSurface、viewport、lease 或已呈现 |

报告记载 regular 四轮与 experiment 两次尝试，布局能复核上述采样，无法逐轮补齐唯一首错。报告引述两个进程的队列创建/销毁、`EglWrapperHookLayer init Failed!` 与 `munmap ... fail`。归档和同名本地副本中没有原始 hilog 流可关联 PID/TID、epoch 和原始 EGL 返回值。销毁队列也可发生在主动 reset 或 surface 换代，系统日志本身不足以判为因果。T448 若仍有原流，补交原文件、SHA、时钟域、进程、实际安装回执和各操作定位。缺失就保留缺证，使用新冷探针取代猜测。 [T448 原件目录](../test/reports/t448)

## 实际入口与状态链

T438 regular/experiment 最终包 SHA 逐位吻合，arm64 `libgrsm1.so` 分别为 c61a2a76…8d、314562c9…bf。T421 为 a55a9281…df，三者是不同二进制。对照本地生成源与 `0228a7d`，T438 的 `bridge_state.h`、transport/scene 和旧 `GpuResidentM1` 匹配。T438 app bridge 的源码增量为 `experienceCapabilities` 查询和注册，构建增量为独立 profile 宏，Ability 普通启动落到 Index。由菜单进入 `GpuSandWaterExperience`，首次 show 读 capability、冷选默认 V1、`model.reset`，同步 active/session 并创建 `grs-m1-experience` SURFACE。T421 使用 `grsm1=true` 直达旧诊断页 `GpuResidentM1`。两页调用相同 native XComponent 回调和 GL owner 路径，page/reset/窗口事件次序是待核差异。T447 验证的是接线和 native 桩，T438 软件 GL host 把窗口面换成 ANGLE pbuffer 并注入时钟，二者均无现场 Surface 时序证据。 [T438 构建与入口](../test/t438/README.md) · [T447 差分报告](test-report-T447-experience-diff-v1.md)

`app_bridge.cpp` 的 `created/changed/destroyed` 和 `newRun` 都会走 `invalidate`。 `BridgeState::InvalidateLocked` 增 epoch、撤销 READY 和清未发队列，将状态设为 STATE_LOST，同时保留旧 identity。 `App::loop` 才在单独 owner 线程按 active/window/reset gate 获取自己的 window 引用，创建 EGL display/config/context/window surface、makeCurrent，核 viewport、创建 SOURCE/QUANTUM transport 与 cold warmup。V1 仅在上述成功且 `CurrentLocked(seen,held)` 为真后通过 `CommitReadyLocked` 写入公开 identity。内部 `++runId` 发生得更早，页面只读公开 identity。

因此 regular 布局中的 0:2、0:3、0:4 各表示对应 run 至少成功提交过 READY。之后显示 STATE_LOST，只能确定发生过换代并可能带着旧公开身份。run 0:3 的 V1 CLOCK_GAP 来自 native anchor/loop 门，该门与 V2 真实页面 pulse 的双域门有别。V1 `PagePulseLocked` 返回的对外接受标签也是兼容投影。不要用这个状态推断 touch 导致 GAP、真实 V2 pulse 连续、某一批 flush 超时，或由 EGL init 首错导致该 run 失败。 [V1/V2 控制合同](architecture-T401-gpu-progress-clock-v2-v1.md)

目前 `status()` 有当前 epoch、readyEpoch、旧公开 run 和少量阶段字段，体验页只投影通用 state、stage 和 run。 `created` 与 `changed` 都执行 replacement，即使传入 window 相同。一次回调可触发重建，也可能是合法 viewport 变化，当前没有带原因和尺寸的可归属回执。 `publish` 对过期 epoch 丢弃旧结果，GL 串行初始化合并成一个 EGL_CREATE，transport 失败常折叠为 TRANSPORT_CREATE，下一次失效可以覆盖当时 UI 状态。 `changed`、生命周期、首 root 阻塞及初始化失败都有明确探针，尚未取得足够现场证据选择相应行为修复。

T421/T410 的三份 FROZEN 回执验证同一设备族上独立诊断路由的真实预约与 owner 安全点。其初始权限均为 PAUSED_CLOCK，最早原因依次为 CLOCK_GAP、WAIT_PAGE_PULSE、GPU_PROGRESS_TIMEOUT，lastCompleted 均为0，不能用作健康 GPU 渲染对照。T448 的实际显示 profile 为 **K192/Kw128**。按 T433 核算，steady isolation compute buffer payload **37,728,720B**，单 QUANTUM arena **11,843,584B**。约85MB 对应 K512/Kw128。1280×1770 仅为布局尺寸，若 native viewport 同尺寸，两张 RGBA8 色目标约18,124,800B，且在首 Draw 阶段分配。必须分别归档实际分配、GL 错和生命周期，不能从黑屏或布局推算设备内存失败。 [T433 资源账](architecture-T433-gpu-resident-migration-inventory-v1.md) · [T410 三档报告](test-report-T410-device-v2-progress-v1.md)

## developer 最小冷探针

不使用真机。以 T438 双 profile 冻结生成源新增诊断 revision `T451_GPU_BOOT_PROBE_V1`。保留普通菜单入口及同包 `grsm1=true` 旧入口供配对，沿同一生产 worker 路径。输出小型独立冷收据，不依赖 T421 安全点已 ARMED。禁用探针时保持原调度和业务语义。合并公共 `app_bridge` 或 Ability 接线需单一集成 owner，避免覆盖 T450 采集入口和 T452 独立 M2。用独立生成目录签包。

1. **关联键与首错保留。** 每次冷启动/重建尝试有不可复用的 bootSerial，绑定 build profile、实际时钟 revision、PID、owner TID、目标 epoch、surface generation、window 代号与 native 尺寸、候选 run、已公开 run/readyEpoch、session 和配置。指针可使用进程内代号。记录每次 UI reset、profile/optics reset、surface CREATED/CHANGED/DESTROYED、active/page session 的原始来源与变更前后值。第一次失败或撤权不可被后续 STATE_LOST 覆盖，带原因的旧代失败只归旧 attempt，不污染新 run。提供 latest 与有限前几次冻结收据，输出 dropped 和完整性。探针导出不得依赖物理接受或当前窗口可用。
2. **获取与创建。** `init` 记录 XComponent unwrap/register 结果，回调侧记录 surface generation、引用结果及尺寸查询。owner 侧记录是否拿到引用，分别报告 `WAIT_WINDOW`、`WAIT_ACTIVE`、`RESET_REQUIRED`、引用失败、epoch 变化。在真正执行的调用分别记 enter/exit 与返回值。EGL 顺序为 getDisplay、initialize、bindAPI、chooseConfig及数量/选择属性、createContext、createWindowSurface、makeCurrent、querySurface。任一步失败立即锁存该线程的原始错误再清理，不能让另一个 EGL 调用覆盖首错。过期或空窗口进入明确终态。记录 makeCurrent 后实际 context/window surface/thread 和 viewport，能力值以真实 context 为准。
3. **创建与分配。** 最小分段为 SOURCE/QUANTUM capability、shader compile/link、buffer 分配、PrepareProgram、warmup begin/end、READY commit。分配以对象类、请求/已知累计 payload、GL 错和失败对象记录。保源/量子 event arena、scratch 和后续 scene FBO 不同阶段。shader 编译失败才抓有界原 log。READY 失败时区分 epoch/active/window，实验 V2 另报真实 pulse 或传感器 gate，保留原权限规则。
4. **首个运行切点。** READY 后只记录第一轮可用时间、首 SOURCE/QUANTUM 意图和第一个 QUANTUM 的 stage/batch、staging flush、批 flush、fence、poll/map/accept、第一组 Draw/Swap。保留首个失败/撤权的 stage、nonce、原始控制 anchor、UI frameNs、native receiveNs、实际 gap、期限和 elapsed。V1 标 anchor GAP 域，V2 分 UI callback/native 接收/250ms job 域。driver 调用未返回时，可读最近 enter，无假造 exit 或完成时刻。超时仍沿既有撤权，资源由原 owner 安全退休。
5. **采样开销边界。** 用固定容量的原始值，复用已有 bridge/submit hooks。锁只保护快照和因果序号，GL、格式化、磁盘导出留锁外。首 root 的当前阶段由短小单写者记录，禁止高频逐帧输出 hilog 或在 UI 轮询诊断。终态/退出后才导出原字节和 SHA，采集中避免 dumpLayout/导出扰动。探针只观察，不改额度、调度、首错判据或许可。

开发验证至少覆盖原生成页和生产 native 桥的冷诊断接线。用可控桩分别覆盖无窗口、引用失败、EGL 单切点失败及同线程首错、capability/分配失败、create 未返回、Create 成功后 READY 因换代拒绝、READY 后 V1 GAP、真实 V2 wait/GAP 与 GPU timeout 的分账、晚失败不覆新代、首次 Draw 或 Swap 失败。另核正常同源离线 GL 和原预约/接受无回归。任何失败记录均应能离线只读导出，保正常零 CPU 池搬运和原条款。交编译、签名验签、版本化 schema、非机结果和可复跑说明。设备结果另由 tester/effector 取证。

## 现场续测与后续分流

真机卡串行，签包到件后先执行一次普通菜单 V1 与同包直接 Want V1，匹配 K192/Kw128、朝向、冷启动、输入和实际 viewport。viewport 无法匹配时如实分列，不能把差异全归入口。无触摸先取 owner 建立到首个 QUANTUM 的有界窗口，接一次显式新空场，另做背景恢复或真实尺寸改变负控。之后只在 experiment 冷选实际 V2 并核对 activeRevision，重复出现的阻断按 firstFailure 分开归档。保持新跑与旧跑的 PID/epoch 独立，核新探针的完整性与 SHA，若同时取原 T409 格式才交 T443 checker。T448 可沿原卡在有效新内容出现后继续沙水与体验，避免重复当前四轮无原始切点的采样。

| 现场首证 | 后续处理 |
|---|---|
| owner 未取得窗口/active，或 READY 时 epoch 已变 | 修入口绑定与 surface 代际。仅在实证同代、同有效 viewport 且无重建必要时去重 CHANGED，保真正换代的撤权、资源退休和新 run |
| 某个 EGL、capability 或 alloc 返回明确错误 | 按已证调用及实际资源 profile 修订工程合同，保原始错误与 unsupported 语义。保留设备能力不足这一可判终态 |
| READY 后首 root 的 V1 anchor GAP、V2真实 pulse GAP 或 GPU 期限 | 分开交控制/提交工程，结合原时钟与同 owner 首个未返回段。33,333,333ns、250ms 与 120Hz 物理不改，30fps 独立验 |
| 首接受已有，场景或 Swap 首错 | 按 lease、viewport、token、FBO 和原 GL/EGL 错定位，回放有效接受前缀。仍沿 XComponent+shader |

保持 GPU 与新渲染路径，失败报告 FAULT 或完整旧有效帧标 STALE，不启用软件 GL、Canvas 或 CPU 热计算替代。执行者完成当前范围和角色必要检查，局部受阻先完成其余，失败回原卡写未完成项、原错、证据、已尝试和真实前置。不允许修改后续工作清单或滴答同名清单，本人行状态有限例外。

本轮已完成原卡和原件核对、三个实际签包与冻结源码对照、regular 可证阶段纠偏及最小冷探针工程稿。确切现场第一条失败调用仍缺绑定身份的 native 首错原件，不能从已存布局或离线软件 GL 补造。前置为 developer 冷探针签包和 tester 同包实机取证，结果回 architect 继续首次归因。
