# T107：单沙＋明水工程方案 V1

2026-10-27；architect；原卡 `6adf343de4b06ee35743c76a`；目标7/P3，排前六目标之后，目标6 compute最高调度优先。不使用真机，不实施/重跑测试。唯一算法依据为T122 `e1d0ec7`交接及 `algorithm-single-sand-clear-water-v1.md` §3–7，18项CW合同不改。**ENGINEERING_DESIGN_READY，尚未实现/验证；开发实际开始须有T108测试先行基线，不新增manager批准门。**

## 1. 架构选择与现行代码冲突

采用独立 `P011ClearWaterEngine` owner，首版不把新medium塞入WaterSandField旧M2/Hot分支。新模块共享数据类型/纯数值规则，旧对象、运行中队列、ledger与GPU owner完全隔离。理由由现行代码而非文件名推断：

| 现行位置 | 不能直接继承的语义 | 复用/工程安排 |
| --- | --- | --- |
| WaterSandField.getWaterVolume/getTotals/updateSandTop | wet从waterTop减S/U/P推水；dry用占据包络回写waterTop | 新W为权威，eta只RN(S+W)；新owner不调用旧更新链 |
| Hot.beginStepHotBody / auditParticleHot / M2Sink.free | 只接受WET_COLUMN_V1/DRY_AIR_V1；用S+U+P<=waterTop审计，wet LAND/跨列按waterTop计算余量 | 新mode不能用eta冒旧waterTop；单独capacity/guard/sink与audit，旧两个分支不改 |
| Hot.collectParticlePlanInto | 按起点wet决定整段速度、旧列限制；并非T122水面交点分段 | 新粒子分段模块承担新介质合同，不给旧函数换mode字符串冒完成 |
| P011ParticleField.transferHotState / P011ParticleSnapshot | 固定13个池buffer及live/free/nextId，原地copy，canonical alias拒绝 | 可复用独立cold-created池与非alias scratch，禁止旧对象patch/JSON身份热调用；cold setup不可混计零分配 |
| P051TouchSourceAdapter / GestureRateClock | 绑定干沙model；clock允许最多积压1s | 新controller复用坐标/主指针规则，新增kind/inputGeneration与物理采样窗口；不更改旧clock使用者 |
| ScenePreparation→MaterialData→FrameAdapter | 旧剖面/水雾布局，T176已有合法域/错误缺证 | 新packet/preparation严格分离；不借本卡修T176或给旧renderer加clamp |
| scene_layers/compositor | 旧不透明WATER_FOG/INTERFACE_FOG、固定七层与V1/V2布局 | 新clear分支与新packet V3，共用完整scene targets/原子swap与恢复；旧V1/V2及T105 base strip不重释 |

P011ClearWaterEngine在自己的UI physics owner上同步运行，单写者、禁止reentrant；页面渲染只读双publication槽。首版不搬worker，不借GPU原型的pair ABI承担新物理，不改变GPU highest priority或T171生成白名单。后续compute迁移需独立任务与收益/设备证据。

首版保持单一SAND_1=phase0，bl/ml/suspH/suspL恒0，无U回退，无第二沙种；配置开启未实现writer必须UNSUPPORTED_FEATURE，不能静默跳过。床为bh/mh，LAND入mh。可用现有P011ParticleField作为**独立canonical存储owner**，新事务只用其定容转存/验证，不调用旧M2Sink或旧medium audit。池满首版按算法允许的资源拒绝，不要求实现merge；若另版开启merge，必须复用确定选对/原merge F64/lineage及全事务证明，不能顺手加旧全扫描或隐形悬沙。已有其它路线T123/GPU不回退。

## 2. 版本、状态与模块所有权

| 合同 | V1值 |
| --- | --- |
| mediumMode / routeId | SINGLE_SAND_CLEAR_WATER_V1 / single-sand-clear-water-v1 |
| inventory / hydraulic | CLEAR_WATER_INVENTORY_V1 / CLEAR_WATER_HEAD_RELAX_V1 |
| particleMedium / legalState | CLEAR_WATER_SEGMENTED_PARTICLE_V1 / CW_EXISTING_FROZEN_GUARD_POLICY_V1 |
| source / input | SAND_WATER_SOURCE_V1 / CLEAR_WATER_TOUCH_V1 |
| publication / scene / optics | CLEAR_WATER_PUBLICATION_V1 / CLEAR_WATER_SCENE_V1 / CLEAR_WATER_OPTICS_V1 |
| native ABI / display profile | P013_SCENE_PACKET_V3_CLEAR_WATER / CW_RAW_COLUMN_PROFILE_V1 |

legalState明确沿适用的既有H/guard/预算与RN审核，不自称P011_STRICT_REPRESENTABLE_STATE_V1已获证；T104/T110/T135未支持域不因本新mode自动获得严格证书。若某所需操作只能靠未证明严格核完成，报告具体UNSUPPORTED与设计前置，不绕检查。

新文件建议：physics `P011ClearWaterTypes/State/Source/Hydraulics/Particles/Guard/Engine/Publication.ets`；input `P051ClearWaterSourceController.ets`；render `P013ClearWaterPreparation/Controller/Types.ets`；page `SingleSandClearWater.ets`；native `clear_water_contract.h`、`clear_water_layers.h/.cpp`。共同类型先交付，state/source/hydro/particles各模块所有权随最小任务书冻结，单卡一个owner或按hunk协调。

冷配置 `ClearWaterRunConfig{revision,configRev,seed,N=128,C,particleCapacity,massPackage,kappaW,dragWet,settlingSpeed,rateSand,rateWater,maxSourceVolume,dtWindowPolicy,opticsRev,sigmaWater,tintLinear[3],surfaceGain,capacities}`。C沿当前demo的256、pool主档512，不自动加大上限；N固定128。所有值有限，C/massPackage>0，其它物理速率/强度非负，tint[0,1]，revision/身份为安全整数。κ/速率/湿终端/消光等**候选数值全部PENDING_CALIBRATION**，配置必须显式提供并归档；本稿不给默认值冒已批准效果门槛。运行重配只能停源/清待提交后新config generation，不改变已提交库存；改变N/C/pool布局需新run。

State SoA（F64）为W、bh、mh；S、eta、P、R为派生缓存，顺序固定 `S=(((bh+0)+mh)+0)`、`eta=RN(S+W)`、`R=RN(eta+P)`。P由当前pool真实phase0 live质量按确定顺序归约，非authoritative可自由写库存；重建后与cache校验，不负值钳零。每次接受的shadow边界有finite/W>=0/单沙/cache/R容量检查；RN失败拒绝候选或整个子步回滚，不能补偿非法旧W。

池复用13组storage格式，sourceKind使用独立路线声明的外源沙身份（映射原外源数值kind=1，sourceId为沙源配置，water不生成粒子身份）；新owner/run namespace不与旧池混用。ID正Uint32、nextId与提交序号耗尽均fail closed，不复用已退役ID。full LAND退slot一次，partial保留ID/source；初版没有merge时可用cold定容prev/next活动链维护递增ID处理次序，新生追加、退役摘链O(1)，不每步sort/新数组。若该顺序与已冻结粒子事件合同有冲突，交architect，不私改事件消费顺序。

## 3. 接口、错误域及事务状态机

建议固定out接口（类型字段为本文合同，不暗示已存在实现）：

```text
configureClearWater(config, outRun): Status                         // cold
enqueueSourceInto(request, outQueueReceipt): Status                 // bounded input
commitQueuedSourcesInto(boundaryIdentity, outBatch): Status         // source txn
advanceSubstepInto(dt, acceptedAngle, outStep): Status               // one atomic step
copyPublicationInto(preallocatedSlot): Status                       // post-commit only
stopSourceGeneration(reason, inputGeneration): Status
resetClearWaterInto(config, outRun): Status                         // cold new run
```

Status单独枚举，不重编号旧WaterSandError/Hot码：0 OK；1 INVALID_CONFIG；2 INVALID_IDENTITY；3 INVALID_VALUE；4 STALE_RUN；5 STALE_INPUT_GENERATION；6 STALE_GEOMETRY；7 IDENTITY_CONFLICT；8 QUEUE_FULL；9 DEDUP_FULL；10 EVENT_FULL；11 POOL_FULL；12 CAPACITY_FULL；13 PARTICLE_RESERVED_CAPACITY；14 POSITION_IN_BED；15 NUMERIC_UNREPRESENTABLE；16 AUDIT_FAILED；17 TOKEN_EXHAUSTED；18 REENTRANT；19 UNSUPPORTED_FEATURE；20 PUBLICATION_INVALID；21 INVALID_TIME_WINDOW；22 PARTIAL_CAPACITY（允许部分准入的receipt disposition，admitted>0仍是成功，非failed step）。pending/NO_OP/REPLAY是receipt disposition，不用0体积成功事件装填。

### 3.1 SourceRequest/Receipt及有界去重

Request所有字段：revision、runId/runGeneration、configRev、geometryEpoch、inputGeneration、gestureId、sourceId、requestSeq、kind枚举0=SAND/1=WATER、x/y、requestedVolume、timestamp、timestampUnit(NS/MS)、seed。所有身份整数安全且正（generation/seq从1开始），位置0<=x<128、0<=y<=C且不在床内；有限requested>0。WATER的y只验证触点，入口在floor(x)的自由面，不绘制未模拟的飞行水滴。

幂等key为同run内 `(sourceId,inputGeneration,gestureId,requestSeq,kind)`，payload含config/geometry、raw timestamp/unit、x/y/volume/seed所有原位型。先拒旧run；已有key则相同payload返回冻结原receipt，不重新准入（即使手势已经停止，仍是只读查询旧成功），不同payload拒IDENTITY_CONFLICT且不改原记录；未登记key才验证当前输入/几何代与资源。requestSeq与kindSuccessSeq不同，正成功才增加对应kind的sourceSuccessCount/kindSuccessSeq/I_kind；water与sand分开，零量/拒绝不前进成功身份。

Queue=64行，dedup=4096行/run（资源配置可缩小，扩大需新capacity revision/内存表）；定容SoA/逐项比较，无运行Map/Set或忘旧记录的LRU。队列按enqueueOrdinal顺序消费，同一未提交key合并为一条；所有“可提交事务”先预留dedup/receipt/event/ID空间，耗尽在写库存前拒绝。拒绝可用固定receipt行登记，DEDUP_FULL时不驱逐旧成功；直到显式reset新run才清表。4096限制意味着持续输入可能耗尽，必须真实报告，不承诺无限会话；不能为长时测试偷偷滚动遗忘幂等身份。

Receipt复写request身份，带requested/admitted/rejected、reason/disposition/replay、transactionId?、kindSuccessSeq?、runGeneration/sourceCommit/physicsCommitId、water/sand全局与触列before/after、controlAdmitted与actualInventoryDelta、sourceLedgerRev。拒绝transactionId/kindSuccessSeq=null，不能用0伪合法ID。对RN控制量与实际差分分别记账；若控制量正而所有目标库存位型均未改变，拒NUMERIC_UNREPRESENTABLE且不推进成功身份，不新设epsilon。

**定容字段offset**（每槽8B，F64值或原F64位型，little-endian编码版本CW_SOURCE_ROW_LE_V1）：queue行20槽，0 runGeneration、1 configRev、2 geometryEpoch、3 inputGeneration、4 gestureId、5 sourceId、6 requestSeq、7 kind、8 x、9 y、10 requestedVolume、11 rawTimestamp、12 unit(0ns/1ms)、13 seed、14 enqueueOrdinal、15 queueState、16 receiptIndex（未分配=-1）、17 normalizedTimeMs、18 clockWindowId、19 reserved0。runId/revision字串冷绑定owner，入队先逐字验证，不把旧run字串丢弃后当同run。payload比对按0–13原bits（排除ordinal/状态/索引），不能只比hash或数字===忽略-0。

dedup行56槽：0–19保存原request行；20 status、21 disposition、22 admitted、23 rejected、24 transactionId（0仅内部null编码）、25 kindSuccessSeq（同）、26 sourceCommit、27 physicsCommit、28/29 globalWaterBefore/After、30/31 globalSandBefore/After、32/33 columnWaterBefore/After、34/35 columnSandBefore/After、36 controlAdmitted、37 actualWaterDelta、38 actualSandDelta、39 waterSuccessCount、40 sandSuccessCount、41 I_water、42 I_sand、43 sourceLedgerRev、44 errorStage、45–55 reserved0。回执requested由槽10、身份由原行读取，replay仅输出标志、不修改已冻结原receipt。固定key比较0/3/4/5/6/7（run、输入代、gesture、source、reqSeq、kind），payload其余位型都核；clockWindowId为已验证采样窗口身份，加入冲突检查，不作为成功源序号。layout manifest必须与这些offset一致，新增字段须升版并重算bytes，不截字段。

源事务顺序：校验身份/去重→局部容量与pool资源预检→固定sourceBefore/Work复制→按q写候选→重建S/eta/P/R及分相/水/源账审核→准备receipt/event→一次publish所有authority与成功账/幂等记录/commit→热栈外物化回执。失败恢复源前的W/床/pool/ID/free/cache/预算/source/成功序号/event；请求尝试ordinal或错误槽可消耗但不是成功身份。源已成功后，后续step失败只能回滚step，不撤回该源。

WATER q=min(request,C−S−P−W)，余量<=0拒绝；P预留导致拒绝时reason粒子预留，不能称“液体水已满”。只改触列W，不能先分到全场；内部水流在后续子步发生。SAND q还受原外源包规格、空槽及local capacity约束，按massPackage有界拆包、尾包实际质量；不走legacy initialize/inject、addWater的全局分配或改变初始基准。超RN拆包/有限/容量边界拒绝，不能凭epsilon凑一个正包。相同shadow先算所有birth，全部通过才写canonical；明确部分准入量，rejected不算已入库后丢失。

### 3.2 守恒和冻结阈值映射

初始化/reset仅一次形成 `initialWater=ΣW、initialSand=Σ(bh+mh)+ΣliveMass`。运行期 `expectedWater=initialWater+I_water`、`expectedSand=initialSand+I_sand`；每成功源只增加该kind累计，不重置initial。新owner不混用旧getWaterVolume；可兼容导出 `waterTransactionNet=I_water、sourceInjectedH=I_sand、sourceInjectedL=0` 的**只读映射**，但新source ledger有独立水成功记录，不篡改p011-external-admissions-v1沙协议。

沿WaterSandField.getDrift原式：`abs(actual−expected)/max(expected,1e-9)`，water/sand分别计算；audit保持 `drift>=0.001` 为FAIL。固体预算保持原300/s total及已适用local28/s按dt、双端实际debit/R9 refund一次、H=tan30；水不消耗固体预算。原逐步局部守恒采用 `abs(after−before)<=1e-6` 的现行绝对界，source比较包含真实admitted而不是无源差；本卡不得改为新相对阈值/宽限。旧容量审计1e-7、库存/预算EPS1e-9的原作用域分别保留，不拿它们豁免设计要求的W非负、单沙或cache身份。新mode权威W<0/非有限、U/L非零或R非法直接拒候选，不靠旧getWaterVolume的max吞错；没有申请更宽阈值。

## 4. 子步、水流、粒子及床的工程时序

输入时钟沿当前model `max frame dt=1/30`，一个物理substep<=1/60；page可对一个active tick分成最多两次advanceSubstep，先成功源再逐子步。每次substep独立保存input shadow/commit，失败还原该子步入口，先前已成功源/子步仍有真实commit；返回lastAcceptedCommit+failedAttempt，禁止展示失败candidate。dt=0的advanceSubstep严格无写/不消耗源，source边界独立API不可暗藏于step(0)。

1. freeze本子步实际accepted θ/uTheta（沿既有T50/物理接受政策，不用raw sensor角），source boundary完成后复制stepBefore到Work。
2. Hydraulics在Work固定S/P/θ，对偶数、奇数邻边各一次，substepSerial奇偶决定先后。每边实时取当前W/head/z，严格按T122五项界求q；只访问邻列。shadow成对加减、finite/W>=0/R及实际守恒审核，失败按算法拒pair/整步；不得clamp非法旧W或无限二分找好q。q低于可表示变化时记录NO_EFFECT，不伪传输量。固定S/θ能量旁证按 `ΔE=-q*delta+q²` 与原始before/after测量分列，实数公式不顶替RN库存审核。
3. 冻结此时etaMedium[128]，本子步后续LAND/床变化不回写它。Particles采用T122空气式和湿终端式；wet当W>0且S<y<=etaMedium，CONTACT_WAIT/床接触优先。对原列边界有界分段的候选线段，求本列eta首次交点；空间取交点，按比例分配空气/湿时长，速度用该段时长推进；同列最多一次介质切换，多列继续剩余时间，总事件受L上限。相等时间床优先，边界λ=0/1防零时长无限重入。不得沿旧Hot起点wet判整个dt，不额外湿段再加重力或水流携沙。
4. 跨列先stage旧/新P及R；接收满则保留旧ID/质量在原边界CONTACT_WAIT/等待，不发成功crossing。LAND保留W，只移实际grant的pool→mh；partial留ID/source，full只退slot一次；post床抬eta，下一水流阶段响应。事件/lineage均stage到固定arena，不在失败candidate前外发。
5. 单沙床松弛沿现有guard闭式规则、同accepted θ和固体预算，叠加C−S−W−P接收余量；同一完整patch验证后提交，不先写床后补水。当前单种L/U=0，混合交换/渗流/未实现lift不得暗中启用；若配置需要相应writer而无支持核，拒配置并列真实前置，不声称完整strict route。
6. 在Work按固定递增ID遍历重建P、按CanonicalS重建S/eta/R，核W/库存/source/ID/free-list/phase/source、原预算、所有guard与容量。逐粒临时cache仅供shadow预检，最终用真实pool归约，微小缓存残差不能开永久额外容量；最终不符整步回滚，不能钳零。一次authoritative接受后更新commit、封存publication，不做每粒canonical copy/audit以冒O(K+L)。

复杂度界指新路径：两遍水O(N)、池与cache归约O(K+N)、活动链遍历/有界事件O(K+L)，不继承旧每seed三audit乘数或全粒两两搜索。失败不允许无限重试；源批可逐事务按队列上限计总成本，不能拿单子步界冒整批/frame界。RN反例造成重复失败需architect按真实状态分流；不为性能去掉guard/审核。

## 5. Publication、native V3与光学布局

`P011ClearWaterPublication`有两份固定bank，发布run/state/config/physicsCommit/sourceCommit、publicationKind、θ与其revision、所有新route版本、sourceLedgerKey、S/W/eta/P/bh/mh/R/wet、同一pool快照。写非读bank→核完身份→一次切publishedIndex；render持只读borrow到同步copy结束，异步导出独立冷副本。reset/restore只接受完整同版本snapshot，旧schema没有W一律INCOMPATIBLE_SNAPSHOT，不能从旧waterTop反推库存。部分恢复禁止，context丢失不修改任何物理。

新render controller单独调用 `drawClearWaterSceneV3(frame, profile64, particleProof64, xy32, meta32, optics64)`，不进入旧SCENE_PACKET_V1/V2 parser。frame继承session/surface/context/geometry/continuity/box/DPI/lastGood身份，新增inventory/medium/optics/profile/config/sourceLedger revisions和SHA，绑定一个run/state/physicsCommit/FrameToken。P1旧配置57槽及T105 base-strip字段不重释；新sceneConfig分支独立binding/digest。

**CPU源profile64：Float64Array(128×8)，列主序index=8*c+offset。**

| offset | 值 | native检查 |
| --- | --- | --- |
| 0/1/2 | S / W / eta | 全有限，W>=0，S为提供bh/mh的CanonicalS，eta为相同RN(S+W) |
| 3 | P | >=0，与同publication pool列归约及capacity身份一致 |
| 4/5 | bh / mh | >=0，单沙床；无L/U隐形库存 |
| 6 | R | 固定RN(eta+P)，<=C；原容量审计旁证分列 |
| 7 | wet | 只能0/1，等于W>0；W=0不得以eta>0误画水 |

native验证完整typed视图/offset/length/可分离buffer及有限范围，合法F64才生成GPU专用128×2 RGBA32F：row0 `[S,W,eta,bh]`，row1 `[mh,P,R,wet]`。每分量转换有限、层序不逆；不要求 `f32(S)+f32(W)==f32(eta)`，也不对非法F64做clamp。profile64按原bytes hash，GPU F32 hash另列；GL上传/读取不能把byte hash和数值等价混为一谈。粒子原F64域先校验，再复用P013_COLUMN_PROJECTION的显示副本保护，ID/state/phase原校验不放宽。

particleProof64为Float64Array(8×particleCapacity)，前count行按递增ID，offset0 id、1 mass、2 x64、3 y64、4 vx64、5 vy64、6 sourceId、7 packedTag（phase0/state1或2的原数值tag）；未使用尾区置0并不参加归约。frame带count/capacity和proof raw SHA。native核正唯一ID、有限正mass/运动/原x/y域、合法整数tag/sourceId，并以原x64列域按同顺序归约P；禁止从经过F32投影的x猜质量列。每行xy32/meta32与对应proof的ID/tag/原域/列保持投影关系核对，不能只有位置而没有mass证据就声称验证过P。Native源归约与canonical求和保留binary64逐次RN/禁fast-math和重结合；若平台不支持该算术/字节序合同则拒配置，不能以误差阈值替代身份检查。shader只用既有合法投影副本，proof不写回物理。

V3的光学配置 `Float64Array(8)`：0 sigmaWater；1–3 tintLinear RGB；4 surfaceGain；5 opticsRev；6 profileRev数值码1；7 reserved=0。全有限、sigma/gain>=0、RGB[0,1]、revision合法；任何配置变化新binding/digest，不能复用旧fog-density范围0.5..1。FrameToken同时绑定desired optics和source profile，context重建重上传并恢复相同config，禁止拿新水profile配旧沙目标。

新层序：BACKGROUND→SAND_BED→PARTICLES→CLEAR_WATER→WATER_SURFACE→FOREGROUND。旧WATER_FOG与INTERFACE_FOG在clear分支**不执行**；先床与真实粒子、再水，所有前景背景shader。每列采用原始离散profile（nearest列，128列；不自行平滑η）与相同模型坐标/镜像约定。wet=0或像素y>eta则水alpha0；否则 `path=clamp(eta−max(y,S),0,W)`、T=exp(-sigma*path)、alpha=1−T，输出(alpha*tintLinear,alpha)，GL blend ONE/ONE_MINUS_SRC_ALPHA。这里clamp是**已合法数据的光路公式**，不是修库存；床下按W衰减、水中粒子按到面深度、水上不染。水不重复套沙材质L/ToneMap。

使用现有线性混合+sRGB编码目标契约，水层不在同FBO采其颜色、不额外加反馈FBO；sigma0内部透射恒等，独立水面高光仅W>0。单色SAND_1沿原heavy沙显示身份复用，不生成第二沙库存。颜色参数仍待标定，T105/T121旧色彩证明不等新水层设备PASS。

完整candidate每层成功、真实Present/Swap后才一起commit targets/config lease/LastGood；V3 LastGood需同scene revision、run、context/surface/geometry/尺寸/镜像及配置兼容。失败100类输入、200/300类GL等仍分别保留raw reason，并增加新namespace `CW_PROFILE/CW_CONFIG/CW_TOKEN/CW_LAYER` stage，不更改旧错误定义。拒绝返回FAULT或显式STALE完整旧scene，不能只复用旧水层、只画粒子冒新成功；老V1/V2 LastGood不拿来伪装V3兼容。context/resource恢复失败保持明确状态，不Canvas回退。

## 6. 输入、时钟与页面接线

新page只挂XComponent和同框无绘制输入Stack，提供掉沙/掉水选择与reset；新增导航冷开关/明确页面路由，旧EmptySandDemo/SandCanvas默认不改变。接线hunk为EntryAbility/Main_pages等注册和新页入口，不改旧gesture source revision。

P051ClearWaterSourceController owns主pointer、kind、run/config/geometry/inputGeneration、gestureId/seq与独立clock wrapper。DOWN首采样0；MOVE仅更新位置，不产生第二条时间源；active tick从monotonic同域timestamp归一ns→ms。`inputWindowPolicy=CW_ACTIVE_TICK_DT_V1`：上次有效采样到本次的秒数须>0且<=当前active tick dt（该dt<=1/30），超窗/反向/无效时间suspend并0，本次时间成为下一时基；不沿旧1s cap补倒积压。对dt=0首次tick只建立时基，不能发量。requested=rateKind×acceptedSeconds，再受maxSourceVolume与真实capacity控制，不能静默把丢帧时间计成零后延迟补发。

实现比较统一用同一raw frame timestamp单位的差值，clockWindowId绑定本active tick与其前一个tick；物理dt和source elapsed由同一差值一次归一生成，避免先ns→ms再作差而与物理dt重复舍入比较。若间隔超过1/30或跨暂停，当前不生成源；不把截到1/30后的dt当原跨帧时长继续补量。MS入口先按显式单位绑定窗口，未知单位/不同时间域拒绝，不能靠数值大小猜单位。

切kind/UP/CANCEL/后台/页面退出/surface或geometry失效统一stop屏障：停clock、释放主pointer、增加inputGeneration、废弃未提交旧代queue；切kind必须新DOWN；已成功源保留。reset先stop→清pending→newrun/initial/sourceledger→新publication，旧run迟到请求拒。采集用固定request/out槽，避免MOVE风暴new对象；mode选择只改输入kind，不重置物理/水库存。

真实传感器只经既有T50映射和accepted angle政策，不能用raw姿态直接画水面、不能因目标7改±30或shake强度。新页gravity/effect性能需另测；T177的“真实手持”及T176故障不算本新页的效果通过。

## 7. 内存、资源上界与失败策略

以下为**建议定容配置**的可核下限，非整机RSS承诺；实际build另列alignment/对象/NAPI/driver/栈/GL目标/诊断和旧存活场景资源，不批准提高既有性能上限。

| 新owner载荷 | 主档计量 |
| --- | --- |
| canonical/sourceBefore/stepBefore/Work四bank各7N个F64（W,bh,mh,S,eta,P,R） | 28N×8=28672B，另etaMedium 1024B |
| 双publication profile64各8N个F64 | 16384B |
| 双bridge particleProof64各8K个F64 | K512时65536B，native副本及xy/meta缓冲另列 |
| 池canonical/sourceBefore/stepBefore/Work/双publication共6份 | 数组每份55K+16N；K512/N128时6×30208=181248B，标量/绑定另列 |
| 活动链prev/next六bank（若publication不需链则只四bank） | 最多6×2K×4=24576B，不重复把它算进55K |
| queue64每行20个8B槽 | 10240B，offset按§3.1固定，升版字段需重算 |
| dedup4096每行key+payload+receipt预算56个8B槽 | 1835008B；runId冷统一intern，动态变长文本不进热表 |
| 事件E4096、lineage8192及独立回执/双plan/证据ring | 按实际字段SoA逐数组列bytes，不能以“已定容”省略峰值 |
| GPU新clear profile双staging 128×2×RGBA32F | 8192B；full-frame双FBO沿T56另计，context重建旧/新重叠也计 |

已知上述前七数值小计不包含最后事件等及driver，工程交付必须出完整allocation ledger。dedup足额预留可能成为主要内存负担，低优先目标不能挤掉GPU关键资源；若不满足现有预算，明确容量/会话上限取舍交manager/design，不改幂等记忆规则或声称内存已PASS。所有队列/事件/ID/表满在物理写入前拒绝或候选回滚，无热扩容。双publication与固定shader对象在cold建，运行无每frame Canvas/位图生成。

## 8. 最小developer任务书、实际依赖与CW映射

| 阶段/owner | 文件与交付 | 真正前置及验证边界 |
| --- | --- | --- |
| A developer：库存/源事务 | 新Types/State/Source/Engine基础、independent pool owner/phase0、reset/snapshot/ledger/dedup/queue/数值错误 | 本稿+T122+T108先行基线；不等GPU或T176修复，禁止改旧WaterSandField语义。编译/离线自测交付，不等设备 |
| B developer：耦合 | 新Hydraulics/Particles/Guard及完整step shadow/预算/events，source已成功step失败边界 | A真实字段/txn交付及对应独立无阻断证据；不是仅文件存在。严格核未支持点显式列出，不复用错误M2容量 |
| C developer：新scene与输入页 | ClearWaterPublication、Preparation/Controller/Types、新native clear layers/parser/config，page/controller/导航注册 | packet/types可先离线准备；整卡实际集成等A/B可用publication及T108对应断言，避免把准备当可整卡开工。共享native compositor与T178/GPU任务按hunk版本协调 |
| 独立tester | CW01–18独立用例、每事务原数据、native/GL分支/光学CPU参考/原模式窄回归 | A/B/C各开发实际交付后分段执行，失败先architect；缺功能预测试RED=NOT_IMPLEMENTED，非产品PASS |
| effector真机 | 水面/透床/入水减速/积床、真实加水/低头扩散、切源停止、浅深干满/隔床、手持方向与完整scene帧时 | 有效包+独立无阻断证据后按设备队列，不打断在途；GPU前置齐时优先GPU。设备不作为developer关闭门 |

CW逐组落点：01 State/reset无水mask；02–03 Source局部W/容量/P预留；04–06 Particles分段/LAND、同ID/W不消失及后续水流；07–09 Hydraulics五上界/能量/隔床/墙/角度；10跨列与床容量/等待；11–12 Source非法/资源/重复/冲突/分kind账；13–14 Controller时钟/stop/切源/代际；15 Engine source/step不同before及rollback；16 V3 raw profile与光学/遮挡；17 native完整scene/identity/swap/LastGood；18所有模块定容/复杂度/长时身份与性能。每项必须保留T106§8原断言，不用旧dry/雾图/静态API存在冒新路线通过。

独立数值oracle按T122公式及原RN操作图复算，不调用被测hydro/particle计算核生成同一预期；测试端可用精确有理数/独立分段式，不把实数公式证明代替运行RN审计。水源q小于ULP、边界q=0/δ/2、P预留、LANDpartial/full、source成功后stepFAIL、参数/packet错版本、water0但S>0、低透明度仍遮床等反例必须含。原DRIFT_LIMIT/EPS/H/1e-6比较符与T104/T105支持状态逐项核，不更改estimand。

当前T122到件允许本工程与T108测试先行；T108尚未交付不能直接把后续开发卡当有全部前置。T174/T168等P5共用文件在途时，本目标先做独立新文件/文档，不撤销他人改动；待真正合入hunk再给新source manifest。目标7/P3低排期不是实质blocked原因。

所有下游prompt写“不允许修改 `D:\hmprojects\HelloWorld\docs\后续工作清单.md`”（仅本人状态有限例外），不更新滴答同名清单，不回退共享修改；离线不使用真机，设备真机标签和串行前置。前景背景仅XComponent+shader，无Canvas上屏/失败回退；故障FAULT或显式STALE完整上一有效新管线帧。manager据完成通知登记/派工，不新增验收门。

本轮只读T122/现行源码和已有工程合同，输出具体接口/事务/packet/所有权/依赖；没有实现新mode、没有业务测试或设备结论。T107工程方案交付不等目标7产品PASS。
