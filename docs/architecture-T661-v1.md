# T661 · G13 同页配置和完整诊断采集归因

2026-11-19；不使用真机，不执行测试/构建/产品。本卡消费T654全部24份UI原件、两份capture文本、v1-partial、当前probe源码、七个实际signed HAP、T638/T649及真实Store。原件SHA、签名包内ELF/ABC SHA与BuildId见 `analysis-T661-evidence-v1.json`，不是凭报告摘要下结论。

结论为 **TESTER_FIXTURE_AND_IDENTITY_GAP**。已有独立Store读API可直接从真实G13页调用；本轮未证明需要新增产品持久线程/三槽导出。旧232进程终止属于differentAttempt，不能跨新进程借原ticket。没有G13设备资格/物理/性能PASS。

## 1. 原件的实际边界

- l2.json：boot `271023133916096`，firstFailureAttempt/queuedTicket209，handled0，`CONFIG_FAILED / CAPABILITY:EXIT / CONFIGURE_ERROR_29`。
- l4.json：boot `271319633950940`，firstFailureAttempt/queuedTicket110，同样CAPABILITY:EXIT/error29。证明这两个配置到达过现有能力查询后的边界；不能说“从未Build所以能力没测”。确切数值未保存，状态是 **RAW_VALUES_NOT_CAPTURED**。
- lA.json是另一boot `271724245495783`，UNCONFIGURED、queued/handled/firstFailure全0，却请求旧110。其STALE不能证明旧110页失效或Store通道阻塞。
- lC至后续文本是又一boot `271944415857815`：configure返回 `attempt:"1"`，probe紧接g13Status的 `attemptId:"2"` 被regex提取后写AppStorage。**2是状态API调用编号，不是配置owner ticket**。报告称“QUEUED attempt2”不成立。queued1也未必已安装为active config ticket，仍需读取控制锚。
- `t654-capture1/2.txt`共3条 STATUS，每条严格600字符、全部不是完整JSON；最后截在native.acceptedQuantum附近。当前probe `status.slice(0,600)`，configure回执slice200，page0/3 slice2400，page1/2只有长度摘要。没有8192×4 rawbase64、完整capability、control-anchor或decodedSHA。不能称“完整status已证明 capability不在构建”。

七个signed HAP逐一核对均与各自签名收据匹配，包内libgrs_g13.so完全相同：SHA `403bb9bb1ae3025d5b8a4097671d5d7ff3d2d6ad0aba3d183c2e5c0ed8bad3eb`，BuildId `69d14bc65a1acb6146f2c27a7ae767b96c71cd4a`；ABC/包SHA各另存。现存只有一个当前T654Probe.ets，并无七份历史源码快照，不伪称重建全部七版源码；实际包/ABC身份足以说明多次改probe签名没有换G13 native实现。缺同进程maps/loaded-module证据时，包内ELF不能冒实际加载模块证明。

## 2. 真实 API / 生命周期

`g13InitDiagnostics(attempt:string,page:number)`只接完整十进制字符串U64与0..3整数；`"0"`可探索最新payload，不能把Number/regex截断值当宽整数。bridge分派直接调用InitObservation().Page，自有mutex复制8192B；不排GL任务、不走App工作队列。接收条件是**当前ConfigTicket.attempt或firstFailureAttempt**，仅queuedTicket不授权，不等同任何NAPI decode attemptId。非法page为INVALID_ARGUMENT；合法参数但非所属ticket可STALE。默认209/110不能跨进程复用。

`g13Status()`已有 `native.capability`、compile、scope、initialization控制摘要。Publish在配置失败后仍发布原cached能力对象；T638 Store与status不依赖新线程才存在。真实G13ResearchAbility默认loadContent GpuResidentG13；tester的t654probe分支改成无XComponent的独立probe页，再router换页。实际研究页可放同页tester collector，同页ArkTS调用已有API和XComponent共存并无代码上的禁止。独立worker只在真正证明UI调用受阻时考虑，不因另页UNCONFIGURED就宣布同Ability互斥。

error29：原始l2/l4已把范围收窄到CAPABILITY:EXIT之后的gate。现有transport依次查询blocks、bindings、invocations、high-float精度、laneX、shared、maxBlock并保存cached capability，再判rawGL/阈值。确切哪项失败须raw数值，不能猜驱动不支持；不同新attempt若到PROGRAM_BUILD/REFLECT/Prepare等29出口须分别归类，不借旧CAPABILITY失败覆盖。阈值沿T649：blocks/bindings≥4，invocations/laneX≥64，floatPrecision≥23，shared≥4096，maxBlock≥实际profile WORK bytes，rawGL=0；数值通过也不等于FP32/math/scene或物理PASS。

## 3. 给原 T654 的唯一下一轮采集范围

tester维护夹具，不修改产品实现。把文件collector嵌入真实 `GpuResidentG13.ets` 的实际G13研究Ability/同页、在配置前就arm；保持原XComponent、session及owner lifecycle。仅发**一次**正式配置，先持久保存configure**完整原返回字符串**及其真实attempt字段，再采集；不要通过g13Status.attemptId寻找配置。禁止probe无XComponent签名循环、router切页、重新configure/active/pulse来“刷新”失败。设备执行按既有真机队列串行，非developer关闭门。

1. 采配置前完整status和页面/Ability/module/surface/context/run身份；捕获完整configure回执，不slice。
2. 同页等initialization出现失败终态；立即停止collector引入的控制命令。保留XComponent和owner，不先Stop/离页/前后台切换。完整status解析firstFailureAttempt、queued/handled、native能力对象；若正确ticket尚未install，以`g13InitDiagnostics("0",3)`作只读探索，解control后再选firstFailure/current ticket，不盲重试旧号。
3. 同一稳定失败窗口逐页调用0..3，每次返回**只调用一次API**并完整保存JSON原字节，严格base64解码每页8192B、记录原JSONSHA/decodedSHA/页offset/真实boot和ticket；拼接仅是离线解释不是制造原件。对page3控制锚与before/after status复核无新配置、cancel/context/window变化，Stage逐行核身份。
4. 段字典沿T638/T649：decode[0,4096)、stages[4096,20480)64×256、两log[20480,28672)、control[28672,32768)。保存firstFailure/lastProgress/queue/handled/cancelGeneration/proposed identity/configRawSHA/seedReceiptSHA、各stage seq/ns/boot/attempt/ticket/window/context/tid/op/API_RETURN/rawGL/EGL/phase/edge/sourceSHA以及log原字节/长度/截断。原始-1保unknown，不能转0。
5. 最多3轮、status最多4次、单记录UTF8≤16KiB、raw页8192B、整目录≤1MiB含index，计入所有探索/锚页；重复锚页占预算。无重配置时sampleSeq仍只代表Stage计数，**不是全payload原子版本**；同sampleSeq不自动证明decode/log/control同一时点。不能拼成“冻结32KiB”再宣PASS；不一致就INCONSISTENT_SNAPSHOT精确回architect，保全部页。

失败原始回执不能丢在Text.maxLines或slice后；日志落tester filesDir并读出，不在GL热路写文件。表单/页显示可摘要，文件保存必须原长。现有Store页API不改endpoint，不新增查询/同步，不新增产品side channel。下轮只补上述缺失数值/阶段/身份，不能重复证明同类STALE而不修attempt来源。

## 4. 分流与停止边界

原T654 tester修夹具并执行独立取证；产品实现本轮无充分缺陷证据，不派泛化developer线程工程，不借T622 M2三槽跨到G13。若同页read API确实无法返回：提交相同boot/ticket的完整前后status、调用start/end/异常、page/context/window/ownerTid、正确参数、实际加载ELF/ABC与已试worker或外部文件读的明确原错，再由architect评估有上限的冷读seam；缺的是页稳定性或能力raw值时按具体缺口补，不默认扩大模块。

执行prompt：tester需要真机标签、保持设备串行；architect本卡不使用真机。不允许修改 D:\hmprojects\HelloWorld\docs\后续工作清单.md（仅本人状态有限例外），不更新滴答同名清单。要么完成本卡当前约定范围及本角色必要检查，要么遇到确实无法继续的具体问题才停止；不能做一点就停下。测试脚本/签名部署/采证归tester；developer只开发编译，无新增验收门。全部背景/前景XComponent+shader，FAULT/明确STALE，无Canvas或软件GL产品回退；CPU停止、目标10暂停、水13→14→12。

## 2026-11-20 T686 同页续证结论

T672四页和r1-status已证真实配置216进入 CAPABILITY 后错误29。缺同步返回、前后锚与目录索引，四页非原子证明。现有出口可读失败域，修复边界已推进至非法 GL 查询参数与精度证明来源。见 [T686](architecture-T686-v1.md)。
