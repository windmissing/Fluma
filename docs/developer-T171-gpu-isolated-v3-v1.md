# T171 GPU 隔离 V3 开发交付

2026-10-27；依据 T169 5444bb9 §2–5。不使用真机，未改生产 entry。V2 生成器/ETS模板仍独立保留，native 诊断扩展有新源码身份，历史设备测量不重写。

## 负载、共同证据与计时

新入口 `test/p013_compute/tools/generate_transaction_project_v3.cjs`，输出 `.codex/local-test/t171-project`。先执行原V2生成器，再按9项V3 overlay白名单逐笔替换；每笔包含原/后SHA、原换行及逆diff，能逐字节还原到V2结果。源manifest绑定全部模板/native/fixture及13项原参数；wire V1/canonical identity V2/原CPU merge算术及预算守恒审核均保留。

四个白名单fixture：WET_192_64、WET_192_128、WET_512_64、WET_512_128。仍由原public injectGestureSand先注 nmax×.1，再注64×.1/128×.1，固定列64/source17/x64.5/y100/seed13及两次sequence/inputTime，steps不超过32。不手写事务池或修改准入。

32步离线实测：每route/fixture实际提交merge为64/128，selectorCalls同为64/128，三路场、13组池原字节及标量、来源/预算原bits、pair与原F64、事件/lineage、refund均相同。GPU臂为明确标注的CPU interface stand-in，**不提供GPU执行或设备收益结论**。

V3共同记录为冷定容数值缓冲：selector 16384×32、event 65536×48、lineage 131072、page 8192×12、refund 16384×20；所有route同档。首尾完整状态、成功页在清尾前冻结，rollback/audit/λ/candidateRole计数保留。超出容量标overflow并使本次测量无效，保留原物理执行，不静默截断冒完整。此容量对当前四fixture已观测未溢出，不声称任意用户负载均够。

正确性臂输出raw F64 hex，避免JSON丢-0；性能臂关闭全扫描oracle及大dump，保留权威身份/SHA/prepare/原谓词/事务audit、逐请求身份与分项时钟、最终物理状态和选对/事件/refund摘要。FNV32仅关联摘要，不冒SHA身份或独立正确性证明。

PERFORMANCE固定3warmup/route，然后30个完整配对block，六种排列各5次、seed13打乱；失败原样记入既定schedule，不补抽，资源退休失败则故障终止。GPU context跨CPU/GPU块保留到末尾reset，每个route都冷重建field。候选ordinal保持会话单调，attempt相对ordinal另列。

时钟域：ETS/UI/Worker均OHOS ACTIVE uptime毫秒，native独立steady_clock毫秒，二者不直接相减；Date.now只关联。分列queue、冷GPU setup、field构造、完整physical attempt、编码、write/fsync、cleanup和UI收到报告。请求层含bridge、native校验、prepare validate/freeze/sort/range/hash-pack、upload、dispatch/barriers、fence、map/copy/unmap、consume及NAPI回传对象编码；嵌套区间不相加，未完成native阶段NaN导出为null。无GPU timer则NOT_MEASURED，fence只称CPU等待。

nativeTimings每行32个F64：0..14依次validation、prepareValidate/freeze/sort/range/hashPack、prepare总、upload、dispatch、fence、map、request总、consume、replyObject、returnEncode；15=ETS bridge；16..29为owner/context/request/payload/mutation/attempt/substep的lo/hi；30/31=candidateRole/全局ordinal。native状态/原错误另列。共同pairRows的12是selector墙钟、30/31是原workspace owner/run；物理摘要排除这些时钟/身份和mutation serial，与身份原文分列。

## 真实注点和常驻日志

只在生成隔离target定义P011_GPU_TEST_INJECTION；非测试预处理结果无armTestInjectionV3/testIllegalUpload接口。cold plan绑定run/env/thread，默认NONE、request前一次武装/一次消耗，不提供生产注入。

- QUARANTINE_GL：成功upload后真实glBindBuffer非法target，由正常glGetError进入现有slot4/quarantine；实际原码/第二请求BINDING_UNAVAILABLE/reset退休/new owner恢复由Worker记录，设备未执行。
- OWNER_EXHAUSTION：专用State allocator冷置MAX-1，使用与正常进程allocator相同CAS函数；倒数有效代成功，reset后再configure真实返回0/OWNER_GENERATION_EXHAUSTED，保持MAX不清零。输出process/test-instance/hi-lo，恢复明确要求新OS进程，不冒新env等于进程重置。
- STALE_REQUEST_HI / STALE_CONSUME_HI：分别在原identity校验前改变请求hi，及真实execute后改变独立current-owner镜像hi再调用原verifyAndConsume；不改prepared/result/lo。两路径分列NOT_RUN/STALE，Worker保留失败前后状态和reset后有效请求。
- SHADER_SHA_MISMATCH：只改本run私有副本实际字节，expected仍为build绑定原SHA；native V3额外核对编译期三SHA。原program实际read/hash拒绝，compile/link为NOT_RUN，记录路径/长度/mtime/expected/actual；使用同run完整原副本重新configure恢复。

三shader成功/失败都查询INFO_LOG_LENGTH/GetInfoLog；compile与link分别记录状态、实际返回字节、原长度、固定4096字节预算截断、日志采集原码。EMPTY/NOT_RUN/CAPTURE_FAILED/NOT_CAPTURED口径分开，显式长度字符串保留嵌入NUL，不覆盖first GL/EGL failure。State保留program日志到RESET。

## 规模微测与验证

MICRO模式是独立request-only synthetic矩阵，不冒完整事务：192/256/512×PAIR/NO_PAIR，同一确定族，3warm/cell，6个轮换block×4请求/cell=24稳态/cell；记录实际pool/group/Y/radius/K/h、source/upload bytes、context、返回和时钟。每cell-block新context、setup在请求计时外；此策略显式记录，不推三点近线性，也不把fence命名GPU时间。

- `node test/run_t171_gpu_v3_units.cjs`：10/10，四fixture×三路32步、参数拒绝、6排列、持续ordinal、release无注入符号、逐笔逆diff与原算术核对。
- `node test/run_t171_worker_pipeline_units.cjs`：2/2。实际生成Worker/field在SDK+NAPI CPU桩上执行：99attempt/一次configure一次reset；MICRO162请求（18warm+144正式）、42次真实桩reset，三规模/两族均覆盖。不是设备时钟或GL证据。
- `test/p013_compute/build_t171.cmd`：MSVC实际binding/prepare/owner+NAPI/EXECUTOR doubles回归；原T159断言保留；真CAS耗尽、两种hi-word拒绝/恢复、NUL日志与缺测值通过；另直接执行真实program()的私有文件字节变更→SHA早退，不调用GL冒执行。
- 原S2脚本仅将生成目录替换为t171-project执行，三路各12/12通过；原测试断言不改，输出s2.txt。
- 生成工程Native/CompileArkTS/PackageHap通过；SignHap仍为既有00303116密码长度失败，未改配置。日志及SHA/manifest见test/reports/t171。

T172独立正确性/协议验证及T173设备真实故障、收益、UI竞争等后续执行；本卡无设备收益、GPU timer、真实GL注错/恢复成功结论。生产迁移未授权。
