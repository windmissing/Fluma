# T233：GPU 驻留 M0 逐 word 工程定版 V1（T241 工程修订 V2）

> **T255 产品接点隔离**：`test/t246/protocol.h` 的 256-word tail 是平台微探针私有 revision，既不含此处 completion256 的全部身份、stage/audit/replay/stock/receipt，也没有产品 GPU root。平台可在独立窗口证同 context compute→fragment→完整 swap、实际内存分配与生命周期，且不等待 T236；但任何物理/source commit、accepted bank 切换和 publication lease 只能消费 T241 产品 completion 的全部字段、尾 fence/GL/error/tombstone 检查。synthetic token 错配仅证明 host 拒绝函数；真实 late fence、context loss/后台重建和 release 必须另证，未证标 NOT_RUN。

> **T242 平台接续边界**：T232 在独立 pbuffer 对 256 U32/1024B 尾包的微探针通过，只覆盖同步/读回形状；不是此 manifest 的完整 completion256 字段、五 root 同 tick、验签/审计及实际 GPU M0 bank commit。256B `GL_SHADER_STORAGE_BUFFER_OFFSET_ALIGNMENT` 约束 range base，不改变本稿 `4*index` 的内部 word ABI；实际每个 range 的 base、length 与总对象 bytes 分别核。vertex SSBO blocks=0，正式窗口需要 fragment SSBO fetch 或另版 GPU pack 路线实测，不能假定顶点 shader 直读。15,060,496/36,997,904B 是 payload 算术，不是 T232 实际并存分配 PASS；真机探针须保留双 arena、双发布、lease、全帧目标及 LastGood 峰值、GL 错误与释放记录。离线 T236 实施不因此等待真机。

> **T241 修订，2026-10-29 定版**：T236 `c7ab688` 发现 K192 双 pad256 索引原模板短 512B，且产品 event/completion 枚举未冻结。本稿第 2、3、6 节与同名 manifest 的 `engineeringRevision=T241_M0_WIRE_AND_SORT_V2` 现为有效工程合同；原 15,059,984B 是 T230/T233 设计模板历史值，不可继续按它冷分配 K192。M0 字段 word index 未搬移，既有生成头需要按新 manifest SHA 重生成；产品 wire 数值此前未定，本次首次定版，不把 T236 mock `CompletionPolicy` 当旧产品协议。M1 的 T238 manifest 以本修订为基线另版读取。

2026-10-28，architect；原卡 `6ae1ea8be4b0c3103c13e7ce`。目标6/P5。消费 T230 设计 `b8f7e3f` 和 T231 平台补正 `4722534`；本稿冻结 **GRS_Q20_V1 隔离新 run** 的工程接口，配套机器可读 `analysis-T233-gpu-resident-M0-engineering-manifest-v1.json`。状态 `ENGINEERING_READY / CAPABILITY_REQUIRED / PRODUCT_NOT_IMPLEMENTED`。不使用真机；不修改业务代码、生产默认路由或设备设置。新 XComponent+shader 是全部背景、前景和场景的唯一上屏路径；故障为 FAULT 或标明 STALE 的上一完整有效新管线帧，绝无 Canvas 回退。

## 1. 冻结范围、版本及取证边界

M0 是独立空容器、单 H 沙、无 W/J 的 source 出生、Q20 运动/DDA/跨列/LAND、三色床边、全域审计及事件重放、整 bank 提交、同 token 床/粒子完整呈现。源队列、停/重置、角输入、满池拒绝与故障可报告。M1 水/J、M2 驻留 pair/merge、M3 旧全通道各有独立 feature bit；M0 对这些配置在库存写前 `UNSUPPORTED_FEATURE`，旧 run/snapshot 亦拒绝导入。**绝不**用 CPU 热路补算被拒绝的阶段、将旧 F64 轨迹称为等价，或降低守恒、容量、角 guard、ID、确定选对、root 原子性的硬门。算法数值、时间、角表、预算、事件 schema、资源上界均持有 manifest 修订号与 shader/source SHA。T230 的角表 SHA 是设计模型参考值，不能当已批准生产表；正式构建须独立高精度生成、逐值核误差并记录真实 SHA。

T231 曾建议每 frame 一个 64 word/256B `StepResult256`；此处被 T230 的 **`GRS_COMPLETION256_WORD_V1`、每 root 256 word/1024B** 取代。最多 G1+F4=5 root/tick，最多五个尾 fence、5120B 常规读回。T231 探针需按此更正，不把缩减后的结果包或只有一次 fence 计成本。

## 2. 逐 word 布局和分配算式

机器 manifest 的 `layouts.*.words` 数组下标即 U32 word index，字节偏移为 `4*index`；保留位必须写 0、读 0。所有 SSBO 按 `std430` 的 U32 数组传输，连续 U32 没有隐式 padding；有符号 Q20 是二补数原始 word，U64 为低 word 在前，F64 输入仅为原始 IEEE 754 binary64 的 low/high 两 word。禁止把 C++ `struct` 内存布局、GLSL `vec3` 或 host endian 猜测当 wire；冷启动用单 word 0x01020304/高低 word 测试，失败拒绝 profile。每一 buffer base、range、`GL_SHADER_STORAGE_BUFFER_OFFSET_ALIGNMENT`、`GLsizeiptr` 加乘均 checked；offset 未对齐或 buffer 小于最后一个 word 则拒绝，不能截断。

| 布局 revision | 固定长度 | 写者、有效性与要点 |
| --- | ---: | --- |
| `GRS_SAND16_WORD_V1` | 16 word/64B | sand slot SoA，slotGeneration 防迟到/复用；mass=0 才 free。ID 不回退，prev/next 仅兼容字段、M0 为 0，不以链序仲裁。 |
| `GRS_WATER12_WORD_V1` | 12 word/48B | M0 全 0 且审核；M1 才升能力位。 |
| `GRS_COLUMN16_WORD_V1` | 16 word/64B | bh/bl/mh/ml/UH/UL/W/J/PH/PL、S/mix/eta/envelope/localUsed/localRemaining；M0 仅 bh/mh/PH 可有物理值，其余相/水库存 0。派生量每 root 重算并核。 |
| `GRS_STATE_HEADER256_WORD_V1` | 256 word/1024B | 三 bank 各有独立 header；magic/revision、完整 run/owner/context/source/physics/control/geometry/config 身份、accepted source prefix、next IDs、全局 U64 账/预算/事件、角与时间、索引版本、layout/shader digest；末尾 reserved 全 0。base bank 的 header 不被 candidate 写。 |
| `GRS_SOURCE_COMMAND32_WORD_V1` | 32 word/128B | 完整 request/input/gesture/geometry/source/clock 原始字段；dedup 比较全部 32 word，不用短 hash 判相同。 |
| `GRS_TERMINAL32_WORD_V1` | 32 word/128B | 源终态；dedup64=command32+terminal32，D4096 不驱逐。receipt64 与之同型。 |
| `GRS_SOURCE_EVENT16_WORD_V1` | 16 word/64B | source slab D4096，current header prefix 限有效行。 |
| `GRS_EVENT64_WORD_V1` | 64 word/256B | GPU 事件，包含完整 root/stage/ordinal、真实旧新 slot/列/stock、预算、精确 λ 分子分母、来源/token。未使用字段全 0，不能拿一个 hash 冒整条事件。 |
| `GRS_PUBLICATION_SAND16/COLUMN8/HEADER256_WORD_V1` | `64K+64Kw+32N+1024` B/slot | 双发布槽，每槽有完整身份、state/scene 版本、seal；仅接受后 lease 给 renderer。 |
| `GRS_COMPLETION256_WORD_V1` | 256 word/1024B | 每 root 一个尾结果。头含全部身份/outcome/首错/提交提议/ledger、event、stage、publication、资源计数；摘要只用于 host 拒错和诊断，不代替 GPU 审计及独立逐 word oracle。 |

布局中的多 word SHA（8×U32）按 digest 原始字节 little-endian 映射，任何一个 word 不匹配即拒。`magic/revision/length` 全部先核。manifest 的 `header256` 和 `completion256` 为本 architect 具体字段分配；T230 未逐槽列出的空位保留为 0，不得私用而仍叫 V1。event64 允许动作前后状态及 rational λ，失败段也保留首错；实际写者按 stage 固定、globalOrdinal 单调。双 frame arena 中已封存上帧和本帧皆保留；冻结诊断占满即背压，不覆盖证据。

SoA 逻辑槽 `slot s` 的 word 地址为 `base + 4*(field*K+s)`，列为 `base+4*(field*N+c)`，water 为 `field*Kw+w`；publication 各数组同理。header 与 payload 独立连续段、各段 GL 对齐上取整，padding 非 payload、不可读作状态。CPU/独立 tester 使用 manifest 计算 index，不复用产品 C++ 宏当 oracle。源、dedup、事件 append slab 的可见 prefix 由 current header 决定，candidate 只写下一无效行；失败不增长 prefix，不能改此前有效行。slab 本身的冷清零和退休须绑定 run/owner epoch。

资源公式来自 T230，经 T241 修正双排序索引：N128、Kw64、Q16、D4096，K192/512；`Eframe=133K+508`，每 event 256B，双 arena。三 state `3*(64K+48Kw+64N+1024)`，双 publication `2*(64K+64Kw+32N+1024)`，motion/index/scan/edge/audit、dedup/event/queue/status/completion/receipt、角表和控制均逐项列 ledger。双索引由旧 `8K` 改为 **`8*Kpad`**：K192 pad256 时 2×256×4=2048B，较旧 1536B 增 512B；K512 pad512 不变 4096B。修订 payload K192 **15,060,496B**、最大单 arena **6,667,264B**；K512 **36,997,904B**、最大单 arena **17,562,624B**。这只是数组 payload；实际峰值另加 GL allocation/alignment、program/staging、XComponent render targets `8*w*h`、lease/LastGood 共存、NAPI/CPU oracle、冷 trace/checkpoint 等。T131 旧设备 max block 1GiB、global bindings16；T231 报 compute stage blocks16，必须在新 XComponent context 重测；绝不能从单 block 限值推整套可并存。

## 3. 四 binding 和 dispatch 工程表

每个 compute program 至多四个 SSBO binding，固定语义 `0=READ_BASE`, `1=WRITE_CANDIDATE`, `2=WORK`, `3=EVENT_OR_RESULT`；阶段之间重绑 ranges，不能一边把 candidate 当输出又从同 alias 读、也不能旧 binding 留脏。状态 copy 用 current→candidate，replay 用 base+event→spare；audit 读 candidate，publisher 写未 leased publication 槽。多段 state 要么在一个大 buffer 的严格对齐 range 内分阶段绑定，要么 separate object；实际对象表逐 pass 列 `object/rangeBytes/baseOffset/consumer/barrier/GLerror`。一个 pass 若需要超过四个读写域，拆 dispatch 并加数据依赖 barrier，不能暗添第五 binding；global≥4、stage≥4 及 workspace alignment 是启动门。

| pass 类 | binding 0 | binding 1 | binding 2 | binding 3 | 提交前核验 |
| --- | --- | --- | --- | --- | --- |
| copy / root prepare | current state 段 | candidate 对应段 | status256/control | 当前 frame event header | 段覆盖完整、root nonce、一条 owner 指令序列 |
| source scan / birth | candidate free flags | candidate sand/column 对应段 | scan/index | command32 与 dedup 当前 prefix；需要另一个域即拆 pass | slot 升序、ID 前缀、source/事件写者唯一 |
| sort / proposal / resolver | candidate 当前只读段 | candidate 下一写段或 proposal | 双 index/motion scratch | frame event64 当前追加 range | ID 稳定序、不得读写 alias、event credit |
| 三色边 propose / alloc / apply | candidate 列只读段 | candidate 列目标段 | edge/预算 scratch | frame event64 当前追加 range | 同色不冲突、跨色 barrier、固定 edge grant 顺序 |
| audit / replay | base 对应段 | candidate 对应段 | spare 对应段 | sealed event64 range | 每个分段逐 word 对比，首错固定最小 ordinal；需 result 时另 dispatch |
| pack / seal | 候选只读段 | 未 leased publication 段 | status256 | completion256 对应 slot | publication token 全字段一致、结果写完再尾 barrier/fence；host 接受前仍不可显示 |

这是 binding **分阶段角色**，并不承诺一个 dispatch 能同时见 sand、water、columns、header、事件和所有 scratch；实现须为每个段显式排 pass 和真实 dispatch ledger。尤其 replay 比对结果与 event 输入争同一槽时拆 pass，不把只写部分完成态当整 bank 通过。`event64` 失败尝试也占 arena credit；首次 fatal 后封存当前 frame/首错并停该 frame 的后续 root，诊断冻结或退休后才可复用，不能在同一容量界下无限重试覆盖失败行。

`local_size_x=64`，group count、每 block bytes、共享用量逐阶段对新 context 的实测 limits 检查。K192 排序 pad256/36层，quantum `23+36=59`；source `16+2*8=32`；G1+F4 tick **268 dispatch**。K512 pad512/45层，quantum68、source34、tick **306 dispatch**。每排序层读完整 `Kpad` 输入数组、以 `i< i xor j` 的唯一 invocation 同时写两端到另一完整 `Kpad` 输出数组，层间 SSBO barrier 后交换读写角色。索引 word 存 physical slot：真实0..K−1，padding index i 存 `slot=i≥K`；比较 key 为 `(valid desc,id,slot)`，padding `valid=0/id=0` 且 slot 唯一，真实 `MAX_ID` 仍由 valid 区分；**不得丢弃 index≥K 的临时有效项**。T236 在 `k=128,j=64` 已复现64个真实ID进入K192 padding。最终只核 sorted live 前缀与无重复/丢ID，padding余段无效；真实 slot 容量仍K，双 pad 索引只在 sort init→resolver/final fold 生命周期内占 WORK，不复用给事件或水索引。WORK 内两数组逻辑起点分别0与`4*Kpad`字节，raw combined `8*Kpad`；若作为独立 SSBO range 绑定，各起点再依设备 `GL_SHADER_STORAGE_BUFFER_OFFSET_ALIGNMENT` 对齐，padding/实际对象字节另计。这个改动**不新增 dispatch**，但增加512B payload 与真实分配门；运行 stage 若拆 pass 必重算上界。主运动最多 32 行/slot/quantum，另末接触 1 行；frame 容量 `4*(33K+N-1)+K`。这些是固定 stage 上界，不是已测实际 GPU 时延；shader 融合只有另修 revision 且同 oracle、barrier/成本重算后才可减记。stage 顺序：copy→prepare→ID sort→proposal→ID 序 resolver→三色各 propose/allocate/apply→final fold→入口/work audit→spare replay+audit→publication pack/F32 投影核→seal completion。source 的 copy/control/free scan/birth/fold/audit/replay/pub/seal 独立 root，正常拒绝也封 metadata-only candidate 并逐 word 核库存等于 base。任何 fatal 在首错后停止对可见域写入。

跨 dispatch 写读用 `GL_SHADER_STORAGE_BARRIER_BIT`；若把 SSBO 数据作为 vertex attribute/indirect/image/texture/buffer copy/map 消费，还加与真实 consumer 相符的 barrier。workgroup barrier 不跨组。所有程序、binding、barrier、dispatch、fence、map 和 swap 的原码/时间按 root/frame 记录；GL error 不由下一 root 掩盖。

## 4. 整 root 接受、控制面与同帧发布

native 的唯一 GL owner 维护 accepted bank、root nonce、run/owner/context/surface epoch、配置/几何/数值/角表/layout/shader revision、冷资源和 FIFO。UI 只异步递交输入，不能等 GPU；native 只做协议检查、G≤1/F≤4 时钟切片、GL 生命周期/有界 completion 校验/selector，**不做**逐粒积分、碰撞、guard、预算、pair、每 birth 全 sort/SHA 或整池热读回。固定 `a=carry+120*deltaNs`、`F=floor(a/1e9)`、`carry'=a%1e9`，F>4 拒绝；dt0 严格无写，F0 只接收控制 carry。暂停/倒退/超窗清 carry，恢复首 tick 0；每 quantum 单独 root，前面成功不因后面失败回滚。

每 root 冻结完整身份与资源 credit，GPU 从 current 构 candidate，事件/审核/重放逐 word 与 spare 相等，写 sealed candidate publication/completion，**从不改 current selector**；尾 barrier、fence、异步 poll，成功才 map 1024B。native 校全部身份、版本、root nonce、当前 base revision/owner/context、completion 长度、首错、GL 原码、tombstone 和 lease。通过后在同 owner 的不可分割控制段接受 `bank+revision+receipt`；sourceSuccess 此刻才外发。未接受前不得递交依赖 candidate 的 root。正常拒绝只在 metadata-only 且 stock 全 word 等 base、终态 dedup 正确时接受；错误/不等/迟到维持原 current。取消、stop、reset 不撤销已接受源/quantum；inflight 标 tombstone，候选/资源 QUARANTINED，late fence 只能退休；上下文丢失为 `OWNER_STATE_LOST`，旧 GL authority 不可用，M0 不支持恢复旧 run，不能用摘要重造 pool。

接受后将**同一 accepted bank** 打包到独立 publication slot，或在候选 root 内 pack 好并于接受后 lease；不得在校验前显示 candidate。`GRS_GPU_SCENE_Q20_V1` 单独解析 Q20 authority 与经整数软件 RNE 得到的 F32 可视化，不谎称旧 F64 renderer parser 通过。列投影若 F32 越界，取前一个可表示 F32 且保持原列；库存/来源不由 F32 反算。M0 `DRY_Q20_OCCUPIED_ENVELOPE` 显示床 S 和活动沙，W/J=0、明水层关闭。scene 的背景、床、粒子、前景全部读同 publication token；真实 shader draw+swap 成功才标 `CURRENT`，物理/source commit 与视觉 CURRENT 分账。失败只 FAULT 或具备有效同 context 资源与完整旧 token 的 STALE。两 publication 槽有 render release fence；占满则背压/旧完整帧，不能覆盖 lease。首选同一 XComponent window GL context 单 owner；跨 context 仅在 T232 证明 share-group、producer flush/fence、consumer wait/release 与重建后启用。当前旧 pair pbuffer 与 renderer 都以 `EGL_NO_CONTEXT` 创建，GLuint 不可互用；无共享时不以 CPU 下载整池再上传冒称 GPU 驻留端到端。

## 5. 能力状态、失败成本和独立接力

离线工程允许 `CAPABILITY_REQUIRED`：新 XComponent context 的整数/扩展实编、4 binding/每stage blocks、offset alignment、真实 15/37MB 与 render 并存冷分配、barrier/1024B fence、same-context compute→draw、context loss/lease、timer 均由 T232/独立设备卡实证；没有设备结果不能写 PASS，也无需阻塞离线 M0 模块开发。profile 不够为 `MEMORY_PROFILE_UNSUPPORTED`，整数/同步不支持为 `CAPABILITY_UNSUPPORTED`，不能删 event/replay/guard 继续叫同 revision。FP64 与 float atomic 是备选探针，不是 Q20 必需能力。GPU timer 缺测记 `NOT_MEASURED`，不能从 CPU wait 倒推。失败 root 的 dispatch、barrier、fence、map、release、quarantine、封存 event、完整 render/STALE 成本全计；物理已接受后冷 capture IO 失败只 `CAPTURE_INCOMPLETE`，不撤销历史成功。

**T236 最小开发卡可执行 prompt**：不使用真机。新增 `entry/src/main/cpp/gpu_resident/` 独立模块、manifest 驱动的布局/checked offset/整数原语/三 bank、source32+dedup64+event64、Q20 h120/角表/三色床、全 audit+spare replay、completion256word+尾 fence+native accept、双 publication 与 scene token。M0 只开干单H空新run；其余 feature 明确拒绝，满池明确拒绝，stop/reset/late/context loss 按§4。一个指定**平台集成 owner**独占 `p013renderer/egl_context.*`、`scene_compositor.*`、`napi_bridge.cpp`、`entry/src/main/cpp/CMakeLists.txt` 共享 hunk，算法开发不要并改公共 hunk；旧 pair 与 CPU route 保留对照，生产默认不切。构建和非真机 primitive/fixture 自测属于 developer；真实 context、端到端与性能给 tester/effector。不得修改 `docs/后续工作清单.md`，唯本任务行状态有限例外；不改滴答同名清单；共享修改不回退。全部背景/前景仅新 XComponent+shader，任何故障 FAULT/完整有效 STALE，禁止 Canvas 显示或回退。

独立 tester 依 GR01–GR09 执行 M0：整数/RNE/U64/输入残差；source FIFO/free scan/ID/pool full；DDA/tie/guard/三色预算；每 stage 注故障、event arena满、翻位重放、fence迟到/stop/reset/context loss；逐 word bank/event/completion/scene token 差分，oracle 用独立 Python 整数/Fraction，不调用产品 helper；不同 workgroup 调度结果逐 word 相同；设备同时记录实际全部 dispatch/alloc/峰值/读回/失败成本与完整 UI 帧同负载分母。GR10–GR12 对应 M1–M3，未实现预测试为 `NOT_IMPLEMENTED`，不记产品通过。effector 在真实 M0 当前帧后看持续触摸、落床、角响应、拒绝/故障 STALE 与 CPU 对照。失败先 architect 归因；manager 登记并接力，不增验收门。

| 独立编号 | 最小 oracle / 设备观察与判定 |
| --- | --- |
| GR01 | U32 carry/borrow、32×32→64、U128/div、signed RNE tie/overflow、binary64 floor 量化、U64 总量跨 2³²；原 bit 输入与整数 oracle 差分。 |
| GR02 | FIFO/去重全 command32、slot 升序 free scan、ID 耗尽、部分/满池/冲突、旧 epoch、先 source 成功后 step 失败；ACK/sourceSuccess/CURRENT 分开。 |
| GR03 | 独立 Fraction DDA、同刻床/墙/列 tie、高床/反向边界、上限 32+1 行、q0 CONTACT 与材料 ZERO、partial/full LAND。 |
| GR04 | 全 `k∈[-2,2]` 整数可行区间、角表误差和 Hq605394 严格界、每 writer 与末审 actual guard；入口坏床必 FAIL。 |
| GR05 | 同列多 ID 仲裁、mod3 写域与跨色 barrier、总/局部预算 prefix、不同 GPU 调度逐 word 相同。 |
| GR06 | 每 stage 首错、event 满、replay 翻位、未接受 candidate 不碰 current；late/reset/stop tombstone、fence/timeout/context loss 及已有成功保留。 |
| GR07 | K192/512 实际 pass/binding/alignment、268/306 上界和失败成本、修订后 15,060,496/36,997,904B payload 对照真实 allocation/峰值；K192 `k128/j64` 有效ID进padding再出，热路无每 birth 全 sort/SHA/池读回。 |
| GR08 | compute→同 XComponent scene 同 token 床/活动沙、F32 跨列投影、各层完整 CURRENT；GL/present 错 STALE/FAULT、lease/release、无 Canvas。 |
| GR09 | 120Hz carry/0–4 quantum、暂停恢复首 tick0、失败 duration、accepted 角/commit、offered/accepted/背压与端到端帧分母。 |
| GR10 | M1 的 W/J、水面五界、在途到达、停源与守恒；未实现记 `NOT_IMPLEMENTED`，不把干场 PASS 扩成明水 PASS。 |
| GR11 | M2 完整 pair key/非邻最小 b/动态 merge/slot 复用、真实 GPU 更新索引与满池；未实现记 `NOT_IMPLEMENTED`。 |
| GR12 | M3 各旧 channel 的表达式/round/guard/预算/相库存、真实 λ/preview/权威/一致性；unsupported 位图能拒绝，未实现不冒全通道覆盖。 |

本稿只证明工程交接可执行；没有实现、构建、真机、完整帧收益或产品 PASS 结论。

## 6. T241：产品 wire 数值、逐 kind mask 与提交判定（修订 V2）

T236 `handoff-questions.json` 的第二项为**工程缺口**：T233 V1 给了 word index，却未冻结数值域，不能让产品 shader 与 tester 自猜。机器 manifest 新增 `wire`、`eventKinds`、`eventPatchRules`，为本节的机器可读副本。M0 wire revision word=**1**，M1 新 revision word=**2**；这个数值是格式号，`numericRev/layoutRev/algorithmRev` 仍单独绑，完整 layout/shader SHA 才是产物身份。各 packet 的 word0 magic 为其 ASCII `GRSH/GRSC/GRSE/GRSR/GRSP` 的 little-endian U32；completion/publication word3 seal=`0xC0DEC0DE`，GPU 完成所有写后**最后**写 seal，尾 barrier/fence 保证 host 见全包。未写/半写、长度不是256、非零保留位均拒，不用短 magic/SHA 取代完整身份核。header word3 是 flags、不是 seal。sourceEvent16 无独立 magic，受已接受 header prefix 与 source revision 限定。

| wire 域 | 数值与合法状态 |
| --- | --- |
| `rootKind` | SOURCE=1、QUANTUM=2；0/其它非法。 |
| `outcome` | SUCCESS=1、METADATA_REJECT=2、DEDUP_REPLAY=3、FATAL_ERROR=4；仅前三种在全审计、重放、尾 fence、native 身份核通过后可接受 bank，fatal 无物理提交。 |
| source terminal status | SUCCESS=1、PARTIAL=2、REJECTED=3；0/其它非法。PARTIAL 仍是**本次 q>0 的源成功**，rejectedUnits 可正；SUCCESS 的 rejectedUnits=0。量化/容量/slot导致 q=0 时 REJECTED，I/commit/ID 不增。 |
| reason | NONE=0；UNSUPPORTED_FEATURE=1、INPUT_RANGE_UNSUPPORTED=2、NUMERIC_RESOLUTION_REJECTED=3、POSITION_IN_BED=4、POOL_FULL=5、CAPACITY_REJECTED=6、ID_EXHAUSTED=7、DUPLICATE_KEY_CONFLICT=8、STALE_IDENTITY=9、CLOCK_WINDOW_REJECTED=10、SOURCE_LOG_FULL=11、EVENT_ARENA_FULL=12、NUMERIC_OVERFLOW=13、GUARD_VIOLATION=14、AUDIT_FAILED=15、REPLAY_MISMATCH=16、TIME_NO_PROGRESS=17、GL_FAILURE=18、OWNER_STATE_LOST=19、CONFIG_UNSUPPORTED=20、SLOT_GENERATION_EXHAUSTED=21、UNSUPPORTED_INITIAL_STATE=22。新 reason 须升 wire revision，不能复用旧号。 |
| source kind/delivery | command kind SAND_H=0，M0 其它拒；terminal deliveryMode SAND=0。M1 的 WATER=1、DIRECT_W=1、FLIGHT_J=2 仅在 revision2+feature bit1 有效。 |
| event kind V1 | SOURCE_BIRTH=1、MOVE=2、CROSS=3、CONTACT=4、LAND=5、BED=6、FINAL_CONTACT=7；其余拒。M1 V2 自有 1–14 编码，reader 必先核 revision，不拿同号当同义。 |
| event stage | SOURCE=1、MOTION=2、BED=3、FINAL=4；field BED 只用3，source birth只用1；firstErrorStage 的 1–9 另按 COPY/ADMISSION/SORT/MOTION/BED/AUDIT/REPLAY/PUBLICATION/SEAL，不能混为 event stage。 |
| root status | COLD=0、INFLIGHT=1、SEALED=2、FATAL=3；accepted header 只能 SEALED，GPU 不写 canonical selector。 |

`phaseSourceState` (sand word6) 是单 U32：bit0–3 state FREE0/AIR1/CONTACT_WAIT2/SIDE_WAIT3，bit4–5 phase H0（L1只给将来新 revision），bit6–7 sourceKind SAND_H0，bit8–11 medium AIR0（M1 WET1 在 V2），bit12–15 waitReason NONE0/BED1/SIDE_CAPACITY2/SIDE_HIGH_BED3/SIDE_BUDGET4/WALL5，其余位0。M0 仅 H/AIR；water12 全0。M0 新空 run 的 `bh=0`，没有固结写者；若冷 fixture 试图导入非0 bh/分层，拒 `UNSUPPORTED_INITIAL_STATE=22`，不让 BED 日志只记 mh 却漏 bh。`state` 与 `waitReason` 必须相容：FREE只NONE，AIR只NONE，CONTACT_WAIT可BED，SIDE_WAIT只SIDE_*/WALL；非法组合入口/末审失败。

event64 flags(word52)：bit0–3 oldState、4–7 newState、8 oldMedium、9 newMedium、10–13 oldWait、14–17 newWait、18–19 namespace SAND0/FIELD2/SOURCE3（WATER1 只 V2）、20–21 TURN axis 在M0恒0、22–31恒0。old/new 值须与事件前后 slot 的 `phaseSourceState` 解包一致。field 行 slot/generation/id/sourceId(word16–19)全0，SOURCE_BIRTH namespace3，其它沙运动 namespace0；字段53 failureReason 在成功 event 恒0，54 surfaceEdge 仅 CROSS/CONTACT/BED 可用，55 phase 在M0恒0，56–63 M0保留0。每 kind 的 `usedWordIndices` 包括允许非零的 word（有的值仍可为0），其补集**必须逐 word为0**；event63之外无隐藏 payload。replay 按 ordinal、stage、namespace、来源与所有 used/zero 字段核，不能仅以 hash 相等接受。event0–15 是每条的固定身份，word12–13 是生成该事件的成功 sourceCommit（source root 用提议 commit）；word48–49 是原始粒子来源，必须对现存 slot 或 source receipt 的 origin，而非独立改写；word50–51 request 身份必须与来源命令匹配。field 的 request/origin 不适用时0。

| event kind | stock A/B 与实际 patch | λ、预算和回放要求 |
| --- | --- | --- |
| SOURCE_BIRTH | A=当前列 PH 前/后；free sand slot `massBefore=0→massAfter=q`，id/slotGeneration/来源/位置从 command 与分配前缀生成；I_sand 与 header/source terminal 用 q 核。 | 不消耗固体预算；λ、旧速度均0；新增 ID 只一次，raw request 与 terminal 后核。 |
| MOVE | slot 位置/速度前后，stock A/B 不适用；不改质量/列。 | 有理 λ=1/1（全余弦段），M0 固定步积分核重新计算；flags 的 WAIT/介质不暗变。 |
| CROSS | A=from PH，B=to PH；同一个整包 mass，旧新 column、位置/速度、local/total budget 前后同时核。 | 有理最早列界 λ；目标容量含 S+W+P+J，T一次/L两端一次，拒不足不能产生 CROSS。 |
| CONTACT | slot 位置/速度/状态，q=0 也可有真实几何写，stock A/B 不适用。 | 有理最早床/墙/高床 λ，tie 床→墙→列；材料 LAND 另行，不把 CONTACT 当质量成功。 |
| LAND | A=mh，B=PH；slot mass−q，mh+q，PH−q；full 清 slot /generation 保留。 | 受 Hq/容量/T/L 的同一 q，预算 word36–43 实扣；q>0 才记材料事件。 |
| BED | A=donor mh，B=receiver mh；两列一增一减同一 q，bh始终0。 | 三色 edge 升序 grant、guard/容量/T/L，field slot/id/source=0，surfaceEdge=实际边。 |
| FINAL_CONTACT | 末床后位置/状态/支撑释放，质量与库存不变。 | 无正运动时间，λ/预算0；不做第二次 LAND。 |

event 的 before/after 配对不得以候选当前快照填两边；GPU replay 从 base 按真实 ordinal，逐行核 old fields、apply 写集，再核 new fields。每类只可写上述域，未列域逐 word 原值；全 replay 后另核两 bank 的 sand/column/header 与来源前缀、事件 cursor/预算/派生，firstMismatch 最小 `(stage,ordinal,wordIndex)`，成功时 completion word101=`0xFFFFFFFF`，102/103=0。这个 sentinel 与 T236 mock 显式 policy 可对齐；开发代码需按产品 manifest 注入，而不是把测试枚举反向写成权威。

completion word3 seal 通过还不足以接受。SUCCESS 必 reason0、首错0、receiptReplayFlag0、metadataOnlyStockEqual0；SOURCE SUCCESS/PARTIAL 有 admittedUnits>0、source/physics commit 各+1、I_sand 与 sand总量各加 q，receipt status按是否有真实**整数**拒量为 SUCCESS/PARTIAL，terminal successSeq=本次 proposedSourceCommit；输入小于一量子的原始残差另报，不凭残差把所有源标 PARTIAL；QUANTUM SUCCESS sourceCommit不变、physicsCommit+1、receiptStatus=0、source receipt各字段0，库存总量/I不变。header 的 sourceLedgerRev U64 是新增 dedup terminal 行数，必须与 acceptedSourceCount 相等；新key成功或拒绝各+1，旧key冲突与同key replay不变。METADATA_REJECT 仅 SOURCE，reason非0、receiptStatus=REJECTED、admittedUnits/birthCount=0、stock全 word等base、source/physics commit不变、sourceEvent prefix不增；新key拒绝可追加一条 dedup 终态，冲突旧key不同payload不追加，均不记sourceSuccess。DEDUP_REPLAY 仅 SOURCE/同key全32word相同；completion 的**receipt投影字段**复制原终态（故 admittedUnits **可能>0**），原完整 terminal32 保留在 GPU dedup；receiptReplayFlag=1、stock/commit/prefix均不变；原 dedup terminal 的 replayFlag 永远0且不可重写，native 此次 `sourceSuccess=false`、不重复通知旧成功。FATAL_ERROR reason非0/首错保留，GPU 可给诊断completion，但 native**不接受 candidate**、不增长 prefix、不发布 CURRENT；GL/fence/context错误即使无可信completion也同样失败。接受的三种 outcome 需 stage/audit logical mask `0xFF/0x3FF`、full token、GL原码0、尾 fence成功、无tombstone；mask是8个根逻辑阶段与10个不变量族，**不是**268/306个dispatch的计数。completion word105 `fenceStatus=0` 表示GPU不知host fence状态，word113 `actualBytesMapped=0` 是GPU封包时值；实际 fence/map 在host账记录，不能伪填GPU已等待。

T236 现有 `RootOwner` 是 mock 控制面，`ValidCompletion` 尚用单一 TEST policy 与 `Poll` 的通用接受逻辑；developer 按上述产品 outcome 修复：按 rootKind 校可接受状态、replay不得再次通知成功、metadata-only 两种 prefix 分支、fatal不可落 canonical bank；不要把 `replayOutcome` 当新的源提交。T241 只修工程文档/manifest，**不宣 T236 物理主核已实现**。扩容、wire 和 M1 兼容后，T236 可继续离线 source/运动/guard/audit/replay/scene 开发；真实设备能力仍按 T232 精确结果分支，非本工程修订阻塞。
