# T764 · G13 数值门前独立可见性探针

2026-11-21 · G20 · P5 · architect · 离线归因和实施合同。

> T777 补充。T767 实装以 Q5/Q410 的 OP12 作 scope gate，再由 Q6/Q411 的 OP13 做实际 copy。独立诊断 HAP 优先用原授权主 bundle 的显式构建，仍保独立诊断入口和 owner/context，按 T777 管理同名安装冲突、证据和恢复。 [T777 签名工程合同](architecture-T777-v1.md)

新增一个仅在诊断构建可达的 G13 离屏入口，在独立 owner、上下文和资源上执行选定的生产 pass。该入口完成后只交付可见性证据和退役收据。T730 的 BOOT、逐角色 FP32 资格、根调度和发布链照原路径。近期先走 935F 上已有 T756 GLES 角色，收集 T760 缺失的绑定和可见性原件。 [T753 绑定合同](architecture-T753-v1.md) · [T764 原件联核](architecture-T764-evidence-v1.json)

## T760 首次工程归因

| 可证阶段 | T760 原件 |
| --- | --- |
| 签包、映射与 BOOT 编译 | T756 未签 SHA `ab373f28…` 与签名输入相同，签后 SHA `868106e2…`，包内及实际加载 ELF SHA `369d5ee8…`、BuildId `e5ec50ff…`。attempt 191、PID 46924、start tick 8613992、owner TID 47218、EGL context 390351290432、program 2 同域。T475 的 `G13_COMPUTE` 事件对应 **BOOT**（23,861 B、`e1893369…`），实际 source compile、link、reflect 后落盘 301,961 B。其余计算角色和同身份新进程暖加载在该轮未观测。 |
| BOOT 数值结论 | 同 attempt 的 fence 原值 37146，GL 0，CTRL 尾封印 `0xc0de1301`、error 27。OP65 WORK +53 的次正规 `0x00000001` 变成 0，+59 半 ULP tie 得到 `0x3f800000` 并通过。OP66 两轮封印均在，原字中输入保留和结果生成两项均为 0，masks `[12,12]`。`BOOT_EFFECTIVE_TERMINAL:EXIT` 的首拒仍为 OP65。 |
| T756 隔离域 | `Transport::Poll` 在 OP65/66 成功后才置 `ready_`。owner 只有在 `BootReady()` 后才进入 `PrepareSteady`，完整路径再等全部资格与 Controller ready。T760 走到 `CONFIG_FAILED / INIT_ERROR_27`。原 BOOT CTRL/WORK 的 map 已证明该冷路径；双 bank、逐角色跨 program、共享 WORK 及 Scene 顶点路径均未执行。T757 的全表核对和合成 archive 判定继续成立。 |

证据属于两个问题。产品资格仍由 T741/T744 判定。本卡首次工程归因是 **可见性隔离验证缺独立入口**，可由 developer 接线。 [T760 报告](test-report-T760-v1.md) · [T760 数值原件](../test/reports/t491/t760/t760-diag-full.json) · [T760 落盘原件](../test/reports/t491/t760/t756-compute-persisted.jsonl) · [T757 独立离线检查](test-report-T757-v1.md)

## 入口与发布边界

1. 在独立诊断 HAP 构建目标内启用单一一次性入口，编译开关默认关闭，关闭时产物无诊断导出和路由。仅接收版本、固定用例枚举和 attempt，拒绝通用 op/ordinal 注入、任意 buffer、配置和 shader。每进程最多一个在途 attempt，同号读取只读报告，重跑须退休并产生新 attempt。由专用 native owner 创建新 **non-shared** GLES 3.1+ EGL context 与私有 pbuffer，独立包身份或进程，不接产品 owner 的窗口和命令队列。原 XComponent 和产品 NAPI 入口保持原业务路径。
2. 入口自己的状态为 `CREATED → ROLES_LINKED → PROBES → RETIRING → RETIRED`，故障保具体阶段与原码。上下文能力、同 owner/真实反射、program ID 与 role SHA/ABI、冻结 buffer 分配是独立前置。用当前角色构建/反射助手装入需要的原字节，将身份记为探针专用。允许链接、反射原 role 作可见性取证。此路径没有 `Controller`、`Transport::BeginRoot`、`PrepareSteady`、`Accept/Publish` 或 `Scene::Draw` 的能力，绝不能设置 `BOOT_READY / COMPUTE_READY / CAP_FP32=QUALIFIED`、创建根 permit、推进 commit/quanta/FIFO、签场景租约或调用 Swap。GLES 这一路私有 cache 冷启动可禁导入/导出；若启用则按 probe variant + 环境 + role 隔离目录和 key，生产 `Prepared`、缓存命中和产品计分不可由这里写入。
3. 让生产与诊断共用小的实际提交原语。保持生产的 `ResolveRootBinding`、`PipelineRoleForOpcode`、同代 program 核验、四个 SSBO 实绑、uniform 上传、每次 dispatch 后 storage barrier、尾 `BUFFER_UPDATE | VERTEX_ATTRIB_ARRAY`、fence/flush/poll 和有界 map 的实现来源相同；共享 helper 接收明确的 role、pass 和资源参数，生产 wrapper 仍先验 permit/预算。探针只准内部固定列表，使用原 ordinal 参与 resolver 与 `uIndex`，独立 `probeStep` 编号，不能用探针步号换原 ordinal。记录实际 `GL_CURRENT_PROGRAM`、indexed SSBO 名称及 size/range、原 uniforms 和 GL 返回，核 sources 与装包字节；再执行。留出错绑拦截前的只读检查，负控不得污染生产判断或偷跑生产根。 [原提交](../entry/src/main/cpp/gpu_resident/g13flow/transport.cpp#L401) · [角色与屏障](../entry/src/main/cpp/gpu_resident/g13flow/transport.cpp#L502)
4. 按固定 64×64 的生产 profile 建独立 BASE、CANDIDATE、REPLAY、WORK、CTRL 18,688 words、PACK 和观测资源，四个 binding 物理不重叠。两个 scope 共用一份 WORK。测试相关段先填各自不同的合法位型或 canary，未用段不进入 pass 的读域。使用当前资产生成的 layout/dispatch、初始化依赖和正常值的固定 fixture，记录 CPU 上传范围/字节 SHA 与 GPU 自己生成的原字。可构造比较前的局部 Ctrl/seal fixture，但其来源必须标 `FIXTURE` 并记录 offset；拆开的 phase 不能冒充完整根、真实数学封口或 PASS 轨迹。

## 固定用例

下表用 Q 表示原 QUANTUM 计划的 0-based ordinal。OP48 的本地循环明确让相邻 lane 访问原 WORK；跨 lane 证据至少含一个 ACTIVE lane 的真实邻接读。64×64 下可选内部 U 面 `f=127/128` 覆盖 lane 63→0，及反向的 lane 0→63。用不同但可精确表示的正常位型，拒绝用边界已钳制值、全零比较、已收敛、零黏性或 PROJECT_NOOP 代替活跃路径。阶段预期由 tester 从固定 fixture 独立算，不能拿同次设备结果回填期望。

| 用例 | 正控和原证 | 最小负控 |
| --- | --- | --- |
| 双 scope、同 dispatch 跨 lane | Q5/410 的原 OP12 分别实绑 BASE→CANDIDATE/REPLAY，校验 `uScope=0/1` 和两 bank 来源。Q814 的真实 OP6 消费两个 bank 与各自 CTRL trace，按模板验证 scope 字、nonce、封印，且 `CTRL[802]=0`、error=0。OP6 的跨 lane 原子 OR 到 lane 0 判定，由右列对照证明。另取两臂 Q27/432 的 OP48 block0，有活跃邻接依赖、`CTRL[729]=16`、block+14=16、正确 slot 与两段原记录起点 **3456/11648**。 | 分别将 CANDIDATE 与 REPLAY 同绑、令原 REPLAY 行 scope 改成 1、交换 trace scope/漏 seal，记录首拒。物理错绑或错计划须在 dispatch 前拒绝。另在独立 Q814 输入中只改 REPLAY 一个 velocity 原字，分别由 word 319/320 对应 lane 63/0 命中，实际 `CTRL[802]=1`、error=18、audit bit13 未置。 |
| 跨 program 与 WORK 复用 | Q26→27 与 Q431→432 使用不同的 `OP_18→OP_48` 原 program，绑定同一 WORK。真实 VISC_SETUP 写 RHS 和初始槽；OP48 消费并按原式旋转 slot 5/6/7。另取同 role 连续 block 作为对照（仅在第一个 block 未收敛时要求第二 block 实际迭代）。切换 scope 后输入带不同活动 face 标记，先产后读，核实际 RHS/最终槽、c[525]/c[729] 和当前 scope trace。跨 program 边之间不插 host map、readback、额外 fence 或 `glFinish`。用独立 attempt 停在 SETUP 后取 RHS 原字作为阶段旁证。 | 将上一 scope 的不同有限 sentinel 留在共用 WORK，重复合法 REPLAY 仍须得到该 scope 新生产的原字。缺 SETUP 的模板列为缺 producer，不能作为合格正控。用错 program/同 ID 错代保首拒，漏 storage barrier 的测试侧记录必须被 checker 拒绝，即使某次驱动偶然可见。 |
| GPU→map | 最后一写走同生产 tail helper，记录实际 mask、fence、flush、同代 signalled，之后 `CopyBuffer` 读 CTRL completion 物理尾（byte 73,728、1,024 B）及必要的 WORK/trace 小段。由本次 GPU 独立写唯一非初值，如实际 OP6 的 `CTRL[802]` 或 OP48 的 trace，另保生产尾位置上的探针写见证。生产尾探针见证可用独立小 GPU writer 写专用隔离 CTRL 的非零 attempt marker，证据明确标 `PROBE_WRITER`，不执行 Q819、不冒充 Completion。保存 map 指针有无、unmap、GL 原码、offset/bytes、搬回的原字。 | 输入 marker 初值与 GPU 输出不同，错误 offset/污染 word、缺 `BUFFER_UPDATE`、null fence、WAIT_FAILED、timeout、旧代 fence 与 unmap 失败都不能取得当前 PASS。 |
| GPU→实际顶点读取 | Q817/818 的原 OP9/10 对隔离 CANDIDATE→PACK，PACK 源使用可区分的两个 cell 和可校验 header/index。提取生产 Scene 的整数属性布置作为共用 helper，PACK offsets 256/272/288/304 bytes、stride 64，state tracer 按原 profile 偏移和 stride 32。保同一 compute→vertex barrier，在 **map PACK 前** 直接使用同一 GPU 对象实际 vertex fetch。以小的只转发 U32 的诊断 vertex/fragment shader 对两个固定实例输出到完整性检查通过的私有整数 FBO，使用多个像素/附件读出各组原始 uvec4，含第二实例的 stride 验证。先完成 draw/readback（建议有界 pixel pack buffer）再同 owner fence 和读回。tracer 用原 GPU state writer 的本次确切标记验证相同边；若实现只覆盖 PACK，逐项标 tracer 未测。此用例的 draw/readback 与产品 Scene 着色和呈现证据分开。 | 上一代 PACK marker、错属性 offset/stride、漏一个顶点屏障或错 draw 资源交给独立 checker 明确拒绝。实际不完整整数 FBO 或回读能力不足，记录原返回 `NOT_RUN_CAPABILITY`，其他可见性用例可继续。无画面成功、无 `eglSwapBuffers`、无帧率统计。 |

Q5/410 的 reset 正控从 `c[514]=c[518]=c[519]=c[768]=c[769]=0` 开始，两目标初值与 BASE 不同。Q814 比较使用独立固定图像，`c[129]=c[520]=0`、`c[518]=c[519]=1`，ACTUAL nonce 2056/2057 同 136/137，3071 为原封印。两段 8,192 words 的比较输入除各 1,024-word 块的 scope 字外相等，scope 字分别为 0/1，包括尚未使用的微步槽。REPLAY root 状态和封口由 OP6 本身写入。若要测混入旧 nonce 或缺 seal，篡改的是 fixture 的对应原字，保干净对照。Q26/27 与 Q431/432 的 WORK 夹具采用 `c[762]=0`、无 selected/skip/旧 error、正黏性及有限非零 `work(3)` 输入。SETUP 先写 4/5/6，7 由 OP48 首轮写入才可被读取，block0 末经 `5+((16+1)%3)=7` 取结果。保 RHS/slot 的生产和消费范围，不用 host 把结果补到 GPU buffer。 [原 OP6 与入口 guard](../entry/src/main/cpp/gpu_resident/g13flow/pipeline.comp#L109) · [原 WORK 迭代](../entry/src/main/cpp/gpu_resident/g13flow/viscosity.glsl#L2)

生产角色执行了局部数值逻辑时保原错误码，不旁路其内部 guard。仅对选定 fixture 与实际覆盖区给结果，局部算术不匹配时附原字，分别判断数据、绑定、可见性与数值问题。所有新/改 shader 按真实完整生成输入逐个验 **≤49,152 B**。运行原 T756 角色优先，附加 writer/转发 shader 用单独 probe role；不在接近上限的 OP48 上拼额外逻辑。 [T756 角色回执](receipt-T756-split-v1.json) · [原 dispatch authority](architecture-T607-g13-dispatch-v1.json) · [原 Scene 属性](../entry/src/main/cpp/gpu_resident/g13flow/scene.cpp#L135)

## 产物、退役与接力

- **有界原件。** 每次 attempt 输出版本化的 `G13_VISIBILITY_PROBE_V1` 诊断侧车，与原 BOOT 结论分存。记录包/签名回执/实际 ELF SHA 和映射 BuildId、pid/start/owner tid、driver/API/context/owner-generation、probe nonce、layout/plan/role SHA 和 bytes、阶段单调时钟、每次角色切换与真实 tuple、buffer allocation ID/range、原 ordinal/uScope、barrier 和 fence 命令及原返回。保首失败、最后成功、最后提交和终态，不读取估算原字、不用日志缺项补零。每 attempt 最多 256 条事件及 64 KiB 原字证据，超界走 `INCOMPLETE_TRACE` 并保首错和退役。设备测试每个固定用例可分 attempt 留证，避免扩张为全根排程。
- **真实退役。** 探针同 owner 执行，每轮最多 10 dispatch 或 8 ms 后归还 owner 调度，wait 使用无阻塞 poll；计时和取消不能强行宣称驱动调用已退出。先记录取消/首失败和已提交工作，未发 GPU 命令才可直接释放。各已提交对象以覆盖其最后使用的 probe fence 退休；wait timeout 留在途，null fence/WAIT_FAILED/上下文丢失隔离整个 probe 代，禁止重用，走独立 context 销毁与同代清理，无法证明退休留 `RETIRE_PENDING`。destroy/回收早于晚到旧事件时，旧回执只归旧 attempt。产品根仍用原 **250 ms**、真实 pulse 新鲜度和原提交审计；探针时长只报自身。
- **后端与资格。** 第一版做 T756 GLES 复用。同一 case/ordinal/range/oracle 可接 Vulkan 已验证的实际生产绑定/同步代码时，按 T753/T731 记录 Compute→Compute、Compute→Transfer→Host 和 Compute→Vertex Input 的实提交、fence 与非 coherent invalidate。仅用目标设备实际支持的模块，缺规定 FP32 能力时保拒绝。单独 U32 诊断模块仅证明自己的通路。需要合格设备或已验证的 GPU 等价路线的项目包括所有产品 role 的原 FP32、T730 全根与原 oracle、真正 XComponent Swap、同身份暖加载与有效 30 FPS。935F GLES 可先完成独立可见性诊断，产品入口仍须保原 FTZ 字和首拒。
- **实施交接。** developer 实现诊断构建/入口和共享提交 helper，编译检查默认产品包无诊断导出、实际角色/布局原件和 48 KiB 门。tester 固定有限正常数 fixture、独立判定各段原值与缺边、错代、非法入口、取消/退役用例，复用 T757 resolver 检查。effector 用已签隔离包跑同机 GLES 分段并交各用例原件，单列同时期已有的 BOOT 实测。首个缺证或失败交 architect，产品 T730 按原前置排程。

离线原件与选定计划核验使用 `python tools/architecture/analyze_t764.py`。本卡交付工程合同与原件联核，设备效果及产品实现留给执行卡。**G20 约 33%（4/12）**，T760 已证 BOOT 落盘，三条优化路线均待实际首个有效 Swap。 [T741 计分口径](architecture-T741-v1.md#g20-完成度口径)
