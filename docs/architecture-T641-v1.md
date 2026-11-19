# T641：G9 接收时间位移拨动独立 profile 工程闭包

> 2026-11-19 T647接受链修订：消费`architecture-T647-v1.md`及acceptance机器表G9_RX行；完整真实DM审核16/17、预测/校正/接受时间必须实现，不能复用当前错误2024默认gate。GNR1的12/2404、DM2完成尾及RootConfig76..83保留，Runtime表已登记正式依赖；可同源生成专用validator，不等默认DM设备PASS、不CPU补位。

2026-11-19，architect，不使用真机；消费manager对T637候选的采用裁定、`algorithm-T637-v1.md`、manifest/fixtures及实际g9dm源码。结论 **ENGINEERING_READY_FOR_T644 / PRODUCT_NOT_IMPLEMENTED_NOT_DEVICE_VALIDATED**。本卡交完整实现合同，不实现产品代码、不构建或测试产品；仅对独立布局标本作AArch64 syntax-only编译。后继T644开发编译、T645独立验证，不增manager批准/集中准备门。

正式表：`architecture-T641-wire-v1.json`（Config64/RAW64/INPUT128/RECEIPT_CONTROL256/RootConfig128）、`architecture-T641-runtime-v1.json`（入口/能力/映射/哈希/锁序）、`architecture-T641-storage-cold-v1.json`（40960B/冷布局/资源）、`architecture-T641-seed-config-v1.json`（两档完整构造与SHA）。生成/只读有限检查为`analysis-T641-engineering-v1.py`，证据同名JSON；不是物理oracle或产品PASS。

## 1. 新域、现有资产和逐文件接线

profile=`G9_DM_NATIVE_RECEIPT_SPATIAL_R1`，code=LE32('GNR1')；输入域`NATIVE_RECEIPT_SPATIAL_CONTROL`/3，默认OFF、显式研究入口。接收时间只用于资格/年龄，J与cap用位移和**固定Dref125000000ns**；同有效端点、不同合法接收时长必须同J/cap。不声称真实手速、手功、硬件采样年龄、UI前延迟、完整上游轨迹或漏通知休眠的墙钟TTL。旧g9spf REAL_TOUCH_R2、g9dm SYNTHETIC、T616路线A保持原语义/未资格状态，不将本域叫REAL_TOUCH_R2或CLOCK_SYNTHETIC。

实际主树有g9dm Controller/Transport/Scene/Cold组件，却未找到完整dmResearchConfig/dmConfigure/dmSession产品页桥；GpuResidentG9当前连接g9spf，并非DM入口。旧g9dm InputGate仅准synthetic，ModelEnvelope会重盖命令时间，Controller多处max(now,Now())，不能拿来直接承载新域。g9dm Cold还残留旧g9spf字符串和较小容量，ColdScope中kEngineeringSha为短占位字符串却按64B复制；这些路径都不能原样复制为新route。它们是本次新增接线范围，不是等待旧产品页出现的外部前置。

| T644 文件归属 | 完整职责 |
|---|---|
| `gpu_resident/g9rx/{contract_generated.h,contract_generated.glsl,reader_schema.json}` | 从本四表单源生成所有版本/字段/偏移/掩码/错误/完整12与2404调度，拒旧，不手抄。 |
| `g9rx/{receipt_clock,loaded_identity,receipt_ingress,geometry_witness,rx_protocol}.{h,cpp}` | checked时钟/模块身份、唯一同步接收通道、双身份、RAW64解码、固定存储及本稿状态机。 |
| `g9rx/{controller,transport,scene,bridge,cold}.{h,cpp}` | 一套真实owner、FIFO/父时钟与根事务、真实DM计算/场景lease、完整双record冷原件；不再嵌套一份旧Controller FIFO。 |
| `g9rx/{input_gate,receipt_recipe}.glsl`及DM新生成变体 | 替换身份/输入及Config访问接缝，GPU重算固定参考配方；DM动员/预测校正/守恒/来源/能量/CFL数学不改。 |
| `tools/g9rx/generate_assets.py`、新`kernels.cmake/CMakeLists.txt`及类型声明 | 新`libgrs_g9rx.so`，严格生成依赖/缺头或失败fatal、离线实际GLSL编译纳入编译检查；旧g9spf/g9dm/T549不覆写。 |
| `pages/GpuResidentG9DmRx.ets`、`physics/GpuG9RxPort.ets/GpuG9RxCold.ets`、Ability/路由注册 | 默认OFF→研究配置K192/512→会话/激活/首pulse→真实DM accepted frame→输入待命→同步onTouch→owner→GPU→新管线呈现→有界导出；错误具名回显。 |

新RxController拥有真实g9dm物理Transport变体和父ProgressClock/Planning原语，替换输入控制层；不是先运行旧CPU/旧R1，再假装成DM。独立NativeReceiptCapability不调用旧`AuthorizeClock(mapping.current)`或厂商弱proof provider，不通过synthetic=true绕门。

函数接线必须落实到同一真实实例：`bridge::TouchIngressCallback`先`receipt_clock::CheckedClockRead`再`receipt_ingress::TryIngress`；后者调用`SnapshotMappingBeforeParse/DecodeRaw64/ValidateStroke/ComputeSpatialRecipe/OfferOriginal`。`controller::OwnerTick`依次`MaintainExpiry/ClaimOriginal/FreezeRootInput`，交`transport::SubmitRoot`及新`SOURCE_VALIDATE_INPUT`，只在`OnTypedCompletion→PermitCommit→CommitAccepted`成功后`rx_protocol::ConsumeAndPromote`；`scene::PresentAccepted`成功后`geometry_witness::PublishPresentedMapping`。`cold::BeginCapture`在根边界`SnapshotCurrentAndBase`并pin真实bank，`WritePrefix/WriteSegmentPage`与`ReleaseCapture`遵父broker，`GpuG9RxCold`逐页写档交新reader。这些是T644必须提供或等价可定位的职责边界，不允许只做未调用helper；GPU提交前与接受commit前分别核年龄，不用present时刻冒根commit。

## 2. 完整配置、合法种子和版本隔离

Config64 magic GRC1/rev1，字段0..63完全沿T637并由wire表逐word细化：全部基础物理参数和8个DM值、Dref/资格/域/策略、physicalSpecSHA/inputPolicySHA/seedTemplateSHA、56..63零。researchOnlyFlags=1（仅显式未批准研究），pointPolicy1、domain3、physicalParameterRevision1、mobilizationPolicy2。两档数值沿DM_RESEARCH_R0，不新增调参或复用批准标签；参数域及关系沿T554/T637，不把动态输入资格当材料标定。

Seed表完整构造T552修正S40（列9=B=S+F、origin/source完整），保DM物理magic/revision2、profile改GNR1、header106..113取新Config[12..19]。先动态身份/parameterSHA=0生成模板→模板SHA→新Config64/parameterSHA→填真实run/owner/context/inputEpoch与header42..49参数SHA→**实际上传字节SHA**→五bank同一原件→完整GPU初审→accepted场景；不在hash后补缓存，不热改旧run/state.profile。K192模板SHA `38625623e360557b2a62f92beab32260885b4016a17eaa506f6a73845eab1ab7`，K512 `efe106e7063905a832541bbb8123fae2f3462af8cf51d9e160b9929d9ed457b9`；参数SHA分别`f7c133d23a49587c9905c6e5ac2e1d4739fe1271421446c9fd12a7791e62a858`和`a58ae49f86339c73cb5fdb6a6dd2b52229d6a876fe49f7331bc05b67140e07ad`。这是规格构造，不是已部署SHA；表内示例run7等身份不能拿来上真机。

新INPUT magic GNRX/rev3、profile GNR1；state/event/path/completion的DM结构/物理magic和revision2不变，profile/parameter/layout/algorithm/pipeline/cache/reader全另立。不能继承实际旧代码中的revision1或按`uRequiredPasses==2024`猜QUANTUM。RootConfig128保前8物理参数、16参数SHA/24layout/32pipeline/40input/48actualSeed SHA、64..127完整新Config64；新DM参数在word76..83，**不是**旧112..119。也不能继续用旧20标量ParameterHash冒完整Config64 SHA。

## 3. NativeReceiptCapability、加载身份和首取时

专属NAPI callback `g9RxTouchIngress` 第一项实际操作是CheckedClockRead（clock_gettime(CLOCK_MONOTONIC)），在get_cb_info、Product实例获取、解析、mapping/锁和任何投递前取r；不要先走通用Api分派解析再计时。`CheckedTime {bool ok; uint64_t ns; int rawErrno;}` checked sec/nsec/乘加，0ns可以是合法值，不能用0同时表示失败。clock_getres冷资格须成功且0<resolution≤1000000ns；binary64坐标操作要求FE_TONEAREST、no-fast-math/no-FMA，每入口复核不合则具名拒绝。reported resolution不是准确度/晶振精度证明。

Capability64有独立magic/rev、真实clock id/resolution、clock/lifecycle generation、完整9×U64 scope、实际loaded module SHA、编译时clock conversion source SHA、T637 clock/input政策SHA，字段见runtime表。LoadedIdentity冷读取本模块`dladdr`/ELF BuildId与可核文件、checked路径/fstat/文件SHA，收据≤1024B；采集失败或身份变化回CAPABILITY_UNAVAILABLE/LOAD_IDENTITY_CHANGED，不填零或接受JS verified。编译交付source manifest覆盖clock/转换/NAPI/映射源码与编译选项，**不内嵌自身ELF SHA制造循环**：模块whole-file SHA在加载后计算；source→ELF关系由构建sidecar/BuildId/独立T645映射核对。native校验的是公开API声明与本应用实现/加载身份，不宣厂商MMI链证明。T644交编译/sidecar即可，T645实际加载核不作为开发关闭门。

runtime.enums登记Capability状态bits1/2/4/8/16，恰31方可资格；nativeApiPolicyRevision=1、loaderPolicy=1表示本模块dladdr/file/BuildId/whole-file SHA policy。Mapping状态bits1/2/4，恰7才可输入；未知bits拒绝。每次read失败清CLOCK_READ_OK并撤销资格；重新资格须新clockGeneration，不靠旧31续用。BASE历史flags1=BASE_CAP_REFERENCE_ONLY、2=PRIOR_ROOT_ARCHIVE_ABSENT，只是冷档资格说明，不授live权限。

对单一同步UI通道维护lastIngressReceipt，对唯一GL owner维护lastOwnerTime；各自有序采样和因果比较拒回退，相同值合法。禁止全局max(now,last)掩盖倒退，也不要把两线程先采后调度的处理顺序误当clock倒退。根Claim/Freeze/commit分别必须不早于其因果rU/前一root时刻；真实回退撤销能力/活动stroke/待提交命令，原件保首错。

## 4. 同步RAW64与8ms验证

唯一入口`g9RxTouchIngress(raw64:Uint32Array):string`，RAW64另有GRWP magic/rev1，不是ROOT INPUT。字段包含事件type(DOWN0/MOVE1/UP2/CANCEL3)、pointer/changed/active counts、同一changed点9个坐标/比例/origin的IEEE64位型、opaque sample bits/presence及native期望GeometryKey/epoch/session/generation/parameterSHA；**没有r、J、cap或verified槽**。原sample缺失存0+absent，存在即按API所交Number位型存（含NaN或>2^53），不用于时钟/排序/配方；局部/窗口/屏幕等坐标字段必须存在且有限，不能用任意0补缺API。DataView LE封装同步调用，不通过Promise/worker/异步重排，不插补MOVE。

使用T629/T630已资格provider的严格TypedArray适配：uint32/64元素、offset0、backing精确256B、非detached，记录raw length/单位/规范长度，UNKNOWN安全拒绝；不得盲除4或N/4N二选。新consumer由T644接入共用adapter，不拷贝旧g9Words裸返回token。所有回包有revision/profile/domain、boot、scope/generation/attempt/ingressSeq、state/code、elapsed、known/unknown资格和首错；≤2048B，不在触摸热路径IO/日志大拼接。

clock后立即try领取ingress gate，再try协议mutex，复制固定mapping/cap快照及其actualPresentNs后释放锁，然后才解析RAW64。映射必须已成功present且presentNs≤r；若刚有更晚映射替换、无旧快照可取，明确MAPPING_AFTER_RECEIPT/WAIT_PRESENTED_MAPPING拒绝，不把稍后新帧当入口已见帧。再try协议锁核当前generation、状态、坐标/配方/队列，并在最后checked validatedReady时核0≤validated−r≤8000000ns（等号合法），超限取消stroke，**r不刷新**。锁不等待；不持锁调用NAPI/GL/loader/文件，不用sleep等窗口。

通道并发/重入→INGRESS_BUSY，atomic stroke-cancel generation保证活动stroke失效，已在排队线性化点之前的UP不得遗漏该冲突。已完成线性化的合法命令不因后到坏调用重复退款；过期scope调用不取消新scope stroke。不可在持旧记录锁时再排GL owner同步等待。DOWN重复取消旧stroke且本次拒；MOVE≤256只更新计数/摘要和取消检查、无力；UP active=0或1但changed必须原pointer，duration∈[8333334,1000000000]ns；CANCEL/多指/失pointer/geometry变/时钟换代/超时均不产生命令。r相同由checked U64 ingress序号全序，序号溢出新epoch而非wrap。

CANCEL允许无changed点：仅eventType3、当前scope echo、expected hashes及magic/rev/source保留，pointer/count/sample/坐标payload全零；native先核当前scope才取消，并明确CANCELLED无命令。DOWN/MOVE/UP changedCount必须1，DOWN/MOVE activeTouches必须1，samplePresent只0/1且absent原bits必须零；未知事件、保留非零或坏布局拒绝，不把缺点的CANCEL拿去算映射/配方。旧scope坏调用不清新scope，当前scope协议违规则取消当前stroke。

## 5. GeometryKey 与 FrameWitness

Mapping只由**本新XComponent+shader实际完整帧成功Swap且同token后校验通过**的GL owner发布；初始无accepted场/STALE/FAULT不能授权。RxClock可先冷资格，普通NONE量子先产生真实accepted场，再首次成功present取得输入mapping；不拿空白FBO或synthetic矩形放行触摸。

GeometryKey精确67 LE32/268B前像见runtime表：domain/rev/profile/pointPolicy、scope18word、geometry/viewport revision、mapping generation、rect原位型、scale/origin位型、angle、parameter/actualSeed/mappingSpec SHA。viewport尺寸由native producer改变时必先递增viewportRevision，不从不可信echo授予；record持revision和实际rect，完整数字viewport在native mapping/外部呈现收据中。**排除commit/publication/tick/root nonce**，所以正常30fps更新不取消同几何长手势。DOWN/UP的GeometryKey必须相同，点scale/origin须与native几何位型相等、origin=0；新policy明确局部vp，无未知旋转/transform时才接受。`px=localX*scale`等及模型映射逐操作binary64 RNE，半开像素框与右端Q20格拒绝，不clamp；window/display只诊断，不替代坐标源。

FrameWitness精确41 LE32/164B：domain/rev/profile、scope18、GeometryKeySHA8、实际physicsCommit U64、实际publication U64、pipelineSHA8；SHA写record downTokenSha/upTokenSha，publication另存。DOWN与UP允许不同帧证人，不比较commit相等。两实际commit用固定FrameCommitSidecar20×16B保留，并在新ColdHeader32..39登记四个commit，绝不由publication反算commit。这样reader能从record/header重算两帧hash，不只拿opaque SHA自证。它证明native成功呈现见证的绑定，不证明硬件采样当时屏幕或上游帧选择。

Geometry/参数/seed/angle/viewport/坐标policy改变先撤销cap/stroke/待处理输入；hide/show、Stop、permission/window/surface/context/run/config/inputEpoch变化亦如此，恢复新generation/inputEpoch。原已提交物理历史保留，未退休GPU/scene/cold对象继续按fence/context退役。owner成功Swap但暂时拿不到协议锁时标INPUT_MAPPING_PENDING并使旧输入mapping无效，下轮安全发布；不为了mapping在GL下等UI锁、不伪造present成功令牌。

## 6. FIFO、冻结、配方与唯一消费

FIFO16含SOURCE/IMPULSE，MOVE不占槽；满拒本命令，不覆旧、不合并、不自动重送。native分配commandSeq，不回收丢弃序号；SOURCE用户intent键独立保在固定sidecar，沿父有意义输入规则。IMPULSE绝不调用source/target注册来rearm BLOCKED。

原正文精确去重，不以stable SHA授权。StableCommand前像103word：domain/rev、profile、run、inputEpoch、parameterSHA和INPUT40..127原body；nonce/pose刷新不改变它。queued/claimed/lastConsumed窗口内相同seq同正文仅ACK，改正文拒；更老且未保留原正文的键明确STALE/TERMINAL_NOT_RETAINED，不伪称已比对、也不重入队。普通入口重复UP为IDLE，不再造seq。没有无限历史。

每次Offer、Claim、Freeze和提交查0≤now−rU≤250000000ns；body[43..44]始终原UP receipt，不随root/pose重盖。owner先维护过期队首，无live根时逐个具名丢弃（最多16），不为过期命令启动空根；已claim则撤候选并等真退休后按原slot/seq移除，不误弹后来队首。父规划仍RUNNABLE时后继有效头可推进；BLOCKED不由IMPULSE/过期维护擅自rearm。

Claim记录rootBegin/nonce/deadline（begin+250ms checked）、BASE_LAST_CONSUMED快照；Freeze更新合法manual observation envelope后固定完整INPUT128，再算frozenInputSHA并封CURRENT record。claimNs=rootBegin、freezeNs≥claimNs；提交同时满足父root期限和rU+250ms，不能延长任一门。pose可在真实授权pulse更新自身native观察时间；命令rU不刷新。原`ModelEnvelope`和旧`InputGate.Freeze`不能偷改新命令时间。

dx/dy按位型精确trunc(abs×Q)带符号，deadzone平方严格小于(Q/4)²才NO_IMPULSE；radius4，固定Dref125ms。GPU在既有SOURCE_VALIDATE_INPUT/CONFIG_INPUT_GATE重算Dref、flags、adapter、deltaQ、RNE、Jx/Jy/cap、SAT、范围和保留零，随后才有物理writer。固定分母使未限值参考速度恰为2×deltaQ；通用RNE依整商/余数ties-even，不从receiptDuration除出速度。|vx|≤24Q、vy≤32Q、mrefQ=Q/16；cap完整U128、所有宽值4limb或十进制字符串，不经JS Number。没有GPU float64需求、无新增gate dispatch，无CPU S/F/P演化。

实际DM Hamilton/来源/正功/余量/CFL/六J/trial/authority/拒前缀/accepted replay不改；命令上限不能代真实状态波速，也不能clip速度绕失败。`g9ConsumeCommand`在candidate写去重胶囊不等native已消费；仅完整typed尾、当前scope/clock/年龄、真实transport Commit同一成功路径后pop、存完整frozen INPUT并晋升lastConsumed。all-unapplied accepted照样消费一次；BLOCKED/FAULT/late/取消不改BASE消费记录，保原body可在原年龄内重试、nonce变化不加力/退功。

## 7. 40960B、锁序和实际sizeof交付

协议对象为`alignas(64) RxProtocolPayload { std::array<uint32_t,10240> words; };`，wire表定义每段，禁止std::vector/string/另份旧FIFO藏payload。20×1024B witness（queue0..15、active16、frozen17、BASE18、lastConsumed19）；四metadata各256B（config/seedMeta/capability/mapping）；1024B rootConfig/序列化scratch；16×64B journal；1024B control（前128word控制，后128word manual pose）；19×512B INPUT（16queue、frozen16、lastSource17、lastImpulse18）；剩余6656B桥/对齐。

桥区明确两RAW64=512B、reply2048、加载收据1024、20×16B帧commit sidecar320、两mapping scratch512、两个SHA workspace256、16×16B source intent keys256、保留1728，合6656。private draft与公开record不同类型：queue/active未冻结槽不冒RECEIPT_CONTROL256 status1，只有真正Freeze才写magic/role/status/SHA并进入冷档。lastConsumed克隆时设role2，不回填或改动旧CURRENT归档。

T644编译必须static_assert sizeof=40960、alignof=64、全部offsetof/extent和边界；另报RxSync（mutex/atomic gate等固定helper，≤256B）的实际sizeof，不暗称包含在40960内。编译期常量/布局符号或compiler record layout可交sizeof证据，不要求执行产品测试。父controller固定对象、线程栈、NAPI/运行时、driver/FBO/cache峰值仍独立实报。不得保留父22528B输入协议再叠40960B。

本卡`architecture-T641-protocol-layout-v1.hpp`独立标本已用SDK26 clang的`aarch64-linux-ohos`/C++17做syntax-only布局检查：payload及替代typed布局均sizeof40960/align64，段offset为0/20480/21504/22528/23552/24576/34304；RxSyncState实际64B/align8。命令、编译器/标本/原始layout日志SHA及结果见`analysis-T641-abi-v1.json`，复现脚本同名PY。没有运行程序/GLSL/物理或产品编译；产品集成后仍须报自己的实际布局，不拿此标本冒实现证据。

统一锁序：入口先clock→不等待的ingress gate→try协议mutex，复制快照即解锁解析，最终try锁验证并线性化；GL owner在锁外GL/IO/等fence，短锁仅转控制状态或复制固定原件。Transport PermitCheck重入同一协议锁前必须已离开提交锁，无owner→protocol→owner嵌套等待。readonly status/loaded receipt/导出页不发SOURCE、不授资格、不唤醒物理；journal只保16条、有lost计数/rolling SHA，不冒无限历史。ring64B含kind/status、generation、seq、nonce、time、rU、commit/publication，完整JSON外部见证关联同根INPUT_SHA；缺提交/呈现时刻不从frozen record反推。

## 8. 冷原件、双record与reader

新capability `G9_DM_RX_COLD_V1`、page `G9_DM_RX_SEGMENT_PAGE_V1`，新ColdHeader GRXC/rev1/seal0xC0DE6371，**只在新profile登记**header32..247元数据，248..255仍零；其余T637 prefix布局不动：CURRENT@3072、BASE@3328（均word偏移，各256word），Config64@3584、SeedMeta64@3648、3712..8192零。header40..103保实际Capability64，32..39四commit补齐FrameWitness前像，其余源/算法/布局/pipeline/config/seed/加载收据SHA、presence/history/journal资格逐word见storage表，不能偷用旧reader未知区。

SeedMeta64完整word表亦在storage：GRSM/rev1/64/seal0xC0DE6372，4..11模板SHA、12..19实际上传SHA、20..27参数SHA、28..29初始M0、30..31初始origin、32初始sourceId、33seedRevision1、34..35上传字节数、36模板算法1、37..63零。均取真实初始audited seed，而非本根BASE总量；本表不自hash。TerminalJournal每条16word有明确kind/status、clockGeneration/seq/nonce/observedNs/rU/commit/publication，环完整原件不塞冷prefix零区，按外部关联收据保存；header只存界限与rolling SHA，不凭摘要声称恢复已丢历史。

CURRENT只在本根有真实IMPULSE时存在；NONE/SOURCE根CURRENT全零。BASE是开始本根时物理BASE内最后真正消费IMPULSE的record，初始无消费全零；SOURCE不晋升它。已知记录保留0..239真实字段/240..255零，magic/role/status1表明“冻结原件”不表已接受。对应FrameCommitSidecar随record同槽拷贝。BASE state胶囊在word384..511，可核原命令正文/身份，完整原INPUT可用时还可核其SHA；前一根完整物理历史未提供不能宣整个旧根replay。BASE旧cap generation不硬套本根Capability64，具名BASE_CAP_REFERENCE_ONLY/PRIOR_ROOT_ARCHIVE_ABSENT；当前实时资格和历史重放资格分轴。

根提交/present时间**不回填**record；外部生命周期原件用clock generation/nonce/seq/INPUT_SHA匹配并验证原age。离线COMMAND_REPLAY永远读冻结rU/Dref，不用当前墙钟拒老档，也不把旧档重新塞live队列。MOVE摘要按runtime表domain/初值/每步/结尾精确LE前像，所证只限实际送达点，非全硬件轨迹。

ROOT/FULL仍4/10非prefix段和父DM每段原件，prefix无self descriptor，外部rawPrefix/payloadRaw/pagesFile SHA分名，末manifest不自hash。actual raw bytes、EVENT64+PATH16子范围、每页≤1024U32/16KiB、caller-span、进程共享broker两slot/单builder/reader/60s不续、实际free退信用全部生成表驱动。冷builder固定caller-span直接填，GPU/scene pin实际退休前不覆写；不在JS整档聚合、不增第三slot。使用本表两档新容量，不能沿当前g9dm残留较小常数/旧g9spf page字符串出档。

K512已知包络=98022528+140009472+376119296+40960+16384=614208640B；K192对应由表计算。DM SOURCE12/QUANTUM2404、G1F4最大9628、≤875520 DM Eval、4SSBO、正常INPUT512B/CONFIG512B/完成尾1024B和一fence均不增。seed临时591872/673792B在冷builder前真实释放；旧profile slot/pin/资源未退，切换WOULD_BLOCK，不逻辑free冒归还。30fps/250ms root/GAP33333333ns/120Hz及物理门不放宽。

## 9. T645 独立接口与判定

生产只暴露runtime表列明的研究配置/session/active/pulse/target/同步RAW/source/status/loadedIdentity/cold APIs；**没有可提交任意r/J/verified的通用IMPULSE API**。T645 native组件harness可在独立test-only构建注入ClockReader、特定验证停点、坏冻结INPUT/重复正文、真实同核心FIFO/FrameWitness fixture；宏及test build/来源身份分离，禁止测试布尔/时钟传入生产入口。测试脚本/夹具/签名部署/执行由tester维护，developer只实现具名test seam并编译。

独立覆盖T637所有fixture族R/A/V/P/D/S/N，并重点取原件：首取时前无解析、8ms±1/时间相等与回退/getres边界/模块失配；rU年龄Offer/Claim/Freeze/Commit±1和父deadline；批量/整体迟到仍sampleAgeUnknown；同位移20ms/200ms同J/cap；多指/UP零active/MOVE256与257/暂停恢复/重复DOWN；仅commit变应可UP、GeometryKey变必须取消；FIFO16/满/过期队首/幂等/改body/nonce、BLOCKED不rearm不消费、accepted全unapplied消费；GPU坏J/cap/flags/Dref/adapter/混版在writer前拒；CURRENT/BASE/role/保留零/四帧commit/哈希/真实冷reader；真实S/F/P、功、来源、CFL/replay、root/GAP/30fps成本另测。

status给stage/firstFailure/原API错误/known-unknown、queue/claimed/consumed及nonce，readonly不等待GL。能力未实现/实际provider未知记准确NOT_IMPLEMENTED/UNQUALIFIED；只有输入新域资格、命令重放、物理接受、可见帧四域分别有原件才能各自判定。开发编译不等这些设备证据，不把对比旧REAL_TOUCH资格或T616厂商bundle设作新profile前置。

## 10. T644 一次完整实施 prompt

不使用真机。按本稿四表与T637设计，一次交新g9rx实际library/页面/入口、首取时/加载身份/同步协议、独立Config/合法种子/新INPUT/GPU配方gate、真实DM调用/六J与原子消费、scene与双身份、固定40960B/锁序退役、CURRENT/BASE/完整cold collector+reader和独立验证seam；维护生成链、编译和ABI/sizeof收据，不能只交schema/helper/入口壳。现有DM页面/桥缺口属于本卡完整开发范围，不等待并不存在的旧页；源/模块签包执行准备归tester，不设开发设备门。局部缺真实平台接口时先完成其余部分，再原错精确回architect，不用synthetic伪运行、不自降门。

要么完成本卡当前约定范围及本角色必要检查，要么遇到确实无法继续的具体问题才停止；不能做一点就停下。局部分支受阻先完成其它可做部分。阻断回执含全部未完成项、复现/原始错误/原件与SHA、已试办法、为何不能自行继续、所需输入/真实前置和接手角色，原卡评论并本人行看评论；成功本角色交付结束待验收。评论不可用留本地报告并明确未同步。

不允许修改 D:\hmprojects\HelloWorld\docs\后续工作清单.md（仅本人行状态有限例外），不改其它列/他行/队列或滴答同名清单。共享源码不回退他人工作；GPU驻留优先、CPU停止投入。全部前景背景只XComponent+shader，故障FAULT或显式STALE上一有效新管线帧，禁Canvas/软件GL产品回退。目标1–11不变、10暂停，水线13→14→12，旧湿M1保持暂停。tester/effector设备执行标真机且串行，失败先architect分流，不新增manager验收门。


## 2026-11-19 T665：RX capture 生成兼容与单源身份修订

[T665](architecture-T665-v1.md)及architecture-T665-contract-v1.json核实际T645包与T644旧ELF：native BuildId不同，生产shader均为同一145268B/74c820ec…；设备1728行拒g9Capture内slot局部复用。仅RX生成路径作具名严格唯一的scope hygiene（clearSlot/scanSlot/selectedSlot及显式scan块），冻结父源/默认线/DM数学/T647门不改；同一canonical LF原串贯穿comp/header/actual ELF/hash，完整转换/依赖/cache/reader身份与最终生产编译检查由developer实施。既有harness原件有额外身份检查差异，不能冒生产源码通过。修后原T645签名部署/设备完整未完续办；不借T666幂等构建作新shader门，不宣LOAD或静态编译为产品PASS。
