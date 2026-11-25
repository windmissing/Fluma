# T982 · 目标 14 的 16×16 移动床预览接口

2026-11-25 · architect · 当前仅 Intel UHD 630。先用现成 G14 解析 GPU 切格执行固定、移动和镜像三臂，优先看到水量方向、边界和床片。该切格试验和 shader 接线可独立实施。实际 G13 16×16 场由同设备诊断适配器接入，再在切格上求开面通量修正。T611 和目标 12 仍使用各自的正式前置。 [T952 切格算法](algorithm-G14-T952-prescribed-sand-boundary-v1.md) · [T956 工程原合同](architecture-T956-v1.md) · [T978 预览工件](developer-T978-v1.md)

## 1. 可立即实施的三臂

闭盒 `L=H=0.10 m`，预填水，单位外厚度，16×16、`h=1/160 m`。水域在单值床上方，床顶点 x 固定在列界。`ξ=x/L`、`s=2ξ−1`，三臂共用 `b₀/h=31/4−|s|/2`。设 `T=1/4 s`，`r=1/40 s`，三臂 `m=0,+1,−1`，床为 `b/h=b₀/h+m·a(t)·s`。`a(t)` 在起步 `t²/[2r(T−r)]`，中段 `(t−r/2)/(T−r)`，末段 `1−(T−t)²/[2r(T−r)]`。两端床速均为零，三臂共用起止时段，镜像由 `i→16−i` 移植。床最大位移一格，移动臂最窄水隙 **7.75 格**，固定臂 **8.25 格**，均满足四格下限。水总面积始终为 `17/3200 m²`。

解析 `ψ=−L·m·h·a′·(ξ²−ξ)·(1−3η²+2η³)`，其中 `η=(y−b)/(H−b)`。取 `u=∂yψ`、`v=−∂xψ`。实际给定床速 `U_b=(0,m·h·a′·s)`，壁上 `v−b_x u=b_t`，顶和侧的外法向通量为零。初始 `a′=0`，三臂同场静止。用于牵引的小域解析 `p_g=4m(ξ−1/2)+4(y/H−1/2) Pa` 给定相对规约压，`ρ=1000 kg/m³, μ=.01 Pa·s`，与动量真实求解所得压力分开标识。法向 `n_s=(-b_x,1)/sqrt(1+b_x²)` 指向水，水对沙压力 `−p_g n_s`。解析黏性按 T952 C 的 `A=6L·m·h·a′·(ξ²−ξ)/(H−b)²`、`c=2m·h·a′/L` 在每列段以实际 `b_x` 算法切应力。力矩参考 `(L/2,0)`，壁功以相同壁速、相同力和样点核对。 [T952 牵引与正负符号](algorithm-G14-T952-prescribed-sand-boundary-v1.md#2-通量压力和载荷)

每名义步 `1/120 s`，共 30 根。每段的起止、坡度翻转和所有格顶点过线用有理数留档。两速度切换在根边界 `3/120` 和 `27/120`，所有内部格事件出现在中段。固定臂 30 个子区间。移动与镜像各 36 个子区间、单根最多 2。中线统一以 +x 为正。M+ 右床升、左床降，累积净水量 **−1/6400 m²**，镜像 **+1/6400 m²**，固定 0。该净值可独立从左半域实际扫掠 `∫_0^{L/2}[b(x,T)−b(x,0)]dx` 得到。单面负号只翻对应 +x 面，镜像后的 +y 面按列反射并保符号。压力、黏性各分量在镜像下 x 与绕中心力矩反号，y 与壁功同号。沙侧与水侧冲量及功逐项反号。 [有理时间、顶点和夹具](../test/artifacts/t982/fixtures.json) · [独立面积/扫掠/面通量与牵引 oracle](../test/artifacts/t982/oracle.py)

离线复算的两条移动臂 `max |D∫Q−S|=2.753×10⁻²¹ m²`，固定臂为零。闭面漏量为零。仅按床位置更新掩膜并令 Q=0 时，最大格水量缺口为 `1.35634×10⁻⁶ m²`。移动臂全程沙侧相对压冲量 `(-0.00185546875,+0.003125) N·s/m`，法向黏性和切向黏性冲量各有独立原值。镜像、水侧反作用、重复生成产生的 SHA 可据原件复核。 [离线数值原件](../test/artifacts/t982/oracle.json)

**Developer T984 可直接实施。** 复用 T957 `pc_g14_boundary_probe` 的 Vulkan 路径、原四 SSBO 和 `FREEZE→CUT_CELLS→CUT_FACES_PIECES→FACE_FLUX→BED_TRACTION→AUDIT→SEAL_INTERVAL→SEAL_ROOT`。新增独立 16×16 类型，建议 `kind=4`，模式值 `0/1/2` 分别 fixed/moving/mirror。以新 fixture/manifest SHA 进入。原解析 kind3 在 `bed/壁速/牵引` 有 C 场景常量，新类型显式指定新床形和运动。`input-N` 原 4,096 B 内的头、顶点、child `[t0,t1]`、L/H/ρ/μ 位置可复用。扩展的基线、幅值、ramp 固化于版本和选择参数，保装载核对并在 shader 实际使用。固定、移动、镜像子区间各自同一份开面流量。16×16 有 256 格、544 面、最多 272 片容量上界，原 ABI 能容纳，先复用稳定行布局。T962 已实测 `shaderFloat64` 及原 1+2⁻³⁰ 拆字，新 PE 继续记录该门。**Tester** 用脚本独立复核实际 fixture digest、正确 Nx/Ny、真时间段、至少四格、水量守恒、镜像、共用面与两侧负载，特设闭面有流、错时间/错代、固定壁速套移动几何及只遮罩无排水对照。 [原 GPU fixture 入口](../pc/vulkan/sand/g14_boundary.cpp:283) · [T962 六正六负](../test/reports/t962/report.md)

## 2. 真实 G13 场接缝与排水修正

| 字段 | 16×16 适配约定 |
| --- | --- |
| 原字和范围 | G13 同 root 的 `CANDIDATE` State 中 `uOffset=256 words`、`vOffset=528 words`、每种 272 FP32；有方向面据 `u[y*17+x]`,`v[y*16+x]`。动态 `π` 从 word 800 起，256 个 signed Q32.32 low/high pair。`PACK1` header64、cell16、全缓冲 16,640 B 供同源绘图，格心速度不得反推或伪造原 MAC。 |
| 资源与生产者 | `fieldKind, Nx/Ny, layout/config/role/fixture SHA, run/root/nonce, sourceBank, fieldGeneration, geometry/pose/bed/coordinateRevision, interval/subinterval, modelTick, VkDevice/queue/fence/generation/VkResult, buffer offset/range/allocGeneration, readLeaseSerial`。记录来源是 `T978_COARSE_PREVIEW_ROOT_RETIRED` 的真实近似私有根，acceptedRoot/typedToken 保持空，原业务及时/迟到各自保留。不得仅凭文件 SHA 相同建立运行时租约。 |
| 单位与压力 | 速度用盒坐标相对速度，G13 无量纲 `u_SI=U0·u*`。预览显式选择 `L0=.10m,T0=1s,ρ=1000`，故 `U0=.10m/s,P0=10Pa`，G13 时刻 `t_SI=T0·t*`。源 `ν*=1/64` 给出 `μ=ρν*L0²/T0=.15625Pa·s`；保 Config 实际黏度和声明标度。分析臂 `.01` 与此不同。动态压力是 `P0·π/2³²`，若保守势由源提供，沿其规约只加一次。T978 无已知绝对物理参考时保相对压，切格湿域不重定源规约。 |
| 时间覆盖 | G13 原 `J1/micro0` 的实际操作时段与 `QPC` 壁钟各记。G14 的每个时间 `[t0,t1]` 必须落在已声明源采样范围或有明确的另一种预览模型。一次 T978 私有快照可作 G14 初始条件或明示冻结外场，标签 `INITIAL_SNAPSHOT`/`FROZEN_INPUT`，G14 自己沿床时序推进；后续无实测动态源不得将原 G13 nonce 宣称为后 29 根物理场。 |

**数据保持 GPU 驻留。** 后续适配在 T978 `Boot/RunT833Candidate` 所在 VkDevice/queue 下挂窄 G14 pass，或等价把两个 owner 所需代码汇入同一连接，读借原 State bank 的实 GPU 只读范围。以结束原计算、Completion/lease/当前 geometry 校验并建立 shader-write→compute-read 的真可见性依赖为前置。G14 结束，等所用 compute/graphics fence 后再释放原 pin。不可把另一 PE 的 VkBuffer 数值传进来当跨进程缓冲，也不可用冷档经 CPU 回灌冒称同根 GPU 驻留。冷档只用于独立验证。复用 T957 Float64 时需在共同 device 创建设备时实际查询和启用，T978 现有获准计算和 WSI 自身不证明新 feature 已启用。 [现有 G13 源保留入口](../entry/src/main/cpp/gpu_resident/g13vulkan_boot/t833_candidate.inc:1076) · [原生来源适配字段](architecture-T956-v1.md#11-gridview-adapter-与时段身份)

**实际水量随床改变。** 每子区间切出 `A_old,A_new`、独立 `S=∫(U_b·n_s)dsdt`、同一物理面的累计活开长与 wet 图。以时间平均活开长 `l̄_f=∫l_fdt/Δt` 乘同段法向 MAC 速度得 provisional 平均 `Q*`；冻结源模型须明确。封闭外壁始终 Q=0，闭面不进算子。用 T952 的 `Λ_f=Δt·l̄_f/(ρ·ℓ_f)` 和 `DΛDᵀδp=S/Δt−DQ*`，得到 `Q=Q*+ΛDᵀδp`。初版 GPU 稀疏图在 256 格有界运行，分量逐个核兼容和固定规约，输出真的迭代数、残差、首差和开放面映射。只接近求解的图与测试阈值沿新的明确 preview 身份。无供体的新生格、末干格无去向、非正有效开长、小格导致收敛失败分别报；不能截小面流去藏 `DQ` 余额。分量规范压变动不得改变净水量。扩展到长时间持续推进时，保湿面积和已修正面速度的前态来源，正式动量、黏性和时间尺度别从这个压力校正臂推定。

从同段修正后的开放面速度重建水侧 `∇u` 和已定床速约束，记录 stencil/秩/降阶/是否缺证。近似压力可按明确同单位的 `P0·π*+δp` 分账输出，保原源相对压力和 G14 修正项及其各自规约。每片用 `−p_g n_s` 和 `μ(∇u+∇uᵀ)n_s`，作用和反作用共享同一份积分、同一点及同床速度。失去物理压力参考则保相对牵引，绝对载荷具名缺证；局部梯度无根据则不输出假剪切。这些近似指标仅给本预览和对照。 [T952 切格投影与压力](algorithm-G14-T952-prescribed-sand-boundary-v1.md#2-通量压力和载荷)

## 3. Shader 与交接

**先交独立解析 GPU 预览。** 从通过 GPU 审计的已封存子代生成小只读 SSBO，保 `run/nonce/bedRevision/geometryGeneration/interval/fieldKind/packetBytes/rootStatus`，并按实际 GPU 积分取 `wetFraction, bed polyline, corrected/signed face amounts, approximate cell arrows, pressure/load quality`。用现有 Win32 `PresentBoot` 同 VkDevice WSI，新增目标14专用 shader，GPU compute-write→fragment-read（如经 GPU copy 再含 transfer-write→fragment-read）。片元绘制实阴影床和开放水域的箭头，从模拟 y 上到屏幕 y 下只转换一次。显示「目标14 · 16×16 · 解析移动床」，蓝区表示真实本子段的湿部；影像或箭头能插值相邻已核 GPU 层，但须给实际模拟时刻和源身份。至少保起步、运动中和终点可见差，或明确首交只有静态的中段 frame，动态回放只在同版多个原 GPU 根封存后启用。无需 T981 原画面。实际 16×16 MAC 钩子到件后换标签「切格近似流场」，第一张图独立带实际 graphics fence/imageIndex/present QPC 和原 buffer SHA，性能、原业务窗和私有晚尾分列。 [现有单 SSBO WSI 示例](../pc/vulkan/wsi.cpp) · [T978 同根首帧合同](architecture-T980-v1.md)

**角色和前置。** T984 可先交三臂 headless 与显式同根 shader 工件；独立 tester 可从冻结输入、真实 seed 和 oracle 做 checker 和负控。Effector 先跑有版本身份的三臂与实际可见前后时刻，再等同 VkDevice 的 G13 16×16 只读诊断 lease 和修正 shader 工件，分别对比实际 MAC source、`Q*`、修正 Q、床形和清晰的近似载荷。G13 正式可信 GridView 是 T611 实际前置；只有经正式目标14验收的流场及边界载荷才进目标12。当前只做 16×16，GPU 驻留，XComponent+shader 唯一上屏，禁止 Canvas 回退。本卡没有运行 GPU 或实际窗口。
