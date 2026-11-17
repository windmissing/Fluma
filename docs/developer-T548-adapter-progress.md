# T548 阻断与局部开发交付

2026-11-17，developer。不使用真机，不执行测试/仿真/端到端。整卡未完成，默认真实触摸入口未启用。

已实现：按T543机器表的Config64及规范seed填充helper；binary64位型解码、checked128整数配方/RNE/饱和与workCap；半开像素映射及同pointer单指DOWN/MOVE/UP状态机；GPU四limb整数复算，核原delta/dt/J/cap/flags/radius，零额外dispatch/binding。GPU检查只对INPUT revision2生效，旧V1生产路径不变。kernel_assets.cpp包含头文件用于实际native编译。没有把这些helper冒端到端真实adapter。

具体阻断：T543机器表seed.canonicalAssignments.columnEvery32Words仅0=41943040、6=1，allOtherWordsZero=true，因此列9=0。T529 g9spf/audit.glsl的g9AuditMassGeometry逐列要求g9Column(c,9)==g9Material(c)，state_access.glsl material=S+F=41943040。T543又要求首次CURRENT前完整map/质量/容量/规范零审计。两者不能同时满足。模板SHA243b2b95586f5aa7f22610b13bb826f514cb4e27775a9476f99fcebbcadfa468、Config64内模板SHA及parameterSha均被冻结，不能私补列9后继续宣同身份，也不能删审计门。

已检查MD/机器表 canonicalAssignments、生成schema字段及shader derived/audit切点；没有执行产品测试。需要architect明确列9规范40Q并同步template/config/parameter/actual示例SHA，或正式修订初始化派生缓存语义与其原始上传/审计时序。原卡已逐项报告。未接入可能创建混身份run的临时代码，保原transport/config行为。

未完成：新Config64研究页/native入口、真实ArkTS点与native时钟证明/PresentedMapping接线、初始化GPU审计、INPUT V2冻结与证据队列、cold V2双record/collector/reader、独立签包完整身份。这些最终链需统一新seed与parameter身份及合法初始CURRENT；helper不意味着这些已交付。收到architect精确修订后developer继续原卡，T544不得据局部代码开真实效果门。T546共享解码/安全错误串另有正式稿，未将其未交实现冒已接线。

局部编译：完整t548-partial隔离工程hvigor native/ArkTS/HAP；实际shader ES3.1仅compile/link通过，没有dispatch/draw。原件/日志见test/reports/t548/development.json。不得据编译给物理PASS或整卡完成。本人行看评论。
