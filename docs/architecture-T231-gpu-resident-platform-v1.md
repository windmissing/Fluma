# T231：GPU 驻留整步的平台与工程边界 V1

> **T242 设备证据修订（2026-10-29）**：T232 在独立 pbuffer 内证明 4096 项整数原语逐位相符、跨 dispatch SSBO barrier 128 项零错、尾部 256 word/1024B 微探针零错。P-E 真实 XComponent window 同 context compute→draw/swap 未运行；不能由 pbuffer 的 GL limit 或 fence 推导产品驻留链已通。该 context 报 vertex SSBO blocks=0、fragment=16；P1 的顶点 shader 直读 SSBO 只能作为待测条件路线，先在实际窗口验证 fragment 整数 fetch，失败时升版 GPU image/texture 或 VBO pack，逐路线记新增 bytes/pass/barrier，不用 CPU 全池回传。offsetAlignment=256B 约束 `glBindBufferRange` 的 base offset，不意味着连续 U32 内部 word 都须 256B 对齐。单 SSBO block limit=1GiB 只是限值查询，不证明 M0 15/37MB、M1 23/55/25/57MB 连同双 event、双发布、完整新管线 render targets 能同时分配。所有限值和分配须在真实 window context 重查，逐对象记录实际 bytes/offset、失败 GL 错误与峰值。

2026-10-28，architect；原卡 `6ae1c798e4b0c1a6bd6163a9`。与 T230 算法设计并行，**本稿只定平台、生命周期、可验证接线和最小探针**，不替 T230 定数值表示、merge 顺序或事务算法。目标6/P5最高调度优先；旧 pair 原型与 CPU 全链是正确性/成本对照，不是 GPU 驻留整步实现。状态 `PLATFORM_ENGINEERING_READY_FOR_PROBES / DEVICE_CAPABILITY_PARTIAL / GPU_RESIDENT_PRODUCT_NOT_IMPLEMENTED`。不使用真机，不改业务代码、设备设置或生产开关，不重跑测试。所有渲染含背景/前景只能走新 XComponent+shader；故障返回 FAULT 或明确 STALE 的完整上一有效新管线帧，绝不 Canvas 上屏或回退。

## 1. 核证范围：已测、未测、不能外推

逐项只读核 `test/reports/t131/t131.json` 与 `docs/test-report-T131-compute-capability-v1.md`、`docs/architecture-T138-gpu-pair-prototype-v1.md`、`docs/architecture-T183-gpu-cost-next-v1.md`、`docs/test-report-T199-gpu-strict-device-v1.md`、`docs/architecture-T205-whole-transaction-v1.md`、`docs/developer-T212-tx-gpu-bridge-v1.md`，以及当前 `test/p013_compute/pair_executor_v1.cpp`、`entry/src/main/cpp/p013renderer/egl_context.cpp`。T131 原始设备身份是 VYG-AL00/Mali B318、GLES 3.2/GLSL ES 3.20；整数 map 的 310es shader 已编译/派发，192/512/4096逐元素一致且重派重复；**原始 JSON** `limits` 记录全局 SSBO bindings16、每 compute stage SSBO blocks16、单block 1GiB、shared32KiB、group count每轴65535/size1024/invocations1024。正文报告省略了global bindings数值，不能因此把原始已测项写成未知。T131 **没有**证实跨 context共享、数值 F64/FTZ/rounding、浮点原子、GPU timer、驻留全链或 compute→XComponent直接可见。整数 pair 的后续真机证据仅覆盖其原型范围，不能把 CPU merge/审计/发布算作 GPU 已迁移。

| 项目 | 当前证据与严格结论 | T231 后续取证 |
| --- | --- | --- |
| GLES/compute/SSBO | T131设备 GLES3.2、310es整数核、全局bindings16/compute blocks16、单block/工作组上限；旧pair实际六binding且pbuffer私有context | 在XComponent真实context**重查**上述已知值与每stage/combined blocks、`GL_SHADER_STORAGE_BUFFER_OFFSET_ALIGNMENT` 的同run完整原值+GL错误；按T230最终layout逐pass核，不因两个16数值碰巧相同就混用枚举/上限 |
| dispatch/barrier/fence | T131 `glFenceSync+glClientWaitSync`/map真实运行；旧pair build/query/reduce有逐dispatch `GL_SHADER_STORAGE_BARRIER_BIT`、尾 `GL_BUFFER_UPDATE_BARRIER_BIT`，每请求尾同步读128B | 新整步多pass pingpong、compute→vertex/texture/SSBO消费的正确barrier组合、无每merge同步、最末只读有界证书；先核输出再谈性能 |
| 整数/浮点原子 | GLSL ES 3.20 标准的 buffer/shared `atomic*` 原型是 int/uint；旧pair不以浮点原子加质量 | 用整数竞争/collision/repeat探针核实际排序与容量；若T230要浮点原子，必须查明确extension并编译执行/精确差分；**不能预设存在或用乱序float加法满足原账** |
| FP64/F64仿真 | T138/T205的权威RN64准备/merge/guard仍在CPU；GLSL ES3.20核心把`double`列为保留字，现无设备厂商扩展或仿F64数值轨迹证据 | 查实际extension/driver，原生double若无明确扩展则记CORE_UNSUPPORTED；以整数word仿F64/定点候选分别编译、运行，验证±0、subnormal、溢出、ties、串并归约次序、FTZ/舍入/收缩；不能断“GPU整步不可行”或静默改阈值 |
| GPU时间/收益 | T131是CPU提交/fence等待/读回分列，非GPU kernel timer；T199严格O0/O2 设备 GPU旧pair臂没有可测attempt收益，仍慢于LEGACY，GPUtimer未测/UI竞争未跑 | `GL_EXT_disjoint_timer_query`/等效能力与disjoint状态真查；计CPU enqueue/dispatch/fence/submit/呈现及GPU有效query，缺项 `NOT_MEASURED`，不得从CPU fence等待倒算GPU核时 |
| 资源/恢复 | 旧pair `eglCreateContext(...,EGL_NO_CONTEXT,...)` 的独立pbuffer、owner thread/epoch、双slot隔离与新PID耗尽恢复有证；renderer `eglCreateContext(...,EGL_NO_CONTEXT,...)` 创建独立window context | 同context compute→draw；可选共享context对象/fence；context loss/resize/background/重建后对象名与旧帧token失效、资源真实释放、内存峰值和设备竞争均需新证 |

T131原始 `gpuFenceWaitMs` 热档 p50约0.27–0.32ms，`test/reports/t131/t131_stats_summary.json` 只代表整数map微核/同步条件；不能乘一个猜测的merge数就当整步实测。T183设备旧route稳态 hash/pack 与 bridge 占显著时间；T199 O2没改善全attempt。T205/T212 的新隔离桥仍每请求CPU freeze/13-buffer hash、range/sort、upload、尾fence/readback、CPU写实际merge/账/rollback；T212 HOST_DOUBLE/unsigned包不是设备迁移证据。因此**驻留路线的工程目标是以一个整step事务范围装载一次候选，GPU内部跨pass推进，末尾有界诊断和渲染直接消费同代数据**，不再把每merge旧pair调用当主方案。算法正确性、数值版本、可并行范围由T230确定。

## 2. 推荐拓扑：同一渲染 GL context 先行，跨context为受证选项

**首选平台原型**：新XComponent window EGL context 的唯一 GL owner thread 上，按可用任务队列串行提交 `compute candidate passes → memory barriers → candidate verify/commit marker → render same accepted bank → full scene present/swap`。现 renderer `EglContext` 已在 GL 调用时记录 `currentOwner`，`Swap/Unbind` 拒错误线程；不能由 ArkTS物理Worker直接使用其context或GL名字。UI线程只交固定大小的输入/FrameToken/取消请求并异步收回执，不等待每次dispatch/fence；GL owner在预算内执行，长step应明确 BACKPRESSURE/FAULT或显式上一完整新管线 STALE，不暗藏UI阻塞。若当前页面架构不允许同一owner运行整step，就在独立native GL worker创建**从渲染context传入share_context**的新context并保留window context；这要求在两context创建/重建时协调实际EGLDisplay/config/share group与对象生命周期，当前两处 `EGL_NO_CONTEXT` **并不共享**。共享选项要先通过§5探针，不能仅看到相同GLuint值或context版本就判共享。

同context路径不需CPU搬运当前完整pool：cold `GL_SHADER_STORAGE_BUFFER` 固定创建权威A/候选B和event/ledger/cert/presentation缓冲；每step只更新有界输入命令、版本token，candidate B从A按T230定版流程构造，所有下一pass只读当前候选代。pass之间按实际消费者配置 `glMemoryBarrier(GL_SHADER_STORAGE_BARRIER_BIT)`；若render从SSBO取数据，则最终还要该consumer对应barrier；若把同buffer作vertex attributes，加`GL_VERTEX_ATTRIB_ARRAY_BARRIER_BIT`；若image/纹理消费则依实际写入与采样路径加image/texture-fetch相关bit。barrier解决同context命令/缓存可见性，**不是CPU/GPU完成等待**；只有CPU必须读证书或跨context时才用fence，并按实际EGL/GL同步语义等待。相关原理见 [Khronos GLES3.1 §7.11](https://registry.khronos.org/OpenGL/specs/es/3.1/es_spec_3.1.pdf)。不得在一个dispatch内假设工作组之间有全局barrier；需分dispatch/阶段。所有pass的SSBO绑定数/每block大小/总buffer字节、workgroups和`GLsizeiptr`乘法先checked，冷配置失败明确CAPABILITY_UNSUPPORTED。

共享context备选：GL owner A compute提交后写完整候选标识，flush/fence且缓存可见性正确；GL owner B render在**受证的共享资源和跨context同步**后核owner/epoch/run/step token，仅读accepted bank。两线程不得同时把同一EGLContext current；共享对象的读写时序需显式同步，[EGL1.4规范](https://registry.khronos.org/EGL/specs/eglspec.1.4.pdf)也要求应用避免一个context改buffer而另一个正用它绘制。探针须证明实际设备的共享对象、fence等待/错误、取消及context loss重建；失败则退回**同context方案**，不退Canvas、不把跨context不共享误当GPU障碍。若平台拓扑最终只能 `glMapBufferRange → CPU全pool拷贝 → glBufferSubData` 给renderer，这只能作为独立**copy成本对照或故障诊断**，不能命名“GPU驻留/零往返”；要有单独backend/version和完整帧分母。

## 3. SSBO pingpong、事务身份和有界读回

权威A与candidate B不得alias；本step的所有物理列/池/索引/水沙/源账/事件/预算/undo或重放摘要必须同一root token，不能只迁pair索引而留merge与高度场在CPU。冻结输入包括run/owner/contextEpoch/step/attempt/substep/sourceCommit/config/数值revision/输入窗口、各buffer layout字节与shader SHA；迟到/旧代输出一律不能commit。GPU candidate先产生**失败原因/首ordinal与有界证书**，主机只在完整阶段成功、必要不变量/证书按T230满足后切`acceptedIndex`，再把该bank交render。不可在尚未验明的 candidate 上交换权威指针，不能把`glFenceSync`或shader里一个“done”字当全事务正确性证明。数值选择若无法在GPU证守恒/确定选对/事务，应保留明确实验状态与CPU oracle差分，不开生产默认、不下调既有硬门。

为避免GPU→CPU读回每merge放大，普通热帧只允许**一次有界结果包**，建议固定 `StepResult256`（64×u32，header包含run/owner/context/attempt、candidateBank、stage/firstFailure、commitEligibility、计数、摘要/版本及reserved），与T230最终证书布局对齐后升版；CPU读回若需要实际完成保证，尾部一次 fence、有界wait、map这256B，并单列wait/map CPU时间。大型event/全pool导出仅在显式诊断/独立测试分支，另计字节/时延；不能因正常帧无全量读回就声称已证明物理全域等价。**若T230选的可靠提交需每帧CPU完整证书或读回更多字节**，按其真实需求升版、重算收益，不能强行缩到256B。当前CPU source/物理owner向GPU迁移之前，某些seed/admission可能仍需CPU输入；每次CPU↔GPU边界都应列次数与bytes，逐merge边界原则上不满足驻留目标。

`GpuResidentFrameToken`最少有run/owner/contextEpoch/surfaceEpoch/step/attempt/acceptedBank/sourceCommit/configRev/numericRev/layoutRev/shaderSHA/sceneRevision/尺寸/geometry/desired vs rendered。GPU物理成功和画面CURRENT是两个提交：candidate接收后才可发物理commit；完整新XComponent scene所有层用同accepted token并真实swap成功才发CURRENT。失败时返回FAULT或与旧token相符的完整STALE；不得把新物理部分数组与旧背景/前景/水层拼接或偷偷Canvas画缺层。context lost使全部GL名字、fence、inflight token失效，owner代递增；仍在的CPU冷数据可按已证明snapshot协议重建，否则新run/故障，不把旧 GPU 对象名跨epoch复用。取消/timeout slot QUARANTINED直到确认GPU完成或context销毁，不在迟到写入时重用B。资源退役需记录实际live SSBO/texture/FBO/program/sync及峰值、失败阶段；T199新PID恢复仅证明旧pair owner边界，不代替此新renderer共存路径。

## 4. 数值能力选择必须先探针，算法与平台不循环等待

T230可离线并行设计至少两类显式数值路线：原RN64逐操作保持/仿真，以及版本化F32/定点/其他表示；本稿不预先批准任何一种为原模型等价。[Khronos GLSL ES3.20规范](https://registry.khronos.org/OpenGL/specs/es/3.2/GLSL_ES_Specification_3.20.html)将`double`列为**核心保留字**，列出的标准buffer原子形参为int/uint；所以不能凭设备GLES3.2推原生FP64或float atomic。若驱动另有可用扩展，必须列extension和实际编译运行；整数word仿F64/F32/定点的次正规、FTZ、舍入、收缩、性能也必须本机实证。尤其不同顺序的并行浮点归约通常改变原bit；GPU整数原子即使可执行也只提供整数操作的原子性，不能自动导出原F64高度/质量守恒。数值候选必须携带`numericRev`、明示支持域与独立CPU oracle/原始位级差分；受限候选若不能达到既有硬门则如实FAIL/NOT_IMPLEMENTED，不用EPS、clamp、隐式重排或renderer容差修库存。

本卡只是平台能力说明，**T230不以T231探针完成为设计前置**；T231也不等T230定最终算法才给探针。探针只回答可用操作、资源与交接；T230到件后architect另补逐pass binding/offset/事务证书/资源manifest，再转developer。若FP64原生不支持，继续评估仿F64/定点候选；若共享context不通，先用同context；只有同context整步GPU与可行数值方案均不成立，才依据具体失败证据讨论其它平台路径。

## 5. 最小真机探针矩阵与接口（交manager派tester）

下列是**新真机测试卡任务书，不在T231离线卡执行**。tester须打“真机”标签、设备串行且GPU优先；保留设备/驱动/GLSL/EGL/OS/构建SHA、所有shader源和编译/link日志、原始GL/EGL错误、每样本owner/context/surface epoch、输入/输出raw bits及独立CPU oracle；失败交architect首次归因。测试使用隔离probe target，不改生产开关/设备设置、不得把旧pair探针改名冒新覆盖；仅新XComponent+shader含前背景，FAULT/完整STALE，禁Canvas回退。可在同一卡中按阶段取证，阶段A的失败不阻T230离线设计。

| Probe | 最小构造与必须保存的断言 | 作用 |
| --- | --- | --- |
| P1 同一context接线 | 在XComponent当前window context查询完整GL限制/extension/function pointers；创建两个32–256B SSBO，dispatch写确定整数pattern→正确barrier→同context draw shader读同buffer且真实swap；再故意漏barrier的负例只供诊断，不能当规范oracle | 核最优零CPU全pool读回的compute→render可行性；记录native线程/GL owner、帧token及SCREEN/CURRENT，非仅pbuffer compile |
| P2 数值/原子 | 先查GLSL/extension，核心`double`预期编译拒绝作负例；**仅在声明原生FP64扩展时**逐项编译运行其基本运算。另跑整数word仿F64/F32/uint定点候选及raw bit、subnormal/±0/tie/overflow、收缩/FTZ；同地址整数atomic竞争/重复及浮点atomic extension枚举/编译（若方案用） | 报CORE_UNSUPPORTED/EXT_SUPPORTED/UNSUPPORTED/GL_ERROR、原bits与CPU严格参考，不以“GL3.2”代替F64可用结论。无extension时不做不存在的浮点原子PASS |
| P3 资源/边界 | N/K主档与T230拟定峰值的每passSSBO bind/block/offset alignment、workgroup数、shared、双bank+event+证书+render资源并存；定容失败/compile失败/alloc失败原码 | 用真实bytes/peak/live对象核低于各上限和设备内存；不把T131单block1GiB当可分配总量 |
| P4 同步/生命周期 | 同context多pass pingpong+barrier+最终render；若要跨线程则用 `eglCreateContext(...,shareRenderContext,...)` 建共享context、同一buffer可见、fence/等待、取消迟到、resize/background/context lost后重建；每段完整token与旧对象失效 | 选择同context还是共享；未测共享不能写已可用。保留恢复前后资源/epoch/FAULT或完整STALE |
| P5 成本/竞争 | 冷/热、K192/512、0/1/2 step、实际birth/merge密度，记录enqueue、CPU pack/upload、逐pass dispatch CPU、barrier、fence wait/map、可用GPU disjoint timer、render submit/present/swap、完整frame和UI竞争；每组原始样本+重复 | 比较GPU驻留候选与T199旧pair/CPU对照的**同输入完整帧**；GPU timer无效为NOT_MEASURED，不从p50分项相加或排除失败帧冒收益 |

建议隔离probe入口：`probeResidentCapabilities(contextKey, outLimits)`；`probeSameContextHandoff(frameToken, seedWords, out)`；`probeNumericVariant(numericRev, rawFixtures, outBits)`；`probeSharedContextHandoff(renderContextKey, out)`；`probeLifecycleFault(kind, expectedEpoch, out)`；`probeResidentFrameCost(fixtureId, repeat, outTrace)`。`out`均预分配且带stage/rawGL/rawEGL/shaderSHA/owner/contextEpoch/byteCount/CPU与GPU计时状态；结果JSON仅归档层冷生成。独立tester可按T230新增的必需操作增补精确P2 fixture，但不因此推迟P1/P3/P4。

## 6. 接力范围

开发平台集成的唯一owner负责 `entry/src/main/cpp/p013renderer/egl_context.*`、`scene_compositor.*`、`napi_bridge.cpp`/CMake 的限定hunk与新GPU驻留模块；算法开发owner负责T230后的state/pass/数值/事务root，不与平台owner同时覆盖公共hunk。新bridge只在独立实验route冷开关/新run运行，默认生产和旧pair/T224/T225对照不改。tester先执行§5平台探针，再按T230工程manifest补全整步独立差分、故障注入、设备成本和同XComponent帧证据；effector仅在真实完整呈现后测主观效果。开发负责编译及非真机自测，真机/端到端另卡，失败先architect归因；manager按报告登记与派工，不新增验收门。本稿工程完成只证明**探针任务可执行、平台边界清楚**，不证明GPU驻留产品可行/收益/生产迁移。
