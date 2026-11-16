# T524 目标9三态GPU工程闭包 v1

2026-11-16，architect，P4，不使用真机。消费T515 `gpu-spf-depth-averaged-coupling-v1`。独立 `G9_SPF_Q20_V1` 研究route，工程可交完整实现，产品NOT_IMPLEMENTED。参数无默认值，真实触摸/摇晃映射与效果阈值仍待产品裁决。机器合同 `architecture-T524-g9-spf-manifest-v1.json`，逐pass/布局/接口静态核对见 `analysis-T524-T527-engineering-checks-v1.json`。

## 1. 独立管线、模块及依赖

新增 `gpu_resident/g9spf/`，不以V5宏替换M2生成器。版本前缀 `G9SPF`，route/profile/layout/event/path/input/scene/cold各具独立schema和SHA；物理字段revision1仅在G9命名空间，不使用M1/M2/V4/V5 magic或声称wire6。旧endpoint/capability/默认profile均保持。页面研究选择必须新空run，旧床/旧悬沙不隐式导入，水/轻沙/S3/S4/45°/倒立冷拒绝。

| 模块/建议文件 | 必须交付职责 |
|---|---|
| schema_generator、protocol/layout_generated、pass_table | 从本manifest/T515字典生成shader/native/reader同一布局、used masks、表及SHA |
| wide_integer.glsl / numeric_primitives | U32 limb U64/S64/U128/S128、RNE/TZ/定向区间/ceil_sqrt、checked fault |
| state_banks、work_layout、arena_pair | 五state、四EVENT/PATH pair、planned/sealed、定容scope/错误归约 |
| source_kernel、input_gate、command_capsules | INPUT128真位型、两命令seq/capsule、GPU解码、单P birth/seed来源 |
| activation、flight、surface_flux、origin_map、friction_deposit、capture、launch | S↔F↔P全部writer，真实px/py与环境账，不仅三接口 |
| trial_solver、choice_verifier、semantic_replay、completion_gate | 六J试验、第一成功验证、独立拒绝前缀、完整流/状态比较和typed root gate |
| root_owner、run、control_adapter | owner线程独占GL/物理数组；T508/T498控制复用，QUANTUM一次quota、SOURCE零时间 |
| scene、lease、cold_payload、cold_reader/collector | 同token S/F/P绘制、不可变原件、显式cold预算/版本拒绝 |
| G9SpfController/独立研究页、G9Input128Adapter | 完整显式参数/来源种子、原始输入、合成标签、capture/receipt/freeze与冷声明 |

可以复用已交owner/lease/迟到隔离/cache/有界cold/整数思想和T498控制合同；必须替换旧LAND→S、bed三色搬床、V5 LIFT、湿源λ/η、旧120分母轨迹及无动量高度场。G9内S没有邻边搬运writer，唯一贴床载体F，唯一离床载体P。**不需要先等T517/V5**；同卡实现独立G9模块及所需共用控制adapter即可。T522/V4可复跑工具有助复用但不是G9数学或工程开工的强制前置；不复制其未适用物理内核。

真实前置是T515设计、本工程、完整显式研究配置与同源SDK构建环境；开发可用SYNTHETIC_TEST输入，不等产品手感阈值或真机窗口。T520独立预期/T528效果规格另角色推进，不作为developer编译及必要非机自测完成后的关卡。没有外部真实映射证明时返回CALIBRATION_CONFIG_REQUIRED/INPUT_CLOCK_UNVERIFIED，不猜旧shake残差对应力。

## 2. 五bank、state与WORK

CURRENT权威只读；BASE是本根入口完整冻结；CANDIDATE为发现试验及权威重算；SELECTED保首次成功；REPLAY先验拒绝前缀，后重置为已选流重放。五bank各包含header256、SOURCE/IMPULSE两INPUT128胶囊、column32[N]、bedCohort4[N×64]、flowOrigin4[N×64]、packet64[K]、sourceRow16[4096]。不把p或origin藏在host sidecar，free packet证书/未用来源及保留位清0，slot generation仍保正确定义。

| K | state单份B | EVENT64行 | PATH16行 | pair单份B | 原设计常驻B |
|---|---:|---:|---:|---:|---:|
|192|591,872|33,488|49,152|11,718,656|51,006,080|
|512|673,792|54,288|131,072|22,286,336|93,688,960|

state中S/F/m为Q20；F的p为S64 Q40；packet vx/vy为S32 Q20，y2为Q21双坐标。cohort底→顶；flowOrigin按(origin64,sourceId)升序均匀混合，不伪造垂直谱系。sourceRow绑定run来源1..4096，无digest代替命令capsule。所有trial复制全部字段，restTicks/来源/ID/dedup/cumulative work/反冲/birthMicro均回滚，不只复制S/F。

原WORK逐段按T515：face129×32word=16,512B、twoColumn staging32word[N×2]=32,768B、Hamilton32word[N×64]=1,048,576B、两份originU64[4096]=65,536B、双particle indexU32[K]、input512B、六summary64word=1536B、root/stage4096B、completion1024B。Hamilton scratch并非实际state map，不别名仍将被后pass读取的输入。

工程补充 **114,688B定容WORK**：1,536B独立配置512B+quota1024B，512B root control扩展，9,216B列S128能量/反冲暂存，32,768B排序/接触slot/path provisional及slot generation快照，65,536B逐origin双向核对扩展，5,120B stage/replay诊断。原WORK的同名功能实现若能证明安全复用可后续减账，本首版不靠猜测重叠。工程实际基础payload为K192 **51,120,768B**、K512 **93,803,648B**；额外段用途/offset机器表固定，不能在局部std::vector另分而不计。

四arena：TRIAL、SELECTED、PUBLIC0、PUBLIC1，每个单GL对象内EVENT64后PATH16，最后可用字节精确；root/stage descriptor在WORK/既有control。发现失败复用TRIAL但先保summary；选中复制到SELECTED，AUTHORITATIVE用TRIAL，replay将重新计算出的字段与SELECTED/权威比，不再申请第五普通arena。FULL_TRIAL_TRACE显式额外6pair，K19270,311,936B、K512133,718,016B，按J分别标实际有效行，未执行trial标NOT_EXECUTED；启用前预约GPU+冷导出信用。

公开scene直接读被lease锁住的完整CURRENT，不另造库存pub副本。接受交换CURRENT/CANDIDATE及token；旧CURRENT未完成scene fence不得当下一候选/BASE覆盖，需等待退休或BUSY，不偷偷增加第六bank。frame/public arena也按代租约退休；Stop/late/GL错只FAULT或完整上一帧STALE。

## 3. 四binding与有限同步

正常solver：0=BASE或只读冻结前态，1=本scope WRITE state，2=WORK，3=EVENT_PATH pair。每个stage整个单workgroup64lane；全局数据在同组内bounded loops处理，uniform barrier+memoryBarrierBuffer分隔。粒子lane按slot stride64独占写；需排序/Hamilton/top-first/ordered capture时lane0定序或64lane固定归约，所有lane过相同barrier。shared声明上限4096B，只放小归约/控制，完整来源/包/path放已计WORK。禁跨组自旋、禁把L²/K²链除64冒性能证明。

源/面输入始终从该pass冻结字段读，apply不能让另一面读刚更新F。十个微步pass跨dispatch的GL_SHADER_STORAGE_BARRIER_BIT必须保留；粒子并发写后单独fold Psum。邻面q/jp共享同一face记录，质量按donor左→右的remaining-Hamilton，配对动量不强改成q*u，q0仍可压力传力。墙只反冲不穿质量。

COPY_SELECTED阶段可bind0=CANDIDATE、1=SELECTED、2=WORK、3=TRIAL，把state复制；event/path以GL_COPY_READ/WRITE冷/私有copy或同一dispatch内3绑定源、WORK定容小chunk传递，不能同时需要两个arena+两个state加WORK五binding。更直接实现为state和arena分别glCopyBufferSubData（计copy调用/字节与屏障），此非compute dispatch，保持固定pass数量但不能隐去成本。COPY操作检查原permit/同context/不重叠及GL错误。

最终比较分多pass：state SELECTED↔CANDIDATE、REPLAY↔CANDIDATE各用binding0/1，WORK2，第三槽绑定所需事件对象；stream比较以arena0/1、WORK2、BASE3，全部字逐条按mask核，role bits/seal先各自合法后才规范化允许差异。比较结果锁存WORK，不拿hash替代。Scene只绑定当前完整state，fragment/vertex能力另探测，不额外混读另token的F/p。

## 4. 完整pass时序与选择重算

根固定12control pass；六发现scope各 `8+10J`，独立拒绝前缀验证同六scope；AUTHORITATIVE与REPLAY各预排J32最大328槽，实际选中J之外统一GPU no-op，不host读取试验结果跳pass。共2024次，SOURCE独立12，G1+F4为8108；实际执行/no-op/copy各计，不冒成2024个物理步。原250ms root期限自INTENT冻结，batch最多10顶层op/8ms检查；每dispatch/copy/Flush/fence/accept/Swap仍核permit。2024只是有界，绝非设备已满足30fps/250ms。

每scope八控制pass：INIT_COPY；INPUT_AND_CFL；GRAVITY_WORK；TOUCH_ATOMIC；AFTER_TOUCH_CFL；FINAL_MASS_ORIGIN_MOMENTUM；FINAL_ENERGY_CURSORS；SCOPE_SEAL_AND_CHOICE。前五在micro前、后三在micro后，不能移动touch到每micro重复消费。发现scope从BASE开始；触摸/重力账在各试验内可重算，但唯一接受权威root只提交一次。

每micro十pass：

1. ACTIVATE_REQUEST（冻结B/p、drive含dShear、top-band请求/信用）。
2. ACTIVATE_APPLY（S→F零初动量、来源top-first、局部能量）。
3. FLIGHT_PATH（冻结B/解析常加速度、TURN/T16/接触待定，余时/反冲留证）。
4. PARTICLE_FOLD（真实column/Psum、定序contact索引、容量/路径界）。
5. FACE_FLUX_MOBILITY（HR/Rusanov、共同jp、床修正/gx、完整pTrial/静止掩码）。
6. FLOW_ORIGIN_RESERVE（两面总取量/容量、remaining-Hamilton及全部来源可表示）。
7. FLOW_PAIR_APPLY（配对q/jp及环境账，同一冻结face数据）。
8. FRICTION_REST_DEPOSIT（proximal、不反号增能、rest+=32/J、F→S完整预检/提交）。
9. ORDERED_CAPTURE（column/bottomY2/id、最新B整包P→F、精确m*vx、投影能量门）。
10. MICRO_AUDIT_SEAL（fold/来源/容量/嵌入/动量/能量/CFL、共同event/path封前缀）。

touch专属激活/横冲量/launch在TOUCH_ATOMIC里按column升序完整临时预检，正功cap不足回滚本列touch全部效果，负功和激活混合损失不抵后续正功。F→P取最小origin/低free/新ID，q*vx精确扣p，Jy余数unapplied，新包birthMicro禁止当步再运动。capture本micro末，下一micro才入流；不把停止供沙当清速度。

12根control：ROOT_INIT、BASE_FREEZE、CONFIG_INPUT_GATE、TRIAL_SCHEDULE_BEGIN、DISCOVERY_COMMIT_SELECTION（含私有copy）、AUTHORITATIVE_BEGIN、REJECT_PREFIX_BEGIN、REPLAY_BEGIN、COMPARE_SELECTED_AUTHORITATIVE、COMPARE_REPLAY_STREAM、PACK_SCENE_METADATA、ROOT_SEAL。这些是序列锚点，实际schedule按发现→权威→拒绝前缀→replay排序插入。机器表逐索引/role/J/micro展开，不把控制名当能任意换序。

最早可行J由入口CFL算，发现scope低于该J写NOT_ELIGIBLE；其它依次尝试，第一成功后更细槽no-op。独立前缀验证在REPLAY bank重算所有更小**合格**J及首错cut，未合格CFL也必须独立重算该资格拒绝；不得仅比较原summary hash。权威重算用选中J从BASE，保CANDIDATE；前缀验证不覆盖它；最后REPLAY重置BASE消费完整所选stream并按实际微步语义独立生成写入，比较完整state/input consumption/counters/events/path。

## 5. S64/S128原语与数值合同

GPU不要求int64扩展，统一low-first U32 limb：U64/S64两word、U128/S128四word，signed两补码。接口至少 `checkedAdd/Sub/Neg`, `umul64x64`, `smul64x64`, `checkedWideMul`, `udivmod128`, `sdivTZ`, `sdivRNE`, `floor/ceilRational`, `ceilIsqrt64`, `energyInterval`。除0、MIN取负、乘加溢出、窄化必须先检测，不能先F32运算再转整数。signed除法对绝对值做divmod，再按余数×2与分母比较、恰半看quotient奇偶，最后恢复符号；RNE(-1/2)=0、RNE(-3/2)=-2，负零规范0。ceil_sqrt先floor isqrt并核square==n，不能浮点sqrt后猜。

轨迹以片段入口构造完整有理分子，t=τ/(65536*120J)，x/y2/v一次定向舍入，不能逐查点累计误差。最多8PATH/包/micro，16轮lower_bound用[0,65536]闭域、先处理起点已接触/端点，TURN有理点不先取整猜左右；同时命中按T515优先级。碰面精确坐标与自由量化轨迹之差另留反冲/能量，超包络PATH_BOUND为FAULT不尝试隐藏。typed再试原因只含数值/CFL/容量/能量分辨率等正式正常原因，坏来源/溢出/replay/seal为FAULT。

能量p²/F用U128 p²（≤2^116），先divmod再把quotient/remainder各自移24计算定向上下界；禁止先p²<<24溢出。p=0/F=0为0，有p无F拒。相同未改物理项先完整字段比较/符号抵消，no-op ΔE严格0；不能独立floor/ceil后让零变化误报RESOLUTION。所有局部转换及闭合micro分别过门，不借后续摩擦掩盖此前加能。单根瞬时能量/正功按T515域，累计S128仍checked耗尽FAULT。

环境Px/Py重力/触摸/source/支撑墙反冲按事件实际增量累计，不由最后残差补齐。SOURCE携入能量独立；重力角度变更只算固定旧state势能差一次，不瞬时改速度。restTicks单位1/3840s，U64加后饱和到显式settleTicks；trial/replay不外显推进计时。

## 6. INPUT128、去重和控制接受

冷配置必填g、muStart≥muStop、band、entrainRate、shearCoeff、settleTicks、K及来源种子/参数revision/SHA；范围逐字T515。模型值与产品推荐/阈值分开。真实adapter须单独给像素/手势→冲量和capture时域证明，缺值禁IMPULSE或拒启该模式；合成直接给模型J/workCap必须标SYNTHETIC，不冒手持证据。

新G9入口接受INPUT128原U32 words/原始binary64，native只验证协议/时域/身份并冻结receipt/freeze/root nonce，GPU负责物理解码/分配。source/impulse body按T515 words40..127＋alsoCompare索引完整字节及run/input/parameter身份比较；没有参与该kind的字段必须0。各自单调U64 seq，精确重复仅返回原消费回执，不重复发物理；同seq变body BAD_DUPLICATE、较小STALE。BLOCKED/FAULT不消费；已接受但全unapplied touch仍消费，不能池腾空后旧手势复活。

接口合同：

```cpp
Result ConfigureResearch(const G9Config&, const SeedFileIdentity&);
Result OfferInput128(const std::array<uint32_t,128>& raw);
Result BeginRoot(const RootPermit&, RootKind);
Result PumpBounded();
Result PollCompletion(G9Completion256&);
Result CaptureCold(const G9ColdReservation&, Span<uint32_t> callerStorage);
```

SOURCE12pass仅单H P birth/请求与拒量/原始来源/ID/证书/dedup/ledger审计/scene打包/rootseal，不执行摩擦/计时/流动/touch；q0原回执可消费seq但不增加外源质量。原请求≤256非负，floor后≤244666和列净空，非法初始质心不能抬到柱顶蒙混。

G9 completion独立256U32，机器表给root/profile/完整身份、nonce/base/proposed、实际dispatch/barrier/审核16族=0xffff、event/path planned+sealed、J/六trial首错、scope/输入参数SHA、库存/动量/功诊断和seal。U128质量/能量结论由GPU全审核，native核结构/计数/GL和原permit，不能只看总质量0就放行。正式G9 outcome4正常BLOCKED通过**typed adapter**转 `BlockPlanningControl`，绝不把数字4直接当V4 fatal或数字5当blocked；G9 FAULT5另走故障。adapter只能由G9 completion_gate完整核G9独立magic/revision/pass表/16审核族/实际identity后构造中立 `ValidatedBlockedControlRecord`，控制层只接受这个私有构造证明；旧V4 wrapper仍核其自身15审核族与V4表，禁止把G9尾强转成ValidatedBlockedTailV4或省略G9审核。切点锁内再核当前permit/deadline/Fresh/代号，避免校验后撤权竞态。QUANTUM成功才AcceptQuantum一次；SOURCE/去重/q0 metadata不加quota A，错nonce/late尾不可接受。

G9 BLOCKED时间/真实新输入/再入pulse沿T508+T498，不另造时间桶。新source意图成功FIFO入队、有效target改变或显式control mode改变才增加meaningful key；heartbeat/相同sample/旧命令重发不能arm。**首版IMPULSE入队本身不授unlock**，它不是T508已冻结的新SOURCE/target/显式manual↔sensor切换三类事件；BLOCKED时保输入身份和顺序，须由既有合格事件再入后才可能消费，过期照原规则拒绝。若产品需要仅一次冲量唤醒blocked，要designer另扩控制政策，不能native自动改成source意图。Stop/GAP/250ms优先，WAIT不消费输入。

## 7. EVENT/PATH、scene与cold

EVENT64逐kind word表/mask沿T51514kind，seal末写；PATH16同parentOrdinal/micro/slot/tau关联，不跨scope，scope descriptor绑定role/trial/J/branchEpoch/nonce/arena generation。每复合操作先预留全部EVENT和PATH，写body，质量/来源/动量/能量均成功再共同sealed推进；失败只留诊断pending，不冒成功prefix。SOURCE ≤K+16；micro≤5N+2K；root E≤32eµ+(4N+K+16)，PATH≤32×8K，每次emit检查绝不靠“应该够”。无动作stage只写摘要不补造物质事件。

ALGORITHMIC_MAP_REPLAY按完整前态和q重算top-first/Hamilton全部写入，最后逐word比较，不另宣有V5附属片流。EVENT仅role两bits和重新计算的seal可规范化，其它字包括来源/物理量/τ/计时/拒因均比较；AUTHORITATIVE/REPLAY须沿用选中发现trial的trial编号、J和逻辑ordinal，不能因它们预排32个micro槽而改event.word3，再把该差异擅自排除。PATH完整比对且parent/时间有序。summary不是失败trial完整原件，FULL未预约不得声称全试验可离线检阅。

scene同token绑定完整CURRENT：S填0..S，F填S..S+F，P按真实x/y2/质量显示；FLOW与REST F分列，shader速度细节来自真实p/F，不能浓度/贴图另造库存。上下接触面用质心±m/2定义，不恢复旧finalSand上推。XComponent owner/context/window实际尺寸和state lease一致，resize先撤旧资格/退休，新旧FBO峰值分别记；失败FAULT或标STALE上一完整有效新管线帧。

G9 cold独立 `G9_SPF_COLD_V1`，同T503机制两slot/一builder/一reader、60s请求起TTL、不续期、取消/replace pin/信用在分配前。不能套M2或V5 payload解读G9。ROOT档保prefix32KiB+BASE/最终state两份+SELECTED/权威两pair+input/config/quota（小原件已prefix），FULL在此加六trial pair；成功两原件完整比较，摘要档标summary。每slotROOT预算 `32768+2S+2A`，FULL再加6A；两FULL累计max=`2*(32768+2*673792+8*22286336)=359342080B`。逐页≤1024U32/16KiB按段完整行，scope/role/J/nonce/参数、planned/sealed/原错误必保；最多32段，页/UTF8上限按段取整生成。不让冷导出改变物理deadline；信用/时限不足WOULD_BLOCK/INCOMPLETE，不假COMPLETE。新增cold native/GL保留峰值单列，不包含上表普通payload。

## 8. 开发完整范围与必要检查

完整最小范围为独立route全S/F/P、动量/能量/来源、所有双向writer与输入命令、J试验/前缀验证/语义replay、scene/lease/cold、生成器/构建同源与typed控制adapter；缺一项只记局部完成不能停卡。编译/native+ArkTS必要非机自测/签包ELF源码身份齐可交开发，不等待独立真机或效果门。

实际数值helper核signed边界/MIN/half-even/除零/wide乘除/isqrt/interval；schema所有field/mask/保留0、五bank及scratch半开区间、两K资源/2024/12/8108与最大EVENT/PATH。真实离线GPU自测：纯S静止、REST F静止掩码、p反转/停源不清p、S→F混合、P→F投影正功拒、F→S全map、F→P有功/余冲量、双面Hamilton tie、来源64/层满、P嵌入/路径8/容量、选中非首J/拒前缀复算、跨trial时间污染、坏scope/seal/晚尾/Stop/错误后保CURRENT；旧三接口/旧reader应拒G9。T520另算独立期望，不用本replay自证物理。

设备后续带“真机”标签串行核真实shader编译/SSBO/shared/峰值/2024dispatch耗时与250ms、30fps内容帧、系统像素及实际输入。未测记NOT_MEASURED，参数/支持域不足先回architect或designer，不取消审核求实时，不恢复CPU投入。要么完成本卡当前约定范围及本角色必要检查，要么遇到确实无法继续的具体问题才停止；不能做一点就停下，局部受阻先做其它项。不允许修改 `D:\hmprojects\HelloWorld\docs\后续工作清单.md`，仅本人行状态有限例外，不改其它行列/滴答同名清单。前景背景仅XComponent+shader，禁止Canvas/软件GL产品回退。
