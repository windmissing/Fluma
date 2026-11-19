# T682 · M2 单 API 链接开放调用与对象来源

2026-11-19，architect。依据 T590-v3 的 c8434b96 原件、T663 真实生成源码及交付、T591-v3 原始采集和 T678 工程裁定。完成原件 SHA、签包/ELF、着色器字节与事件时序的静态核对。机器路由及逐项证据见 `architecture-T682-contract-v1.json` 和 `analysis-T682-evidence-v1.json`。

**T590 的单 API 游标记录 `QUANTUM_FINAL_V3` 的 `glLinkProgram(4)` 在采样时开放约 20.568s。T591 的 40.141s 属于另一次 `SOURCE_V3` 的复合 LINK，具体单 API 未采到。** 同一份 T590 原件还记录 `SOURCE_V3` 完成复合 LINK，用了 73.432s。本轮确认冷创建有多程序长耗时，单 API 最终返回、驱动内部原因及永久死锁均未建立。

## 原件与对象身份

| 观测 | 本次身份 | 已捕获边界 |
|---|---|---|
| T590-v3 | bootSerial=1，epoch=3，window=387040645056，EGL context=387373651584，ownerTid=43807，candidateRun=1，surface/contextGeneration/geometry=3，config/owner=1 | `current.stage=LINK`、object2/program4、stage enter=286807760186096ns，exit=0。cursor revision1/majorStage5/api6/edge1/sequence71，begin=286807760186721ns、end=0、returned/rawGl=-1、32B保留区为零。cursor五项身份和 Attempt 一致，sealed=false、nativeEndNs=0、firstFailure=null、dropped=0。 |
| T591-v3 | bootSerial=1，epoch=2，window=387041997984，context=387048859216，ownerTid=4360 | 末次 LOCKED_TICK 是 **object0/program2，SOURCE_V3，复合 LINK**，inCallDuration=40140833750ns。object1/2 源身份仍空。该复合阶段可覆盖 Link、LinkStatus、GetError 及阶段收尾。该次终态未采到。 |
| T590-a2 | T678 的 pid14871、boot attempt3、epoch3、ownerTid15134，windowId为bootSurface代理值 | ProgramEnter 后原件结束，具体对象、单API及持续时长未采。 |

T590 序列化时刻按模组公式从 stage enter 加 inCallDuration 反推，得到 286828327869065ns。阶段 ENTER 比 API ENTER 早625ns，换算的 API 开放时长为20567682344ns，整个创建从 nativeBegin 到采样为124764355782ns。20.568s 是一次截尾观测的下界，文件名墙钟和 collector 窗口均不用于给 native 计时。该创建尝试尚无成功返回/资源发布原件，root 的250ms期限有独立起点。

真实 `m2/create_diagnostics.h` 的 `Build` 对每个 object 先写源 SHA 和 `strlen(source)`，随后调用 `glShaderSource`。在 COMPILE 阶段依次执行 `glCompileShader`、`glGetShaderiv(GL_COMPILE_STATUS)`、既有 `glGetError`，再落持久阶段事件。进入 LINK 前已走过 compile log、`glCreateProgram` 与 `glAttachShader`。LINK 的单 API 顺序是 `glLinkProgram`、`glGetProgramiv(GL_LINK_STATUS)`、既有 `glGetError`，之后另记 link log。

| object 和实际输入 | source SHA / bytes | 已完成 COMPILE | 已完成 LINK |
|---|---|---|---|
| 0 · SOURCE_V3，`SourceCoreTransport::CreateUsing(FullSourceKernel())`，shader1 → program2 | `b7d0a027128811db429440dec0cb94c77c8c26167fc7b507c611ffb3158606a1` / 85839 | 3.966885s，success=true/rawGl=0 | **73.431850s**，success=true/rawGl=0，有 REFLECT 和 PREPARE EXIT |
| 1 · QUANTUM_INTERMEDIATE，`QuantumCandidate::Create → QuantumKernel()`，shader3 → program4 | `54946590ca762cc8d9bb2e153e3fbab839b74bdf70fd474ad7b94b621f580f96` / 26429 | 3.118146s，success=true/rawGl=0 | 11.883292s，success=true/rawGl=0，后续分配和 REFLECT 已返 |
| 2 · QUANTUM_FINAL_V3，`FullM1Transport::Create → QuantumCommitKernel()`，shader3 → program4 | `0ef4f0457a8be135953bff0ce61845d854c6c7774e36ee0d6a79a7a3605a31cc` / 54857 | 11.779646s，success=true/rawGl=0 | 当前仅 `glLinkProgram(4)` ENTER，独立的 LINK_STATUS、link glGetError、log 与 Finish 均无返回证据 |

program4 和 shader3 在不同 object 的生命周期内被重新分配。源码在 object1 的 Create 成功后删除中间 program，再创建最终 program。object2 持久 ATTACH 事件的 `handle=4,a=3` 把 program4 和 shader3 绑定，游标的 handle4因此明确是**当前 program**。object2 的 COMPILE EXIT 证明 compile 返回、`GL_COMPILE_STATUS` 返回非零且阶段 `glGetError` 为0。最新游标覆盖了此前的数值返回，阶段事件只保留布尔成功和原错。object2 compileLog queried=true、reportedLength/copiedLength=0、queryError=0，linkLog queried=false。空 hex、`current.rawGl=0`、attempt code=0 都不能证明当前 LINK 成功。object0/1 的 LINK 时长是整个阶段合计，具体每个 API 的时间未保留。

## 包、程序字节和出口来源

三份签包均与签名收据的 SHA 相符。T590-cursor2 的重装包 signed SHA `6b1faa1df776c52b356313f52a22d7d021bdea6704cbdb424fe07a613246c647`，验签输入 `49c5f6b6…`，与原 T663 unsigned `d2a522e2…` 分别登记。实际包内 libgrsm1.so 的 SHA `7a0bcbaab37c3fabcff3a5874164bbea1767e2877db903ad867477fdb781253f`、BuildId `c23e231c781476010af91f631c60d2239371205c` 同 T663 原交付，ABC 已变为 `397766f8…`。旧 T591 和 a2 的 libgrsm1 BuildId 分别为 `0cf0ffde1232b06e0ee044ef519cf01c11120dd9` 和 `c789cf34c0b0f16212341c39010b7780e15654e1`。

上述三份签包内 ELF 均包含同一组三段逐字节匹配、以 NUL 结束的 GLSL ES 3.10 字符串。object0/2 还直接与生成 `full_kernel.cpp` 字面量哈希匹配，中间程序由 `QuantumKernel` 返回生成的 core。运行采样中 layout=`6e01a7bd26032beb1efd3cb58d8fb7377c4a5e4c16903572187dc11d9b87c73e`，combined family SHA=`7e19a455a8e4b213a3823111cc0c00c802912fdc5f361b05d57f82c923654a49`，K192/Kw128、feature7/wire3/policy1。combined 值来自 `AppShaderSha → FullM1ShaderSha`，不是当前 object 的 `glShaderSource` 字节哈希。两次真机记录的驱动字符串均为 HUAWEI Maleoon 935F / GLES 3.2 B320.89d539c90932f442acb8859a1797b348e4060bcc。数值能力 admission 仅证明已记录能力和布局检查，不能证明某程序可以链接或按期限交付。

本次 manifest 记录 `DECLARATION_MISSING`，package 与 ELF 声明均空，同进程 loaded file/BuildId 和进程起点未保存。9899B native.json 的 SHA 和0B的-1.log字节均与 manifest 一致，-0.log/-2.log当前仅有0B及空SHA收据，未见原文件。原 export 按 bootSerial 分次取三个 m2CreateLog，和先取的 native snapshot 有时间差。该目录未留 T676-A 的连续 ndjson 或真实终态收据。cur_l3–l7 与“导出 M2 创建原始诊断”按钮支持实际 M2 页导出来源。替换入口的 `T590Cursor.ets` 只有独立采集 UI，无 XComponent，不能单独作为真实创建采集入口，也不能用其源码证明本轮 collector 在创建前已 arm。签包身份已核，设备同进程加载身份仍须独立关联。

## 分流与工程落点

1. **证据和用例修正。** T590-v3 的“同一API两次、稳定复现”应撤下。T591 与 T590 的 SOURCE_V3 证据只支持同族源程序 LINK 曾长时间开放、另一 attempt 最终完成该阶段。当前 object2 为单点截尾，晚返回、持续阻塞和进程退出仍待区分。T678 的 a2 boot、正式 M1 运行期、Windows SDK native 大链接及 host ANGLE dispatch 各自保持独立，不据设备规则倒推共同根因。没有证据指向物理算法错误或某一函数/循环造成驱动缺陷。
2. **已定位的工程差异。** T663 实际诊断生成链的三个 compute 创建点直接走 `create468::Build` 的源码分支。T475/T491 另一条产物已经交付持久化及暖启实证，T491 设备是 Maleoon 910，当前为 Maleoon 935F，旧 binary 或暖启数据不能跨驱动当命中证明。若目标是当前路线减少重复冷创建，先让 tester 核实际产品基线的三个 role 是否已接 `ProgramFactory/ProgramArtifactStore`，复用其真实产物做同设备冷/暖对照。若同产品真实生成线路确实绕过缓存，developer 只从实际生成链复用已交工厂和现有收据，保证源码 miss 路径仍走真实 shader、原 cursor/错误、反射、绑定、保守失效及仅一次原 GPU GLSL 回退。原 T663 诊断基线留作未启用持久化的对照。缓存命中可省重复 compile/link，首次成功前及不兼容环境仍需源码路径。任何缓存接线沿 T462/T475 合同走实际产品路径，当前续证保留现有 GL 调用数量。
3. **冷启动结构对照。** SOURCE 的单程序统一 `uPass` 覆盖初始化、排序、执行、重放、publication/audit/seal，最终 QUANTUM 合并 candidate、audit/replay、metadata、publication 与 completion。中间 QUANTUM 在分配、初始 reflection、pass table 创建后被删除并替换，当前源码在这段之间未提交中间 program 的运行 dispatch。designer 的候选应先比较三程序基线、只为 FullM1Transport 拆出资源初始化并消除中间临时 program、按依赖边界生成较小专用 program。中间省掉的编译/链接时间不能直接当新版本节省值，须保原独立 quantum 路径及等效 ABI/有效反射、生命周期、fail-close 和实际设备验证。优先比较 SOURCE 与最终 QUANTUM 两个主要冷启动段的总开销，避免只削某函数、用源码字节或单个 link 度量当因果。

结构候选先对齐已有 frozen K/Kw 档、事件/排序、整数守恒和 replay/commit/真实 publication 账本，保证旧/新输入输出、barrier 依赖、SSBO 布局、token/owner 和取消语义一致。保留极端容量和非法 scope 的拒绝。评估每 role compile、link/status、反射/准备、总真实 Create/READY、binary replay、总 GPU dispatch/barrier 与首 root/首有效呈现。分开冷缓存、新进程暖缓存和同上下文复用。沿目标的 30fps 运行预算评估拆分后的稳态成本，启动改善不能代替稳定帧。首开未测清前不冻结拆分、不按观测删物理/审计 pass。唯有配对控制得出可复制收益才转正式设计与工程实现。

## 最小同 attempt 补证

先找现存同一进程的原名诊断、三个原始 log、manifest/WS_EVENT、后续 tick 或终态以及实际进程加载映射。没有同进程原始 bytes 或加载映射时按字段登记 NOT_CAPTURED，避免用新进程 maps 补旧档。新采证仅沿 T676-A 既有 collector/实际页出口，保实际 M2 XComponent、native context、真实 K192/Kw128 场景和创建 owner。tester 离线负责签包、比对实际产物、结构化解析和工具；effector 串行真机部署、确认真实页/入口/进程起点、采集原件；T676-B 正式 M1 独立接续。

- 先留同一进程标识与实际加载模块路径、BuildId、可核 backing ELF SHA，并与签包绑定。不可读时保准确读取对象、权限/错误及可用字段，不拿 Want 声明当 loaded 证明。每次 native 返回保存原串与 SHA，核 `bootSerial/epoch/window/context/ownerTid`、program role+sourceSHA、cursor revision/api/sequence/edge/handle、current/events、首错、begin/end、sealed/drop。只从同 scope 的可证记录归因。优先保存首采、cursor/阶段变化和窗口末样，本轮只在 log 元数据变化或真终态时取对象日志。
- 从 arm 起沿原上限 ≤60s、最多64次诊断 native 查询（log 计入）、单持久记录≤16KiB UTF8、目录≤1MiB含32KiB索引预留。单次 API 出参必要时按≤8192B原始字节分片，全串 SHA、offset、length 一起存，每片不另调用 native。实际页在该窗口未到 object2 如实报该终点，不能扩窗或拼后续 attempt。当前原件是创建开始124.764s之后的手动快照，不能冒充完整60s collector链。已存在的后段原件可直接取，不反复重启以复制空证。
- 终态要求同 attempt `sealed===true && nativeEndNs>0`，保实际返回码。下一次可证原始样本若越过该 API，才可确定发生进展，精确时延仍须同次 EXIT 记录。采样末仍 ENTER 分成 OPEN_AT_OBSERVATION_END 与真实窗口/预算/容量结束，进程退出用真实 fault 或退出收据。打开的 LINK 不授予资源 READY、render/current 或 M3/G9 原剩余矩阵放行。

本卡是 architect 归因与工程交接，完成必要静态核对，不使用真机、不运行测试/构建/产品。T663 接口、既有 T676 工作及其它任务状态由各责任人处理。保持 GPU 驻留优先、CPU 停投、目标10暂停、13→14→12→15→16→17→18→19。前景背景仅 XComponent+shader，失败 FAULT 或标记 STALE 的上一有效帧。续证不新增 GL query/线程、不调宽门、不加 manager 或开发关闭前置。

要么完成本卡当前约定范围及本角色必要检查，要么遇到确实无法继续的具体问题才停止；不能做一点就停下。不允许修改 D:\hmprojects\HelloWorld\docs\后续工作清单.md（仅执行者本人状态有限例外），不修改其它行列、队列及滴答同名清单。
