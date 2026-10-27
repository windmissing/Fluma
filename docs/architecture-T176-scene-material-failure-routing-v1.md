# T176：T95 SCENE_MATERIAL_DATA 首次归因与首错取证合同 V1

2026-10-27；architect；原卡 `6ae06c77e4b0e6ce82e0821d`。不使用真机、不重跑测试、不修改业务/测试或阈值。消费T95 `5234226`正式报告、原dump/截图、T56场景合同、T166/T167边界及现行源代码。**路径归因与工程规格部分交付；同帧材料原bits及内部拒绝原因缺失，唯一数值根因未定位，本人状态“看评论”。**

## 1. 已经能裁定的失败层级

`SCENE_MATERIAL_DATA` 的唯一源码生产点是 `P013ScenePreparation.pack` 捕获 `P013MaterialDataPreparation.pack` 抛出的任意异常；它不是GL上传失败码，也不是shader编译日志。完整路径为：

`EmptySandDemo.tick → copyPublishedRenderStateInto → SceneController.drawNewPipelineFrame → ScenePreparation.pack → MaterialDataPreparation.pack → [throw] → sourceFailure='SCENE_MATERIAL_DATA' → native scene桥 → SceneCompositor.Reject(code100)`。

页面实际传 `pipeline=0`，走BASIC_FULLSCENE_V1，native `SceneConfiguration.Material()` 为false；不调用static grain/strata bundle stage，也不走P1 base strip。材质**CPU数据准备仍无条件执行**，基础沙/水/层剖面也依赖它，不能为了规避故障跳过pack、改成仅画粒子。T95报告称“T149 V2六图资产/上传链接线问题”没有代码证据：T149是FIELD_COLUMN原值诊断；六图相关历史是T48/T54/T55。本故障在当前candidate的 `SceneGlGpu.Begin/Upload/Layer/Present` 前被拒绝；prepare EGL/surface及配置绑定可能已执行，不把“尚未提交本candidate”扩大成没有任何GL调用。

code100本身还可表示scene参数/源域/typed range/particle等拒绝，但本轮**reason明确为SCENE_MATERIAL_DATA**，将范围收窄到上述ArkTS捕获链。不能从code100数字推具体坏列、waterTop、纹理、六图或shader。

## 2. 原始日志核对与身份边界

逐行只读解析 `test/artifacts/t95-demo-unblock-retest-20261027`，按PID+run分组；派生摘录/hash见 `docs/analysis-T176-scene-material-evidence-v1.json`，不覆写旧raw。

| 臂 | PID / run | 同PID present采集值 | 首个code100（demo日志身份） |
| --- | --- | --- | --- |
| R1 | 24446 / p011-run-1793089653277-3 | code0=244、100=176、-1 NOT_READY=1 | R1-full.txt:938；16:27:52.002，commit492/page frame243 |
| R2 | 24752 / p011-run-1793089732766-3 | code0=268、100=74、-1 NOT_READY=1 | R2-full.txt:1452；16:29:18.007，commit549/page frame270 |

R1原文件开头夹有旧PID23437的4条成功present，报告248个code0是跨PID汇总；R2开头也有旧PID24446尾部sensor。原100的176+74=250在各自PID仍成立，但只是**已采到日志条数**，非完整尝试分母。日志scene条数R1为187、R2为113，不能强行与demo present条数一一相减推漏帧/重复错误。R0/R3各有一条冷启NOT_READY，后续无注入样本为code0；“全程code0全绿”应限定ready以后。输入与注入后出现错误的相关性成立，不能推出所有注入必失败或唯一触发输入。

第一条保留的R1 native scene故障日志在:939，16:27:52.095：request=`...:2:494:243`，lastGood=`...:2:490:241`，state=FAULT、swap=false、staleSwap=false；R2在:1441，16:29:17.701：request=`...:2:551:270`、lastGood=`...:2:547:268`，同为FAULT/双swap=false。日志分通道缓冲/筛选且frame计数不同，不能仅按临近行、墙钟或相同frame数字硬配同一帧。

页面serial与SceneController.serial是独立计数；页面首个NOT_READY可能不推进scene serial。补证同时记录 `pageFrameSerial/context.frameSerial` 与 `sceneFrameId/frameToken`，按run/stateEpoch/physicsCommitId+sceneFrameToken关联，而不是假设固定偏移量。现存R1同frame数字243可对应demo commit492与native commit494，不能据此直接报出版混帧；尚无同一token对应原材料数组的正证据。

`R1-t45.jpeg`确见故障文字和若干粒子，native回执明确FAULT，**不能据可见粒子认定当帧局部成功或背景层当帧漏画**。可能看到此前保留的surface内容；lastGood token非空也不等成功重呈。Reject尝试STALE的条件包括同run/context/surface/geometry/continuity/尺寸/box/mirror和真实Present(true)+Swap；原raw没有逐项兼容判定或重呈错误原码，无法唯一解释为什么FAULT而非STALE。

首次R1故障附近还出现 `model.stop reason=GEOMETRY_CHANGED`，desiredConfig后续2→5；页面status文字与layoutWeight容器可能影响布局，但现证据不足以证明故障文字引起resize。它可能影响重呈兼容/触摸停止，是单独待测的显示恢复条件，不能当材料首错原因。不要为了维持旧帧绕geometry检查。

### 2.1 包与源码身份

T95声明s150签包SHA=`f28ec28e639e2d5b7ccec265b9023890664531819b8b98094ca8348c7c3bd726`、源a419f5a、树指纹aee7374caa8ff7cb。本轮在常见entry/build及.codex/local-test归档位置未定位到s150包/逐文件manifest，**未独立核签包到源码对应**；不把声明当已复算身份。当前ScenePreparation/MaterialDataPreparation/FrameAdapter/SceneController、native桥和compositor相对a419f5a无diff，足以解释这条候选源码路径，不能独自证明历史HAP含全部相同依赖。先请保留已有包/签包回执/构建manifest，不重建一个新包冒历史证据。

## 3. material.pack 的确切拒绝分支

外层catch目前抹掉错误名/首列/原值；因此下表各分支都须保留到精准首错，不能现在只挑某个分支修复。

| 内部原因 | 原始谓词/阶段 | 尚缺证据 |
| --- | --- | --- |
| MATERIAL_DATA_LAYOUT | boxW/H非有限或<=0；mh/ml、warpH/L/M、spark长度非128 | 实际box、所有数组类型/长度/byteOffset/byteLength、对应geometry |
| MATERIAL_DATA_VALUE | 每列warpH/L/M/spark/mh/ml任一非有限，或spark/mh/ml<0 | 第一命中列/字段/比较mask及原bits；不能用全局deposited或pM代替 |
| MATERIAL_DATA_PROFILE | display=P013FrameAdapter.pack返回false | 下列source与F32阶段第一拒绝谓词 |
| MATERIAL_FLOAT32_RANGE | 最终1536个staging分量存在非有限F32 | row/col/channel、F64计算中间量、转换前后bits及所有scale |
| 非预期异常 | 类型/数组访问、set异常或其它运行时异常，同样被catch统一吞掉 | 已知enum以外保留name/message/阶段及缺失原因，不伪归上述值域错误 |

FrameAdapter在source阶段要求cols128/capacity有限正、sandTop/waterTop/bh/bl长度128；逐列所有top/water/bottom/mix有限，严格 `0<=bottom<=mix<=top<=water<=capacity`，mix=bh+bl。随后四遍三点平滑（原Float64顺序）、F32转换再核有限/同层序及上界。后一检查不可因source初检通过就省略；最终MaterialData还用mh/ml计算mix成分、37点权重坡度及warp/spark，可能独立出错。

`MaterialDataPreparation.staging`与其已发布data分开，只有所有检查过才set到data；ScenePreparation在material成功前已更新部分motion/warp/candidate粒子slot，失败时不得把这些当前局部数组与旧data作为成功帧提交。native当前有sourceFailure拒绝路径，继续保留；不把“传给native旧packet以取失败回执”误记成功上传当前材料。

### 3.1 已发现的源级边界不一致，仍不是设备数值根因

物理dry audit允许waterTop及材料分量在[-EPS,0)边界，并要求dry waterTop与S+particleMass差在EPS内；T166只把Hot的waterTop对齐<-EPS，其它Hot材料分量仍严格非负。renderer既没有EPS规则也没有dry边界转换，仍严格top<=water。对于物理允许的小残差/派生高度舍入差，该renderer域可能更窄。

还存在独立加法结合顺序：物理updateSandTop的S是 `(((bh+bl)+mh)+ml)`，dry waterTop再依原顺序加suspH/suspL/pool列质量；copyStateTo发布的sandTop却是 `((bh+mh)+(bl+ml))`。浮点结合律不成立，mixed情况下即使各分量合法也可能top与water排序不同。T95为干场重沙输入，不能直接用一个人工mixed反例替代本设备原因。原copyStateTo还会先updateSandTop；取证不得额外再调用copyStateTo/snapshot触发派生重写后冒“首错原值”。应读取已经交付给pack的publication。

T167的残差C1/C2等是离线证据；T95没有逐列waterTop/材料/cache原bits或内层PROFILE错误。报告所谓设备保持`-2^-55`原bits未由T95原dump证明，且T167多组残差并非一个唯一值。**不允许据此给renderer加EPS、clamp、把water强抬至top、修改S结合顺序或写回物理。** 当前只裁定“有真实CPU准备拒绝、物理/渲染域存在候选不一致”；具体列/域/计算根因关键缺证。

## 4. 先修工程：P013_SCENE_MATERIAL_REJECTION_V1 有界旁路诊断

developer不使用真机。模块所有权为 `render/P013ScenePreparation.ets`、`render/preparation/P013MaterialDataPreparation.ets`、`render/P013FrameAdapter.ets`、`render/P013SceneController.ets` 及必要native `scene_compositor/napi_bridge`诊断字段；仅需要时按hunk协调EmptySandDemo的诊断导出。该阶段**不修数值、不改材质配方/上传/包协议判据**，保留现有sourceFailure总类与code100，版本化新增诊断sidecar，不改endpoint/particles账本契约。

建议只读接口 `getLastMaterialRejection(): P013MaterialRejectionV1|null` 与冷定容诊断workspace（实际类型名可按项目风格），包含：

- run/stateEpoch/physicsCommitId/publicationKind、field.physicsCommitId、ledgerCommitKey、route/medium，pageFrame/contextFrame/sceneFrameId/frameToken；session/context/surface/geometry/continuity、源/目标viewport、boxW/H及scale、pipeline/config binding/digest与实际source/build身份。
- 首错outerStage、innerReason、sourceValidation或F32Validation阶段、firstColumn/row/channel、predicateMask、原数组类型/长度/offset/byteLength及binding identity；预期失败枚举精确映射，不吞未知异常。
- 对该列保留waterTop、bh/bl/mh/ml/suspH/L、sandTop/hs/hl及pool colMassH/L原buffer bits、源sum的每次RN中间值与bits；非有限原数组从buffer读，不用JSON number往返损坏NaN payload。derived值标DERIVED，不能伪称原存储bits；负零保留。
- 首个失败publication的10组F64场数组及2组池列cache原字节（128列共12288B）固定封存；必要F32 source/profile/staging有效前缀、warp/spark/motion及geometry各自标阶段/有效性。若拿不到完整当前数据则NOT_CAPTURED；旧staging无效区不得标当前。pool完整回放未取则明确false，只取cache不能声称精确物理重演。

保留“本run最早成功摘要+首次失败快照+固定最近8条摘要”，重复同指纹只计count，不逐帧全dump/全数组JSON。buffers/byte views冷创建，界定捕获数量/bytes上限，首错拷贝发生在**原失败位置**，不再跑pack/step/audit/更新派生场，也不增加guard/排序次数。首次判定在原if/return旁写固定数值信息；异常catch只锁存原原因，不因为诊断失败覆盖物理/渲染首错。hash在捕获冻结后导出阶段算，计时披露诊断开销；输出version/hash/字节长/dropped/missingReason，原source数组前后hash不变。

以同一frameToken贯通内部原因→sourceFailure→native receipt；若native参数校验先于sourceFailure失败，两个stage各自存，不用sourceFailure覆盖实际firstFailure。冷取证只读借用publication，在下个tick复用前拷到独立定容槽，异步导出不读会被改写的typed-array引用。不要为记录再调用live field.snapshot，这会调用updateSandTop且使证据脱离被拒包。

### 4.1 STALE/FAULT 与包身份补充

native Reject旁路追加兼容失败mask（run/context/surface/geometry/continuity/size/box/mirror/lastGood-valid）、replayAttempted、presentOldStatus/swapOldStatus、相关GL/EGL首次原码及lastGood源身份；未执行阶段null+NOT_RUN。**主首错SCENE_MATERIAL_DATA与重呈失败分开**，不以后一个错误覆盖前一个。保持兼容才可replay、完整scene先swap再更新LastGood/租约的T56合同；禁局部粒子帧假成功、禁geometry不兼容旧帧冒CURRENT。页面继续FAULT或标明STALE旧有效新管线帧，前景/背景均XComponent+shader。

源hash至少覆盖页面→publication copy→两个preparation/adapter→scene桥/compositor及依赖；保留原s150包身份与新诊断包身份区别。主工程安装覆盖隔离GPU包/设备窗口由执行卡记录，不读/改签名凭据、不借用另一个run/另一包的bits填旧首错。

## 5. 最小独立测试与设备补证，回T176分流

| 接力 | 可执行范围与完成证据 |
| --- | --- |
| developer（不使用真机） | 仅实施§4旁路首错/同帧身份/STALE原因诊断；保留全部拒绝谓词、例外、数值算法和T56原子性。编译/不涉及真机自测交付即可，不等设备结论关闭开发卡 |
| 独立tester（不使用真机） | 独立构造LAYOUT、每类VALUE、PROFILE每个层序/source/F32拒绝和最终F32 overflow、非预期throw；断言准确首错及bits/source不写/首错不覆/捕获容量/新run清理、P0/P1及wet/dry均保留原结果。注入仅测试夹具，非法源仍拒绝；NAPI真实分支与桩明确区分。复用T56 scene/gpu/bridge及T91必要边界，按受影响范围窄回归，不照开发脚本改名 |
| effector（真机、串行） | 有效诊断包+独立无阻断交付后只做一条空场控制与一条同T95拖动注入至首个SCENE_MATERIAL_DATA的短复现；启动立即锁PID/run/package/source，先取同frameToken首错sidecar/原bytes及完整receipt。能复现就停宽矩阵，必要时R2只作一次确认；若未复现如实留未触发/缺证，不能拿离线输入冒设备原因 |
| 回architect/T176 | 根据innerReason+bits+来源mask确定修复点，工程问题先升工程规格再developer，设计域/新投影交designer；原用例不合理才带证据交manager，不预先放宽 |

设备采集必须stdout/stderr/退出码/实际文件字节/hash归档，同run/substep/commit和frameToken关联，必要的fence/close按真实请求而非抓第一个历史CLOSED。当前run-console及analyze_t95.py未见于指定归档，R1 ledger/admissions亦未见；优先补既存文件/导出结果，不伪称已读完整账本。脚本Dump筛选会漏部分scene CURRENT和系统原日志，补证保存原日志，再派生过滤；PID行尾查活/空freeze文件只说明已检到范围，无完整会话外推。

后续实际修复路由预先限定：

- 若PROFILE命中合法dry residual或发布结合顺序差：先核真实物理合法域和渲染剖面语义，designer裁定是否需**仅显示副本**的有界投影/统一派生表示及上界/源不写证明，再architect落pack/native对应版本。不能借T166既有EPS授权擅加renderer EPS/clamp。
- 若VALUE/LAYOUT或错误发布身份/绑定：工程修复正确producer/准备顺序/范围/生命周期，保持非法值拒绝；不把坏数据钳进合法范围。
- 若source合法而F32输出溢出：以实际scale/row表达式分流数值表示设计或工程单位错误；不能去掉finite检查。
- 若只是geometry/layout导致STALE不可用：独立修输入同框及稳定viewport/显式失效与恢复，不能称材料首错已修；若确因状态文案改变布局，UI尺寸方案须有实测bounds证据。
- 若非预期异常/容量问题：按真实exception/source身份处理工程；当前已实现pack失败不是NOT_IMPLEMENTED产品预测试。诊断功能尚未实现的预期RED可登记NOT_IMPLEMENTED，不冒产品PASS。

所有下游prompt写“不允许修改 `D:\hmprojects\HelloWorld\docs\后续工作清单.md`”（仅本人状态有限例外），不更新滴答同名清单，不回退共享修改。离线卡明确不使用真机；设备卡真机标签、有效包与串行前置；渲染含前景背景仅新XComponent+shader，禁止Canvas上屏/失败回退，失败FAULT或显式STALE。GPU目标6最高优先仍并行接力，manager登记调度、不新增验收门。

## 6. T95物理/卡死结论的保留边界

R1已采摘要：commit607→727→847，sourceEpoch均7；depositedMass 0→0→1.1547005383792515，particleMass 4.354428215026855→4.354428215026857→3.199727676647607，总量保持约4.354428215026859。它支持本dry run有落床总量增长和粒子总量下降，不抹掉T95的进展；但metricCommitKey均null，未读到完整ledger/particle事件，不据总量推逐粒FLYING→CONTACT_WAIT→LAND或完整L1收敛：最后一个约10秒窗口后的趋势未采齐。R2已采末点dep仍0，不称两臂都LAND通过。

R1/R2原dump未见physics-failed/first_code5_trace，冻结文件0B、能读到同run物理commit继续前进，支持**本干场有限窗口未复发先前code5/检索到的freeze**。R2采样physicsTimeMs可达466，不能将无冻结宣成帧时/交互性能PASS。T116/T129双场卡死的真实负载与独立稳定性前置不因此关闭；T177真实手持缺口仍按其卡接续。

本轮核原dump、路径/谓词、截图与源差异，已确定CPU准备拒绝及诊断丢因工程缺口，但没有取得拒绝同帧材料bits/内部原因，故不定唯一根因、不授权数值修复。工程取证规格交付，T176保留看评论等待精确补证后续判；未改entry/test、未运行测试或使用真机。
