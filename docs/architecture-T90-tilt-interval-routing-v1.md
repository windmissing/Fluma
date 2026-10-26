# T90 规划机制到件与设备code5分流 v1

日期：2026-10-26。architect。原卡 `6adf0846e4b06ee3573f40a3`。不使用真机。消费T86 bfe45b4报告/三个raw JSON/执行器、T79/T85及T82 09e3800设备报告。只读，不重跑测试、不改业务/用例。

## 1. 离线机制已闭合到哪一层

step73两个实际候选为REQUESTED θ0与OLD θ0，均PLANS_READY；CLAMPED因REQUESTED_INTERVAL_INFEASIBLE未执行。collect/particle-null分支已排除；同计划deltaS[63..65]=0，决定边63/64为±0.5773502691896262。hardMax=0.5773502691896257，因此全局slopeLo=+4.440892098500626e-16、slopeHi=−4.440892098500626e-16，**对这两次生成的固定计划**区间为空。preexisting允许1e-12残差，两处数值合法性口径不一致已被直接证实。

T86的6/6是证据用例完成，不是code5业务修复通过。现有trace等价正证据保留。T86报告“与浮点无关/全部候选角无解”过度泛化：固定delta=0的精确±H闭区间交集是[0,0]，并非天然无解；本次空隙确由存储/计算残差参与产生，不能排除数值边界。不同候选θ会重跑粒子/collect生成不同delta，不能把θ0计划的空区间套到θ5计划。

## 2. θ5对照证据的实际缺口

执行器f5在失败后setTilt(5)再step，结果摘要记录ok=true及REQUESTED feasible/accepted5；setTilt只改target/命令阶段/唤醒，不直接推进床层，但会改变规划输入和候选计划。不能因同一材料起点推断两个preview计划必须相同。

关键采集问题：`p086_theta5_trace.json`实际写的是主`traces[traces.length-1]`，不是局部`t5`，本轮核读该文件target/requested仍0。θ5完整S0、delta/per-edge没有按文件名归档；E01仅检查 `accepted!==null && traceAfter!==undefined`，而setTilt返回void，因此此判断本身不是接受成功的严格断言。摘要afterStep.ok/feasible可作观察，**不能用6/6推成θ5完整机制取证已齐**。D01的tan30Hex为全零、exceedsHardMax=false也与其数值不符，按采集缺口纠正，不改原raw或用这些辅助标签推翻实际边值。

## 3. 分流：局部数值/合法态设计问题成立

在preview正常生成、相同状态经过既有松弛/回滚后，preexisting将状态视为允许，candidate严格slope交集却阻断；未见缓存/rollback偏离证据。按T79§5第三行，**转designer定义合法态与数值边界一致性**，不先给developer加epsilon。这不是NOT_IMPLEMENTED预测试，也不能直接判预期fail-closed产品合格。T86报告的“正常安息角产物必须接受”最终由设计合同说明，不由tester断言取代。

designer任务书：不使用真机。不允许修改 `D:\hmprojects\HelloWorld\docs\后续工作清单.md`，仅自身行状态有限例外；不改滴答同名清单、不回退共享修改。所有渲染只走XComponent+shader含背景/前景，禁止Canvas上屏/回退；失败FAULT或明确STALE上一有效新管线帧。

基于本反例及P011现行§19.5/§20设计局部revision：统一松弛产物、preexisting与candidate合法态的数值表示/误差边界；说明θtarget=θbefore=0时定向区间[0,0]、角速度交集、精确边界±H、相邻ULP两侧、真实超限的接受/拒绝语义。比较方案，给守恒/guard不放宽证明与误差来源，不以随意epsilon、放大tan30或绕过planner求通过。preview→commit一致、预算/事务/失败回滚/非冻结原合同保留；旧判据有修订必须显式版本化，冻结测试不可私改。说明与大倾角外部姿态新方案的关系，不借本卡自动启用另一整套物理设计。

θ5逐候选对照应补齐再用于可行性证明，但不必等它才启动上述已成立局部设计。设计交付后architect落最小P011TiltPlanningPass/guard/trace接口与回归，再转developer；不在本稿预选容差。

## 4. 精确tester续证（已有轨迹补缺，不重复全套）

修执行器仅新增版本化证据：归档`t5`实际最后θ5 trace，setTilt前/后/step前各物理S0和完整池/预算/身份摘要，证明材料起点关系；明确target、commandPhase等合法变化。θ0失败和θ5成功各输出真实plans/per-channel delta、63/64及全边区间，按生产顺序复算。修tan30Hex/超限辅助值，finite/Inf/NaN明确编码；E01断言step.ok、trace.requested/target5、result.accepted，而非void返回值。保留bfe45b4原报告/raw，不覆盖或将旧6/6改成产品PASS。补证回architect，tester不整体归因。

## 5. T82设备样本：相同错误码不等于同根因

114a7ee/s144 R2有5笔真实dry准入、effective30、commit577后页面显示code5；触摸修复和可见粒子/UP证据成立，不回退T71。R5同角3s未触发且已见粒子悬空，不证明长期稳定或LAND已完成。设备没有该失败attempt的planner trace/S0/池与候选计划，**无法判定与离线step73是同一空隙机制**，也不能因FAIL恰在沉降过程中断言已经LAND成堆（报告同时记录deposited=0、LAND未证）。

模型tick在首个step失败后设置failed=true并stop；随后页面每tick的page-physics-failed可以来自模型失败门，未必再次调用field.step。commit不变不是field永久frozen的证据。保留原失败run invalid，不通过自动重试/清failed/reset抹错误，不把已成功外源回滚。

设备精确补证由manager派tester/effector独立真机串行卡：同构建完整身份与R2动作，触摸诊断→真实receipt/sourceEpoch→首失败attempt匹配，启用T85已有trace **只在首次code5有界导出**（含COMPLETE/dropped标志）；保留run/state/attempt/config、medium、dt、target/effective/rate/真实sensor输入、S0全数组、每候选stage/plans/delta/edge/blockedReason、粒子contact/LAND事件与失败前后快照/rollback。若现有设备导出入口不可用，developer仅接有界诊断导出及副作用等价，不先改物理。不得在每帧打印全数组或调用递增身份getter。按run/PID分开R2/R3 flush，不合并不同run记录。

此设备卡需要“真机”标签；本T90归因本身不使用设备。可先离线固定dry初态+30°同输入/准入序列复现；缺精确sensor/状态则明确相似候选，不冒称精确重现。若观察不足先补首失败机制，不凭3s悬空开动力学返修；长窗LAND沿T95后续测。

## 6. T95前置与工程边界

T95整卡的正式长窗LAND/手持复测前置：设备code5有足够机制裁定，且若确定缺陷则对应设计/工程修复与开发离线证据交付；仅离线step73修复不能在没有同机制证据时自动放行设备问题。为补首失败trace可另派窄取证卡，不把整张T95因可做局部准备改立即开工。manager据完成通知/实质依赖推进，不新增批准门。

若后续证明设备属于现合同允许的不可行输入而非缺陷，designer仍需给产品可用性/允许输入处理方案及合法完成证据；不能以fail-closed正确将该演示产品判PASS。若是另一个collect/particle错误或真实超角，回architect按具体状态分流，不套用本次数值修订。

后续工程仅按设计revision改对应区间/guard/表示hunk；原endpoint/peak/commit契约、外源累加、质量/容量/性能门不改。回归覆盖原step73/θ5、无注入/容量512/停输、精确边界与真违规、preview纯度/commit一致/回滚非冻结、dry30°首失败及修复轨迹。设备呈现始终新管线，FAULT/STALE身份诚实。

本轮完成离线设计分流与设备待证路由；根因修复及设备同因均未宣称完成。只改本文及自身状态，原卡报告，后续manager建卡派工。
