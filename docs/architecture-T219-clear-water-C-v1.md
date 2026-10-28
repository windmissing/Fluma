# T219：单沙明水 C 段工程接线 V1

2026-10-28，architect；原卡 `6ae19c8ae4b0c3103c0d2244`。依据 T106/T107/T122/T196/T203、T209 开发交付和 T215 独立报告，并核现行 A/B、输入及 P013 native 接口。逐数组、来源哈希及预算在 `analysis-T219-clear-water-C-engineering-manifest-v1.json`。**部分工程交付：publication、源生命周期及已定义的透射层可接线；完整水面高光和真实传感器目标角/摇晃存在两处设计缺口，须交 designer 裁定。C 尚未实现、编译或测试。** 不使用真机，不重跑模型/产品测试。目标7/P3让位于前六目标，GPU compute 调度最高。

## 1. 既成事实与职责边界

单沙明水是独立 `SINGLE_SAND_CLEAR_WATER_V1` owner；A 的权威 W、源事务和 B 的正 dt 子步已经存在。B `advanceSubstepInto(dt, acceptedAngle, out)` 只接受外部给定的 ±30° 已接受角；`attachBInto` 为冷入口。T215 离线独立 12/12 不能推出 C shader、手持或真机通过。`snapshotB()` 是冷深拷贝：A 部分为调用时 canonical，B arena/header 是最后一次成功封存，后续 source 成功后两者可属不同 commit。`restoreBInto` **实际存在但返回 `UNSUPPORTED_FEATURE`**；C 的 context 恢复只能从仍在的当前 owner 重新 capture/reupload，不能宣称 B 快照恢复。

维持 N=128、C=256、K≤512（主档192/512），W 为真实水库存；`S=RN(RN(bh+0)+mh)`、`eta=RN(S+W)`、P 按递增 ID 折回原 x 列、`R=RN(eta+P)≤C`。`W=0` 才是干列，不能由 `eta>0` 推定有水。L/U 继续为零。触点仅给源落入局部有效列/表面，不把 y 当下落水滴。旧 wet、灰绿雾、初始化基准、旧 WaterSandField 与 Canvas 均不得冒充本路线。**所有背景、沙、明水、前景仅在新 XComponent+shader 管线绘制，不允许 Canvas 上屏或失败回退。**

工程边界是只读观察当前权威状态、完整新场景呈现和输入隔离，不改 A/B 算术、源事件次序、事务成功定义或旧 V1/V2 packet。C 失败不得回滚已成功的 source/B commit；反之未完成 C 帧不得宣称当前物理已显示。

## 2. 同一权威边界的 publication

新增 C 专用 `ClearWaterPublicationSlot` 两个 cold 固定 bank，最少含 `profile64[8N]`、`particleProof64[8K]`、`originSourceCommit64[K]`、`waitReason8[K]`、`xy32[2K]`、`meta32[2K]`、`header64[64]`、`ledger64[64]`、`optics64[8]`。具体长度与每份预算见 manifest；只用 count 个证明行，尾部清零。JS/ETS 与 native 各拥有自己的双 packet，native 在任何 GL 操作前复制并验证，因此不存在借用 owner 正在写的数组。所有跨层视图长度、offset、backing、非 alias、finite、容量和 run/config 绑定在冷创建/热入口验证；失效返回具 stage 的 FAULT，不能先改一半 published slot。

给现有 `P011ClearWaterEngine` 增加**只读、预分配**入口 `copyCurrentSceneInto(boundary, out): ClearWaterStatus`。Engine 集成者在同一个 busy 保护区内检查目标数组与五 bank 完全不 alias，再沿 canonical 活动链递增 ID 扫一次并复制 profile、proof、B origin/wait、当前 source ledger 摘要、当前 physics/source commit 和最后封存 B 标识。它不提交源、不 advance、不清计数；O(N+K)，不得每帧调用 `snapshotB()` 复制完整 queue/dedup/event。cold B config、shader 参数由 C controller 独立 immutable copy/revision 绑定，不经私有字段反射。B 未 attach 时 C fail `NOT_IMPLEMENTED_B_COUPLING`，不按静态水画正 dt 完整场景。

capture 次序：固定当前 run/generation/config/geometry/input 版本→等待本 active tick 的源边界与至多两个 B 子步**各自真实提交或失败返回**→在一个 busy/read fence 内复制 current canonical→独立检查/打包/计算原始字节 digest→只切换 ETS publishedIndex 一次。capturing 期间 reset/stop/配置变化必须串行；失败丢 inactive slot，旧 publication 仍只代表旧 commit。成功 source 先于失败 B 的情形，capture **应展示含该成功源的当前 canonical**，并带 `lastBPhysicsCommit < currentPhysicsCommit`；不能把旧 sealed arena/header 当这次新源的 event proof。第二 B 子步失败则保留第一步真实成功的 canonical。`dt=0` 不提交排队源、不推进物理；若页面要处理 source，须有显式 `commitQueuedSourcesInto` 源边界，不借零 dt 触发。

`header64` 中 0–39 槽依 manifest 顺序：owner/run/pubSerial/stateEpoch/configRev/currentPhysics/currentSource/kind/inputGen/geometry/lastBstep/lastBphysics/lastBsource/sealedArena/hasSealedB/acceptedAngle/angleValid/angleRev/count/K/N/C/sourceLedgerRev/水沙成功计数/I水/I沙/初始水沙/失败attempt-stage-reason/布局码/绑定码/opticsRev/captureStatus/BConfigRev/inputPolicyRev；40–63 必须零。所有整数槽须安全整数及精确域，布尔仅0/1；失败三元组是**最近一次实际 B 失败诊断**，不能把失败当本次 publication 的成功 step。`hasSealedB=0` 时 B 步、arena 和 acceptedAngle 仅按明确空值语义报告，不伪造事件证明。最新 angle 只来自成功 B 接受的输入；source-only publication 可沿用最后成功角并标明 angle 所属 B commit，若从未成功则 `angleValid=0`。`stateEpoch` 绑定 runGeneration，publicationSerial 独立单调；reset 新 run 不复用旧 token。

`ledger64` 是 C 帧摘要而非完整队列拷贝。逐槽：0–4 为 runGeneration/configRev/currentSourceCommit/currentPhysicsCommit/sourceLedgerRev；5–10 为 waterSuccessCount/sandSuccessCount/I_water/I_sand/initialWater/initialSand；11–14 为当前 canonical 按列序 RN 折出的 totalW/totalS/totalP/totalR；15=`hasTerminalReceipt`；16–20 为最后已处理终态 receipt 的 sourceCommit/physicsCommit/transactionId（无则0）/kindSuccessSeq（无则0）/coarse status；21–26 为 sealed B 的 sourceCommit/physicsCommit/arenaSerial/stepSerial/hasSealed/acceptedAngle（无效角以0配合 header.angleValid=0）；27–63 置零。最后终态 receipt 只取**本 run 当前 source boundary 实际处理**且保留的结果；reset 后旧 `lastReceipt` 不能沿用，拿不到可靠值时令15=0、16–20全零。完整 source 请求身份与 payload 仍须经§3的精确 reader 获取，不能由这五槽猜出。runId/configDigest 留冷 immutable frame binding，不截断成不唯一 numeric hash。原成功账是 authority；摘要不是凭空重算的第二账。native 只核与 frame/header/profile 可核的部分，其余 ledger 用 `sourceLedgerRev` 和同 owner 事务 proof 绑定，不冒称单凭摘要已验证整张 dedup 表。

`FrameToken` 是显式结构，包含 sceneRevision=C1、runId/runGeneration、ownerId、currentPhysicsCommit、currentSourceCommit、publicationSerial、B sealed identity、session/context/surface/geometry/尺寸/DPI/镜像/box、config/optics/profile/projection/scene bindings 及 frameId。序列化/比较按版本化字段逐项，不能只 hash 后不比原字段；同一 token 的所有 layer 只读同一 packet。desired token 与实际 rendered token 分开，STALE 必须回报旧 token。冷启动/配置改动/尺寸变化不得跨不兼容 LastGood 复用。

## 3. 源触摸和成功回执

新独立空容器页面只注册 XComponent 与不绘制的输入 UI，选择 `SAND=0/WATER=1`；页面、controller、frame clock 与旧 `P051EmptySandDemo`/旧 gesture adapter 分离。一次只有一个主 pointer。DOWN 分配新 gesture/input 身份且首采样0；MOVE 只更新坐标；每个 active tick 最多一个源采样，UP/CANCEL/切 kind/后台/退出/geometry或surface失效统一 `stopSourceGeneration`，递增 inputGeneration、释放 pointer、清旧代未提交 queue 和时钟；切 kind 必须新 DOWN。已提交源绝不清除。reset 为先 stop，再明确新 run 空库存/新 ledger/新 publication；不能以 `resetClearWaterInto(initial)` 在运行时模拟掉水。

使用同一单调 raw clock 的 `previous/current/unit NS|MS`，**先在原单位作差一次**再换算秒；active dt>0 且≤1/30，至多两个≤1/60 B 子步。首 tick、跨暂停/超窗、倒退、NaN、单位/域改变一律停止本代并本次0，不保留 backlog，也不将截短 dt 后补量。`bindSourceClockWindow` 要求与上一 raw current 连续；gap 后调用 stop 开新代和新 DOWN，不用伪 previous 续链。requested volume=显式 rateKind×该真实秒数，再由 maxSourceVolume、列容量和真实事务裁决；不得改旧 rate/physics 规则。各请求先 `enqueueSourceInto` 获 PENDING ACK，再显式 source boundary `commitQueuedSourcesInto`，ACK/admitted=0 **不是成功**。

当前 Engine 没有可读的逐请求终态 receipt API；不可用二次 enqueue 充当通用查询（不存在或冲突的请求会产生新队列副作用）。在 Source/Engine 公共 hunk 加 `copySourceReceiptInto(exactIdentity, exactPayload, out)`，只查现有 dedup 终态，既不写 queue/ordinal/ledger，也不调用 commit。身份沿现存 generation/input/gesture/source/seq/kind 与原始 payload bits，含 clock/geometry/config/time；相同 key 不同 payload 返回既有冲突语义；未终态返回独立 `NOT_COMMITTED` 查询结果，不能伪造 A coarse status。终态 ACCEPTED/PARTIAL 的 `transactionId/kindSuccessSeq/sourceCommit/physicsCommit/admitted` 才给 UI 一次成功反馈；REJECTED 与 no-effect 不给成功。查询重放同终态只更新显示，不重复计量/动效。若 batch≤64，预分配 64×56 个 F64 回执槽并按实际 processed 数填，超界显式拒绝；不复制 4096 行 dedup 热表。证据包含本次真实 source commit 及本 FrameToken；UI 不能以 ACK 或目标水层像素推断源成功。

## 4. 已确定的 V3 packet、沙水 shader 与错误

独立 `drawClearWaterSceneV3(frame, profile64, particleProof64, xy32, meta32, optics64)`，旧 V1/V2 parser/config57/水雾 slot 完全不复用。`frame` 是版本化结构，额外承载 `header64/ledger64/origin64/wait8` 的受检 typed views 及 token/config binding；六参数签名不变，native 的 C parser 显式校验这些字段，再复制到 native 所有 packet bank。不把新字段塞进旧 frame 的未定义位置。origin/wait 是 B 当前 canonical 对应粒子的证据附带；native 可校其形状/对齐/合法域，若 shader 不使用，不能据此宣称完整历史 replay。

`profile64[8*c+0..7]=[S,W,eta,P,bh,mh,R,wet]`；`particleProof64[8*i+0..7]=[id,mass,x64,y64,vx64,vy64,sourceId,packedTag]`，活行严格递增 ID，tag=`phase|(state<<8)`、phase0/state1或2，meta 两槽为 `[tag,id]`。native 按**原 x64** 的列和相同 ID 顺序重折 P、逐列核 RN 身份/容量/边界，验证 F64、输入 revision、无 alias/脱离、证明尾零。禁用 fast-math/reassociation；F32 上传后只核有限/次序/投影身份，**不**要求 F32(S)+F32(W)=F32(eta)，也不能用 F32 x 猜质量列。xy32 使用既有 `P013_COLUMN_PROJECTION` 精确等价规则：若 round-to-F32 跨原列上界，则退到前一 F32；y 越容量同理；C 的 bits scratch 预建8B，避免既有 helper 的逐粒临时 view 分配。profile 上传 128×2 RGBA32F：row0 S/W/eta/bh，row1 mh/P/R/wet；列 nearest，禁止视觉平滑反写 authority。

V3 optics 的已定槽仅 `[sigmaWater,tintLinearR,G,B,surfaceGain,opticsRev,profileRev=1,reserved=0]`。`sigma≥0`、tint各[0,1]，全部 finite；颜色采用线性空间，`GL_ONE/GL_ONE_MINUS_SRC_ALPHA` 预乘合成。层序 `BACKGROUND→SAND_BED→PARTICLES→CLEAR_WATER→WATER_SURFACE→FOREGROUND`；旧 `WATER_FOG/INTERFACE_FOG` 在 C 分支绝不执行。床和真实 phase0 SAND_1 粒子共用同一种已批准沙材质/身份，不能凭空加 L 沙或 tracer。明水层：`W=0 || y>eta => alpha=0`，否则 `path=clamp(eta-max(y,S),0,W)`, `T=exp(-sigma*path)`, `alpha=1-T`, 预乘 `(alpha*tintLinear,alpha)`。每像素按真实深度分开水上/水中，床在水下仍通过 W 衰减可见；sigma0 是**内部透射**恒等。水层不读正在写的 full-frame 目标，不增反馈 FBO，不套旧沙 ToneMap；沿现有线性 blend 与 `GL_SRGB8_ALPHA8` 目标/sRGB 编码实测合同。`WATER_SURFACE` 的具体覆盖和增益公式尚未定义，见§7；未裁定前不能把旧固定 2.5px highlight 当 C 正式实现。

整 scene candidate 包括背景/沙/水/前景、native verify、绘制、present/swap。仅成功 swap 后共同提交 target/config lease/LastGood 与 renderedToken；任一步失败保持完整旧 scene，返回 FAULT 或带**旧 renderedToken** 的 STALE，不能用当前沙配旧水或反向拼接。LastGood 要同时兼容新 C revision、run、context/surface、geometry、尺寸/镜像、config/optics/projection；旧 V1/V2 LastGood 不可充 C。context loss 冷重建 shader/FBO/typed上传后从现有 owner capture 同一 accepted publication；若 owner 不在或重建失败明确 FAULT/STALE，不调用不支持的 restoreB。新增 C 错误 stage `CW_CAPTURE/CW_PROFILE/CW_PROOF/CW_SOURCE/CW_TOKEN/CW_CONFIG/CW_LAYER/CW_PRESENT` 可用独立 namespace；不重编号 A `ClearWaterStatus` 0..22 或 P013 V1/V2 错误。所有失败保留 raw status、首次 stage、requested/rendered token 和是否实际 swap。

## 5. 资源、工作与文件所有权

manifest 逐数组列 26 个 C 新定容模板。双 ETS packet 分别 52,736B(K192)/109,696B(K512)，双 native packet 同量；native校验P数组、双 F32 profile、双GPU profile纹理、粒子 VBO、controller trace/counters/64回执槽及projection bits 合计另101,640B/111,880B。列明新增载荷为 **207,112B(K192)/331,272B(K512)**；这是模板字节和，不是实测 RSS 或完整峰值。T209 的 A+B 真实 owner 载荷 4,230,656B/6,312,256B 单列，绝不把旧 T107 的示意小计替代实际。双 full-frame `GL_SRGB8_ALPHA8` 目标 payload 为 `8×widthPx×heightPx`，优先复用现有成对目标；若同时存活旧/新场景、context restore 临时双份、纹理/driver、program/VAO、冷配置、NAPI/ETS对象、资产及可选 `snapshotB()`，必须在真实 allocation ledger 另加。容量耗尽先返回具体 stage，不热扩容。

每 tick 工作模型：G 个实际源边界 × A 源事务真实成本 + F≤2 个 B 正 dt 尝试（包含其 candidate/replay/audit）+ 一次 O(N+K) canonical capture + 一次 O(N+K) native F64 proof/投影核与 hash + 上传 `4096+16×liveCount` bytes(profile+xy/meta) + 六层实际像素工作 + present/swap。源 G、排队 Q、B event/count、帧边界/故障、CPU/GPU计时和 fullframe尺寸均需记录；不能把 PENDING ACK、cold export 或隐含队列 drain 从分母抹去。GPU timer 未有真实证据时标 `NOT_MEASURED`，不拿模拟CPU时间充设备收益。GPU compute 共享 compositor/native hunk 由集成 owner 串行协调，优先让位 T211/T213；不因 C 原型改变前六目标生产开关。

建议文件责任：`physics/clearwater/P011ClearWaterEngine.ets` 仅加 owner 内只读 capture，`P011ClearWaterSource.ets` 仅加精确终态 receipt reader，保留 row20/56/24 和旧签名；新增 CTypes/Publication/SourceController/FrameClock/GravityPort；新增 standalone ClearWater 页与导航注册；renderer 新 `clearwater_packet/config/layers`，只在 `napi_bridge.cpp`、CMake、types d.ts、scene compositor 指定 hunk 接 V3 并保持旧分支。共享文件一名集成 owner，开发前按实际 HEAD 重新核 ABI/manifest；不要求由 architect 改业务代码。developer 负责编译与非真机自测，独立 tester/effector 另持 C 的 packet、视觉/输入和设备证据，不加 manager 验收门。

## 6. 离线验证入口与完成语义

实现后开发自测至少覆盖：源 ACK 与真实终态分离、同 key replay 不双计、切源/暂停时钟屏障、first DOWN=0、source 成功后 B 失败 publication 包含成功源、第二子步失败保留第一、`snapshotB` 历史 arena 不冒 current、raw x64 列归约与 F32 跨列投影、W0/浅水/深水透射、沙水/背景前景同 token、配置/GL/swap 故障只 FAULT 或完整 STALE、context 重上传。每项保存原始 token、source receipt 和故障 stage。独立视觉/真机仍由 tester/effector 接续；未实现分支记 `NOT_IMPLEMENTED`，无设备证据不记产品 PASS。水面与真实手持的测试 oracle 必须待§7设计落地，不以工程示例自造通过标准。

## 7. 必须由 designer 裁定的两处缺口

**CW-C-D1 水面高光。** T106 §6仅给“W>0 时独立细高光”，T107 §5给 `surfaceGain`，没有覆盖宽度单位（模型/屏幕像素）、边缘 profile/抗锯齿、浅水与干湿交界/隔床断面规则、linear premult RGB/alpha 和 gain 的合成公式。现有 P013 固定宽度旧 highlight 属另一视觉路线，不能默认为 C。请 designer 交可计算 shader 与 CPU oracle、参数合法域/默认候选、optics layout/revision 变更和 W→0/断连边界例；再由 architect 定版工程、developer 接 WATER_SURFACE。**已定透射层和 publication 不依赖此决策；完整 C 可视产品结果依赖。**

**CW-C-D2 真实目标角与摇晃。** `P051GravityInputAdapter` 输出 ±89° `targetTiltDeg`、shakeInput/magnitude/stale；B 仅消耗调用者提供的 ±30° `acceptedAngle`，BConfig 的 gModel/dragAir 是常量，没有 shake 通道。T107“复用 T50/accepted angle 政策”不足以唯一确定在此独立路线的 target→accepted 映射、滤波/速率、跨 tick/两个子步身份、异常与 stale、B 失败后角是否更新；旧 WaterSandField planner 不能直接当本路线实现。请 designer 明确完整版本化状态机、每 tick/substep 接受时刻与记录字段，并裁定 shake **仅诊断**还是实际驱动物理/床；若驱动，给力/质量守恒关系、B 接口/配置 schema 和独立 oracle。不得自行把目标角 clamp±30、丢 shake 或把原始传感器角画水面后宣称真实手持完成。**显式已接受角的离线夹具与触摸源事务可先开发；手持重力/摇晃完整接线待设计。**

这两项按原卡部分交付回执交 manager 派 designer，不把失败归为 tester 缺数据，也不伪记 T219/C 全完成。设计回稿需带 revision 和具体例，architect 再将对应两个 hunk 定版，developer 继续；不新增 manager 验收前置。
