# T96 developer 回执：粒子池同步热点优化

日期：2026-10-26。未使用真机。开发交付，设备性能结论尚待独立复测。

- `mergeOldestHot` 从当前 scratch 池按 ID 递增搜索，找到字典序最小合法 `(a,b)` 即短路。匹配谓词、`mergePairHot` 加权顺序、无匹配错误与回滚保持原合同；每次 merge 重新读取 scratch，不复用冻结池索引。
- birth 入口保留一次 collect 的冻结池、预测器和溢出校验，第二次被覆盖的 collect 改为复制完整身份头并验证 B 缓冲。普通 motion 双 collect 不变。
- 可开关的池进度标量计数 seed/birth/merge/collect、候选检查、耗时、active/journal/lineage、free/live/nextId、首错误、真实 shake requestEnergy 与更新后 energy。页面诊断门默认关闭，手动开启后最多写 128 条 gate→clock→request→inject 阶段摘要，超限显式 TRACE_INCOMPLETE；不开启不生成诊断字符串。

离线结果：`run_t96_pool_hot_units.cjs` 5/5 PASS；N16/N512 满池兼容情形分别 16/512 次候选检查、一次 collect、选中 ID[1,2] 且质心守恒。无匹配情形分别 2048/2097152 次候选检查（含现有 8 次 λ 重试），池逐值回滚。外部 30s timeout runner 用时约 2.1s、exit0；最坏情形仍为 O(N²)，**不宣称设备性能通过**。既有 hot 11/11、patch 12/12、sink 17/17、S4 6/6、M2 13/13、事务 14/14、T85 7/7 均通过；hvigor ArkTS/native/HAP BUILD SUCCESSFUL，零编译错误。

T78 原始设备轨迹没有可直接复演的完整输入与池快照，三臂精确离线重放标记 **UNPROVEN**；T97 独立有界/等价取证及后续设备卡据此继续，不把有限循环或本机耗时当产品通过。
