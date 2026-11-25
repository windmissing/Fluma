# T988 · 目标 9 长根、目标 13 八格与续根、目标 14 真场切格

2026-11-25 · architect · 当前 Intel UHD 630 `8086:3e98`、驱动 `31.0.101.2140`。三项可独立实施。目标 9 已有成对热点方向，首项继续缩 OP21 内的一处精确商余。目标 13 可同计划测真 8×8 私有预览，并由 T986 快臂单独接续真实 SOURCE→QUANTUM。目标 14 可借真实 16×16 MAC 场做一次移动床开面修正。产品和正式资格各用原判据。 [T981 正式报告](../test/reports/t981/report.md) · [T987 正式报告](../test/reports/t987/report.md) · [离线复算原件与脚本](../test/artifacts/t988/evidence.json)

## 1. 目标 9 的下一处等价工作

T987 r2 的八个 QUANTUM 原 q0→14 点，以 36 位回卷和 `83.333ns/tick` 重算，均在原 fence 晚退役可读。全部 `GRANT_DEADLINE`、原 `VK_TIMEOUT`、业务准时 0/8。ABBA 与 BAAB 的 B−A 整根分别为 **−81.713/−14.347 ms**，六个 OP25/27 的合计差 **−52.986/−33.625 ms**。保 T976 面通量候选及其已建立等价，六段短于对照的方向在两个配对块中一致。A1 位置升高及整根次序漂移仍须保留。B 整根实测 1044.50–1096.56 ms。假定六段完全省去、其余原时长固定，B 仍有 **612.679–640.816 ms**。这给报告里「约一秒」与减去六段后的两种量明确的边界。 [原配对与全部试次](../test/reports/t987/runs/g9-paired-r2)

| 同原 fence 已测范围 | 诊断时间 ms | 可归因范围 |
| --- | ---: | --- |
| T987 r2 B 的 DISCOVERY J1 / AUTH / REPLAY | 329.664–343.330 / 297.466–342.205 / 321.919–333.851 | 三段实际有效物理与证书都在内 |
| T987 r2 B 的 CLEAR / COMPARE·PACK·SEAL | 24.456–29.621 / 49.554–55.102 | 粗窗，不能进一步分配给各自函数 |
| T968 operations OP21，三 scope | 53.151 / 43.859 / 45.301 | `g9Cfl→g9SnapshotMicro→dmRequest` 合并槽，独立原根 |
| T968 audit OP32，三 scope | 24.279 / 22.945 / 22.838 | `Fold/CFL/质量几何/动量/能量/微步证书` 合并槽，独立原根 |
| T968 audit OP33，三 scope | 48.896 / 46.711 / 50.523 | 终段质量几何、动量及 RootEnd |
| T968 audit OP42 | 24.880 | PACK 元信息及 root audit；在粗 COMPARE 段内 |

**首个开发候选。** 固定 `inventorySingleScan=true/faceExactDivide=true`，只给 OP21 `dmRequest` 中 `upper` 的一次 `g9UDiv(omega·deficit,120·J·65536+omega)` 加可关闭的精确 64÷32 分支。复用已计算的两份原 `uvec4`。仅当 N 的高两 limb 为零且 D 恰一个非零 limb 时走快支。令 `qHi=hi/d, r=hi%d`，然后对 lo 的 32 位执行 restoring，`carry=r>>31`、`t=(r<<1)|nextBit`，`carry!=0 || t>=d` 时减 d 并置当前商位。每步结果始终满足 `0≤r<d`，即使中间 33 位靠 uint 回卷表示，最终 `n=q*d+r` 与原 floor 商余一致。高位或 D 为零走原 128 位函数和原返回顺序；不改变产品的窄域或错误输入判据。只改这一调用，保 `g9UMul/g9UAdd` 原溢出、其他 `dmEval`/CFL/舍入、128 列循环、所有事件和三 scope。独立离线向量已核 **20,481** 组精确商余，另有 **103** 个应保原函数的宽分母。纯数学测试只证明新分支等价。 [OP21 原 upper 调用](../entry/src/main/cpp/gpu_resident/g9dm/pipeline.comp:1251) · [独立整数模型](../test/artifacts/t988/reconcile.py)

**独立验证与次序。** Tester 以同模块检查商余向量、编译后的实际替换与非候选 opcode 模块 SHA，覆盖分母 0、32 位最高位、宽于 32、全 64 位最大值、临界余数、J1/2/4 正控及真 Q 拒绝前缀。Effector 用同 PE/输入、既有 SOURCE retired 准备、相邻 ABBA/BAAB 先看 `--quantum-detail operations` 中 OP21 的 9/815/2013 槽，再取 `audit` 窗中 OP32/33/42。继续保 2404 逻辑、三 scope 独立、原同 fence、原始首错及 EVENT/PATH/选中与两侧实际物理原字。测到的候选只占 OP21 中一项成本。若 OP33/OP42 仍占重，第二项可把 source 空槽与 map padding 的纯读零校验作工作组并行预判，lane 0 仍在原遍历位置消费独立标志并保此前写入/首拒排序。先以同根细点定位此函数内贡献，避免把旧 OP33 合并跨度全归给清零。 [质量审计原代码](../entry/src/main/cpp/gpu_resident/g9dm/pipeline.comp:1849)

## 2. 目标 13 真 8×8 性能对照

| T981 16×16 | host submit→原 fence ms | 同原 fence GPU 根 ms | ACTUAL / REPLAY ms | 819 seal / 其 SHA ms | root fence→Present / graphics submit→fence ms |
| --- | ---: | ---: | ---: | ---: | ---: |
| circulation | 908.150 | 907.826 | 521.520 / 358.346 | 23.568 / 23.529 | 55.754 / 0.394 |
| projection | 1016.612 | 1016.139 | 606.608 / 374.276 | 27.395 / 27.355 | 76.439 / 0.346 |
| zero | 884.795 | 884.416 | 521.854 / 332.426 | 25.800 / 25.759 | 53.116 / 0.339 |

同原 fence 计时取 `ROOT_BEGIN→BUSINESS_COPIES_DONE`（36 位），两个 active scope 占 **96.5–96.9%**。冷拷贝提交至 fence **0.649–1.769 ms**，原 fence 至 Present 包含冷证、主机检查与 WSI 准备。Present API 自身本三次约 0.012–0.017 ms。三臂各 **820 逻辑、852 物理、852 compute barrier**。ACTUAL J1 的 16 个压力块时间合计 **109.343/137.681/20.187 ms**，前两臂到 block 5 后转近空操作，zero 在 block 0 后即转近空操作。整个 ACTUAL/REPLAY 仍重，细点不能把其余时间一概归成压力。OP47 的 ACTUAL/REPLAY J1 序号 **64/469**，`gpuSha(2,uL[4])` cover 整 State（含固定 128 KiB tracer 保留区）。其次在相邻点单独量该 SHA 与相邻 audit、J1 scope 中尾段，以及无作业范围的派发/屏障。819 SHA 与 OP47 各自产证，保封存原字和 gate。 [16 原日志](../test/reports/t981/runs/g13-16-circulation/preview.jsonl) · [OP47 全状态摘要](../entry/src/main/cpp/gpu_resident/g13flow/trace.glsl:7)

**T989 可立刻实现真 8。** 为 Win32 私有 F02 性能对照定义独立 `G13_WIN32_PREVIEW8_PROFILE_V1`，仅显式 `--g13-coarse-preview 8` / `--g13-coarse-preview-zero 8` 可选。dimension `8²`、MAC `72+72=144`、`h=1/8`、MG `8→4`（cell 起点 0/64，合计 80）、reductionGroups=3。State offsets 为 `u/v/pi/tracers=256/328/400/528` words，总 **33,296 words / 133,184 B**。WORK 为 `10×144 + 4×160 + 4×64 + 48 + 4096 = 6480 words / 25,920 B`，snapshot 冷档若实际启用另加 128 B。PACK header 64 words、每 cell 16，总 **1,088 words / 4,352 B**。沿现 trace 量 44 套算子 sweep 标记/active V，记录实 active/no-op 与收敛。profile 表、Config[4/5/10]、Seed 与 Config 自哈希、BootLayout UBO、多段冷档 ranges、PACK header + 实际线像都核到 8。生产拒绝用显式私有选择及尺寸，保 T607 canonical 布局/哈希及其 profile 表，避免当前 `p==kProfiles` 依赖首项的位置变化。私有表及规则完整 SHA 与原 layoutSHA 分开存，两者共同标识工件。普通 16/32、prod Find/Validate、工具和正式 F02 等级沿原入口。 [布局公式](analysis-T607-engineering-layout-v1.py:160) · [现有 profile 合格门](../entry/src/main/cpp/gpu_resident/g13flow/wire.h:138) · [8 夹具完整字段及 SHA](../test/artifacts/t988/preview8-fixtures.json)

**首测保持同计划。** 保私有 J1、ACTUAL/REPLAY、同代 compare、staged OP11 scalar seal、PACK、820/852 及时间点。T980 离散场重新计算在 8 个格上，非零两臂各 112/144 面，circulation 预期轴向四点约 `±0.0220971`、压力约 `±0.242037`；纯梯度保压力并趋零，静水两场零，墙法速零。独立 checker 通过真 u/v/π 原字、布局、两 scope 记录、late/业务标签及 WSI/PACK 融合说明尺寸。保持原残差、审计、seal 与 bit-identities 门，失败留第一原码和循环残差。只有 8 明确败在物理阈值且用户需要独立轻量预览时才另立算法与标识。真 8 不用图像缩放原 16。developer 可运行 `python test/artifacts/t988/make_preview8.py --out <目录>` 按原 SHA 生成三臂。effector 在同一设备和构建上按固定工件对照 8 和 16，记录 compile/preheat、host、GPU 各前后、active/no-op、819、cold 与 WSI。网面积为 1/4，固定 128 KiB、ACTUAL/REPLAY 之外的 GPU 时间、852 次派发等不能从面积按比例推时。把两 scope 全理想化到 1/4、其余原样，三臂 GPU 约 **248/280/244 ms**，只说明 250 ms 仍需量证和业务余量。新增 OP47/相邻时间点同次出具时优先与压力热窗拆项。

## 3. 目标 13 SOURCE→QUANTUM 最窄接续

T987 三个隔离 SOURCE 均为 16/16，原输入 SHA `790991b2`，快/组合/优化 singleton 的 grant→match **121.884/136.957/129.050 ms**，原 outcome=1/首错 0/0，同原 fence，业务根仍为空。组合真实 `vkCreateComputePipelines` 完成 **287.346/302.026 s**，解释了原 180 s 观察窗。暖 singleton 的角色创建约毫秒，日志中的 `pipelineCache=null` 表示应用未传 cache，不据此标识实际驱动磁盘命中。 [三臂原件](../test/reports/t987/runs/g13-source) · [T986 实现与取证](developer-T986-v1.md)

1. **单独 opt-in 链。** T989 新显式 `--g13-source-chain` 在原 `REST_BOOT_RAMP_SOURCE` 上选 T986 四 singleton flags=1，并为真实 Q 显式选现成窄 `OP48_VISCOSITY/OP49_PRESSURE` 与 staged scalar seal。链专门在第一 SOURCE grant 之前准备并资格两套实际 pipeline，避免角色资格重置 command/fence/CTRL 触碰已 pinned 的 SOURCE。编译创建、OP66 与更早的 Q 角色预检失败独立留原点，计时不充入两 grant。T986 单独 SOURCE 保继续恰 16 dispatch 及无 eager Q。链只能覆盖一对根。 [源码准备及绑定](../entry/src/main/cpp/gpu_resident/g13vulkan_boot/t833_candidate.inc:192)
2. **业务规则下的私有延续。** SOURCE 沿 `root(true,k_source,0,1,4,0,1,0)`。仅同 authority、geometry、input 和实际提交 `VK_SUCCESS`、原 fence `VK_SUCCESS`、在第一原 250 ms 内、原 Completion/16 槽/双作用域全过时创建私有 lineage。保存 run、nonce、SOURCE seq 1、generation、原 fence 和原 grant、Candidate bank=1 的 handle/offset/range/allocation-generation、阶段可见性。Q 沿真实 `root(false,k_quantum,1,0,3,1,1,0)`，从同 VkDevice/queue、原 Candidate bank=1 读为 BASE、Q 候选 bank=0、REPLAY bank=2，绑定读屏障复用原范围。真实 normal Q `permit[92]=0`，使用独立 250 ms grant/nonce，同 sourceSeq 1、把前一根的私有 `lineageNonce` 纳入原 base provenance/SHA，`commit=1`、输入 quantum tick 0、通过时输出推进至 tick 1。不得因 cold frame 跨进程拼接。旧 `acceptedNonce` 业务变量和对外语义不接受私有链。 [原参数与 permit 构建](../entry/src/main/cpp/gpu_resident/g13vulkan_boot/t833_candidate.inc:357)
3. **可查失败。** 保独立原 fence generation、origin 时钟和窗口新鲜度。Vulkan handle 数值可在销毁后复用（T981 projection 的原与冷拷贝 fence 实际相同数值），身份以 handle、寿命/代和 device/queue 校验。Q 重新提交前必须证明 SOURCE 内容与代仍持有，离线比较同一链 SOURCE CANDIDATE 与 Q BASE 全字。SOURCE 晚、被拒、旧代或丢 lease 则 Q 不提交。Q 原窗晚时仍可按当前 scope 真退役后只读冷档查首错，分别记录 grant/原 fence/冷拷贝 fence 与 (820) 逻辑、不为 Q 更换 lineage。三种实际终态可登记私有链及时、私有 Q 晚尾或首拒，业务 `acceptedRoot/typedToken` 全空。正式 promotion 要另采业务令牌与真实连续根，tester 先独立核负控，effector 只在 T989 包和 checker 齐时采实际链。

## 4. 目标 14 同一 VkDevice 的冻结真场切格

T981 的 T984 三臂已对解析预览守恒、镜像、载荷和实际 shader 上屏。真场首步复用其同一床夹具及切格/上屏结构，从 T981 通过的 16×16 circulation 私有根开始。给现有 G13 Win32 owner 增显式试验入口，在创建实际 device 时先查询并只在该入口启用 `shaderFloat64`。记录设备 UUID、驱动、实际 LUID、queue family、limits 和 enable 收据。T984 的独立 4 SSBO owner 可拆成借用 `VkDevice/queue/functions` 的模式，生产 G13/G14 独立入口保留。若同设备 stage 至少支持 5 个 storage bindings，新增只读 State binding；否则在新私有 WORK 版本保留三个命名不重叠范围，以同设备 GPU Copy 取 MAC 和压力原字后沿 4 binding。对私有源根分别记录 `compute shader-write→compute-read` 或 `compute→transfer→compute`，持有快照到最后一次 compute/graphics fence。先保当前 16×16 可见近似与独立解析臂；真场单独有身份。 [G13 PACK/lease 交接](../entry/src/main/cpp/gpu_resident/g13vulkan_boot/t833_candidate.inc:1006) · [G14 Vulkan 资源接缝](../pc/vulkan/sand/g14_boundary.cpp:124)

1. **先跑一个移动子区间。** T982 的 `[1/8,2/15] s`，固定、移动与镜像/零源对照都可复用。明确 `FROZEN_INPUT`，记录 G13 root nonce/原 fence generation、CANDIDATE generation、stateSHA、geometry/bed revision、时段、源模型物理时刻。读原 CANDIDATE `u/v` 从 word `256/528` 结束各 272，signed Q32.32 `π` 从 word 800；GPU 用 T982 `U0=.1m/s,P0=10Pa` 和单位。镜像试验要同时在 GPU 映射原场 `u'(i,j)=−u(16−i,j)`、`v'(i,j)=v(15−i,j)`、`π'(i,j)=π(15−i,j)`，仅镜像床时分别解释外场和边界，禁止套镜像总力对称性。真实 frozen 流体输入的当前角色不提供 30 个 G13 动力快照。
2. **保真正水量通量。** `CUT_CELLS/FACES` 存 `A0,A1,∫l_fdt` 和独立床扫掠 `S`。计算 `Q*=ū_n·l̄_f`，墙封闭，按开面图用 `Λ=Δt·l̄_f/(ρℓ)`、`DΛDᵀδp=S/Δt−DQ*` 校正。每个 wet component 验证兼容和固定规约、真实有效面连通及退化格状态。先在 GPU 做确定的 FP64 预览解，记录真实迭代次数、残差、首差、`Q*`、`Q`、压力修正与各分量；体积初诊门 `max|S−Δt DQ|≤10⁻¹² m²`，并核全局面积、闭面零流和移动/镜像的定向中线。若隐式校正达不到预览 gate，保原点和状态。T981 的实际 MAC 原字离线重算该湿图：移动/镜像 **147 湿格、268 活面**，移动逐格原缺口 **1.333×10⁻⁶ m²**，修正到 **4.798×10⁻¹⁸ m²**，中线移动/镜像 `∓5.787037×10⁻⁶ m²`。固定中线零；移动中的零场仍有同向排水。此为数学可行性和 verifier 对照，产品求解只在 GPU。 [冻结真场短段原件](../test/artifacts/t988/cut-evidence.json) · [离线复算](../test/artifacts/t988/cut_fixture.py)
3. **先证流场，再开放载荷。** 真场来自 `Q*` 与投影后 `Q` 的对照，输出当前湿部、床原形、修正速度和相对压。`P0·π/2³²` 与修正 `δp` 分账，缺规范压参考时显示相对载荷。梯度秩和壁速约束未核实前先标记 shear 缺证。要对合作用和反作用时从同片、同 time 积分实牵引与力矩，不能沿解析 `ψ` 的速度或常数粘性覆盖源，真场黏度按 T982 的标度独立记。画面由同一 G14 封存 wet/bed/flow SSBO 的 shader pass 使用；根/源/图形代必须同时匹配。记录图形 fence、imageIndex、Present、包 SHA 和床运动相应像素。至少交一个 moving 和固定/镜像或零对照，能判 `Q*` 与真实修正的区别后再接全 30 根。样例未达到的物理目标交 architect 首归因。 [T982 数据、单位、切格和画面接口](architecture-T982-v1.md)

T611 仍取正式可信 GridView，目标 12 继续等待已验收的目标 14 流场与边界载荷。T988 只做离线复算与合同，未运行设备或窗口。GPU 驻留，XComponent+shader 唯一上屏，禁止 Canvas。
