# T152 GPU真机失败工程分流 V1

2026-10-27；architect；原卡`6adff62ce4b03273ad8d5fc0`。GPU目标6最高优先。不使用真机，不重跑测试，不改业务。消费T138、T141源码/manifest、T142报告/原始文件/改名shader副本及现有binding/variant生成器。**工程分流交付：wire缺陷、shader合法性与测试包闭包缺口先文档后developer；算法未见需退T137 designer的新反例。** 设备采用/全事务收益仍UNPROVEN；不启生产GPU路线。

## 1. 证据身份与可消费范围

逐文件标准SHA核对：T142归档的3份originalShaders与当前T141原shader一致，3份改名副本hash也一致；build/query归一化仅替换whole-word `half→halfM`、`length→segLen`、`stop→stopAt`、`start→segStart`，reduce字节未变。`.codex/local-test/t141/`现有driver、diag与so实文件hash分别匹配T142 manifest，so亦匹配T141 manifest。保持这些旧原始文件，不把副本改名运行冒原shader已通过。

正式`t142_device_driver_first.json`可解析：192/512 execute=OK，但pairOracle=ORACLE_MISMATCH；driver将任何consume非预期归为此名，**不能据它定算法选对错**。两份正式`fixture_diag.json`/`timing.json`本身非法JSON，均有数组开头`[,`；diag源`runIdx++?",":","`及sample同式在首项也输出逗号。仅在内存删除这些明确的前导逗号可解析各2池、计时各5样本，所有样本consume=PROTOCOL_ERROR/agree=false；本轮未覆盖或“修绿”原文件。这是tester导出工程缺陷，需版本化修订，不能以JSON不可解析否定已经可见的真实result词或凭手修产物签PASS。

T142正文“slot/epoch/identity协议PASS”范围须收窄：execute完成/合成result离线检查不等真实GPU结果被owner消费。少量192 NO_PAIR、512 pair16/342（slots15/341）与CPU原扫描一致属于**定点建议值观察**，不是全部排序层、身份注错、连续merge或完整算法设备通过；diag中的leafTopSample实际取CPU leaves，不能作GPU逐层表回读证据。

## 2. F-T142-1：T138合同权威，精确修shader，不松owner

T138 `P011_GPU_PAIR_WIRE_V1` Header256/Result128逐word是权威，T137的128B预留不构成另一个布局。`pair_owner_v1.cpp`验证与T138一致。以下r为Result word，h为Header word；GLSL `H.h[j]`是一组4words，不是h[j]。

| Result words | 按合同来自 | 当前shader |
| --- | --- | --- |
| 12..15 | h28..31 requestToken | H.h[7]，正确 |
| 16..19 | h14..17：contextEpoch与payloadSerial lo/hi | H.h[3].zw + H.h[4].xy，正确 |
| 20..23 | h18..19 mutation lo/hi，h48..49 identitySHA头2词 | H.h[4].zw + H.h[12].xy，正确 |
| 24..27 | h50..53 identitySHA中4词 | 当前H.h[13]=h52..55，**错位，跳过h50/51** |
| 28..31 | h54..55 identitySHA末2词、DONE、0 | 当前H.h[14].xy=h56/57 reserved，**错误** |

准确说，owner serial回显消费h14..19；h20/21是attemptSerial，不是需要额外直接回显的mutation字段。序列跨uvec4但当前R[4]/R[5]这部分已经正确，不能把它们一起改错。最小正式shader改动只需：

```glsl
R.result[6] = uvec4(H.h[12].z, H.h[12].w, H.h[13].x, H.h[13].y);
R.result[7] = uvec4(H.h[13].z, H.h[13].w, 0x444f4e45u, 0u);
```

保留R[0..5]和owner对magic/layout/status/flags/dispatch/token/epoch/serial/SHA/marker的严格校验；不忽略SHA、不仅核pair、不换owner预期去适配错误GPU。旧fixture SHA尾词非零而错误shader写0，解释PROTOCOL_ERROR；某些偶然重复词可能掩盖错位，不能笼统说“任意输入数学上永远不可达”。修后仍需真实consume回归，不以静态映射代设备PASS。

developer将word偏移和source lane映射定义为命名常量/构建期schema，禁止手写`12+1`/`13+1`当含义；CPU与shader可以从相同schema生成，但**独立测试预期必须按T138字节规范另写**，不能用同生成器自证。

独立回归：header64words填唯一非零不同值，特别h12..23/28..31/48..55，高32位也非零；比较完整32word结果和n0/1 CPU_EMPTY_SHORTCUT同布局。PAIR/NO_PAIR各跑；逐词翻转r12..29、marker/magic/layout/dispatch/status/slot/rank、owner代/当前pool/配置/取消与旧context必须拒绝；篡改不能让合法pair绕过身份。当前离线合成result仅测试CPU制造端和owner一致，未覆盖shader错位，故新增端到端shader回显断言是必要缺口补齐。

## 3. F-T142-2：保留字与其它命名问题分开

[Khronos GLSL ES3.10 §3.7](https://registry.khronos.org/OpenGL/specs/es/3.1/GLSL_ES_Specification_3.10.pdf)将`half`列为未来保留字，使用会编译错误。build源码确实声明`uint ... half=...`，这是可独立确定的工程错误，应正式改为halfM/halfCount；不把它归为Mali特有问题或算法设计错误。

`length`是内建函数/数组方法名，**不是该保留关键字表中的half同类**；`stop`、`start`也不在该表。当前归档目录只见SHADER_COMPILE阶段JSON，未见报告所引“7 errors/Expected token…”的原始完整shader info log；不能从联合改名通过证明每个名字独立冲突，不能编造driver日志。最小修订可采用已归档验证过的四个改名，保持整数算法/边界/dispatch不动；但报告只能定性为“half确属保留字，其余保守避名，单独拒编原因未证”。

tester先从合法已有采集位置补归档原始编译stdout/stderr/info log、源SHA与stage（不要求先重跑设备来证明half）。若旧log确不存在，标NOT_CAPTURED；下次正式修订shader采设备compile/link完整log，仍有拒编才按准确行/词局部补证，不把所有失败归某个保留字。developer离线环境若无ES3.1，标SHADER_COMPILE_NOT_RUN；有合规离线GLSL编译器可加编译检查，但不冒目标驱动执行。

## 4. 完整事务缺口：需可安装的隔离项目及真实GPU接线

当前generator只复制ETS闭包并替换Hot中3行选pair源；adapter文件虽复制，EntryAbility/DeviceRunAssembly均未调用armT141GpuPair。variant没有module.json5/资源/native CMake/worker打包骨架。`run_variant_patch.cjs`注册CPU精确索引stand-in，只证明替换接口可承载原S2测试，**明确不是GPU完整事务验证**。不能让tester临时写产品/用此stand-in报收益。

developer实施范围限`test/p013_compute/`生成器、binding/executor/shader、测试专用ETS/worker/native模板与生成测试项目，不改生产entry/native renderer/Hot/M2算术。完整项目建议冷生成在`.codex/local-test/t141-project/`，生成前检查目标路径在workspace且不覆盖共享源码。复制现有工程的必要hvigor/module/resources/oh-package与native依赖骨架到独立根，记录source manifest；具体bundle以测试可用签名profile授权范围为准，不擅改签名/身份。必须交明确可执行build命令和生成文件清单，不只ETS目录。

| 位置 | 具体接线 |
| --- | --- |
| generated entry/src/main/module.json5及resources | 独立测试入口Ability/页面；pages与资源完整。生产不读取测试Want或启用GPU |
| generated entry/build-profile.json5 | externalNativeOptions指向生成cpp/CMake，abi arm64-v8a；workers列表显式加测试物理worker，保留有用既有workers |
| generated entry/src/main/cpp/CMakeLists.txt | 按需包含原新渲染管线依赖及独立`p011_gpu_pair_napi` target，链接ace_napi/EGL/GLESv3、C++17、-fno-fast-math/-ffp-contract=off，输出`libp011_gpu_pair_prototype.so`与ArkTS导入一致；不靠native CLI可运行冒HAP可加载 |
| generated resources/rawfile/t141/glsl | 正式3shader及manifest；Ability复制真实原字节到应用可访问目录，记录hash，不假设rawfile是filesystem路径；配置前verify SHA，禁止tester副本残留替代正式文件 |
| test Ability/UI | 只送有界配置/输入命令与收不可变结果，不能UI同步执行完整物理事务；UI画面若启用，仍只有XComponent+shader |
| test physics Worker | 在本worker内加载变体Hot/M2/WaterSandField及adapter，资源初始化READY后调用armT141GpuPair；arm成功后才允许step/seed/preview。reset/run换代或结束在同线程disarm；缺binding/compile失败立即报告，禁CPU fallback |

所有import须收敛到生成Hot依赖闭包，原mergePairHot/prepare/apply/finish/账本/rollback不改。生成器白名单diff继续强制，增加入口/worker/build模板单独白名单；每次生成归档源/生成/selector/adapter/GLSL/so/HAP/ABI hash与native compile日志。签名失败只报告SignHap原错，compile/PackageHap通过不等有效签包；设备任务须安装成功并记录实际加载so和GLSL身份。

## 5. binding身份、owner线程与错误闭合补齐

当前NAPI是进程static State、只用slot0，identity从work取uint32、硬写candidateRole4、mutationSerial=requestSerial、diagnosticOracle恒true。它可作为隔离pair原型，不满足T138完整候选/事务身份和性能模式合同。与打包接线一并修订，不只补调用arm就称全链完成：

- State改为绑定本NAPI env/测试worker生命周期、固定owner线程的独占实例；创建/GL执行/取消/清理均同owner，错误线程拒绝，不从其它env操作同context。双slot是隔离储备，非并发依赖merge；报实际slot/live/peak，不能当前只用slot0冒双slot恢复已测。
- 定义测试`T141RequestContextV2`，完整run/state/config字符串、owner/context/request/attempt/substep/poolMutation的hi/lo、candidateRole/ordinal、seed/params/input revision原值以及snapshot/build身份；保留wire V1 uint32字节布局。不能用uint32(work.stepEpoch/attemptToken)偷偷缩原safe整数域，不能一律role4冒preview/seed。
- selector在真实原调用边界传当前context，冷元数据跟随整个原事务；同一scratch每次实际merge/birth/其它mutation递增mutationSerial，别名/内容改变均使旧结果失效。requestSerial仅标请求，不代替pool版本；配置/取消/owner结束使旧result不可消费。诊断oracle开关冷配置，正确性时启用，性能模式关闭独立全扫但保留身份/谓词/错误消费。
- identity canonical bytes固定沿T138规范，当前prepare使用的修订字符串/头字段次序/buildSha字符串序列化须和文档逐字节核对后冻结；不可不同模块各算“等价JSON”。以显式identityEncodingRevision登记修订，归档完整golden bytes/hash，wire字段无需因序列化内部差异静默改偏移。完整owner sidecar校验不能只靠摘要。
- `executeIntegerPair=OK`只代表链执行/读取结束，**verifyAndConsume非OK/NO_PAIR不得计成功样本**；driver单列execStatus/consumeStatus/oracleVerdict，不能把PROTOCOL_ERROR统一打印ORACLE_MISMATCH。协议拒绝、STALE、算法差分错各自保留原词/输入。

timeout/WAIT_FAILED/mapNULL/unmapFALSE/GL错令slot QUARANTINED，不覆盖在途buffer，禁重复new slot逃避故障；本owner在确认完成或销毁失效context后退休。现在executor若先glGetError判断再由fail再读，会丢第一次原码，须在首次读时保存，stage+rawError+waitReturn一并透传NAPI。恢复重查limits/重编译分配、epoch递增，旧header/result/typed视图全失效；测试ready不能因native仅readback成功就回true。资源reset/eglMakeCurrent/销毁失败原码应记录，不能计数清0冒驱动已安全回收。

## 6. 最小开发与独立验证顺序

1. developer（不使用真机）：只修正式reduce末两向量与shader避名、schema/owner独立golden布局；补driver结果分类与GL原码，离线编译/注错自测交付，可独立完成此小修，不等完整设备收益。
2. tester（不使用真机）：修自有diag JSON首项逗号、逐样本新request身份（当前5轮复用同一id）、序列化parse/schema校验与原log归档；旧文件保留，新导出独立版本/转换说明/SHA。fixture/synthetic来源明确，无需重索T140已有oracle。
3. developer（不使用真机）：完成生成HAP项目、Worker/NAPI READY/arm接线、完整context和原事务闭包，三路分别旧扫描/T123/GPU，所有差异仅pair selector与可审查测试元数据；CPU stand-in只单列接口测试。开发/编译/离线自测交付不以真机为关卡前置。
4. tester（真机、同机串行）：修后真实shader先验证PAIR/NO_PAIR消费成功及错位/错token/高word非零必拒绝，再走完整attempt对照；失败先architect分流，避免又在协议失败时采整套收益。无需重跑与修改无关的宽渲染验收矩阵。

## 7. 完整attempt与收益判定保留原门

相同设备/构建/原before/config/input/seed跑旧扫描、T123、GPU，192/512主档，早/晚/无pair、稀疏/满、跨group/边界、连续merge质心变、批量birth/tail、λ重试、preview/authoritative完整原顺序。每次pair/NO_PAIR/status及最终pool所有原字节、free-list/nextId、预算/退款、事件/lineage/rollback比对；每merge重建CPU精确range和GPU索引，不batch pair，GPU不写权威物理量。原始run/request/epoch/mutation/dispatch/fence/readback/输入hash全关联。

端到端attempt墙钟从固定排队/owner领取起点到真实commit/失败清理，分列queue与service；包括freeze/prepare RN64排序范围、hash/pack/桥、上传、11/12 dispatch/barrier、wait/map/unmap、身份/谓词核对、CPU merge/audit/ledger及全部失败候选。5轮旧requestCpuMs≈.86/.87是**不含prepare且consume失败的链级墙钟**，不作完整pair或attempt收益；CPU fenceWait含排队/调度且与GPU执行可重叠，不叫GPU runtime。timer未测继续NOT_MEASURED。

冷context/编译/首次buffer/首dispatch与持久warm分列，原始逐轮与失败/outlier保留，固定分位算法；不把第一轮的max笼统称“冷p95”，不把分项p95相加成总p95。真机context恢复、timeout/错run/取消/迟到注错、资源live/peak/泄漏、UI输入延迟和新渲染竞争分别给证据；当前均未证，不以理论机制PASS覆盖NOT_RUN。

只有完整attempt相对T123改善、重复批次误差支持且原最坏/UI/资源/正确性门不破才可提采用建议；无新百分比/容差门。协议修复前的缺证不是算法不采用终局，修后也不自动生产迁移。若独立最小pair差分出现真实反例再精确退designer；本轮wire/命名/包接线均属工程或采集问题。

下游prompt必须含：不允许修改 `D:\hmprojects\HelloWorld\docs\后续工作清单.md`（仅自己行状态有限例外），不改滴答同名清单、不回退他人修改。无设备卡明确不使用真机，设备卡真机标签并串行。所有渲染含背景前景只走新XComponent+shader，禁止Canvas上屏/失败回退；失败FAULT或显式STALE上一有效新管线帧，不伪装当前成功。manager登记不验收，不新加验收门。

本轮交付为工程分流完成；原T142失败/NOT_RUN保留，无业务修改/真机/测试重跑或产品收益结论。
