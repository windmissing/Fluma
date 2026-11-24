# T909 · Windows PC 开发回执

2026-11-24。按 [T908](architecture-T908-v1.md) 和 T907 的 [水线](architecture-T907-water-v1.md)、[G13](architecture-T907-g13-v1.md)、[G15/G16](architecture-T907-g15-g16-v1.md) 合同分项交付。

| 子项 | 已交付 | 剩余 |
| --- | --- | --- |
| ① 重力反馈 | `--m1-manual-gravity` 下标题显示请求、上一已呈现实角、待应用或床形限制。shader 箭头从同 lease 的 Completion 实角和原 Q20 角表取向量。R、Home 和右上角 R 按钮发 0° 请求。按钮与沙水触控互斥，按实际 client 和 geometry 判定。首个真实 QUANTUM 才给实际箭头。 | GPU 限制面、合法角上下边界和该面前态原字还未入单一收据。待连续受限、可动和回正 PC 复采。 |
| ② 堆积、水和图像 | `--m1-observe-series REQUEST PREFIX` 在 1–120 帧显式窗口采集，SOURCE 及 QUANTUM 实根沿原 `M1PROOT1` 复制，同 lease 取实际 WSI ROI 原字和哈希。修正 QUANTUM/REPLAY 冷档涉及的 transfer-source 用法。归档提取包含 LAND、ARRIVE、WATER_EDGE 原行及逐列 S/P/W/J/η。`--m1-observe-kappa-zero` 仅用于 FullM1 采证，启动时冻结。 | 当前像素位置由窗口矩形冻结。飞包可按 ID 和代从同根原字定位并核中心与 ROI，自动追踪 ROI、逐像素覆盖与 ARRIVE 后消失的独立验证仍待。 |
| ③ M1 阶段与库存 | `--m1-stage-timing` 暴露同模块上传、prepare、水、排序、运动、床流、到达、审计、独立 replay、pack、seal、尾拷贝原 tick，迟退役只作诊断。 `--m1-inventory-v1` 独立候选将列库存累计与活槽收集改成一次顺序扫池和一次扫列，保原有效范围、首错、来源核查与原 replay。 | 归约部分是 O(K+128)，活 ID 重复核查仍为 O(A²)，原排序与呈现照旧。GPU 图形时间、匹配轻重负载的实际占比、候选与旧版同输入原字等价均待独立 PC 采证。 |
| ④ G13 证据与裁剪 | `--g13-tail-timing` 增加 815–819 和最终 transfer 的拆点。 `--g13-seal-v1` 仅替换 QUANTUM 的 opcode 11，组内并行清 256 字及复制两份 255 字前缀，保持完整状态 SHA、两 scope、原首错、seal、850 派发和 250 ms 业务门。 | 实际收益与逐字等价待 PC 对照。完整 SHA 为链式计算，该候选保留原实现。active J/micro 求解热点及空 trial 的安全裁剪待细分量测和证据。 |
| ⑤ 大角度 | 23,041 行 ±45° Q20 角表双精度档独立生成并逐字相同。G15 搬移前的 ±45° 面支撑/静摩擦方向算子、G16 孤立盘平移壁及单调转壁首撞算子已编译，并交 Windows Vulkan 诊断宿主。 | G15 的 DM2 扩角 profile、连续微步姿态、面通量/来源、能量、typed accepted 和场景尚未接入。G16 尚缺 T843 连续姿态接入、多盘桶/邻接与真实接触步进，当前仅到 T910 孤立接触算子。 |

## 工件与复采入口

| 工件 | 仓库相对路径 | SHA-256 |
| --- | --- | --- |
| Win32 x64 Release | `.codex/local-test/t909-pc-ninja/hello_world_pc.exe` | `f742cf6a9ba0b422824c06d4ad76de54bd07cc078d90e17902d4abf7fbebc484` |
| 大角度算子宿主 x64 Release | `.codex/local-test/t909-dm2-release/t909_angle_probe.exe` | `1e5c8851efdeb774002257937d594fd211cd92d89fa7d0434d24824d68c5e21f` |
| M1 库存候选 | `.codex/local-test/t909-inventory/quantum_candidate_inventory_v1.spv` | `9b2ceaa612225e28e9f472ed59e53670b6edf2dbbd108e3b43bb8f38e01bc8ed` |
| G13 seal v1 | `t909-pc-ninja/g13_assets/PC_OP_00_9_11_SEAL_V1.spv` | `8561de2010e256371145d3070054064e6be362a708e21cb5745d240f1f3250ec` |

用 [观察脚本](../pc/vulkan/t909_observe.ps1) 给新输出目录、实际窗口像素 X/Y 矩形及首帧。脚本记录实际 argv 和模块校验值。输出三块横向相邻的 16×16 ROI。沙床在实际 client 底部约 16 px 的共同区域对齐后再跑水线。水线用同布局和入料分别执行默认和 `-KappaZero`。用 `python pc/vulkan/read_t909_observation.py <输出目录> [--packet ID GENERATION]` 提取收据。独立物理和像素判据沿 T910。

G13 使用 T901 F02 同一 CONFIG、STATE 和 `--g13-diagnostics hot --g13-slot-snapshot`。每臂另加 `--g13-tail-timing`，实验臂再加 `--g13-seal-v1`，按两臂交错记录实际 GPU、驱动、模块版本和同 fence 原字。冷档 dump 用新路径，保实际拒因和拒绝根。

```powershell
$p = '.codex/local-test/t909-angle'
$exe = '.codex/local-test/t909-dm2-release/t909_angle_probe.exe'
& $exe g15 "$p/g15_support_v1.spv" "$p/g15-fixture.bin" "$p/g15_angle_q20.i32le" "$p/g15-output.bin"
& $exe g16 "$p/g16_isolated_contact_v1.spv" "$p/g16-translate.bin" "$p/g16-translate-output.bin"
& $exe g16 "$p/g16_isolated_contact_v1.spv" "$p/g16-rotate.bin" "$p/g16-rotate-output.bin"
```

大角度表及模块可由 `pc/vulkan/build_t909_angle_probes.py` 重建，模块哈希在同目录 `receipt.json`。G15 输入为 LE `<iiII`（宽、角 Q8、摩擦 Q16、面数）后接有符号相邻床差，长度同单位。输出每面 LE `<qqiI`（驱动力分子、法向分子、方向、状态）。G16 输入 LE `<12fI`，输出 LE `<I8f`，字段次序在 shader 声明。现有三个输入文件依次是 T910 ±45° 静摩擦 fixture、平移壁和恒角速度壁孤立盘。宿主在单 fence 成功且 250 ms 内才写原字，收据保 `acceptedRoot:null`。

## 核证与交接

Windows 两条 Release 构建通过，新增 shader 完成 SPIR-V 验证，最大新增完整 shader 输入 24,056 B。G13 原 30 个模块 SHA 保持一致。现有 T910 离线自检通过，读取 T904 的 10 份已封存原档通过，PowerShell 和 Python 脚本语法通过。未取得本轮集显/目标独显 GPU 原件。effector 可先采①–④及孤立大角度算子；其余实现沿 T907/T908 由 developer 接续。滴答搜索 T909 无结果，本地保留回执。

## 续接交付

- Win32 新工件 `.codex/local-test/t909-r2-ninja/hello_world_pc.exe`，SHA-256 `d37e45b9f2a2832ab41b6141335b92bf5de24d51e66baa87558863997cb30124`。此前工件保留供 T912 对照。
- `--m1-angle-witness-v1` 配套 `.codex/local-test/t909-angle-witness/quantum_candidate_angle_witness_v1.spv`。该候选保留原角表与裁限判据，独立写 12 个 GPU 证据字，含 nonce、合法角上下界、两侧首个限制面、四个前态床高及切线界。面号 127 表示默认角表界。收据只在同根接受后给出，Completion 和独立 replay 兼容。与库存候选分臂，构建使用原 inventory 生成器加 `--angle-witness`。
- `--m1-observe-packet REQUEST PREFIX` 的请求文件含 `firstFrame count packetId generation` 四个整数，搭配 `--m1-dual-source`。根据已接受 publication 的位置定 8×8 包中心、近邻及底部 ROI，包离开后复采上一次位置。原件另记 packet 样本根、mass 和 FP32 字。包出现、同根和是否被遮挡由独立 checker 分别判。
- `--m1-stage-timing` 新增真实图形提交内 draw 起止和 copy 尾的设备原始 tick。收据绑定同根、publication 代及 graphics fence，缺点位保可用位和原 VkResult。
- `g15-pose` 使用 T843 一秒 quintic 的端点和 T907 的整数 1/7680 s 时间，GPU 两肢计算后 RNE 到 Q8，读取同一 ±45° 角表并写定点面支撑结果。静止/正反向端点及 85°/s 边界有具名输入范围。新诊断宿主 `.codex/local-test/t909-r2-dm2-release/t909_angle_probe.exe` 的 SHA-256 为 `fd76d1991562195954066a2ce1472684658a4eaffbf30d1d8988ea8dcbcfb781`。同上构建脚本输出 `g15_pose_v1.spv`。输入 LE `<iiiIII`（列宽、起止角 Q8、局部时间 tick、摩擦 Q16、面数）随后是有符号床差。输出每面 `<qqiIiI`（驱动力、法向、方向、状态、取样角、取样 tick）。

两条 Release 构建、SPIR-V 验证和 T910 回归通过。G15 连续姿态和面支撑已能合并诊断，DM2 物理事务中的搬移、来源与能量审计、typed 接受及场景仍待接入。G16 多盘扩域按清单等待 T912 孤立算子实测及接口回执。新增候选的端到端 PC 原件由 effector 复采。

## 连续姿态下的干沙面通量算子

`g15-flux` 复用 T843 的整数时间姿态和 T647 宽算术。按同一取样角用带符号等效床重构左右深度，再计算共享质量与动量通量、两侧压力修正。墙面的质量流归零，保留动量反力。静摩擦约束只在无已有水平动量的支撑面对质量流生效。计算前核坡面、承托和初态容量，核原整数宽度、0.45 CFL 和收窄。

用 `build_t909_angle_probes.py` 编译 `g15_flux_v1.spv`（SHA-256 `41d52e58b47e70afca62491a8c76fadb398419275852dd585939ab86615d1826`），再用 `t909_angle_probe.exe g15-flux MODULE INPUT ANGLES OUTPUT`。输入头 LE `<iiiIIIII`（列宽、段起止 Q8 角、局部 tick、摩擦 Q16、面数、重力、J），随后每面 `<IIIIqq`（左右床/流层高度与左右水平动量），最前和最后为墙。输出每面 64 B，字段次序见 shader。输出 status 0 为算子通过、1 为输入或算术拒绝、2 为支撑域拒绝、3 为 CFL 拒绝。拒根不消费动量和通量。

本算子只接收给定的预测或中点态，尚未接入原 DM2 的 predictor/corrector 调度、Hamilton 来源、能量账和 typed 接受。T912 已有孤立接触原字，G16 多盘仍缺完整接口回执与小域阶段独立判定。

## G15 面通量与逐来源搬移

`g15-origin` 用同一 Vulkan 命令缓冲和 fence 执行面通量与来源分账。两次派发之间通过 storage buffer 和 compute barrier 传递实际面数据。中点夹具的相邻面必须给出一致的列态。来源分账由事务入口的 F 库存和来源段出发，按实际可用量裁剪，供料列先左后右。Hamilton 余数相等时按原来源序。入流只参与最终列合并，不能参与同一微步的出流。两端记录同一有符号 q。每列检查 S/F/P 总占位、结果来源段数及合并质量，任一列失败时输出全零候选行和首错头。

- 宿主 `.codex/local-test/t909-r4-dm2-release/t909_angle_probe.exe` 的 SHA-256 为 `6c9151519d09f18703cd0d48f3e0ff749023fef5f79bcb18d2c003492201d464`。
- 新模块 `.codex/local-test/t909-r4-angle/g15_origin_v1.spv` 的 SHA-256 为 `e129edcbaa2380fc5b0a9c69f0cc696eedf6ce6be0ed0c47ca1defb71d0b6516`。同目录保编译收据与三列样本原件。

```powershell
$p = '.codex/local-test/t909-r4-angle'
$exe = '.codex/local-test/t909-r4-dm2-release/t909_angle_probe.exe'
& $exe g15-origin "$p/g15_flux_v1.spv" "$p/g15-origin-flux-input.bin" "$p/g15_angle_q20.i32le" "$p/g15_origin_v1.spv" "$p/g15-origin-base.bin" "$p/g15-origin-out.bin"
```

输入来源列每列 1040 B，头 LE `<IIII>` 为 `flow,occupied,count,reserved=0`，随后 64 个 `<IIII>` 为来源 ID 低字、来源 ID 高字、generation、质量。沿父流层定点库存，来源 ID 高字为零，非零来源按键升序。结果头 LE `<IIiI>` 为状态、首错列（成功为 0xffffffff）、取样 Q8 角、tick。状态 1 为入口账或布局、2 为面身份、3 为分账算术或质量、4 为占位、5 为来源段容量，257/258/259 依次透传面输入或算术、支撑、CFL 拒绝。每列 1040 B，头 LE `<IIii>` 是 F、新来源段数、左 q、右 q，随后同格式来源段。样本三列的预计算面 q 依次为 `0,-119,119,0`，F 为 `16503,130834,16503`。两个 119 依次按来源键分出 `60+59` 与 `59+60`，可用 T910 独立口径比对 GPU 原件。各模式仍报 `acceptedRoot:null`。

Windows DM2 Release 已构建，SPIR-V 验证通过，T910 离线自检和截短来源输入拒绝通过。开发交付限于中点输入到来源候选的连接。正式 DM2 profile 的动员、预测/中点构造、动量与能量账、typed 提交和 shader 场景仍待接入，开发 PC GPU 取证由 effector 执行。T912 尚未交 G16 孤立阶段正式报告与多盘接口回执。
