# T686 · G13 错误 29 的真实失败边界与工程修订

2026-11-20，architect。T654 v2 的完整状态和页原件已把失败锁定到 **配置票据216的 CAPABILITY:EXIT**。T672 实际生成源码给 `glGetShaderPrecisionFormat` 传入了 `GL_COMPUTE_SHADER`，GLES 3.2 §7.12 对这个入口只允许顶点或片元阶段，非法枚举会产生 `GL_INVALID_ENUM`。本轮原件的 rawGL=1280、precision=0 与该错误相符。先修 G13 能力查询参数和证明来源，随后再测真实 compute 数值资格。[Khronos OpenGL ES 3.2 规范 §7.12](https://registry.khronos.org/OpenGL/specs/es/3.2/es_spec_3.2.pdf)

| 原件 | 可登记结果 |
|---|---|
| status-before | boot287785986262658，同页初始 WINDOW_AVAILABLE，UNCONFIGURED，lastCommand.attempt=0，capability=null。214是 status 的 API 调用序号。 |
| r1-status | 同 boot，FAULT/error29，lastCommand=g13Configure/REJECTED/216。firstFailureAttempt=queuedTicket=216，handledTicket=0，lastProgress=CAPABILITY:EXIT。217是 status 调用序号。该失败路径没有正常完成 handled 确认，不能用0倒推出未进入 GL。 |
| page0..3 | 原JSON SHA和严格 base64 后 8192B×4 的独立 SHA已核，offset为0/8192/16384/24576。control 含 boot287785986262658、active/queued216、queueSerial3、cancelGeneration4。stage1 DECODE_OK，3–14在真实 window387041308736、ownerTid53071 建立EGL，到 seq15/16 在 context387369110592 进入/退出 CAPABILITY，EXIT rawGL1280，无 PROGRAM_BUILD。 |
| 缺件 | 本地只有六份文件，无 configure.json、前后page3锚、index。原始 Configure **同步返回**未保存，可从下游状态和同票据stage确认实际消费、不能代填同步返回或声称全包采集成功。四次 sampleSeq16只计stage序列，不能授权原子拼接。 |

page0开头的216、512、128、512来自 `InitDecodeFields` 的 ticket/长度与 backing bytes。这里的128是 typed input 元素规模，GL数值从同票据 r1-status 的 capability 读取。blocks16≥4、bindings32≥4、invocations1024≥64、laneX1024≥64、shared32768≥4096、maxBlock1073741824覆盖现有五个 profile 的 WORK 上界2331392B。precision=0 是调用前初始化，在无效查询后不能解释为实际 compute 精度；rawGL1280使现有 gate 返回29。CAPABILITY末尾一次 glGetError 聚合多个查询，不能证明该错误是唯一可能的GL错误，但源码中的非法调用已经独立成立并足以解释本次拒因。配置只走到能力拒绝，后续程序/可信流场/呈现均未到。

## Developer 工程边界

1. 更正 `entry/src/main/cpp/gpu_resident/g13flow/transport.cpp::Transport::Configure` 的生产实现和实际 T672 生成基线。CAP_COMPUTE 保留六项资源值、当前真实context、现有GL错误门和 profile WORK 预算。停止用 COMPUTE 枚举调用 precision-format。若原有 API 需保浮点声明查询，用同context合法 `GL_VERTEX_SHADER, GL_HIGH_FLOAT` 取得真实 range/precision 和 rawGL，记录 `precisionSource=VERTEX_HIGH_FLOAT_DECLARATION` 与阈值23，仅作声明预筛；区分未测、真正低精度和错误返回。不得把顶点读数或 GLSL 规范常数报成 compute 实测。已有 `floatPrecision` 若继续输出，附具名来源，错/无结果输出显式未知，保旧记录解释。
2. 独立 CAP_FP32 保持 T607 数值资格。声明值合格仅允许继续真实 compute Build/Reflect/数值证明所需的准备步骤。只有当前真实 compute 产物在同context的受控 `highp` 计算和读回证据，覆盖 T607 精度/有限性/次正规策略和重放，才发布 CAP_FP32 合格。沿既有同页数值验证与 accepted-field 准入；缺相应实现时先登记真实缺项并把相应 gate 保为 UNKNOWN/NOT_MEASURED，别填23或跳过数值门。CAP_MATH、CAP_SCENE、Swap 保独立。若实际现有顺序缺隔离的精度证明，须在发布可信 accepted 场之前使用相同 compute 路线的有界设备证明，输入/输出与现有合成验证隔离，不当生产水力提交。
3. 保留原 rawGL、firstFailure、ticket和阶段边界。修订后出现真实非零GL或阈值失败继续 fail-close，字段清楚指向实际检查阶段。诊断值沿现有 capability/status 和8192×4页出口，保持已有1984B能力JSON、16KiB记录及诊断页总长预算。实际生成件+编译与模块 SHA/BuildId归developer。签包、离线 parser 与检查脚本归tester，设备不作为开发结束前置。

GLES shader highp 的存储精度声明和运算行为有不同资格，后者继续按本项目数值合同测量。[Khronos GLSL ES 3.20 §4.7](https://registry.khronos.org/OpenGL/specs/es/3.2/GLSL_ES_Specification_3.20.html)

## 最短续证

先取原 T672 filesDir 同目录漏导出的 configure.json、r1-anchor-before/after、r1-index 或 r1-failure，记录存在性和原字节 SHA。原代码在 Configure 前 arm，随后直接把返回原串保存到 configure.json，诊断轮按失败216调用前锚/四页/后锚；六件拷贝不能确定是错版本、漏拉文件还是写失败。优先比对加载包/实际页及文件索引。没有产品冷读缺口证据，继续复用已有接口。

修订包实际可用后由 effector 串行进入同一 G13页，等真实 XComponent 就绪，只发一次 Configure、立刻保存完整同步返回和同boot/attempt，保持失败窗口或已配窗口，不重置、不跳页。读取 status 和按真实Configure回执/firstFailure/current ticket选择的前后锚、0..3页及原API错误。tester用T672 analyze及证据SHA离线核对，前后锚相等也只称 ANCHORS_EQUAL_NOT_ATOMIC。保最多三轮、status≤4、单记录≤16KiB、目录≤1MiB；阈值及结果按 CAP_COMPUTE→FP32→MATH→SCENE 区分。已有 CAPABILITY 失败原值可直接交工程，不因缺锚强制重复同样失败。

本卡不使用真机。要么完成本卡当前约定范围及本角色必要检查，要么遇到确实无法继续的具体问题才停止；不能做一点就停下。不允许修改 D:\hmprojects\HelloWorld\docs\后续工作清单.md（仅本人状态例外）。GPU驻留，CPU停投，目标10暂停，水线13→14→12。仅 XComponent+shader 上屏，失败 FAULT 或明确 STALE。
