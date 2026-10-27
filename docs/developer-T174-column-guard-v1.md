# T174 M2 单列 guard 定容改造交付

2026-10-27，依据 T170 8110ace §3/3.1/5。不使用真机。只修改 WaterSandField、P011MaterialGuard、P011NaturalObservation，未改工具、M2Sink/Hot、预算/审核次数或渲染。

新增冷初始化三槽 workspace/out 与 p011GuardAdmitNumericInto；M2 host 保持标量签名、先 updateSandTop、左边先于右边、原 influence/residual/RN 运算、Math.tan(30*Math.PI/180)、lo/hi/tie/非有限值/早返语义。通用 Map/Set 核原样保留给其它 writer 和冻结参考。对原 Map/Set 的 -0 key 规范化显式保持；六个层库存约束、λ/η、300/s、退款和 audit 都未改。

N=naturalObservation 独立开启时，M2 host 拒绝证据只复制标量到2048行 SoA，不保留借用数组/out。原 channel/limiting/affected顺序/rejected/clipped 在 Hot 外按原序物化；其它 writer 的旧记录入口先排空前面的 M2 页，不改字段协议。失败/rollback 证据保留，flush 不随物理 rollback 丢弃。

容量/时序依据：当前绑定 requestCapacity=2048，原 plan storage/header/compare 在逐行执行前验证 count≤capacity；M2Sink.applyInto 每行最多一处 host guard，数值核/SoA producer 不含用户回调或重入调用。因此一独立 apply 内记录≤2048，拒绝条件只减少行数。WaterSandField 仅两处 motion apply 调用，均在进入前清页、返回/早返/异常的 finally 清页；preview 维持原 N=null，seed/dry-batch 的 spawn 或 birthOnly 分支不调用 host guard。单 owner 同步 Hot 域内没有嵌套的 N producer；观测 sink 在 finishSubstep、Hot 返回后才调用。

该证明针对当前生产闭包（OFF_ALL 的P/I/E/G，N独立ON/OFF）及生产只读SDK时钟接法，不授予任意可重入/可写物理状态的外部 clock/callback 通用证明。未知回调仍由工具保守FAIL，不作为零分配豁免。直接脱离 apply-span 的内部测试调用须按页冷消费，不能无限累积。冷 flush 检测越2048时明确抛 UNPROVEN_PARTICLE_GUARD_PAGE_CAPACITY，不静默输出截断证据；合法原Q闭包不会走此分支。没有增加物理准入拒绝、扩容热页或减少 guard 调用。

setProbe、beginSubstep、finishSubstep 冷边界也排空页；队列保存原 evidence-array target，避免重绑后写入新probe/新substep。两处apply原字面锚点保留，T171/V2隔离生成器的旧白名单仍通过；未覆盖已归档T171包。

`test/run_t174_column_guard_units.cjs`：9/9，11520组数值核与961d837冻结原核逐位相等，256组实际host调用含边界列/-0/非整数/非有限值；2048行顺序/借用副本/重绑；实际LAND的两guard/refund/event/state，以及故障rollback/throw保留N证据；N ON/OFF物理原字节一致。LAND拒绝证据用显式对抗fixture（collect tilt=0、host live tilt=20）触发，正常项在原/新外层均返回原PREVIEW_COMMIT_MISMATCH=6，不冒产品成功；ROLLBACK均99、THROW均原异常。ownerId/pageLease/runId作为独立实例身份规范化，物理数组、库存、票据/事件内容不删。

Hot11/M213/patch12/sink17/refund/gesture/planning/T166/T154/dry admission共10套回归全部通过。主工程Native、CompileArkTS、PackageHap通过；SignHap仍为既有00303116密码长度失败，未改签名配置。报告/源及包SHA/日志在test/reports/t174。独立测试归T175，设备/性能/GC及卡死因果NOT_RUN；不由new Map被移除推13秒卡死根因，也不代替T168完整门禁。
