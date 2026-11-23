# T885 · PC 多根接口与后续目标并行架构

2026-11-23 · architect · P3 · Windows Vulkan · 原件与源码离线复核

**目标 1–6 已可用现有工件复采固定重力下的干沙、停源和持续呈现；双态像素和可控重力缺可验的 PC 接口。** 目标 7/8 沿 T882 修同一 FullM1 窗口，目标 9 的 DM2 三态与目标 11 的 R1 真通量可并行准备各自角色、只读证据和呈现接入。水线和大角度、分层、厚度继续按实际物理输入开工。 [原件复算](architecture-T885-evidence-v1.json) · [T824 PC 基线](architecture-T824-v1.md)

## A · 目标 1–6 可直接复用的实证

T856 原件钉 `38a25f9d`，T876 原件钉 `f70ed1c9`，后者协议源码钉 `9101958c`。核工作区原件和 Git 原档，比较时统一检出换行，保存实际文件 SHA。实际 T853、T872 PE 和 `full_source/quantum_candidate/quantum_commit` 三份 SPIR-V 复核均匹配冻结 SHA。复算用原 `Completion256`、`windowFrame`、`WIN32_POINTER`，证明 root/run、source/physics commit、publication slot/generation 和五项 WSI 原返回可联。T876 OFFER 的额外引号仅沿 T496 定点适配，原件保留。 [T856](../test/reports/t856/report.md) · [T876](../test/reports/t876/report.md) · [T853 工件](../test/reports/t853/build.json) · [T872 工件](../test/reports/t872/handoff.md)

| 原件 | 实际窗口证据 | 释放后根与落床线索 |
| --- | --- | --- |
| T856 固定 / run2 / run3 | 固定 SOURCE 接受 131,072 Q20。短按和 25 s 输入档各只有 1 个接受 QUANTUM 同根呈现，分别 126,172 / 136,485 Q20。 | 两份交互档可作 Down→SOURCE→QUANTUM→Present→释放正例。25 s 是输入观察窗，适用的原冷档路径只呈现首根。 |
| T876 run1 / run3 / run5 | 单一沙源在 FullM1 中分别有 213 / 199 / 280 个不同根实际排队呈现，首末间隔 8.954 / 8.158 / 12.047 s。 | 首次 Up 至下次 Down 前分别已有 28 / 7 / 19 个新根。全程 sourceCommit 恒 1，沙库存恒定。第 365 / 434 / 393 行分别见活沙包 1→0，库存仍为 97,584 / 100,011 / 116,887 Q20。 |
| T876 run2 / run4 | 分别有 5 个有效根，首个量子在 SOURCE WSI 阶段，实际为水单源 / 沙单源。 | 第 89 / 87 行分别遇到真实 `ROOT_DEADLINE`，之后未受理第二源。按 T880 保留为短窗失败样本。 |

T876 三条长窗的 `sandLive` 降到零、`S+P` 不变，按 M1 账可判已接受沙留在床库存。转换根各有两个新事件槽，原日志未留相应事件 payload，也未给分列 S/P 字，`Completion256[95]` 在这五轮均为零。可登记三轮停源后继续运行与最终库存证据。逐笔 `LAND`、历时和长期稳定形状仍需 `LAND` 原字及分列库存。T856 历史冷档原封套 `TRUNCATED_PAYLOAD`，两份注明 payload-only 的副本在当前 T848 均联成 `NEXT_FRAME`。T863 的 94,408 B 原封套严格通过，可给冷档格式及首根 reader 正例。 [T848 reader](../test/t848/check.py) · [T863 原件](../test/reports/t863/report.md)

| 目标 | T883/T884 可使用的正例或直接复采 | 剩余接口或证据 |
| --- | --- | --- |
| 1 / 3 高度场、粒子与停源累积 | 用 T856 固定 SOURCE 对照和 T872 `--m1-window … --m1-dual-source --m1-source-trace` 只选沙，固定源位置及原模块。首个接受后松手并保持窗口，按 Up QPC 关联后续根。原 T856 dry 入口和 T876 FullM1 中的沙单源分别登记 profile 与实际请求量。 | 采同代 SOURCE 后、含 `LAND` 的 QUANTUM 与稳定窗末根，逐根按 S/P 原列、有效范围和事件源 ID 核持续存在及守恒。可独立用 `--m1-root-on-busy` 采一个同根后继档，先确认它确实命中所需时段，覆盖多时段需同一运行的按键预约或定容分根采集。 |
| 2 / 4 shader 窗口与同帧双态 | 两个 T856 实际窗口及 T876 多根是 Vulkan shader 同根呈现正例。可另用可采后继根的夹具验证一帧。 | Win32 当前只有一个只读探针像素 `(floor(129·width/256), height−1)`，五轮都读到 `0xff525355`，沙床和沙粒还使用同色。新增有界同 lease ROI 或独立窗口图像配两个可区分位置及 GPU 原 S/P，至少一根同时非零，保同根 viewport/采样坐标、格式、颜色编码与 imageIndex。跨窗代有效呈现另取日志。 |
| 5 空箱与可控重力 | 当前 Win32 pointer/held、捕获、撤销能验证入料和清源。若真实窗口先出现可留空箱图像，按空资源/尚无业务根的身份留档。 | T856/T876 首张业务帧都在首源之后，`M1_POINTER_READY` 只等输入。空箱验收需同代零库存、零伪造 SOURCE、真实 shader 窗口。T872 `WM_KEYDOWN` 只接 1/2 选料，QUANTUM 构建默认 `QuantumParameters`，所有原 `angleAcceptedIndex=0`。现有角表上传证明资源就绪。Developer 可在原 M1 ±30° 域独立交有界重力输入适配，保原带符号单位、输入实际 QPC、锁定 grant 和原 `angleAcceptedIndex`，用正、负、回中核画面/质心。中途更新只影响后续合法 grant，同 grant SOURCE/QUANTUM 保一致，旧代或超域具名拒绝。 |
| 6 compute 长窗与送显 | 用 T876 的唯一接受根、原 VkResult 与实际 QPC 复算业务/退役，保首次、续帧、超期、STALE、重复帧及退役分账。 | 60 Hz 工位先采根吞吐、实际新提交、重显和卡顿，`vkQueuePresentKHR` 返回只证明进队。当前接口缺真实可见间隔或可用的 Present 跟踪，目标环境的持续帧率、尾延迟与负载仍需对应设备实测。 |

建议 T884 先用 T872 只做沙源、不切水的独立运行，避免把第二次按住混进停源窗。能用原工件采到的事实先交。归档用后继 Busy 模式须保存预约和命中键；同一产品窗口跑完整片段和独立冷读对照，各自留走过的帧。上述修订只增加必要的输入、原字和像素观测，计算及 shader 渲染保原，实现可与 T882 并行，但同一 Win32/M1 入口按文件责任合入。 [T858 热／冷边界](architecture-T858-v1.md) · [T869 后继根原件](../test/reports/t869/report.md) · [PC 呈现口径](design-pc-effects-120fps-v1.md)

## B · FullM1、G9 与 G11 的可独立切片

**共用实际外层接缝。** `Window::Input/Pulse/SelectMaterial` 产生失效可追的 Win32 指针/时标，`RunBoot` 持本代 owner 与真实 Vulkan 资源，`PresentBoot` 当前消费一个 M1 `VkBuffer` 并用 `nextPublication` 连续读同代 lease。可复用 HWND、window/geometry 代、QPC、实际队列/fence、资源退役及 shader 送显。适配层应从各族已有具名 typed gate 读取真实接受键。冷读绑定 `(family/profile, actual device/program set, run/owner/context/surface/geometry, nonce/plan, base→accepted commit, bank/arena generation, publication/lease)`，保实际缓冲范围、字节序、围栏和无效化。公共层不解释材料算法，独立 reader 从原族 schema 解字段。迟到/撤权仅退役，不能发布新根。Root/fence/WSI、热尾、冷读、可见帧分别落回有效证据域。 [Win32 入口](../pc/vulkan/main.cpp) · [M1 SOURCE 接线](../entry/src/main/cpp/gpu_resident/g13vulkan_boot/m1_source.inc) · [WSI](../pc/vulkan/wsi.cpp)

| 切片 | 可并行实施与先交付 | 放行所需真实输入及原件 |
| --- | --- | --- |
| 7 · FullM1 第二源 | 当前有语义 `--m1-dual-source`、`1/2`、`command32 kind=0/1`、沙水共用接受链。T882 沿 T880 提供同 hold 合法预约、同代撤权与终态。Tester 先接原 `Completion256`、offer 两端和首帧/续帧关联。 | T882 编译工件复跑沙→水、水→沙以及换料中撤权。一个 run 两笔接受的不同材料 SOURCE，随后同一 root/lease 上有非零沙水，核 `S+P`、`W+J` 与实际事件/像素。T876 固定 probe 与单源通过只给公共接线。 |
| 8 · 侧扩散与水质感 | 复用 `sand16/water20/column16` 布局，现 Win32 shader 已取 bed/W/η 与真实 J 的静态分支。可先独立完成正 `kappa` / 零 `kappa` 列边原字采集适配和 W0/干槽/水面/J 前后图像采样定义，恢复 T832 要求的全量场景语义时先逐 pass 核规格。 | 有真实双源窗口后，使用中央入水和左右均可达的固定初态，交实际正/负方向事件范围、W 变化、ARRIVE 前后、同 lease 业务像素并核整数 guard。专用单水边流可先取证，其结果范围保持单水。 [T832](architecture-T832-v1.md) · [T880](architecture-T880-v1.md) |
| 9 · 三态 writer 与原族档 | 公共平台触发已由 T856/T876 满足。先做 T845 指定 DM2/K192/S40 的 Windows 角色和绑定 manifest、BOOT 与 SOURCE。12 / 2404 原槽、`0xffff / 0x3ffff`、实际 GPU stage/bit 和 typed completion。T883 可同时做显式 R1/DM2/RX reader 路由，拒绝把 M1PCOLD1 当三态档。 | 实际 DM2 角色能力及本族 SOURCE 后继续 QUANTUM，DM20 先证真实动员，再 DM22/29/31 和双向来源。分别取同 token 的 `G9_SPF_DM_COLD_V2` ROOT/FULL、`S/F/P`、面 `EDGE_FLUX`、来源和实际 scene lease。FULL 六个 discovery arena 先预约。K192 原单 slot ROOT/FULL 预算 26,750,976 / 103,354,368 B，另核 Vulkan 对齐、证据信用及并存窗资源。 [T845](architecture-T845-v1.md) |
| 11 · R1 真通量与联合发布 | 可先做 G11 R1 小档 K192/V2048 的生成、反射、范围、布局和 W/V 独立校验。沿 T639/T839 的四 SSBO 加 UBO 与 36 B push、原 19 / 2543 R1 槽。以实际 R1 父 writer 供同根四 map、ACTUAL/REPLAY 与 `G11_JOINT_PUBLICATION_V2`，不把 Win32 M1 的单 publication ABI硬套联合根。 | 到真实 R1 父根及非零有符号 `q` 后，采 `(run, nonce, micro, phase, role)` 的单批 witness（R1 micro=0 FLOW 可先用 ACTUAL 699→702），保真实 rank、反向或零流负控及同 lease shader。DM2 默认档与 R1 是独立资格，G11 的 R1 路不必等待 DM2，DM 组合按自身 2923 槽及实际父证据再进入。 [T839](architecture-T839-v1.md) · [T639](developer-T639-v2.md) |

现行 Win32 CLI 有 M1 业务窗口及 G13 候选入口，G9 三态和 G11 联合发布需要新接线。并行时先交各族独立的 Vulkan 角色 manifest 和离线 reader，随后接真实 owner 与窗口。各编译单元的完整新增 shader 输入、实际变体和直接生成 SPIR-V 按 49,152 B 合同逐项核验，保原逻辑、barrier、实际角色、原字和业务输出。沿原短热尾及受限冷读，GPU 驻留、shader 出图、禁止 Canvas 回退。 [T845 ABI](architecture-T845-v1.md) · [T839 编译及证据](architecture-T839-v1.md)

## C · 后续目标可复用接口与开发触发

### 13 → 14 → 12

| 阶段 | 可先复用的 PC 输入与观测 | 进入下一阶段的真实触发 |
| --- | --- | --- |
| 13 纯流场 | 同一 PC 可跑 T877 `full/hot` 诊断，对齐 F02/03/05 的真实 Config128、State、Win32 pulse 和各实际角色。T874 可读真实 SOURCE ABI 与 V2 冷件、原 PACK 和 GridView 来源。输出真实 u/v FP32、π Q32、φ/平均势、压力参考、ν/单位、网格/面/ghost 与时段；原 QPC 许可及 actual/retire 边沿单列。 | F02 PRIVATE_PROJECT 只给单算子。F03 需要实际接受根序列及收敛，F05 与原 T828/T631/T866 门取得同代速度、物理压力、黏性的可信 PC 数值，并交来源可定位的只读 `GridView`、实际输入/单位和边界范围。T879 同机分段和独显对照各在其资源范围推进。 |
| 14 给定沙轮廓与运动 | 可先准备 GridView 接口的版本检查、给定轮廓时间流和只读边界载荷取证字段。留 `(geometry/time/interval, wall prescribed pose/velocity, swept volume/open face, normal/tangent, pressure reference/shear)` 与真实压力/剪切分量。逐面记录方向、面积、力/冲量、功及流体侧反力配对，单位沿 13。 | T611 等 13 可信 PC 场再定耦合与精度；运动边界/压力规范对新几何的适用性另证。放行 12 需 14 实际同代可信流场与实际边界载荷，能用原字独立复算壁功和冲量。 |
| 12 沙动员、冲刷与反馈 | 可提前定只读消费及回执接口，输入为 14 的已接受 `GridView`、带几何及同间隔的真实 traction/impulse，输出每状态来源账、反作用及已接受新轮廓 version。用脚本载荷可预演封套、拒绝旧形状和资源退役，留其来源。 | T612 等同代真实载荷原件及其独立参考。设计冻结后再实施动员／离床／悬浮输运／再沉积与下一间隔轮廓反馈，按实际力/功、两相量与轮廓演进验。 |

已复跑 T874，历史 REST SOURCE ABI 及三组合成和六项负控通过。现有 G13 原件尚无符合本门的可信完整场，T611/T612 各维持原真实前置。 [T828 验收迁移](architecture-T828-v1.md) · [T873 根期限](architecture-T873-v1.md) · [T877 诊断入口](../test/reports/t877/handoff.md) · [T874 原档适配](../test/reports/t874/handoff.md)

### 15 至 19

| 目标 | 已有 PC 固定输入及需保的输出 | 允许进入实现的触发 |
| --- | --- | --- |
| 15 ±45° | T843 两份 301 点 10 ms 精确连续角轨迹。实际运动保未 wrap 角、ω/α、容器轴、世界/盒坐标、轨迹接受时段。观测净搬移、质心/前缘、动员/功、崩塌与终堆。 | 干态可独立进入候选验证，先冻结材料/壁和独立参考、判据及资源，证明跨原 ±30° 的表示和步进；湿态等待有效 14/12。 |
| 16 跨 90° 到 180° | `G16-invert` 401 点，80→100→180°；留实际空隙、脱离根、自由段轨迹、碰壁冲量与重新堆积。局部接触和全床分开。 | 需要能表达真实自由体积、任意壁支持和接触的算法/容量合同，跨角稳定性和守恒有独立参考。原稀疏 P 的容量正反例引作约束。 |
| 17 全转、停转、反向、重复 | `G17-turns` 1,501 点，含 179→181°、停转和越 360° 后反向。记录未 wrap 相位、累计时间、真实惯性/壁功、历史来源和被接受根。 | 16 的支撑、接触及自由体积有可用接口后，再验真正连续多圈、暂停和反向；角度显示或周期重置不能决定物理身份。 |
| 18 层间差速与剪切 | T846 C17–C20 的等均速不同剖面、反向、交换层序、守恒/耗散和 donor 动量参考。保存按深度的速度、动量、层间相反冲量、壁剪切和误差预算。 | 待独立参考及以同驱动/误差/资源预算比较候选，冻结真实可观测状态和容量，再选实现。双层 F 保持候选，可与其他层数或闭包并行评审。 |
| 19 真实厚度及前后壁 | T846 C21–C24 的 2/4/8 mm、Nz 8/16/32/64、no-slip 两板、前后激励、z 速度和壁剪切/通量参考；留截面、真实 z state、实际体积/孔隙与六壁载荷。 | 先定三维表示、边界精度和资源合同。薄隙解析例可独立验证；真实 3D 沙水则等待适用的 13/14/12 可信物理与反馈合同，并另证三维压力、壁接触、耦合和细化。 |

以上保 T829 C01–C24/E01–E15 及原物理判据。T843 四份输入已逐个用原 checker 校验，T846 的 8 份参考和 8 项负控已复跑。它们可给 Windows 实际输入回放与独立比较，尚无 PC 物理交付。重力方向控制和规定旋转是两种输入，前者验证目标 5 的控制接口，后者的相位、ω/α 和作用力进入 15–17 的实际求解器。按调度顺序 **13→14→12→15→16→17→18→19**，有真实输入的独立准备可以先做。 [T829 前置与对应表](architecture-T829-v1.md) · [T843 输入](test-report-T843-v1.md) · [T846 子问题](test-report-T846-v1.md)
