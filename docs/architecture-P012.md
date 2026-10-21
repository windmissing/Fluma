# P-012 表现层工程架构 v1

日期：2026-10-20。编写：designer（按本次任务产出工程化设计）。技术路线：`appearanceRouteId=p012-render-material-v1`。状态：`ENGINEERING_DESIGN / NOT_IMPLEMENTED / PERF_NOT_VALIDATED`。本文落实P1/P2/P3的工程契约，不代表这些条件已经通过实现验收。

上游：[architecture-P011.md §23.3](architecture-P011.md) 的 `APPROVED_WITH_CONDITIONS`、[algorithm-P012-render-v1.md](algorithm-P012-render-v1.md) §1–11，特别是§10验收约定。P4测量附表由effector并行交付、tester复核；不阻塞本文落盘，缺附表时量化效果验收仍不可签字。本文件不改P-011物理、checker、遥测或测试用例。

## 1. 不变量、交付顺序与模块选型（P1）

两条红线：**只读物理场；外观与物理验收隔离。** renderer不持有WaterSandField/FluidField实例，不调用step/getTotals/inject/invert180，不写snapshot或物理数组；只使用页面交付的快照与显示上下文。128列及既有几何映射保持，物理routeId、快照schema、P011/WS日志均不增加外观字段。外观身份仅存在配置、独立视觉/性能证据manifest中。

交付顺序固定 `E → E+A → E+A+B → +C → +D`；OFF恢复原渲染。第一梯队E/A/B共同满足效果和性能后才签字，C/D未启用须标未验证。hybrid使用独立adapter和 `p012-render-material-v1-hybrid` 证据身份，缺失层/悬浮字段标N/A，不模拟水沙库存。本版先交watersand，hybrid不能继承其通过结论。

| 方案 | 判断 |
|---|---|
| 在SandCanvas.renderWaterSand内内联替换所有材质逻辑 | 不选；现有页面兼有输入、物理step、遥测、生命周期，加入缓存和运动历史难以保证释放与隔离，OFF容易意外改变 |
| **独立P012WaterSandRenderer + 页面薄分发入口** | 选用；页面只控制显示选择/生命周期，renderer封装adapter与缓存；原renderWaterSand保留为OFF/canonical，便于同构建配对 |
| Worker/native/shader独立管线 | 不作为v1前置；当前Canvas2D能力足够，新增跨线程快照/图像所有权成本尚无必要证据 |

拟议新增模块（均为规划路径，本任务不创建源码）：

| 模块 | 唯一职责/所有权 |
|---|---|
| `entry/src/main/ets/render/P012WaterSandRenderer.ets` | 帧绘制编排，拥有下列adapter/cache，对外prepare/render/resetContinuity/resize/dispose |
| `P012WaterSandAdapter.ets` | 独占双缓冲快照及O(128)相库存校验，输出只读帧视图 |
| `P012MaterialMotionCache.ets` | 相净搬运代理、UV双缓冲、可信度与静止冻结；没有物理控制接口 |
| `P012MaterialTextureCache.ets` | tile/atlas/ImageBitmap/Pattern/离屏scratch的唯一拥有者，资源预算与重建代次 |
| `P012RenderTypes.ets` | 显式字段/枚举/接口；不使用any或向物理类型追加字段 |

现有接入点为 [SandCanvas.ets](../entry/src/main/ets/pages/SandCanvas.ets) 中 `ws.copyStateTo(waterSandSnapshot)` 之后、`renderWaterSand(ctx,snapshot)` 调用处。页面新增薄分发：canonical物理证据或OFF时直接调用原方法；P-012档位ready时调用独立renderer。一次帧只选一个完整渲染分支，不能先画旧3px硬沙线再叠新云雾。

原renderWaterSand内部的profile复制/既有平滑、box坐标映射与倒立变换需在P-012几何视图中等价实现，结果只用于显示mask；用于P2库存、守恒与运动代理的始终是**原始未平滑物理值**。不额外平滑、不改S或层厚来达成视觉指标。ctx.save/restore按分支配对，倒立视觉变换只做一次。

## 2. 页面与renderer接口

`render(ctx, snapshot, frameContext)` 同步读取同一主线程内刚完成copyStateTo的完整快照；不跨await持有输入。frameContext是页面私有的显示契约，至少含：

| 字段 | 来源与用途 |
|---|---|
| fieldEpoch、continuityEpoch | 页面在物理实例重建、reset、注入、模式切换、倒立事件等既有处理点递增；只用于断开视觉配对，不写物理 |
| frameSerial、sampleTimeMs、frameDeltaMs | 页面实际采样序号、单调时间及现有帧间隔；不伪造物理substepId |
| inputMode、sourceKind、physicalRouteId、runIdentity | 已有页面身份的只读副本；不能由外观开关改变 |
| boxX/Y/W/H、pixelRatio、visualInverted、geometryEpoch | 现有显示变换；DPI/vp/截图px换算集中定义 |
| appearanceTier、renderSeed、configRevision | 外观独立配置；seed不调用/消耗物理随机源 |
| canonicalEvidence | 由证据采集入口明确指定；为true强制OFF，不根据绿像素checker结果动态选择 |

页面接收生命周期事件时只通知renderer，物理事件仍由原业务执行一次。renderer不注册第二套传感器、不消费输入命令、不生成P011_EVENT。帧上下文对象和只读视图可复用，不逐帧构造大对象。

OFF时不执行P-012 adapter/UV更新、不开P-012绘制pass；从OFF重新启用先重建运动配对，不能跨OFF时段累计平流。若纹理尚未ready，可显示原OFF或已ready的较低档并在独立manifest标 `WARMUP/FALLBACK`；此段不能算申请档位的效果/性能通过样本。稳定测量期间禁止静默降档。

## 3. 双缓冲相库存与只读数据面（P2）

### 3.1 字段与持有者

SandCanvas现有waterSandSnapshot由页面持有，下帧copyStateTo会原地覆盖。adapter不能保存其数组引用充当前帧，更不能将同一引用当“前后两份”。P012WaterSandAdapter在初始化一次分配slot[0]/slot[1]，每slot如下：

| slot字段 | 类型/长度 | 所有权与用途 |
|---|---|---|
| bh, bl, mh, ml, suspH, suspL, sandTopRaw, waterTopRaw | 8个Float64Array[128] | adapter深复制，原始材料/边界；其他模块只读 |
| massH, massL | 2个Float64Array[128] | 同一循环计算bh+mh+suspH、bl+ml+suspL |
| depositedH, depositedL | 2个Float64Array[128] | bh+mh、bl+ml，用于区分沉积profile变化与仅悬浮变化 |
| totalH, totalL, absMassH, absMassL | number | Kahan补偿求和结果与误差尺度 |
| cols, capacity, schemaVersion | number | 必须匹配WaterSandSnapshot schemaVersion=2和128列 |
| fieldEpoch, continuityEpoch, geometryEpoch, frameSerial, sampleTimeMs, inputMode, sourceKind, runIdentity | 显式标量/身份 | 前后配对与生命周期判别 |
| valid, invalidReason | boolean/枚举 | 有限性/长度/身份检查结果，不回写物理 |

两slot数组合计 `2×12×128×8=24576 bytes`，属于公共0.50MiB预算。`prevIndex/currIndex`、Kahan临时标量也归adapter；页面不管理其内部交换。

对renderer提供只读借用 `MaterialFrameView`：标量readonly、列值用显式访问器（如massHAt(c)、bhAt(c)），不向外暴露可写TypedArray。仅写 `readonly array: Float64Array` 并不能防止元素被改，因此不能据此声称只读。adapter自身是其私有副本的唯一写者；renderer/motion只能写各自派生buffer，视图不跨帧保留。

独立派生字段包括maskProfiles、normalX/Z、concentrationH/L、fluxH/L[129]（两端外边界槽置0）、deltaXH/L[128]、UVH/UVL双缓冲、16组裁片参数；全部renderer私有，不alias slot或页面snapshot。maskProfiles承接既有平滑，绝不覆盖mass或raw边界。

### 3.2 O(128)守恒可信度检查

一次循环复制8个原始字段、计算4个派生相字段并Kahan求和。定义 `M_H[c]=bh+mh+suspH`、`M_L[c]=bl+ml+suspL`；无需调用field.getTotals()。前后相总量分别比较，H/L不能相互抵销。

```text
tol_p = 64 * N * Number.EPSILON * max(1, absMass_p_old + absMass_p_new)
conserved_p = abs(total_p_new - total_p_old) <= tol_p
```

这是**渲染代理的数值可信度门槛**，用于拒绝无法证明闭边界搬运的估计；不是新增或修改物理守恒验收阈值。即使物理checker允许的微小漂移大于本门槛，也只冻结对应视觉运动，不补质量、不改物理结果、不提高tol以追求运动效果。所有关键数组必须有限且相库存非负；异常只返回 `INVALID_RENDER_INPUT`。

配对还要求身份/模式/几何一致、frameSerial相邻、时间单调、没有已知reset/inject/invert/后台断档。显示帧间隔>100ms作为v1视觉连续性中断，冻结本次代理并重新建基线；该门槛不影响物理dt/跟手性判定，不通过补帧动画追赶。相为空或局部Mbar不高于同一数值误差尺度时该相/列位移代理无效，不对稀薄库存除极小数放大速度。

有效配对时按设计§5.2计算 `F_p[e]=−prefixSum(M_new−M_old)`，外侧通量为0；右端总量误差在tol内仍记录为数值余量，不把残差均摊回物理列。deltaX与UV只写motion cache。若仅悬浮变化、depositedH/L及沉积profile在数值精度内未变，则沙体UV不动，可信雾层可以更新；没有净搬运证据时不以倾角符号或墙钟推进噪声。

### 3.3 每帧交换与失效策略

1. 同步复制最新页面snapshot到curr空闲slot；prev保持不可变。复制/检查未完成前不发布视图。
2. 首帧或连续性中断：只用curr绘当前静态材质，deltaX=0，初始化UV/方向状态；不能从全零prev构造巨大通量。
3. 合格配对：prev+curr共同提供运动计算；绘制只读curr原始/派生视图。静态相同快照保持相位不动。
4. 本帧处理完成后交换slot索引，下帧覆盖旧prev所在空闲slot。原视图借用立即到期；绘制失败清配对，不能重复应用同一deltaX。
5. curr有限但相总量不守恒：本次冻结运动，curr作为下一帧新基线；重置对应UV连续性，不跨注入跳变拟合。curr非有限/损坏：不替换合法prev，清pairValid，可保留最后合法显示；独立视觉证据标无效，不能称最新物理状态已成功显示。

两个slot都不持有物理引用；初始化/清理只操作renderer内存。dispose丢弃全部slot引用；不调用不存在的TypedArray.free，也不承诺GC立即归还native内存。

## 4. TypedArray、bitmap与重建生命周期（P1）

本地SDK `26.0.0/ets/component/canvas.d.ts` 已声明ImageBitmap.close、drawImage裁片重载、createPattern（可null）、OffscreenCanvas.transferToImageBitmap；采用这些接口。v1不创建PixelMap；若后续确需PixelMap，必须另列拥有者与release时点后评审，不假定它与ImageBitmap同生命周期。

| 资源 | 谁分配/持有 | 复用与重建 | 谁释放/何时 |
|---|---|---|---|
| 页面waterSandSnapshot | SandCanvas现有逻辑 | 每步copyStateTo原地更新；物理场重建按原逻辑重建 | 页面原生命周期；P-012不释放、不修改 |
| 双slot、库存/flux/UV/profile TypedArray | adapter/motion各自构造时 | 128列固定；普通帧复用；reset只清状态；身份/几何不兼容清配对 | renderer.dispose断开引用，由运行时GC；不得逐帧new |
| E/A/B/C tile、D atlas ImageBitmap | TextureCache，在首展示前分批生成/加载 | 以(configRevision, seed, pixelScale, paletteRevision, tier)为key；只生成所需档位 | TextureCache唯一调用close；换代提交后旧代无绘制引用时、OFF释放或页面退出时执行 |
| CanvasPattern/gradient | TextureCache或renderer | 按对应bitmap/context/尺寸缓存；createPattern=null转有限drawImage平铺 | 先移除Pattern/gradient和ctx样式引用，再close所依赖bitmap；不虚构Pattern.close |
| 小OffscreenCanvas/context、ImageData scratch | TextureCache生成任务 | 一个有上界的scratch复用，不逐帧transfer/readback | 任务完成/取消断开引用；输出bitmap单独close；计入重建峰值 |
| Path/mask/组间羽化描述 | renderer | 复用标量/数组；轮廓变更重算128列路径，不生成整屏bitmap | renderer.dispose清引用；绘制save/restore避免ctx跨帧持有旧pattern |

外观从OFF启用时创建/prepare资源；切OFF在该帧绘制结束后释放增强bitmap、清配对，原OFF立即可用。E→EA等升档保留兼容tile，只建新增资源；降档释放不再需要的资源，不能在后台保留全档而声称低档内存更小。

resize/DPI变化：先取消旧生成代次，重算显示映射/mask；仅pixelScale相关纹理重建，固定模型库存buffer可复用但清配对。新代完整ready后于帧边界原子切换，旧代close；老新代+scratch合计峰值≤8MiB，若放不下先释放旧代并显示明确warmup档，不能超预算双驻留。

reset/run/模式切换：清库存配对、UV增量与D方向历史，确定性tile可复用。自然方向反转保留seed，按新可信deltaX更新，不整块换噪声相位；倒立事件只跟随页面已有视觉变换并清配对，不调用第二次invert。后台暂停/恢复清连续性，不追赶视觉时间。

页面aboutToDisappear先断开帧回调，再调用renderer.dispose：标disposed、递增cacheGeneration、取消未完成任务；异步资源结果若generation过期立即close，不能挂回已退出页面。重复dispose幂等，bitmap只能关闭一次。释放的对象不得仍被下一帧drawImage引用。CPU bitmap字节、离屏surface、native/GPU缓存分别采样；close不等于可忽略驱动延迟释放占用。

## 5. D方向atlas的实现档位（P1）

方向来自设计§5.2可信库存重分布和当前轮廓的显示映射，只是净搬运的渲染代理。当前snapshot没有二维真实速度；采用deltaX的方向及原轮廓切向构造显示轴，轴角按π周期处理，位移正负仍由UV平流保留。不从tilt造方向，不冒充深层/表层/悬浮各自速度。

| 档位 | atlas与绘制 | 资格/预算 |
|---|---|---|
| D0 | 无方向atlas，B正常颗粒；stretch=1 | 静止/代理无效或D未启用；只能标D未验证，不能当D通过档 |
| **D1（v1选用）** | 4轴向0°/45°/90°/135°，每轴×3材质×128² RGBA；相邻两轴归一化交叉淡化，stretch按设计2–4上限映射 | atlas=0.75MiB；至多16组，每组仅对可信运动贡献最大的一个材质启用D，其余保留B平流颗粒；不减少轮廓128列 |
| D2（后续未选） | 8方向或每组全部材质同时双atlas混合 | 现有D额外16 draw/1MiB可能不够；须另版预算与配对测量，不能默认升级 |

D1从同一确定性连续噪声源预生成各方向裁片，使用同一UV和归一化权重，避免方向切换换seed。材质选择同等贡献时保持前帧选择、无历史时按固定H/L/混合次序；贡献失效时回stretch=1，不能靠持续淡化动画冒充运动。拉伸作用于B原裁片：每组替换一个原B绘制为两次drawImage，因此最多新增16次，符合D≤16预算；前缘1–3px毛边并入已有A遮罩/纹理alpha，不再另加16笔前缘绘制。

D1 atlas 0.75MiB之外最多留0.25MiB给生成scratch/小状态，重建峰值仍受总8MiB；不得悄悄为每亮度档复制整套atlas。B若从16组升32组，必须重审整体draw预算；本版不自动把D扩成32组。主材质以外未做各向异性这一能力边界写入配置manifest，验收录像不足以满足D锚点则D保持未验证，不能降锚点。

## 6. 绘制与内存预算

沿用设计§8.2：E/A/B/C/D/公共CPU增量预算依次0.25/0.35/0.40/0.15/0.20/0.15ms，合计≤1.50ms；新增绘制16/48/48/16/16/16，合计≤160（原基础几何另列）；常驻纹理/缓存≤4MiB，重建峰值≤8MiB。第一梯队含公共≤1.15ms。所有16/32组近似只用于纹理，geometry始终128列。

CPU预算工程口径取稳定窗口内**配对增量P95**；同时报告P50与每pass归因，不能把各pass P95简单求和假装整体P95。总绘制计数包括createPattern失败后的fallback、atlas淡化和羽化；不能只数主drawImage。draw/fill/stroke等实际提交命令的定义在性能脚本manifest冻结，OFF基础调用数与新增调用数分列。

性能优化只在外观模块进行：预着色缓存、裁屏外绘制、复用mask、合并材质裁片。不得降低物理步频/列数、漏step、用关闭E/A/B过性能、在计时窗口静默退档。O(128)库存处理包含于公共0.15ms，不另藏在“物理耗时”中。

## 7. 真机配对测量与PERF_BLOCKED（P3）

### 7.1 主体与脚本归属

| 责任方 | 交付/执行 |
|---|---|
| tester | 拥有拟议 `test/p012_perf_capture.ps1`（设备控制/系统trace采集）及 `test/p012_perf_analyze.py`（帧时/FPS/CPU/内存配对分析）；冻结工具版本、命令、事件映射、时钟单位、样本排除与manifest；不改water_sand_check.py或P011 checker |
| effector | 在指定真机执行采集脚本，冻结设备/系统/温度/刷新率/显示尺寸/输入，保存原始trace、录像及OFF/ON关键帧；并行出P4 ROI/指标附表 |
| developer | 提供同HAP可切换OFF/各档的入口，必要的**独立性能profiling构建**与只读测试钩子；不得写P011/WS生产遥测。负责缓存/绘制优化与生命周期正确性 |
| architect | 审查性能瓶颈与渲染组织改版；designer维护效果/代理边界；manager决定发布档位/排期，不由执行者降门槛 |

脚本当前仅定义归属，本文不声称其已实现。预检必须确认真机系统profiler/trace能输出本应用可归属的presented-frame时间戳，以及UI线程adapter+renderer执行区间；缺任一项报告 `PERF_BLOCKED/DATA_SOURCE_UNAVAILABLE`。回调间隔、HUD的52–60、录像编码帧率只能作诊断，不可替代实际呈现帧时间。具体SDK/设备工具命令由tester能力探测后冻结，不编造未验证的trace事件名。

CPU时间使用profiler执行区间区分同步主线程工作与异步GPU/合成等待；若使用独立性能构建trace标记，其采样开销在OFF/ON一致且需单独测量。相同插桩构建用于配对；另留无插桩release的呈现帧复核。不能把Canvas方法返回时间当GPU完成时间。

### 7.2 OFF采集时点与执行序列

**正式OFF基线必须在同一待测HAP中、启用任一增强档之前采集**：已有物理构建、输入、初态、geometry、renderer选择入口已冻结，OFF字节/图像行为回归确认，页面与设备预热后开始。历史e2f02a1截图和52–60FPS只用于背景，不作为新build比较分母。物理build、渲染代码、系统/刷新率或关键显示配置改变均重新采OFF。

每档进行至少3个独立OFF/ON配对块；同设备、同build、同初态/种子/TEST输入schedule、同采集配置。第一块先OFF→ON，第二块ON→OFF，第三块OFF→ON，以已采OFF基线为前提减少先后偏差。每次重置初态、预热10s，然后采至少60s完整固定序列，覆盖静置、±15°、慢倾/回正等匹配动作；固定序列及hash由effector/tester在开采前登记。EXPERIENCE另作同条件配对，不能拿TEST通过替代传感器体验证据。

运行中不生成截图/readback或额外录屏干扰主性能窗口；视觉录像/关键帧用同schedule的独立轮次采集，或OFF/ON均使用相同录屏并单列有录屏配置，不能混算。冷启动、纹理重建/DPI/进后台分别采专项峰值，不能删掉稳定测量期间发生的掉帧/GC作为“异常”；系统中断使一轮无效时整对重采并保留原因。

### 7.3 统计与判定

- 帧间隔为连续有效present时间戳之差，单位ms；报告P50/P95（线性插值分位）、全部原始值、每动作段及总体。1秒不重叠窗口FPS=该秒真实present帧数，报告分布及最差完整1秒窗；不能把`1000/P95(frameTime)`标作FPS分布的P95。
- 同一动作位置/窗口配对，报告ON−OFF帧时差与ON−OFF FPS差、各轮数值及跨轮中位差。CPU增量为ON的adapter+render工作与OFF对应legacy render工作之差，物理step不计入该CPU分项，但**整帧性能包含所有工作**。
- 沿用设计“不下降”：帧时P50/P95不得高于对应OFF，窗口FPS分布/最差稳定窗口不得低于OFF；同时满足CPU≤1.50ms、draw≤160、常驻≤4MiB、暂态≤8MiB。结果只在计时工具分辨率内比较，不能额外引入5%退化容忍。轮间噪声使结论矛盾则 `PERF_INCONCLUSIVE`，保留证据并补配对，不强行判PASS。
- 至少3对只是首轮最低量，不是统计功效保证。报告各对差值与按配对块重采样的95%不确定区间（10000次、seed=20261020；不能把同run所有帧当独立重复）。区间不能排除性能退化时不签“无下降”；若证据不足以证明既有目标，应改善测量或实现，不改目标。
- 内存以cache自有可核算字节+平台实际资源观测交叉核对，纹理解码、GPU副本、pattern保留与scratch计入。无法测清总资源时标 `MEMORY_EVIDENCE_MISSING`，不得仅用PNG文件大小证明≤4MiB。

### 7.4 上报路径

tester输出独立 `test/artifacts/p012-perf/<build>/<device>/<tier>/<pairId>/`（拟议证据目录），包含manifest、原始trace、脚本hash、分析表、OFF/ON差值、内存峰值、fallback档位和失效原因。effector在对应效果报告引用，不把性能数据塞进P011遥测。

超预算或无可信数据时tester登记 `PERF_BLOCKED`（细分CPU/GPU或帧时/MEMORY/DRAW/DATA_SOURCE），同时给architect和manager可定位的最差窗口及资源清单；architect给developer优化方案，涉及效果取舍由designer复核。任一档性能未过，该档保持未验证；第一梯队E/A/B未过不靠关闭其中一项发布同名通过结论。P4缺失另记 `VISUAL_MEASUREMENT_PENDING`，不混同性能阻断。

## 8. 验收与交付清单（§23.3 + 设计§10）

| 条件 | 本文定义的工程交付 | 后续验收证据 |
|---|---|---|
| P1 集成点 | §1–2独立renderer/OFF分支、§4资源归属、§5 D1档 | developer实现+architect评审；OFF/canonical无回归、档位配置真实、反复重建无资源累积 |
| P2 数据面 | §3双slot字段、O(128)相总量求和、视图借用与失效规则 | tester核验输入snapshot未变、前后buffer不alias、H/L独立、同快照零平流、注入/reset/倒立断配对、稀薄库存冻结 |
| P3 性能 | §6–7预算、脚本/采集/报告归属、同build OFF基线 | 至少3对真机原始数据、帧时P50/P95/FPS/CPU/内存/draw；满足设计不下降才签字 |
| P4 测量附表（并行） | 接收effector的ROI坐标/尺度变换、Y/HF_h/maxGrad/row-mean/steps定义与脚本hash | tester复核；参考帧与同build OFF同口径重算后，方可使用量化效果锚点；本文不代填或修改附表 |

视觉验证严格沿用设计§10与 [TEST_PLAN_T1_T4.md](../test/TEST_PLAN_T1_T4.md) 的SV-05/06/07，完整动作录像+8张参考关键帧，静置f0/f4、流动f2/f6重点复核；不改评分量表/通过门槛，不用静帧噪声证明流动。E水体p50约200/沙max约240及p99，A过渡数十px/maxGrad<8/匹配雾区std>15，B沙行std≥15/HF_h≥0.8，C振荡≥20，D匹配流动steps向≥6改善等全部引用原设计与P4口径，不进入物理checker。

每档manifest同时记录appearanceRouteId/档位/seed/config/hash、物理route/build/初态/input schedule/hash、设备/模式/显示尺度、素材与脚本hash；效果、性能、数据完整性分别判定。以既有只读快照比较验证同物理step序列下开关外观不改变材料状态；另做端到端实时性能验证，不能用固定快照回放的性能替代真实运行。

本次完成的是P1/P2/P3架构规则落盘。P4并行不阻塞架构文档；实现、真机性能、量化效果与SV评审均保持待执行，未授权通过修改物理、checker或采集阈值满足外观验收。

## 9. architect 工程集成决策（2026-10-20）

**状态：本节为 architect 对本文（§1–8）的评审结论 + developer 任务切分决策，`APPROVED_WITH_CONDITIONS → READY_FOR_DEVELOPER`（第一梯队 E/A/B）。**

### 9.1 架构稿评审

| 评审项 | 判定 | 说明 |
|---|---|---|
| 双 slot 生命周期（§3） | **APPROVED** | 页面 snapshot 每帧 copyStateTo 原地覆盖（SandCanvas.ets:1381-1386 已核对）→ adapter 深复制双 slot 是唯一安全方案；交换/失效五条规则（含守恒失败冻结、非有限保护 prev）完备；24KB 内存核算正确 |
| 守恒门槛数值容差（§3.2） | **APPROVED** | `tol_p=64·N·EPS·max(1,|M_old|+|M_new|)`：N=128 时 tol≈3.6e-8（M~2e4 量级），物理 Kahan 误差 ~5.7e-10 << tol——容差有 60 倍余量且不过宽，作为渲染代理可信度门槛（非物理验收）定位正确 |
| 性能测量口径（§6–7） | **APPROVED_WITH_NOTE** | 配对增量 P95（非逐 pass P95 相加）、presented-frame 时间戳、10000 次配对块 bootstrap（seed=20261020）均可执行；**NOTE**：预检的 DATA_SOURCE_UNAVAILABLE 路径必须先于任何档位实现执行——若真机 trace 拿不到 UI 线程区间，P3 全部测量阻塞，此时按 §7.4 PERF_BLOCKED 上报，不得以 HUD FPS 降级替代。建议 tester 在 E 实现前先做一次空跑预检（用 OFF 档验证采集链路） |
| OFF 基线时点（§7.2） | **APPROVED** | 同 HAP 先 OFF 后 ON、物理/渲染/系统任一变更重采——与 P-011 观测冻结纪律同构，无漏洞 |
| 资源归属表（§4） | **APPROVED** | TextureCache 唯一 close 权、换代原子切换、8MiB 峰值上限、dispose 幂等——生命周期闭环；「close≠GC 立即归还」的诚实声明保留 |
| D1 档位（§5） | **APPROVED** | 4 轴×3 材质×128²RGBA=0.75MiB 核算正确；「每组仅主材质启用 D」的裁剪合理；D2 明确不选 |

### 9.2 SandCanvas 薄分发入口精确改造

改造点唯一：`SandCanvas.ets` 渲染调用处（当前 :1386 `this.renderWaterSand(this.ctx, this.waterSandSnapshot)`）：

```ts
// 改造后（伪代码，字段命名以实现为准）
if (this.p012Renderer !== null && this.p012TierReady &&
    !this.canonicalEvidenceMode) {
  this.p012Renderer.render(this.ctx, this.waterSandSnapshot,
    this.buildP012FrameContext());
} else {
  this.renderWaterSand(this.ctx, this.waterSandSnapshot);  // 原样保留
}
```

约束：①一次帧只走一个分支，禁止叠加；②`buildP012FrameContext()` 复用页面既有字段（frameSerial/box/epoch 等），不新建传感器或第二套事件源；③`canonicalEvidenceMode` 与 `waterSandSnapshotEnabled` 联动——物理证据采集期间（wssnapshot=true 且 checker 消费）强制 OFF 分支；④P012 renderer 的创建/销毁挂接页面 aboutToAppear/aboutToDisappear，外观开关（appearanceTier）只改 renderer 配置不重建物理。

**生命周期挂接点**：aboutToAppear 创建 renderer（可延迟到首次 P-012 启用）、每帧 render 前由页面递增 fieldEpoch/continuityEpoch（在既有 reset/inject/invert/模式切换处理点各加一行）、aboutToDisappear 断帧回调后调 dispose。页面总改动量预计 <60 行，全部为分发与 epoch 递增，材质逻辑零侵入。

### 9.3 分批交付切分（commit 粒度与验收边界）

| 批次 | commit 内容 | 验收边界（该 commit 的 Definition of Done） |
|---|---|---|
| **E-batch** | P012RenderTypes + P012WaterSandAdapter（双 slot/守恒检查）+ P012WaterSandRenderer 骨架（E 色调/光照/高光）+ 薄分发 + OFF 回归用例 | ①OFF 分支像素级回归（同物理 seed 逐帧比对原输出）②E 档 SV-07 色调锚点初评（effector）③adapter 守恒检查用例（H/L 独立、注入/reset 断配对）④性能预检空跑（OFF 档采集链路验证，§9.1 NOTE） |
| **EA-batch** | A 云雾/斑驳（tile 生成 + 界面散射 + 水体雾） | ①E+A 联合验收（过渡宽度/maxGrad/水体 std——设计稿 §10.2 表）②静态无闪烁/运动无贴纸（录像判定）③帧时配对首轮（OFF vs E+A） |
| **EAB-batch** | B 颗粒（grain tile + UV 平流 + 运动代理） | ①第一梯队联合验收全表（E/A/B 全锚点 + SV-05/06/07）②3 对完整 OFF/ON 配对块 + bootstrap 区间（§7.3）③同物理 step 序列开关外观状态一致性（只读快照比对）④第一梯队签字或 PERF_BLOCKED 上报 |
| **C/D（后续）** | 层理/流痕 | 各自独立验收，未启用标未验证；不并入第一梯队签字 |

每批一个 commit、一个独立效果轮次；批内失败不回滚已签批次（route 开关可关 C/D，E/A/B 已签部分若后续批次引入回归则该批不签）。

### 9.4 developer 任务切分（第一梯队工作量分解）

| 任务 | 模块 | 估算要点 |
|---|---|---|
| D-E1 类型与 adapter | P012RenderTypes + P012WaterSandAdapter（§3 全部：双 slot/Kahan/视图/失效） | 纯数据面，无绘制；可独立单测（Node 通道复用 run_p011_planning_units.cjs 的 TS 加载模式） |
| D-E2 薄分发与生命周期 | SandCanvas 改造（§9.2）+ renderer 骨架 + epoch 递增 | 页面侧小改动；OFF 回归是主要验收 |
| D-E3 E 档材质 | 色调 LUT/伪法向光照/液面高光/去 127 上限 | 依赖 D-E1/E2；SV-07 + 量化锚点初评 |
| D-A1 云雾 | 界面 smoothstep 包络 + 两尺度 tile + tau 光学 | 依赖 D-E3（E 是显示前提）；E+A 联合验收 |
| D-B1 颗粒 | grain tile + sigmaG 相位混合 + 16 组裁片 | 依赖 D-E3；与 D-A1 可并行开发、联合验收 |
| D-B2 运动代理 | P012MaterialMotionCache（deltaX/UV/可信度/冻结） | 依赖 D-E1；纯数据面可先行单测（同快照零平流/注入断配对/稀薄冻结） |
| D-公共 | P012MaterialTextureCache + 资源生命周期（§4 表） | 贯穿各批；dispose/换代/close 用例随 E-batch 交付 |

建议执行序：D-E1 ∥ D-B2（数据面并行）→ D-E2 → D-E3 → D-A1 ∥ D-B1 → EAB 联合验收。性能脚本（tester 的 p012_perf_capture/analyze）与 P4 附表（effector）并行推进，均不阻塞 developer 开工。

### 9.5 结论

工程稿质量高，数值全部复核无误，模块边界/资源归属/测量口径可直接开发。唯一前置风险是性能采集链路（§9.1 NOTE），tester 预检先行。两处小修订（不阻塞）：①§2 frameContext 的 `canonicalEvidence` 建议明确来源枚举（wssnapshot 标志/tester 采集入口/手动开关，三者任一为真即强制 OFF）；②§3.1 slot 字段表补 `waterTopRaw` 在 waterLevel 动态下的每帧刷新说明（已含于 8 数组，建议显式点名避免遗漏）。

变更记录：2026-10-20，依据 manager【architect·工程设计】任务新增 §9。架构稿评审 APPROVED（性能预检 1 项 NOTE）；薄分发改造/分批切分/任务分解落盘；状态 READY_FOR_DEVELOPER（第一梯队 E/A/B），交 manager 审后路由 developer。

## 10. EAB PERF_BLOCKED 归因与工程修复方案（2026-10-21）

**状态：归因结论=【实现方案问题】（分支 A）。§10 工程方案落盘，developer PERF 修复任务解锁。视觉逐位一致为硬约束：本节全部手段不触碰 §5.1 材质模型（加性颗粒 `Y=base+sigma·nG`、tile 平铺、三带 clip、全局平流偏移均不变）。**

依据：`dev-report-P012-EAB-batch-v1.md` §4.5/§7（PERF_BLOCKED：tier3 EAB 12/12/11 fps vs tier0-2 持平 30-60）、architecture §9.3 EAB-batch DoD ④、algorithm §5.3 预算口径。归因方法：成本模型估算（提交面积×每像素采样成本）+ 三候选收益上界 + 与 66ms/帧净增的量级核对。

### 10.1 归因分析

**成本模型**（与实测吻合）：

| 成本项 | 当前实现 | 估算 |
|---|---|---|
| pattern fill 光栅 | 每帧 3 带 × fillRect(1443×1611)≈2.33Mpx = **6.98Mpx 提交**；clip 后有效面积仅 ~0.84Mpx（沙区），冗余率 ~8.3× | ~4ns/px 采样+混合 × 6.98Mpx ≈ 28ms/带量级，三带占 66ms 净增的大头 ✓ |
| createPattern | 每帧 3 次（对象创建+256²纹理上传 256KB/次） | ~1-3ms/帧 |
| 254 点 Path clip | 每带一次软件光栅化 | O(点数×带宽)，次要 |

**判定依据**：
1. **算法层没有要求这个执行方式**——§5.1 的加性模型是逐输出像素的数学定义，不规定「对整盒+512px 冗余提交 pattern fill」；tile 平铺、clip 区域、fillRect 参数全部是执行策略。算法预算（CPU O(128) 差分、≤48 draw）本实现完全满足（3 draw），说明预算口径没有预期光栅成本，但这属于预算口径未覆盖的**执行层低效**，不是材质模型的结构性成本。
2. **候选收益上界足够覆盖 3× 差距**：候选 2（包围盒收紧）单项即把提交面积 6.98→~0.7Mpx（8-10×），光栅成本 66ms→~7-9ms；叠加候选 1 后残量 <5ms，tier3 从 12fps → 45fps+，远超 ≥30fps 目标。工程手段富余量 ~4×。
3. **排除分支 B 的证据**：若是算法方案问题（材质模型太贵），则最小正确实现（精确包围盒+缓存 pattern）仍应超预算——但包围盒后的有效面积 0.84Mpx 本身就是 E/EA 档同量级（EA 持平 OFF），说明「沙区大小的 pattern 采样」本身可承受，冗余才是问题。

### 10.2 修复方案组合与实施顺序

**第一步（1+2 并行，预期收回全部差距）：**

- **F1 pattern 缓存**：三带 `CanvasPattern` 构造一次复用（成员字段，`grainBitmap*` 不变则不重建）；`createPattern` null 的 fallback 路径保留。收益 ~1-3ms/帧 + 消除每帧纹理上传。
- **F2 包围盒收紧**：每带 fillRect 从整盒 `(boxX−256, boxY−256, fullW+512, boxH+512)` 改为该带列包围盒——遍历 128 列对该带 `bandTopY/bandBottomY` 取 min/max（已有平滑曲线数据，O(128) 廉价），fillRect 改为 `(boxX, minTopY, fullW, maxBotY−minTopY)` 并向外扩 tilePx 一圈（保证 pattern 平铺周期覆盖）；**空带跳过**（如 `bl` 全 0 时轻沙带 band 高度≤0，直接 continue 不 save/clip/fill）。**视觉逐位一致性论证**：clip Path 不变，fillRect 只是缩小了「clip 之前提交的矩形」；clip 裁剪后输出像素集合与原先严格相同（原整盒 fill 被 clip 裁剩的就是包围盒∩曲线区域），pattern 采样相位由 translate 决定与 fillRect 原点无关——逐位一致成立。
- **回退策略**：F2 的包围盒计算若出现带边界 NaN/非有限（曲线异常），回退到整盒 fillRect（保留旧参数路径），并在一次性日志记录；不因几何异常丢带。

**第二步（仅当 1+2 后 tier3 仍 <30fps 时启用）：**

- **F3 离屏预合成**：颗粒层（三带 clip+pattern 合成的结果）预合成到盒尺寸离屏画布，每帧 `drawImage(offscreen, dx, 0)` + translate 提交；平流偏移或几何变化（带曲线 min/max 或 UV 变化超过阈值）时才重合成。静止态退化为单次 blit（~0.29Mpx，2-4ms）。**这是结构性兜底，不是首选**——改动面大（离屏生命周期），且 1+2 的富余量大概率用不到它。

### 10.3 离屏画布生命周期与接口（F3 预置设计，F1/F2 不需要）

若启用 F3：离屏画布归属 `P012MaterialTextureCache`（§4 资源表新增一行）；创建时机=首次 EAB 渲染；重建时机=`geometryEpoch` 变化（DPI/尺寸）或 `fieldEpoch` 变化（物理场重建）；释放=appearanceTier 降档离开 EAB 或 renderer.dispose。`continuityEpoch`（reset/注入/模式切换）只清平流偏移历史、不重建离屏画布（几何未变）。重建峰值计入 §4 的 8MiB 上限（盒尺寸 ARGB ≈ 931×311×4 ≈ 1.16MiB，远低于上限）。

### 10.4 逐项消融 want 参数定义（替代已删的 s94 renderSeed 钩子）

新增两个独立 want 参数（不占用渲染种子；EntryAbility 解析块照 `p012tier` 模式追加）：

| 参数 | 取值 | 语义 | 用途 |
|---|---|---|---|
| `p012ablate` | `off`/`grain`/`fog`/`spark`（字符串） | 消融开关：`grain`=跳过颗粒层、`fog`=跳过雾层、`spark`=跳过亮粒、`off`=全部启用（默认） | 成本隔离：分别量化各层贡献 |
| `p012fillbox` | `full`/`tight`（字符串） | fillRect 策略：`tight`=F2 包围盒（默认，交付后）、`full`=旧整盒（A/B 回归对照） | 验证 F2 前后视觉逐位一致+性能对比 |

AppStorage 键名同名；renderer 读取处集中在 drawSandGrain 入口（单点分支）；参数非法值=忽略并用默认（与 p012tier 同容错风格）。这两个参数属**外观诊断域**，不进物理 checker/遥测。

### 10.5 验收口径

1. **tier3 ≥30fps 判定细则**：`tools/run_p012_perf_ladder.py` 同相位（25s 冻结静置后）三帧 FPS 窗口全部 ≥30；对照 tier0 OFF 同相位窗口无下降趋势（EAB 帧时增量 = EAB 帧时 − OFF 帧时 ≤ 33ms − OFF 帧时，即总帧时不超 33ms 预算的 60fps 档）。修复后需重跑 §4.3 的 3 对 OFF/ON 配对块（PERF 数据）。
2. **锚点逐位一致或差异可解释**：修复后 `run_p012_eab_pairs.py` 的锚点测量须与 §4.1 表逐位一致（F1/F2 理论上逐位一致——同 pattern 同 clip 同输出像素）；若实测有差异，解释边界限定为：抗锯齿边缘半像素（fillRect 原点变化导致的亚像素重采样差异，允许 ±1 灰度级的带边界单像素列差异），沙区内部测量 ROI 必须逐位一致。超出该边界的差异=修复引入回归，不接受。
3. **消融验证**：`p012fillbox=full` vs `tight` 同场景截图 ROI 逐位比对（除带边界亚像素列）+ FPS 对比（预期 8-10×）；`p012ablate=grain` 复现 tier2 水平（确认颗粒层就是唯一成本源）。

### 10.6 风险清单

| 风险 | 缓解 |
|---|---|
| pattern 缓存失效时机：bitmap 重建（tier 升降档、种子变化）后缓存 pattern 悬挂旧位图 | 缓存键=(bitmap 引用)；`makeGrainBitmap` 重建处置空 pattern 缓存；dispose 释放 |
| ArkTS createPattern 返回 null 的时机与旧版不同 | fallback 路径保留（透明填充+一次性日志），不因缓存改变 null 语义 |
| 包围盒的曲线异常（NaN/空带） | F2 回退整盒路径；空带跳过前先验证带高度>0（`maxBotY−minTopY>0`） |
| 离屏画布内存（若 F3） | §4 的 8MiB 峰值上限内（1.16MiB/画布）；TextureCache 唯一持有 |
| 亚像素差异争议 | §10.5-2 的解释边界预先冻结；测量 ROI 排除带边界 ±1px 列 |
| F1+F2 后仍不达标 | F3 兜底已预置（§10.3）；若 F3 仍不达标则重新归因（届时再评估是否分支 B） |

### 10.7 结论

**归因：实现方案问题（分支 A）**——成本在执行层（8.3× 光栅冗余 + 每帧 pattern 重建），不在 §5.1 材质模型；工程手段（F1+F2）收益上界 ~10×，对 3× 差距有 4× 富余。修复方案 F1/F2 先行（视觉逐位一致可证），F3 预置兜底；消融 want 参数（p012ablate/p012fillbox）替代 renderSeed 钩子；验收口径与回退策略冻结。**developer PERF 修复任务（6ad7fe32e4b0a2dfeb1e1b92）解锁。**

变更记录：2026-10-21，依据 manager【P-012·PERF归因】任务新增 §10。归因=实现方案问题（成本模型+收益上界论证）；F1/F2/F3 方案、消融参数、验收口径、风险清单落盘；分支 B 不触发。

## 11. C 层理工程方案（2026-10-21，依据 design-P012-C-stratification-anchors-v1.md）

**状态：C1 工程方案 `READY_FOR_DEVELOPER`。设计稿已给定的项（伪代码时序/复杂度 O(128+K)/K≤16/预算 ≤16 draw、CPU 配对增量 P95≤0.15ms、常驻 ≤0.25MiB/bandNoise 语义/C 先于 B/A/非负权重线性插值保层序）确认可行，不重设计；本节只做 designer 留给 architect 的三件事：缓存表示选型、采样接口定义、ArkTS/Canvas 可行性翻译。**

### 11.1 缓存表示选型（三件事之一）

**选定：窄高「条纹位图 + drawImage 平铺」（位图预生成），逐帧零像素重生成。** 候选对比：

| 表示 | 判定 | 理由 |
|---|---|---|
| **A. 纵向 1D bandNoise 表（Float64Array）+ 逐帧逐列 fillStyle 计算** | 否决 | 每帧对 ~沙区像素做 JS 数值计算（bandNoise/phase/envelope）——正是设计稿「逐帧无全屏像素重生成」禁止的形态；且 Canvas2D 无逐像素写回通道（putImageData 全屏重建更贵） |
| **B. 窄高条纹位图 + drawImage 平铺（选定）** | **选定** | bandNoise 是**稳定深度函数**（seed 固定、phase=depth/λ+warp，不随帧变）→ 可一次性烘焙进位图；运行期只是「平铺 + 逐带 clip + globalAlpha」——与 B 档 grain 的 s99 优化路径完全同构（已实测 32/32/33fps），零新风险。位图按 256px 高周期烘焙（λ 冻结后 λpx=λ_material×scale），宽 64px（warp 是横向慢变，64px 内可完整表达）；三材质三张（H/L/M 各一 seed），常驻 3×64×256×4=196KB ≤0.25MiB 预算 ✓ |
| C. 逐帧 OffscreenCanvas 重合成 | 否决 | 每帧全屏 putImageData/drawImage 链 = 设计稿禁止的全屏重生成；且引入离屏生命周期管理，B 已证明不需要 |

**烘焙细节**：位图像素 = `deltaY(u_mod) = envelope·amplitude·bandNoise(seed, u_mod/λpx)`，u_mod∈[0,256)；位图**灰度单通道语义、纯不透明**（alpha=255，RGB 同值——增量在运行期经 globalAlpha 混合，吸取 s94 教训绝不烘焙 alpha）；运行期 `ctx.globalAlpha = amplitude_scene / 255`（envelope 的带边界平滑衰减烘焙在位图纵向边缘，幅度换算在 alpha 一侧）。**λ 冻结后重烘焙**：λ_material 在 run 首个合格厚层场景冻结（设计稿 §3.1），DPI/几何 epoch 变化时按新 scale 重烘焙位图（复用 B 档 buildGrainBitmaps 的 epoch 钩子模式）。

**warp 的处理（关键差异）**：grain 的平流是**全局平移**（一个 off 值）；C 的 warp 是**逐列慢变相位**——位图平铺只能给全局平移，逐列相位必须由**多段平铺**近似：把 128 列按 warp 相位分段（相邻列 warp 差 >1/16 周期即断段，K≤16 段上限），每段用该段中心列的 warp 相位做 drawImage 的 x 偏移。段内相位误差 ≤1/32 周期（肉眼不可辨），段数 K 由 warp 幅度 ≤1/4 周期的设计约束自然封顶：极端满幅 warp 时 128 列相位跨度 1/4 周期 → 按 1/16 阈值最多 4 段/带，三带 ≤12 段 ≤ K≤16 预算 ✓。

### 11.2 采样接口定义（三件事之二：MaterialColumnSampler，C/E3 共享）

新增 renderer 内部类（放 P012WaterSandRenderer.ets 或独立文件 P012MaterialSampler.ets——建议独立，E3 将复用）：

```ts
// P012MaterialSampler.ets（新文件，纯函数式，只读）
export class P012MaterialColumnSampler {
  // 输入：smooth 数组（renderer 已有：smoothBh/smoothMixStart/smoothSandTop，
  // 每帧 updateSmoothed 后传入）——已是 128 列平滑值
  constructor(bh: Float64Array, mixStart: Float64Array, sandTop: Float64Array) {}

  /** 任意显示 x（px）处的连续材料层高。非负权重线性插值保层序：
   * t = (x/colW - floor) ∈ [0,1)；bh(x) = bh[i]·(1-t) + bh[i+1]·t；
   * mixStart/sandTop 同式。非负(1-t,t)权重 → 厚度与边界单调线性 →
   * 层序保序（bottomH ≤ mixStart(x) ≤ bh(x) ≤ sandTop(x) 逐点保持）。 */
  bhAtPx(x: number): number;
  mixStartAtPx(x: number): number;
  sandTopAtPx(x: number): number;
}
```

- **与 E3 的共享边界**：接口只做「显示 x → 连续层高」一层（C 着色与 E3 雾 alpha 都需要）；C 不等 E3 交付（设计稿 §4 明示），E3 复用时直接 import，禁止第二套插值实现。
- **采样密度**：C 用逐段平铺（11.1），段内不采样——sampler 只在**分段点**调用（K+1 次/带），O(128+K) 中 128 是 smooth 数组既有更新成本，K 是段边界采样。不引入 O(128r) 的加密采样。
- **物理列 128 不变**：sampler 是光学着色专用，轮廓绘制仍用逐列 fillRect（drawSandColumns 不改）。

### 11.3 ArkTS/Canvas 可行性确认（三件事之三）

1. **OffscreenCanvas 烘焙**：`makeGrainBitmap` 先例（:580-611）证明 OffscreenCanvas/createImageData/putImageData/transferToImageBitmap 链路可用——C 位图烘焙同一模式，纯增量复制，可行性 PASS。
2. **drawImage 平铺**：drawSandGrain s99 版（:712-745）证明 clip+整数坐标 drawImage 平铺达 32/33fps——C 复用同骨架（bandTopY/bandBottomY/带曲线 clip/整数化偏移全部现成），可行性 PASS。
3. **globalAlpha 混合**：B 档 P012_B_GRAIN_ALPHA 先例；C 用同法（位图不透明 + alpha 控制增量幅度），且 |deltaY|≤8 的设计上限 → alpha ≤8/255≈0.031，混合成本极低，PASS。
4. **不构造 128×层数 Path**：分段平铺天然回避——每段一个 clip（带曲线）+ ≤3 drawImage/段，Path 点数仍 2×128（与 B 档相同），PASS。
5. **C3 与 E 缓存复用审计（三件事之 2）**：C3（宽明暗块作 C1 低频基底）**复用 E 档 shadeLut 光照**——C3 的 2–4 宽块本质是「按 mixStart/bh/sandTop 分域的粗粒度光照分区」，E 的 8 档 shade 已按坡度分域；C3 实现为「在 C1 位图之上叠 2–4 个低频明暗矩形带（clip 同带）」即可，**不新建缓存**（零额外位图，2–4 draw 从 C 的 16 draw 预算出）。审计结论：可行且零复用成本，但 C3 不单独验收（设计稿 §3 已定，只作 C1 基底）。
6. **风险点（新增）**：C 位图高度 256px 是显示 px；λ 冻结值是材料深度单位 → 烘焙时需 scale 换算（λpx = λ_material × heightToY 的逆变换系数）。若 scale 在 run 中途变化（resize/DPI），epoch 钩子重烘焙（11.1 已列）。**呼吸风险**：λ 不随层高归一化（设计稿红线），层高变化时层纹数量按实际厚度增减——这是设计行为不是 bug，单测覆盖。

### 11.4 成本核算方式（三配对组合归因接口）

照 §10.4 消融模式新增 want 参数 `p012ablate` 枚举加 `strat`（跳过 C 层理）——C 的配对归因（设计稿 §5 表）全部通过现有消融机制执行：

| 配对 | 执行方式 | 归因输出 |
|---|---|---|
| EAB ↔ EAB+C | `p012ablate=strat` vs `off`，同 build/seed/输入 | C 净增量：draw、CPU P95（配对块）、帧时 |
| EAB+C 内部 C1/C3 | C3 是 C1 的叠加子层，无独立开关（设计稿定位）；如需诊断，临时 build 常量（不进 want） | C1 vs C1+C3 的幅值差异（效果域，非性能域） |
| EAB+C ↔ EAB+C+E3 | E3 交付后补测（设计稿 §5 第 3/4 行） | 共享 sampler 收益（本节 11.2 预置） |

帧时统计沿用 §7.3 配对块（P50/P95）+ §10.5 口径；C 自身 draw 计数进 `lastDrawCount` 现有链路（无需新遥测）。

### 11.5 预算违规重排裁决入口（三件事之 3：流程约定）

developer 实现后若 C 增量 **P95>0.15ms 或 draw>16**：**停下回 architect，不得自行优化绕过**。重排流程（约定）：
1. developer 报告违规数据（配对块 JSON + draw 计数），注明形态（段数超标/烘焙重入/drawImage 次数）；
2. architect 按形态重排：段数超标→warp 分段阈值 1/16→1/8（视觉 trade-off 评估）；drawImage 超标→段合并策略/位图加宽减少横向平铺数；烘焙重入→epoch 粒度调整；
3. 重排后出修订段（本节追加），developer 按修订实现；
4. **禁止的绕过**：减物理列、跳 step、隐藏关 B/A、降帧求过、把 C 移到 OFF 分支渲染（设计稿 §5 红线照抄）。

### 11.6 风险与单测范围

| 风险 | 处置 |
|---|---|
| warp 分段阈值导致的可见段缝 | 段边界两侧 drawImage 用同一位图同相位中心 → 相位连续（无跳变），仅 alpha 混合叠加处可能有半像素重采样——单测断言段边界列的采样相位连续 |
| λ 换算错误（材料深度 vs 显示 px） | 单测：同 seed/λ 下位图周期与 heightToY 逆变换一致；层高不变时层纹间距稳定（多帧差分零漂移） |
| 位图烘焙 alpha 事故（s94 复发） | 单测断言位图 alpha 通道全 255；增量只经 globalAlpha |
| C 档开关与消融交叉 | want 参数 `strat` 与 tier 无 EAB 时 C 不激活（C 只在 EAB 档上叠加——appearanceTier 仍为 EAB，C 由消融开关控制，不新增 tier 枚举） |
| 空带/薄带 | bandTopY/bandBottomY 现有逻辑复用（高度≤0.5px 跳过）；INSUFFICIENT_LAYER_SUPPORT 按设计稿 §6 由 effector 登记场景 |
| epoch 断配对 | resetContinuity 现有钩子 + 位图重烘焙挂 geometry epoch——单测：reset 后 C 层纹 seed 不变（材料身份不变）、相位基线不变 |

单测清单（Node 通道，照 run_p011_planning_units.cjs 模式）：①sampler 线性插值保层序（含边界 x=0/x=boxW）；②bandNoise 位图零均值有界（烘焙后统计验证）；③分段连续性（段边界相位连续）；④λ 换算一致性；⑤位图 alpha=255 全检；⑥消融 strat 跳过 C（draw 计数不含 C）；⑦epoch 断配对后 seed/相位稳定；⑧空带跳过。

### 11.7 结论

三件事完成：**缓存=窄高位图+分段 drawImage 平铺**（与 B 档 s99 同构，零新风险）；**采样=MaterialColumnSampler 非负权重线性插值**（独立文件，E3 复用预置）；**ArkTS 可行性=确认**（OffscreenCanvas/drawImage/globalAlpha/整数坐标全部有交付先例）。C3 复用 E shadeLut 零新缓存。消融走 `p012ablate=strat` 现有机制；预算违规回 architect 重排流程已约定（11.5）。**developer C 实现任务解锁**（任务书 PIPELINE 第 (2) 步完成）。

变更记录：2026-10-21，依据 manager【P-012·C层理工程】任务新增 §11。设计稿三件事落盘：缓存表示选型（位图+分段平铺）/采样接口（P012MaterialColumnSampler 契约）/ArkTS 可行性确认（三先例）+C3 复用审计/成本归因接口（消融 strat）/预算违规重排流程约定。C 锚点冻结前置（设计稿 §2.2/§6）不在本节范围，归 tester/effector。
