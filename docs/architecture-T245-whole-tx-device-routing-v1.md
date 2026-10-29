# T245 整体事务设备缺证首次分流

2026-10-29，architect；原卡 `6ae28b60e4b0ab67702b91c7`。消费 T225 正式报告及 `test/reports/t225/` 三有效原始结果、T205 工程合同。**三路选对/终态局部通过，完整事务与设备收益仍缺证；现无可归因产品 FAIL。** 不使用真机、不重跑已通过矩阵、不改业务代码。

## 已证与边界

同 `WET_192_64`、32 step/64 root，GPU_PAIR_V1、LEGACY_SCAN、TX_ADJ_AVL 三路全部 COMMITTED，64/64 终态逐字节相等；GPU native pair 64/64 与 CPU oracle 一致，三 shader SHA/编译执行、11 dispatch 与 fence 有原始记录。三路总物理时间分别 1380/2606/3753ms，**仅此 synthetic fixture 的各一轮**；不能称服务 p50/p95、整体收益或真实 UI 收益。GPU/legacy 来自 t212-h1-final，AVL 来自 t211-compile-final，两份 sourceManifest 不同；跨包可做 fixture 语义正确性对照，不能作严格同 binary 成本对照。首次 avl3 因 T212 包无 TreeAdapter 失败，后续用 T211 包 avl4 成功，是隔离工程合包缺口，不是算法/产品失败。

T225 明确未做满池 synthetic、B/M/A/λ 逐计数、同 PID 两 freeze 臂、服务分布和 20 cut 故障矩阵。原始 ledger 的某些位置在 GPU/legacy 中量纲或作用域不同（报告例：49 列 5 vs 32），且 one-null 2304、both-diff 98；这些数不能逐槽做同义等价或相减计算收益。只比较带**schema revision、字段名、单位、时间作用域、inclusive/exclusive、计数分母与采样边界**均相同的 ledger 行。状态/receipt/nativeRows/after 的原字节与 pair oracle 是已成立的局部正确性证据，ledger 不同本身不是事务 FAIL。

## 工程合同修订与接力

T205 的差分入口按 `fixtureId+entryStateBits+commandSequence+paramsBits+algorithmRev+profileRev` 冻结，还须记录源包/二进制/进程 PID/owner、accepted 与 offered 分母。`B/M/A/λ` 是可审的真实工作量：分别保存每 root/阶段的事件、库存与选对、比较/访问/分配及有理时间账的**原始计数与定义**；禁止只给一个总 ledger 或把执行器不同语义槽映射成同名。每 cut 保存输入 snapshot、首错/回执、after 全域与未提交 root 的回滚字节；synthetic 满池须明确标记，不冒原现场。

**developer 离线合包工程（不使用真机）**：若要同 binary 三 backend 严格成本对照，补齐同一隔离构建的 TreeAdapter 与旧 GPU bridge 两路径；提供三路统一 fixture/trace schema、冻结两臂与 ledger 字段词典/单位，编译和非真机自测。此为成本可比性的工程前置，不能用当前跨包结果替代。资源 `runtimeNativeDriverBytes=NOT_TOTALLED`、`externalCopies=UNBOUNDED_EXTERNAL` 不得写完整峰值 PASS；需要逐层实际 live/peak 与外部副本计数。

**tester 设备补证（真机、串行，旧 pair 低于 GPU 驻留 P5 取证排程）**：不重跑已证的 64root 三路终态矩阵。先同 binary/PID/fixture 跑 GPU/legacy/AVL 的两 freeze 臂与满池 synthetic 锚点，保存每 root 原始 B/M/A/λ、faultPlan 和 admission/offered/accepted；然后 20 cut 逐点独立注入、校验 original firstCode/receipt/events/lineage/rollback 和资源退休；按同负载重复取整服务 p50/p95、prepare/worker/fence/map/archive、成功与失败帧及 UI 完整新管线 present/swap 成本。若同包合入尚未完成，correctness 的现有证据保持有效，严格同包成本项标 `ENGINEERING_PREREQUISITE`。只用新 XComponent+shader 上屏含前景/背景，故障 FAULT 或完整有效 STALE，禁 Canvas。

若补证发现实现偏离，先回 architect 按具体 cut/trace 归因，工程文档先修再 developer；设计矛盾转 designer，用例争议转 manager；尚未实现的功能登记 NOT_IMPLEMENTED，不冒产品 PASS。T225 现有局部通过无需复验或新增 manager 验收门。
