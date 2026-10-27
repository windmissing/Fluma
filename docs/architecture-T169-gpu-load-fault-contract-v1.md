# T169：GPU 高 merge 负载、真实注错与计时合同 V1

2026-10-27；architect；原卡 `6ae055c1e4b0127832eabca3`。不使用真机，不实施或重跑测试。消费 T164 `c217447` 正式报告、五份 JSONL、24×2 原始样本、diag 源码、T138/T152 及现行 T159 隔离模板。**工程设计交付；现有负载不证明性能收益/无退化，不放行生产迁移。**

## 1. 原始证据的成立范围与报告修正

| 文件 | 行数 / STEP行数 | queuedAt / receivedAt / completedAt（原Date.now毫秒） | wall / service+cleanup / queue ms |
| --- | --- | --- | --- |
| t164gpu32-GPU.jsonl | 40 / 32 | 1793083390353 / 1793083390370 / 1793083391178 | 825 / 808 / 17 |
| t164t12332-T123.jsonl | 37 / 32 | 1793083432498 / 1793083432514 / 1793083433289 | 791 / 775 / 16 |
| t164legacy32-LEGACY_SCAN.jsonl | 37 / 32 | 1793083464405 / 1793083464423 / 1793083465178 | 773 / 755 / 18 |

三路按 GPU→T123→LEGACY 固定先后、各一次，非配对交错重复实验。serviceStart 在 Worker 收到命令后、建native/field之前；completedAt 在disarm/reset与最终摘要构造前，**文件write/fsync及postMessage在计时之外**。GPU有冷EGL/编译/分配/READY及cleanup，CPU没有同等native启动；rows.push/JSON/快照/原池hex在service内。故它是这个隔离命令的冷端到端服务区间，不是稳态GPU微核，也不能当整段UI接收完成时延。

三路 AFTER 的13组poolBytes逐组完全相同，共12608B；按数组顺序将hex解码并拼接的SHA256为 `ab9e30bdd335ddcdc0bc65bd5870b3989e47ac9ed4ce50700912d13189b39241`，JSON字符串编码的SHA为 `9d0605cdfb532b208df49061eb0c885b56c4f0733d39f25d37b013c1445ad676`。报告“8cbd96b7…”没有对应编码说明，不能沿用为原池字节身份。AFTER.snapshot逐字段比较只有externalSourceState不同，其它字段JSON值相同；externalSourceState含每run身份，须独立分离可变身份后检物理值，不能删整个字段。poolBytes不包括live/free/nextId标量及完整source/预算/事件历史；本轮events三路均空数组，不证明事件级/lineage逐事务正确。BEFORE/完整snapshot、scalar和收据应另核，不由13组bytes替代。

GPU32三次请求全role1/attempt2/substep1，均PAIR、exec/consume OK、oracle AGREE，说明发生在第二次seed，不是每STEP持续merge。requestCpuMs合计 **3.445937ms**，prepareCpuMs另计 **3.960469ms**；两者还不含整个NAPI入参验证/消费oracle/回传和ETS序列化。因此不能将3.45ms当全部GPU路线额外成本或称成本可忽略。diagnosticOracle=true使GPU消费再跑旧扫描；CPU路由没有等价request归档，当前没有三路逐pair比较正证据。GPU512只有单路wall2711ms，不能归因为“物理更重、与路由无关”。gpu1是4STEP/12行，gpu32是32STEP/40行，报告混写应按原文件修正。

### 1.1 fence 结果只登记条件关联

原JSON各24条exec OK/agree/protocolPass，无须重跑。按偶数样本两中位数平均的p50：192为0.771016ms，512为1.527110ms；按既有nearest-rank p95（ceil(.95×24)=23）分别 **1.274840、1.846090ms**，报告1.228/1.804不是这一口径。min–max分别0.549844–1.818910、1.285780–2.005470ms，范围有重叠，不能写“不重叠”。Pearson(index,time)=-0.131476/-0.305827可复算，但弱相关不能排除温度/时钟/驱动/跨池顺序漂移。

diag先192全部采完，再新建512 context：Nmax与fixture内容、NO_PAIR/PAIR、K256/512、h8/9、11/12 dispatch、冷context及先后顺序同时变化。24条重复同池是条件重复，不是24个独立规模点；两点不能证明随N近线性，也不能将fence CPU等待直接命名GPU执行时间。所有leafTopSample均CPU叶数组，非GPU回读。archive列出的部分文件名与实际归档不同，后续manifest须从真实文件生成并保留源hash。

## 2. 高 merge 同输入完整事务：隔离协议 V3

维护现有 `T159_TRANSACTION_V2` 用例、steps 1..32和生成器旧白名单；新增**测试专用V3**命令/fixture，不默改V2输入。无需延长steps，也不改wire V1、canonical identity V2、CPU权威merge、预算/guard/audit/preview/λ及rollback。不得改主工程entry。GPU仍最高调度优先，合成负载明确synthetic，非T129冻结重放。

建议接口 `RunBlockV3{revision,blockId,fixtureId,fixtureSha256,route,steps<=32,oracleMode,timingMode,repeatId}`；fixture冷装载白名单资产，由build/source manifest绑定，不接任意脚本。配置ID包含所有原参数、fixture版本/hash、oracle/计时模式。fixture输入含nmax=192/512、原massPackage=.1、原字段初始化、固定列64、sourceId17、x64.5/y100/seed13及实际sequence/inputTime，三路物理输入相同；run/owner/context/request身份仍各自真实生成，不能复制旧token以求hash同一。

**第一版可执行候选**：保留原第一请求 `heavy[64]=nmax×.1` 填满池，第二请求从.25改为 `64×.1`；同phase/source/sourceId/state/列/高度的新包每占一个满池槽会请求一次原选对，预计在一个seed里产生约64次merge。该预测只用于造fixture，不当实测计数；RN拆包/容量/λ会影响实际数。再加128包候选（12.8质量），先离线合法准入/容量/质量及同输入三路验证，超出合法domain则报告拒绝或选较小已预登记候选，不手写pool/免费补床/改包质量以凑M。waterTop随原注入排水更新、所有来源账及票据由原public注入执行；不绕原准入。每候选保留同一32STEP预算，先用短正确性版本再完整32STEP。若实际merge仍少，标LOW_MERGE_FIXTURE，不将结果归GPU无收益；需要其它算法造池再交designer，不擅改选对谓词。

三路共同元数据应固定容量记录：selectorCalls、PAIR/NO_PAIR/error、merge提交数、失败/λ/preview/authoritative次数、每请求Nmax/live/group/poolMutation、完整pair IDs/slots/原F64及调用stage。CPU两路也记录，不能只GPU有REQUESTS。将最大行数与cold buffer容量绑定到fixture操作数/原Q上限，超出明确证据溢出，不静默截断。test-only元数据新增hunk须V3白名单及逆向还原校验；物理merge算术函数保持原字节。固定字段的物理状态比较涵盖场/13池buffer及标量、source/预算、refund、commit、每事务事件/lineage和故障before/after；在既有清尾前冷定容复制数值摘要，不能用结束时空eventTail证明历史。诊断导出在热栈外，所有成本显式归账。

正确性臂oracle ON，独立tester不复用GPU排序/range实现。通过后性能臂仅关闭诊断全扫描oracle/全层dump，身份/SHA/精确prepare/原谓词/事务审计不能关闭。三路共用同等计数档；性能臂也在批后核最终状态及选对摘要，不能靠关闭oracle掩盖错对。

### 2.1 重复顺序与时钟

- 冷启动另报每route每fixture首样本；稳态保持同一Worker/env和GPU context/buffer，在块尾才reset。每次物理field都从同fixture冷重建，计入哪一层提前冻结；不可让GPU路由省field初始化而CPU包含。
- warmup每route/fixture固定3次，不入稳态统计但保留原数据。稳态最低30个完整三路配对block：六种route排列各5次，固定随机种子打乱block；失败照计，不补抽到成功，不删除outlier。此为测量采样预算，不是新增产品通过阈值。设备执行串行，在途真机run不打断。
- queue→Worker receive跨线程采用同一设备单调时钟域，记录来源/单位；Date.now可留关联时间，不能冒单调测量。native steady_clock和ETS uptime分别记域，不直接相减。分别输出command wall（含queue至cleanup）、owner服务、冷setup、fixture构造、物理attempt、证据编码、cleanup、archive/write/fsync与UI收到报告。完整物理attempt包含全部seed/step/失败λ/preview/CPUmerge/账本audit；总计不得用分项p50相加。
- 请求层输出ETS bridge wall、native入口验证、freeze/validate/sort/range/hash/pack prepare、upload、dispatch+barriers、fence、map/copy/unmap、owner revalidation/oracle、NAPI回传。互相嵌套区间标清，重叠不累加；失败阶段未完成为null+状态，不能默认为0。
- 原始每attempt/请求带run/block/fixture/route/oracle/build/owner/epoch/serial及失败原因。成功时延p50、nearest-rank p95、max与所有attempt失败率/完成率同报；另报配对差及不确定范围，成功条件统计不隐藏失败。没有GPU timer则NOT_MEASURED，fence非timer。采用裁决继续沿T138完整收益/UI/资源硬边界，不另设任意百分比。

## 3. 四类注错：独立测试构建真实路径

新增 `P011_GPU_TEST_INJECTION` 编译宏，仅隔离生成项目定义；release/生产无接口无符号，默认NONE且握手含注错revision/source身份。注错安排由worker owner接收冷plan，run/序号/phase限定、单次消耗，拒绝跨env/thread或运行中随意更改。不能用返回ERROR/篡改rawGlError冒实际错误。

| 类别 | 最小注点与真实判定 | 必需证据与恢复 |
| --- | --- | --- |
| quarantine→reset | executor已准备/上传阶段，测试宏内执行一次真实非法GL目标操作；由正常glGetError采到实际GL错误进入现有slot=4/quarantined分支。若用deadline超时版本，必须实际glClientWait返回TIMEOUT_EXPIRED才算超时；已signaled不硬判timeout | 注错前后slot/live/epoch、实际GL调用和首次原码、binding ready=false、第二请求BINDING_UNAVAILABLE且无消费/写池；reset真实retire、live0后重新configure新owner/context，成功请求。reset失败保留资源/禁复用，不能假清零；故障事务回滚field/pool/budget/source/event |
| owner代耗尽 | binding allocateOwner的**隔离进程/专用allocator实例**冷置近UINT64_MAX；仍调用原CAS分配函数，倒数有效代成功，下一次真实返回0，configure走OWNER_GENERATION_EXHAUSTED；不靠GL错误注入 | 记录hi/lo与无回绕、无READY/无新提交；耗尽单调allocator在同进程不可“清零恢复”，正常恢复另开测试Worker进程/独立隔离执行进程且记录新process identity；不能只新建env假装进程全局atomic重置 |
| stale hi-word | 两个子例：请求进入identity校验前仅改变某个64位身份的hi保持lo；以及真实GPU完成后、verifyAndConsume前，测试宏改变**独立current owner测试镜像**的owner/context hi而不改prepared/result，走sameIdentity→STALE | 原/改hi-lo、GPU完成/consume阶段、拒绝先后、result不消费及所有权威状态不写；前置校验拒绝不冒“GPU迟到消费拒绝”已测。错误信封回显word篡改另类应PROTOCOL_ERROR，不混STALE。新有效身份的恢复请求须走原校验 |
| SHADER_SHA_MISMATCH | 优先无需native通用故障码：Worker在launcher拷贝后、configure前，向本run私有shader目录冷复制原三份，只改一份副本的实际字节；预期SHA仍是受build manifest绑定的原hash。调用原program()实际read/hash拒绝 | expected/observedSHA、真实路径/长度/mtime、compile阶段NOT_RUN、原拒绝stage、ready=false与清理；删/重建此run副本再configure成功。只改expectedSHA也能测拒绝，但单列EXPECTATION_TAMPER，不能代替实际文件偏移试验；绝不改共享rawfile/cache或生产shader |

四项当前为**测试通道NOT_IMPLEMENTED/NOT_INJECTABLE**，不是已发现产品缺陷；此轮首次分流到隔离工程实施。真实执行若不符合失败/恢复合同，再按工程/算法/用例回architect。T159宿主NAPI/EXECUTOR doubles的已有assert保留为离线工具证据，不能代替设备GL原码/生命周期。

### 3.1 compile/link 常驻字段

RequestDiagnostics新增每shader的source/expected/actualSHA、compileStatus、compileLogStatus/text/returnedBytes/truncated/missingReason，以及各program对应link字段，冷configure归档后保留到RESET结果中。**成功也查询** GL_INFO_LOG_LENGTH和GetShader/ProgramInfoLog；长度0=`EMPTY`，未编译（SHA拒绝）=`NOT_RUN`，取日志失败=`CAPTURE_FAILED`+真实原码，旧证据缺失=`NOT_CAPTURED`。不能以“无拒编”推“无可采日志”。文本有NUL按实际返回字节长度存，固定预算截断附原长/截断标志，JSON转义无损。首次GL/EGL错误与日志采集后错误分开，不能覆盖firstFailure；适用三shader成功/拒编、link成功/失败及SHA早退场景。

## 4. 最小规模补证矩阵

在请求微测另建同一确定生成族：Nmax192/256（同K256/h8/dispatch11）及512（K512/h9/dispatch12），各满池、共同PAIR族和NO_PAIR族；记录实际live/K/h、group/Y/range及payload bytes。每个6格先固定3warm，再6个交错block×4请求=24稳态样本，块顺序轮换规模/族；不要连续采完一池才换另一池。若要隔离padding，可加同live192、Nmax256/512一对，明确此对考察容量/K而非live。每block记录设备/driver/context和热状态、背景负载及采样时间，缺测就说明；不调用权限绕过或改SELinux。

先回答“受控族/调度下的条件等待差异”即可，不把三点自动拟合近线性，不由时间序相关弱排除漂移。完整高merge事务和此微测分母分开，不能用后者冒三路收益。

## 5. 接力任务书与所有权

1. developer（不使用真机）：只负责 `test/p013_compute/ets/t159` V3 launcher/worker/selector/native包装、生成器与native pair_binding/executor诊断和测试宏、隔离fixture资产/manifest；生产entry零修改。顺序先高merge合同/共同计数与计时，再四类真实注点及常驻日志；保留V2、旧白名单及wire/identity。开发交付以编译/离线自测及生成逆diff可核为界，不以设备收益为关闭前置。
2. 独立tester（不使用真机）：负责独立三路状态/每pair/original F64/事务/refund/事件/rollback核对、V3参数与边界拒绝、注点只测构建可用、计时缺测/phase定义/原码不被重写/manifest与JSONschema。测试工具所有权与native实现分开；不得只复跑开发断言称独立。
3. 真机tester：有效隔离包及独立无阻断结论后串行执行正确性→高merge交错收益→四类真实拒绝/恢复→必要规模矩阵；复用T165页面loadContent就绪流程，不改worker旧入口协议来绕COMMAND_INVALID。归档stdout/stderr/退出码、包/source/so/shader身份与安装覆盖情况；完成后按设备队列恢复需要的主工程包。UI渲染竞争若未实现标NOT_RUN，不冒产品收益。失败先回architect，manager按交付调度，无验收门。

所有派工prompt必须含：不允许修改 `D:\hmprojects\HelloWorld\docs\后续工作清单.md`（仅本人状态有限例外），不改滴答同名清单，不回退他人工作。离线卡明确不使用真机；设备卡真机标签、设备前置及串行队列。前景/背景渲染仅新XComponent+shader，无Canvas上屏/回退；失败FAULT或标明STALE上一有效新管线帧。生产迁移仍未授权。

本轮只读解析原JSON及源码、复算hash/样本摘要；未重跑GPU/物理测试，没有产品PASS。T164原报告修正需求在原卡回执及本稿登记，既有raw不覆写。
