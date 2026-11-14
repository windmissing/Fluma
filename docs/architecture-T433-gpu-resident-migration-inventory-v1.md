# T433 · GPU 驻留仿真迁移盘点与后续两阶段工程

2026-11-09，architect，P5。不使用真机，只交工程文档和派工建议，不改业务。依据当前源码、T421 签包来源及 T230/T233/T237/T238、T280/T281、T324/T401/T407/T417 已交合同。CPU 性能优化、补测和新证明已按用户指令停止，已有 CPU oracle 仅作可复用参考。本稿不重做 T421 预约修复或 T422 独立生产差分，不提前判 T410 通过。

## 1. 工程结论

**M1 的主要物理工作和场景数据已经驻 GPU，后续重点是减少现有驻留链内部的复制和提交开销，并把独立入口工程化。** 不能把当前状态描述成“CPU 算完一池粒子再上传 GPU”，也不能把一次 completion 接受称为每粒子同步。M2 动态选对/合并及 M3 旧全通道仍未实现，另有算法、wire 和资源前置。

推荐两阶段为 A「M1 有效来源账搬运与热路径传输闭合」和 B「GPU M1 显式体验入口及构建能力收口」。A 不改变 root 次序和接受协议。B 复用 M1 已有沙水效果，新增效果按 GPU 能力版本继续扩展。M1 设备失败先沿 T410 回 architect，不能直接转投 M2 或加大 root 批量来掩盖失败。

## 2. 版本与入口

已核 T421-release 本地签包为 8,556,531B，SHA256 `5450ad3052654de11994e180870e0a6b9e0db7c80d2a6e95aa572003367ae751`，与 `test/reports/t257/T421-release.json` 一致。配套 [逐文件核对与资源算术](analysis-T433-gpu-resident-inventory-v1.json) 记录22件来源 SHA。21件与 `.codex/local-test/t421-release/entry/src/main/` 逐字节相同，唯一差异为根 `cpp/CMakeLists.txt`。隔离来源新增 `target_compile_definitions(grsm1 PRIVATE GRS_PROGRESS_V2_EXPERIMENT)`，仓库常规构建没有此定义。它是有效能力差异，不能忽略。

| 入口/构建 | 实际落点 | 当前边界 |
|---|---|---|
| 常规 app 路由 | `EntryAbility.onCreate` 读取 Want `grsm1`，`onWindowStageCreate` 选 `pages/GpuResidentM1`；`main_pages.json` 登记页面 | M1 是显式 Want 入口。`Index.ets::build` 当前按钮进入 `ClearWaterDemo`，没有默认改为 GPU M1 |
| 常规 native | 根 CMake 的 `grsm1` 编译 `app_bridge.cpp`，定义 `GRS_APP_M1`，链接同 `grs_foundation` | `setProgressV2` 在未定义实验宏时返回 `PROGRESS_V2_UNSUPPORTED`；保留 V1。存在该函数不等于 V2 可用 |
| T421 隔离包 | 同一 M1 源码加 `GRS_PROGRESS_V2_EXPERIMENT` | 具有 V1/V2 冷选、新 run、预约采集和30Hz绘制路径；运行时 `progressV2/nextProgressV2` 初始仍为 false，需显式冷选 V2 |
| 物理 owner | `app_bridge.cpp::App::loop` 创建 AppTransport/Run/Scene，唯一工作线程持有 native window/EGL context | UI 线程只交协议、真实 pulse 和传感器输入，不持有权威池。EGL/GL 对象不得跨线程直接用 |
| 历史 M0 | `GpuResidentM0`、revision1 模块 | 单 H 干场的独立 policy，不能按 M1 reader 或后续 M2 解释 |
| 旧 CPU/明水页面 | `P051DerivedV2Demo`、`ClearWaterDemo` 等旧路线 | 本轮不优化或扩展，不把它们的 F64 结果、库存或系统像素证据嫁接给新整数 run |

## 3. 按阶段的数据所有权与证据

下表路径除页面外均相对 `entry/src/main/cpp/gpu_resident/`。能力状态为源码/已交离线证据，不代表 T421 真机完成。

| 阶段 | 实际符号、数据和所有权 | 传输/同步 | 已交证据与剩余边界 |
|---|---|---|---|
| 触摸、姿态、源协议 | `GpuM1DemoController.down/tickSnapshot/sensor`，`App::offer`、`UiState::Sample/Offer`；CPU 持 command32、FIFO和输入代际 | 每次实际 offer 128B NAPI 原字节拷贝。只算触点坐标、请求量和目标角，未算权威库存 | T228 V4、T407/T409 UI检查。真实触摸成功链仍归 T410；shake 当前存于 UiState 诊断，不能当已实现摇晃侵蚀/再悬浮 |
| 时间配额与规划 | `ProgressClockV2::Pulse/Claim`、`Run::AdvancePlanV2/Pump`；CPU 持真实回调域、carry、pending≤4、源 FIFO≤16及 root nonce | 小型控制面，无整池回传。每计划至多 G1+F4，执行顺序仍由 CPU owner 排程 | T404/T412整数配额及 T416账。规划不是旧 F64 preview/λ/seed 全链，后者属于 M3 未实现范围 |
| SOURCE | `SourceCoreTransport::Submit`、`FullSourceKernel`、`source_core.comp`、`full_source_audit.glsl`；GPU 从接受 bank 写另一 bank，command/dedup/sourceEvent留 GPU | command128B+uniform，8 dispatch/9 barrier/一个尾 fence；无每 birth 回传 | T240 source 闭环、T280全 M1 SOURCE。SAND_H、DIRECT_W、FLIGHT_J 已实现，含真实拒量、去重及末ID；设备双源链未获证 |
| QUANTUM 物理 | `QuantumCandidate::BuildPassTable/RunPassRange`，`quantum_stages.glsl` 的 waterPropose/waterPrefix/edgeApply、prepare、arrivals、finishCandidate，`quantum_motion.glsl::resolvePool/resolvePacket` | current→candidate GPU复制；每步整来源 slab GPU复制，事件前缀也在GPU复制；排序与物理均在GPU | T279候选、T280整步和独立整数回放。水侧扩散、冻结介质、AIR/WET运动、沙LAND、床三色、按(column,y,id)ARRIVE已实现，实际设备速度/呈现仍待证 |
| 审计/重放/封签 | `quantum_replay.glsl::quantumAudit/replayQuantum`，`quantum_commit_main.glsl` pass21–28，SOURCE原spare replay；GPU持独立spare及完整event64 | GPU逐事件重放、逐word核候选后pack publication与completion；不下载事件让CPU判物理 | T280四档独立oracle152/0及污染反例。自检与独立tester是两层证据，不合并为真机PASS |
| 尾包接受 | `FullM1Transport::Poll`、`SourceCoreTransport::Poll`、`SourceRootOwner::InspectCompletion/AcceptCompletion`、`RootIsolation::Commit` | 同root尾 fence零超时poll，signaled后map1024B；短控制事务一次切bank/header/receipt/pub资格 | 身份、stage255/audit1023、来源/总量/GL/tombstone/期限均保留。CPU header是封签摘要镜像，不是完整权威池，不能据它重建context丢失状态 |
| 发布/lease | `SourceRootOwner::Acquire/Release`、`SourceCoreTransport::Retire`、`FullM1Transport::Acquire/ReleasePublication` | 两个GPU publication槽，generation+lease，renderer释放fence已退休才复用；满槽背压 | T280/T281持lease、错generation、late/Stop/失败前缀离线证据。未接受candidate不可Acquire，不能以减少同步为由先画 |
| 场景与呈现 | `m1::Scene::Draw/Token`，`scene.frag`、`water_glyph.*`，app Swap callback | 直接fragment SSBO、粒子VBO视图读取GPU publication。完整FBO→窗口Swap；无池下载/纹理回灌。optics64B为冷配置，几何/少量uniform逐draw提交 | T281真实软件GLES196/0及像素oracle20/0。GPU M1床目前是均匀颜色，未接旧全套分层/颗粒材质；同XComponent不等于目标4所有材质完成。系统呈现仍需设备证据 |
| 诊断与冷档 | `FullM1Transport::Freeze/Capture`、`SourceCoreTransport::CaptureAccepted`；T409 capture/timeline与页面冷文件 | 显式freeze才读整bank+event；token/摘要冷档另计。host Fixture上传仅测试宏可用 | T421预约/冷档自测已有，T422负责独立差分，本卡不重复。冷导出不能进入正常每帧路径或被省略出性能分母 |

M1 固定单H/初始bh=0、integer Q20/T16，新空run、feature3/revision2。湿介质阻力和 LAND 抬水面已有，但侵蚀、悬浮平流、固结/迟滞、旧多相源求解和完整分层物理没有因此实现。source成功、QUANTUM成功、native CURRENT与系统实际呈现分别记账。

## 4. 真实传输、dispatch和内存账

### 4.1 热传输与同步

| 通路 | 当前频率和字节 | 裁定 |
|---|---|---|
| UI→native command | 实际 offer 128B，先过源/代际门再排队；不是每粒子上传 | 保留完整32word及原输入分母，不合并/丢弃offered伪收益 |
| CPU→GPU SOURCE | 每SOURCE root command128B；当前 `r.uploadedBytes=128+12*4+3*8+8*4=232B` 包括逻辑uniform数据 | 232B是逻辑提交量，非驱动总流量或延迟 |
| CPU→GPU QUANTUM | `Enqueue::submittedUniformBytes`、`RunPassRange::uploadedStage33` 实际计uniform；无pool payload上传 | 按本版本真实计数，不拿历史T280常量当所有构建固定值 |
| GPU→CPU completion | 每实际map尾包1024B；每计划最多5root，即最多5120B。拒绝尾也可能已map，撤权晚尾可直接拒而map0 | 是每计划上界，不能称每显示帧5120B；30Hz显示与120Hz配额分别计量 |
| GPU内来源账 | `QuantumCandidate::Enqueue` 每QUANTUM无条件 `glCopyBufferSubData(...,4096*16*4)`，即262144B | 是GPU→GPU，不是CPU往返。优先消除无效后缀复制，真实耗时未知，不能把字节直接换算fps |
| GPU内事件前缀 | 同函数在count>0且buffer不同时复制 `count*64*4`，即256×count B | 保留完整事件/失败前缀。当前已经按有效count，不能重复声称尚待改成有效前缀 |
| GPU内状态/发布 | state/pool及spare、候选恢复/authority copy、publication pack均在GPU | 不能删除回滚/审计所需副本或把它们算成host下载 |
| 场景消费 | GPU pool下载0B、CPU pool上传0B；另有实际几何/opticsuniform、双全帧目标和Swap | 0B只描述池payload，不代表0 CPU/GL调用或0同步 |
| 显式冷capture | 每state `64*(K+Kw)+9216` B，加有效events×256B | 单列冷IO/诊断开销及冻结背压，不能以冷抓全池冒热路径常态 |

`Poll`、publication release退休使用 `glClientWaitSync(...,0,0)`，不是无限忙等；signaled后的map、glFlush和eglSwap仍可能受驱动/排队影响。本轮没有新设备计时，不能归定谁主导。`App::loop` 对可运行的BATCH_ACTIVE不固定sleep8ms，等待flight/retry时为1ms轮询，普通空闲才8ms；不得据旧注释构造“每batch固定8ms”的性能下界。

本实现 QUANTUM pass数为 `35+L(Kpad)+2*L(Kw)`，`L(n)=log2(n)*(log2(n)+1)/2`，四档为113/127/122/136。SOURCE为8pass。它与 T238 早期114/128/123/137设计包络有1pass差别，T280已记录实际实现，后续按实际表和逐pass回执对账。排序的每个k/j项是展开后的独立pass，`ExecuteBatch`每次最多10项并核8ms预算，不能拿头文件“Sort算一个op”的旧注释代替代码。非末批flush、最终fence/flush及staging flush当前均存在；T324/T341及后续合同约束继续适用，本卡不授权删除或改变这些边界。

### 4.2 资源公式核对

下表为从 T421 同源Create/alloc分支复算的稳态buffer payload，非真机allocation/RSS。M1 App始终绑定RootIsolation，额外有一个QUANTUM完整scratch event arena和一个SOURCE scratch event。不能继续拿早期无isolation的T280数作为现App全量。

| K/Kw | state B | 单publication B | QUANTUM dispatch | 无isolation buffer B | App启用isolation buffer B |
|---|---:|---:|---:|---:|---:|
| 192/64 | 25,600 | 26,624 | 113 | 23,707,088 | 34,551,504 |
| 192/128 | 29,696 | 31,744 | 127 | 25,835,728 | 37,728,720 |
| 512/64 | 46,080 | 47,104 | 122 | 55,637,712 | 82,374,608 |
| 512/128 | 50,176 | 52,224 | 136 | 57,766,352 | 85,551,824 |

资源算术的差额为 `(K+1)*256 + E*256`，其中 `E=4*(48K+16Kw+254)+K`。`PrepareColdPrefix(twoBatches)`还临时分配一个完整E×256 eventScratch，不含在稳态数内。双FBO再加至少 `8*width*height`，resize旧新共存、program/JIT、GL对齐、驱动、native/JS ring和冷档另列。分配失败必须明确unsupported/fault，禁止静默降池、删事件或关replay。

## 5. 当前未实现能力和GPU扩展接缝

| 项目 | 当前实际边界 | 下一接缝和真实前置 |
|---|---|---|
| M1 源/落水/侧扩散/湿沙运动/到达 | shader、审核、replay、publication及scene已交离线实现 | 接T410设备SOURCE→ACK→QUANTUM→同token Draw/Swap及负控；不另建重复“GPU实现落水”卡 |
| M2 确定选对/merge/birth | `configureM1`精确接受feature3/revision2，source按free/容量拒绝；当前无M2核。M1中的budget `pair`布尔不是粒子pair算法 | T230 §7已有单workgroup持久循环设计。实施前需M2完整key/predicate、partial-source、weighted RNE、ID/来源/slot generation、事件used-mask、布局/资源/dispatch上界的独立manifest及oracle，不能直接复用旧只读pair探针wire |
| M3 原旧通道/preview/λ/seed | 当前M1不具备旧三候选preview、R8 λ和≤2N seeds、旧悬浮/固结/S3/S4等全链 | T230 §6–7仅有设计方向，需designer逐公式冻结单位/round/时序/容量，再architect逐表达式、branch bank及feature bit工程化。CPU热补算或每seed回host均不接受 |
| 旧完整材质迁移 | M1 scene已含水体/水面/J和单色床/活动沙，不等价旧P1完整材质 | 依据目标4/T408实际系统证据确定所需视觉子集。新增GPU scene adapter直接读同token publication；静态纹理可冷上传，不能下载池供CPU烘焙每帧TData。不在本卡重做T408 |
| 摇晃新增效果 | 传感器和目标角已接，shake值目前诊断 | 强度、沉积/侵蚀/悬浮与时间窗须有新GPU policy设计；接口以冷参数revision+每root冻结小型输入扩展，不把摇晃回调当物理成功 |
| 产品入口/能力 | Want可进入，Index仍旧入口；V2为隔离构建开关 | 阶段B建立显式GPU能力/版本入口。V2产品化时限和启用范围沿T401既有待裁决项，不把实验250ms自动升格生产承诺 |

任何新效果由GPU candidate写入，GPU审核/重放、同一1024B尾接受和publication消费闭环。优先复用source/quantum root，不为每个效果、每粒子或每merge增加CPU接受点。新增字段先升numeric/layout/algorithm/scene与feature能力，冻结资源和失败码后实现，旧revision reader继续拒未知。尚无上述闭包的M2/M3任务属于真实阻塞，不能仅做零碎准备就放行整卡实施。

## 6. 必留正确性与证据边界

- 真实FrameCallback pulse、双时钟间隔和session沿原33,333,333ns GAP撤权；30fps只约束显示目标，不能反过来放宽真实控制门。120Hz整数配额、pending4、FIFO16、generated/accepted/overload/revoked/carry全账保持。模拟秒/墙钟秒/新物理内容帧分别报告。
- V2 root从原JobIntent起250,000,000ns绝对期限不续期，continuation/flush/status/重试不重开root、不改nonce；首错后不产生新接受。Swap前后均保原permit检查，迟到成功Swap不能写CURRENT。现源码按`stamp>deadline`拒绝，本卡不改相等边界。
- 只允许同GL owner和context的accepted bank→candidate→审核/重放→sealed completion→原子host接受。保SOURCE/QUANTUM成功前缀、守恒/容量/Hq/预算/来源/末ID、元数据拒绝和去重语义，不把摘要替代GPU审核。
- source32、event64、completion256、完整epoch/token、lease/release/quarantine均保持。旧context丢失为STATE_LOST，新空run重建，不从摘要恢复。full trace冻结、事件容量满和lease满真实背压。
- 全部背景前景只用XComponent+shader。失败FAULT或明确STALE完整上一有效新管线帧；禁止Canvas、CPU热物理补算或整池下载回灌作为回退。

T280四档完整root、T281软件GLES场景和T421实际App/ANGLE预约属于已交离线证据。T301仅设备路由/部分状态，T405错窗/缺档、T410旧包CAPTURE_BUSY未行使主目标的边界继续保留。T421签包已到件，T410可在设备窗口续测；T422独立差分沿原卡，不新增为开发关闭前置。本轮不运行这些测试，也不把历史报告重标为新包实测。

## 7. 阶段A · M1有效来源账搬运与热传输闭合

2026-11-09，T444 勘误。来源账每条64B，事件前缀每条256B，二者独立。T435 原字节断言按勘误处理。 [T444 计量与测试合同](architecture-T444-gpu-prefix-copy-oracle-erratum-v1.md)

### A-D developer任务范围

不使用真机。以T421来源冻结一个独立版本，首版只选择**每QUANTUM复制有效sourceEvent16前缀**，同时登记实际copy边。不更改物理shader顺序、pass表、tail格式、四binding和flush布局，不并行重做预约。

1. `FullM1Transport::SubmitQuantum`从当前已接受 `SourceRootOwner::Header()[34]`获取有效sourceEvent条数n，checked核n≤4096、字段版本和GPU接受身份一致。新增内部staging参数携带完整身份+n，不把它扩成线上业务endpoint。`QuantumCandidate::Enqueue`复制 `64*n` B，n=0跳过该copy，原barrier保留。每步都重复制本次有效前缀，首版不做跨root增量缓存，避免未接受/换代误复用。n或资格无法证明时走原完整复制并在原切点判错，不增加提前业务拒绝或抢改首错。
2. 保留WORK原容量与旧后缀字节，**不得CPU清零整个256KiB后缀来抵消省量**。核 `inventory/emit/quantumAudit/replayQuantum` 的所有来源访问，索引只能在已接受n内，非法origin仍在原检查切点失败。未知路径维持原完整复制并记原因，不以读越界换优化。header或prefix来源不能由JS/显示文本推导。
3. 记录ledgerCopyBytes、eventPrefixCopyBytes、stateCopyBytes、completionMappedBytes、coldMappedBytes和相应root身份，使用有界计数/冷出口，不每粒子新增日志/时钟。已有cold schema仅以显式revision扩展，旧消费器拒未知，不与T409/T422同名字段混义。正常无pool host搬运、无64B standalone Poll被误接为完整接受门。
4. SOURCE/QUANTUM拒绝、Stop/重置/resize/context loss、同sourceCommit重复quantum均校验真实prefix。保留原完整路线作为同包冷选对照，只能新run选择。不同路由不跨run复用lease、workspace或staging身份。

预计仅在n<4096时减少 `(4096-n)*64` B/QUANTUM GPU复制，n=4096省量0。它是确定的工作削减，不承诺30fps；要减少排序/审计/pass或root接受点必须另修合同和证明，不能附带混入本卡。编译、非机实际GL/同源逻辑差分、签包/验签及复跑README完成后开发卡交付，不等待另卡设备结果。

### A-T tester独立任务范围

离线差分卡不使用真机，消费开发实际实现而非另写产品镜像。独立构造n=0/1/4095/4096，DIRECT_W不占water slot但增加来源账、部分成功、拒绝、replay、SOURCE后连续QUANTUM、停源在途J→W、LAND及失败成功前缀。未复制后缀写非零毒值，非法origin、错n/旧run/重建workspace和capacity/ID耗尽必须精准拒绝，不能因更早错误遮住被测门。比对原accepted state/event/tail/pub原字节、首错和有效前缀，并核copy计数确实减少且host map/上传没有转移增长。

设备收益另卡，打真机标签串行排队，GPU T410当前窗口优先。真实前置为A-D签包和可运行采集通道，不拿离线差分另加开发关卡。对照同包新run冷选、相同K/Kw/seed/请求/姿态/温度、相同实际成功源prefix与Q，分别n小/接近满；记录CPU submit/flush/poll/map/swap墙时、实际dispatch/byte、accepted quantum/s、新物理内容fps、输入延迟和系统呈现。无支持的GPU计时就标NOT_MEASURED，不用copy bytes推算耗时。30fps与物理速度不足分别报告。

## 8. 阶段B · GPU M1显式体验入口与版本能力收口

2026-11-10，T451 核实 T448 regular 原档已有 V1 READY 提交后的 CLOCK_GAP。experiment 原档的实际时钟仍为 V1。新包按 T451 合同取窗口交接、READY 和首个运行切点的同代原件。 [T451 首次归因与取证合同](architecture-T451-gpu-experience-state-lost-v1.md)

### B-D developer任务范围和前置

不使用真机。消费已冻结M1配置/控制/scene接口，建立可从普通app界面进入的**显式GPU沙水体验入口**，直接进入同一M1页面与native owner。沿同一个新空run做选沙/选水、触摸落下、UP停源而已成功J继续落水、姿态/手动互斥、重置和前后台。复用已实现M1效果，不移植CPU F64 facade、不改T408页面准备、不顺便增加M2/M3。

1. 构建能力需可核验。提供冷capability revision、build profile、是否支持V2、K/Kw/features/scene policy；普通构建返回PROGRESS_V2_UNSUPPORTED时页面显示明确能力结果，不伪选中V2。源码相同但宏不同的HAP分别记录SHA和支持范围。
2. 把实验定义放在独立命名的构建profile/option及清晰的README，不通过修改公共CMake无条件启用。实验V2仍需冷选新run；V1/V2不在同run切换，不把120Hz物理和30Hz显示绑定为同频。
3. 从旧入口进入GPU体验必须显式新空run，不能导入CPU/M0 snapshot，不跨context拿GLuint。传感器无效、source未READY、配置unsupported、内存不足、旧token都保具体拒因。控件/计数来自同snapshot身份，release和故障状态仍沿现合同。
4. 新效果预留接缝只登记feature/numeric/scene revision和unsupported结果，不声称新物理已支持。若后续确需M2满池合并，先消费第5节M2正式manifest和独立oracle，再另实施GPU常驻loop；当前不热调用CPU pair。

B-D的入口与构建接线在M1接口冻结后可离线实施，不必等待A的性能收益，也不以T410真机完成作为开发关闭前置。与A共享的native接口集成需串行。**生产默认路由切换与实验V2产品化**仍需要T410同窗非零接受/真实呈现证据，以及T401原有生产时限/启用范围裁定；这两个真实前置未齐时只交付显式实验入口，不暗改默认。它是既有产品语义待裁定项，不新增manager验收门。

### B-T tester/effector任务范围

tester离线先核实际capability与构建宏、新run隔离、unsupported/旧代/前后台/错误恢复，CPU路径不被调用。设备卡打真机标签串行，消费B-D新签包，复用T410和T411/T412/T416工具做普通界面真实触摸双源、停止J继续落、LAND/ARRIVE库存同token、真实姿态/手动互斥、不同profile资源及负控。T410已交部分只按对应包/窗口复用，不复制整卡或把旧原件冒新包。

effector另以真机和真实操作者条件验证连续掉沙/注水、侧扩散、水质感、倾角响应与停源体验。报告当前GPU M1单H材质范围；旧完整材质/摇晃效果未实现时具体登记，不用平滑界面或重复旧帧代替有效内容。输出功能、30fps新内容、模拟时间速率、系统显示和主观体验各项结论；失败先交architect。

## 9. 共享区、调度与执行约束

| 区域 | 独占/可并行关系 |
|---|---|
| `app_bridge.cpp`、`evidence_capture_*.inc`、`GpuResidentM1.ets`、`GpuM1DemoController.ets` | T421/T422实际生产路径/消费还在使用。T433不改。后续A/B需要修改时由单一集成owner冻结来源后串行，禁止同时替换公共hunk |
| `m1/full_transport.*`、`quantum_candidate.*`、source_owner/quantum_owner、shared wire | A-D唯一owner，B只消费接口。root接受/lease/wire改动必须回architect，不由另一路补丁并行覆盖 |
| `m1/scene.*`、shader、fullframe_targets、全局CMake、EntryAbility/Index/main_pages | B集成owner，不能与A/设备签包重建交叉覆盖来源；静态隔离模块可先准备，正式集成串行 |
| 独立oracle、判读器、文档 | 可不同文件并行，独立测试不得改产品实现以凑通过。T422现有独立差分继续原卡，不并入A/B重新验一次 |
| CPU相关实现/证明/补测 | 按用户停止投入，不新增CPU工作或自动恢复已停T429–T431/T227 |

所有后续执行task prompt必须附带以下要求。要么完成本卡当前约定范围及本角色必要检查，要么遇到确实无法继续的具体问题才停止；不能做一点就停下，局部分支受阻先完成其它可做项。不允许修改 `D:\hmprojects\HelloWorld\docs\后续工作清单.md`，仅本人行状态为有限例外，不修改其它行列/队列或滴答同名清单。接单原卡评论已开始、本人行进行中；成功交付原卡回执、本人行结束待验收；失败/FAIL/BLOCKED/关键证据不足原卡写未完成项、复现/原始错误、证据、已试办法、不能自行继续原因、所需前置和接手角色，本人行看评论。共享树不回退他人。非设备卡明确不使用真机，设备卡必须真机标签并串行。全部前景背景仅XComponent+shader，禁止Canvas上屏/回退，失败FAULT或明确STALE完整旧有效帧。

本稿只提供范围和真实依赖，不创建任务、不调整队列、不提前切默认或判产品PASS，由manager按现有登记职责派工。T410续测和T422独立差分继续最高优先，A的离线工程不抢当前设备窗口。M1设备收益出现具体瓶颈后再决定是否投资更复杂的pass合并、GPU间接dispatch或M2索引。

## 10. 本卡必要检查与交付

已完成原卡、当前入口/生产与隔离宏、22文件来源、签包SHA、GPU写者/host接受/lease、热冷传输、实际pass表、四档资源公式和已交证据边界核对。四档无isolation公式与T280原报告逐项吻合，isolation及cold transient按本版alloc分支另算。未编译业务、重跑产品/设备测试或改变任何阈值。后续两阶段给出了developer、独立tester和设备/体验工作范围及真实前置。

本轮自行选择优先删除来源账无效后缀复制、保留逐root接受与flush布局，再收口显式GPU体验入口。未选择尚无同窗证据支撑的“取消completion”“多root一次接受”或默认启用实验V2。吞吐、资源真实峰值、系统呈现与完整效果仍待相应设备任务验证。

## 2026-11-14 T500资源生命周期补充

[T500盘点](architecture-T500-gpu-resident-resource-lifecycle-v1.md)以T492实际M1/M2生成源复核四档对象账，M1 isolation数保持、M2与生成profile一致。当前冷额外E在首次FBO创建前删除，已知活跃峰值按max(B+E,B+8wh)分阶段算；resize先删旧FBO，不把假定旧新共存当已知应用实占。driver退役存储、系统surface、host快照/capture/冷trace/cache副本另账，真实物理峰值仍未测。长序列独立接力，不改旧开发关闭范围或容量。
