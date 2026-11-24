# T931 · 长程掉沙、G15 建管线和 OFF 同输入工程修订

2026-11-24 · architect · Windows PC Vulkan · 离线。T927 的长程负例来自旧 127 派发紧凑 shader。35 派发候选短窗的设备时间优先指向独立 replay、实际碰撞运动和前置审计。G15 opcode 23 与曾快速返回的 DM2 模块只差一条非语义源码扩展声明，加载前缀另有变化。自然 OFF 两臂的物理输入已经分叉，下一步应复用一次真实采样做私有诊断。 [原始证据复算](architecture-T931-evidence-v1.json) · [T927 报告](../test/reports/t927/report.md)

## 1. 长程推进与入料

### 实际版本和边界

- `t926-longrun`、`t926-recovery` 和 `t926-pile-pixels` 均为 T926 PE 配 **T918 紧凑三模块**，单量子 grant，所有 Q 127 派发。三者未请求阶段 GPU 时标。同一 PE 的 `t926-fastcandidate` 才是 35 派发、单量子 grant、阶段时标的 10 s 短窗。短窗 parity 则使用 cap=4，分别 127/35 派发。四者的实物与测试目的须各自登记。
- 长程以首 Down 起算，脚本报告 175.2 s，最后拒根业务 stop 为 175.459 s。按 run、nonce、实际提交 VkResult 和 Completion256 去重，**558 根实际 Q 提交，接受 25 根、总物理推进 0.2083 s**。最后接受 Q 在 **5.851 s**（原行 806，活沙 113）。26 笔直接实收 SOURCE 共 **13,270,305 Q20（12.6555 单位）**，最后一笔在 **5.892 s**（832，活沙 118，仍有 74 个空沙槽）。最后六次探针均零实收，其中五次合法 Down 拿到 hold，一次 Down 无授权。全程 SOURCE 预约终态 4,025 条，3,743 条被更新的相邻区间取代、256 条旧授权撤销、26 条实际接受。后段至少有真实预约产生和被撤销，容量耗尽无证。
- 513 次 ROOT_DEADLINE 中 **509 次**首等还剩 ≥245 ms。其 p50 首等预算 249.706 ms、record 0.250 ms、QueueSubmit 调用 0.033 ms、submit 返回至业务停止 251.369 ms、stop 后同 fence 退役等待 16.965 ms。所有 513 根最后一笔业务 wait 为 VK_TIMEOUT，退役为 VK_SUCCESS。另 15 次 LATE_FENCE_SIGNAL 是已信号晚于期限，5 次 CLOCK_GAP 有独立原因。按最后拒根停点，配额 `16507 = 25 accepted + 13819 overload + 2658 revoked + 4 pending + 1 active`。overload 记到达需求未变为可消费信用的损失，不能拿来增加本文实际接受数，也不能由它倒推 GPU 各段耗时。
- 247 组 `RETIRED→NEW_BASELINE→HOLD_REVALIDATED` 是恢复边沿与授权证据。原件另有 285 个等待新 Down 的标记。两条长按原件没有 `M1_RETIRE_PENDING→M1_RETIRE_WAIT→M1_TOMBSTONE_RETIRED`，也没有 Q 物理 fence 仍 pending 的收据。T925 中**第一次退役超时、续等后同代恢复**这一修复支路仍缺 PC 阳性。原 `M1_HOLD_RECOVERY.reason` 为固定字符串，归因按根 authority 和原 QPC，不能把全部恢复标成 CLOCK_GAP。
- T927 的 Frame 50 同根归档有 940,389 Q20 床库存，观测 ROI 有真实沙色 2/5/0 px。原行 31 Down、571 Up、609 Present 复算持按 4.00753 s，**该帧在 Down 后 11.29406 s、Up 后 7.28653 s**。这条正证覆盖后段的可见沙床，2–4 s 首堆门当前无同根阳性。帧号不能充当实际墙钟。 [长程原件](../test/reports/t927/runs/t926-longrun/argv.json) · [像素原件](../test/reports/t927/runs/t926-pile-pixels/pixel-evidence.json) · [T925 原故障](architecture-T925-v1.md)

### 35 派发短窗的 GPU 实测

`t926-fastcandidate` 有 26 SOURCE、56 真实接受 Q 的可用设备点，以及 1 个窗口关闭拒根的同 fence 退役点。按 run/geometry/nonce/fence 与业务根对应，GPU tick 用 36 位回绕与 83.333 ns 周期复算。图形按同一 acceptedRoot/generation 的 GPU draw 计。长程与短窗的时钟及工件分开，表内阶段为当前设备边沿之间的区间，含同一命令里的 barrier。

| 进入 Q 的活沙 | Q 数 | GPU 根 p50/p95 ms | 沙运动及邻接阶段 p50 | 独立审计至 replay 开始 p50 | 独立 replay+metadata p50 | 主排序/到达排序 p50 | shader draw p50 |
|---|---:|---:|---:|---:|---:|---:|---:|
| 4–31 | 9 | 61.92/76.90 | 9.67 | 22.11 | 17.70 | 0.113/0.196 | 0.317 |
| 36–63 | 7 | 103.53/134.62 | 24.37 | 27.42 | 44.41 | 0.092/0.333 | 0.698 |
| 67–93 | 7 | 160.00/183.26 | 41.19 | 35.32 | 74.07 | 0.091/0.397 | 0.965 |
| 96–107 | 33 | 190.09/202.00 | 48.40 | 39.85 | 88.62 | 0.091/0.793 | 1.142 |

最重组有 96–195 条实际事件，GPU 根最大 207.09 ms。阶段 21 是独立审计，现有时标只能量合计，需再分出库存、角表和来源账各自耗时。阶段 22–24 合并了 BASE 的 spare copy、逐行 replay 和 metadata。stage 11 的区间也含 stage 10，因此下轮要给作用点两侧取时标。SOURCE 本批 GPU 根 p50/p95 38.51/49.38 ms。现有 query 行含固定队列、池、fence，后续补实际对象代和每次 reset serial。

T927 的 cap=4 短对照实际接受 59 和 86 根，其 record→stop p50 约 112.5/82.6 ms。它是人工输入形状相同的两个自然试次。SOURCE 分别 **8 / 10** 笔、4,560,212 / 5,445,557 Q20，BASE、请求量和事件不同。6 个相同活沙点的窄字段可记观测相合，≥32 活沙差分无同根种子依据。bitonic 的 workgroup 粒度只改变调度，两个版本的每个接受 Q 都消费 1/120 s。不能据自然运行的事件数差异裁为算法或排序步长差。 [设备点位与派发](../entry/src/main/cpp/gpu_resident/g13vulkan_boot/m1_quantum.inc) · [127/35 真计划](../entry/src/main/cpp/gpu_resident/m1/quantum_passes.h)

### 下一轮实施和裁决

1. **先补可分辨测量。** 沿 T926 build 用匹配的 35/127 角色包及同一可复现输入。先固单量子、真实预算、姿态、输入场景、CPU/GPU BASE、画面及 query 开关，以真实 query 开关做一次交错对照，免把点位开销当收益。补 GPU 21 审计的库存/角域/来源分点，22 拷贝、23 replay、24 metadata，以及 stage 10/11 边界。每根记活沙/水、行数和行类型、源前缀、触及列、端点与最深连续冲突链。同批 0/32/64/96/120 活沙、等沙量静床和真实绘制/离屏的两个独立因子各采。对重负载优先用同 seed 单根，之后再用 `drive_t927.py --scenario longrun` 的实际 10 循环、沉降、6 探针校验 127/35 两臂。应从原 stdout 重新按 nonce 计，不用 runner 早期的 `M1_QUANTUM/M1_SOURCE` 名称计数。
2. **优先改逐根审计和 replay。** 35 派发短窗的主排序和到达排序均在亚毫秒量级，重态下主要预算在 replay 与 metadata（88.62 ms）、运动与邻接（48.40 ms）、独立审计（39.85 ms）。第一实现增量把 stage 21 的全池逐列扫描、重复 ID、角域、来源前缀拆成有界的 GPU 验证段。库存分别用确定归约，ID 用稳定键排序后邻接校验，角域仅在对完整原 Q20 表核单调及重复值后用两端二分。保原错误先后与有效槽/零区覆盖，保源账累积和原读写。逐字验证的前置可并行核行格式、mask、身份和不依赖前行的字段，再由从 BASE 开始的独立 stage 23 处理真实状态依赖。动作段独立测构造与提交，候选可并行产生待核几何，冲突、列预算与真实事件提交保原有序语义。只在读写集合和账面交换律已证时引入独立行群。目标是让扫描成本随槽、列、角表、实用事件行和来源记录的规模线性增长，另计稳定排序和必须保留的状态依赖。校验仍覆盖全部实用事件行、零槽和独立 spare。
3. **对照和止损。** 每增量使用同一 BASE、冻结业务载荷与操作时间（第 3 节），同时核逻辑 slot 与实际 GPU 写集。比较候选/独立 spare、源账、全部已用事件、Completion、首个拒绝及确定未写区。用错行、重复 ID、旧代、迟 fence、可观测事件列竞争及无活动重床作负控。画面单独量同代 GPU draw 和 WSI，优先处理上述 GPU 阶段。优化版先证明代表负载下 ≥60 s 后的真实 SOURCE 与 Q、逐笔 LAND 及同根 S/P 守恒。像素门沿 T908 固定夹具，实测 Down 到同根 Present ≤4 s，注入点附近连续三列各至少 6 个物理像素，并保留松手后的沉降与保持窗口。T925 异步退役需要第一轮超时、同对象代后到成功、再新会话和实际同 Down 的独立正例。开发集显计正确性与实际成本，目标独显由用户测真实场景 30 fps，继续保每 Q 物理时间和 250 ms。 [T908 像素和停积门](architecture-T908-v1.md)

## 2. G15 opcode 23

两份独立保存的原始 stdout SHA 见证据 JSON，均止于第 121 行。第 120 行 shader module 实际返回 0，第 121 行是 `vkCreateComputePipelines` ENTER，没有 EXIT，未出 BOOT 或业务根。T927 摘要另报共五次、45–240 s、两个构建和两种 seed CLI，能独立审核的是现存两份原始输出。具体结束时点和各次资格设备、owner 栈、CPU 工时尚缺配对收据。按原代码，`VulkanOwner::Open` 顺序建整套角色，失败在 seed/根消费之前。dune/plain 的切换不能隔离该 API 的输入。

- **停住的角色是 `G9_OP_FLIGHT_PATH`（23/0x17）。** G15 发布 PE `307e5248…f76e`，磁盘且日志确认的角色 SHA `29e0d466…3e24`，123,696 B。T924 baseline/分段对照是 `5f8fcc77…4c01`，123,644 B，同 4 SSBO+1 UBO、240 B Common、20 B push、布局 `a35e2f4d…750b`、compute main、flags=0、app cache=null。历史对照的 opcode 23 三次返回 0，**1.1205 / 1.2290 / 1.2462 ms**。
- 解码 SPIR-V 指令和 header，G15 的 **唯一字节差异**是 52 B 的 `OpSourceExtension GL_EXT_shader_explicit_arithmetic_types_int64`。删这一条后整模块 SHA 就是 baseline；两者 capability 均只有 Shader。源码扩展记录无 SPIR-V 执行语义，仍可能改变驱动内部缓存键或输入分支。之前的 0–22 中，去掉同类源码记录后仍有 **17、18、20、21、22** 五个模块不同。旧/new PE 与实际调用上下文也是独立变量。 [SPIR-V 指令规范](https://registry.khronos.org/SPIR-V/specs/unified1/SPIRV.html) · [G15 生成器](../pc/vulkan/build_sand_roles.py) · [原 Open](../pc/vulkan/sand/owner.cpp)
- **与 G13 OP_48 的交集是 Vulkan 建管线层。** OP_48 是 G13 的 261,304 B 模块 `f01959cc…`，布局 `b9390359…`、24 B push，其实际 minidump 已定位到 igc64 持续用 CPU，noopt ABBA 仍无返回。G15 目前只有相同 API 的无 EXIT 形态，缺本调用的实际驱动栈和 CPU 计量。两者不能共享 igc64 的 RIP、私有编译阶段或永久挂起标签。 [OP_48 原栈](developer-T921-v1.md) · [noopt 原对照](../test/reports/t923/report.md)

**Developer 接口和 effector 最小对照。** 用同一 G15 host 的私有资格入口，维持真实 sand layout，首组新进程交错做 opcode 23 单角色原字与只去掉该条非语义记录的字节，外部记录两个 SHA。两者都显式保真实 flags/cache/create-info。第二组固定完全同一目标模块，只换匹配的 DM2/G15 原前缀，保持角色个数和顺序，先用 0..22，不从 seed/输出变体做二分。只有前缀实测分化，才限在五个差异角色做最小前缀或逐一交换。单角色也无分化，优先监督同一调用、看栈/CPU/缓存，避免重跑已消耗的同条件长窗。每臂要有设备/队列/loader/ICD 和 compiler 文件 SHA、owner TID、真实 ENTER/EXIT、20/45 s minidump、CPU 增量及监督截止，原工具 `qualify_role.supervise` 可复用，但须适配 sand 的 opcode/TID/频率，G13 的 CLI 不识别这个角色。若要交去记录候选，必须重新产一致清单、生成头与 PE，核全表、输入 SHA 和新包身份。私有诊断不获 BOOT 或输出资格。开发可把准备管线移出用户 UI 阻塞边界，实际角色就绪后才开放 G15 QUANTUM。支持并启用 cache-control 的设备可独立诊断 compile-required 返回，实际可用性与产品阶段资格另采。 [Vulkan cache-control](https://docs.vulkan.org/refpages/latest/refpages/source/VK_EXT_pipeline_creation_cache_control.html)

经上述新原件确立 OP23 可通过，才续核 G15 正反沙丘 ±45°、真实 typed Q、质量/来源/能量、拒绝原子性及同根 shader 证据。每对照分别记录实际集显或独显及版本。

## 3. 自然 OFF×2 的实际分叉和可比接缝

同一 T926 PE 和三份非紧凑模块，两进程都跑完 52 动作。离线重跑 T897 checker 得出口码 0，两臂各自通过结构检查，producer 到 owner 终态唯一，分别 **29 / 48** 个真实入队拍。双臂比较结论为 `CLOCK_OR_INPUT_DIFFERENCE`，实际退役见原输出。compact 模块的 `M1_MODULE_SHA` 是已发生的合同负控。

| 项 | OFF1 | OFF2 |
|---|---:|---:|
| action 1 宣布时刻 | 17 ms | 17 ms |
| action 1 实际 callback−READY | 27.2209 ms | 22.9258 ms |
| action 1 callback→native / native→owner | 2.3 µs / 71.4 µs | 0.6 µs / 15,620.6 µs |
| action 7 实际相邻 callback Δ（6→8） | 15.0267 ms | 16.1751 ms |
| action 7 实收 | 126,053 Q20 | 135,686 Q20 |
| action 29 | REVOKED | EXPIRED / STALE_ENDPOINT，年龄 139.979 ms |

原 SOURCE command SHA 分别 `c2387d6e…ae72`、`bd7cff05…df49`。action 1 的差已在实际 callback，action 7 还改变了真实物理质量。严格比较的首差成立，不能靠减去每个 READY、使用脚本 16 ms 或只对终点质量来抹平。 [T927 OFF 收据](../test/reports/t927/runs/off-pair-t926-full/receipt.json) · [既有 T897 独立判据](../test/t897/check.py)

1. **先交窄的私有等输入诊断。** 由 developer 复用自然 OFF 已交接缝，把 OFF1 action 7 的真实取样选作一份固定参考，保真实日志 SHA、原 callback/receipt QPC 和频率、实际 SOURCE 体及 Q20/request、producer/32-word command、合法样本物料和相邻时钟。另经实际 GPU 冷读回封存 SOURCE 前的空 BASE、设备/role/config/几何和紧接的一次 Q 初态。若旧原件只有日志，按同版冻结空 BASE 的 GPU 原字并验证新一份参考真实承认，保所选参考身份。数据包读入先核范围、关联、版本、等式和 SOURCE 命令的实际载荷，不能用声明时刻替换采样 delta。
2. **分两套时钟，保原业务 gate。** 私有入口可固定参考 workload 的实际物理 Δ、Q 步和源质量，派生新 run 的私有 scope/nonce/seed。当前实时入队、提交、fence、退役和诊断 watchdog 使用新进程真实时钟。参考 QPC 只识别参考样本及其物理载荷，禁止填入实时 freshness/authority/permit 字。若原 path 无法分离 workload 和真实授权，先在私有 post-freeze 接口消费完整固定输入，仅授予私有 GPU candidate/独立 replay 证据，`acceptedRoot=null`，无生产 CURRENT 或照搬旧 live permit。若要走同一业务 guard，当前真实拍必然决定自己合法的质量和窗口，此臂的实际时域差仍报告。原 `--m1-d1-replay-prefix` 是 accepted root 冷档输出路径，不能把它当受控输入入口。
3. **最小验证顺序。** 同档位正确模块、同逻辑输入、同 BASE，先私有 OFF/OFF，验证上传 bytes/哈希、剩余配额、SOURCE 原字、第一 Q 的 BASE/候选/独立 spare 及事件范围。再切仅观察路径 ON 与冷档封存开关，保原 permit/fence 收据和首个逻辑差异。只归一经字典证明为每臂身份外壳的字段，实质输入、已用 EVENT、容量、原生可验证摘要与旧字各自保存。负控用 action 29 的真实过期参考、篡改 Δ、错 session/geometry、动作晚达、拒绝后迟退役及 OFF 热页旁路。若要进入 T865 正式四臂，另有完整同实际时域/输入/owner/retire 证明后才能继续。T496、T504 原门保持。

**角色交接。** Developer 可独立推进长程计量与审计增量、G15 的窄资格入口、OFF 固定输入接缝。Tester 核三条线的独立原字、包一致性和负控。Effector 用配套工件取 35 派发长程、高负载迟退役链、opcode23 本调用栈及实际接受证据。尚缺 35 派发等输入长程、≤4 s 同根像素、G15 本次驱动栈与产品根、严格合法四臂。三项现有原件已离线复算。GPU 驻留、shader 渲染，禁止 Canvas 回退。
