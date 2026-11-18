# T613 目标13旧GPU资产复用与迁移边界

2026-11-18，architect，不使用真机。盘点范围为M1/M2/M3/G9/G11/G12的源码、构建、冷证据、资源及scene；本卡完成资产分流和无侵入接缝，**不选择目标13模型/离散，不实现求解器**。T605决定模型与数值策略，T607据正式设计完成工程。目标1–11内容、状态、优先级保持，目标10继续暂停；水线严格13纯流场→14给定沙边界影响水→12水载荷影响沙并反馈。旧T583/T595/T602/T603/T604暂缓，旧代码/报告不删除、不回退。

结论是复用**工程基础设施及其约束**，新建独立流场状态、求解过程、证据字典和呈现接线。旧水量列高、包运动、孔隙压力或沙表层marker不能通过改名成为速度/压力/黏性求解。各条目的精确文件、符号定位、原始字节SHA与本轮工作树版本见 [机器清单](architecture-T613-g13-reuse-manifest-v1.json)。I01–I26是本卡资产索引，不是新增执行卡。

## 版本与构建边界

机器清单保存snapshotHead、86个文件的字节收据和符号定位、跟踪文件的工作树差异。**工作树SHA是这次盘点的版本**，不能只拿HEAD声称未提交修改已被该commit包含；历史生成树/签包/设备实载另有身份链。符号存在仅说明该文件含此实现，不等于已绑定运行入口。

| 旧路线 | 可定位版本与实际落点 | 复用判定 |
|---|---|---|
| M1 | `m1/layout_generated.h` revision2/features3；`FullM1Transport`、`m1::Run`与scene；另有SOURCE-only `m1::Plan` | 生命周期/统计可作模板，水物理专属。`Plan`仍返回ProtocolReadyShaderNotImplemented，不能因此把独立FullM1Transport也误写成全无实现。 |
| M2 | `m2/layout_generated.h` wire/layout/algorithm/source3、medium2；`RootGate`/SOURCE128/merge；隔离生成route=m2 | 真实旧SOURCE/合并资产，非流场。canonical bridge仍按GRS_APP_M1选择M1；必须追隔离overlay，不能仅看目录名。 |
| M3 | m3a V4、m3b V5；`ControllerV5`/`TransportV5`/`LineageProfile` | 状态/来源/事务观测模板；lift、occupied、S/F/P/J、来源段与budget属旧模型。V5 FULL T596仍在途，不作目标13前置。 |
| G9 | `g9spf`旧SPF R1；`g9dm` G9_SPF_DM_Q20_V2，schema里的kEngineeringSha还是construction标签，非真实内容SHA | 分层资格、owner/lease可参考；DM动员与六J试探等是沙模型。版本身份用清单实际SHA，禁止把标签当密码学摘要。 |
| G11 | T563/T566 VisualConfig64V1、G11R/G11D组合profile | 已有accepted-material-plan marker与glyph源码，不等于流体粒子追踪或速度场；cold真实数据导出仍缺实现。 |
| 旧G12 | G12_POROUS_SPF_Q20_V1/T567/T576；`g12water::Transport`、water_core及生成器 | 旧孔隙/沙水耦合的部分实现。多湿writer显式NotImplemented，不能复用为新13或新12已完成求解。 |

I01/I02的构建图有实质差别：canonical `gpu_resident/CMakeLists.txt`直接含m3a/g9dm/g11vis/g12water，**没有直接含m3b/g9spf**；T517/T529隔离生成器分别追加它们和页面/bridge。根`cpp/CMakeLists.txt`创建grsm0/grsm1，均从app_bridge编译，只有grsm1定义GRS_APP_M1。复制整个grs_foundation或复用libgrsm1名字会把旧route/宏/字典一起带入，不是纯基础库复用。

G11/G12的CMake生成器相对路径`${CMAKE_CURRENT_SOURCE_DIR}/../../../test/...`在当前canonical位置解析到`entry/src/test/...`，实际脚本在仓库`test/`；条件EXISTS在本轮为false。隔离T599会显式预生成额外头。**这是静态构建接线风险，不是本轮重新编译的失败报告**；也不据此改正在进行的旧任务。T607给新目标独立明确生成步骤，缺头/旧SHA必须configure失败，不依赖旧缓存头或WARNING继续。

## 通用基础的复用清单

下表“复用”均要求新route/ABI/布局/实际context正确，未授予新设备资格或性能结论。

| 索引 | 具体文件和符号（相对`entry/src/main/cpp/`） | 可复用部分与风险边界 |
|---|---|---|
| I03/I04 | `gpu_resident/program_cache/{program_factory,graphics_program_factory,artifact_store}.h`：Build、BuildGraphics、Environment、ProgramArtifactStore::Configure；`uniform_locations.h`：Prepare/Find | 保已存在的编译结果持久化、串行文件worker、编译/链接/反射/warmup分段收据。新shader最终组装字节、新binding ABI/layout、link配置和实际GL/EGL环境组成新key，不能复用旧program key/GLuint或承诺跨驱动二进制可用。缓存缺失/拒绝走正常源码构建，原GL/EGL错误保留；不另建第二套缓存。 |
| I05 | `gpu_resident/napi_words_contract.{h,cpp}`：ReadWords<N>/SnapshotJson；`bridge_state.h` | typedarray类型/长度/offset/底层buffer与首错观察顺序可复用。Route只列M3A/M3B/G9，SnapshotJson显式attempt当前仍仅匹配firstFailure；新目标不能塞G9槽或假定已解决T581。新route需独立具名诊断空间/端点绑定，exact boot/generation/attempt，不通过放宽decode入场。 |
| I06 | `gpu_resident/app_bridge.cpp`：App/单owner线程/Swap；`bridge_state.h`：InvalidateLocked、CommitReadyLocked；ArkTS M1DemoFrame | 只复用owner与生命周期模式：UI提交不可变命令，GL owner做GPU工作，撤资格先于退休。App直接含M1/M0类型、32word包和16队列，不能强转流场输入；epoch+permission+window/context/run分离，禁止把旧READY当新solver READY。 |
| I07 | `gpu_resident/resource_ledger.h`：Ledger/HostScope；`resource_gl.h`：Bind及分配/删除wrapper | 复用有界逻辑分配账、host capacity/peak、context/scope/owner身份及原错保留。宏wrapper只在实际编译单元包含且绑定ledger时覆盖，不能凭文件存在称全部计账；不跨宏范围无意改第三方GL。LOGICAL_REQUEST/DRIVER_NOT_MEASURED不是RSS或驱动退休内存测量。 |
| I08 | `gpu_resident/g9spf/stage_capability.h`：StageCapabilityV2/QueryCapabilityI；M2 ProbeCurrent对照 | 复用COMPUTE/SCENE/PRESENT分域和raw pname/value/status/required。新求解各阶段实际binding、shared/workgroup/最大block/format限制由新表导出；旧4SSBO/64lane/fragment≥1不自动成为新门。vertexSSBO=0可用已验证设计形式的整数attribute路径，但compute通过不等于能呈现。 |
| I09/I10 | `control.h` RootOwner、`m1/full_transport.h` ContinueSubmit、`g9dm/{controller,transport,progress_clock_v3}.h` | 复用candidate未接受不发布、completion后原子commit、不可变lease/fence退休、取消不回滚已发GPU但不接受的原则。旧256word completion及permit/hash字段值、固定120配额/4Q/250ms/GAP属于旧profile；新13物理dt、迭代/收敛、预算由T605/T607明定，不能由旧时钟替设计。旧路线阈值保持。30fps显示不等于120fps，也不能代替求解/接受时间。 |
| I11 | `gpu_resident/cold_slot_broker.{h,cpp,cmake}`：GrsColdReserve、Lease、DetachedBlock | 复用单shared DSO的进程两slot/单builder/单reader及真实释放归还票据。新route号必须唯一且不撞旧号，routeBudget正式登记；当前全进程上限524375552B，不能复制broker到STATIC或每route自开双槽绕过。此cap不等于可用GPU总额。 |
| I12/I13 | m3b cold_capture/contract、GpuBoundedCold/GpuM3Cold/GpuG9Cold、m2/cold_reader.py | 复用冷冻结→caller-owned span→有限分页→hash/文件原件→独立reader的模式。现有V5字典、61段、SOURCE上下文不迁名；T596部分锚点已补、完整root/reader等仍未完，不能阻塞新13等旧FULL，也不能宣已可直接复用完整writer。 |
| I14 | `p013renderer/fullframe_targets.{h,cpp}`：Ensure/Commit/Bytes；`gpu_resident/g9dm/scene.cpp`：AcquireScene/ReleaseScene/CanPresent | 可复用两份GPU整帧目标与成功后换lastGood、同lease前后资格核验、整数attribute scene模式。FBO成本现为8×width×height payload，实际格式/对齐/驱动另计。新流场字段采样、色标、矢量/等值图及token全部新ABI，旧S/F/P偏移不能直读。Swap成功与displayPresentVerified分域。 |
| I25 | `gpu_resident/integer.{h,cpp}`、integer_primitives.glsl、g9spf/wide_integer.glsl | 只复用checked加乘、宽整数、RNE等基础算术的已定义语义，前提新设计选用相容数制。旧Q20/Q22/Q42、能量换算与整数政策不能替T605决定流体精度或量纲，也不要求新求解强行整数化。 |

## 旧数值假设与骨架的隔离

I15–I19保源码为历史/参考，不直接进入目标13物理依赖：

- M1 `water_edge.comp`以列库存/高度差、kappaW、奇偶边proposal搬运水量；`quantum_motion.glsl::resolvePacket`按AIR/WET、终速、列几何/水面处理包。它们没有因此提供目标13要求的独立速度/压力/黏性解，不能把高度代理改名压力、把水glyph当体积流场。
- M2 SOURCE128/free-slot/merge/origin及`ResidentBufferBytes`围绕固定N128/K/Kw和旧packet/state/publication；可参考有界队列/合并的实现技巧，不能带入新流体网格/边界或强制新场只有128列。
- M3 V4/V5的BULK/孔隙、lift、occupied求交、来源physical/provenance、E/Ea事件预算属旧沙/水包模型。守恒/来源审计思想可用，新场有哪些守恒量、边界通量、离散误差由T605明确，不能拿旧逐word轨迹等价当流体可信性。
- G9 R1/DM v2是S/F/P库存、表层动量、沙包飞行、摩擦/沉积/动员及六J试探/拒绝前缀/重放。其`pressure`/energy等字段不自动是新目标的流体压力/黏性应力；DM的最大可行整数q不是流体压力求解器。

I20（T599在途）已删除mapping授clock、补native CLOCK_MONOTONIC与256word存储，但真实成功Swap映射、页面→INPUT128→消费、完整冷双record仍未闭合。新目标如需要真实手势/边界输入，必须保存真实producer/单位/epoch/事件身份；不先依赖该未完链，不用Date.now或调用计数伪证。目标13不借此提前做沙轮廓反馈。

I21/I22 G11的marker由父沙accepted material-plan份额驱动；用于沙表层观测。给目标13画可视化时，不能用这些marker计算或证明真实速度。`g11vis::Cold::Read`目前只返回窗口元信息，没有真实word数组/不可变GPU快照；声明PAGE不是完整冷证据。其scene token主要由版本/尺寸组成，不能替新状态commit/lease身份。

I23/I24 旧G12明确不作为新水求解后端：

- `test/t576_generate_pipeline.cjs`生成g12NotImplemented并让DragHalfA/B、PressureA/B、WaterFace、CoupledForceReserve、WaterOriginReserve、CoupledFlowApply、CapturePlan、ArrivalPlan/Apply走故障；不是仅待执行验证的完整湿模型。
- `g12SourceWetGate`直接true；water_core的HYDRO取常量压缩性/简化湿长度，`g12JFlight`未写回完整轨迹。已存在H/J库存、压力命名和32pass表，不能证明其数值定义或writer已实现。
- `g12water::Cold::Read`仅返回id/segment/offset/words等元数据，Slot没有完整冻结数据；`Scene::Draw`沿旧porous布局显示，成功尺寸token不是同lease呈现证明。新13既不能用此cold冒原件，也不能继承湿角/孔隙/沙反冲限制作为纯流场设计。
- 这些是旧资产适用边界；不在T613修旧G12、不恢复已暂缓T602等工作、不提前展开14/12。

## 目标13的无侵入接缝

下列为T607须落实的接口职责，**不是已经新增的endpoint或已冻结物理schema**。模块路径建议`gpu_resident/g13flow/`及独立页面/类型声明，由T607根据T605落地最终命名。禁止把新profile塞到G9/G12旧code、reader或magic中，也不允许新页面覆盖现有默认route。

| 新接缝 | 输入/输出、权威owner及可复用API | T607须补齐的正式字段 |
|---|---|---|
| BuildG13Assets | 冷构建把T605 kernel与layout组装为新源/hash/ABI；复用现有CMake读字节与SHA方式 | 新target、生成依赖闭包、重复符号检查、实际NAPI property表、库/页面路由；独立最小依赖，不顺手抽取/重构所有旧route。 |
| ConfigureG13 | GL owner只接合法config/seed/边界描述的不可变copy；调用现有Build/BuildGraphics/Reflect/Prepared与Store | 模型/离散/精度/单位/网格/边界/参数/seed版本和SHA、context、冷配置完成结果。ConfigReady不等于已求解/已呈现。 |
| SubmitG13 / PollG13 / AcceptG13 | native owner收命令；提交给新GPU求解模块；sealed完成记录核身份/数值状态后原子发布 | 新permit、root/step/迭代身份，真实dt、收敛/非收敛/预算拒绝，candidate/current与被接受物理时间；具体方法、收敛阈值由T605，不沿旧sort/试探表。 |
| AcquireG13Scene / ReleaseG13Scene | 只租借已接受的同版场；GPU读新字段并画到FullFrameTargets，成功Swap后记录PresentedMapping | lease/version、context/surface/geometry、状态commit、frame、色标/量纲与有效性；取消/失败FAULT或标stale的旧有效新管线帧。禁止Canvas背景/前景/回退。 |
| BeginG13Cold / ReadG13Cold / EndG13Cold | GL owner冻结该commit的真实GPU场及求解/输入摘要；GrsColdReserve先预约，caller-span分页，文件worker落盘 | 新schema、段/单位/网格位置、坐标、速度/压力等设计字段、边界/config/seed/hash、solver残差/迭代/通量与时间、完整性/首错。不能返回仅元信息的PAGE当数据，也不把旧FULL61段套新场。 |
| G13Resources / G13Capabilities | 统一登记新分配、退役、场/临时workspace/scene/cold/cache等同时存活峰值，查询当前设备真实门 | T605确定未知数/迭代后才定资源公式/dispatch/bandwidth/预算；DRIVER_NOT_MEASURED保真，检查最大单block/尺寸乘加/对齐溢出。旧K/Kw、894MB并存估计或某profile快慢均不外推。 |

CPU仅承担既有控制/冷IO/证据搬运，**不新增CPU求解、优化、oracle物理stepper或产品回退**。T606按用户已派独立解析/制造解基准定义可信性，不从旧输出反求期望。渲染观察30fps即可；数值时间步与迭代速度、冷编译/缓存加载、GPU工作、native提交/等待、draw/Swap分别记账，不能用帧率掩盖求解落后或未接受结果。无侵入复用不等于没有新实现工作。

## 证据范围与接手

I26的T579签包流程、现有安装/进程/ELF/SHA设施可以沿用，新任务重新绑定自己的source/HAP/ELF/配置/输入和执行scope；不将旧M2修空日志PASS、旧G9设备探针或本地DM反例PASS外推新13。原T461报告有NO_TRACE/未完成表述，详细Create域以已交T588为准：COMPILE完成、LINK进入未封口，绝非纯流场验证。

T605可独立继续模型/离散设计，T613不增加其审批门；T606继续独立基准。T607消费T605正式方案及本清单，决定上述新ABI/资源/状态/owner接线；T608完成实现与编译即可交付，签名部署、执行和独立GPU定量/观测由T609/T610的tester/effector沿现有设施承担，不新增开发测试门。14的沙边界/载荷接口等13可信门实际成立后由T611冻结，12反馈由T612随后设计；本卡不提前定义沙压力或耦合离散。

本卡必要检查为路径/符号/原始SHA/构建相对路径和报告适用域核对，脚本 [analysis-T613-source-inventory-v1.py](analysis-T613-source-inventory-v1.py) 只做静态盘点，不运行CMake、旧generator、物理/产品测试或真机。后续prompt必须包含：不允许修改`D:\hmprojects\HelloWorld\docs\后续工作清单.md`（仅本人行状态例外），不更新滴答同名清单；要么完整完成本卡当前约定范围及角色必要检查，要么遇到确实无法继续的具体问题才停止，局部受阻先做其它可做项。具体阻断原卡列未完/原错复现/证据/尝试/不能继续原因/所需输入前置与接手角色，成功结束待验收，失败或关键缺证看评论。
