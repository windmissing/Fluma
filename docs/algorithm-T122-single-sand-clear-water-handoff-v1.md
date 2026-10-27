# T122 单沙明水方案存档与三线交接 v1

2026-10-27，designer；原卡 `6adf3c1de4b0e6ce82cd4117`，承接已关闭T106。交接版本 `T122_HANDOFF_V1`，算法路线仍为 `SINGLE_SAND_CLEAR_WATER_V1`。**方案文件／接口／可测合同交接完成，DESIGN_READY_FOR_ARCHITECT；不重开T106，不重复从零设计，不代表产品实现或效果验证通过。**

当前归属目标7，P3排在前六大目标之后；目标6为仿真compute shader管线，保持GPU最高调度优先。历史T106文中的“目标六／前五”仅是当时编号，本次已在算法正文顶部加当前适用性说明，未改算法或既有阈值。

本轮不使用真机、不改业务代码、不重跑测试、不占设备窗口。所有渲染含背景前景只走新XComponent+shader，禁止Canvas上屏／失败回退；失败FAULT或显式STALE上一有效新管线帧。不允许修改 `D:\hmprojects\HelloWorld\docs\后续工作清单.md`，仅自己行状态有限例外，不改其它行列／队列／滴答同名清单，不回退共享修改。manager按完成通知登记推进，无新增验收门。

## 1. 实际到件、历史和本次存档

本次开始时约定三份文件已真实存在于共享工作区，Git状态均为untracked，而非不存在；T122建卡时的缺件描述是历史观察。本轮复用这些产物，并补交接索引、目标编号适用性及指纹清单，不把用户完成通知当文件证据。

| 资产 | 实际路径 | 用途／性质 |
|---|---|---|
| 正式算法 | [algorithm-single-sand-clear-water-v1.md](algorithm-single-sand-clear-water-v1.md) | T107/T108共同消费；§1–9完整路线、接口、阶段、18组合同 |
| T106独立复盘 | [retro-T106-single-sand-clear-water-v1.md](retro-T106-single-sand-clear-water-v1.md) | 原候选取舍及近似限制；保留历史日期／内容 |
| T106设计算术／输入证据 | [design-T106-single-sand-water-evidence-v1.json](design-T106-single-sand-water-evidence-v1.json) | 2026-10-26源码／报告SHA和精确有理数示例，非产品测试 |
| 本次交接manifest | [design-T122-clear-water-handoff-v1.json](design-T122-clear-water-handoff-v1.json) | 资产SHA/长度、历史输入与当前差异、CW/接口交接完整性 |
| 本次独立复盘 | [retro-T122-clear-water-handoff-v1.md](retro-T122-clear-water-handoff-v1.md) | 说明为何复用原件、保留历史而补交当前适用性 |

上述全部路径相对 `D:\hmprojects\HelloWorld` 的docs目录，文件可直接读取。存档提交只包含这五份资产及本说明，不包含业务代码、他人修改或调度清单。最终真实commit在T106与T122原卡回执列出；manifest不自嵌尚未产生的commit或自身SHA，避免循环身份。manifest分别记录原始工作区bytes/SHA256及Git规范化blob的OID/bytes/SHA256，避免Windows换行转换被误认为文本内容缺失。当前工作区HEAD不是原T106设计交付commit，不能拿它冒充历史提交。

## 2. 已定的最小范围和近似

复用T106推荐路线：显式真实水库存W、单一SAND_1物种（可由H/0承载）、真实粒子→LAND→床，保守邻接水头流、触摸掉沙／掉水双源成功账、同FrameToken透射明水。无水／有水保持同mediumMode，不能将dry占据包络作为水面或将历史灰绿水雾改名为明水。

最小近似必须随工程／测试继续传递：飞行沙是稀疏点粒子，不计入浸没排水，LAND后以床体积挤水；水流为过阻尼水头松弛，未模拟惯性波或水流携沙；掉水为触点对应列的自由面真实质量入口，未模拟空中水滴飞行。床不可渗透、不新增湿沙黏聚／第二沙种／油相／SPH。容量R=S+W+P中的P预留是保守政策，不能拿它当真实水体积。

## 3. T107可消费的工程接口索引

T107任务：`6adf343de4b06ee35743c76a`，architect，输出约定 `docs/architecture-single-sand-clear-water-v1.md`。以下是已有算法合同的定位，不替代architect编写工程方案。

| 三线／接口 | 已定输入输出与不变量 | T106位置 | T107继续落地 |
|---|---|---|---|
| 仿真状态 | S为CanonicalS；W权威非负；P真实pool；U/L=0；η=S+W；R≤C | §3.1 | 独立mode、定容数组／cache／rollback、原dry/wet隔离，不能隐改旧数组语义 |
| 源账与守恒 | SandInitial+I_sand、WaterInitial+I_water；成功源分开累计，运行源不重设基准 | §3.2、§5 | 映射原审计归约／比较方式及事务身份，保持原门槛，不自造容差 |
| 水沙耦合 | 邻接成对q的5项界；同accepted θ；分段介质；LAND只转沙、W不消失；内部换列／床动也守容量 | §4.1–4.4 | 固定子步次序／有界事件／容量准入／完整影子patch／错误码，不调用未支持严格核绕过 |
| 注入API | `SAND_WATER_SOURCE_V1`；SourceRequest/Receipt；SAND/WATER分开，幂等、正成功身份、局部容量 | §5 | typed接口／有界去重和请求队列、回执字段、source与step回滚边界、时钟dt窗口版本 |
| 触摸生命周期 | 主指针、几何代／输入代；切kind立即stop，新DOWN首采样0；UP/Cancel/后台/reset失效不追赶 | §5 | controller/page/adapter所有权、停止屏障和队列代隔离，rate单位模型体积/s |
| 同帧渲染 | `CLEAR_WATER_SCENE_V1`／`CLEAR_WATER_OPTICS_V1`；S/W/η/pool/source commit同FrameToken；线性预乘alpha透射 | §6 | ArkTS/Native packet布局／校验／config、shader pass、水面与沙遮挡、完整scene LastGood/swap/恢复 |
| 阶段／预算 | A库存事务→B耦合→C明水，O(N+K+L)阶段路径、水O(N)、定容资源，无热扩容 | §7 | 逐文件hunk、真实共享修改依赖／总内存峰值、最小developer拆分，不以真机为开发关闭门 |

已有接口版本是设计合同，并非宣称对应类型／函数已经实现。T107仍负责具体字段offset、错误码、模块分界、时间窗口与配置布局。其工程方案和T108基线形成开发前置；设计到件本身不授权直接生产实施。

完整严格合法态route与compute事务迁移是否支持，仍按其各自正式工程／实现证据判断，不能因本路线“另一个mode”绕H/EPS/质量／预算检查。共享文件已变化时按当前接口接工程，不回退CPU索引、GPU或渲染线进行中修改。

## 4. T108可消费的测试合同与效果后续

T108任务：`6adf343de4b06ee35743c76b`，tester，不使用真机。既有输出约定为 `docs/test-report-single-sand-clear-water-baseline-v1.md`、test-cases编号条目及独立执行器；本交接不修改这些tester所有文件。

正式18组断言逐字以T106 §8为准，本次只给定位映射：

| 合同组 | 覆盖内容 | 先行基线解释 |
|---|---|---|
| CW-01–03 | 空run、水源实际库存、局部准入／满列／P预留 | 缺新mode/API列NOT_IMPLEMENTED，不把旧加水当通过 |
| CW-04–06 | 单沙出生／介质分段／LAND同ID、W不丢失、床挤水 | 现有dry粒子成功不推出目标7湿段／排水通过 |
| CW-07–10 | 通量上界、守恒／能量、隔床／墙、倾角、内部转移容量 | 旧全局levelWater不是新邻接核oracle，可依算法实数式及原审计定义夹具 |
| CW-11–15 | 非法输入／资源耗尽／幂等、切源／生命周期、source与step原子回滚 | 保留拒绝身份及成功源账，未实现预测试RED登记结束本轮归因 |
| CW-16–17 | 同publication水沙／透射／遮挡、FAULT或完整STALE、无Canvas | 本卡可先测packet/错误身份，设备视觉效果另卡，不以离线测试假签真机 |
| CW-18 | 长时定容资源／源序号／复杂度／峰值性能 | 沿既有性能合同，未实现时明确未执行，不靠平均FPS或旧报告冒通过 |

所有新数值效果参数继续 `PENDING_CALIBRATION`：κ、水源／沙源速率及单次上限、dragWet/settlingSpeed、消光tint/高光和新增分段误差（若原线未覆盖）。测试可输出测量值与候选configRev，不能给未批准值贴既有门槛标签。适用原DRIFT_LIMIT=0.001、相关逐步守恒1e-6及原归一化／比较符／EPS/H／固体预算保留，T107负责准确映射，designer本次不改阈值。

effector后续需求仍为T106 §8六组观察：透水辨床／粒子、入水减速／积床、真实加水抬面／低头扩散、倾角方向、切源停止、全干／浅深水／近满／隔床对比。固定scene、参数revision和同帧源量／水深；未派未执行，不声明效果达标，真机卡由manager另建并串行排队。

## 5. 证据时点与完成边界

本轮验证的是文件存在／可读取、Markdown结构／引用、18个CW编号齐全、接口版本及三线定位、SHA/长度和存档范围；未重跑原测试、未执行业务或真机。T106 evidence是设计算术示例，不是产品测试结果。

读取历史13个输入SHA并与本轮当前文件比较，其中6项已变化：goals.md、PROJECT_GOALS.md、WaterSandField.ets、P011ParticleHot.ets、P013SceneTypes.ets、scene_compositor.cpp。历史SHA原样保留，当前SHA和MATCH/CHANGED在T122 manifest单列。此差异说明盘点不能当最新源码已验证；**不是方案文件缺失或T122交付阻塞**。T107工程阶段本就应盘点当前hunk／兼容接口；若发现实质冲突／设计缺口按既有architect流程分流，不在本交接凭SHA差异认定缺陷或要求重验T106。

仅本任务状态成功改“结束待验收”并在T122／原T106回执报告真实commit、路径、接口和CW合同。T107/T108的方案文件前置已经提供，可由manager按通知推进；本session不改其行／标签／队列、不创建或执行后续卡。完成的是交接，目标7实现与产品指标仍待开发／独立双测，无manager新增批准门。
