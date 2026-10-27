# T168：T170 §4/5 工具续作交付

开发员；2026-10-27；不使用真机。执行顺序 T171（961d837）→T174（c797ea9）→本卡。仅改分析工具、夹具、报告；本卡不改 entry、物理门限、事务、退款、guard/audit 次数或渲染。

## 当前结论与范围

当前生产源码 OFF_ALL：24 个原始根、60/60 原 registry AST 身份、1032 个上下文，源码分配 0、内建分配效应 0、unresolved 0、issue 0，remaining=0。`P011NaturalObservationProbe.recordParticleColumnGuardInto` 仍在实际闭包内；没有把 N 随 P/E 关闭来取零。

这是源级、有限闭包、指定配置断言下的分析结果。没有测运行时分配次数、GC、冻结因果、设备性能或端到端产品指标。T175 独立复核另行执行。未知/不支持程序仍失败，不要求分析器证明任意 JavaScript 程序。

E ON 和 P ON 继续报告真实构造、字符串/迭代器/集合效应及未知时钟；数值以 `test/reports/t168-rev2/gate-summary.json` 为准。无参 API 与冻结 50ebbff 原 API 深比较一致：35 remaining，原退出约定不变。未缩减根或 registry，未更新事实契约 fingerprint。

## 实施

- 同一 REF 在 nullable/分支 join 后保留对象身份；super 静态选 base 实现，callee 保留 derived-this；已知实例精确虚派发，未知基类接收者枚举本地继承实现集合，排除伪同名类。
- 普通数组仅在 readonly cold owner、唯一 dense initializer、容量、整数索引上界、无重赋值及当前未逃逸同时成立时接受已有元素覆盖。当前 fieldBinding 为 8 槽。未知索引、越界、改 length、push 和逃逸继续阻断。
- 局部对象/集合/数组/闭包具有身份及单调 may-points-to 边。未知调用记录可达 havoc 及为何保留未达入口事实；含 workspace 的 Map/closure 必须使入口事实失效。冷集合未知内容或 opaque/shared 接收者保留全局保守失效。
- 闭包构造即记 allocation；调用才执行实际 body，传递捕获 env、参数和箭头 lexical-this。捕获 binding 后续重赋值的 live-cell 能力尚不支持，显式 NOT_IMPLEMENTED、阻断，而非沿过期快照剪枝。
- Map/Set 品牌来自实际 lib constructor/readonly 初始化；Map.set、Set.add、Array.push 等记增长效应；keys/values/entries 记迭代器效应。伪 get/set 必须执行其体或 unresolved。未知原型修改继续阻断。
- for-of 执行本地（含继承）iterator/next/done、元素 body、有限不动点和 return/throw/break 的 IteratorClose；潜在抛错路径保守执行 close。内建迭代器创建/next 结果记效应；未知协议/绑定/不收敛标 NOT_IMPLEMENTED。保守 close 可能产生假阳性，不据此丢掉物理工作或真实证据。
- 链式 receiver 及全部实参先求值，数值 toString、字符串 padStart 等记字符串结果效应。位点键包含 owner/start/**end**/kind，避免内外调用同起点而合并漏记。
- 四层 SOURCE_ALLOCATIONS / ALLOCATION_EFFECTS / PROOF_GAPS / NOT_IMPLEMENTED_ANALYZER 均保留阻断项，顶层 unresolved 仍为兼容总表，不靠分类减计数。

## 自验与证据

命令：

```text
node test/run_t168_hot_context_graph_units.cjs
node test/run_t168_rev2_effect_units.cjs
node test/tools/run_t168_gate_report.cjs
node test/tools/archive_t168_v1_reconciliation.cjs
node test/run_t168_original_t155_regression.cjs
hvigorw assembleHap --no-daemon
node test/tools/finish_t168_delivery_manifest.cjs
```

原 25 开发反例全过；原 tester 16 条断言由 developer 重放，断言未改，仅输出/基准归档路径及报告身份改到 rev2，**不是新独立 tester 验证**。新增 33 项夹具含固定数组、super/虚调用、有限 havoc、闭包、内建增长、零/多元素及 abrupt close、继承迭代器、嵌套字符串；合计 74 项。逐项结果见 effect-selftest.json。finish 脚本核全部计数、当前工具/契约/业务源码与图内 raw hash 一致后才输出交付 manifest。

首轮新增反例发现链式同起点位点合并；已修正并重跑。另一个夹具曾把 closure 源文本中的 marker 当作已执行 constructor，已改为精确 constructor-kind+完整表达式判定；未放松分析 verdict。

旧 81（15 allocation +66 unresolved）从 git:6c0c654 逐字节拷贝至 `test/reports/t168-rev2/original-81/`，附原 SHA、逐条旧 witness 与当前 owner 上下文对账；不覆盖原归档。对账分类只作索引，不参与剪枝或判定。T174 之后的源码/本工具实际 hash 全在当前图 manifest。

Native 编译通过，CompileArkTS、PackageHap 增量通过。assembleHap 末段仍因已有 SignHap 00303116（storePassword/keyPassword 长度配置）失败，详见 build.txt；未改签名材料，未把整体 assemble 记成功。当前 unsigned HAP/源码/工具身份见 delivery-manifest.json。

未知外部实现、动态原型、未支持语法/闭包 live cell、无法收敛循环与超限均保留失败，不作全程序无分配承诺。GPU、运行时 GC 与 T156 设备证据不在本卡开发自验范围。
