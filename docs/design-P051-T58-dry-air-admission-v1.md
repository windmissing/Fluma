# T58：干空容器外源粒子准入与空气触点出生 v1

日期：2026-10-26。角色：designer。任务：`6ade1b05e4b06ee3572f27d2`。路线：`P051_DRY_AIR_SOURCE_V1`。状态：**DESIGN_READY_FOR_T59 / NOT_IMPLEMENTED / NOT_VALIDATED**。不使用真机；只交方案与离线反例，不修改产品、冻结判据或测试代码。

## 1. 唯一推荐

新增**显式干场run模式 + 独立干场外源事务**，复用现有粒子池、提交/回滚框架、四层预算、LAND与同提交发布。旧 `injectGestureSand()` 的湿列检查、NO_WATER及水位求解保持原路径；新DEMO明确调用干场接口。

干场冷启动为真实 `water=heavy=light=particle=0`，初始各数组、源累计、sourceEpoch与独立成功准入计数均为0。触点在沙床上方的空气内时，所有本次出生粒子使用该触点的物理X/Y，不夹到水面、不抖动到列中心；出生只增加真实粒子库存，不能先增加床高。

必须一并接通干场的容量、空气动力学、跨列与审计。只删除NO_WATER或直接调用pool.allocate无法满足此方案，也不属于T59允许实现。

## 2. 现有接口不能直接完成的证据

输入：`docs/developer-T51-empty-sand-demo.md`、T50重力方案与当前源码。

| 现有位置 | 事实与影响 |
|---|---|
| `P051EmptySandDemo.reset()` | 初始化128列×160隐藏水库存；仅沙/源/粒子为0，不能算真干场 |
| `WaterSandField.injectGestureSand()` | 请求列均无自由水时返回NO_WATER；后续η求解和出生Y都依赖水位 |
| `P011ParticleM2FieldBinding.seed()` | 走真实HOT提交，但仍使用旧SPAWN语义；仅换外层Y实参不足以解除内层水域限制 |
| `P011ParticleHot.editPoolHot()` | SPAWN再检查 `[S+contactEps,waterTop−contactEps]`，X重设为列中心加jitter，速度重设为湿场terminalSpeed方向；并非精确空气触点出生 |
| `P011ParticleM2CommitSink` | SPAWN/CROSSING容量按 `waterTop−S−U−P`；无水时余量为0 |
| HOT/WaterSandField审计 | 要求总固体占据不超过waterTop及capacity；只令waterTop一直为0会导致有沙后的审计失败 |
| HOT与冷参考空气分支 | 已有空气运动，但其X只有阻尼、Y固定减9.8×dt，未让空气重力方向随effectiveTilt改变；不能仅靠湿场方向公式宣称干场T50已满足 |

因此没有已存在、可直接供页面调用并完整守恒/计数/发布的干场准入接口。底层allocate、legacy `inject()`（会重设守恒基线）、湿场seed都不能代替新事务。

## 3. 状态定义：零水量与占据包络

沿用现有按列体积/高度单位。定义：

```text
S[c] = bh[c] + bl[c] + mh[c] + ml[c]       // 已沉积沙
U[c] = suspH[c] + suspL[c]                 // 旧列悬沙
P[c] = poolMassH[c] + poolMassL[c]          // 真实在途粒子
O[c] = S[c] + U[c] + P[c]                 // 按列体积占据
W[c] = max(0, waterTop[c] - O[c])          // 现有getWaterVolume定义
```

推荐不新建第二套水/质量账本：干场显式规定 `W[c]=0`，复用waterTop存储退化的**占据体积包络** `waterTop[c]=O[c]`。冷启动O=0所以waterTop也逐列为0；出现沙后waterTop可以非零，但必须与O相等，实际水库存仍严格为0。这不是加入透明水：不允许有任意waterTop高度、自由水量、浮力或水阻力，诊断同时展示W和O，不把ΣwaterTop标成水量。

模式必须显式为 `mediumMode=DRY_AIR_V1`，不得根据“waterTop是否为0”推断。该模式下：

- 容量余量为 `F[c]=C−O[c]`；既有 `O≤waterTop≤C` 审计保留，新增零自由水/包络等式检查。不得用 `min(C,O)`裁剪超量库存。
- 所有粒子在动力学上都为airborne，不再用 `pY≤waterTop` 判断湿润。waterTop=O只是一维容量表示，不能作为存在水的证据。
- 干场v1保持 `U[c]=0`；动态质量只在床S与真实粒子P之间迁移。池满时外源缩量/拒绝，内部扬起保留在床并退款，不降级为不可见列悬沙。
- 水平衡、液体扩散/水中沉降等水专属算子在干场不产生请求；既有颗粒床滑移/分相库存/四层预算与tan30 guard继续执行。水添加/移除、S3含水率/其它不兼容路线返回明确MODE_CONFLICT，不在活run切换介质。

包络必须是每个**候选事务**的派生状态：SPAWN、跨列、LAND、LIFT、MERGE和床输运改变S/U/P后，在该候选审计之前按同一候选数据更新O/waterTop。不能等发布后补写，也不能让planner预演用旧池、真实提交用新池。公共湿场算子和旧水量基线不改义。

现有自然观测 `waterTotalBefore/After`有直接ΣwaterTop的量，不足以证明自由水。干场新增明确的 `freeWaterTotal`、`occupiedVolumeTotal`、mediumRevision证据；保留旧字段并注明其旧定义，不能用它冒充“干场有水”或“水守恒已测”。

## 4. 精确接口与生命周期

新增DEMO专用类型（建议文件 `physics/P051DryParticleAdmission.ets`），不给旧请求添加含糊的allowNoWater开关：

```ts
interface P051DryRunConfig {
  revision: 'P051_DRY_AIR_SOURCE_V1';
  sourceId: number;                          // DEMO固定51；安全uint32正整数
  particleParams: P011ParticleParams;
}
interface P051DryAdmissionRequest {
  revision: 'P051_DRY_AIR_SOURCE_V1';
  sourceId: number;
  sequence: number;                          // 本source单调递增安全整数
  inputTimeMs: number;                       // 单调应用时钟，毫秒
  seed: number;
  x: number; y: number;                      // 列坐标 / 竖直材料坐标，y向上
  requestedH: number; requestedL: number;     // 分相实际体积请求
}
interface P051DryAdmissionReceipt {
  revision: 'P051_DRY_AIR_SOURCE_V1';
  runId: string; sourceId: number; sequence: number;
  requestedH: number; requestedL: number;
  admittedH: number; admittedL: number; lambda: number;
  x: number; y: number;
  rejectedReason: string | null;
  admissionTransactionId: number | null;
  sourceEpoch: number; physicsCommitId: number;
}

WaterSandField.initializeDryParticleDemo(config): DryRunResult;
WaterSandField.injectDryGestureSand(request): P051DryAdmissionReceipt;
```

方法名可同义，字段含义不得换。一次请求为**一个真实触点、可含H/L两相**，v1不支持用单个x/y代表多列铺撒；若未来要多点批量源，另版明确每点坐标，不能擅自按列中心重采样。页面先依据T62确定的同一内容区做触点逆映射，再给本接口；物理接口严格校验坐标，不作触点钳位。

`initializeDryParticleDemo`是明确的新run/reset操作：预检几何/参数/不兼容路线后，成组初始化空水/空沙/空池、干场模式、粒子route及source身份；清除旧队列/速率时钟/滤波输入/已提交dry sequence；生成新run与RESET发布。初始化失败不能留下半个新模式。`setParticleRoute()`本身不把湿场暗改为干场；普通旧initialize/reset调用保持既有湿场默认。切换介质须明确reset，不能遗弃现存粒子。

干场/出生/空气动力版本必须随run manifest、snapshot、HOT frozen input、publication和诊断保存。仍可用现有 `dual-field-particle-v1` 唯一粒子route和schema4布局，新增medium身份而不改旧route字符串。旧快照缺medium时按历史湿场解释；新干场快照medium/源身份缺失或矛盾则拒绝restore，不靠水量猜测。

## 5. 准入数学与原子提交

### 5.1 预检

- 请求版本/sourceId匹配该dry run，sequence和inputTime合法；同source已成功sequence不得再次准入。DEMO单source只需保存最后成功sequence，旧序列/重复返回拒绝或已存在回执，库存与计数不变。
- `requestedH/L`有限且≥0，和Q有限。Q=0返回零准入NO_OP，不推进epoch/count/commit，不改零基线。
- `0≤x<128`、`0≤y≤C`且有限，c=floor(x)。触点须在床上方 `y≥S[c]+contactEps`；被床遮挡/超界返回BIRTH_OCCLUDED/INVALID_POSITION，不把y抬高或夹至水面。容器边框点按域拒绝，空气内合法点保持原坐标。
- 只允许活跃、未冻结、无未结束HOT事务的dry粒子run。不得在busy状态直接写pool。

### 5.2 容量与统一λ

在单个冻结候选快照计算 `F=max(0,C−O[c])`，初值 `λ=min(1,F/Q)`。两相用同一个λ：`aH=λ·requestedH`、`aL=λ·requestedL`。不按颜色先后抢剩余量。

复用既有准入搜索的至多8次/λmin=1e-4边界：试准入前检查 `O+aH+aL≤C`，按massPackage分包所需slot、ID余量、固定请求/事件/lineage容量。可因slot容量不足统一缩小λ；若两相均请求正量却只够一个phase的出生slot，则继续缩量或拒绝，不能只接一种颜色。乘法下溢或非有限值必须明确拒绝，不悄悄丢极小正质量。分包尾量保留现有精确非零尾量语义，不设新的质量截断。

干场外源v1不为了接新源而合并旧粒子；free slot不足时缩量/拒绝。旧湿场和既有独立MERGE算子照常。不存在“直接往床加沙”或“池满降级到susp”的外源兜底。

### 5.3 真实出生与事务线性化

T59在M2绑定层增加批量干场出生入口（如 `seedDryBatch`），复用HOT plan/patch/rollback，**整个H/L请求只begin/finish一次**。不能对每个phase各自结束一笔可见事务后才尝试另一相。

dry SPAWN placement显式为 `DRY_POINT_V1`：每个分包pX=x、pY=y、pVx=pVy=0，phase保留，sourceKind=外源0，唯一pId沿现有allocator产生，pState=1。不调用旧水域low/high限制，不加旧列中心jitter或湿场terminalSpeed；sourceId/seed仍入身份。旧 `WET_COLUMN_V1`出生规则保持。

事务步骤：

1. 冻结field/pool及源边界，保存数组、粒子cache/freeSlots/nextId、HOT日志page/token/backlog、birth事件长度、预算与源相关状态，生成候选请求。
2. dry容量分支按C−O而非水位余量暂存真实粒子。更新该候选占据包络，逐相审计质量增量、零水量、U=0、容量、有限值/ID/状态与ΔS=0；失败完整回滚后统一λ重试。
3. 所有候选通过后，准备T39成功准入record和receipt，并预检sourceEpoch、publication commit、事件容量等所有可失败项。
4. 在无回调的提交尾部成组发布pool/包络、IH/IL、sourceEpoch+1、独立成功record/count+1、lastDrySequence与一个EXTERNAL_ADMISSION publication。H/L两相、多个分包都只算一次成功准入。
5. 源和render提交完成后再export事件/回调。导出失败记证据INCOMPLETE，不把已成功事务返回成失败而引发再次注入；复用T39语义。

任一拒绝/试探/回滚均不得改变成功源统计、nextId/free list、可见publication、预算或本来有效的粒子；成功外源之后另一个物理step失败，只回滚该step，不能撤销已发布的外源成功事务。页面队列如需新增，必须区分dry请求类型，沿用现有有界队列/每子步≤1笔/反突发时钟语义；本DEMO可继续现有T36时钟→每tick一次同步准入，不必另建队列。

## 6. E从0起、预算与持续动力学

### 6.1 分相守恒与空分母

```text
EH = H0 + IH       EL = L0 + IL       E = EH + EL
H = Σ(bh+mh+suspH) + poolH
L = Σ(bl+ml+suspL) + poolL
W = Σmax(0,waterTop−O) = 0
```

本run的H0/L0/W0始终保持初始化的0。第一次成功外源后仅增加IH/IL，不重设initialHeavy/Light，不调用legacy inject重置基线。使用既有绝对质量审计和源累计，零期望相出现非零库存就是异常。

E=0时，守恒判断走绝对误差/实际空库存，比例型endpoint/悬沙/t90输出EMPTY_BASELINE或N/A_EMPTY，不做0/0、不伪造初始库存或声称空场已通过沉降协议。既有getDrift用EPS保持数值有限的计算可保留，但不能据此冒充协议分母存在；E>0后按真实EH/EL/E计算。正式旧endpoint/peak协议不由本设计改义，dry run带独立medium身份，适用性由对应测试协议显式登记。

### 6.2 预算与LAND

- 外源SPAWN的ΔS=0，按既有T1规则不扣场内搬运预算；触摸请求率继续由T36单调时钟控制。增加O是外部库存进入，不是免费的内部搬运。
- CROSSING仍整粒一列相邻移动，local两端与total只扣既有票据规则规定的量；LAND仍先reserve、再tan30 guard、退款未获准部分，按phase只给相应床层加获准量，并减少粒子同量。
- 部分LAND维持state2和精确余量；完整LAND释放粒子。床增量/粒子减量/包络更新在同一候选与同一publication中成立。新干场容量C−O不放宽C、库存、坡度或预算；接收列满仍拒绝跨列/滞留，不瞬移到其它列。
- 床滑移等内部writer、planner shadow与live apply都使用dry容量helper和同一冻结P/U；包络只是相应变化的派生表示，不参与水事务预算。每个writer后更新候选包络再审计，不得仅在最终getTotals里用max掩盖中间越界。

### 6.3 空气与T50方向的最小兼容

当前空气分支忽略横向重力。为使已批准T50在干场仍真实作用于粒子，dry-only动力版本 `DRY_AIR_GRAVITY_V1`应使用与场同一个accepted/effectiveTilt：

```text
vxNext = vx * exp(−0.01*dt) + gAir*sin(thetaAccepted)*dt
vyNext = vy                 − gAir*cos(thetaAccepted)*dt
```

gAir沿用当前空气分支数值9.8，单位是模型材料坐标/秒²，不是把传感器m/s²直接注入模型，也不声称已作真实材料定标。θ=0退化为旧空气候选式，正角向右、负角向左；H/L在空气中共享重力加速度，不偷加重轻分选目标。wet模式（包括wet场里暂时高于水面的粒子）保留旧方程。冷参考与HOT collect均需同版，双collect、preview与live消费同一medium/θ，不增加调用频率或跨角域。

dry LIFT继续使用现有源强度、票据/guard与phase提取顺序，出生Y为床上方接触位置，空气状态不作水位夹紧；池不足时保留原床并退还未使用票据，不走DEGRADE→susp。水专属悬沙列始终为0。T50不允许传感器自动invert180；干场v1也不得调用旧invert把床倒入U。若显式调用旧invert，dry模式在任何写入前报告DRY_INVERT_UNSUPPORTED并保持原状；完整干场倒置再起沙不是本次接口交付内容，旧wet invert继续原语义。

## 7. T59文件/hunk任务书

| 所有权 | 工作内容 |
|---|---|
| 新 `physics/P051DryParticleAdmission.ets` | 请求/回执/模式版本、预检及纯准入计算；不独占第二份质量真值 |
| `WaterSandField.ets` dry-source/medium/hot-host相关hunk | 显式初始化/准入；真实W=0与包络；source/count/E边界；dry容量helper；水算子/不兼容接口门；完整snapshot/reset/restore和同提交发布 |
| `P011ParticleM2Field.ets` | dry medium随冻结buffer传递；同一batch两相出生；不借旧seed伪造低层成功 |
| `P011ParticleHot.ets`、`P011ParticleField.ets`及必要prototype/reference | DRY_POINT placement、dry空气向量、固定缓冲checkpoint/rollback；旧wet默认行为不变 |
| `P011ParticleM2Sink.ets` | dry容量C−O、候选包络更新、池满lift保床/退款；不删旧wet free-water门槛 |
| `P011ParticleM2Projection.ets`及field planner mirror相关hunk | medium/placement/derived envelope身份和preview/live一致性；不能用旧wet预演核对新dry提交 |
| `P011PublishedRenderState.ets`/snapshot源身份 | 加入显式medium/动力版本；保持真实field+pool同commit；旧缺字段默认wet，新dry缺失拒绝 |
| `P051EmptySandDemo.ets` | reset调用dry初始化，无waterRegion160；时钟换算后调用dry point请求、两相真实准入、失败停止与reset不留旧源债 |
| `EmptySandDemo.ets`必要诊断hunk | 打出真实freeWater/medium/源与发布身份；触摸内容区和geometry变更须先与T62交接，复用其逆映射，不覆盖其渲染修复 |

渲染不需要新native材质/布局：包络O≥S保证现有P013FrameAdapter排序校验；T49 basic沙pass不画水。应实测dry publication能通过同提交适配器并无水pass；不能把包络当水域画水雾。T62修尺寸/Float32投影继续独立；T59不顺手修改其guard或投影规则。共享文件先按hunk交接，T47/T62其它修改不得回退。

复杂度与端侧约束：单触点两相预检/容量计算为O(1)，候选分包为O(出生粒数)；完整checkpoint/独立审计沿既有O(cols+nmax+固定日志容量)，统一λ最多8次试探，禁止无界重试。SPAWN/LAND只更新本列包络，跨列更新两列；通过现有分相粒子cache得到P，不在每个writer里反复扫描整个池。重置/restore可O(cols+nmax)。HOT新增mode/placement元数据和必要checkpoint使用冷创建的固定存储，不能按粒子/每子步动态扩容；不增加物理子步或绕开原预算追求性能。本方案不给毫秒耗时/帧率通过承诺，T59回执需披露新缓冲字节和实际自测结果。

实施次序：tester先冻结下列干场反例/RED→T59落最小实现与非真机自测/编译/入库回执→独立客观和同用例设备效果验证。若当前缺独立测试卡，由manager沿既有台账补建；不让developer代签真机效果，也不增加manager验收门。本任务交付的是唯一可实施合同，相关效果仍未验证。

## 8. 必须覆盖的离线反例

本表为designer输入/期望，未在本任务执行；tester实现/冻结客观测试，developer可做相应非真机自测。

| 编号 | 输入/故障 | 必须结果 |
|---|---|---|
| D01 | dry冷启、空tick、reset | W/H/L/P/U、源/count/epoch均0；cold waterTop逐列0；数值无NaN；空分母状态明确 |
| D02 | 同零水状态调用旧湿场injectGestureSand | 仍NO_WATER，所有库存/source/publication不变；dry接口不是旧guard旁路 |
| D03 | 空气触点x=20.25、y=220、C=256，H=.1 | pX=20.25、pY=220，初速0；U与床不增，W保持0；O/包络只增.1；只有一个外源发布 |
| D04 | 同点H=.3/L=.2 | 每相分包和尾量精确保留、出生坐标相同；IH/IL分别累计、epoch/count各+1；一个phase后故障则整笔回滚 |
| D05 | 某列接近C、或池只剩一个slot但H/L均正 | 同一λ或整笔拒绝，不先接某相；无法通过则源/ID/free list/事件page/预算完整恢复 |
| D06 | y在床内、y>C、x=128/负数、NaN/负质量/溢出和 | 明确拒绝，不夹水面/床面、不生粒/加床、无源累计；Q=0是NO_OP |
| D07 | 完全无水，出生后连续step到LAND | W全程0；无wet阻力/湿terminalSpeed；H/L=在途+床，各相守恒；包络在跨列/落床后一致 |
| D08 | 接收列满、local/total/guard不足、部分LAND | 既有预算与guard保持；余量state2/原列滞留，床与粒子同commit；无越容、重复扣total或丢质量 |
| D09 | request/field/pool审计、event/ID、prepare前后的注入故障 | 失败无半笔发布、ghost source或ID消耗；导出sink失败只影响证据完整性，不重做成功准入 |
| D10 | 重复sequence、Up/Cancel/后台后重进、不同帧频与长停顿 | 同请求不重计、不积追赶注入；T36 ns→秒一致，成功量而非请求量入E |
| D11 | E=0、仅H有源、仅L有源、H/L同笔 | 空分母可识别、另一相无假库存；成功后不重设initial基线；callback重复不改count |
| D12 | θ=0/±15°、短时shake及停止 | dry HOT/冷参考同向/同accepted角；0°旧空气退化式保持；shake池满保床退款，不转U；不扩角/改预算 |
| D13 | snapshot/restore/reset、缺medium旧快照、medium篡改 | dry身份/源/计数/包络成组；旧快照仅按wet语义；非法dry还原不部分写状态；不恢复旧raw流为完整证据 |
| D14 | dry请求发给wet run、含水/其它route试图加入dry、dry invert | 模式错误在写入前拒绝；原wet注入/水位、倒置、湿沉降和T39/T40原例维持 |
| D15 | dry出版field+pool→T49/T62渲染适配 | 真实同commit且保持medium；床/粒子无双绘、水量0；Float32拒绘沿独立投影契约，不能修改物理出生点迎合GPU |

质量/预算/容量/坡度容差沿用现有规范，不从本表新设及格线。测试报告区分接口/数值通过与屏幕效果未测；真机任务须打“真机”标签并串行排队，由manager派工。

## 9. 取舍与本轮边界

选择本路线是为了保留原湿场guard与既有渲染/容量不变量，而不是伪装一层不绘制的水。不能采纳隐藏waterTop160、删NO_WATER、仅替换外层出生Y、页面直接写pool或先加床再发视觉粒子的做法。另起完整干沙引擎虽隔离彻底，但会重复预算、分相源、回滚、LAND和发布契约，非本次最小方案。

本稿补齐T50生命周期表里“reset后水为0”的实际实现前提：旧T51候选仍是隐藏水；完成T59并取得对应证据后才能称真干场。T50重力映射不因此自动标定，T52拒绘不因此自动解除，完整G2水中双沙目标也不因dry DEMO通过而达成。

本轮核验范围：T58原卡、T51回执、T50稿、WaterSandField湿准入/水量/预算/审计/发布、M2 binding/sink、HOT出生/空气/审计、冷参考与投影、P013FrameAdapter。未修改业务/用例、未运行测试或使用设备。方案交付完成，T58仅本行状态改为结束待验收并在原卡报告，由manager登记后续。
