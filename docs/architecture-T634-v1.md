# T634：G11 组合 VIS 产品入口与完整消费者闭包

> 2026-11-19消费T639实际阻断的正式修订见`architecture-T647-v1.md`，本稿机器表已登记T647新tail/Expected/父proof接缝。DM默认实际2404接受/真实16–17审核/时间需修，不能以R1强转、抹审核或改尾宽松gate解锁；T639已交ede5891安全部分保留，完整实施按该合同继续。

2026-11-19，architect，不使用真机，不执行GLSL/产品构建或测试。结论：产品接线缺口成立；已有组合transport/512–516消费者代码，但**未被产品owner实例化**，不能说代码全无消费者。现有模块另有静态完整性缺口，开发范围须完整落实T563，不能仅新增按钮后直接使用。

消费T600 v2、T563算法工程/manifest/两dispatch表、T566 1f0cf9c及实际生成器/模块。T600签包334fd90f…的handoff目录仅留产物，未留完整可核源树，故下列源码结论绑定当前/T566源码，不冒已在该包的GL上复现。T600所述SandCanvas属于旧CPU CA，与GPU组合消费者无关，不派该页返修。G9首拒沿T621/T629/T630；组合开发不等待其真机通过。

## 静态缺口清单

| 编号 | 已读源码事实与工程处理 |
|---|---|
| W01 | kernels.cmake只把四cpp编入foundation；无产品page/controller/bridge实例持有VisualTransport/CompositeScene/Cold。新增专属组合owner与研究页，不把父shader直接替换或仅复用旧G9页按钮。 |
| W02 | CompositeCompletion.words只有64U32，Pump却memcpy1024B，静态越界768B。改为256U32并static_assert1024，回包/typed validator同一完整尾。先修再接通，不拿运行崩溃取证当必要前置。 |
| W03 | Common将required audit mask写uRequiredPasses；父源码又用12/2024判断root类型。新增显式uRootKind，pass count和required mask独立；组合生成器消除所有按父总数猜kind处，不改父默认产物。 |
| W04 | CPU在Pump后无条件设置bit18/19；Commit未走完整父typed接受。视觉审核位只能GPU实际audit/glyph成功后写尾，native核全部物理/视觉要求及seal/nonce/代/数量，不手工补PASS。 |
| W05 | DM分支复用g9spf Profile/Config并将physical profile写0。用真实R1/DM类型化facade和各自布局/seed/header/expected contract，禁止R1对象reinterpret/同布局猜DM，组合code不能取代父物理profile。 |
| W06 | visual_stage仅有plan/ID/marker writer函数定义无实际调用；PlanApply只记摘要/快照，不落实marker物质映射和初始化。按T563实际材料writer成功分支产最终份额，消费sealed计划更新marker；digest不代授权。 |
| W07 | RootCopy在非lane0提前return后lane0遇barrier；Audit也有非一致返回，且引用超出for作用域的slot。统一所有lane经过屏障，积累错误后统一分支；字符串被C++编进库不等GLSL已编译。 |
| W08 | 四map布局声明OLD_BED/NEW_BED/OLD_FLOW/NEW_FLOW，但旧flow写第1槽、新bed写第2槽，顺序颠倒；source replay还需从B/BASE取旧图。按正式字节偏移和真实writer两侧取样，不能拿authority新C冒replay旧map。 |
| W09 | Draw调用PreparePrograms(transport.Resources()?0:0)，该函数拒generation0，正常success永为false；token仅尺寸。使用实际非零context/publication generation+联合lease/token，不能只把0改1冒身份。 |
| W10 | glyph producer为oldXY/newXY/kind/blend/ghost/appearance，vertex却把aGlyph0.x当owner、坐标错位；packet/Pstyle/id/gen路径未完整绘制。按唯一glyph8字典、accepted ticks相位和T563两anchor圆盘/遮罩、真实P层重做消费，非wall插值。 |
| W11 | scene直接画FBO0、缺联合scene lease/固定WORK pin和完整packet层；不能证明同publication完整新帧。沿新FullFrameTargets两FBO和同token完整前背景present，成功才更新presentedMapping。 |
| W12 | Cold只存offset元数据，Read只回窗口描述，无原U32数据/SHA/父ROOT/FULL/broker；token压成U64或id缩U32。补真实不可变payload/页/信用生命周期，不能把PAGE元数据称已捕获。 |

这些是工程/未实现域，非算法门放宽、非用例争议。未实际执行部分仍NOT_IMPLEMENTED/NOT_RUN，不能由静态audit宣物理FAIL或产品PASS。

W01还含生成链接线：现kernels.cmake用CMAKE_CURRENT_SOURCE_DIR/../../../test定位脚本，在gpu_resident目录作用域会指向entry/src/test；EXISTS不满足直接跳过、生成失败也仅WARNING，可能消费旧header。改用经核验的repoRoot（或CMAKE_CURRENT_LIST_DIR正确解析）找到真实脚本，依赖父源/VIS源/两JSON/contract全部输入，生成到本次build独立目录；missing/非零exit/输入SHA不匹配必须构建失败，不能拿现存header兜底。链接器可裁掉无引用对象，object已编译更不能证明最终ELF消费者存在。

## 完整产品入口与线程

新增`g11vis/{controller,bridge,parent_adapter,publication,diagnostics}.{h,cpp}`及`pages/GpuResidentG11.ets`、`physics/GpuG11Port/GpuG11Cold.ets`和native声明；保现有模块/生成器路径。专属library/Ability或明确新路由，默认研究OFF，冷选parent=R1/DM、K192/512、V2048/8192。配置、会话、源、调度、scene/cold都由同一个GL owner持有；UI仅排命令和只读结果，不读marker/库存推进、不临时借父App私有GL对象。

NAPI明确接口：`g11ResearchConfigure(parentKind:string,parentRequest:string,visualConfig64:Uint32Array,seedReceipt:string):string`；parentRequest/seedReceipt各≤4096B，由**对应父正式parser/研究配方**验证，非任意JSON默认赋值。视觉config严格T563 64word，SHA取完整LE原件。`g11BeginSession(scopeJSON)`、`g11SetActive(bool,scopeJSON)`、`g11PagePulse(session,seq,uiNs)`、`g11Offer128(words)`、`g11Stop(scopeJSON)`、`g11Status()`沿T546/T550安全/同代语义；宽整数字符串。未知父请求或未完成adapter具名NOT_IMPLEMENTED_PARENT，不能静默用R1填DM。

parent_adapter内部tagged union区分g9spf/g9dm的Config/Permit/ExpectedContract/Seed/资源及生产数学：两边都调用已交真实父实现，视觉仅扩大WORK并交错VIS，不回写q/库存/时间/父参数SHA。真实触摸资格仍沿T626，不因可见marker放行；测试源/原生控制必须另标SYNTHETIC_TEST/NATIVE_CONTROL，不冒REAL_TOUCH。

## 固定调度与双提交

正式两表仍R1 SOURCE19/QUANTUM2543、DM19/2923；source/quantum显式选择，requiredMask分别0xcffff/0xcffff与0xcffff/0xfffff。root BEGIN冻结同一物理BASE和视觉CURRENT，CURRENT只读，COPY一次复制到ACTUAL/REPLAY；role2/4沿各自bank独立累计，发现/拒绝scope不产生视觉副作用。SOURCE两个批次及Q每scope1+4J的BEGIN/APPLY切点、FLOW BEGIN在ORIGIN_RESERVE前、APPLY在PAIR_APPLY后，精确沿T563逐行表，不能仅在末尾添加五个opcode。

map固定偏移0/131072/262144/393216B，对应OLD_BED/NEW_BED/OLD_FLOW/NEW_FLOW。writer生成真实Hamilton最终份额/切取区间/packet id-generation/sealed plan；同role计划消费更新marker与本role独立ID计数，q0不新生。65536×8word计划严格aligned/bounded，旧错误的绝对地址条件和ordinal%258别名不能当容量审计。初始化GPU真实库存配方、池满VIS_DENSITY_LIMITED、noVisibleLayer和phase rebase沿T558/T563，不能随机补点冒通量。

GPU逐word ACTUAL/REPLAY/marker/header/plan范围来源审核→GLYPH bounds/fields审核→父ROOT_SEAL（位18/19已真实写入）→尾fence。native typed验整个Completion256和NativeEvidence，核根kind/nonce/deadline/实际pass/required mask/父及组合SHA/baseVisualGeneration；仅接受后在同一非GL提交锁切(物理bank、视觉bank/generation、accepted tick、publication token)。无物理先提交再等视觉、无CPU强补bit，无失败单切。视觉generation U64超限具名拒绝，不wrap。

联合SceneLease同时pin物理bank、视觉bank和glyph所在WORK。唯一glyph未fence退休前背压新root/cold写，不能后台覆写再画旧token；Stop/晚完成/错误context均不发布。每root一尾fence、普通尾读1024B/四compute SSBO不增，最大表dispatch照旧，250ms/GAP/120Hz/显示30fps不改；compile阶段可做离线GLSL编译，运行/设备资格归tester。

## 新管线呈现与冷原件

glyph8精确为[oldXQ24,oldYQ23,newXQ24,newYQ23,kindFlags,blendQ16,oldPacketMassOr0,appearanceKey]，vertex两uvec4恰取0..3和4..7，不能从坐标位猜owner；相位由accepted物理ticks及marker phaseStart/span算Q16，重复/STALE帧冻结，不将span≤256直接当Q16 alpha。两anchor圆盘、同真实S/F轮廓裁剪、单P ghost与真实P/id/gen/Pstyle attrs0..5沿T563；不补假流线/中间连接线或隐藏未采样真实P。

实际非零context generation传PreparePrograms，完整底材→glyph→P/单ghost→前景渲染到同一FullFrameTargets帧，原SSBO/vertex consumer barrier保留，满足完整token再swap。`presentedMapping`含parent/composite/config/pipeline/visual generation/physics commit/tick/frame/context，并由真实成功swap产生；尺寸字符串不作token。失败FAULT或显式STALE上一有效新管线帧，初始无有效帧则具名等待。

`g11ColdBegin(token,mode)`、`g11ColdPage(id,segment,offset)`、`g11ColdEnd(id)`只捕获联合token已封原件：父ROOT/FULL + 三marker/glyph/计划/control/四map，段字典与T563一致，当前逻辑bank不能丢；使用现有进程broker两slot/单builder/reader、60s不续、真实free后退信用，caller-span流式读/归档/核SHA，不能在Read返回时重新取活GPU而跨帧。raw≤1024U32/页、JSON≤16KiB、所有id字符串，完整文件manifest及哈希闭环。singlePhase witness按T563预约，V2048/8192固定3149824/4722688B分别记GPU+native单份，不造全历史。原资源8档/最大两DM FULL388308992B/最大已知加witness包络641920128B均不增，不用新副本弥补pin设计。

## 接力与验证边界

developer按上表W01–W12及T563完成入口/实际parent adapter/marker与计划/typed事务/scene/真实cold，维护生成链与native/GLSL/reader同一ABI并编译；不能仅提交配置壳或NotImplemented就称整卡完成。T629共用reader和T630实际provider取证并行，G11接线不等待设备先通过。T600/T601等真实非空组合帧再做父ON/OFF逐word、位18/19负控、同token像素与真实效果；编译或生成行数不冒PASS。任何缺真实父接口可做范围继续、具体阻断回architect，不退旧CPU CA或扩大CPU线。

本稿与`architecture-T634-composite-closure-v1.json`补充T563，未改变其算法/阈值/资源表。完整执行prompt约束见`architecture-T633-T636-execution-boundaries-v1.md`。
