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
