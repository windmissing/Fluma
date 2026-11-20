# T727 · 目标20 Shader 编译优化与对照方案

2026-11-20，architect，P5。本卡不使用真机。首批实施选择 **G13 的独立 Boot 与首个完整时间步**。以 GLES 在 Maleoon 上的真实产品入口交第一步，复用已有缓存和取证。M1/M2 继续消费 T696/T717，RX/DM 复用经 G13 验证的生成办法。保留 Vulkan 小原型入口，正式双后端采用同一计算与发布合同。 [量测合同](architecture-T727-measurement-v1.json)

## 已有原件和可复用基线

| 路线 | 当前工件和实际源码 | 已观察到的边界与剩余缺证 |
| --- | --- | --- |
| **G13** | T654-g13c 签包 d92ee0fb…，包内 BuildId 1af61bbf…，GLSL **259,025B / 7563c614…**。T720 正式开发收据的 G13 unsigned fadf926a… / BuildId 428dea61… 含同一源码，并修 UI 同步等待。工作区另有 T725-g13 签包 dc1294df… / BuildId 03503fcc…，离线核到同源，仅作为候选，当前缺完整签装和运行收据。 | 原 t654c2 的 ticket 172 在 `PROGRAM_BUILD / SHADER_COMPILE ENTER`，报告等待超过 90 秒后离开页面。缺同进程完整加载、真实编译退出和系统级退出原因，阈值和首帧未观察。以实际可部署的同一 T720 后继父版做新单程序与拆分配对。 |
| **RX** | T719-rx e6a34091… / BuildId 0f591129…；T720-rx 3c6af26b… / BuildId 27c55879…，两版实际 GLSL **146,301B / 2cb9a848…**。 | T719 原 PID 23170 的 mapped BuildId 匹配，ENQUEUED 到 SHADER_COMPILE ENTER 约 110.027ms，超过 240 秒未见退出。T720 v7 记录超过 900 秒仍在冷编译，原采样混合前台与后台。已见缓存预载和取消门生效，本源码没有 PERSISTED、BINARY_LOAD 或首帧。保已有产物和原件。 |
| **DM** | T718-dm cb636553…，配套 lib BuildId 9954e008…，包内 GLSL **146,316B / c10b15ee…**。独立 g9_dm_differential 与包内私有库分别绑定。 | 三个进程曾同时运行，CPU 累计时间增长，CREATE JSONL 为 0B；实际加载同票身份和原编译/链接游标不足。现有采样支持程序仍在消耗 CPU，完成进度、确切单 API、同等条件的独立时长未知。后续仅用该计算入口作算力对照；要测首帧须真实页面与 XComponent。 |
| **M1** | T714 THREE_OFF e7974456… / BuildId e369d8b0…，包内 SOURCE **28,567B / 47c0076c…**、FINAL_Q **52,892B / 5d4e983f…**，逐角色源码与 T701 表匹配。 | T714 CLOCK_V2 已见物理 0:1 和 STALE_PAGE_SESSION，未留同 scope 首 Swap、加载 ELF 或逐角色 API；先用 T717 修订工件核真实 session 和首帧，不能把状态归为长编译。 |
| **M2** | T714 TWO_ON 98065974… / BuildId 78dcff92…，SOURCE **85,839B / b7d0a027…**、FINAL_Q **54,857B / 0ef4f045…**，与 T705 对应角色相符。 | T705 SOURCE 链接开放 32.233489688 秒，T711 THREE_OFF 另一次 65.411456094 秒。T714 TWO_ON 的 SOURCE 复合 LINK 成功返回 **72.165237031 秒**，中间 Q 未使用，随后 FINAL_Q 的 `glCompileShader` 开放至少 **10.323131562 秒**（由游标换算；T717 报阶段时长 10.323132969 秒）。该档未封口，加载身份及缓存命中仍待原件。 |

以上在工作区按签包 SHA 解包核对了列出的 ELF 与嵌入源码。包内 BuildId 和设备同进程实际加载身份分列。RX、DM、G13 到达相近阶段，各自源码、驱动返回和输入独立。源码长度与 CPU 时间均不用于估算剩余时间。工作区 G13 候选摘要中的数小时估计及等待 RX 缓存落盘属于现场安排，目标20的实际依赖是同父工件和真实可用输入。真机照旧串行。 [T720 编译收据](../test/reports/t720/compile.json) [T717 首帧归因](developer-T717-v1.md) [T705 链接原件](analysis-T702-T705-evidence-v1.json) [T711 真机报告](test-report-T711-v2.md)

T475 已有程序持久化与负控。T491 在 Maleoon 910 上实际记录 M1/M2 冷六角色 PERSISTED、新进程暖六角色加载且源码编译次数为 0、暖 READY。该设备的首根期限仍挡住上屏。这份资料作为缓存机制和收据格式对照，935F 及各实际路线只用自身环境的缓存和首帧证据。 [T475 缓存工程](developer-T475-program-cache.md) [T491 设备报告](test-report-T491-scene-cache-device-v1.md)

## 首批方案与代价

1. **依赖拆分是冷启主方案。** 现有 G13 `tools/g13/generate_assets.py` 汇入 layout、math 与共享 GLSL，写出单份 assembled source，`g13flow/transport.cpp` 用 programcache::Build(13, …) 后才反射与初始化。生成链应从受审查的函数/入口清单输出真实角色闭包与 opcode→program 映射。先发只含 64/65/66 的 Boot。保 66 原来直接执行 FP32 探针的路径，也保 64/65 前的 lane、控制字与同步 prologue、原双轮资格和 init fence。每份生成源保自身实际 bytes/SHA、源闭包、必要/可优化 uniform 表及 SSBO 读写边。沿原 T607 dispatch 表为首个完整时间步引入资源/根推进及必要的 PACK/AUDIT，Boot 以外先保完整原算法程序作为对照。若完整推进仍主导首帧，再只按真实调用闭包拆源事务或根阶段。推进里只有测到的关键依赖才进一步拆 advection、viscosity、pressure/projection。阶段间仍用原 buffer 和真实顺序。代价是程序切换、反射、cache 入盘、程序与驱动内存峰值，全部计入总成本。单纯删几个 `uOp` 条件或把整个源码分成多个入口名，无法证明编译输入已缩小。
2. **删临时 Q 在 M1/M2 沿 T696。** T701/T714 提供三/两程序臂，后者实际省掉中间 Q 的构建。只在同一 M1 或 M2 父版比较，固定 SOURCE/FINAL 字节与缓存策略。保资源分配、角表、原反射、warmup 与独立 Q 自用 shader，记录节约是否反映在首帧。现有 M2 已实际遇到 SOURCE 链接和 FINAL_Q 编译，删中间 Q 无法移除这两个调用。T717 M1 的会话与恢复结果独立登记，无需重建一份旧 T701。
3. **T475 持久化是暖启主方案。** 复用实际工厂及 filesDir，按角色、精确生成源、layout/绑定 ABI、link 配置、GL/EGL 环境和格式绑定。角色名或源文件路径单独不足以命中。复用现有写盘线程、首轮 PERSISTED 收据、反射/准备成功后的资格以及一次同 GLSL 回落；区分预载完成、查到字节、真实驱动加载合格、提取完成和实际持久化。部分命中报每个角色。现有限制 32MiB/条、128MiB payload、32 条仍生效，拆分后独立核临时载入、提取、写盘和驱逐开销。
4. **调度保障可控性。** 复用 T720 让状态、撤权及取消在 GL owner 长调用期间仍可读；真实派发的 driver 调用自然返回后由原 owner 清理，再断言新代许可。UI 给阶段和已等待时间，观察端留单次 ENTER/RETURNED/EXIT。异步搬移、预编译或延后初始化须报告从配置请求到首帧的总成本及提前启动的编译/I/O 时间。没有真实调用返回时保截尾，避免排重复尝试或把监控拉长当修复。

### 不改变算法及后端边界

保留原算法、精度/舍入、边界与输入、时间步/迭代、协议/索引、资源范围、已验收数值阈值、审计和提交。整数按原逐字要求，浮点沿原 G13 合同与 oracle，不能借跨后端放宽门。GPU 写入和完成 fence 之前的结果不得公布。Buffer 布局和 descriptor/uniform 责任由每角色清单核对，反射和 binding 仍在真实程序上执行。退役、错代、超期、撤权和迟到完成保真实首错以及上一有效帧 STALE。 [G13 原工程合同](architecture-T607-g13-engineering-v1.md) [T721 原判据](test-report-T721-v1.md) [用户重构交付约束](developer-T720-v1.md#2026-11-20--大-shader-重构的交付约束)

共用角色、op 顺序、数学、资源布局、输入与输出判据。后端单独实现程序/管线创建、绑定、依赖同步、提交和显示；GLES 的 GL shader/program 与 Vulkan 的 SPIR-V/pipeline、环境资格和缓存各自绑定。Vulkan 对照走同一真实 XComponent 原生窗口，先核实际设备驱动、计算能力、surface/swapchain、同步、精度及放行，测 shader module、compute/graphics pipeline 创建、首呈现与同输入结果。离线 SPIR-V 后仍测设备 pipeline 创建。保两路冷启动选择；更换后端只在安全退役后的新 run 或新进程，清旧代资源和呈现 token，重建缓存身份。同一设备、相同用例的 Vulkan 原型通过原数值、冷启和持续有效 30 FPS 验证后再扩大。 [Khronos OHOS Surface](https://docs.vulkan.org/refpages/latest/refpages/source/VK_OHOS_surface.html) [Khronos 管线规范](https://docs.vulkan.org/spec/latest/chapters/pipelines.html)

## 可执行对照与可见交付

**G13 最短序列。** 由 developer 在同一父版生成单程序与拆分臂及 role/BuildId 回执，沿现有编译路径交付。旧父版使用符合身份的可用收据，无需重复 T701/T711/T720 开发工作。tester 离线接现有 T721 和四页解析、真实性和负控。effector 在已核签包/实际映射的真实 G13 页串行做两组同档位对照，每组两臂固定输入。第一组复用 REST 验证 Boot，第二组固定 T631/T649 下的已有非零流场用例完成首步、审计与真实上屏。

| 交付点 | 实物与判据 |
| --- | --- |
| **第一步 · Boot** | 一份可部署增量，G13 真实页展示阶段、已耗时、初始化原回执及真实 READY。已有许可允许时展示有 token 的真实初始场并标未推进；仅 READY 时实际效果保持未呈现。 |
| **第二步 · 首个完整时间步** | 同输入真实 ROOT，包括速度、压力、黏性和原 pack/audit 的结果，首个有效 Swap 和截图或短录屏。统计全冷路径而非只统计 Boot。 |
| **第三步 · 连续与暖启** | 保持已通过物理门，测稳定有效 **30 FPS**，同时保真实 Q 推进计数，不以重复帧填充。并行离线准备 Vulkan 角色输入，真机原型仍串行。 |
| **接力 · RX/DM、M1/M2** | G13 验证拆分办法后分别接 RX/DM Boot 与首根。M1 复用 T717 同会话证据，M2 复用 T714 TWO_ON 与同父 T696 臂，分别归因。DM 的独立 ELF 结果可先验证算法，呈现另等真实页面。 |

G13 首轮配对 A 为原单程序冷缓存，B 为拆分冷缓存，两者固定相同持久化开关及场景程序。A/B 各自取得 PERSISTED 后再做 C/D 的对应暖进程轮。缓存坏条目和部分命中分别作独立负控和补充。比较 A/B 回答首次编译改进，A/C 与 B/D 回答各自的暖启动收益。以同条件 A/B 的首个完整时间步为主要冷启对照，Boot 配对用于解释耗时归属。M1/M2 的三/两程序与 off/on 四臂仍按 T696 各自的固定路线执行。 [T696 四臂合同](architecture-T696-v1.md)

**实验控制。** 同设备/OS/driver/profile/种子/真实路由及呈现条件。每轮仅一个应用与实际 GL owner；各臂单独新进程。冷轮只清项目专用程序缓存或确保存储为空，并登记系统驱动隐式缓存条件 UNKNOWN 或已证。保数据新进程跑暖轮，以前次同 key 的 PERSISTED 及实际 BINARY_LOAD 和反射为基准。测试失配和坏条目时使用独立测试目录/副本，验证实际首拒及一次回落。新 context 与冷进程分列。先做一对有效可用的配对，初步看方向；做收益裁定时沿 T696 每臂建议至少五次交替配对，并如实报告样本量。热度/前后台/输入激活动作及实际包身份可用同源回执绑定。原 G13/RX 90/240 秒观察界和 M2 60s/64 query 等既有采集界按路线复用，60s 采集窗口与程序生命周期分别记录。允许约定预算内同 scope 的真实续件，禁止把两次不同配置拼成一个样本。

**计时合同。** 全部使用同设备 monotonic 原生时钟，单位 ns、宽整数用十进制字符串。记录页面配置 action 与原生配置接受、owner 消费、资源/上下文 READY、各程序实际 API 边、自身准备/反射/初始化、compute READY、显式激活、首根进入/接受、首个同 token 的实际有效 Swap 和原返回，以及进程退出、撤销或预算封口。开始点取原生收到真实配置；页面热启动与人工激活延迟分别保留。报告「配置到首有效 Swap」完整墙时及激活相关区间。缓存预载如早于配置需披露前移成本。冷轮计算每个 API 的 paired EXIT−ENTER 和当前阶段下界，跨阶段总值使用真实起止墙时。候选在原版仍截尾前成功时可给同条件的保守改善下界，二者都未到首帧则该指标继续待测。T475 聚合阶段与精确 API 分列，`glLinkProgram` 返回、LINK_STATUS、日志/反射、首次 dispatch/draw 可能分别耗时，不能用后一时刻回填缺失 EXIT。

量测文件按一个进程的一次配置填写。离线 reader 额外核原件 SHA、事件唯一序号、时间域、调用号与 ENTER/RETURNED/EXIT 同 scope、状态和原因与原值一致。准确 API 优先用 RETURNED 或同调用的 EXIT 原边，区分调用返回与后续检查。状态成功必须引用对应事件序。首 Swap 由后端实际呈现原返回、许可及同一真实 ROOT token 联核，独立计算入口的该字段标 NOT_APPLICABLE。逐程序缓存命中需真实加载后反射，Vulkan 仅记录保真 cache feedback，缺该能力时命中 UNKNOWN。合计用所有尝试作分母，保失败/截尾/未达样本，给成功数和耗时样本范围，禁止用只统计成功样本宣称整体长尾改善。 [量测 JSON Schema](architecture-T727-measurement-v1.json)

**采用标准。** 首个同 run/context/scene token 与真实 accepted root 对齐的成功 Swap 才计产品首帧，旧帧/空 READY/故障帧附证但不计。发布等价先过原 ABI/同步/退役负控和数值门，再比较同条件冷启成功率与耗时下界、新进程暖启成功率、具体可用时延、以及稳定有效 30 FPS 和首帧后的真实 Q/root 成本。分阶段记录 CPU/墙时、角色切换成本、GPU/host/缓存的已知增量字节及失败率。现有冷启动产品预算未有新的用户数值裁定，测后以预算与对照结果决定范围，不把低样本长尾或未观测点写成达标。

developer 负责拆分实现、有效构建和原静态依赖/身份交付，tester 维护离线对照解析、oracle 和签包设施，effector 串行真机采证并交每步画面，architect 负责有原件的首因分流。目标20按 P5 独立承接，沿已有首帧专项卡复用产物，不设新的 manager 等待门。不使用真机。要么完成本卡当前约定范围及本角色必要检查，要么遇到确实无法继续的具体问题才停止；不能做一点就停下。不允许修改 D:\hmprojects\HelloWorld\docs\后续工作清单.md（仅本人行状态例外）。GPU 驻留，CPU 停投，前景背景仅 XComponent+shader，禁 Canvas/软件 GL 回退。
