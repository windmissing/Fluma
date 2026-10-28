# T197 全事务复杂度方案复盘 v1

2026-10-28，designer；P011_WHOLE_TRANSACTION_V1；未实施／未用真机。

以T116 V3为唯一失败基线，纠正了“B天然等于N”“lift都乘8”“particleMassAt每列扫N”等误设。现行重复seed/audit/collect/copy与ID/token扫描均保留在public/frame公式中，field C²、η循环、观测／JSON/IO与GPU SHA/fence也不藏到常数项。

源码续读还确认correction byRatio的内扫后立即break，只执行一次，不能见嵌套就断言新的平方热点；水的donor/receiver与每patch全S重算才有各自真实平方上界。非法输入的原首码先后同样不能被新入口catch-all audit抢走；无证路径须保留原执行并实计成本。

选型上放弃一次预选多pair、跨seed合成一次审核和泛化增量sum。前两者改变质心依赖及失败/预算次序，后者改变F64 slot-order fold。本版以邻接见证决定最小eligible端点，再作完整区间最小ID查询，实现可失效的动态派生索引；仅在实际key改变处更新，不把旧质心结果复用。

核心取舍是承认数值审核残余O(A_num*(N+C))：结构树省重复排序与线扫，证书只对相同read-set复用，dirty时原图重跑。物理预算不被工作量预算替代，原子准入与异步owner明确是新行为版本，不以丢请求/只跑小样本制造收益。

完整owner改善UI调度与总体算力改进分别评估；O2/GPU继续是隔离对照。无设备真实N/B/M不阻设计，也不填成0或反推旧freeze输入。下一步architect工程化＋T198独立全链规格，在同offered/accepted输入、同失败前缀、同F64/事件/源账下核结构与资源包络，不回到逐点热点补丁。
