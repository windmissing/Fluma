# T320 零分配定向证明补证（离线，结束待验收）

2026-10-30，tester 离线。按 architecture-T316 用同一前后 manifest（static-before/current.json.gz）与当前源码逐项归因，不改静态工具、不设白名单、不放宽断言。

## 一、显式位点 39→39：**(owner, text) 集合完全一致**

- 表面 23 新/23 消失（按 owner+line 对比）**全部为行号位移**——T311 计量代码插入导致行漂移，逐条 (owner, text) 对比零新增零消失（工具记录在 `t320_summary.json`）。
- 结论：T316 "逐层为显式源码分配位点 39→39" 由 (owner,text) 集合等价确证；**无一例真实新增 `new`**。

## 二、内建效应 2→2：同位点保持

两层均 `P011PoolProgressDiagnostic.checkLong` line125（冷诊断路径，非热路径）——保持。

## 三、净 18 记录逐项归因（unresolved 层 102→120）

| 类别 | 数量 | 归因 |
|---|---|---|
| `Date.now`（begin/end × UNRESOLVED_CALL/UNKNOWN_CALL_OR_ALIAS_ESCAPE） | 4 | **T311 新热时钟原语证明义务**（见 §四） |
| `CONTEXT_LIMIT_FAIL_CLOSED` 新增 | 37 | T311 计量接线（receipt.begin/end/syncHotContext）插入 → 调用图扩大，分析器上下文超限 fail-closed |
| `CONTEXT_LIMIT_FAIL_CLOSED` 消失 | −32 | 同因位移（TxWorkTrace.leave 等旧上下文合并/移动） |

净计数与分配无关——全部为**证明缺口与工具未实现层**，非运行时新分配证据。

## 四、Date.now 原语定向证据（T316 义务①）

- **规范层**：ECMA-262 §20.4.2.1 `Date.now()` 返回原始 `number`（时间值），规范语义无堆分配要求。
- **不可证层**：目标 ArkTS/方舟运行时的 `Date.now` native 实现是否内部分配，**无法从项目源码证明** → 按卡记 **PROOF_GAP**（不臆测"永远无分配"，不给白名单）。
- **既有同类**：旧 `TxWorkTrace.clock`（line12）同原语已在开工前图中（TxWorkTrace.enter line35 unresolved）——非新类别，旧门同样未闭合；不能只给新两处豁免。
- 运行佐证（非静态证明）：T312 真机 32 行 T311_STAGE 计量输出、长期运行稳定。

## 五、诊断开关等价性

- 源码：`begin/end/next` 均有 `if(!this.enabled)` guard——关闭态连 `Date.now` 都不执行；`emitProgressCold/exportCold` 注释明确仅冷诊断 timer 调用。
- 运行：T311 97 项自测全绿（含 M2/多 R/N/rollback/时钟异常），运行等价有佐证；**静态同输入等价仍属 PROOF_GAP**（分析器未实现层覆盖）。

## 六、结论

T320 **结束待验收**：净 18 全部归因为证明缺口/工具未实现（非真实热分配）；39 显式位点与 2 内建效应保持原状未闭合（正常热路径可达性仍未证）；`NOT_ZERO_ALLOCATION/164` 维持 FAIL，不推静态 PASS。交回 architect：若需闭合 Date.now 义务，需目标运行时原语摘要工具或架构裁定热计量移位；静态门收口不阻 GPU 主线（T316 原判维持）。

证据：`test/reports/t320/t320_summary.json`（前后 manifest 逐层对账记录）。
