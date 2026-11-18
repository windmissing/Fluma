# T547：离线Create后段与空日志导出的分项归因

> 2026-11-18 T588新证修订：T461 v3合法空raw导出窄PASS保留；同次native证明COMPILE完成且LINK进入未封口，非NO_ATTEMPT，未查询link日志不得称观测到空日志。外部包/ELF/安装/PID声明可由tester沿现有设施补齐，V5 FULL不是M2 Create前置；具体分流以 [T588工程稿](architecture-T588-first-attribution-v1.md) 为准。

2026-11-17，architect，不使用真机、不跑GL长测/产品测试。输入T496 v2、T461取证v2、T518/T521及相应原件/同源源码。结论分开：**T496仍是精确调用证据不足，先修测试设施并做有界定位；T461的合法空raw被md.update拒绝是已具备证据的导出工程缺陷，先修合同再交developer。** 不从任一项推断物理/shader/呈现失败，不延长900s，不恢复CPU线。

## 1. T496原件复算：纠正报告边界

`test/reports/t496/gltrace-C0.ndjson`实际378行=189对；C2实际660行，**前378行逐字等于整个C0文件**，第379行callSeq重置0、wallNs转入C2窗口。C0完整段有glCompileShader9、glLinkProgram6、glCreateProgram147；剔除继承前缀后的C2自身段282行=141对，全部为glCreateProgram，compile/link均0。两段各自被覆盖的调用均配对。报告的306/576行、C2有18次compile以及“环境SHA不匹配导致冷重编译”不能由这些正式原件支持；不能把包含C0的累积文件归为C2重编译。审计脚本逐callSeq/函数配对并保文件SHA，不修改tester报告或原件。

trace所有runId/arm为空，缺PID/tid/context/role/key/attempt/caller；不能仅凭新文件名证明来自唯一新进程。新段分界由同一前缀、callSeq重置及wall窗口确定，是可审计切分，不补造缺失身份。六函数sink来自单独gl_forward_sink TU，其RunId/Arm未初始化；TraceFile固定同一路径且ios::app，足以解释跨arm继承。没有不返回的已覆盖调用，不等于所有GL/EGL/程序binary/prepare均已排除。

CREATE_WAIT由RunCreateOnly主线程读captureCommittedEpoch后发出，**不是owner心跳**。不能据iter断言owner无死锁或仅条件等待；新出现GL调用只能证明相应时间该转发点有执行。资源commit还要求transport/scene成功、CurrentLocked及recovery.Resources，captureCommittedEpoch在该提交分支才写；其未达不能定位是Create未返、Create反复失败/重试、scene失败或commit资格失败。首次cold段及后续反复glCreateProgram也必须分attempt，不能压成一个900s编译。

主循环450000×sleep2ms是下界计数，不是900s wall守护；报告称外层用timeout920，原C0首末GL timestamp跨度约916.857s，不能宣称此文件证明严格900s内终止。run_create_only.cmd还存在`codex\...`缺前导点、按image名杀所有t496.exe及按窗口标题清理的设施缺陷。新runner只管理本次子进程树，不杀其它会话。

ABI来源门仍有T518指出的缺陷：build.cmd继续findstr sizeof而MSVC原布局写size(...)；两个sizeof输出当前均0B。不能沿用echo GATE_OK作为非空layout证明。独立原layout可以另核，但不足以让坏自动门变PASS。CREATE_ONLY成功断言在主循环退出后才写；本地log-C0/C2各7行，无CREATE_RESOURCE_COMMIT断言/最终summary，因此报告“退出前已写false”不是现存原件事实，watchdog退出应另具名WATCHDOG_TIMEOUT，不能伪造不存在的业务assert。

## 2. 缓存匹配：先完整身份，不能再猜环境

补读现存 `.codex/local-test/t496/{C0-files,C2-files}/gpu-program-cache/v1` 的封装和收据。属于T496的现存记录环境SHA为 `2aabb8c88eabed25386ede5af0b3fd16cfe498a5c53ba42f5239900f2258a5b0`，与旧候选seed声明相同，出现compute BINARY_LOAD/REFLECT_PREPARE_OK及SCENE_FULL BINARY_LOAD_ENTER；C2目录又保留拷入的HOST_APP/T496_C0历史记录。**目录存在、entries=6或某条route不能证明本次C2六role暖成功，也不能证明环境不匹配。** 收据缺严格run/attempt关联，另有同key的shader描述/历史来源差异、.jsonl.tmp未完成替换，均先保原件，不能据最后一行认定GL调用挂住。C2自身缺compile与“整个环境必miss”推断冲突，但不据此宣布全部缓存准入正确。

冻结下一轮方案：沿**所选同一frozen T492完整树、同编译器/宏/includes/generated头和实际同一组ANGLE DLL绝对路径+SHA**创建匹配seed；不换ABI、backend或伪造key，不重用“只因文件名前缀一样”的任意artifact。每role的source完整字节、binding/layout、link配置、environment、binary format、payloadSHA和封装key都应能复算；graphics含vertex+fragment各stage及完整ABI，不能只比环境SHA。

计算key严格沿原工厂：compute为`T475_PROGRAM_KEY_V1`后依次LE64长度+source/layout/environment/link原始字节；graphics为`T480_GRAPHICS_KEY_V1`，对每stage先LE32 GLenum作为一项LE64长度字段，再完整source一项，最后link/ABI/environment各项。摘要是完整字节SHA256，不把shaderSHA文本当source。封装T475B001头144B：magic8、format U32、payloadLength U32、ASCII key64、ASCII payloadSHA64；文件长度/format/摘要及当前key全匹配才是候选，真正BINARY_LOAD还必须实际API+link+reflection+prepare资格全部通过。

先静态验证旧候选六role的完整元组；任一源/ABI/key/环境不匹配就具名MISS，不改文件名/包头令其通过。在不运行D1的同源**seed-only原工厂**入口形成六role并等待原Store持久化完成，写本attempt独立seed manifest（每role完整元组、DLL/源树/EXE、生成begin/end与closure）；缺role/不能完成时保准确先失败切点，不将C2标可测暖命中。该seed-only是测试设施，不是CPU物理路径。

之后每arm独立新进程和唯一新空输出目录：C0无bin；C1三compute bin；C2六role。只复制明确manifest列出的完整.bin，**不复制旧.jsonl/.tmp/trace**。Store.Configure仍先于owner，核计数/文件+元组，不用在同进程冷编译后的内存命中冒独立暖臂。每进程attempt wall上限900s，含seed-only自身也独立同一上限，不因seed失败加时；已匹配seed可复用而不反复重建。C1不能由C2文件里继承的compile计数声称已执行；是否需跑完整矩阵按T518合同真实前置，未执行如实NOT_RUN。

## 3. 精确sink清单与配对规则

机器表 `architecture-T547-create-export-manifest-v1.json` 列全部函数/源文件/阶段。必须覆盖以下家族，并在每个实际调用前落ENTER、原样调用一次、返回后EXIT；**不能只观察完成回执**：

| 域 | 必须覆盖的实际切点 |
| --- | --- |
| EGL/宿主映射 | t496HostDisplay→eglGetPlatformDisplayEXT、eglInitialize/BindAPI/ChooseConfig/CreateContext、t496HostWindow→eglCreatePbufferSurface、eglMakeCurrent、eglQuerySurface/QueryContext、SwapBuffers及DestroySurface/DestroyContext/Terminate；eglGetProcAddress解析也记录返回是否存在。原有GetCurrentContext/Display/Surface及GetError只转发原调用结果 |
| 编译/载入/查询 | 六个既有编译函数，以及glProgramParameteri、glProgramBinary、glGetProgramBinary、glGetProgramiv/GetShaderiv、原shader/program infoLog、DeleteProgram/DeleteShader、环境glGetString/GetStringi/GetIntegerv/GetInteger64v；记录pname、对象、binary format/bytes、结果资格 |
| reflection | glGetActiveUniform/GetUniformLocation、GetProgramInterfaceiv/GetProgramResourceIndex/GetProgramResourceiv、GetActiveAttrib/GetAttribLocation/GetFragDataLocation/IsProgram |
| resource/prepare | Gen/Bind/BufferData/SubData/BindBufferBase/CopyBufferSubData/DeleteBuffers、Gen/Bind/DeleteVertexArrays、UseProgram和实际Uniform调用、DispatchCompute/MemoryBarrier/Flush/Finish/FenceSync/ClientWaitSync/DeleteSync/MapBufferRange/UnmapBuffer；记录长度/flags/timeout/返回码，禁止为观测强加finish或wait |
| scene/FBO | Scene::PreparePrograms内三个role BuildGraphics→ReflectGraphics→GraphicsPrepared、VAO、候选CurrentLocked检查；实际FBO/texture分配/FramebufferTexture2D/CheckFramebufferStatus和Draw/Viewport若该attempt达到它们，记录NOT_REACHED而非先做scene draw |
| 高层返回/commit | source.CreateUsing→quantum.Create→FINAL program替换→Reflect→ObservePrepare/PrepareProgram→Prepared→PrepareColdPrefix或单warmup→transport.Create返回；scene.PreparePrograms返回；recovery.Resources、CurrentLocked、资源commit拒因、captureCommittedEpoch写入、WAIT_NEW_SESSION/RecoverySessionLocked/首PagePulse/TryReadyV2Locked/READY |

绑定源为 `.codex/local-test/t492-m1-delivery/entry/src/main/cpp/gpu_resident`，不是共享HEAD。EGL映射在driver TU，六函数在gl_forward_sink TU，binary/query的其它入口在t488/gl_wrappers.inc、t485/gl_exports.inc、t324/gl_extra.cpp。实际函数只能有一个extern转发定义，不能重复定义/宏递归或漏其它TU。采用原PFNGL/EGL签名和返回型；GL/EGL error只记录**原业务已读取**的值，禁止额外getError吞首因；observer链保原callback，不用观测改变ready/permission/lease/资源回收。

高层已有T456 bootProbe.Copy及T492诊断可先只读保存，watcher每秒导固定副本、30s摘要，不得NAPI唤醒/模拟UI延长时间。App状态仅try_lock短拷贝，拿不到写APP_LOCK_BUSY而不是卡watcher；bootProbe自有小锁，不在其锁中做文件I/O。只有同attempt、完整ENTER/EXIT/返回值才可断言高层成功；ring掉落必须计数，不能拿其最后ENTER单独判API未返。

无法用既有观察点/转发函数覆盖的高层间隙，**精确交developer增加冷/test observer seam**（机器表列其函数、参数、enter/exit及返回状态），由tester装配；不能叫tester改canonical业务控制/shader或将缺marker默认为未返。冻结镜像允许的只有明确测试转发/观察差异；保原hash、instrumented hash和去除观察片段后的语义差异清单，类布局/控制路径不改。

每条记录含schema、runId/arm、PID/ownerTid/context generation、createAttemptId、callId/parentCallId、functionId、role/key及来源、wallNs/scriptNs独立U64字符串、RETURN/ENTER/EXIT、参数摘要/原status。跨TU共享**一个进程sink实例**，由main在owner前Initialize一次；不能再靠各TU未设的inline字符串或固定append文件。文件用本attempt独立路径且首次create-new，复用存在文件直接拒绝；每记录≤4096B，每attempt sink+快照总≤32MiB（测试磁盘信用，非产品4MiB诊断预算），超限报告EVIDENCE_BUDGET并终止本attempt，不截JSON冒完整。

每owner保存独立原子ActiveCall（函数enum、callId、phase），phase区分ENTER_EMIT/IN_API/RETURNED/EXIT_EMIT；所有共享字段原子或受短锁保护，不能以非原子seqlock造成C++数据竞争。ENTER append+flush成功后才标IN_API并原调用，返回先标RETURNED再写EXIT。watcher据此区分sink I/O停顿与API内停顿；单条缺EXIT不足以判驱动挂死。sink I/O不持App/调度锁，不转发GPU调用；观测开销单列，不拿该离线cold耗时冒真机250ms性能。

外部watchdog绑定本次child PID/进程树与起始wall，900s到点留下超时/ActiveCall/文件SHA并终止**该测试子进程树**；禁止taskkill按image/窗口标题清理其它任务。runner校arm枚举与解析后workspace目录，使用唯一目录而非拼字符串remove_all旧目录。watchdog必须输出exitCode/起止/deadline/attempt及最后已落序号；记录未生成则UNKNOWN，不把exit124自动翻译成某业务断言FAIL。

## 4. 下一次判读与职责

先修设施非空/showIncludes门、trace身份/继承、runner边界和source provenance；门失败不运行长测。CREATE_ONLY不安装阻塞式D1OwnerHook、不提前BeginSession冒READY；只等真实资源commit并记录中间结果。一次900s仍未到commit就按下表结束本轮，不再加时：

- ActiveCall=IN_API且同attempt ENTER持久化、无RETURN/EXIT → 精确到该调用/参数/driver环境，交architect；未覆盖域不外推。
- API全返回但transport code非OK/scene明确fail → 保第一code/原GL/EGL/role/日志，architect先工程归因，不能直接拆kernel。
- Create反复返回/retire重试或CurrentLocked/recovery.Resources拒绝 → 逐attempt及代号链分析，不叫作一次长JIT。
- codeOK/sceneOK，但资源commit资格拒绝或captureCommittedEpoch未写 →工程/观察域；缺返回记录先补该seam。
- captureCommittedEpoch已匹配，才依生产RecoverySessionLocked(serial,epoch,run,page)→首PagePulse baseline→READY走D1；commit≠READY，不能倒用首pulse修饰尚未commit的create。D1/D2仍保T494非空SOURCE/QUANTUM及真实Run/Tick/HasFlight门，不由账推测。
- 只有watcher/main CREATE_WAIT或sink I/O失败 →证据不足，保UNKNOWN，不能排除owner阻塞。

本项交tester在T496续办测试设施；如机器表高层seam确实缺，交developer只开发/编译/提交该观察接口，不执行测试。匹配seed与后段sink同步解决，不能又先长等900s才发现来源门仍为空。

## 5. T461：合法空raw的导出工程修订

canonical源是 `test/t468/M2CreateColdFiles.ets`；T468 generator复制到交付树，T482-final生成件与canonical逐字一致。不能只修忽略目录中生成件。`saveRaw`允许hex=''，生成Uint8Array(0)，完成open/fsync/close后**无条件md.update({data:bytes})**；T482交付页exportCreateDiagnostic先逐object保存raw，再native.json/manifest。现场lA确有 `CREATE_EXPORT_FAILED Error: update failed`；本地create-raw0.bin为0B，目录前后原件出现0B object0而无后续native/manifest，与在首次hash处抛错一致。报告称三次0B设备SHA；本地所持一份空raw和有限目录记录仅独立核这些原件，不凭同一0B文件宣称三份完整下载。

修订T521当时“尚无证据修改saveRaw”的结论：**现在有工程修复依据**。这是导出证据链错误，不是编译/根/物理失败。NO_PREVIOUS_CREATE在serial1、NO_TRACE在未完成root域仍合法；不借此次修复改变其准入。

唯一空输入处理：保原filename/hex≤8192/偶数/小写hex门、checked长度、write循环/fsync/finally close；**仅在成功完成空文件fsync与close之后**，若bytes.length==0，返回bytes0及数学常量SHA256(empty)=`e3b0c44298fc1c149afbf4c8996fb92427ae41e4649b934ca495991b7852b855`。不调用md.update(empty)，也不猜digest在未update状态的runtime语义。此常量仅合法零字节域，不用于任何非空数据或I/O失败，不能以“catch crypto错误就当空”兜底。非空仍原crypto SHA256原字节，digest须32B/最终hex64；失败如实中断。

原三个raw+native.json+manifest.json及WS_EVENT成功链保持协议；每个成功回执name/实际bytes/SHA对齐，native/manifest须非空且JSON可解析。全部文件成功后才发CREATE_DIAGNOSTIC_EXPORTED/WS_EVENT；raw0B不作失败，native0B不能借该常量冒合法诊断。失败保确切object/阶段（FORMAT/OPEN/WRITE/FSYNC/CLOSE/HASH/JSON/MANIFEST）和首次原错误，清理异常另列，不写成功manifest/WS_EVENT、不删除已存在原件。此次不加新endpoint、不加物理读回/dispatch，不扩4096B raw/4MiB诊断信用。

旧 `test/t468/page_units.cjs` 的fakeCrypto.update接受空数组，因此其旧empty例只能证明模拟环境，不能证明真机兼容。交独立tester换一个明确“空update抛错”的平台行为替身（空路径应零次调用crypto），补empty/非空/badhex/4096上界/短写/写0/fsync/close/非空crypto失败及完整成功链；开发不执行这些测试。修复后T461原卡用正确新签包/串行真机补齐原T521 native/manifest/WS_EVENT完整证据，再回architect判M2 create/READY/root，仍不猜本次缺失的code/firstFailure。

## 6. 接力prompt和交付边界

developer（空导出修复及确实缺失的只读观察seam）：不使用真机。不允许修改 `D:\hmprojects\HelloWorld\docs\后续工作清单.md`（仅本人行状态有限例外），不更新滴答同名清单。按本稿修改canonical T468 writer、必要的有界阶段错误及同源generator/签包声明；只修已明确工程项，不改物理/算法/时钟/渲染门。不执行测试，职责仅开发/编译/提交，独立tester不作为开发关闭前置。前景背景均新XComponent+shader，失败FAULT或标明STALE上一完整有效帧，禁止Canvas/软件GL产品回退。CPU停投、root250ms/GAP33333333ns/120Hz配额/显示30fps、defaultOFF保持。要么完成本卡当前约定范围及本角色必要检查，要么遇到确实无法继续的具体问题才停止；不能做一点就停下。局部阻断先完成其它项；具体回执依用户规则原卡写未完/原错/复现/证据/尝试/不能继续原因/前置和接手角色；失败本人行看评论、成功结束待验收。

tester（T496离线设施续办）：不使用真机，按§2–4及机器表精确sink/匹配环境seed，先通过同源非空门再一次有界attempt，补回architect，不延900s、不碰canonical业务、不得改他人清单/滴答同名清单；同样须完成约定范围或给具体不能继续的错误回执。T461设备补证另按其真机标签串行，不将设备测试塞入developer。

本轮完成只读SHA/JSONL配对、前缀污染、源/布局门审计和有限导出/回执合同向量，见 `analysis-T547-create-export-evidence-v1.json`。产品Create具体卡点/新包实际导出效果尚未验证，均不冒PASS；manager据交付登记推进角色，不新增验收门。
