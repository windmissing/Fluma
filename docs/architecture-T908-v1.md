# T908 · PC Demo 负载、堆积与重力响应归因 v1

2026-11-24 · architect · Windows Vulkan FullM1 · 离线。现场负载越大时的降速已复核，主要随**活跃在途物料**改变。全床静止时，即使库存更高，新接受根也明显回升。堆积同时受实收入料和偏慢的仿真时钟限制。正向重力输入已抵达同根 GPU 与窗口，现场的 +15°/+30° 被现有床形保护裁到 +2.609375°，且当前窗口没有实际重力指示。工程修订接续 T898/T832，保原物理、守恒、业务期限与 GPU 驻留。 [T898](architecture-T898-v1.md) · [T832](architecture-T832-v1.md)

2026-11-24 · T940 修订单一集成版。下文 y=16 只作明确选低位的测试夹具，真实输入用实画姿态逆映射。集成盒边、物料用同一接受姿态，屏幕重力箭头向下，以状态显示请求与实角差。下文固定盒的 (sinθ,cosθ) 箭头口径留全幅诊断。 [T940 集成版坐标合同](architecture-T940-v1.md)

## 1. 原件与计量

另一份同版普通 Demo 在高负载下进入退役等待超时并关闭入料。按真实根、容量和恢复链的交接见 [T925](architecture-T925-v1.md)。

用户原件 `.codex/local-test/demo-0317c9d04e5548728b4614c97277dd71/demo.jsonl` 是仍增长的日志。本次固定取前 58,000 行完整字节（76,657,893 B，SHA256 `4f22f28cea031083b81c682c3d0abc89fa5a3cef1d50118911bfa43d76f5ba00`）。0 s 为首次真实 Down 的 QPC `16396683084782`，时基 10 MHz。PID 20412，run `16395936734607`，geometry/config `2/2`，784×461，60 Hz。实际 Intel UHD 630（`8086:3e98`，Vulkan driver 原值 `1656924`，版本 `31.0.101.2140`）。现场进程 argv 使用 T899 A、`--m1-window`、`--m1-empty-preview --m1-manual-gravity --m1-one-quantum-grant --m1-dual-source`。本机 PE SHA256 `c09e87faae0e432a9cf67e717a24d229fce2bc72a075e5d9beb77c7dd777da79`，三模块 SHA 与日志分别吻合（`1912af9e…`、`b8f0e327…`、`caf0614c…`）。 [T899 工件](developer-T899-v1.md)

只按同 run+nonce 计真正 accepted SOURCE/QUANTUM。用各自的 completion rootKind/nonce、实际 fence、owner、authority 及 sourceCommit 去重。图像另核实际图形与 WSI 结果、publication slot/generation、physicsCommit 和 image index。采用 accepted Q 的 fence QPC 做半开窗口。联合收据有 471 行重复 `m1GravityGrant` 键，接受 Q 的角对象以嵌套 nonce 对上自身 rootKind 和 Completion256 word 107。共 8152 个接受 Q，原字 quantumSerial 从 1 到 8152 无间断，同根 Present 入队证据同数。实际可见与像素按各自原件裁定。根收支逐次通过原生成配额等式和 Q20 库存对照。

## 2. A · 负载上来后的推进

| 从首次 Down 起算 | 沙库存约（模型单位）与活沙中位/峰值 | Q/s · 仿真秒/墙钟秒 | Q submit 退出到成功 fence p50/p95 | 图形 submit 退出到 fence p50/p95 | 当窗拒绝 |
|---|---|---:|---:|---:|---|
| 0–20 s | 0.13→3.97，0/39 | 18.80 · 0.1567 | 38.37/87.22 ms | 0.45/2.40 ms | 1 GAP |
| 20–40 s | 4.07→7.86，70/83 | 4.45 · 0.0371 | 196.21/228.76 ms | 4.75/6.88 ms | 3 DEADLINE、3 LATE |
| 40–60 s | 7.86→15.53，29/97 | 6.90 · 0.0575 | 91.32/219.97 ms | 2.17/5.51 ms | 3 DEADLINE、2 LATE、1 GAP |
| 150–170 s | 25.12 恒定，0/0 | 25.70 · 0.2142 | 37.79/39.36 ms | 0.41/0.61 ms | 0 |
| 200–220 s | 32.49→38.21，43.5/76 | 7.00 · 0.0583 | 118.19/221.90 ms | 2.72/5.69 ms | 1 DEADLINE |
| 225–245 s | 38.21→38.33，0/5（倾斜阶段） | 21.60 · 0.1800 | 39.34/49.62 ms | 0.53/1.58 ms | 6 GAP |
| 300–320 s | 沙 38.33 恒定，活沙 0，活水峰 68 | 5.90 · 0.0492 | 127.96/162.48 ms | 4.62/6.53 ms | 0 |
| 400–420 s | 同一大沙床，沙/水活包均 0 | 23.85 · 0.1988 | 40.34/42.04 ms | 0.42/0.94 ms | 1 GAP |

- **已知的慢在何处。** 各窗口成功 Q 的录制 p50 约 0.18–0.26 ms，接受时 pending 均到现有上限 4。高活沙窗口相对静床约慢 5.8 倍。提交到 fence 的区间包含实际 GPU 命令、队列、屏障及 host 观察粒度。现场没有 Vulkan device timestamp，不能定量分摊这些成分。固定 127 个计算派发在轻重臂相同，主要累积负载嫌疑是实际运动/重放行数、库存审计和逐像素粒包循环。活水单独使路径变慢支持相同方向，后续同种子对照才判因果份额。
- **授权、退役与物理时钟。** 共 61 次失败退役，24 次 `CLOCK_GAP`、19 次 `LATE_FENCE_SIGNAL`、18 次 `ROOT_DEADLINE`。20–40 s 不跟 SOURCE 的 64 个成功 Q 拿到约 249.76 ms 首等预算，p50 仍为 210.65 ms。跟 SOURCE 共用一次新授权的 25 个成功 Q 首等预算 p50 为 213.26 ms。12 次 ROOT_DEADLINE 在首等预算超过 245 ms 时仍失败，例如原行 20643 的 249.729 ms。即使窗内零失败，静床也仅推进约 0.214 仿真秒/墙钟秒。末行配额账 `58188=8152 accepted+4 pending+0 active+49745 overload+287 revoked`。8152 个 Q 的实收时间为 67.933 s，首 Down 至末成功 fence 约 489.819 s。每个 Q 消费 1/120 s，未见重复或跳步推进。现象首先是实收吞吐不足，额度到顶以原策略记 overload。单根完整预算、共 grant 余量、pulse 断续、退役或 WSI 等待分别保原值。

### A 的整体实施方案

| 层 | 当前代价与这轮工程合同 |
|---|---|
| 库存、角域与排序 | 令 N 为列数，K 为两池容量和，A 为活盘/包数，R 为实际事件行数。现 `inventory()` 单 lane 按列扫描满池，约 `O(N·K+A·K)`，prepare 与 finish 都运行。K192/Kw128 单次列扫描就检查 40,960 个槽位。角域按 127 面求区间后仍串行扫 15,361 个角表项。127 派发中 92 次来自三套固定排序网络（256/128/128 的 36/28/28 层）。优先组合为稳定活索引和每列分段归约：一次 GPU 扫池产 compact、列归属及质量，合法/范围/来源逐包校核，确定排序后相邻核重复 ID，并保实际来源和 generation。列与全域用固定顺序整数宽归约，目标 `O(N+K+A)` 的库存归约，必要排序为确定宽度的 radix/prefix 或有界稳定桶。分别保运动 ID 顺序和 ARRIVE 的列、y、ID 顺序。角表若经原字全域验证单调，以两端二分找到同一合法区间，保 0 和 ±边界的 Q20 定点 tie。 |
| 运动、冲突与独立 replay | 保原固定步、全场字典、几何定点、LAND/BED_EDGE、两色水和三色床、物料来源及守恒。GPU 可以并行构造每个活包的候选及冲突范围，实际提交仍沿原顺序结算跨接收列、床高变化和原预算。仅在证明读写闭包互不干扰时并行，冲突段按原序执行，保第一错、全部原事件顺序、字节与拒绝回滚。保 accepted gate 的独立 spare replay 和全域核验。热点剩在 R 时按有界块核 R 及依赖深度，先证等价，再替换阶段。目标为 `O(N+K+A+R)` 数据扫描加排序和实际冲突链，最坏上界从原事件容量和实际依赖算。 |
| 画面、控制与退役 | 当前 fragment 对容器像素遍历飞沙、飞水包，保留满像素的成本近 `O(pixels×A)`。床/W 用按列 shader，粒/包改同代 compact 实例或有界按 tile bin 的 shader，把成本拉回 `O(pixels+可见覆盖+bin)`。保原沙遮挡顺序、水 alpha 及一次 sRGB，按 GPU lease 验像素差。拆开真实 pulse/owner 配额、compute 的 Q 提交和 30 fps 显示节奏，用同一许可账持续领真实 Q。可在下一可呈现周期内消费数个已接受 Q，按最后同代 accepted publication 呈现，保护两槽、fence 和实际 WSI 退役。无证书的 root 继续具名拒绝及上一有效 STALE 帧。用纯 Q fresh grant 与 SOURCE+Q 共 grant 对照，保原 `250 ms / 33,333,333 ns` 和无旧输入补发。 |

developer 先交同模块成对测量开关和 GPU 范围探针，再依次提交上述库存/排序、运动/replay、显示/授权的等价候选。每步可回到原版同输入，shader 完整编译输入沿原 49,152 B 单元限制。tester 用相同种子重放 0/32/64/96 活沙及无源高床、0/32/64 活水，分别开、关真实 SOURCE 和绘制，冷后热态各量。按实际相同 root 采 GPU 时间戳 `prepare/角域/排序/运动/床或水/replay/pack/graphics/copy`，关联队列及 workgroup、R/活包、CPU record/submit、retire/budget/Present。设纯 Q 和共 grant 交错臂，各臂先同输入、同 GPU/driver，失败仍留同 fence 晚退役原件。无 GPU query 或校准时继续报 host fence span。独显目标为用户实际场景持续 30 帧/s，另报新 accepted 画面、acceptedSimTime/wallTime、Q p50/p95/p99、最坏和停顿频率。开发集显报告正确性与诊断。 [当前 127 派发](../entry/src/main/cpp/gpu_resident/m1/quantum_passes.h) · [库存和角域](../entry/src/main/cpp/gpu_resident/m1/quantum_stages.glsl) · [现 PC shader](../pc/vulkan/m1_dry_scene.frag)

## 3. B · 入料、落床和可见堆积

现场前缀共实收 331 笔沙 SOURCE、40,191,818 Q20，水另 135 笔、16,163,157 Q20。五条独立 `M1_SOURCE_SUBMIT` 都是 `SOURCE_HOLD_REVOKED`，成功 SOURCE 多与后续 QUANTUM/WSI 写在同一行，必须从原 SOURCE nonce + Completion 计。T901 两套原字分别是 8.005 s 中 2 笔共 269,071 Q20（0.0321 模型单位/s），5.000 s 中 37 笔共 4,837,378 Q20（0.923 单位/s）。T901 报告文字的 3、38 各多计一笔；总量与原字一致。对照冻结位形、输入和授权条件后，才把改进份额归给单量子 grant。 [T901 原件目录](../test/reports/t901/)

| 现场持按 | 实收 SOURCE | 闭合的 LAND 质量与延迟 |
|---|---|---|
| hold 10017，13.238 s | 66 笔，8,040,925 Q20，约 0.579 单位/s | 与前后持按共享活粒，无法用活粒归零的端点隔离此 hold 的 LAND。 |
| hold 28859，6.963 s | 21 笔，2,545,223 Q20，约 0.349 单位/s | 原行 29791→31449，两个端点活沙均为 0，中间只包含该 hold 的实收。总 LAND 等于新增 2,545,223 Q20，12.768 s 均值 0.190 单位/s。松手时 31 个活沙，后约 5.762 墙钟秒、54 个实际 Q（0.450 s 物理）归零。 |
| hold 30401，14.576 s | 59 笔，7,724,264 Q20，约 0.505 单位/s | 原行 33048→40786，同样可用零活沙端点闭合。总 LAND 同量，23.169 s 均值 0.318 单位/s。松手时 92 个活沙，后 8.557 墙钟秒、72 个实际 Q（0.600 s 物理）归零。 |

全砂闭合时 `P=0`，当前 FullM1 的 LAND 把 P 转 S，BED_EDGE 只重分配 S，所以端点能证窗口总落床质量，缺每秒 LAND 时间分布和 per-column 可见堆高。首次零活沙帧原行 227 只有 809,873 Q20。即使全部挤在一列，以当前 461 px/256 模型高度算，其理论最大堆高也只有 1.39 px。后续实收变多后须取实际 128 列分布和窗口底部 ROI 定量。当前 `windowFrame.pixel` 只是单点，无法拟合堆高。当前 8 单位/s 是按每个合法相邻 pulse 间隔申报的速率，实际接受只覆盖这些间隔的一部分。基线优先减少入料资格与计算等待损失，同步保可见粒子与稳定床的独立阶段。

- **校验器缺陷。** 当前 `test/t900/check.py` 把 `Completion256[95]>0` 当 LAND 的门。writer 的 `W[25]` 只在 kind 9 BED_EDGE 时递增，LAND 是 kind 8，包含于粒子事件聚合。仅有 LAND 的真实根完全可能 word 95 为零。tester 改用 T907 的实际 `[eventStart,eventEnd)` 原字，解码 kind 8 及同根/slot/generation/来源，按 O(有效行数) 重放 S 前后与 P 前后。新增纯 LAND、只有 BED_EDGE、错代和动粒遮挡负控。现场每秒保同时段 requested、accepted SOURCE q 和 LAND q/count、实际入料受拒原因及留待落床的 P。
- **首版 Demo 可见门（本稿拟定，待实施实测）。** 水平空箱中心固定输入，名义窗口 784×461，实际 viewport 原件入档。4 s 内在注入点附近同一接受帧底部出现连续三列、每列物理床高至少 6 个物理像素（等效参考尺度，宽约 18 px），松手后低位标准夹具在 3 s 墙钟内结束可见落沙，后续至少 1 s 维持同一积沙。夹具首轮固定 `x=64,y=16` 模型单位，再对照实用户高落点，连续同力无重复 Down。6 px 在参考视口约 3.332 模型单位/列，三列的必要沉积下界为 10,481,211 Q20，4 s 即至少 2.50 单位/s，实际坡脚和散布还会多需入量。此门先用来判断交互吞吐需要改善多少，物理拟真另用高落点和原速率对照。优先改善真实接受覆盖与物理吞吐。必要时用版本化 Demo 低落点或入料配置，经原 cap、来源和守恒验证，保持默认物理对照。
- 同一 accepted publication 取 128 列 `S/P`、独立重放 `ΣS+ΣP=累计实际沙 admitted`，QUANTUM 全局总账不变，失败、撤权和重复 SOURCE 无新增。用 GPU 床和活动粒分层生成独立底部 ROI，记录 actual window/revision/viewport/image/lease、颜色格式和前后原像素。tester 按列高映射的边界容差 1 px、床颜色和连续列验证，实际沉积已达门而像素缺席才转 shader 映射。开发可复用 T907 的 M1PROOT1 与 ROI 双 pin 和事件原件。 [原事件 writer 与计数](../entry/src/main/cpp/gpu_resident/m1/quantum_replay.glsl) · [T900 checker](../test/t900/check.py) · [T907 同根取证](architecture-T907-water-v1.md)

## 4. C · 重力接收、床形保护和画面反馈

| 输入原行 / seq | 对应首个真实同根画面 | 接受角与实际阶段 |
|---|---|---|
| 44485 / 41524，+15° | 44487，nonce `16398933728803`，physicsCommit 3167，image 1，85.248 ms 后 Present 入队 | `+668/256°=+2.609375°`，根真实受理，`constrained=true`。仍有 4 个活沙，BED_EDGE 5 条。 |
| 44515 / 41525，+30° | 44517，nonce `16398947199313`，physicsCommit 3196，image 0，64.916 ms | 同一 +2.609375°，活沙为零、BED_EDGE 0、当帧实际重力约束依然生效。 |
| 44651 / 41537，0° | 44653，nonce `16399014067708`，physicsCommit 3307，49.939 ms | 实际回到 0°，该窗仍可能发生后续床流。 |

在用户操作序号 41524–41568 中，121 个正向 15° 和 104 个正向 30° 的实际根均为 +668，其中 44 个根有累计 75 条真实 BED_EDGE，表明这一段有床内搬运，仅凭单点及计数无法重建轮廓或归因某一条边事件。最接近一次主观“没动”的断点在物理角域。以 +15° 的原行 44487 为例，其 BASE 为原行 44486，接受角是 0，final `guardMin/Max=-557546/605394`。既有硬斜率保护 `|ΔS−tanQ20(accepted)|≤605394` 给合法 `tanQ20` 区间 `[0,47848]`。角表的 668 为 47787、669 为 47859，所以 668 正是此床可接受的最大正角。+30° 的相邻 BASE 原行 44516 给上界 47787，同样裁到 668。与此独立，软阈 `557537` 决定每面是否产生 `BED_EDGE`；已经接受的实际角仍可能没有搬运。平坦床在 +15° 的 `tanQ20=280965` 下就是一个静止对照。现 PC shader 没有角度指示层，同一个采样点在前述输入及多个后续帧保持相同原色值。 [设备角表](../entry/src/main/cpp/gpu_resident/angles_generated.h) · [硬角域和床流](../entry/src/main/cpp/gpu_resident/m1/quantum_stages.glsl) · [Win32 实际按键路径](../pc/vulkan/main.cpp)

1. developer 将接受根的 `inputSeq/requested/granted/acceptedAngle/limitReason` 连同 root/commit/slot/generation 放到单一类型化收据。GPU 产合法角上下界、限制面与耗尽原因，冷档留该面原 `S_L/S_R`。SOURCE/Q 放各自字段避免同名覆盖。UI 收到请求可以立即显示“待应用”。容器内实线箭头与数值只读同 lease 的接受角，沿 `+x` 向右、屏幕 `+y` 向下，由定点角表转成 `(sinθ,cosθ)`。受限时可见地给目标/实值和“床形限制”，角度显示保留一位小数。未接受或已退役失败保上一帧实际箭头及 STALE。
2. `R` 键与可点击的 shader 按钮“重力回正”发同一路有序绝对 0° 命令，另保 Home 兼容。按钮 hit-test 约束在实际 viewport/geometry 同代，与沙水输入区互斥。回正操作不扣库存或替换 run，保原 P/S/W/J。如果 0° 仍需裁限，状态显示限制及实际值，由后续接受物理推进恢复到 0° 或具名报告。在首次 SOURCE 前已空箱待键时，UI 仍可显示待接受输入，真实箭头待有效 GPU 发布更新。
3. tester 先做水平无源、缓坡无源、接近硬斜率的早期限角床、可显著迁移的独立支持床，各以完全相同 BASE 分叉 0/+15/+30/负角/回正，留前后连续 ROOT、第一有效帧、同根 shader 箭头像素和逐列体。无失效、33 ms 等级调度下初版交互目标为请求后 150 ms 内有实际状态反馈，出现 stale/受限也必须具体标明。床体是否应动由 oracle 按实际角、每面软阈、真实预算和 acceptedSimTime 分组。固定形变正控须有双方 S 库存等量变化、可重算 BED_EDGE q 和相应的图像变化。受限床的一像素或无位移属于可测结果，由该组 oracle 裁定。对于达到合法角、软阈有足额余量且仍缺输运的场景，先按原事件/像素分流；若设计自身无法给用户需要的大角度搬移，沿 G9/G15 设计切换做独立产品决策。 [G15 工程入口](architecture-T907-g15-g16-v1.md)

## 5. 接力与最小补证

developer 的优先序为类型化计量与重力反馈/回正、同根 LAND/ROI 接入、计算及呈现阶段优化。tester 可并行修正 T900 LAND 门和准备同初态负载/角域用例。实际 PC 采证交 effector。最小补证为同一源输入与 GPU 上的一对轻/重根设备时标（含失败退役与授时原字）、两个零活沙端点间完整 LAND 行及底部连续 ROI、同一个受限和一个可动初态的 ±输入和回正分叉。当前结论可直接用于工程实施，**GPU 各阶段份额、每秒逐笔 LAND 和真实画面堆高仍缺原件**。仅用 GPU shader 渲染，禁止 Canvas 回退。
