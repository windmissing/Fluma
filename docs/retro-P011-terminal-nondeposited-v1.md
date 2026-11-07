# P-011 路线 C 终点未沉积口径：设计阶段复盘 v1

日期：2026-10-24。责任：designer。技术路线：dual-field-particle-v1。状态：DESIGN_FINAL / USER_DECISION_PENDING / NO_IMPLEMENTATION_OR_EFFECT_VALIDATION。

原因：沉淀任务6adc52e4…1949的候选探索与源码复核结果。对应 [方案定稿 §7](design-P011-terminal-nondeposited-fraction-v1.md)。本记录只覆盖设计阶段，不把尚未进行的实现/双报告验证写成已闭环。

## 候选与取舍

- 仅列悬沙：保留旧结果可比性，但会漏掉真实粒子库存；C终点不推荐，历史字段保留。
- 列悬沙+仅飞行粒子：避免名称歧义的表面方案，但遗漏state=2尚未获得LAND授权的质量；否决。
- 列悬沙+全部活粒子：质量分类与canonical库存一致，转列/合并不改变判定量；选为唯一推荐。
- 改5%或统一所有分母：无量测依据且顺带改变SLIP的冻结基准；本轮否决。
- 仅固定末步与末1s最大值：推荐后者防单步低谷，明确这是待用户批准的额外方法变化，不追认旧报告。

## 设计复核暴露的问题

1. SLIP与WS并非同一分母：前者是首子步bh+bl，后者是总沙。显示三分项用累计期望，SLIP checker必须另算。
2. thick倾角持续映射shake输入，不能为所有终点统一增加“shake=0”资格。静置WS与SLIP按各自原schedule判断。
3. context与metric分别取时，physicsTime来自单调化Date.now，多次调用会递增。身份对齐用run/sequence/frame/substep；完整实际时间窗另需schedule/capture边界证据。
4. getMetrics的沉积分母已含外源，updateMetrics的沉降t90仍用initial，且注入没有可靠清除已触发t90。作为既有CONS-04集成检查项交接，不在本卡修复。
5. Nmax限制槽数，MERGE允许质量包变大；Nmax×nominal m仅是未合并名义容量。450units也依赖lift独占共享额度及1.5s假设，不能冒充观测或可达性证明。
6. ≤5%允许小量残余；它不能单独排除永久飞行/待准入。以状态/cohort/事件诊断配合既有无永久悬浮要求，不私加未经标定的寿命阈值。

## 已做的设计核查与后续

已对照工程稿§3.3/§5.6及落床rev1契约、DUALFIELD预注册、goals G2.6、WS-08/09、SLIP-03、源码getMetrics/updateMetrics/注入分支、thick输入映射和SLIP checker；只追加designer设计文档。

后续顺序：manager转用户裁决→tester独立endpoint revision→architect/developer接口落地→同路线双报告→designer正式验证。本轮没有执行C物理测试、没有设备采集、没有VALIDATED结论。若峰值悬沙、长期不落寿命标准或协议输入需变更，分别另起revision，不夹带在终点口径里。
