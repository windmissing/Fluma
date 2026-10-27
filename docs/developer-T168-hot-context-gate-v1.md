# T168 主门禁上下文适配器：部分交付，主图仍 FAIL

日期：2026-10-27。原卡 `6ae0404de4b0127832e4e508`；实施依据仅 T162 正式 V2 §4（50ebbff）。不使用真机，业务 entry 零修改。

## 交付与真实结论

新增 source-root/profile AST 分析器，保留无参 API/CLI 的原保守图与失败退出码2。
开发反例自测通过不代表被分析程序零分配；当前 OFF_ALL **NOT_ZERO_ALLOCATION**，productionHotPathZeroAllocation=false。
原图35 remaining（34 unresolved+1 allocation）原样分列，已通过与 50ebbff 原函数输出逐字段相等断言。

| 图 | 显式分配位点 | unresolved记录 | remaining | registry |
|---|---:|---:|---:|---|
| 原无参保守图 | 1 | 34 | 35 | 60/60 |
| OFF_ALL | 15 | 66 | 81 | 60/60 |
| PROGRESS_OFF_EVIDENCE_ON | 29 | 98 | 127 | 60/60 |
| PROGRESS_ON | 29 | 98 | 127 | 60/60 |

这是静态可能可达的**位点/证据记录**，不是运行分配次数、字节或耗时；同一调用可同时有 unresolved 与别名逃逸记录。新图分母扩展到真实接口实现，不能与旧图35相减推“新增46个泄漏”。

## 已实现

- `audit({profile,sourceRoot,registryPath})` 及 `--profile/--source-root` CLI。无参原逻辑保留；profile图失败仍exit2。
- TypeScript compiler host 将本地 ETS 映射为 TS，沿真实相对 import 建符号依赖；函数/方法按符号、receiver 类型和本地结构接口实现集合解析，纳入 class、继承方法及对象方法；不按方法同名全连。
- Workspace/Index 默认 AST、冷配置/关闭/恢复/重绑的14项写入及其所属控制流指纹：与批准源码合同逐项核对；冷配置三个函数另核hash。任何新增、反向写入/守卫变化都使证明失效。P/I/E/G独立，不从P OFF推出E OFF。
- NULL/UNDEFINED/TRUE/FALSE/UNKNOWN、带来源身份的对象引用；const/重赋、参数/返回摘要、同一实例槽读写、callee别名写回、guard正常延续分支的关联传播。
- if/else、早返、?:、&&/||/??、可选调用的实参短路；try/catch/finally保守合并隐式异常及未知分支早返状态；for/while/do有限域不动点（8轮不收敛即FAIL）；switch保守fallthrough。
- 原显式分配分类和原Math/Number数值内建集合保留；set/fill只在checker验证TypedArray receiver后作为已有存储操作，视图new仍constructor。
- 原24个实际根/getter与60项registry保留。registry按原始每项身份+当前owner/具体AST位点hash核对，缺项/缺owner/缺位点一律FAIL。F14旧文本在dry扩展后已变化：合同显式绑定当前出生对象，不修改原registry或移出分母。
- 每个profile输出源码raw-byte/parsed-text哈希、换行口径、commit/dirty、根、context、调用边、剪枝guard与实参、分配/unresolved根到调用链；ON分配/源阶段旁表，外部clock未知保留。

配置事实是**固定批准AST+实际默认值**的有限证明，不是任意冷程序推理器。工作期间不允许外部重配置是入口断言；热图一旦出现配置写入或未知别名逃逸即FAIL，不能将其当可信豁免。旁表的阶段标签只供阅读，不参与剪枝/豁免。

## 当前 FAIL 的两类证据，交 architect 首次分流

1. **真实闭包扩大后发现的源级分配**：applyParticlePlanInto → applyParticlePlanIntoBody → 接口 P011ParticleHotCommitSink 的实际实现 P011ParticleM2CommitSink.applyInto → host.internalParticleGuard → WaterSandField.internalParticleGuard:4539 `new Map<number,number>()`；之后 materialGuardAdmitIncrement/MaterialGuard 出现对象、集合、数组、闭包。另一个接口实现 Prototype 同时保留。不能删根、只保留Prototype、把M2记ROUTED或白名单来取得零。
2. **分析精度/未实现能力**：Map/Set/Array迭代与内建帮助调用不在旧数值内建集合内，保留unresolved；for-of、closure body/callback摘要为 NOT_IMPLEMENTED/FAIL_CLOSED；未知时钟与未知调用别名逃逸使槽事实widen，可能在OFF图保守地再次到达诊断阈值视图构造。该条不证明OFF实际执行诊断。部分this/别名关联在分支合并后成为UNKNOWN，也保留失败，未按诊断模块可信豁免。

因此本卡**未完成“当前树OFF_ALL零remaining”的结果**，也未宣称完整的任意语法/回调分析能力。诊断调用的局部null剪枝成立，不能替代整图证明。需要architect对真实M2闭包/主门禁范围作裁定，并为剩余能力给精确工具续作；不擅改entry、scope、物理阈值或T155 estimand。

## 验证与复跑

`node test/run_t168_hot_context_graph_units.cjs`：25项开发反例（正式结果 selftest.json）。测试不仅断言总图FAIL，而是逐项确认注入位点出现/不可达、真实receiver选择、配置证明失效及registry缺项。覆盖T162 §4.1清单，额外包含返回null摘要、callee别名写回、loop事实合并和伪TypedArray方法。

`node test/tools/run_t168_gate_report.cjs`：归档三profile与原图、原API精确比较、ON分配/阶段/缺测旁表；归档工具执行成功不等于门禁通过。直接 `node test/probe_p011_particle_hot_graph.cjs --profile OFF_ALL` 仍exit2。该测试工具无需运行物理回归来替代T155；独立测试、动态等价、产品稳定性和设备均NOT_RUN，归tester/effector。

`hvigorw assembleHap --no-daemon`：Native通过，CompileArkTS/PackageHap增量UP-TO-DATE；最终SignHap既有00303116密码长度失败，未修改配置。原始日志 `test/reports/t168/build.txt`，源码/包身份 build-manifest.json。

## 证据

`test/reports/t168/gate-summary.json` 为入口。完整图为 gzip JSON，保留压缩前后SHA，解压后包含全部context/调用边/见证而非裁剪摘要。
`off-all-remaining.json` 提供可直接读取的残余、根及源码manifest；`*-allocations.json` 是ON/主图旁表；`diagnostic-on-source-inventory.json` 是冷初始化/导出/阈值/失败相关源码盘点，阶段标签不授予零分配结论。

执行者状态按规则“看评论”，不是零分配完成；原卡同步部分交付及前置。T155可以消费工具并独立反例验证，但本报告不替代T155完整结论，不解除T156原静态前置。
