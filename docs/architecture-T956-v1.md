# T956 · 目标 14 解析切格原型与 G16 承托裁定合同

2026-11-24 · architect · Windows Vulkan，Intel UHD 630。目标 14 的解析原型和 G16 承托验证可独立开工。正式水场由 T611 消费目标 13 的同代可信 GridView。 [T952 算法](algorithm-G14-T952-prescribed-sand-boundary-v1.md) · [T953 检查入口](../test/reports/t953/handoff.md) · [T951 归因](architecture-T951-v1.md) · [ABI、精确时序和解析对照](architecture-T956-abi-v1.json)

## 1. 目标 14 · 解析小域工程接口

交 Windows Release 诊断命令 `g14-boundary MODULE_MANIFEST FIXTURE OUT`。输出每个名义步的私有收据和逐拓扑区间的切格、真实开面累计通量及双侧载荷。复用 `pc/vulkan/sand/host_device.*` 设备、队列、QPC、内存和 `pass.*` 的范围、屏障与读回。在现有工程增加独立 `pc_g14_boundary_probe` 目标、`g14_boundary.cpp/.h` 和计算角色，按几何、通量与牵引、审计与封存生成三个角色。复用现有 Vulkan 编译入口和 SPIR-V 验证，逐角色登记展开 GLSL、SPIR-V 的实际字节和 SHA-256，完整 GLSL 上限 49,152 B。GPU 执行计算及审计，CPU 装载冻结夹具与导出证据。首轮交计算和原档，后续画面只用同代数据及 shader，禁止 Canvas 回退。 [现有构建接缝](../pc/vulkan/sand/CMakeLists.txt) · [缓冲与读回](../pc/vulkan/sand/pass.h)

最小交付包括 A 固定周期平床及反向剪切、B 双格带实际开口及闭面/错号负控、C 移动斜床两档 `20×10/10 ms`、`40×20/5 ms` 完整推进至 0.25 s。每列内一条仿射床段，顶点 x 固定且与列界对齐。在 C 中以有理数求 `b(iL/Nx,t)=jH/Ny` 的事件时刻，按 (time,kind,grid key) 排，合并同时事件，归入邻区间边界。持续重合只登记边界身份，禁止制造零长度区间。按全程精确枚举，粗档 5 个内部事件时刻、30 个子区间，名义步最多 2；细档 24 个，其中 1 个落在步界，共 73 个子区间，名义步最多 3。每根预约 4 个，超限报首事件。夹具生成器生成参数和时间表，GPU 复核事件并做切割。首版用解析场，以及 B 的制造通量和负控。切格压力求解和小格聚合由 T611 沿 T952/实验 33 接续。 [T952 局部参考](../test/t952/check.py:135) · [实验 33](../test/g21-exp33)

### 1.1 GridView adapter 与时段身份

`AcquireBoundaryView(input, interval, bedStamp, poseStamp) -> GridBoundaryViewV1 | Reject`。原型接 `AnalyticProvider`，正式 `AcceptedG13Provider` 由 T611 接实际发布器。两者提供相同的字段语义、原值来源和只读边界，使用各自真实证据身份。

| 组 | 字段及消费约束 |
| --- | --- |
| 身份、范围和租约 | `kind/schema/profile, fixtureSHA/layoutSHA/moduleSHA, gpu/driver, run/root/nonce, commit, typedToken, acceptedRoot, stateBank, fieldGeneration, geometryGeneration, bedRevision, poseRevision, coordinateRevision, producerFence{device,queue,handle,generation,VkResult}, buffer{resourceSize,allocationGeneration,offset,length,stride}, readLeaseSerial`。解析/制造输入封存真实诊断 run/nonce 和夹具 SHA，typedToken/acceptedRoot 为空。正式臂沿 T953 的 typed 接受、250 ms、同 root/commit/bank/geometry 和实际 fence。进队前核溢出、对齐及真实只读范围并持 pin。 |
| 时空 | `origin, dx/dy, Nx/Ny, boundary, ghostMap, aliasMap, parentRootInterval, intervalId, subIntervalId, [t0,t1], bed/pose/coordinateRevision`。采用盒坐标 X 右 Y 上、SI、x-fast。区间端点用有理数或目标 13 同版本的物理时间定义，QPC 单独记。子区间 [start,end) 加全程末闭端，严格落在源可采样范围，并核场、床和姿态的冻结映射。缺同区间时序/插值合同具名拒绝。 |
| 流场与压力 | u 面索引 `y*(Nx+1)+x`，v 面 `y*Nx+x`。留实际 LE FP32、面有效性和速度标度；π 为每格 LE signed Q32 low/high。附 `P0,phi,phiRef,phiUnits,gaugeShiftPa,pRefPa,referenceKind,rho,mu,wallVelocity,pose,cert`。`p=p_ref+P0·π/2³²−gaugeShiftPa+ρ(φSI−φrefSI)`。T828 的无量纲势用 `φSI−φrefSI=U0²(φ*−mean_source(φ*))`，且 `P0=ρU0²`。保持源规约均值，切格湿区不重算它。若源已存 Pa 的势压项，只加一次。缺物理参考时只报告相对压力，绝对牵引拒绝。 |
| 结果 | `cell{wetId,A0,A1,centroids,sweep,component}`、`face{faceId,cellMinus,cellPlus,endpoints,activeSpan,∫l dt,∫Q dt}`、`bedPiece{ownerCell,segment,subinterval,quadId,x,n,t,Ub}` 和 `load{Jp,Jμn,Jμt,T,W,quality}`。双侧用同一份离散载荷，水侧以反号消费且记实际计数，保持同一积分点和参考中心。原型交账，正式 T611 接入实际水动量方程一次，T612 后续只消费已核的实际载荷。 |

Adapter 保留正式 G13 原 u/v/π/φ 及布局。用于切格的面速和壁速均表达为相对同一个盒坐标系的速度，源若给世界绝对速度，按实际 pose/墙速合同换算，再用同帧同点做压力和剪切验证。T953 的现有 `gridview` 核规则网格的字段范围、单位、压力及剪切；解析/制造臂新增切格 reader，禁止用合成 typed 接受喂正式门。 [T828 GridView](architecture-T828-v1.md) · [T953 checker](../test/t953/check.py:73)

### 1.2 几何、面通量与载荷语义

1. **几何和扫掠。** 每子区间用实际床线裁两端面积。在格内用局部坐标，分别核 `A0/A1` 和墙片扫掠 `S=∫(Ub·n_s) ds dt=∫ b_t dx dt`；禁用面积差冒作第二份证据。床与格边重合时水侧唯一拥有片，重合物理开面关闭；格角零长度只记拓扑。由实际开面和湿片建连通分量，出生和末干格按实际流向核入向供体与出向去向。区间内通量换号时保存符号分段或报缺证。
2. **面量。** 面法向固定 +x/+y，同一份体积/单位厚度通量，minus 格取 +、plus 格取 −。周期 seam 有且仅有一个 canonical 面，闭面严格零。子区间核 `D∫Q dt=S` 和封闭组件相容。竖向面取 `ψ(top)−ψ(bottom)`，水平面取 `ψ(left)−ψ(right)`，只用实际开放端点。跨开闭先切时段。B 以 10 mm 正方格映射 1/8 为 **1.25×10⁻⁵ m²**；两格相反方向出入，闭面各自暴露不平衡。
3. **牵引。** 同片、同点取 `−p n_s` 与 `μ(∇u+∇uᵀ)n_s`，黏性再投影为法/切向；用同一施力点取 `r×t`，同一真实床速取 `Ub·t`。单位厚度下力 N/m、冲量 N·s/m、角冲量 N·m·s/m、功 J/m。C 用同 interval 的解析函数和梯度，仍核床面无穿透。正式重建时供给器附 stencil、秩、误差和降阶状态；缺有效支持具名拒绝。
4. **精度。** 保存 `Real64=(hi FP32,lo FP32)` 原字用于面积、扫掠、累计面量和分项积分，规则折算与求和保持可核顺序。记录 quadrature ID、实际时间/空间 panel、有效节点和舍入。按误差用局部坐标、补偿和双 FP32 运算，或具实际能力及位级探针的 float64 变体；逐项以独立参考验收。T952 的逐格余额 ≤1e−10 m²、独立切面积 ≤1e−12 m²、解析总力 ≤1e−8 N/m、累计壁功 ≤1e−8 J/m 按原门。容量或数值范围不足需有明确首差。

### 1.3 Vulkan 内存、派发和退役

共用 LE 字典生成 GLSL/C++/Python 偏移。在初版上限 `Nx≤40,Ny≤20` 下，800 cells、1660 face slots（含外边界）、840 bed-piece slots（每 x slab 与各 y 带最多一个片，另留退化边界余量）。每名义根最多四个子区间，段基址 256 B 对齐。

| binding | 逻辑字节 | 容量与区段基址 |
| --- | ---: | --- |
| 0 · 输入 RO | 4,096 | header、摘要、最多 41 个列界顶点、四个子区间、解析函数与求积规则。 |
| 1 · 几何 | 211,456 | header 256；Cell64×800（256）；Face64×1660（51,456）；Piece64×840（157,696）。Cell 留 A0/A1/S、双质心、湿片和 component。Face 留邻接、开口时段和开放长度的时间积分。Piece 留区间两端片参数、owner、法切向、壁速和 sample rule，冻结输入可复算逐样点。 |
| 2 · 积分与审计 | 199,424 | header 256；Flux32×1660（256，预留 53,248 B）；Load128×840（53,504）；CellAudit32×800（161,024）；Component16×800（186,624）。Load128 words 0..3 是身份/flags/count，4..15 为 3 个二维 J，16..21 为 3 个 T，22..27 为 3 个 W，28..31 为面积/积分误差见证字段。除前三组身份 flags 外，28..31 及各 J/T/W 采用双字实数。 |
| 3 · 私有封存 | 1,643,776 | 256 B 收据和四个 410,880 B 的 B1/B2 快照。封存实际有效行，未写尾清零或明确排除出有效归档。 |
| **合计** | **2,058,752** | 单条私有在途链。分配实际对齐、staging、query、driver 占用另记。 |

`FREEZE → K×(CUT_CELLS → CUT_FACES_PIECES → FACE_FLUX → BED_TRACTION → AUDIT → SEAL_INTERVAL) → SEAL_ROOT`，最多 26 个 dispatch。Geometry/flux/load 按固定 id 逐行写，64 lanes。AUDIT 初版单 workgroup 确定遍历和确定归约，加上首错和双侧逐项消费计数。push 固定 32 B `{op,child,count,slotGenerationLo,slotGenerationHi,outputWordBase,0,0}`。按真实数据依赖发 compute→compute，再 compute→transfer→host 可见性。收据核实际 VkResult、同 fence/代、计数、范围和实际 SHA。全子区间及 root 审计通过后一次封存 `ANALYTIC_SEALED` 或 `MANUFACTURED_SEALED`，保 `acceptedRoot=null`。原 250 ms 和当前绑定范围照验，晚尾单列。错代、超时、撤权只可保具名诊断。读取路径 pin 原代，等当前计算 fence 后可独立冷拷并登记新 fence/代，真实归还前禁止重置。将来 WSI 同根 shader 的绘制 fence 和图像 retire 各自持 pin。 [现有私有根调度](../pc/vulkan/sand/g16_multi.cpp:496)

**Tester 交接。** 保留 T953 的规则域接受门，新增切格结果 reader。先用 A 的黏性 +.002、压力 −.020 N/m 及反向剪切核两侧符号，再验 B 的单面守恒和闭面、C 两档全程逐格及初始合力。负控要含规约平移 7 Pa 后保持物理压力、直接拿平移规约压当真实压、床与格边重合、端点同样而中途开闭、出生格无供体、重复面账/同号、错时段/代。每一档交冻结夹具、Release 与模块 manifest、原始收据、可回读字段和独立 checker 首差。T611 仍需目标 13 的实际可信同代 GridView 和目标 14 的真实流场/载荷核验。 [T952 离线结果](../test/reports/t952/offline.json) · [T953 离线结果](../test/reports/t953/offline.json)

每笔样点力对瞬时参考、区间累计冲量对同区间积分参考。用区间平均力时以 `J/(t1−t0)` 计算，两侧使用同一窗口。GPU 首差留 `{stage,reason,child,entityKind,key,measured,expectedOrBound}`，按 stage 和 canonical key 定序。加载/错代、面或拓扑、扫掠、分量不相容、初生无供体、末干无去向、重建及载荷失败分别编码。宿主在封存后从实际段原字计算文件 SHA，GPU 封存核身份、有效范围和完成状态。

## 2. G16 · 交 designer 的承托与余时裁定

T951 B13 `static/J128` 首根微步 15，18 条 journal 为 9 盘 × 两组静底壁碰撞。段 0/1 到达全局 1009.732950/1033.599605 µs，第二次冲量后余 **8.067032 µs**。实际 `outcome/reason/stage=2/11/9`，基底未改。两组在约 +2.794 nm 间隙停止法速，以静壁重力自由回落时约需 23.866635 µs，与两组间隔一致。此推导只覆盖同一静壁、同一粒的局部现象。T955 补第二次冲量后私有态、实际区间、完整候选和首错，全域包含所有盘对及实际四壁。 [原事件与微步账](architecture-T951-evidence-v1.json) · [T906 接触合同](algorithm-T906-planar-avalanche-v1.md)

**请 designer 裁定两个分支及相互切换的物理条件。**

- **认证自由余时。** 两次事件后，先证明所有盘/壁在完整余时中没有新向内根，也没有需要压紧反力的持续接触。用同一第二次碰后状态全域重扫，记录实际墙速/角加速度、盘/壁 gap 区间、法速、最早潜在碰撞 key、已用/剩余时长、总位移和资源范围。全域证书成立才可实际漂移及末端审计，消费完整余时。存在承托、第三次真碰或区间未解时保 11/9 或原首个具名失败。请裁定零新事件余时的推进是否可纳入现两事件段预算。裁定到件前只采该诊断结果。
- **持续承托。** 已确认接触，按实际几何、法向相对速度及维持接触所需反力决定持续或解除。按真实有符号 gap 的一、二阶量或有界时段非穿透方程确定法向约束，输出接触活跃时段与脱离原因。维持接触需要拉力或确证分离时，解除并等待下一次真碰撞；正净空的预测约束须单列。碰撞 J、持续承托 J 和预测分别入账，用同一世界姿态的壁速、施力点、接触对反冲和扭矩核壁功、冲量、动量及能量。多盘维持共同速度快照和有序邻接，保持原两事件段、16 轮和 J≤256 资格。若算法需要更多事件处理或不同时间离散，明确新版本和最坏上界，再做等价或独立物理对照。保 T906 的 TOI、残差、补偿积、穿透、0.1 r_min、墙面无拉力和守恒门。

| 最小判别用例 | 输入及应见证据 |
| --- | --- |
| 静壁承托与有隙自由落体 | 静壁接触盘 `m=.01 kg`、法向向上、`g=9.81 m/s²`、相对法速 0。参考持续反力 +0.0981 N；若承托满 8.067032 µs，冲量约 7.914e−7 N·s，静壁功 0。另给正净空且余时无真碰或向外分离，真实壁冲量为零；预测约束另报。 |
| 退壁、再碰与压紧对照 | 法向向上，给时段壁加速度 `−12 m/s²`，维持接触需 `m(9.81−12)=−.0219 N`，应解除并报零拉力。壁向上 `+2 m/s²` 时需 +0.1181 N。用 T910 和实验 6/7 核区间内失支撑、实际再夹根与壁功。 |
| 余时内真第三碰 | 第二次碰后给另一未接触盘，局部 `v_n=−.01 m/s, gap≈4.007848e−8 m`，独立静壁向内根约 **4 µs**，早于 8.067032 µs。独立 checker 用实际上传 FP32 字复算。认证漂移分支须拒绝并保 key。再加区间两端为正而根未隔离的旋转/擦边样本，必须细分诊断或拒绝。 |
| 多盘侧壁与镜像 | 原 B13 两组 ID、底排和上排全态，独立全枚举余时范围；侧壁对照核连续姿态、壁面压紧法向、镜像交换左右墙号、宽相不漏碰、功/冲量及无重复入账。仅移动时间指针、清正 gap、放宽 residual 或强制第三次漂移的变体应失败。 |

### 2.1 B52 同 fence 晚尾判定

沿 T955 补 B52 static/fast `J=256` 的业务及诊断双收据。原样本 **258.6888/251.9185 ms**，业务 Vk=2，Completion 空，日志零字来自未读槽。冻结 PE/module/seed/pose、run/nonce、device/queue、原 fence/代、grant/deadline、查询池/reset serial。分别采录制、提交进出、等待请求/返回、原 fence 同代确认和实际 VkResult。业务结果要求在原绝对期限前读取、校验并封存；迟到提交保持业务拒绝。GPU 时标分 FREEZE、每 32 微步一块、SEAL 和 COPY，带真实推进/段和已失败空体计数。原代 pin 到同 fence 真退役再读查询和独立冷拷完整 Completion、首错、原候选/BASE 摘要及实际 journal 前缀，冷拷单列代。缺点、错代或设备丢失具名记录。 [首轮 B52 原始定位](architecture-T951-v1.md) · [当前宿主](../pc/vulkan/sand/g16_multi.cpp:313)

| 实际收据 | 工程分流 |
| --- | --- |
| 及时原 fence、同代且结果/审计完整 | 进入实际同代私有正例，按原两档、两转速、连续姿态、残差和呈现验证 B；C 等 B 真正例。 |
| 晚尾有效，出现 reason14/stage8 或其它首物理错 | 同时记录业务超时及真首错。按求解原 residual、边和微步分流原原因。未读的零字不用于判旧故障通过。 |
| 晚尾有效，物理门全过 | 保留该臂诊断证据。按真实 GPU 最大作用域作单项等价优化。桶段为主时优先整数 clear/prefix 并行，保稳定散布和候选顺序；接触为主则按共同快照并行边与盘，保每盘有序归集、原首错、算术和迭代限额。 |
| 缺同 fence 时标，或 host 等待粒度存疑 | 按真实 QPC 分开录制、提交、等待、读回和核验。没有经核的时钟关联时禁止由 GPU 相对时标或 host 超额反推绝对完成时刻。优先处理证实的 host 录制/等待开销。 |

本卡交付工程接口和裁定请求。G14 解析实现和 T955 的 G16 取证接口可开工。B13 新承托或余时规则待 designer 裁定，B52 优化以同 fence 实采分项为前置。
