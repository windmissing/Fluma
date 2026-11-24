# T912 · PC Demo 重力堆积、GPU 候选与大角度算子实测（effector）

2026-11-24。按 [T909 开发回执](../../docs/developer-T909-v1.md) 的工件、[T910 独立判据](../../test/t910/handoff.md) 与原版对照，在开发集显 PC 实测。全程 GPU 驻留 Vulkan、shader 渲染，无 Canvas 回退。真机不涉及。

- **PE**：`.codex/local-test/t909-pc-ninja/hello_world_pc.exe`（SHA256 `f742cf6a…ec484`，T909 工件）；模块 `.codex/local-test/t830-m1-bounded/`（candidate `b8f0e327…`、commit `caf0614c…`、source `1912af9e…`）
- **GPU**：Intel UHD 630（`8086:3e98`），Vulkan driver `31.0.101.2140`（driverVersion 1656924），窗口 784×461@60Hz
- **注入**：`test/t912/drive_t912.py`——真实 WM_KEYDOWN/WM_CHAR（方向键/Home/R）、WM_APP+3 按键路径、PostMessage 按压（含周期 WM_MOUSEMOVE）、复位按钮 WM_LBUTTONDOWN/UP（hit 区 `[w-42,w-8)×[8,38)`）；交互脚本 `test/t912/run_g13.py` 为 g13 臂提供 stdout 落盘
- **原件**：`test/reports/t912/runs/`（各场景 demo.jsonl + argv.json + M1PROOT1 档案）；探测输出 `test/reports/t912/outputs/`

## ① Demo 重力反馈（空床/受限床/可动床）

**空床（1-gravity-empty）**：窗口仅空预览帧（`M1_EMPTY_PRESENT_QUEUED`→`EMPTY_PRESENT_RETIRED`），9 笔输入全部 `ACCEPTED_INPUT`：R(0)、+15、+30、R(0)、Home(0)、−15、−30、按钮(0)×2。无沙输入（WIN32_POINTER 0 笔）——**按钮不出沙实测通过**；±30° 边界由 T895 已证（9999/−10023 拒绝），本卡未见 `ANGLE_OUT_OF_RANGE` 因输入未越界。

**可动床（1c-gravity-mobile-bed，6s 持按建床后连续输入）**：`m1GravityFrame` 同根收据 428 帧，请求→实角全部同根留档：

| 输入 | granted | 实际角（范围） | limitReason |
|---|---|---|---|
| +15°（持按中） | 3840 | 3840 | null |
| +30°（持按中） | 7680 | 6182→7385（床形演进） | **BED_SHAPE** |
| R 键复位（持按中） | 0 | 0 | null |
| +15°（UP 后） | 3840 | 838→3840（床逐步放松） | BED_SHAPE→null |
| 屏上按钮复位（UP 后） | 0 | 0 | null |

- **床形限角反馈实测成立**：+30° 请求被裁到 +24.15°→+28.85° 区间并标 `BED_SHAPE`；松手后 +15° 先裁到 +3.27° 再随床松弛回到 +15°。
- **R 键、Home 键、屏上 R 按钮三条复位路径均回到 0°**；按钮点击仅产生 `M1_GRAVITY_INPUT`，不产生沙水触控（pointer 行只有建床按压的 DOWN/UP/CAPTURE_LOST）。
- **空床"待应用"语义**：空床无接受根，重力输入留 `ACCEPTED_INPUT` 但无 `m1GravityFrame`（箭头待首根），与 T909 设计一致。
- 单点像素（col64 底部）：空/建床前 `(82,83,85)` 边框色 → 建床后 `(181,151,111)` 沙色，窗口真实呈现。
- **缺证**：窗口标题栏"请求/实际/待应用/床形限制"文本在日志中无字段（m1GravityFrame 已含等价信息）；受限面上界原字（S_L/S_R 前态）未入单一收据——沿 T909 剩余项归 developer。

## ② 长按入料、逐笔 LAND、底部 ROI 与 4 秒门（2a-sand-observe）

8s 持按（`--m1-observe-series` 自首帧 120 帧、底部三 16×16 ROI 列 63–65）：

- **受理**：31 笔 SOURCE 归档（rootKind=1），共 2,064,374 Q20 = **1.97 模型单位**；逐秒入料 t=0s 0.73u / t=1s 0.76u / t=2s 0.49u，之后**断流**。
- **断流根因（新发现，候选缺陷）**：t≈2.69s 一次 `CLOCK_GAP`（session 1→2，balanced=true）后，恢复的时钟会话重开，但 host 侧 sourceAnchor 仍持有旧会话脉冲——`AdjacentSourcePulses` 要求 `session` 相等，**持按剩余 5.3s 内 offer 恒为 0，入料不再恢复**。复现：本卡 8s 持按仅得 1.97u；对照无观察的 4s 持按（1b）得 3.27u 未中断。建议归 developer（恢复路径重置 sourceAnchor 或按会话键控）。
- **逐笔 LAND**：归档根内 54 笔 kind-8 LAND、65 笔 kind-9 BED_EDGE；每笔 `q=w22−w23>0` 且 `w33−w32==q`、`w34−w35==q` 全部通过（0 笔坏账），LAND 总质量 2,064,374 Q20 与 S 库存增量一致；松手后 2.61s 活沙归零（117 帧非零→0），库存留存不变。
- **底部 ROI 与 4 秒门未达**，量化原因三叠加：
  1. 入料有效速率 0.73u/s（时钟存活期）≪ 门需 2.50u/s；
  2. 仿真时钟仅 ~0.16 s/s（85 帧/12.9s），4s 墙钟仅 0.64 仿真秒；
  3. CLOCK_GAP 断流（上条）使 8s 持按只等效 2.7s。
  - 即使全部 1.97u 集中三列：理论最大床高 1.97/3×461/256 ≈ **1.18px** ≪ 6px 门。实测 S64 峰值床高 1.81px，ROI 中沙色像素首现于 t=6.03s（4px），峰值 42/768px——4 秒见堆在本集显与当前受理链下结构性不可达，与 T908 A 归因一致。
- **同根 S/P/ROI**：`read_t909_observation.py` 全 120 帧同根绑定通过（run/geometry/root/slot/generation/imageIndex/fence 全对），`firstMissingArchive=null`；`root-events-summary.json` 落盘逐根 LAND/BED_EDGE/质量。

## ②-续 水默认 vs kappa-zero（2b/2c）

同布局 8s 持按水（material=1，M1_SOURCE_MODE oldMaterial 0→1 各 1 笔）：

| 项 | 默认 kappa=24 | kappa-zero |
|---|---|---|
| 入料 | 44 笔 SOURCE，5,494,544 Q20 = 5.25u | 48 笔，6,751,972 Q20 = 6.44u |
| ARRIVE(11) | 44 | 48 |
| **WATER_EDGE(10)** | **2,478** | **0** |
| WATER_MOVE(4) | 2,024 | 2,130 |
| 末态 W 分布 | **53 列**（38–90） | **1 列**（col64） |
| 末态 eta(col64) | 0.43u ≈ 0.78px | 同量级单列更高 |

- **kappa 项决定横向铺展**：默认臂 53 列铺开且 2,478 笔边事件，kappa-zero 臂全部质量停在落点单列、零边事件——T907 水线合同的单参数归因在真机上成立。
- **同根像素**：两臂各 120 帧同根 ROI 绑定通过；底部探针点 (394,460) 处水膜 alpha≈1−exp(−0.03×0.43)≈1.3%，像素为边框/微混色（默认 (85,83,82)、kappa-zero (95,90,84) BGRA）——**当前入料速率下水不可见**，与沙线同一根因（速率/时钟），非渲染缺陷。
- **剩余飞包单列追踪**：`--m1-observe-packet` 需要工作树中未提交的重建（t909 发布 exe 无此旗标，usage 核实），本卡不可采——列缺项，归 developer 合入后接测。

## ③ M1 基线 vs inventory-v1 配对时标（3a/3b）

同场景 6s 持按 + `--m1-stage-timing`，两臂各 121/152 个接受根带 GPU 时间戳（36bit/83.333ns，全点可用）：

| 阶段（p50 ms） | 基线 | inventory-v1 | Δ |
|---|---:|---:|---:|
| UPLOAD(同模块上传) | 22.61 | **4.32** | −81% |
| SORT(排序，未改) | 20.67 | 22.61 | ≈0 |
| ARRIVE(到达) | 18.61 | **0.99** | −95% |
| FINAL_AUDIT | 23.66 | 25.15 | ≈0 |
| REPLAY_BEGIN | 38.19 | 40.40 | ≈0 |
| 总和(根→尾拷贝) | 128.3 | **97.6** | −24% |

- p95/max 同向：基线 162.7/167.9ms，候选 128.0/134.5ms。**收益集中在 T909 声明的库存/到达扫描折叠（UPLOAD+ARRIVE −41ms）**；排序、审计、replay 未动，符合"仅替换 inventory() 单 lane 满池扫描"的改动面。
- **等价性缺证**：两次独立运行入料时序不同（同 physicsCommit 集 112 个，sourceCommit 70 个错位、活跃沙逐帧有别），活体交互无法做同输入逐字对照——沿 T909 已登记的"同输入原字等价待采"缺口，需确定性重放路径（developer）。
- 迟退役诊断：3b 含 1 次 `M1_STALE_WAIT_REAL_PULSE`+`M1_READY_PHYSICS` 自主恢复（会话 1→2，balanced），`M1_QUANTUM_PRESENT_RETIRED` 两臂均达；granted 额度/迟到 `lateSupervisor` 原字保留于各 demo.jsonl。

## ③-续 G13 seal-v1 配对——受阻（原始错误与复现）

**受阻项**：`--g13-tail-timing`/`--g13-seal-v1` 配对（815–819 拆点、尾拷贝、原业务接受、迟退役诊断）。

- **复现**（4/4 次确定性）：t909 exe + F02（T901 同一 CONFIG `295c075e…`/STATE `e1c6b49e…`）+ `--g13-diagnostics hot --g13-slot-snapshot [--g13-tail-timing]`，启动后 0.17s 内完成 99 行（OP_00_0_5→OP_18 全 QUALIFIED），随后**卡死在 `vkCreateComputePipelines ENTER, role=OP_48`**（moduleSHA `f01959cc…`，layoutSHA `b9390359…`，pipelineCache=null）；240s/420s/400s/870s 四种预算全部无进展，stderr 空。
- **对照**：同一机器/驱动/场件/模块 SHA，`t890-observation-timing-release.exe`（1e49f2a6…）完成全 181 行，OP_48 编译 1.036s，到 `LATE_RETIRED_DIAGNOSTIC`（250ms 拒绝、businessRejected、18/18 点位）——**排除机器/驱动/场件/模块因素，定位为 t909 exe 构建触发的驱动侧管线编译挂起**。
- **已试**：开/关 `--g13-tail-timing`；可见/隐藏窗口；管道/文件两种 stdout 排水（非排水阻塞，文件句柄直写同样 99 行停）；二次运行（驱动盘缓存暖化假设排除）。
- **停止原因/前置**：无该 exe 源码级差异定位（链接产物差异在 OP_48 编译行为上的驱动级表现），接手角色 **developer**（对比 t890/t909 构建的设备初始化/管线缓存路径；或用 t909 源重出一个 A/B 构建二分）。
- 证据：`runs/g13-stall-evidence.jsonl`（99 行停点）、`runs/g13-stall-summary.json`、对照 `runs/g13-control-t890.jsonl`。

## ④ G15 支撑静摩擦 / G16 孤立接触（对照 T910）

`t909_angle_probe.exe`（`1e5c8851…`）三输入在 Intel UHD 630 上单 fence 全部成功且在 250ms 内（G15 19.5ms；G16 平移 4.3ms；G16 旋转 1.9ms；模块 SHA 与角度表 SHA 全部预检通过）：

- **G15（±45° 整数支撑/静摩擦方向）**：输出 `drive=74145500 normal=74145500 direction=1 state=0`，与解析模型（角表 projection[23040]=(741455,741455)，drive·65536>μS·normal）**逐字相等**；墙面零通量/来源正负控未触发（fixture 单面）。
- **G16 平移壁（toward）**：`status=1, toi=0.0619225502, impulse=0.0155641241, velocity=0.1707460284, wallWork=0.0015564124`——与 T910 `isolated()` 解析值偏差 ≤3e-9（fp32 精度内），相对速度 −0.7075→+0.0707（e=0.1 反弹）通过。
- **G16 旋转壁**：`toi=0.1789547205`，bracket `[0.1789547056, 0.1789547205]`（宽 1.5e-8）；T910 48-iter 解析 0.1788742959（24-iter 半宽差 8e-5，在 shader 的 24 次二分精度内）；`impulse=0.0005918` 与解析 0.0005920 一致，首向内根夹区间成立。
- **明确边界**：算子通过 ≠ 连续产品场景通过——G15 仅孤立 ±45° 单面支撑判据，未接 DM2 扩角 profile/连续微步姿态（T909 未合入）；G16 仅孤立盘平移/恒角速壁首撞，未接 T843 连续姿态、多盘桶/邻接与真实接触步进。归 developer 按 T907 合同接续。

## 结论与缺项清单

| 项 | 状态 |
|---|---|
| ① 重力反馈三床配对、限角/复位/按钮不出沙 | **完成**；限角面原字收据归 developer |
| ② 沙线长按/LAND/同根 ROI + 4s 门归因 | **完成**（门未达且已量化三根因）；CLOCK_GAP 断流为**新候选缺陷**报 developer |
| ②-续 水默认 vs kappa-zero、ARRIVE/WATER_EDGE/同根像素 | **完成**；飞包单列追踪缺 `--m1-observe-packet` 发布件，归 developer |
| ③ M1 基线 vs inventory-v1 阶段时标 | **完成**（−24% 总时长，收益在 UPLOAD/ARRIVE）；同输入等价缺证沿 T909 待确定性重放 |
| ③-续 G13 seal-v1/tail-timing 配对 | **受阻**：t909 exe OP_48 管线编译确定性挂起（4/4），t890 对照排除环境；交 developer |
| ④ G15/G16 孤立算子对照 T910 | **完成**；连续场景接入按 T907 合同归 developer |

实际持续 30fps 性能按卡面约定交用户独显验证；本卡仅集显正确性与诊断。GPU 驻留、shader 渲染全程无 Canvas 回退。
