# T828 · 目标13纯流场 Windows PC 定量验收迁移

2026-11-22 · architect · P3 · PC 工程与 tester 交接

**先在 Windows Vulkan 从原 64×64 REST BOOT 走到可信场，再交目标14。** T824 已在 Intel UHD 630 真硬件上拿到原 OP65/66、fence 和读回正例。完整 CANDIDATE/REPLAY、非零场定量、真实接受、原冷档和 PC 场景仍需新工件。目标14 再依赖13的实际可信场，目标12再依赖14的实际载荷。 [T824 PC 基线](architecture-T824-v1.md) · [真实 BOOT 证据](architecture-T824-boot-evidence-v1.json) · [T721 原联核](test-report-T721-v1.md)

## 实施边界

| 顺序 | 工程接线与接受证据 |
| --- | --- |
| 设备和程序 | T686 纠正过的能力与精度分门继续用。Vulkan 用真实设备资源、队列、FloatControls 和实际 SPIR-V/反射，四 storage binding、layout UBO、64-lane、shared 和指定 profile 的 WORK 块下限。按原能力覆盖五 profile 时以 WORK 最大 2,331,392 B 为门。不得用历史的 GLES 非法 precision query 或顶点 FP 声明代替计算证据。所有实际稳态角色各过原 FP32 双轮 bit、数学资格和真实组布局，T824 的一次 BOOT 只覆盖该程序。 [T686](architecture-T686-v1.md) · [T690](developer-T690-v1.md) |
| 首个可信根 | 使用 T607 canonical source、操作表、每个原 SOURCE 16 次 submit、每个 QUANTUM 820 次 submit。保持 `ROOT_INIT → ACTUAL → REPLAY → SCOPE_COMPARE → PACK/AUDIT → COMPLETION`、原 J 选择和错误优先级。原 250 ms、pulse freshness 33,333,333 ns、期限与同根 permit 不放宽。PC 调度时钟用 QPC，脚本或人工动作均带同 scope 及实际 receipt。保 1024 B `Completion256`、GPU 证书、实际对象与绑定范围、完成校验和 accepted token。`CONFIG_PENDING`、未开始和 late fence 各记实际阶段，复用 T609 对无 accepted 场的判法。 [原工程与 dispatch](architecture-T607-g13-engineering-v1.md) · [T609 原判](test-report-T609-g13-quantify-v1.md) |
| 冷档与只读视图 | 复用 T607 的 Config128、Input128、NativePermit128、StateHeader256、Completion256、ColdHeader256、ColdDescriptor32 和 ROOT/FULL 的版本、目录、SHA 及原 little-endian 原字。后端和 QPC/PE 信息置外层 manifest。同 freeze/lease、真实 fence 后做 `device → bounded staging → archive`，按非 coherent 规则核 flush/invalidate。平场/速度/压力视图只消费同代已接受数据。PRIVATE_PROJECT 与故障冷前缀分别标记，只交其覆盖的诊断证据。 [原布局](architecture-T607-g13-layout-v1.json) · [原归档 reader](../tools/g13/read_archive.py) |

## 可追溯量、单位和误差

- **原字段**。每一轮绑定 `fixtureId/revision + PC可执行 SHA/BuildId 等价身份 + adapter/driver + pipeline/role SHA + config/seed/input/layout/math SHA + PID/start/owner/context/surface/geometry/permit/nonce/plan/commit + root/scope/acceptedInterval`。外层清单记 capture range、segment SHA 和实际退役。头、数组、样点位置与文件 bytes 可反查，不从截图重建物理。宽 ID 用十进制字符串。
- **速度／压力／黏性原值**。MAC 面速度 u/v 的原 FP32 words、面图索引、邻接与 ghost/alias 映射，格心动态压力 `π Q32`、`phi` 与规范参数，运动 `theta/omega/alpha`、黏度、墙面切向速度和壁剪切、真实 RHS/残差/证书/收支。缺原数组的病例明确缺失范围。供14消费的 `GridView` 保真实网格/坐标/边界、同间隔面速度、动态压力+势与实际压力规范。范围按 T607 原表，冷读继续只读该代的实际资源。 [G13 物理量和 GridView](algorithm-G13-pure-fluid-gpu-v1.md)
- **单位不换阈值**。`ν*=μSI·T0/(ρ0·L0²)`，`uSI=u*·L0/T0`，`pSI=[π+φ−mean(φ)+p_ref]·ρ0·(L0/T0)^2`。`p_ref` 是约定压力参考，必须显式带出。Config128 word18 是量化黏度。Completion256 黏性残差 104、散度 106、压力证书 108 是原槽。以声明域面积归一的 RMS 和 max 分开保存，两种单位都附标度；总压力、动态压力和离散求解器残差分别验。
- **原 T721 数值门**。物理 QUANTUM J∈{1,2,4}，散度 RMS/max ≤2⁻¹⁴/2⁻¹²，黏性残差 RMS/max ≤2⁻¹⁶/J、2⁻¹⁴/J，压力 RMS 证书 ≤2⁻¹⁶，投影速度证书 ≤2⁻¹⁶/J。压力证书含真实残差与 RHS 量化/兼容修正界。F02 的更紧判据、F03 的离散时间步比和收敛阶、F05 的物理压力偏差、F06/F07 的壁剪切/流量用各自 T631 有效版本。压力残差过门后仍需独立验物理场误差。 [独立修订与覆盖](test-report-T631-v1.md) · [T721 原字段和门槛](test-report-T721-v1.md)

## 迁移用例扫描顺序

| 原用例 | PC 取证与判定 |
| --- | --- |
| F01 / F02 | 64×64 静水/重力方向及 PRIVATE_PROJECT 拆分正控。F01 核真实速度及总压力，F02 核独立制造投影、物理动态压力分量、实际散度和该例更紧的证书。PRIVATE_PROJECT 只能给算子证据。 |
| F03 / F04 / F05 | 周期剪切阻尼、Taylor–Green 及非零闭盒 MMS／REST／RAMP_SPIN。按 T631 的参数和离散参考，分开速度、实际压力、黏性耗散、离散阶与分裂误差。F05 forcing 从声明解析输入构造，不能拿 GPU 当前输出反推。 |
| F06 / F07 | Couette 起动/稳态采用修正交替符号与 N128、t≥1/120 的有界级数，核实际速度和壁剪切。Poiseuille 采用原 `f_x=1/32、ν=1/64` 与 **Q=1/6**、半格修正和剪切。历史错误数值保留溯源。 |
| F08 / F09 / F10 / F12 | 记录真实受理的旋转时序、离心与科里奥利、均匀周期输运、外功/耗散；运动输入与每根实际 accepted 时间配对。 |
| F11 / F13 / F14 | 显式诊断构建核坏边界、零空间/RHS、alias、撤销/迟到/错代、错 Config/精度及无污染。支持性、首拒和未实现按原分开。 |
| F15 / F16 | 同受理 token 的归档、shader 场和真实窗口；压力图/速度图与场身份关联。同输入采计算/渲染时间、真实新帧、运行进度与内存，沿 T824 的 PC 性能结论。 |

Tester 先跑 `python test/t704/units.py`、T721 原 reader 和 `python tools/g13/read_archive.py <archive> --receipt <receipt>`。T721 四页沿原解码/anchor/布局继续用，Windows loader/队列/VkResult/PE 来源字段走显式平台分支，不能伪造 GL 原值。独立闭式/有界参考沿 T606 及 **T631 v2**，固定场样点、参考压力规范、度量分母、求解网格和时段，复用精确负控。当前单位自检 11 项已复核通过，完整 GPU 流场待运行。 [独立参考](test-report-T606-independent-baseline-v1.md) · [原 fixture 表](design-G13-pure-fluid-fixtures-v1.json) · [修正后的机器表](../test/t606/coverage-table.json)

## 14 和 12 的真实前置

**13 → 14** 取得同代 GPU 速度、物理压力、黏性对照和误差通过，连同版本化 `GridView`、实际可复套输入、边界/单位及证据范围交 T611。14 自己核给定沙轮廓的扫掠、开口面积/法向/壁速、边界压力/剪切、载荷冲量与功。现 G13 规则满盒的零空间和谱界不可沿用到任意切割单元。

**14 → 12** 等同代可信流场、实际边界载荷和独立原件后交 T612，接动员、冲刷、离床扬起、悬浮输运、再沉积及轮廓反馈，保守恒和载荷/反作用同账。原 2D 压力规范值不可直接解释成唯一的沙体有效应力。既有湿 M1 原件可按新合同评估复用。

T828 交付工程与判据。未修改产品逻辑或原阈值；PC 正式结果留待实现与独立测量。 [T607 原工程](architecture-T607-g13-engineering-v1.md) · [水线真实依赖](后续工作清单.md)
