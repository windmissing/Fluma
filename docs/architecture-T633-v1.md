# T633：G13 配置未达、零场和初始化可观测性归因

2026-11-19，architect，不使用真机，不构建/测试求解器。结论：**本轮已捕获的最早失败是NAPI解码，未证进入GPU初始化**；另有包来源、未配置激活和状态表达工程问题。五图黑屏按同一零accepted-field阻断合并，不派五个渲染故障。

## 原件优先的执行链

T609 `l3.json`：g13Configure attempt2返回NAPI_DECODE/error6、stage=TYPEDARRAY_INFO、rawNapiStatus0、snapshotValid=false/copiedWords0。实际签包源`.codex/local-test/t608-delivery/.../bridge.cpp`在此把type/count/data/offset合并判定，失败即不Submit owner Configure。不能从stage知道究竟type、length单位还是data问题；尤其不能把T621的四倍长度测量冒成本G13已观察值。此类接口问题沿T629/T630共用适配/资格线，本卡追加实际G13 consumer接入需求，不重复建立“typedarray不兼容”归因。

T609独立保存的g13-status和T610保存status均earned0、scope几乎全零、firstError0、capability/compile null、root nonce0。T609布局l5确有earned1749/l6–lA有4146并discarded递增，但run/context/config仍0、generation0；须按各自sample/命令分列。earned4146说明控制账在未配置域进账，不证明GPU初始化走到fence，更不证明配置重应用死循环。native firstError只反映controller错误，NAPI失败未写入该域，所以0不是配置成功。

## 包来源偏差

T257签包收据的输入来自`t608-delivery`，unsigned SHA `68b5a89203923ae5d78a15bcd1dcdeb9ba35d0de9777f4ad5bf5bb65223d7ed8`；实存签包SHA `a49229ed9d6ca2a8e0c52b4090a542d1951639320554e44d10d5dbd899484c6e`，包内libgrs_g13 SHA `b0d06f05555ea4888f09adcb3a2eba4548bc3979a0badf8a8f50f1d4cc731a7d`。T608正式development.json最终prod交付却是`t608-release` unsigned `395d8ea7e7207323bccb22da734a0b543582ecc6ecd5fc377206ce804eb2d3d7`、ELF `92414d34cdfda2ea616cb14914d041eccf7feec6fec7470d81246f6e48eb2595`。不是同一输入，早期bridge与release源也不同。

保旧原件为真实早期包结果，不冒dd91018最终包复测。tester负责选取正式已有最终产物核hash、签名/部署/实际加载映射，**不退developer另做签包卡**。这不证明最终包已修复解码或能运行；后续T629/T632新构建另记新身份，不套旧math/config SHA（沿T628过渡规则）。

## 已证静态工程缺陷

1. Status在从未成功Configure时也兜底CONFIG_PENDING，误导为已经冷初始化。2. Controller::SetActive没有配置/Ready门，返回void；不匹配id只Stop，bridge仍回ACTIVE并把Controller零identity复制回App。再次零scope激活可能被接受，AcceptPulse只查session/active即可在未配置域赚账。3. 页面激活按钮无Ready约束，tick可覆盖配置失败message；g13Status/getLastCommand又会被pulse盖过，无法保首解码错。4. owner Create及surface callback没有固定失败阶段/原API状态快照；decoder不输出raw type/count/offset等，现有只读status不足以定位后继具体GL卡点。

恢复工程不修改算法、GAP或配额：`SetActive`改具名结果，必须Controller已Configured且Transport Ready、非retiring、scope完整相等，失败回CONFIG_NOT_READY/STALE_SCOPE，不回ACTIVE、不替换App有效identity；AcceptPulse同样先查配置和Ready，未配置不进earned/reserved/consumed。初始status为UNCONFIGURED，CONFIG_PENDING只在实际接受且未完成的ConfigTicket存在时使用。页面配置失败保留可查firstConfigFailure，激活disabled直到同ticket COMPUTE_READY；用户重复点击不制造session0/新scope。

## 可执行初始化状态链

唯一owner仍控制EGL/GL；每次配置持不可变ConfigTicket{boot,attempt,queueSerial,proposedIdentity,configRawSHA,seedReceiptSHA,cancelGeneration}。顺序：DECODE_OK→CONFIG_QUEUED→WAIT_WINDOW/CONTEXT_CREATE→CAPABILITY→PROGRAM_BUILD/REFLECT→ALLOCATE/SEED→INIT_DISPATCH64→INIT_AUDIT65→INIT_FENCE_PENDING→INIT_TAIL_VALID→COMPUTE_READY→显式ACTIVE→新session首pulse→真实SOURCE/QUANTUM→ACCEPTED_FIELD→scene呈现。decode失败不创建owner ticket；WAIT_WINDOW不是GL_FAILED；各阶段ENTER/EXIT和原错单列。

现代码Controller::Configure返回0后会提交初始fence，Controller::PumpBounded在!live且Transport Busy时调用Complete/Poll，理论上能退休初始化；不能预先宣布该链永不执行。deferred Configure的error34只是busy/lease/retire，不能冒“冷dispatch失败”。修订owner Loop为独立ServiceLifecycle：只要有配置/退休/初始化工作就推进其现有非阻塞poll，不依赖ACTIVE/pulse/业务root；业务BeginGranted仍须Ready且Fresh。deferred重试保同ticket/proposedIdentity，不在每次重试创建新run，scope/窗口/撤权变化须取消旧ticket而非拿新identity执行旧请求。初始化结束后才提交有效scope；Stop/resize销毁按真实fence/context退休，不因逻辑取消提前释放。已有完整旧帧可标STALE，未产生帧显示具名等待/故障，不用Canvas填黑。

## 有界具名seam（developer）

增`G13InitDiagnosticV1`固定32KiB：4个decode/config快照×1024B、64条stage×256B、2个原始log槽×4096B、控制/summary4096B。计入固定native诊断metadata，超限first+last/dropped/truncated显式，不借冷档payload/新全场副本。阶段含sampleSeq、boot/ticket/queue/attempt、window/context/owner tid、op/program/sourceSHA、ENTER/EXIT monotonic、原napi/EGL/GL status、init pass64/65、fence返回、ctrl[255]/ctrl[7]及sealQualified；未读ctrl保NOT_OBSERVED，禁止零冒成功。

在现有实际调用前/后发观察，不额外glFinish、glGetError、map或改变GL序；沿原错误采样点保存真实raw值和首因。program factory提供route13的只读观察hook才能区分shader compile/link，不能用“Build返回前只有ENTER”猜具体驱动调用。shader log保原始bytes/长度/截断，编译输入SHA可定位完整源，不把全shader塞日志。

`g13Status()`只读固定summary（≤16KiB）含currentLifecycle、lastProgress、firstConfigFailure、decodeProfile、queued/handled ticket、cache/compile/capability资格，owner阻塞期间仍可读取最近ENTER。补`g13InitDiagnostics(attemptId:string,page:number):string`只读已存快照，页≤16KiB、最多4页/总32KiB；它不向阻塞GL owner投递请求，不调用GL、不触发Configure/Resume/Root。mutex只复制固定记录，调用外部API时不持诊断锁。T629同一decode adapter给真实rawReturnedLength/kind/offset/data/backing/profile，保T621 UNKNOWN安全拒绝；controller首错、decode首错和export故障分轴。

## tester 原卡接力

先用正确签包与同版本schema保一次配置请求/原reply/同ticket初始化诊断，若仍解码失败只取实际字段交T629/T630，不继续60s等GPU。解码通过才核surface callback与后继阶段；观察窗口≤15s只作为一次补证窗口，不设新的产品编译性能门。无进展导出已存ENTER/EXIT/原错及线程等待证据，不延长等待，不推测driver。原API尚未返回时保持ENTER_OUTSTANDING，不unsafe销毁其context。

Ready后用有效scope激活/首pulse，核来源域/earned、真实root dispatch/尾fence/seal，再核accepted field、五图同token/30fps与冷原件。REST零速度可合法，accepted field缺失才是这轮零输出，不能拿黑色证明数值零解。F01–F16目前NOT_RUN，五图NOT_OBSERVABLE保持；与T632数学修订并行，无互设新门。

职责：tester处理既有包与精准补证；developer修G13生命周期/诊断/实际decode consumer并编译，provider取证归T630；未知后继失败回architect。完整prompt约束见`architecture-T633-T636-execution-boundaries-v1.md`。
