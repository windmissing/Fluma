# T799 · DM SOURCE 双目的 GPU 复制候选

2026-11-22 · 目标 20 · P5 · architect · 离线工程合同。

**候选等待 T797 实测触发。** 显式研究臂拟用一个独立 GPU dispatch 替换 op9 后的两笔 arena copy，保留两份完整、可独立读取的副本。现有 B 中第一笔 arena copy 耗时 76.879 ms，末批首次 flush 耗时 74.366 ms，执行 fence 首次在退役观察到信号为根起后 336.187 ms。C 中重开销转到 fence 创建。GPU 分段时间仍未知。 [T792 数据与归因](architecture-T792-v1.md) · [本卡离线核对](architecture-T799-evidence-v1.json)

## T797 实施触发及排队分流

1. **先复现 B 的有效样本。** 用 T797 的实际 driver、DSO、设备 SHA/BuildId 和清晰的能力分支回执。K192/S40 固定输入、配置、物理 seed、缓存条件、设备和同一 owner 路径。在同一设备窗口取得至少五组交错且有效的重复。报告原始分段、组内差、median 和范围，并保失败、剔除原因及实际分母。用同队列、可用且无 disjoint 的 GPU timestamps 取 op8 后、op9 后且 copy 前、两笔 copy 和屏障后、op11 后的原边。另保 API wall/thread CPU、实际 flush/fence 和已排队状态。若没有可用 GPU clock，使用 T797 四个互斥 prefix fence 的信号首末观测范围与无探针的 B 对照。差分只作为有顺序和时间夹逼的估算；边界重叠或插 fence 后的提交重排足以解释差值时，记未分清。禁止把 API wall、晚退役 signal 或 prefix 差直接记为纯 GPU 执行时间。 [T797 前置合同](architecture-T792-v1.md#下一轮工程合同) · [Khronos disjoint timer query](https://registry.khronos.org/OpenGL/extensions/EXT/EXT_disjoint_timer_query.txt)
2. **触发复制候选的门槛。** 默认 `COPY_COST_UNCONFIRMED`。交错样本中，在扣除 op9 之前排队和共同截点开销后，能稳定归给**两笔 arena copy 及其特有提交依赖**的耗时，保守下界须 **≥20 ms**，且至少是重复样本波动上界的两倍。20 ms 是本合同选择的试作门槛，为原 250 ms 的 8%，用于避免为微小差值增加 GPU 程序和绑定。取有效组间 `max−min`、采样分辨率中的较大者作波动上界。前缀夹逼可用每组「copy 后 prefix 最后 TIMEOUT」减「op9 后 prefix 首次 signal」，时间均相对各自根起点，再扣可见探针干扰与跨进程偏差；缺这两个原边不能构造该下界。若靠 prefix 推断，仍须有实际 API 区间的交叉支持，能排除前序命令在首次 copy API 中代偿。达门槛后的收益及可省份额由候选对照测得，先进入 developer 候选实施与编译。
3. **若主要开销落在 compare/replay。** op8 replay 或 op9 state compare 已耗掉主要时间，copy 前 prefix 就慢，或第一笔 copy API 的时间随前一独立 tranche 改变并移入前序 fence 时，优先归到该具体 producer/pass。保 full replay、逐字 compare、原事件边和 sealing，从该段的真实 GPU/队列证据分流 architect；若要改数学或状态证明，先转 designer。若证据只支持末批通用提交/驱动阻塞，沿 T792 的同队列提交诊断推进；A/C 已显示重开销可能跨调用转移。无法分清时由 effector 补相应端点及分母，候选保待触发。

## 缓冲身份、完整字节和 program

在实际 DM `Transport::Dispatch` 的 `SOURCE_COMPARE` 成功提交、屏障返回以后，在 `SOURCE_PACK_SCENE` 前复制。原 op8 及 op9 的数学、事件、pass 游标和 shader 字节保留。往两个目的各写完整原字节，范围为 **`[0, profile.arenaBytes)`**，包含有效事件、尾后区域和 padding。原编号 0 的 `CURRENT → BASE` 完整 state copy 继续走 `glCopyBufferSubData`，只替换编号 9 的两笔 arena copy。 [原 DM 绑定及复制](<D:/hmprojects/HelloWorld-T771/entry/src/main/cpp/gpu_resident/g9dm/transport.cpp#L785>) · [原 shader compare/audit](<D:/hmprojects/HelloWorld-T771/entry/src/main/cpp/gpu_resident/g9dm/pipeline.comp#L1996>)

| 固定接口 | GPU 源对象 | 目的对象 / 附加条件 |
| --- | --- | --- |
| 独立复制 program 的 SSBO 0/1/2 | 0 为 `buffers_[6]` ACTIVE_EVENTS，只读 | 1 为 `buffers_[7]` SELECTED_EVENTS，2 为 `buffers_[8 + permit.arena]` AUTHORITY_EVENTS，均只写。`permit.arena ∈ {0,1}`。两个目的使用不同有效对象和不相交物理存储。 |
| K192（首轮设备基线） | 12,767,232 B / 3,191,808 u32 | 每目的相同长度，797,952 个 16 B 向量，local size 64，groups X=12,468。编号 0 state copy 保 591,872 B。 |
| K512（范围覆盖及后续独立验证） | 23,334,912 B / 5,833,728 u32 | 每目的相同长度，1,458,432 个 16 B 向量，local size 64，groups X=22,788。编号 0 state copy 保 673,792 B。 |

按生成的 `Profile` 计算字节数、`uvec4` 数和向上取整的组数。std430 下每项四个原 u32，单 invocation 只读一个向量，并以原位写入两目的各一次。bound 为与实际绑定范围核对的向量数，超 bound lane 直接返回。对整数乘加、`GLsizeiptr`、offset、采样长度和实际 `GL_MAX_COMPUTE_WORK_GROUP_COUNT.x` 做检查，核 storage block 数量与单块范围、lane/local size 和 16 B 对齐。三个原 buffer 在已有配置阶段独立分配，首版只允许原 owned buffer，排除 imported/shared backing。对象、物理分配、范围、context、owner 及根 `arenaGeneration` 在提交前一致。0 对象、重复对象、相交 backing、错 arena、被 cold/退役持有、范围短或偏移非零均拒绝；不因布局数值相同替换身份。无需新正式 arena、配置期搬运或 CPU 复制。

候选 shader 为独立 program，配置期编译/链接并验实际三个 block、SSBO binding、数组 stride、计数 uniform（Vulkan 对应 push constant）和 local size。记录编译输入原字节、ABI/后端、SHA、实际反射、program/映射及 cache key。单元完整编译输入 **≤49,152 B**。辅助 program 身份进入显式研究变体的执行计划和证据，不把它填入原 `Permit.shaderSha` 或 GPU completion 的 SOURCE SHA。只有显式选定、工件/能力/布局验证通过才开始该候选根；任何不满足在研究入口明确拒绝，默认 B 仍用原 program 和原接线。 [T727 Shader 字节门](architecture-T727-v1.md) · [T736 SOURCE 身份](architecture-T736-v1.md)

## 调度、可见性和事务

1. **GLES owner 顺序。** 同一真实 owner/context 内，先成功提交 op9 和原 `GL_SHADER_STORAGE_BARRIER_BIT | GL_BUFFER_UPDATE_BARRIER_BIT`，再切到复制 program 并绑定三个精确 SSBO range。检查当前 permit、root deadline、真实 pulse/receipt、对象及范围后提交一次复制 dispatch，紧跟相同两位 visibility barrier，检查实际 GL/EGL 错及 post-guard。恢复真实 SOURCE program 与完整四个绑定，再由原 bind/统一字段路径执行 op10、op11。op10 与 op11 只在当前根的两份副本已提交且 barrier 成功登记时可推进。原执行 fence 排在 op11 和对应屏障后，覆盖辅助 dispatch、两份写入和原封样。后续 map/cold 按原 fence 和 buffer update 可见性。禁止额外 `glFinish`、同步 query、根内整 arena map 或借上一代 fence。 [GLES 3.1 §7.11 shader memory 与 §4.1 sync](https://registry.khronos.org/OpenGL/specs/es/3.1/es_spec_3.1.pdf)
2. **保原分批。** 8 ms 在原单调时钟检查点复核，每 Pump 最多 10 个真实 dispatch（包括辅助）。若提交 op9 后本批 credit 或 soft wall 不足，将复制保为当前根的待提交阶段，正常 flush/yield；新 Pump 重过 guard 并先做复制，再允许 op10。单次驱动调用实际跨过 soft wall 如实记录，原硬时限不得超过。op9 已提交后不重跑，复制已提交后不重复发；分段前后的同一 permit/对象保持冻结。因辅助 dispatch 使批边移动时，同时记录 cursor/flush 实际路径，必要时给 B 增设同一 op9 截点诊断对照，将分批效应与搬运效应各自量出。
3. **保 typed 与退役。** 原算法 pass/屏障计数仍为 12/12，原 BASE API copy/barrier 为 1/1，辅助 dispatch/barrier 另计 1/1。物理 dispatch 总计 13，arena 等价输出为两份，零 arena `glCopyBufferSubData`。T773 的原 B reader 维持现有 `3` 笔 copy 校验。新变体通过版本化 native/reader 证据校验真实 aux op、绑定代号、独立目的、dispatch/barrier、SOURCE 恢复及最终 fence，候选缺证即不可进 map/typed。原 Completion256、1024 B 原 tail 和所有数学/事务判据继续校验。根失败、窗口换代、取消或 fence 晚到后，已提交的 op9 或部分写入只走原退役，冻结三 arena、相应对象和 program 直到同队列真实 fence 到位。禁止此时在同根改走 B、重放或释放/晋升；zero commit、zero quanta，FIFO 按原拒绝分支留存或合法取消。
4. **Vulkan 对照边界。** 当前优先用 GLES 检验已有阻断，已有合格 Vulkan 同族 SOURCE 与真实队列证据时可单独比较。用同三 storage buffer range、实际设备 workgroup 和 range limits。op9 后建立 compute shader write→compute shader read，复制后对两个目的建立 compute shader write→实际后续 shader 或 transfer/readback 的访问依赖；提交 fence 与真实 buffer staging/readback 链分别观测。若改用 `vkCmdCopyBuffer` 保两条完整 GPU copy，映射 compute write→transfer read、transfer write→实际消费者；同队列初版、无跨队列 ownership 切换。Vulkan 实际 SPIR-V、ABI、缓存、信号和 oracle 归该后端。 [Vulkan 同队列同步合同](https://docs.vulkan.org/spec/latest/chapters/synchronization.html) · [项目后端取证](architecture-T731-v1.md)

## 等价性、负控及收益测法

| 核验 | 接收条件 |
| --- | --- |
| 整 arena 原字 | 独立非计时验证轮在实际 GPU op9 后同一冻结域采 `ACTIVE_EVENTS` 以及两目的全范围，利用复制后的同队列 fence 安全读原字。沿现有 `CaptureBuffer`/cold 路径做只读取证，必要时在独立 prefix 轮保 op9 原边快照。验证 `src == dst1` 与 `src == dst2` 两条实际逐字账，记录原件、各自 SHA、长度、首个差异索引与实际身份。全范围包括未使用尾段，但跨进程未初始定义的字不比较假定零值。独立覆盖用例给源全域确定非零模式，对两目标用不同毒值，保首尾和最后向量越界负控。计时轮禁止加入全量回读、毒值初始化或额外 fence。 |
| 实际正 SOURCE | K192/S40 的原 op0–11 和独立 replay 同在真实根，保最近合法 guard、期限内真实 signal、WORK 1024 B map/unmap、typed proof 与原冷档解析。T736 正样本为 `ROOT_BEGIN → SOURCE_BIRTH → ROOT_END`、admitted **209,715 Q20**、commit+1、quanta 不增、FIFO 恰消费一次。身份、时戳等动态字段各自按本根验证，间接数据不能给规则放宽。 |
| 别名和范围错误 | `src=dst1`、`src=dst2`、`dst1=dst2`、不同逻辑名指向重叠物理范围、错误对象或代数、低一向量/高一向量、错误偏移、未全覆盖和调换两路。危险绑定在发 GPU 指令前拒绝；缺写和错路由在原字/证据账检出。两目的内容本应相同，所以调换目的仅靠数据无法检测，必须核物理对象和预期 binding。 |
| 信号与事务错误 | op9 后取消、复制提交后无 barrier、重启后误用旧 pending/句柄、部分写后 GPU/GL 错、复制提交时过线、真 signal 后 pre-map 过线、根或 arena 代号变化、错误 completion/重复或 q0 沿原判据。受控错序和缺 barrier 由接线/证据链显式拒绝，不能依赖设备每次复现脏读。所有拒绝均无 typed 晋升和新有效帧。 |

**收益按全根测。** Effector 使用同设备、实际部署字节、相同配置/seed、缓存条件、owner 与 B 轮询，按 B/候选交错的新进程逐组对照。单列新增 program 的 compile/link、反射和暖加载。在无全量采集的计时根保 `rootBegin → 实际首个执行 signal → map → typed → commit`、owner CPU、copy/辅助 dispatch/barrier、末批 flush/fence 及实际分批形态。失败根用最后一次真实 TIMEOUT 和首次退役 signal 夹逼，不把根终止墙时或观测上界当完成时间。报告每组原差、median、范围与成功率。收益成立须有可重复的同根准入正例，并在可比范围内带来全链净缩短或明确提高期限内成功率。观察到开销移入 fence 或启动编译时，按实际链路单列，纯 copy/span 变短只记局部变化。固定 **250 ms 根期限、33,333,333 ns pulse freshness** 和原 receipt 约束。GPU 驻留、CPU 停投、产品实际呈现沿 XComponent+shader 并单独取首个有效 Swap。 [原读取与冷档对象](<D:/hmprojects/HelloWorld-T771/entry/src/main/cpp/gpu_resident/g9dm/cold_capture.cpp#L17>)

T799 离线复算了两个 Profile 的完整覆盖，主机模型的两目的逐字校验及错别名/短范围反例通过，临时 GLES 和 Vulkan 复制样例通过 glslang 语法与反射检查。临时样例只用于 ABI 复核。本轮未使用真机，未生成项目候选工件。 [复核数据](architecture-T799-evidence-v1.json)
