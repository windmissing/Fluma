# T928 · G16 多盘接触接口与小域放行 v1

> 2026-11-24 T951 复核 T949：B13 第二次底壁碰撞后仍剩约 8.067 µs，触发两事件段上限。B52 的两个超期样本没有 Completion，下一增量先采同 fence 晚尾和分段，再按实际成本做单项等价优化。 [T951 微步账和预算增量](architecture-T951-v1.md)

2026-11-24 · architect · Windows PC Vulkan · 离线。T909 可开工 G16-A 连续旋转壁和 G16-B 的 13/52 盘 GPU 候选、事务与着色。旧旋转壁原件超出 T906 的 TOI 容差。G16-B 实施可并行，结果放行依赖 A 的旋转正证及本阶段独立审计。G16-C 代表负载待小域正例和代表种子。 [工程 ABI 与冷种子](architecture-T928-abi-v1.json) · [T906 算法与阈值](algorithm-T906-planar-avalanche-v1.md) · [T907 布局](architecture-T907-g15-g16-v1.md)

## 1. 复用与隔离证据

| 已有接口 | G16 接入 |
|---|---|
| T909 的 `t909_angle_probe.exe g16 MODULE INPUT OUTPUT`，52 B 输入及 36 B 回执 | 保留平移壁和恒速单调转壁。沿用 `HostDevice` 的 Vulkan 设备、队列、staging、QPC 和原 VkResult。现入口只提交私有 probe，`acceptedRoot=null`。 |
| T909 正式 G15 与 T921 角色包/owner | 复用真实时钟、角色包校验、`Slice`、barrier、readback 与同代退役。G16 独立角色表及 Typed Receipt。G15 列 ABI、±45°角表、DM2 Expected 和 2,404 派发计划归原 profile。 |
| G15/R1 呈现的 `WindowKey / Slice / SceneGate` | 复用窗口作用域、graphics fence pin 和实际 WSI 回执形状。类型化盘 proof 绑定 G16 同根实例及姿态。 |
| T910、T843、T912 | 复用独立孤立解析和原五次姿态，复算 T912 已保存的 GPU 原字。多盘全枚举和多触点残差仍需独立核验。 |

**T912 原字复核。** 平移壁 TOI **0.061922550201 s**，按实际上传 FP32 输入重算 **0.061922552392 s**，冲量和速度满足孤立门。旋转壁 GPU 输出夹区间 **[0.178954705596, 0.178954720497] s**，实际输入的首个向内根在 **0.178874217655 s**。用独立连续 gap 计算，两端均约 **−2.166 µm**，TOI 晚 **0.080503 ms**，超过 T906 **0.05 ms**。0.25 s 初区间二分 24 次的理论间隔约 14.9 ns。工作分流到旋转 TOI 数值和证据接口。下一版采同一输入/模块的端点、近根 gap、实际法向及壁速原字，核函数实现、三角求值误差和夹根边界。当前保留原 fence 成功和平移算子证据。 [T912 原报告](../test/reports/t912/report.md) · [原探针](../pc/vulkan/g16_isolated_contact_v1.comp) · [T910 参考](../test/t910/check.py)

## 2. 13/52 盘和实际壁姿态

- **可复现冷种子。** 盒宽 100 mm、高 200 mm。世界系 X 向右、Y 向上，中心 (0,0)，重力 (0,−9.81) m/s²。复用实验 11 等质量、等面积口径。13 盘半径 5 mm、单盘 0.010 kg，52 盘半径 2.5 mm、单盘 0.0025 kg，均为 0.13 kg、1,021.0176 mm²（约 5.11%）。原软接触末态存在初始重叠及速度差，留作宏观参考。新技术种子用明确的 9+4 / 19+18+15 行列、零初速和自转。初始 `id=origin=1…N, generation=1, material=1`。材料 1 为无切向摩擦、持续及同组多触点零回弹的技术参考。隔离撞击可独立用 T906 的 e∈[0,0.15]。来源与盘身份贯穿全程。
- **上传合同。** LE `Disk64` 按 `<4I9f3I` 存 ID/代/来源/材料，世界 x/y/vx/vy、自转角/角速、r/m/I，最后 flags 和两保留 U32。生成源为有符号整数微米和整数微克，上传单位为 m/kg/rad 的 FP32。质量、惯量和来源用冻结材料及整数质量核验，空槽全零。紧凑种子流为 832/3,328 B，生成命令 `python test/t928/fixtures.py --out OUT`，分别核实际上传 SHA、质心、壁/盘净空。两种种子初态最小壁隙均为 5 µm、最小盘隙均为 10 µm。0° 固定壁加完整允许平移包络，桶法/全枚举候选为 18+9 / 115+19 条，分别最多 5/6 邻边，旋转段容量仍须实查。配对前可用固定 0° 的 GPU 预沉降产同一份已审计 BASE，再用于各档。直接冷种子与预沉降分开标明。两档用同一 BASE，实际窗口起止与材料均封存。 [实验 11 等质量对照](../test/reports/g21-exp11/report.md) · [初态/步长交互](../test/reports/g21-exp12/report.md)
- **T934 修订，2026-11-24。** 小域 V1 的零基 word 13（byte 52）固定为 `1u`，bit 0 表示活动盘槽；word 14、15 固定为 `0u`，其余 flags 位保留为零。紧凑 `[0,N)` 每槽均为活动盘，空尾全字为零。FREEZE 用完整值比较 `word13 == 1u`，覆盖首次上传及以后每次选中的已接受 BASE。合格后逐字复制，运动和接触过程保留该值。两份实名种子、布局和原 ABI JSON 字节均沿用。T932 捕获的旧模块判为实现缺陷，按新模块与匹配宿主复测 B。 [T934 工程修订](architecture-T934-v1.md)
- **姿态输入。** 基线用 T843 的 0→80→100→180→180° 每段 1 s。慢臂前三段各 2 s、末段仍 1 s。近 90° 停留臂和镜像在 ABI 另登记。B 两档初始 J=128/256（65.104/32.552 µs）；实际细分或重试须记录，配对保实际档位。时间以已接受 quanta/120 加 `micro/(120J)` 及真实段内 TOI 表达，同一根内继续余时。T843 10 ms 行用于离线核验，GPU 在实际物理时刻计算原五次段及端点导数。镜像取反输入角、world X、vx 和自转，并交换原左/右墙。零速静盒为独立对照。 [T843 夹具](../test/reports/t843/G16-invert.json)
- **统一壁语义。** 实验 9/11 定义 `box=R(+thetaInput)(world−center)`，数学世界角和角速取输入负值，使正向沿原右壁接力。输出同一 `PoseSample` 的时刻、未 wrap 输入角、世界角/sin/cos、世界角速和角加速度、center/centerVel、inputId。此轮 center 固定，平移留 A 诊断。wallId 0/1/2/3 为原底/顶/左/右。事件时刻投影到真实壁线段，向内法向转回世界，`vwall=centerVel+omegaWorld×(contactWorld−center)`。角点可同时有不同 wallId 两条约束。旧恒速探针用自己的世界角速形状，输入时由独立 gap 核转换。姿态和 shader 使用同一世界位形。 [参考实验实现](../test/g21-exp09/inversion.py)

## 3. 扫掠、容量与实际绑定

即将推进的段用开始姿态作固定参考，按 `(bucket,id,generation)` 稳定分组。每盘只写一个起始中心桶，全桶的 count/prefix 总和为 N，桶头 16 B 不代表只有 16 个盘。B13 桶边 10 mm、10×20，B52 5 mm、20×40。2.5 mm、40×80 用于最大直径不超过 2.5 mm 的后续种子。起始中心越域具名拒绝。

盘 i 的扫掠中心 AABB 外扩 `r_i+r_max+max_j|Δp_j|+eps_gap`，覆盖所有可能在段内相交盘 j 的起始中心。范围随 FP32 误差向外取界，以确定性的扫掠证书再做安全剔除，按较小的盘 key 只发一次。墙对每盘直接验四壁，取平移、盘中心位移和 `sup|omegaWorld|×sup|p−center|` 的区间上界。端点均为正须另证区间始终正，否则细分/拒绝并保首个不明区间。实际半步位移受 (0.1r_{min}) 限制。首次向内根在实际分段内隔离，实际端点和 gap 写证，TOI 在段内用首尾含端点的外向 Q24 tick 表达。碰后对余时重做扫掠。独立 O(N²) 枚举共有 (N(N−1)/2+4N) 条盘对/墙对，B13 为 130，B52 为 1,534。

对候选先 count→prefix→限额，再 scatter。候选边最多 8,192，含墙。每盘候选相邻最多 16，CSR 总方向项至多 16,384，盘对两端各加一条而墙只加一条。盘 key 为以 ID、代排序的两端，wallId 用明确字段并将 B 两字置零。按实际 key/端点稳定归集，记录扫描桶、原始检查数、有效唯一边和最大度。超限在写前记首键及所需数，只在细分能缩减实际候选时重试。独立全枚举路径用于证漏边和 TOI，GPU 主路线仍用分桶。

| Compute set 0 binding | descriptor offset=0 的范围 | 同一 buffer 内的逻辑范围 |
|---|---:|---|
| 0 状态 SSBO | 196,608 B | 0 / 65,536 / 131,072，各 1024×64 B。BASE 只读，两段私有状态用于候选/审计或 ping/pong。 |
| 1 接触 SSBO | 1,179,648 B | 0 / 393,216 / 786,432，各 8192×48 B，分别用 KeyBracket / SolverResidual / GeometryWork，同一 edge index。 |
| 2 工作 SSBO | 608,256 B | 邻接 262,144 B、桶 51,200 B、稳定排序 32,768 B、工作区 262,144 B。工作区按 ABI 分出配置/姿态、收据、CSR 头、两代速度、盘审计、扫掠、归约及 witness。 |
| 3 画面 SSBO/VBO | 65,536 B | 0 / 32,768，各 1024×32 B，PACK 从实际盘及材料身份生成。 |

按 `HostDevice::Allocate` 为四个整缓冲单独分配，绑定 offset 0，段内按字典索引，最大逻辑载荷 2,050,048 B。每个实际绑定记录 buffer/offset/range/generation、`VkMemoryRequirements.size/alignment`、memoryType、真实 limits 和 stage flags。资格检查实际 `maxPerStageDescriptorStorageBuffers/maxDescriptorSetStorageBuffers/maxStorageBufferRange/minStorageBufferOffsetAlignment`、反射及内存账。实际 staging、对齐、FBO/图形和退役中资源另计。内部切片若另绑 descriptor，重验 offset 与范围。GPU 场景缓冲具备 `VERTEX_BUFFER` usage，graphics 以 32 B 实例 stride 读取已接受槽，避免新增顶点 SSBO。Dispatch push 不超过 64 B，完整 shader 编译单元不超过 49,152 B。 [现有 `Slice` 及 helper](../pc/vulkan/sand/pass.h) · [Vulkan 限额](https://docs.vulkan.org/spec/latest/chapters/limits.html)

## 4. 多触点、原子根与 shader lease

2026-11-24 · T943 修订。候选邻接先列潜在边。TOI 以误差区间证明首个向内根或具名未隔离。接触求解使用有真实接触、持续支撑或预测非穿透证据的边，正间隙只有符合预测条件时进入。仍保相同物理、残差、容量和原子性门。 [T943 工程修订](architecture-T943-v1.md)

1. **调度。** FREEZE → PREDICT → BUCKET → CANDIDATE/TOI → ADJACENCY → CONTACT_PREPARE → JACOBI Edge/Gather → INTEGRATE → AUDIT → PACK/SEAL。相邻实际写读用 Vulkan stage/access barrier，跨队列另做 semaphore 和 queue-family 转移。每轮所有边从同一 v/ω 快照算接触增量，再按固定 CSR 归集到下一快照。几何接触点、两端等反冲量、自转矩和墙功都用实际世界接触点。迭代区输出暂存的本轮增量，封存时物化实际总量。每段最多 16 轮、每微步最多两个段、每根 J≤256。TOI 同容差事件一起处理，碰后推进余时并防止重复冲量。22560 是路径的逻辑 ceiling，实际 dispatch、barrier 和独立重放另计。
2. **残差及原字。** 48 B KeyBracket 输出 key、flag、双 TOI、gap、法向与趋近界；SolverResidual 输出 λn/λt、前后 vN/vT、目标法向、zN、补偿乘积、切向残差和有效逆质量；GeometryWork 输出真实接触点、壁速、合冲量、墙功、耗散、两端扭矩。保持 T906 的 εgap=min(50 µm,0.02d_min)、zN≥−1e−3 m/s、abs(λn·zN)≤1e−3 m/s·λn+1e−10 J。静摩擦取实际切向残差≤1e−3 m/s，动摩擦核边界及反向耗散，λn≥0。记录实际最大穿透和段后速度。相对功以更新前后速度复算，粒对只计一次。段尾审计从实际冲量独立求 ΔP、ΔL、Δ(K+U)、墙功与耗散。故障分类保未夹根、过穿透、容量、非有限数、未收敛、守恒、对象/范围与时限，按阶段、微步、段、首 key 输出。
3. **事务。** FREEZE 固定 scope、run/owner/context、nonce、plan/live permit、窗口/geometry、BASE bank 和代、各版本 SHA、真实 deadline。根在私有 bank 写，失败前缀只读诊断。GPU 最后封存绑定 2 byte 350,208 的 Completion256，热读仅 1,024 B。ABI 给出字段的字偏移、阶段码和 GPU 首因。主机独立记提交/退役 VkResult、实际 fence 句柄/代、拒绝前后状态及场景发布身份。严格核同 fence、当前 permit、相同输入字和 typed 收据后，owner 一次更新 BASE/commit/acceptedQuanta/publication。成功根 commit 和 acceptedQuanta 各加 1。拒绝、错代、迟退役保原 bank、commit、物理时间与 CURRENT，保已提交老根资源并按同 fence 独立退役。算子 probe 和私有验证根仅消费私有 commit，`acceptedRoot=null`。业务模式继续用真墙钟、原 250 ms/cadence 和窗口有效性。
4. **同根绘制。** PACK 从通过审计的候选写空闲 `Scene32` 槽，实绘 shader 使用实际 world XY、半径、盘身份及同根姿态。lease 封 run/nonce/commit/acceptedQuanta、inputId、窗口/geometry/swapchain、state/scene buffer+range+generation、slot、disk count、leaseSerial。提交前做 compute 写→vertex-input 读，图形与冷拷贝各持 pin 到同 fence，WSI image/present/retire 单列。拒绝渲染具名故障或标 STALE 的旧有效帧。冷证据留本根选中工作/事件的有效区、原始计数、截断状态和实际副本 fence。可单独运行同 BASE 的独立诊断重放，使用独立结果、资源账和 fence。PC 报实际 GPU/驱动、各段 `B/N/Q/C/I_c`、dispatch/barrier、队列及真实预算。

## 5. 三阶段放行与角色移交

| 阶段 | 现在可做 | 放行证据 |
|---|---|---|
| **G16-A 孤立与连续壁** | 复用已交平移原件。接四壁连续采样，修旋转夹根及有效精度，复用 T910。 | 旧旋转偏差需具名复现并修正。TOI 误差≤0.05 ms、碰后速度误差≤0.01 m/s、孤立越壁≤0.05 mm。顺序覆盖压紧、沿壁滑、解除/重建、真正自由段、快慢退壁、100° 的侧壁承托/离壁、镜像及角点。附真实余时、壁功与 fence。 |
| **G16-B 13/52 盘小域** | 开工两个种子、网格和多盘私有根，按候选无漏、单段解残差、连续根分项交付。实际通过需 A 的连续壁正证。 | 各同 BASE 跑两步长×两转速及静盒。头尾各 0.20 s 原底/原顶占对应段总法向冲量≥85%，同时证侧壁及至少一来源跨角净空。分别报四壁净空与真正无粒间承托的飞行。末段 RMS≤0.10 m/s，13 盘终堆≥12 mm，两档末段平均质心差≤2 mm。多盘穿透≤min(0.25 mm,0.1d_min)，线/角动量误差分别≤1e−3 M√(gH)、1e−3 MH√(gH)，能量残差≤1e−3 MgH。衔接真实 shader 原件。 |
| **G16-C 代表负载** | 可离线准备容量阶梯。 | 1.25 mm 半径在 1,024 盘约 25.13%，现 cap 内可设计合格的约 25% 种子。35% 约需 1,427 盘，须独立证明新容量或增大粒径后依旧保真。依赖 B 正例、代表种子、GPU 阶段耗时及效果。目标独显实测用户要求的 30 fps 和实际物理推进率。 |

Developer 用本 ABI 继续 T909，G15 的 PC 结果无需先到。Tester 接小域全枚举候选/独立余时冲量、首错及范围负控。负控至少覆盖错法向/手性、首碰漏根、错代/重复 key、溢桶、17 邻边、未收敛、拉力、热回读时限、迟到同 fence、scene slot 尚 pinned。Effector 分开采 A/B 实际证据。仍缺 A 旋转 GPU 正例、B GPU 多盘及合格预沉降（若使用）、C 代表材料/种子/成本。其余工程接口可实施。保 GPU 驻留和 shader 渲染，禁止 Canvas 回退。
