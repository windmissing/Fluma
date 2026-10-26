# T93 材质整数补证工程分流 v1

日期：2026-10-26。architect。原卡 `6adf1e5ee4b0127832bb7274`。不使用真机。消费T88 f683edc报告、两个debug执行器/原JSON、T87 c2c35e5与T77/T84；只读，不运行GPU或修改测试。

## 1. 窄结论与设计路由

`test/reports/t88/t88-stage-probe.json`的debug hash `897954e22d96ae53aba1da38d52fcb2bc9716efd11be494cb3f799481e293f68`，(88,78)整数为baseB173、grainB102、strataB127、k1，25邻域已留。接受为**该debug shader执行的整数直证**，不是低精度色值反推。

从debug/生产frag对照，baseByte分支表达式及D插值、k判定、palette/gain链一致，baseByte计算不依赖后续grain/strata/Q/light。该点GL中心88.5/78.5、box(16,16,128,256)，D采列72与73，L带palette blue=150/255、spark开启。本次核读现存T55同hashfixture `32870e766b0d501e17d0093edd00336969354dd2ebae141183b9233d3cccc353`：row2.g[72]=Float32(1.3)、[73]=1，线性插值gain=1.149999976158142，与T77计算一致。

因此**局部数值表示/离散round设计问题已足以转designer研究**：冻结F64同输入算150×gain=172.4999964237→172，而同结构GPU debug得到173。不是凭最终色相似才推断。仍需补本次实际运行fixture/bin/上传uniform位型manifest；现存同hash文件的核读不冒充T88执行时完整身份归档。设计任务可以开始，不把这项补证变成重新验收T88的门。

不能接受T88“该点1.956残差已定性完成”“所有输入/索引/资产错误已排除”的扩大主张：debug attachment0是**RGBA8**（MRT第二路RGBA32UI），虽创建了sRGB pbuffer，实际draw目标并不是sRGB pbuffer；238/181/85对247/219/157来自不同目标链，不能先归咎uniform。连续量、完整uniform/texel索引及同目标生产对照尚缺，无法确认该分叉解释全部生产最终误差。RGBA32UI是整数目标，不是float target，complete也不等于连续量已采。

## 2. 给designer的可执行局部revision任务书

不使用真机。不允许修改 `D:\hmprojects\HelloWorld\docs\后续工作清单.md`，仅自身行状态有限例外；不改滴答同名清单，共享代码不回退。所有渲染仅新XComponent+shader含背景前景，无Canvas上屏/回退，失败FAULT或标明STALE上一有效新管线帧。

输入本稿§1、T77数值反例和T88整数raw。只解决编码域palette×gain→clamp248→round的确定表示：比较保持冻结数学含义的确定离散准备与显式GPU数值表示方案，列误差/接口/资源代价。必须覆盖M带per-fragment fH混色、连续profile插值、half整数邻域、0/248边界、OFF/ON/B/C/spark/镜像及真实七版本关系；不能把列中心byte替代整像素变化。明确CPU主参考、F32影子和新数学revision之间关系，并给可复核等价/差异证明。

禁止epsilon、改gain/palette/材质配方、模糊纹理、全平台gamma/OETF、把F32影子静默改成主参考或放宽阈值。若方案会改变签收表示，显式版本化并交原流程裁决，architect再落type/preparation/shader工程接口，developer不提前盲修。产品numeric仍MEASURED_NOT_CALIBRATED，设计交付不等于颜色PASS。

## 3. Part A半透明证据范围与采集修订

两FBO、六alpha观测可作线性预乘over候选正证据，但本次代码/JSON没有冻结产品合格线，`±1`只是报告观测，不能新增容差签PASS。canonical的expectedLinear255与执行器原raw存在不同模型/数值：如a=.25 G raw期望121.12而canonical121.12、观测122，报告称按量化dst修正后逐字节相等，但不能只凭叙述覆盖公式；必须从实际上传Float32 front与量化后的dst重新计算。sRGB目标dst须先对**实际backRef编码字节**EOTF后参与线性over，不能用linear目标的64/128/191直接当其dst。raw sRGB行未留back/frontRef，误差分解仍缺。

此外每轮back绘制前需要核对实际blend/scissor状态，dither/实际attachment/query/逐阶段errors未完整归档；单点half边界不证明截断或round模式。接受合成原始数据，不认本宿主全部半透明已验证、更不外推scene雾/高光/边框与设备呈现。

## 4. T87采集纠正逐项到件表

| T87要求 | 本次T88实际状态 | 路由 |
|---|---|---|
| EGL1.5核心SRGB T2创建＋实际query | Part B确实调用带SRGB属性创建pbuffer且CHECK成功，反证“缺扩展一定创建不了”；但未实际query/config/attachment完整核对 | 补实际状态；不重写T80历史SKIP为本次PASS，不认静默降级。 |
| 正确GL扩展来源/精确token/write-control | Part A有glGetString(GL_EXTENSIONS)写gl.txt，比T80来源正确；未完整支持/状态与每target绑定记录 | 部分到件，继续补状态，不盲enable。 |
| 正确default GL_BACK encoding/read-format分列 | 两执行器主要画FBO，未补T80 default查询错误 | 未交，tester精确续证。 |
| 正确第六常量与非null上传位型 | 不包含T87常量矩阵修正 | 未交；正确EOTF/OETF独立fixture，旧错公式保留历史。 |
| 四target同context/写读三角对照 | Part A两FBO共context，Part B另一执行；非四目标完整对照 | 未交，不称已排除写/读差异。 |
| errors不覆盖、每常量raw与自动manifest | 现有JSON/debug hash有；未见完整per-stage/run/target/raw/源码exe/DLL/资产manifest闭合 | 未交完整；整数JSON不是全套RGBA/float原始文件。 |
| 连续量及同目标生产/debug对照 | 明确未采gain/Rpre/Lmat/ToneMap；debug为RGBA8目标 | 未交，按§5补，不从85与157反推数学失败。 |

T88 PartA/B执行完成保持登记，不重开、不重复整套矩阵。后续由manager以这些具体缺项派tester，结果回architect；不让tester承担整体归因。

## 5. 精确tester续证与工程落点

测试专用：`t88_stage_probe.cpp`或独立V2，尽量从生产拼装frag加入独立整数/float输出，归档生产与debug两hash及逐行diff；同context、同实际uniform上传、同目标format/状态分别draw。先生产MaterialPass对同fixture重现归档最终色，再debug同目标（SRGB8_ALPHA8 attachment0＋integer attachment）记录最终色和整数，分别验证instrumentation有没有改变base边界结果。不能要求硬凑旧图一致：如不同则如实留MISMATCH，查版本/上传/目标/优化差异。

捕获palette字节/实际uniform Float32位型、D输入相邻texel与插值gain/fH、精确grain/strata坐标、baseByte及各Q/Rpre/Lmat/ToneMap。连续值用实际支持并complete的float目标按阶段分pass；若不可得如实MISSING。原F64主测、F32逐操作诊断、真实GPU值与最终sRGB字节分列。补§4未交T87项，仅针对缺失查询/目标，不重复已交有效观测。

半透明补测留实际back/front编码值、decode后dst、上传alpha/预乘source位型、blend方程/因子、dither生产/disabled分支；精确计算比较表，旧raw不改。每target/run独立合法JSON/raw/manifest，明确RGBA8和SRGB8目标，不因pbuffer有SRGB属性把离屏FBO改名。

若证实工程uniform/输入不符，architect先修订最小material preparation/上传或测试采集hunk再分发，生产未证缺陷不返修。若只证表示分叉，沿§2设计revision继续。完整scene设备A/B/C/性能归T57，受T92同页稳定性前置；不把离屏补证等同设备通过。

本轮无业务/shader/测试/配方/阈值修改，无测试重跑/真机；仅新归因文档与自身交付状态，原卡报告设计可启动及采集剩余缺项。
