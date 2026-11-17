# T546：GPU配置入口拒绝与错误串崩退首次归因

2026-11-17，architect，不使用真机，不运行产品测试。消费T542/T545正式报告、原卡评论、jscrash和源码。结论：**G9未捕获JSON.parse异常和入口拒因不可区分为已确认工程缺陷；M3/G9具体拒绝谓词及“同一typedarray根因”尚未证实。** 物理/资格/期限域均保持NOT_REACHED。本稿与 `architecture-T546-input-rejection-manifest-v1.json` 冻结修复和精确取证合同，工程范围已交付。

## 1. 已证事实与不能合并的推断

原件 `test/reports/t491/t542/jscrash-latest.log`：PID30232、Pura90Pro/MLN-AL00 7.1.0.566、Rss417017kB、11:25:49.830；SyntaxError，栈在GpuResidentG9.ets:43:25/start。原错误消息对输入文字有掩码，**原件本身未逐字暴露完整G9_RESEARCH_CONFIG_REJECTED**；完整token由tester观察及代码唯一该分支支持，不把掩码还原成独立原件实测。报告称3/3复现；本地单份jscrash独立证明其中一次，不把它称三份独立栈。

`GpuResidentG9.start`在try之外执行 `JSON.parse(native.g9ResearchConfig(new Uint32Array([k,full?1:0])))`。native拒绝时返回未JSON编码的token，页面直接解析且无catch，足以解释所见崩退；这不证明native为什么拒。随后g9Configure结果也未核准就安排frame，同属启动事务的错误传播缺口。

`g9Words<N>`逐字委托 `m3Words<N>`，所以两route共享候选故障面；**共享代码不等于已证明同因**。M3_CONFIG_REJECTED为m3Words失败或revision/features/profile/full任一失败；G9_RESEARCH_CONFIG_REJECTED为g9Words失败或profile/full失败。N=16和N=2不同，未有API status/输出值逐项原件；报告的“base==pointer/bytes/detached真机不满足”只能列候选。不能据旧number接口正常排除值/参数数/类型输出等其它谓词。

T545正确包/ELF、四档表、离线SelectProfile探针及页面静态词，可削弱错包/表缺失假设，但不等于调用现场取得的16word已复制正确。源码正常调用词 `[4,15,192,64,7680,8388608,1048576,65536,1048576,0,0,120,524288,1048576,1,0]` 满足静态分支；必须把“预期词”与native实际copy后词分列。K512及其它档在报告中未逐档实际进入GPU，不从K192入口失败外推各档shader失败。Rss417017kB不是OOM证据。

## 2. 严格解码合同：先观测，不放宽

当前m3a/napi.inc谓词必须独立记录，不能再用一个短路bool隐藏首因：

| 顺序 | stage | 通过条件/输出资格 |
| --- | --- | --- |
| 1 | CB_INFO / ARG_COUNT | napi_get_cb_info成功，准确一个实参；读取容量2个的argv以识别额外参数，不把容量1截断视为总数证明 |
| 2 | IS_TYPEDARRAY | API成功且typed=true；不把number[]/ArrayBuffer/DataView/普通对象自动转成Uint32Array |
| 3 | TYPEDARRAY_INFO | API成功后才读取type/length/data/backing/offset；此前全NOT_EVALUATED |
| 4 | ELEMENT_TYPE / ELEMENT_COUNT | type恰napi_uint32_array，length恰N个元素；SDK native_api.h:2158明确length是元素数，不能与bytes混名 |
| 5 | BYTE_OFFSET / DATA_POINTER | offset=0，data非空；不接受subarray或非零offset，也不借copy绕过此约定 |
| 6 | DETACHED_API / DETACHED_VALUE | napi_is_detached_arraybuffer成功且false；unsupported/invalid_arg保持具名拒绝，不等于“未detached” |
| 7 | BACKING_INFO | napi_get_arraybuffer_info成功；输出base/bytes此后才有资格被比较 |
| 8 | BASE_POINTER / BACKING_BYTES | base==data且bytes==checked(N*4)；不能先删除相等门或只用arraybuffer主路径跳过view验证 |
| 9 | WORD_COPY | 前述全通过后，同步memcpy到定长native array；不保存JS裸指针越过NAPI返回/异步owner |
| 10 | API具体语义 | 复制后才按route校revision/features/profile/flags/cold参数，逐项记录实际值及首个拒因；失败不变更route/App/config/run/clock/queue |

既有普通非共享ArrayBuffer所有权要求不变；其它runtime对象/Sendable或共享语义未被该合同明确支持时拒绝，不做隐式兼容。NAPI output和布尔值需三态：VALID(value)/API_FAILED(status)/NOT_EVALUATED；不能打印未初始化type/backing/base/bytes，不能为凑完整日志继续调用以无效backing为参数的后续API。`napi_get_last_error_info`如需读取，应在失败API后立即只读并有界拷贝，不能被后续日志调用覆盖原status。

开发先引入同源 `gpu_resident/napi_words_contract.h`（ReadWordsResult<N>，含firstStage、rawStatus、observations与copiedWords资格），m3a/m3b/g9仅薄委托并传真实route/api，不让G9被标成M3失败；不改N/类型/offset/bytes/范围语义。m3bWords存在同式重复实现，纳入复用检查，不能让T517/M5接力带回旧不透明路径。T543 Config64后续只声明N64，不能因此放宽当前N16/N24/N2。

固定接口职责：`BeginInputCall(InputRoute,InputApiTag,size_t expectedN)->InputCallContext` 分配非零attemptId；`template<size_t N> ReadWordsResult<N> ReadWords(napi_env,napi_callback_info,const InputCallContext&)` 返回固定array与有效位、按序观测，不变更App；`FinishInputCall(const InputCallContext&,const InputObservation&,const SemanticChecks&)` 在App锁外保存一次，首失败锁定；`MakeInputError(napi_env,const InputCallContext&,ErrorCode)->napi_value` 只负责普通错误JSON或保pending exception。`ReadWordsResult.words`仅copyComplete才可用；bool薄委托仍可兼容旧调用式，但不得丢掉上下文/首失败。native声明落 `entry/src/main/cpp/types/libgrsm1/index.d.ts`，route/api为固定enum，不接任意字符串拼日志。

若真实原件显示“文档承诺成功但实际runtime返回另一单位/错误”，先报API status+SDK/runtime/包身份，architect再修兼容方案。此轮不授权 `length==N || length==4N`、offset任意、base不等也copy、跳过detached、number[]兜底等猜测式修复。

## 3. 有界拒因诊断与安全返回

新增冷端点 `gpuInputDiagnosticsV1(route:string,attemptId?:string):string`，route为M3A/M3B/G9枚举，attemptId为十进制U64字符串。诊断请求不走故障的typedarray解码器，也不覆盖被查输入回执。每route最多lastAttempt和本诊断generation首个failure两份固定512B快照，三route合3072B native payload；单次响应UTF8≤4096B。attemptId不匹配现存快照返回SNAPSHOT_EVICTED/NOT_FOUND，绝不拿旧回执解释新调用。generation绑定native boot，并在新run配置事务**已提交为REQUESTED**后换代（不意味着GPU创建/READY成功）；失败的configure不通过先清空原始拒因来“恢复”。

单次响应只展开一个selected snapshot（默认lastAttempt，指定attemptId可取首failure），首failure另以ID/stage引用；不把两份最坏case重复展开突破4096B。诊断序号耗尽具名报错，不wrap复用ID。

快照字段：revision=`GPU_INPUT_DIAGNOSTIC_V1`、route/api、boot/diagGeneration/attemptId（U64字符串）、expectedN/expectedBytes、firstFailedStage、各已执行NAPI的原始数值status、typed/type/length/offset、dataPresent/basePresent/baseEqualsData、backingBytes、detached、copyComplete，已复制时才含配置header最多前16word，semanticChecks各PASS/FAIL/NOT_EVALUATED。原指针地址不落盘；未执行项是null/NOT_EVALUATED，不伪0。该准入诊断与业务firstCause/GL/EGL/clock故障分离，不覆盖已有firstFailure。成功高频pulse只更新固定内存末次状态，不每帧hilog/文件I/O；每generation首次拒绝可有一条有界日志。

接口返回分两类，显式登记，不全局改成另一协议：

- **JSON端点**：g9ResearchConfig、m3Session/g9Session、M3/G9冷capability/state/page/begin/end及M5同类。原成功revision/words/身份仍原样；普通失败也必须是对象JSON，统一错误revision=`GPU_NAPI_ERROR_V1`、ok=false、state=REJECTED/NOT_READY/STALE等、code保原token、api/route/attemptId/diagnosticRevision。错误不含成功words/identity/page体，不因旧token是字符串就返回JSON字符串标量。每错误≤4096B，完整闭合、不截JSON；只引用诊断ID，详细快照从冷端点读。
- **状态端点**：m3Configure/g9Configure、pulse/target/offer等原具名字符串/十进制结果保持兼容；它们不被JSON.parse。调用方必须按具体成功token/返回0核准，失败可读上述诊断，不能凭“返回了字符串”进入RUN_REQUESTED。

存在pending NAPI exception时不能假称已返回普通JSON：保原异常/原status，按平台NAPI规则返回异常哨兵，由UI try/catch接住；不得无条件clear pending异常再报成功。普通拒绝返回完整JSON的路径不得另抛异常。序列化只用有界writer并正确escape，不直接拼任意runtime错误文字。

快照使用独立小锁，输入检查在App mutation之前；已需App锁的语义判断先把局部观测带出锁再记诊断。不得App锁内写文件/打印或拿诊断锁形成反向锁序；不在GL owner增加读回/同步/5th binding，不改变期限、GAP和input消费身份。

## 4. UI启动事务与解析边界

GpuResidentG9.start整体（包括cancel/archive关闭、setActive、读取config、configure和frame安排）进入catch/finally保护。新启动意图撤销旧frame owner并置frame=null、identity空；任何失败都不安排新pulse、不自动重复configure、不递增命令当作已消费。只有具体 `G9_NEW_EMPTY_RESEARCH_RUN_REQUESTED` 成功才初始化新frame。M3/M5同类启动入口同步按其成功token核准；不把本卡变成新算法或参数默认变更。

共用 `parseNativeObject(raw,api,allowedRevisions)`：先核raw为string/非空/原接口UTF8上界，再catch JSON.parse；根必须非null对象且非array，按对应revision验证error或success字段。config.words必须恰声明长度、每项safe integer且0..0xffffffff后才创建Uint32Array，不能用typedarray构造自动截断/模2^32掩坏返回。session.identity十八word同样检查。识别GPU_NAPI_ERROR_V1就保code/attemptId并停止；遇旧包裸token只能以LEGACY_NON_JSON_REPLY显示和留原始有界串，不继续运行，兼容不是“接受旧错误为配置”。JSON字符串/数字/null/缺字段/错revision/超长都受控失败。既有tick catch继续兜底，但不能代替start事件catch。

失败路径保原接口名/首次原code/attemptId，清理错误另列；取消cold/关闭archive/尽力setActive(false)也要单独catch，不能用二次清理异常覆盖首因。页面呈现明确FAULT/当前不可用或标明STALE上一有效完整新管线帧；只有XComponent+shader，禁Canvas/软件GL回退。UI错误提示不把typedarray猜测写成已确认原因。

## 5. 分流和精确补证

**交developer工程卡**：不使用真机。不允许修改 `D:\hmprojects\HelloWorld\docs\后续工作清单.md`（仅本人行状态有限例外），不更新滴答同名清单。完成共享严格解码+具名诊断、JSON端点安全错误、状态端点兼容、G9/M3/M5启动/解析保护，更新canonical源码及T522/T529/T517 generator的同源复制/声明，交编译签包收据；不私删门。职责只开发/编译/提交，不执行测试，不把tester/device加为开发关卡前置。保XComponent+shader唯一上屏、defaultOFF、CPU停投、30fps显示/120Hz配额、root250ms/GAP33333333ns。要么完成本卡当前约定范围及本角色必要检查，要么遇到确实无法继续的具体问题才停止；不能做一点就停下。局部受阻先做其它项；具体阻断依用户规则报告全部未完成/原始错误/证据/尝试/不能继续原因/输入前置/接手角色，原卡评论并本人行看评论；成功结束待验收。

**补证交tester（T542/T545原卡）**：需上述诊断实现的正确新签包，使用真机、设备串行。只做一次每route明确N2/G9研究配置和N16/M3配置的最小入口，先核包/设备so/PID/boot/SDK/runtime，再保UI发出数组的type/N/byteOffset/byteLength及预期词、native同attempt诊断原JSON和UI结果、原始hilog/若仍异常的jscrash。不要先展开长GPU成本矩阵，不延长等Create。判读：(a)具体NAPI API失败/NOT_EVALUATED → architect核runtime契约；(b)API全过但某长度/指针/bytes门失败 → architect核精确值与支持域后developer；(c)copy全过而profile/header/cold失败 →工程值/调用链归因；(d)两routefirstStage不同 →分别处置；(e)两者相同也须原值/环境相容再称同因；(f)成功入GPU才续原资格/成本。所有补证回architect，manager不重跑、不新增验收门。

本轮已做只读原件审计、静态分支及有限契约向量，证据 `analysis-T546-rejection-evidence-v1.json`；具体设备谓词UNKNOWN是被精确定位的后续输入，非本工程稿遗漏，不标产品PASS。
