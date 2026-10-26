# T89 T62旧入口断言与scene合同归因 v1

日期：2026-10-26。architect。原卡 `6adf041ee4b03273ad805146`。不使用真机。状态：**本次FAIL定位为历史静态入口断言失配；新scene功能等价仍需版本化测试证据**。只读代码/报告，不修业务或用例。

## 1. 失败范围

消费 `docs/developer-T71-touch-source.md`、`test/reports/t71/render-repair.txt`与regression.json、T62开发报告和 `test/run_p062_render_repair_units.cjs`；对照T68纯新管线合同、T83输入工程合同及当前T56 scene候选。T71的12套exit0是另外12个套件，不包含本次T62失败套件，不能汇总为全回归PASS。

T62套件前五项运行结果PASS；第六项在首个 `s.includes('submitDualFieldSurfacePx')` 即FAIL。其后的vp2px、onAreaChange/onTouch字符串位置、无border检查**本次都未执行**，不能写成也PASS。此失败是页面不再含旧方法名，不是已运行scene后拒绝几何或错误投影的反例。

T71用户已完成，不重开、不添加开发验收门。T56继续结构；本次没有可证物理/几何业务回归，也不能把源码审阅称为新scene已验证通过。

## 2. 历史目的与现行合同映射

| T62原目的 | 当前候选落点/读取到的工程证据 | 证明边界 |
|---|---|---|
| 首次surface尺寸不能依赖caller猜测 | SceneController.drawNewPipelineFrame→native.prepareDualFieldSurface→sizeSource=EGL_QUERY_SURFACE；getState复核surface/context epoch，full-box使用真实prepared px | 调用链可见；不等于新scene对零尺寸/NaN/race全部已测。 |
| 单位与内容区一致，radius有DPI身份 | 页面同父Stack量W/H；context用该vp；drawNewPipelineFrame传vp2px(1)，scene将box换px；独立input Stack覆盖同内容区 | T83授权从XComponent本体触摸迁到无绘制节点；旧“必须同XComponent挂onTouch”的字面要求已不适用。 |
| 合法Float64粒子不因Float32越列拒绘 | ScenePreparation调用既有DualFieldAdapter.packInto(...P013_COLUMN_PROJECTION,false)，投影前严格校验权威源；false只避免重复旧profile写入 | 保留算法入口；旧五项只覆盖旧controller/adapter，尚不能替scene完整传参/bridge签PASS。 |
| 非法粒子仍拒绝，物理不写回 | adapter先检查原值/质量/ID/state等；native scene桥再检查有限坐标、列域、tag/ID/重复；sourceFailure传给scene Reject | 有拒绝路径，不证明每项均已走新scene回归；失败可按T68重呈STALE，不能要求所有拒绝都完全无swap。 |
| prepare/race失败不冒称当前成功 | controller准备失败/epoch变化返回null，native EnsureCurrent严格核px/epoch；scene current/STALE/FAULT分离 | 旧lastPresentedCommit断言须映射requested/rendered/lastGood，不能把历史lastGood清零当唯一正确行为。 |

当前页面、SceneController/Preparation和native scene部分含并行T56候选，不能用T71提交0b1f7e3独自代表全闭合源码；T71报告已披露精选输入hunk、共享候选构建。后续测试须固定完整T56交付commit，若dirty则归档精确patch与全依赖hash；不以当前工作区推历史设备结果。

## 3. 工程合同补充：P013_SCENE_GEOMETRY_REGRESSION_V1

本稿补充T68/T83及 `architecture-P013-dualfield-particle-shader-adapter-v1.md`，保留T62旧入口及原测试历史身份：

1. 新产品场景入口是drawNewPipelineFrame，仍复用显式prepare＋真实surface px＋epoch复核。方法名可迁移，尺寸来源、严格域检查、列保持投影和源不可变不放宽。不为通过字符串断言在页面重新调用旧局部床/粒子入口。
2. 输入节点和XComponent可为同父同框兄弟；以内容vp几何/epoch与显式inverse map验证，不要求onTouch属于XComponent本体。测量前ready=false；DPI只能在vp→px接口转换，粒子投影不得修非法权威状态。
3. prepare/init/epoch失败不能提交当前candidate。已存在兼容LastGood时是否可重呈须按T68身份合同区分staleSwap与requestedSwap；失败后的requested current=false，不能只看总swap计数或旧lastPresentedCommit是否非零。
4. 非法源导致sourceFailure必须经scene失败回执到上层，不能将保留的旧slot冒充当前合法源；失败不修改物理publication。LastGood只有完整scene成功swap后更新。所有背景/前景/粒子仅新XComponent+shader，禁止Canvas上屏/失败回退；故障FAULT或显式STALE旧有效新管线帧。

这次未确认需要developer返修。若后续新scene回归发现违约，architect按具体反例先修订文档，最小hunk为SceneController准备/epoch/px、ScenePreparation pack传参或native DrawNewPipelineFrame拒绝分支；不重写投影/物理，也不恢复旧Canvas。回归证据未到不得将候选代码判定为已修复产品。

## 4. 用例争议交manager裁决，tester版本化更新

提请manager裁决：原第六项以`submitDualFieldSurfacePx`及XComponent局部字符串布局充当产品入口合同，在T56/T68/T83已授权迁移后不再合理；保留旧脚本/原FAIL，对新scene新增版本化功能测试，旧第六项应明确适用历史T62基线或标SUPERSEDED_BY_AUTHORIZED_SCENE_MIGRATION，**不删除FAIL或原地改期望制造绿灯**。本稿提供裁决材料，不代manager改测试。

获裁决后tester复用现有 `run_t56_scene_units.cjs`等测试资产补明确scene版，而不是再造同义字符串断言。T56尚未交付的功能若预测试RED，标NOT_IMPLEMENTED并结束该项归因；本次missing旧名称则是用例合同失配，不混入未实现RED。必要开发者离线联测与独立tester证据分开管理。

## 5. 精确测试需求与责任

不使用真机。不允许修改 `D:\hmprojects\HelloWorld\docs\后续工作清单.md`，仅自身状态有限例外；不更新滴答同名清单；共享代码不撤销他人改动。所有渲染只走XComponent+shader含背景前景，无Canvas回退，失败FAULT或标明STALE旧有效新管线帧。

| 版本化用例 | 断言/证据（tester负责执行，非整体归因） |
|---|---|
| SCENE-GEO-01 冷启动 | 初始width/height=0，真实prepare回800×480且epoch变更；小数vp调用新controller，记录prepare→getState复核→draw，packet为真实整数px，同publication。非法/零prepared尺寸分别拒绝；native/Ark桩边界明确。 |
| SCENE-GEO-02 生命周期竞争 | prepare失败、surface/context race、后台/恢复/resize；当前draw不成功，active/LastGood不被假新帧替换；有兼容旧帧与无旧帧分别检查STALE/FAULT身份和两类swap。 |
| SCENE-GEO-03 Float32保护 | 同T61右列临界合法值经过scene真实Preparation→bridge；同列投影、源字节不变；非法x/y、NaN、质量、state/phase、ID/重复、publication不一致各拒绝，不提交被污染slot。RAW旧入口原反例保留。 |
| SCENE-GEO-04 同框与密度 | 实际adapter中心/右界/列界和geometryEpoch联测；准备前无触点准入；输入同vp与scene px归一化对应；1及非整数DPI下radius/位置转换。不是通过查某个字符串推运行时布局。 |
| SCENE-GEO-05 完整scene失败 | sourceFailure、任一层失败、swap失败分别记录requested/rendered/lastGood与generation；不允许局部床/粒子帧冒充全场，前景背景均shader。 |

沿原T62五项与旧RAW/非法边界保护继续回归，冻结判据不动。现有T71 11/11和T72被动诊断等报告保持各自范围，不能替scene版测试。测试完成后结果回architect；工程失败再落最小修复，采集/用例问题仍各归其责。

平台真实命中、surface/DPI/几何、同commit可见粒子与完整scene在T82/T57等真机卡独立串行验证，不能用host桩替代；也不将设备验证变成T71关闭前置。

## 6. 交付结论

本次旧入口字符串FAIL属于历史用例失配，待manager裁决测试版本化；新scene功能保护在源码层有对应落点，但缺专属功能回归证据，仍UNPROVEN。T89归因/工程文档交付完成，未改业务/用例/阈值、未重跑测试、未使用真机；仅自身行结束待验收，原卡报告。
