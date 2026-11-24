# T832 v1 · PC 沙水输入与双态合成工程接续

> 2026-11-24 修订：实际水侧扩散与飞包原件按 [T907](architecture-T907-water-v1.md) 接入。PC Demo 落沙可见门、接受角反馈、回正与负载对照按 [T908](architecture-T908-v1.md)。

2026-11-22 · architect · 目标 7 → 8 · Windows x64 Vulkan · PC 工程合同

## 1. 接入条件与交付边界

沿 T824 的 Win32 HWND、`VkSurfaceKHR`、swapchain 和同 GPU owner 接入，复用 FullM1 的 SOURCE / QUANTUM、经接受的 GPU publication 和 M1 shader 场景。T830 交付同一硬件设备上的真实产品窗口、至少一个已接受的非空业务根及可读原件，并给出实际输入、permit、transport、publication/lease 与 Present 的接口和版本后，业务实施可接入。以实际产物为准，工程准备与 T830 并行。若 T830 首根只覆盖干线，沿用其 Windows owner 和平台同步，新增显式 FullM1 profile 及其绑定，先核当前 adapter 对 M1 storage/vertex/fragment、格式、对齐和实际分配的能力。M0 reader 拒绝 M1，干线首根只证明公共平台链路。目标 7/8 的 M1 侧扩散沿既有库存物理，目标 13 → 14 → 12 的速度、载荷及高级冲刷合同另行执行。 [T824](architecture-T824-v1.md) · [T238](architecture-T238-gpu-water-M1-v1.md)

当前 PC 原件覆盖 T824 的 Intel UHD 630 Vulkan 硬件 BOOT，以及 T825/T831 的 ANGLE/D3D11 loader、离屏设施与负控。T831 已交五项负控和离屏设施，其 `WINDOW_PENDING` 仍随 T830 真实构建推进。历史 T280 FullM1 软件后端 1860 项断言、152 例独立重放和 562 项旧 SOURCE 回归，T281 软件场景 196 项断言和 20 项像素检查可作基线。T281 的像素专用运行冻结 `kappa=0`，只覆盖其 ARRIVE/退包等用例。当前缺 Windows Vulkan FullM1 真源、侧扩散和同根窗口原件。 [T825](test-report-T825-v1.md) · [T831](test-report-T831-v1.md) · [T280](../test/reports/t280/delivery.json) · [T281](../test/reports/t281/delivery.json)

## 2. 两种输入共享一个运行

首版选一套既有有效档 `K192/Kw128`、`features=0b0011`、M1 wire revision 2、128 列 × 256 模型高度、Q20、冷配置空跑。原 M1 适用的手控角域是 ±30°。冷配置冻结实际 layout、profile、shader、角、扩散及 optics 参数，留生成输入及实际 SPIR-V 的 SHA。换 profile、optics 或 reset 先关闭源，再建立新空 run。切换选源、停源保留同 run 的已接受状态。初版回归录制 `沙 Down → Move/hold → Up → 切水 → 水 Down → Move/hold → Up`，并以新 run 反序复验。 [原输入实现](../entry/src/main/ets/physics/GpuM1DemoController.ets) · [全根合同](developer-T280-full-m1-commit-v1.md)

1. UI 侧保一种活动指针。用客户区物理像素、实际 DPI、最近已呈现且当前代有效的容器 viewport 归一化为 `(u,v)`，普通模型坐标 `(128u, 256(1−v))`，原镜像标志有效时作一次 `(128(1−u), 256v)`。保原 `0≤x<128, 0≤y≤256` 边界，非法点拒绝，触点原样交 GPU 分流。几何、DPI 或窗代失配先关闭输入，等新代 Ready 和有效 viewport 后重新 Down。档案保原像素及转换结果。
2. 选项只改选源，Ready 下新 Down 才开 `SAND_H=0` 或 `WATER=1` 的源。切源、Up、Cancel、捕获或焦点丢失、越界均撤源并使旧源许可失效。未开始的旧请求按原级别取消，已提交根继续保留退役链，只有通过原撤权/截止/owner 接受门才可修改账本。停源后已接受 P、J、W 仍按 QUANTUM 演进。记录 QPC 原 ticks/frequency、同域 ns 和指针事件序号。原生产者以首有效帧样本锚定，零差无量，`0<Δns≤33,333,333` 才按配置 `rate×Δns/1e9` 产生 offer，逆时钟或长间隔关源，且无补发。物理固定 120 Hz，屏幕真实刷新率与送显节奏另计。采用 M1 V2 时间路径时，沿其绝对 250 ms job deadline 和原有效性检查。 [M1 源推进](developer-T228-gpu-sand-water-demo-v4.md) · [M1 V2 时钟](../entry/src/main/cpp/gpu_resident/m1/progress_clock_v2.h)
3. 调用原 128B LE `command32` 编码和去重。`GRSC=0x43535247`、revision 2，填源 kind、run、intent、input generation、source control、geometry、config、原始采样序号与时钟；原偏移 64/72/80 写 `x/y/requested` 的 binary64，保留位写零。Windows 的事件来源标识随证据保存，沿原 wire 时钟字段语义编码。本地 UI 提交 `ENQUEUED_NOT_ACK` 只证明入队。从真实 owner 保存 `ENQUEUED → ELIGIBLE → CLAIM_ATTEMPT → CLAIM_ACCEPTED → ROOT_STARTED → TERMINAL` 各实际到达阶段和具名拒绝，核同一 run/intent/content、源许可、root nonce、plan、版本及已接受 commit。原有界 GPU tail 每根正常 1024B，只有实际 fence 后 owner 校验成功的 terminal 给入量依据。原去重重放可以返初次 `admittedUnits`，commit 和新增库存增量为零。 [原源事件回执](developer-T597-source-witness.md) · [M1 wire 与重放](architecture-T238-gpu-water-M1-v1.md)

## 3. GPU 双账与侧扩散

- 源量由 GPU 转 Q20，保存 requested、admitted、rejected、原终态和出生/直接入水证据。接受沙源增加在途沙 P，LAND 使 P → 床 S。水在 `y<S` 走原床内拒绝，`S≤y≤η` 走 `DIRECT_W` 加 W，`y>η` 走 `FLIGHT_J` 占一活水包 slot、加 J；ARRIVE 由 J → W。两种类型各有独立来源量、ID/slot、generation 与完成证据。每列 `η=S+W`、`S+P+W+J≤256×2^20`，四库存非负。按接受根对 `Σ(S+P)` 和 `Σ(W+J)` 对账：外源新增才增来源账及 sourceCommit，QUANTUM 中 LAND/ARRIVE/边流只改分配，末尾重放和原子 bank/receipt/publication 检查沿 T280。失败根保留前缀，禁止由 UI 请求值推导库存。
- 保留两色 head2 流。`Δ2` 为正水头差，`q` 取原五界最小值：`floor(kappaW×Δ2/240)`、`floor(Δ2/4)`、给方 W、跨双方床槛的可达水量、收方剩余容量。接受的 `quantumSerial` 轮换首色，原 propose → prefix → apply 与事件次序保留；同一转移两端等量，冻结面介质用 W，ARRIVE 的新 W 从下一 quantum 开始参加 head2。目标 8 用原正 `kappa` 可扩散场景，分别累计窗内向左和向右真实边事件与列 W 变化；夹具选水平、中央入水且两侧可达的区间，逐步对照整数 oracle。边界接收满或有床槛沿原公式判定，左右单步无需相等。留同初态 `kappa=0` 对照，把 ARRIVE 增 W 和 WATER_EDGE 的列间转移分开。 [原算法](algorithm-T237-gpu-water-M1-v1.md) · [原 replay](test-report-T298-full-m1-independent-v1.md)

## 4. 同一接受根的窗口图像

`PcGpuOwner` 经现有接受门，从一个 publication lease 绑定 `sand16/water20/column16` 与对应 optics，保 `GRS_GPU_SCENE_Q20_M1_V2` 场景语义，顺序为背景 → 床 S → 活动沙 P → 水体 W → 真实 η 水面 → 实际飞行 J → 前景 → 合成/Present。Vulkan 保原整数顶点解释、真实 fragment storage 读、屏幕轴转换、线性 premult blend `ONE / ONE_MINUS_SRC_ALPHA` 及着色器与附件的单次 sRGB 编码。水体深度为 `clamp(η−max(y,S),0,W)`，W0 透明，干槽保持断开，J glyph 取 GPU 真实位置；ARRIVE 的接受帧消费更新后的 W/η，并剔除对应的活 J。像素用原场景输出和独立 oracle 对照，在原 T281 深水固定夹具复用其 2 LSB 判据，其余实际窗口用例保各自原判据与实测值。 [场景代码](../entry/src/main/cpp/gpu_resident/m1/scene.cpp) · [像素基线](developer-T281-m1-scene-route-v1.md)

为每帧记录 `adapter/device + window/surface/swapchain/geometry/config/optics 代 + run + acceptedRootNonce/commit + publication slot/generation + imageIndex/presentId(可用时)`，以及 lease acquire/release、GPU/WSI 原结果、时间和真实窗口或独立显示证据。一个图像只有一个被接受的物理快照，沙水的出生允许来自该运行内不同的较早 SOURCE 根。留实际 PACK → vertex/fragment、颜色附件 → 合成、acquire → submit → present 的依赖。退役时分别验证 publication GPU release fence 和 swapchain image 的呈现使用结束，旧代迟到只退役。无法完整画新帧时标 `STALE` 并保同代可用的上一完整 shader 图像，或给 `FAULT`。背景至前景均走原 XComponent+shader 语义及其 PC owner 映射，禁止 Canvas 和 CPU 物理、热路整池读回。新增生成 shader 逐编译单元及变体量计完整输入 ≤49,152 B，另记 SPIR-V bytes；需拆分则沿原 pass 依赖保审计、写集和同根可见性，重放核等价。 [T824 资源同步](architecture-T824-v1.md) · [原双槽交付](developer-T280-full-m1-commit-v1.md)

## 5. 验收原件与接力

| 目标或分支 | 同一实际窗口运行需保留的证据 |
|---|---|
| 7 · 空箱、两类实际源与共存 | 空 run 图像及零账，完整沙→水事件，两个独立非零已接受 SOURCE 终态；至少一帧以单一 lease 显示真实沙、水库存的空间对应。留沙 LAND 和水 FLIGHT_J/ARRIVE 序列；受控另测 DIRECT_W、床内拒绝及停源后真实落下。 |
| 8 · 切换、侧扩散和水质感 | 同一窗口产品复验水→沙与沙→水；核切换撤权和新 Down；正 kappa 两侧真实边流以及守恒，零 kappa 对照；用原 W0、干槽、深水、水面、真实 J 和 ARRIVE 前后窗口帧与像素探针判定物理及合成。 |
| 负控与资源 | 越界、跨几何旧许可、重复 intent、无新增根、晚 fence、占住 lease 与 resize；核原原因、无双记账、不换未接受 bank、无旧代 CURRENT；记录原始 VkResult 及旧有效帧/故障。覆盖不足逐项记 PENDING/FAULT 和缺项。 |

按 T826 机器台账的 `case` 复用：`sand/water mode switch` 和 `dual-state same-frame` 对应本合同的双源及沙水同根，`water lateral spread` 对应目标 8，`conservation invariant` 和 `ROOT/FULL receipt` 为横切证据。T826 临时数字与当前清单目标定义交叉，沙床＋活动沙同帧仍是目标 4，空容器输入/可控重力是目标 5；`surface transport` 夹具另用于目标 11 的实际通量效果。现有台账的 `FIXTURE_READY`、`T80_GATE_PROVEN` 可用于准备独立验证，PC 窗口结论取新运行原件。 [T826 台账](../test/reports/t826/t826-case-migration-ledger.json) · [当前清单](后续工作清单.md)

开发接手时先提交 T830 接口对应表及实际 FullM1 manifest，再按目标 7 的空箱与源账、沙水同根、目标 8 的边流和光学分段出真实窗口结果。任何前置缺项附已有构建/接口和所缺原始字段。独立 PC 检查沿 T831 设施接入，按 M1 V2 真实有效事件数、沙水库存和已接受终态判非空。T831 的 T236 分配容量图样只解释 T236 离屏原件。按当前 PC 帧率合同登记新物理帧、重显和屏幕能力，刷新率不足的首轮窗口只覆盖其实际显示域。 [PC 帧率合同](design-pc-effects-120fps-v1.md)
