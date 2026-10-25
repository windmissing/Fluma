# P-011 D1-R3 S1–S4 开发交付

日期：2026-10-25。角色：developer。任务：6adcc2c1e4b017ab33e1cf05。

状态：**M1_IMPLEMENTED / ISOLATED_ARKTS_COMPILED / ISOLATED_TESTS_PASS / ZERO_ALLOCATION_NODE_VERIFIED / SHARED_WORKSPACE_REGRESSION_BLOCKED / PENDING_MANAGER_ACCEPTANCE**。

## 实现与依据

沿用已验收热接口契约 v1 §2–§9、迁移设计稿 §5 的 S0→S4 顺序。

| 切片 | 落地 | 提交/证据 |
|---|---|---|
| S0 | 已验收定容 workspace，保留旧冷接口 | 1511fbf |
| S1 | 原地 heapsort；固定请求/运动/lineage；双 collect 逐字段比较；完整四层床高；NaN/重复 ID 拒绝；分母单次消费 | 5fd647e；11/0，含64个冷热预测对照场景 |
| S2 | 复用原 transactionBefore/After/Scratch；数值 patch token；六事务；busy/ABA/跨 owner/重复 publish/restore/discard/finish；正尾数与零释放；MAX_SAFE_INTEGER 耗尽 | 1d824f8；12/0；旧池33/0、退款11/0 |
| S3 | 独立 HotBudget/HotPrototype；定容 R9 逐端退款、total一次、bottom零；闭式 guard；逐笔事件/guard 数值证据；全步池/场/预算/sidecar/out 回滚 | 当前历史96508ab；运行时后续89682d9；sink14/0 |
| S4 | 60条原站点保全与 Reachability；全可达图；完整双 collect/apply/finish 沙盒；导出页租约与独立副本；晚换 after-image 拒绝；固定缓冲身份检查 | 857e09a、f808842及本报告对应最终提交；S4 6/0 |

`P011ParticleHot.ets` 为热接口入口；`P011ParticleHotBudget.ets` 为数值预算/R9适配；`P011ParticleHotPrototype.ets` 为独立桩场与guard；`P011ParticleHotEvidence.ets` 只冷导出。`P011ParticleField.ets` 新增的绑定/复制接口保留旧API独立副本语义。未改旧 `refundLocal/materialGuardRefund` 方法体。真实 WaterSandField、λ/η、页面、route生产接线仍归D2。

## 五组验证

1. 静态：[完整调用图](../test/reports/p011-d1-r3-s4/allocation-graph.json)。原 F01–31 / P01–29 全部保留，标记 COLD_DIAGNOSTIC_COMPAT；80个HOT可达节点，显式分配0、未解析调用0。原粗粒度旧探针仍能命中60处冷兼容站点，不把旧exit=2改成PASS；新全图探针和动态探针独立返回ZERO_ALLOCATION/exit=0。
2. 动态：[36,000步原始报告](../test/reports/p011-d1-r3-s4/allocation-36000.json)。600秒模拟时间；N=512、C=128；11个预创建冻结场景轮转，每步重新填固定输入并完整双collect/apply/finish；包含spawn尾数、部分LAND、跨列/容量拒绝、显式merge/degrade、满池merge与lift降级。另在trap内实际执行journal溢出、发布后审计失败与新attempt重试。构造/容器/序列化/增长helper/冷调用trap均0；634个TypedArray引用与长度不变。此为隔离事务沙盒，不宣称连续真机场景轨迹。
3. 句柄：patch12例及S4追加6例涵盖旧primitive token、busy、跨owner、重复操作、discard/restore/rollback后prepare、MAX_SAFE_INTEGER耗尽、out/缓冲复写、页token不可复用。拒绝前后canonical及受测预算/out逐值一致。
4. 物理与日志：冷热六事务逐值比较池SoA、三桶场数组、预算local/total、grant/clipping、事件次序和字段、guard身份；1024个guard边界/符号/non-worsening输入与旧闭式函数对照。旧33+11测试全绿。源质量、CONS-03正尾数和R9旧平分负对照保留。
5. 性能：仅在上述检查通过后披露同一Node实验的CPU与导出/GC/内存。热段P50/P95约0.37/4.65ms，最大318.15ms；172次GC被观察到，热区间起始GC计数0。冷导出、VM/JIT和GC仍有成本，完整值在JSON。不能将Node结果外推为设备ArkTS帧时或VM内部绝对零分配。

契约列出的全部数值遥测在workspace或只读getter就位，包含版本/owner/epoch/attempt/patch状态、各有效计数/容量、错误stage、rollback/stale/busy/overflow、token耗尽、bufferIdentityChanged、legacyColdCallCount、pageBacklog；实跑后后两项为0。源码列举实际为23个名字，沿用全部名字，不因文案“22项”漏字段。

## 编译与回归的证据边界

共享工作区包含并行M2/页面/账本改动，曾出现合并冲突和语法错误；当前完整工作区验证不能宣称零回归。已观测问题包括schema断言不匹配、advection `donorStockBefore`未定义、renderer空material view、endpoint缺C摘要被写为0，以及SandCanvas语法/类型错误。逐例见[验证摘要](../test/reports/p011-d1-r3-s4/validation-summary.json)。未修改这些测试判据或为本卡改写其他卡代码。

为了直接核实本卡的编译可用性，`test/prepare_p011_hot_verification.py`生成独立副本：无关App/field/既有回归代码来自1511fbf；精确叠加当前M1模块与本卡测试，D6观测声明来自已提交版本。显式compile-only worker导入Hot、HotPrototype及HotEvidence，使用DevEco自带hvigor6.26.8执行`default@CompileArkTS`，**BUILD SUCCESSFUL，0编译错误**，见[编译日志](../test/reports/p011-d1-r3-s4/isolated-arkts-build.log)。副本适配了该捆绑版本不支持的wrapper版本/analytics配置键；未修改主工作区配置，未做签名或设备安装。

隔离副本回归：planning106/0、gesture16/0、input124/0、render82/0、tracer9/0、S3 14/0、S4 10/0、advection8/0。该基线gesture为16例；共享树新增的两个回调例依赖另一卡，未把它们算成已在此副本通过。endpoint Python24/0、Node9/0、协议10/10 MATCH；热接口本卡43/0。不将隔离副本结果冒充共享工作区全通道通过。

## 复跑

```powershell
node test/run_p011_particle_hot_units.cjs
node test/run_p011_particle_hot_patch_units.cjs
node test/run_p011_particle_hot_sink_units.cjs
node test/run_p011_particle_hot_s4_units.cjs
node test/probe_p011_particle_hot_graph.cjs
node test/probe_p011_particle_hot_allocations.cjs
python test/prepare_p011_hot_verification.py
```

独立副本的构建使用其`node_modules/@ohos`对捆绑编译器/插件的junction，设置JAVA_HOME至DevEco的jbr和NODE_PATH至副本node_modules，再执行捆绑`tools/hvigor/hvigor/bin/hvigor.js --mode module -p product=default -p module=entry@default default@CompileArkTS --no-daemon`。

本卡实现与证据已入库供审查；共享工作区编译/回归还需相关卡完成合并后复核，D1验收与D2解锁由manager决定。未使用真机。
