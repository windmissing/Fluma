# P-011 路径5：双角参数用户可调化 v1

日期：2026-10-21。责任：designer。依据：用户23:06裁决“先不针对场景标定，把起停角做成用户可调参数，继续推进后面工作”。**路线选择已获用户授权；本工程设计待manager验收及architect细化，尚未实现/验证。**

## 1. 目标、默认值与红线语义

将 thetaStart/thetaStop 的产品用途从“等待场景标定输出”改为**用户配置**。出厂推荐为 **thetaStartDeg=26、thetaStopDeg=20**，来源标识 `factory-recommended`，界面文案“出厂推荐，非材料标定值，可调整”。这两个数原为candidate，不因成为推荐值而获得物理标定证据。

标定协议及历史报告完整保留，未来预设场景包可按原流程重新标定。§19.6/原标定协议“不填默认值”的适用语义明确为：**不得在标定证据不足时，用默认值替代估计值或伪造标定通过**。本路径允许显式推荐配置，但 `thetaStartEstimate/thetaStopEstimate/CI` 仍为null、calibrationStatus为NOT_CALIBRATED。原34run、四cell/24层、CI、搜索窗均未修改，只是不再作为当前产品参数就绪或阶段一关闭的前置。

物理执行继续复用 `natural-relax-hysteresis-v1` 双角状态机与原relax apply；产品底层 `WaterSandParams.routeId` 的既有身份不借本任务改名。correction只能按显式 `relax-correction-weighted-v1` 路由启用；阈值非null不能成为correction开关。

## 2. 参数接口与唯一配置来源

### 2.1 接入WaterSandParams/configure

当前 `WaterSandField.ets:837` 的 `configure(params)` 已有参数范围与finite校验、校验后复制参数的模式；`P011RelaxHysteresis.ets` 已有 `deriveHysteresisThresholds` 的成对不变量校验。工程设计沿用这两个入口，不在UI实现第二套物理阈值解释。

拟新增以下强类型字段（仅接口设计，不是本次代码改动）：

```text
WaterSandParams:
    thetaStartDeg: number
    thetaStopDeg: number
    relaxMode: 'dual-user-configured' | 'dual-protocol' | 'legacy-single'
```

新产品默认 `dual-user-configured`、26/20；两个数为必填完整pair，不允许只传一个或在configure失败时自动补另一个。`legacy-single`仅供明确指定的旧行为回归/兼容入口，不作为产品调参选项；该模式保留旧angleDeg语义和既有位精确基线。`dual-protocol`供明确授权的标定/实验入口，不由用户面板选择。

`angleDeg` 继续是legacy单角参数，不能重命名成thetaStart、不能同时作为停止角或tan30硬上限。双角模式下实际起停判据仅来自新pair；硬上限始终为不可编辑的30°。configure现有对其他参数的校验继续执行，本UI只更新新pair，不向用户开放预算、guard或maxTiltOmega。

物理层持有一份已提交的 `activeThresholdConfig`：pair、tan派生值、relaxMode、configRevision、source/provenance；状态机、planner preview、真实apply和executedThresholds全部读取同一版本。参数完整复制后再派生/绑定的具体字段归属由architect落地，**不得继续让probe是否存在或遥测清理决定物理阈值是否启用**。

当前 `setNaturalObservationRunMetadata(..., thetaStart, thetaStop)` 会调用 `applyRelaxHysteresisConfiguration()`，必须拆开职责：普通元数据更新只记身份、不覆盖用户配置；旧实验入口可在run启动前经显式 `dual-protocol` 适配器把manifest pair送入同一个configure事务。元数据pair与active配置不一致时拒绝启动该实验，不静默“最后写入者获胜”。

### 2.2 安全域与拒绝语义

完整有效条件沿用已有derive函数：

```text
finite(start) && finite(stop)
0 <= stop < start <= 30
slopeStart = tan(start*pi/180)
slopeStop  = tan(stop*pi/180)
slopeHardMax = tan(30*pi/180)              // 用户不可修改
```

选择该安全域而非将旧搜索窗当UI限制：`[25°,30°)/[15°,25°]` 是未来标定的先验候选搜索窗；`0≤stop<start≤30` 是现有状态机可接受的配置不变量。两者用途不同，用户配置域的开放不改标定搜索窗。start=30、stop=0分别按已有边界允许；它们可能使激励难以达到或滑行停止很慢，合法不代表保证活跃/快速收敛。

任何越界、相等/反序、NaN/Infinity、缺字段、字符串未成功解析均拒绝整个pair：物理参数、持久化、配置版本、active与材料状态均不变。复用 `WaterSandParamError`/阈值校验异常边界，由应用层转成可读提示；**不clamp、不交换两值、不以26/20掩盖非法输入**。已有校验不会自动认识新字段，developer须显式把新字段加入finite列表并调用成对derive校验。

用户参数只改变“何时进入/退出active及其请求”。所有请求仍受同一planner可行域、逐writer中央及邻边guard、local/bottom/total/channel预算、库存、容量和H/L守恒控制。既有 total=300/s、bottom=10/s 的冻结运行配置不变；配置更新不得重发额度或放宽已存在的其他局部限额。合法参数下如果物理事务无法提交，仍hold/BLOCKED，不能用用户设置作为安全豁免。

### 2.3 生效事务、持久化与生命周期

用户编辑草稿，点“应用”后整对提交。拖动或输入中间态不进入物理层；成功时在**当前完整子步结束、下一个子步开始前**原子生效。若当前有preview/apply进行中，排队至该边界，不在同一事务中混用两套阈值。

```text
onApply(draftPair):
    validate full proposed WaterSandParams + derive pair, before any mutation
    stage pair and provenance together
at next safe substep boundary:
    commit activeThresholdConfig with new configRevision
    invalidate all plans/snapshots tied to previous configRevision
    reset sliding/phase-private caches; record CONFIGURATION_RESET
    persist accepted pair and provenance; then continue regular step(dt)
```

配置变化清空真实sliding、观测active镜像、旧phase baseline/F/X/residual、correction预留/排序/请求缓存和跨步bracket前驱，开启新configurationEpoch/analysisSegment；首次新子步按新阈值从inactive评估。清理本身不是物理stop事件，不能计入start/stop验收或标定。重新baseline必须在新epoch从实际材料读取，不做目标形状注入。

保留bh/bl/mh/ml、悬浮库存、水体、acceptedTilt、现有输入模式与正常时间推进；配置操作本身材料ΔV=0、不推进物理dt。预算只在正常新子步按dt生成，历史used/守恒累计量不因配置修改清零。快照必须携带configRevision，旧版本preview不能回滚覆盖新参数；运行中的正式schedule锁定参数，用户需先结束实验后再修改，不能把参数编辑当一条schedule命令。

| 事件 | 参数与来源 | 路线私有状态/记录 |
|---|---|---|
| 首次安装/无历史设置的首次初始化 | 26/20、factory-recommended、推荐版本v1 | 正常新场初始化，不标CALIBRATED |
| 普通run/reset | 保留已接受用户pair和来源；材料按既有reset行为处理 | 新run清理active、phase、bracket、预演/预算临时缓存 |
| “恢复推荐” | 显式提交26/20，source恢复factory-recommended | 与应用新pair同一事务，绝不隐式触发correction |
| TEST/EXPERIENCE切换 | 同一pair/来源保持，传感器不能改pair | 沿用模式切换清理路线私有状态和输入所有权，断开跨模式bracket |
| 方向翻转/invert180 | 参数不变 | 沿用既有方向/倒置生命周期；donor/邻边/预演按新状态计算，不沿用旧grant计划 |
| 退出protocol实验 | 恢复进入前保存的用户配置快照，明确记录新epoch | 不把实验candidate写回用户偏好；旧legacy回归入口保持明确模式隔离 |
| 持久化数据损坏/非法pair | 报错并保持当前合法会话配置；冷启动无法恢复时等待用户点“恢复推荐” | 不静默clamp/default，不以错误数据启动模拟 |

旧版本从未存过双角字段时，版本迁移可一次性写入factory-recommended 26/20并记录migration；这属于用户已批准的新功能默认初始化，不能用于掩盖“已存值非法”。持久化写入失败不得展示“已保存”，区分会话已应用与保存失败，可重试；后续重启按实际存储版本恢复。

## 3. 来源、thresholdCalibrationId与执行账本

建议保持字段向后可解析，同时明确标定与配置的语义区别：

| 场景 | thresholdCalibrationId | thresholdSource | 阈值/估计字段 |
|---|---|---|---|
| 出厂推荐或用户配置产品模式 | `user-configured` | `factory-recommended`或`user` | configured/executed为实际pair；estimate/CI为null |
| 固定推荐值的correction对照 | `user-configured` | `factory-recommended`，另记实验身份 | 所有组冻结26/20；不声称标定 |
| 未来正式标定/holdout | 原protocol manifest的真实calibrationId | `protocol-candidate`（未来有正式制品再另登记） | 维持现有估计/CI与身份规则 |
| 明确legacy回归 | 保留旧null等历史契约 | legacy身份 | executed双角为null，不伪装成已启用双角 |

`user-configured`是模式来源标签，**不是唯一实验身份**。新增或在manifest绑定 `thresholdConfigId/configRevision/thresholdSource/recommendationVersion`；configId由完整规范化pair、来源、模式及版本生成可追溯身份，runId仍独立。数值恰等于26/20但由用户输入时source仍为user，只有“恢复推荐”操作切换来源。

产品模式 `runRole=null`，不伪装calibration/holdout来通过旧checker分支；需有独立的配置执行核验分支，即使runRole=null也核验pair、配置版本、configured/executed、slope值、decision阈值和实际状态转换。保留 `decisionG/decisionR/decisionAlpha/transitionReason/thresholdUsed` 及实现/source/build hash，继续区分真实stop与限流。

正式实验manifest与实际配置不一致、单run/epoch内偷偷换pair、模式不匹配都应判执行身份错误；不因“用户可调”跳过身份验证。历史run及校准ID不重写。现有14个eligible事件和统计不足结论保留，不转记为标定PASS；§1A发现的资格问题仍登记给tester，但它们不再驱动无意义的34run扩采。

## 4. UI与交互

在现有SandCanvas参数面板的watersand适用区域添加“滑移起停角”，TEST和EXPERIENCE共用同一设置。两个数值输入/步进控件：起动角、停止角，默认26.0°/20.0°，显示精度与UI步距0.1°；UI粒度不改变底层有限实数的合法条件。

面板提示：

> 起动角决定沙面何时开始滑动；停止角决定滑动何时结束。角度指沙面相对重力的局部偏离，不是手机倾角。允许 0° ≤ 停止角 < 起动角 ≤ 30°。出厂推荐26°/20°，未经材料标定。

交互采用“草稿→应用/取消”，另有“恢复推荐”。动态提示反序/超域错误并禁止提交，保留输入供修改；不要自动移动另一控件。成功显示已应用值；非法例 `(20,26)` 显示“停止角必须小于起动角”，`(31,20)` 显示“起动角不能超过30°”。输入缺失或非数值显示“请输入有效角度”。

接近端点可提示“起动角越高越难触发；停止角越低滑行可能持续更久”，不宣称改变手机倾角跟随上限。正式schedule运行中只读并显示“实验运行中，结束后可调整”。UI不提供预算、guard、硬上限开关，不在产品界面展示checker/遥测实现术语。

## 5. 默认启用与旧回归的兼容边界

新产品默认从可能的legacy单角路径转到显式双角路径，这是本次授权的行为变化，不能伪称与旧默认轨迹位精确一致。发布构建/配置版本必须变更，历史legacy基线保留，由明确legacy模式验证旧行为；新双角默认另立26/20的行为基线。

设置26/20只解决参数可用性，不保证±45°全宽收敛或所有材料效果达标。±15°静止回归继续按原冻结场景与固定推荐值验证；用户若设置较低起动角，局部alpha跨其配置阈值而产生start是预期行为，不能继续要求任意用户pair下start都为0。可调域的执行验收应按实际残差与所配置阈值判断；tan30/预算/守恒在所有配置上都保持硬要求。

## 6. correction对照矩阵：固定推荐值基线

沿用 [correction设计§9](algorithm-P011-relax-correction-v1.md) 与 architecture§24.3，不更改A/B/C/D算法因素：

| 组 | 原对照因素 | 本次统一阈值来源 |
|---|---|---|
| A | 原relax分配，无correction | 双角26/20，factory-recommended，非标定 |
| B | 仅专用预留 | 同A |
| C | 预留+e²权重 | 同A |
| D | 预留+e²×净贡献 | 同A |

A是**双角推荐配置下的原relax**，不是旧tan4单角基线；若要比较legacy须另列对照，不能混成correction收益。四组固定相同材料初态、几何、输入、dt、其他参数、配置hash和阈值来源，只改变既定分配因素。更换用户pair即另开一组完整矩阵，不拼进26/20矩阵；参数清理/预热规则各组一致。

既有E/maxResidual/ΔE_relax、挤占代价、raster、FPS、guard/预算/守恒和CORRECTION_STALLED报告要求不变。对照可以不等待场景标定，但仍须已有工程/遥测前置和manager实验排期；本设计不授权真机采集或correction默认转正。

## 7. 阶段一关闭标准与交接

本次用户裁决把阶段一·A的观测③要求从“完成场景双角统计标定”改为**执行链验证完成＋参数机制就绪**。不将任意代码已能读取candidate等同于产品参数机制已实现。

| 必须提供的证据 | 判定内容 |
|---|---|
| 既有执行链回执 | 保留真实双角读取、边身份、start/hold/stop、限流不误报stop、planner/guard与账本对齐成果，引用准确构建；历史统计不足保留 |
| 配置机制回执 | 完整pair合法/非法/非finite校验；非法无副作用；26/20来源正确；配置版本原子切换；preview/apply/ledger一致；UI应用/恢复/持久化与模式切换符合设计 |
| 安全回归 | 各配置均不改变tan30、全writer guard、预算、守恒、库存/容量和有限值门槛；配置变化不产生材料量变化或额外额度 |
| 状态与路线 | 新epoch清理不会伪报物理stop；null元数据不关停双角；用户配置不启用correction；legacy明确模式与新默认基线分别核验 |
| 阶段一其余B/C | 维持2026-10-20阶段边界与后续正式静止判定口径；已通过硬/软项不得退化；效果仍需原适用证据 |

tester后续至少覆盖：合法默认、0停止角、30起动角、相等/反序/负数/>30/非finite/缺字段；实际残差在start/stop等号与两阈值之间的三态语义；活跃时配置变化、两个输入模式、reset及非法保存。这里只提出验证契约，**本任务不修改测试用例或执行测试**。

实现复杂度：pair校验/派生O(1)，配置epoch切换清缓存O(N)，正常状态机复杂度不变，N=128不变；不增加逐帧扫描或额外物理writer。最终帧率与体验回归由后续验证提供，本设计不声称已通过。

路由：manager验收方案及阶段一标准回写→architect配置/模式/生命周期/遥测与UI接口→tester按新执行目的登记验证→developer实现→tester/effector回执→manager进行阶段一CLOSED评估。原场景标定记录改为 `DEFERRED / NOT_CALIBRATED / NON_BLOCKING_FOR_STAGE1`；不会删除未来协议或追认旧run。

**当前只完成设计；阶段一尚不因本文自动CLOSED，P-011整体和±45°目标包更未完成。**
