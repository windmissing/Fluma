# T940 · 单一集成 Demo 的落点和世界重力合同

2026-11-24 · architect · Windows Vulkan · 离线。归因为宿主改写指针高度、实物画面使用请求角，以及局部重力箭头跨空间未转换。修订范围为集成版输入、帧绑定、SOURCE 反馈及 shader 呈现。原物理公式、合法角域和床形保护沿用。 [T933 工件](developer-T933-v1.md) · [T935 记录](../test/reports/t935/report.md)

## 1. 原件与首因

2026-11-24 · T950 复核。T942/T947 的中心 SOURCE 出生符合本稿原字门。报告的 108.4 px 取自旧粒子的 QUANTUM 运动后坐标。首粒像素与 ±20° 实际自由段仍需同身份取证，唯一入口继续待完整回归。 [T950 原件与补证合同](architecture-T950-v1.md)

- **出生。** T935 像素轮客户区 624×431、DPI 96、角 0°。原指针 (312,215) 对应像素中心 (312.5,215.5)，逆映射至 (64.14501160,32)，随后 CompactPointer 将 y 钳到 16。原行 284 的 SOURCE nonce 16709184332000，其命令字 16–19 解码为 (64.14501160,16)。第 289 行画面对应 Q 根的冷档，SOURCE 事件前缀有该 nonce 的 5 条真实出生、527,270 Q20。首粒 id54/gen1 的事件字 26/27 为 (67260919,16777216)，投回客户区是 (312.50,270.668)，比指针低 **55.168 个物理像素**。这是输入改写造成的偏移。 [指针映射](../pc/vulkan/m1_compact.h) · [SOURCE 出生字段](../entry/src/main/cpp/gpu_resident/m1/source_core.comp) · [同根冷档](../test/reports/t935/runs/pixel-round/root-16709184896788.m1root)
- **实画角与实际角。** T935 主轮 240 个非零请求的实际呈现 QUANTUM 全报 BED_SHAPE，173 根接受 −668/256°，67 根接受 +668/256°。实画最大差为 −30° 对 −2.609375°。第 12519、12639 行还各用旧 lease 重绘，实画 −5°、+10°，对应物理 −2.609375°、+2.609375°。Win32 回调读当时最新请求角，WSI 录制又读一次，两者也可互不相同。 [实际日志](../test/reports/t935/runs/integrated-round/demo.jsonl) · [WSI push](../pc/vulkan/wsi.cpp) · [旧 lease 重绘](../entry/src/main/cpp/gpu_resident/g13vulkan_boot/m1_source.inc)
- **物理和箭头。** 设 a 为接受角、b 为实画盒角。现有干沙加速度方向在盒内为 (sin a,−cos a)，经盒到屏幕的变换为 (sin(a−b),cos(a−b))。a=b 时向屏幕下。gravityIndicator 当前把 (sin a,cos a) 直接传给在屏幕像素绘制的箭头，漏了随盒姿态投影的步骤。粒子方向取决于两角是否一致，需用新包的无碰撞段原字验证。 [运动算子](../entry/src/main/cpp/gpu_resident/m1/quantum_motion.glsl) · [片元箭头和容器](../pc/vulkan/m1_dry_scene.frag)
- **堆积基线。** 旧像素轮第 50 帧 Down→同根 Present 实测 **1.5858371 s**，三个 ROI 沙色像素为 4/9/0，连续三列原字最大床高约 2.29 px。证据覆盖旧低喷口的早期堆积；T908 三列各 6 px 的量化门单列。主轮旧吞吐 19.3 Q/s 用于对照。 [像素日志](../test/reports/t935/runs/pixel-round/demo.jsonl) · [原像素口径](architecture-T908-v1.md)

## 2. 指针以实际呈现帧为准

1. **共用帧变换。** Owner 录制本帧时冻结一个 CompactView，含 run/geometry/window、swapchain、actual client px/DPI、image extent/viewport、有效区域、中心/scale、实画角与 (c,s)、请求序号/角、Q 的角/nonce/commit/slot/publication generation。PresentBoot 用它生成 push，图形 fence 和成功或可用的 WSI Present 后，把实际帧和变换回传 Window。每帧有独立 serial；transformRevision 仅在映射变化时更新。窗口重建或几何失效时清除旧变换。PresentQueued 与 fenced 像素分列。
2. **坐标规则。** Win32 客户区整数事件取像素中心 (x+.5,y+.5)。现进程申请 PerMonitorV2，当前客户区尺寸与实际图像相符时直接使用物理 px，避免二次 DPI 缩放。尺寸、有效区域、surface transform 不同时，消费随帧登记的客户区→viewport 映射，映射未知时反馈「画面更新中」并暂停入料。按钮使用同一画面定位。取屏幕 +x 向右、+y 向下，盒内 +y 向上。令 d=(pixel−viewportCenter)/k，k=.008×min(viewWidth,viewHeight)，局部 p=(64+c·d.x+s·d.y,32+s·d.x−c·d.y)。回投用同矩阵的逆，按实际浮点系数精确复算时除以 c²+s²。保 [24,104)×[0,64) 半开放域，合法 p 直接作为 SOURCE 的 binary64 输入，保原 Q20 floor。移除集成路径的 y=16 钳制。
3. **消费身份。** PointerSample/SourceOffer 绑定原回调、hold、scope 和 transformRevision；SOURCE 的 grant、submit、最终 accept 用已有授权门检查。静止持按沿同一变换消费真实相邻脉冲。实画变换改变时撤回旧未消费 offer，向仍捕获的 UI 取一次新帧下的真实客户区位置和实际 QPC，重置脉冲锚后按新的相邻脉冲生成预约。已提交旧 offer 按同 fence 退役；不回填时钟、补发或更改冻结命令。resize/DPI/最小化沿换代撤旧许可，新几何需同代帧、READY 和新 Down。
4. **不可出生反馈。** 板外沿现规则取消持按并提示新 Down；合法盒内命中床下或槽/列已满，以 SOURCE 实际 GPU 首因反馈「床面遮挡」或「入料容量已满」。有效拒收可更新内部去重账，实收入料、出生、物理时间和质量均无此次增量。预览分支区分业务拒收与传输失败，保原 QUANTUM 路径，后续合法位置可继续；反馈只消费当前 hold/几何的对应结果。用现有状态显示或同 shader 提示。主机诊断扩展留在帧/输入收据，SOURCE 32 字协议保持。

**出生判据。** 独立回投实际 SOURCE 事件字 26/27，与对应指针像素中心误差 **≤1 物理 px**。首个含同 id/gen 的 fenced 帧用该帧真实 publication 的位置投影，像素中心误差 **≤2 px**；空盒固定点在规定 viewport 的首个可见粒中心还须距指针 **≤3 px**。若分频跳过第一张近指针画面，优先调度第一张可用同根帧，再继续原节奏。拖动逐笔核真实输入，后续物理位移与出生分开比较。ROI 的全幅 x/128、y/256 路由只用于全幅诊断；集成模式沿实际帧变换、lease 和像素格式采点。 [现 ROI 接口](../entry/src/main/cpp/gpu_resident/g13vulkan_boot/m1_source.inc) · [冷档提取](../pc/vulkan/read_t909_observation.py)

## 3. 盒体、箭头与状态

- **已发布。** 从 pin 的 QUANTUM lease / Completion256 word 107 取实角 a，令 boxAngleDrawn=a，盒框、床和粒子共用。SOURCE 的 grant 不代表新姿态。停止用旧 lease 按新请求角重绘实物；允许旧 lease 同姿态更新 UI，收据保 baseRoot 且无新物理步、入料或 CURRENT 晋升。标题显示请求/实角，用 input seq、根本身的 grant 和 limit 原因区分「待应用」「床形限制」「角域上限」「上一有效帧」。R/Home/按钮目标相同，实物由同样的实际接受链回正。
- **空盒。** READY 首帧默认水平且 acceptedRoot=null。转动时先显示待应用；实际转动复用已由预览 fence 初始化的 GPU 空 BASE，取真实 QUANTUM-only grant 取得接受角，再同根呈现。普通水平首次入料沿既有 SOURCE→Q，避免给所有点击增加启动根。空盒等待或失败保原帧，真实新脉冲后继续；已发布失败保标记 STALE 的原角/原根。实角因床形限制停在约 2.61° 时，实物保这一角，状态另显示实际请求及限制。
- **世界重力。** 集成版箭头按屏幕向量 Q20 (0,1048576) 绘制，空盒、已发布及受限状态都向屏幕下。运动沿原角表、局部 g·(sin a,−cos a) 及现有 Q20 规则。正角的盒 +x 在屏幕上顺时针向下，负角镜像。实际世界运动以固定已接受角的同粒连续自由段验证。转角时对每帧各自做坐标转换，纯画面旋转无资格充当速度变化。

## 4. 实施与复测

| 范围 | 复测矩阵与判据 |
|---|---|
| **出生与 DPI** | 624×431、784×461、另一宽高比，100%/150% 及跨 DPI 窗口。水平及正负可达角，空盒和有床，低中高、中心/左右、拖动和边界。每项关联 pointer seq/QPC、offer、SOURCE 命令和事件原字、同粒 Q 及像素。板外、床下、槽/列满、换代、实画角切换和迟 fence 有具名负控。 |
| **箭头与实际物理** | 空盒和已发布分别做 0°→+15°→+30°→反向过零→−15°/−30°→回正，R/Home/按钮；可达角正控和 BED_SHAPE 受限床，另测重复请求、角域限位、pending、STALE。连续自由段用原事件位置/速度及实际时间独立复算，世界加速度方向误差 ≤0.1°；箭头做独立像素判据，轴线横偏 ≤1 px 且箭头朝下。负控分别注入请求角误用和局部箭头误用。 |
| **同包体验** | 单一 Start Demo.cmd 与固定 manifest。低位 y≈16 明确点击作旧基线，对照默认中心 y≈32 与高位 y≈56。真实 Down→首达、同根像素及床高分别报告，默认中心 **4 s 内**明显见堆，逐列报告 T908 像素条件。复验松手沉降留存、60 秒后真实 SOURCE 和 Q、停后再出沙、LAND/S/P/来源守恒、转盒和复位。实际 GPU、驱动、shader、真实窗口和时间全入档。 |

去掉钳制后，32/56 高度、g=512 的水平空盒理想下落约需 0.354/0.468 仿真秒；按旧 19.3 Q/s 推算为约 2.20/2.91 墙钟秒，另计源等待及积沙。该估算用来选择复测点。中心见堆退化时，先沿当前分频和长程优化量真实接受覆盖、活动粒与 Q/s，保真实出生、重力、1/120 s 及 250 ms 门。

Developer 改 m1_compact.h、main.cpp、m1_ingress.h、boot.h/m1_source.inc、wsi.h/.cpp 和 m1_dry_scene.frag 的集成分支，保 GPU 驻留、shader 渲染、49,152 B 完整 shader 源码门，禁止 Canvas 回退。Tester 接独立坐标、Q20、拒因、姿态/ROI checker；effector 用锁定同版复采矩阵。离线复核已覆盖原日志、M1PROOT1 与两条旧 lease 重绘，另算 5 种 viewport×7 角的 1,400 点，Q20 量化回投最大误差约 0.000010 px。新包、实际 DPI/倾斜粒子和高位堆积待实证。
