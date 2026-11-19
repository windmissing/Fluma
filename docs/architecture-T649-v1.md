# T649：G13 错误 29 能力与程序阶段首次归因

2026-11-19，architect，不使用真机。结论 **EXACT_EXISTING_EVIDENCE_REQUIRED**：CONFIGURE_ERROR_29真实配置失败成立，具体能力条件或后续program/reflect分支未定；先交tester用现成接口补同attempt的精确字段，不派shader/精度降级或泛日志。已交正式表`architecture-T649-contract-v1.json`、证据`analysis-T649-evidence-v1.json`和有限只读复核脚本；未重跑测试/构建/设备。

## 1. 包、源、加载与原件范围

T643输入HAP2008c704…与T638收据一致，签包8e5cfac9…；签前后libgrs_g13.so均403bb9bb…/1349464B，modules.abc字节亦相同。开发收据中obj目录未strip ELF为1b43b391…，不能强求与包内strip字节同SHA，更不能用前者当包内/已加载SHA。所列T638源在隔离交付树逐文件匹配；当前源匹配情况独立保存在证据，不混主树后续改动。

maps原件有4个同inode57371的libgrs_g13.so文件映射，加1个匿名BSS映射；不是五段都同inode。它证明该路径装入，不含实际映射模块字节SHA/BuildId与进程启动收据，不能据pathname补完整源→实际加载字节证明。需要补时由tester复用原签包/进程核关系，不把该身份缺项误判driver。

本目录实际保存init摘要281B、after摘要319B及l1/l2/l3页面树；**没有4份base64诊断页或32768B raw原件**。摘要rawBytes=32768是payload声明容量，不是已保存32KiB。截图/页面文本有firstFailureAttempt232、CONFIG_FAILED、CAPABILITY:EXIT与queuedTicket232/handledTicket0，dropped/truncated0；不含能力原数值、stage rows、ConfigTicket/解码快照、原shader/program log。页面上的g13Configure回包是QUEUED，不是owner返回；不得跨这些API采样拼接。

## 2. 源码切点与勘误

Transport::Configure先ValidateConfig/Retire/context/profile，进入CAPABILITY；既有七类能力查询与glGetError结束后**立即记CAPABILITY:EXIT**，然后构造capability JSON/driver字符串，再检查阈值，不满足才return29。只有通过这些条件后才State PROGRAM_BUILD并记录ENTER，之后Build/Reflect/Prepared失败也可return29，后面test-hook uniform缺失亦用29。CAPABILITY:EXIT仅证查询段结束，不能证明能力门合格或已进入PROGRAM_BUILD。

按同一ticket的完整未丢stage若最后仍CAPABILITY:EXIT且没有PROGRAM_BUILD:ENTER，首候选是能力判定分支；但当前未保存stage原件/能力值，仍不可直接认定具体条件或断言未调用program。CAPABILITY JSON过长返回30，不混29；rawGL非零可能是查询期间/既有粘性错误，不自动叫硬件资源不支持。每条实际原错要按producer解释。

machine逐项列原门：blocks≥4、bindings≥4、invocations≥64、floatPrecision≥23、laneX≥64、shared≥4096、maxBlock≥该实际profile.ww×4字节、已有rawGL=0。profile须从冻结Config128及对应生成表确定，不用UI默认64×64猜本次配置。没有数值不填PASS或FAIL，不让tester改shader/删binding/降精度/放宽shared或数值门。

queuedTicket表示NAPI请求；InitControl.handledTicket在**初始化实际COMPUTE_READY**后才赋值，不是owner dequeue计数。故handled0不证明配置尚未被owner处理，CONFIGURE_ERROR_29本身反而说明某次Configure已返回失败。当前lastProgress全局、firstFailure锁存首attempt，两者必须用raw stage.ticket/attempt、控制ticket与sample来源核绑定，不能只看文案一致。

`sampleSeq`也不是完整payload版本：T638代码只在Stage递增samples，Queued/Ticket/State/Failure/Decode/Log等变更未必递增。四页同sampleSeq单独不证明原子全快照；现有合同本来就不承诺freeze。此次先在失败后不再发新配置/业务命令的稳定窗口读取，逐页保存与时间/序号，按每条raw身份归因；不靠跨页拼装生成新的“已观测ticket”。该限制是采证口径，当前不先增产品snapshot系统。

## 3. 原有接口足以先补的精确字段

原进程/attempt232若仍活：不重新configure、不激活/给pulse，直接读取完整`g13Status()`，保native.capability、native.compile、native.firstError、lastCommand、initialization和scope原JSON。Controller Status会在失败后通过Publish保能力字符串；即使尚未COMPUTE_READY也可读，页面只显示initialization不等capability不存在。capability中的driver/环境SHA、七原值和rawGL是缓存原件，不发新GL查询。

然后调用`g13InitDiagnostics("232",page)`，page0..3各8192 raw/base64，严格核schema/state/page/offset/rawBytes、原返回sampleSeq、base64长度并各存原JSON/decoded SHA。按ABI：decode[0,4096)、64 stage[4096,20480)、两raw log[20480,28672)、control[28672,32768)。从control读ConfigTicket完整proposed/configRawSHA/seedReceiptSHA/cancelGeneration与当前ticket；从decode取真正128word原配置及provider/单位/offset/状态；从stage取seq、ns、boot/attempt/ticket/queue/window/context/tid、phase/edge、apiReturn和各raw状态/sourceSHA，不把-1当0。

在无新配置的终态窗口可重复读控制页前后锚、全页最多3轮/完整status最多4次，明确每次独立观测；若版本/内容持续变化，保逐页原件并标MIXED_OR_MOVING_SNAPSHOT，不强合32KiB。readonly读取本身不改变payload；不能让收集端不断调用configure来“等稳定”。由于sampleSeq只计stage，必须核实际ticket/monotonic counters/控制原字锚及stage归属，不能把相等seq当冻结保证。

capability按原不等式逐项输出raw/required/结果，maxBlock保GLint64/十进制字符串，字节与word分清。若确过门且有PROGRAM_BUILD ENTER，则取route13 program factory已有SHADER_COMPILE/PROGRAM_LINK/REFLECT原序列、实际PipelineSha、返回/日志reported/copied/truncated；没有该阶段就记NOT_REACHED，不补空log作编译成功。原日志来自现成4096B双槽，不额外查询glGetError/log或改驱动同步。

记录boot、pid+启动时间、实际模块路径/BuildId或可核字节与原签包关系。旧进程消失/232不保留时准确PROCESS_LOST/STALE_ATTEMPT；只允许在相同已核包的一次新attempt重取**缺失字段**，differentAttempt=true，不拼旧232后继、不重复T643整轮或延长等待。每JSON≤16KiB、总目录≤1MiB（完整四页与索引足够），保设备串行标签与首失败即停；签名/现有构建部署/夹具均tester。

## 4. 分流与后继

当前不缺新的产品stage：原32KiB/能力status/program observer尚未被取出。具体数值证明真实平台资源达不到正式profile/精度要求时，architect核证后将设计资源/数学约束交designer；不让tester自行压缩网格或降低门。若已有原错/程序日志显示静态工程编译/反射/配置接线缺陷，先修对应工程稿再developer；若仅有原API失败，保真实错并交architect继续定位。任何模块身份/源码不符先tester修采证/构建闭环，不猜驱动。

T643本轮有限包身份/首失败采证已交付可登记关闭，本卡不撤销其交付，但纠正PROGRAM_BUILD定位推断。T609完整物理与T610可见观测仍未行使，须真实Configure/初始化成功和各自实际前置齐才恢复；补出首失败并不等产品通过，不设新manager核查/批准门。

执行必须完成当前约定范围与本角色必要检查，或遇确实无法继续的具体问题并完整回执才停，不能做一点就停；局部受阻先完成其余。失败列未完、复现原错/证据/已试/不能继续原因、所需前置与角色，原卡评论并本人行看评论，成功结束待验收。不允许修改D:\hmprojects\HelloWorld\docs\后续工作清单.md（仅本人状态例外），不改他行/其它列/队列或滴答同名清单。不使用真机；开发仅开发编译，tester/effector签名部署执行采证、真机串行。GPU驻留/CPU停投，前景背景只XComponent+shader，故障FAULT或明确STALE，禁Canvas/软件GL回退。目标10和旧湿M1暂停，水线13→14→12，不放宽GAP/物理/容量/资格。


## 2026-11-19 T661 原件归因修订

[T661](architecture-T661-v1.md) 核定T654的STATUS实际slice600，probe把g13Status调用attemptId当配置ticket；旧110/209错误29有CAPABILITY:EXIT但raw数值未取全。先原T654 tester同真实G13页保存完整status+4原页/锚；保本合同预算与Store边界，不由换页STALE推导新增产品线程或借M2三槽。新机器表见architecture-T661-contract-v1.json；无产品能力PASS。
