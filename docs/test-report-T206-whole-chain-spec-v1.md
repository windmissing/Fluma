# T206 整体复杂度独立验证规格报告（RED 基线）

日期：2026-10-28。任务：6ae15c23e4b0f11bdf252e9c（tester，离线，接续 T198 用户关闭无交付）。消费：T197 整体方案 `algorithm-T197-whole-transaction-complexity-v1.md` §12。被测：当前树（TX trace 零实现）。

## 结论：RED 基线 + 独立规格交付——13 NOT_IMPLEMENTED / 2 PASS / 0 FAIL

执行器 `test/p206_whole_chain_spec.cjs`；原始 JSON `test/reports/t206/independent.json`；用例登记 `docs/test-cases.md` §19。

## A. 实现面符号核查（7/7 缺，NOT_IMPLEMENTED）

P011_TX_WORK_TRACE_V1 revision、scope ENTER/LEAVE/FAIL/CANCEL 计数、具名 scope 常量（B_REQ/M_FIND_ENTER/PAIR_BUILD/KEY_INSERT/RELOCATE/NEIGHBOR_TEST/RANK_SELECT/RANGE_NODE）、AUDIT_LOGICAL/EXECUTE/CERT_HIT 三分解、COPY_POOL_BYTES/COPY_FIELD_BYTES/UNDO_WRITES 计量、publication/encode/write/fsync bytes、GPU SHA_BLOCKS 分项——产品与工具全部零插桩。

## B. 独立恒等式检验器（2 PASS，规格核心交付）

执行器内建六组恒等式检验函数（独立实现，自带正反例自测通过），trace 到件后逐 scope 使用：

```text
entered = completed + failed + cancelled + stillActive   （闭合 scope）
M_find   = found + noPair + error                        （FIRST_LONG 运行中快照另加 findStillActive/findCancelled）
B_ent ≤ B_req 且 B_commit ≤ B_ent                        （discard 不算 commit）
AUDIT_LOGICAL = AUDIT_EXECUTE + AUDIT_CERT_HIT           （无证 skip 不存在）
copyBytes(root) = Σ exclusive child copyBytes
ΣframeWork = direct-source + queued-source + step stages + observation + publication + IO exclusive totals
```

每条均含"应过"与"应拒"反例（不等式/缺项/错误归类均拒绝），检验器本身经对抗自测。

## C. TX-01..12 逐条规格登记（全部 NOT_IMPLEMENTED）

按 §12 表 12 条逐条登记为机器可读 caseRows（TX-01 C/N/L/B 独立变化与同 M 分母逐 pair 一致 … TX-12 capacity/资源 sweep 与峰值分列），trace 到件后由执行器扩展逐条断言。全部标 NOT_IMPLEMENTED 结束本轮归因归工程（trace 插桩是 T205 工程范围）。

## 边界声明

- **T197 designer 4822 例算术模型不是产品 oracle**（MODEL 核：模型证据 JSON 为 designer 算术证明，独立规格不得复制该模型当实现）。
- partial trace 缺尾部标 INCOMPLETE，不声称完整恒等式已检验；耗时 inclusive parent 与 exclusive child 并列，不异时钟相减；失败/超时/拒绝/预热 outlier 全保留。
- 诊断 OFF 也需低扰动固定计数用于包络；trace 开关绑定 schema，不能无日志填 0 或拿不存在的 array 推设备 PASS。
- 工具字典与 T205 工程可并行对齐，不互设循环；无业务修改、不跑设备。失败上报：无。无阻断。
