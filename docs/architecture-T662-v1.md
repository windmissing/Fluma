# T662 · M2 LINK 开放调用与采集终态首次归因

2026-11-19。架构交付，不使用真机、不运行构建/测试/GL/产品。读取T591-v3报告、collector.ndjson全部103条原始字符串、实际fixture、真实signed HAP与T622/T651 Store源码。逐项字节/SHA/包内BuildId证据见 `analysis-T662-evidence-v1.json`。裁定 **OPEN_CALL_AT_OBSERVATION_END**，不是永久卡死、驱动故障、创建成功或物理PASS。

## 1. 当前真正能证明什么

103行seq0..102连续，tick0..39；40次diagnostics（initial1+discover1+locked38）+24次log（8轮×3）=**64次**，不是报告的“39+24+2=65≈64”。报告算式65确实超64，原件实际调用记录符合64预算；不得放宽上限。8轮log在tick5/10/15/20/25/30/35/36。fixture到预算0后flush，原件没有WINDOW_END/预算终止具名收据；只能按源码与计数推断预算结束，不能说“60秒窗口完整跑完”。持久行最大5593 UTF8 B；本次collector/l1/l2三份原件合计320501B，在预算内。共享目录后来新增别卡文件不混成本次采集原件，也不借此声称整个后续目录永不越限。

39个含attempt的完整样本，bootSerial为字符串`"1"`；epoch2、window387041997984、ownerTid4360、eglContext387048859216、candidateRun1、surface2、contextGeneration2、geometry2、config1、owner1、K192/Kw128、feature7/wire3/policy1全程一致；nativeBeginNs273554856423752。layoutSHA `6e01a7bd26032beb1efd3cb58d8fb7377c4a5e4c16903572187dc11d9b87c73e`，combinedShaderSHA `7e19a455a8e4b213a3823111cc0c00c802912fdc5f361b05d57f82c923654a49`。这是**当前创建attempt**的身份，不能挪给历史T461或Windows进程。

最后样本sealed=false、inCall=true、nativeEndNs=0、firstFailure=null、code0、dropped0/incomplete=false；current为object0/LINK/handle2，ENTER273559698263283、EXIT0，观测时inCallDurationNs40140833750。COMPILE有真实EXIT273559698193596、success=true/rawGl0（耗时约4.841s），CREATE_PROGRAM/ATTACH也有EXIT。**40.14s是这个开放阶段截至该样本的观测时长**，不是永久阻塞判据，不是CPU时间/GL单个API精确耗时。

| object | 角色 | 实际source身份/进度 | 原始log含义 |
|---|---|---|---|
| 0 | SOURCE_V3 | sha b7d0a027128811db429440dec0cb94c77c8c26167fc7b507c611ffb3158606a1；85839B；compile返回，LINK阶段开放 | compileLog queried=true,length0,copied0,empty=true,error0，是真查询空；linkLog queried=false，尚无查询证据 |
| 1 | QUANTUM_INTERMEDIATE | 本attempt sha空/sourceBytes0，Build未到达 | compile/link均queried=false，不是“空日志证明成功” |
| 2 | QUANTUM_FINAL_V3 | 本attempt sha空/sourceBytes0，Build未到达；源码静态finalSha另记，不能冒本attempt提交值 | 同上 |

24条OBJ hex均空，但没有证明三程序link都返回空。m2CreateLog是独立Store.Copy，和同tickdiagnostics也非同一原子snapshot；按bootSerial/object、日志offset/reported/copied/queryError/queried/truncated/empty共同解释。缺对应metadata的hex不能凭长度当状态。

signed HAP SHA `9550454cbec14659075802a37cc589f0a28c89eeba44ef02d1d80ca00cdb5d91`与收据一致；其中负责collector NAPI的libgrsm1.so SHA `04b98996346e2706fd54e6d3da5e91fefd028d5180c408fb231473554f3c559f`、BuildId `0cf0ffde1232b06e0ee044ef519cf01c11120dd9`；包内其它ELF/ABC也逐项登记。当前未有同进程maps/实际loaded-module byte校验，包身份不补成真机加载证明。后续补证需保持该区别。

另按ELF内NUL终止的实际GLSL原字节核对：SOURCE 85839B/b7d0a027…与当前观测相符；计划中的INTERMEDIATE为26429B/54946590ca762cc8d9bb2e153e3fbab839b74bdf70fd474ad7b94b621f580f96，FINAL为54857B/0ef4f045…，对应生成头/调用链分别核对。后两者只是**同包计划程序身份**，本attempt未进入，不能填进runtime空SHA。计划local_size_x64、SSBO0/1/2/3为Base/Candidate/Workspace/Output；本attempt未到REFLECT/PREPARE，不冒实际binding/布局反射通过。

## 2. 38条 TERMINAL_SEEN 是夹具缺陷

sample只检测字符串包含`"sealed"`或`nativeEndNs`键，因此每次合法status都有TERMINAL_SEEN；当前38条对应sealed=false/end0，没有一个真正terminal。serial regex也不作为正式类型解析；严格JSON解码、bootSerial全十进制字符串，不经Number。

terminal必须同attempt **sealed===true且nativeEndNs为非零完整U64**，再按真实code分类SUCCESS/FAILURE；THROW/process lost、budget/window/capacity各自独立终态，不能当创建返回成功。开放阶段的code0是默认值，不授权成功。fixture下一版仅在状态转换时记录terminal，始终先保存原返回字符串。

当前seq连续、39个attempt身份稳定、native.dropped0只能证明**已保存记录/Store事件没有已报告缺口**；record使用body.slice(0,16384)、line.length非UTF8、超长行静默return、flush异常吞掉且没有真实DIAG_IO_FAILED写入。故不能外推“采集全程绝不丢、任何长度都安全”。本批实际行长在限内且JSON完整，没有原件显示该丢失发生；下一夹具改UTF8字节预算、显式oversize/IO/capacity收据、index预留32KiB，不能裁切native JSON。

## 3. 根因域与最小产品工程缺口

现有 `create_diagnostics.h::Build` 的LINK ENTER实际包围：observer ProgramEnter → `glLinkProgram` → `glGetProgramiv(GL_LINK_STATUS)` → `finish`内`glGetError` → Recorder.Exit → observer ProgramExit。**目前无单API ENTER/EXIT**，所以“首个未返回API就是glLinkProgram”超出原件；Store只知道LINK复合阶段开放。COMPILE亦含Compile/状态/Error复合步骤。不能为了获得日志在同context尚开放时从另一线程调用GL，或强查link infoLog等待/重入制造新故障。

现有Store/原始log/同页collector已经能读到上述信息，不存在“没有诊断线程所以取不到”的笼统结论。但它们**缺少单API生产点**，任何再读同一开放LINK样本/延长等待都不能把三个API拆开。这是明确产品观察接缝缺口，先冻结如下最小工程合同，再由developer实现；不新建外部三槽/持久线程，不改算法、shader、link参数、endpoint协议：

- 在原每attempt Store里增加一个**固定128B ApiCursorV1**，三个attempt总增加384B native payload，随Begin清零、复制在同一Store mutex内，sealed后不改。机器表定义全部128B：revision/majorStage/object/api/edge/handle/returned/rawGl各4B共32，sequence/beginNs/endNs/ownerTid/epoch/window/context/bootSerial各8B共64，reserved32B=0。unknown返回/error用-1，不拿0补未观测。
- majorStage保现有enum；api为现有 `LINK_PROGRAM / GET_PROGRAM_LINK_STATUS / GET_ERROR / GET_PROGRAM_INFO_LOG_LENGTH / GET_PROGRAM_INFO_LOG`，并以相同enum前缀登记COMPILE对应真实调用。原调用前ENTER、真实返回后EXIT，仅写cursor、递增checked sequence、保原clock/宽身份。无nested覆盖未退调用。handle是实际GL对象；返回值/已有结果原值记录，不增加任何GL query/glGetError。分支尚未到达保持NOT_ENTERED。
- 保持旧current/events/Finish/code含义；ApiCursor单独定位，不把整个LINK旧事件拆成伪成功。现有m2CreateDiagnostics JSON加**可选apiCursor字段**，旧端点/参数/主revision兼容；m2CreateLog仍0..2，scope/ticket不变。旧reader可忽略，新的精确判读要求apiCursor.revision1且身份完全相符。
- raw log仍只在原调用返回后的既有ReadLog路径获取；不额外执行日志查询。记录已有length/log API结果和queryError来源，具体log字段保持。cursor只保最后API，非无界trace/新hot文件，不添加同步等待。新增Store mutex只包POD更新，锁内绝不调用GL/observer/IO。
- 代码归属为实际被链接的m2/create_diagnostics.h及m2route调用链/JSON序列化；CMake/签名/包内ELF和fixture模块身份要据实重算。不能只修repo某个未链接副本就让tester拿旧t562包验证；developer输出可编译真实artifact及接口身份，签名部署采证仍tester。

该最小seam足以区分“尚在GL调用”“已返但状态查询未返”“错误查询未返/observer或recorder边界”；不承诺API内驱动根因，不加新GL探针或热日志，不用更长等待代替缺生产点。384B为新增显式payload，不宣Store整体正好64KiB；原Store实际sizeof/对齐由developer编译报告。T651的外部≤64KiB fallback并未触发，不能把本cursor伪装为那个大改造。

## 4. 接续与证据清单

tester先在原T591修终态/精确UTF8/预算/index，复用已成功同页采集：arm在真实GpuResidentM1 XComponent挂载之前，正确filesDir，锁同一bootSerial，所有原返回完整保存；总native查询（诊断+log）≤64、≤60s，单record≤16KiB，目录≤1MiB含32KiB索引预留，raw chunk≤8192B。日志仅metadata变化或真正terminal时读，不能8轮空查耗光预算；budget/window结束明确OPEN_CALL_AT_OBSERVATION_END及最后cursor，不等待至“永久”。不再发同条件40s LINK样本充当精确API结论。

developer先完成上述128B接缝/实际编译及旧调用行为保留；不是把tester脚本/签名部署另挂为开发关闭前置。tester后续在同新attempt补：完整cursor+current/events+三个对象source/log元数据、driver/limits、上下文/owner/window/ticket、native begin/end/实际terminal；实际signed HAP/loaded ELF/BuildId，process lost则保原fault/maps/同attempt文件而非补seal。缺某对象因未进入就NOT_ENTERED，不用历史shaderSHA填零字段。

设备M2源码创建LINK、Windows SDK原生链接器、host ANGLE dispatch进程退出是三个不同对象/平台/阶段。T461旧dispatch、T496/host ANGLE、T655 SDKlink-line、T657 host crash均不被本批“精炼为同一LINK域”。需分别保source/layout/program/binding/owner/context/时间/BuildId及attempt原件，只有真正同身份链才能关联；不默认合并根因。

执行prompt：不允许修改 D:\hmprojects\HelloWorld\docs\后续工作清单.md（仅本人状态有限例外），不更新滴答同名清单。要么完成本卡当前约定范围及本角色必要检查，要么遇到确实无法继续的具体问题才停止；不能做一点就停下。developer不使用真机；tester打真机标签并串行执行。所有背景/前景XComponent+shader，失败FAULT或明确STALE，无Canvas/软件GL产品回退；CPU停止、目标10暂停、水13→14→12。工程/夹具修订不是设备/创建/物理指标通过，不增manager验收门。
