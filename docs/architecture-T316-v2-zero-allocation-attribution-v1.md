# T316 · V2 零分配静态失败首次归因 v1

2026-10-30，architect；不使用真机。依据 `docs/developer-T311-v2-work-bound-v1.md`、`test/reports/t311/{delivery.json,static-before.json.gz,static-current.json.gz,static-comparison.json,static-adversarial.json}`、当前 `P011HotWorkReceipt.ets`/粒子路径源码及 T168 静态工具/开发报告。T311 的开发 `f2f177a`、97 项运行检查、两工程编译和两签包交付成立；静态零分配门仍 `FAIL`，T312 可用已交包独立真机，不以本卡新增设备前置。

## 事实分层

T311 开工前原源码的 OFF_ALL 已是 `NOT_ZERO_ALLOCATION/remaining=146`；修订后 `164`。逐层为显式源码分配位点 **39→39**、内建分配效应 **2→2**、证明缺口 **55→59**、分析器未实现 **47→61**。兼容 `unresolved` 记录为 102→120；这不是新增 18 次运行时分配。按 file/owner/text/reason 对账，新增 28 条记录、旧上下文消失 10 条，净增 18。其中四条是新 `P011HotWorkReceipt.begin/end` 的两处 `Date.now()` 各被记为 `UNRESOLVED_CALL` 与 `UNKNOWN_CALL_OR_ALIAS_ESCAPE`；其余主要因新 `complexity.begin/end`、`syncHotContext` 和既有 storage/trace 调用图扩大，出现 `CONTEXT_LIMIT_FAIL_CLOSED`，并非逐项新 `new`。当前显式位点仍含旧例外抛出、构造、数组字面量、字符串拼接等 39 项；它们是否在目标**正常热路径**可达、能否由既有 guard 证明不可达，旧门也未闭合，不能因“新增显式位点为零”宣 ZERO。

`P011HotWorkReceipt` 构造时预分配 `20×3` 当前数组与 `32×80` 行；`begin/end` 在热阶段执行 `Date.now` 并更新固定数组，`emitProgressCold/exportCold` 中的 JSON/Array.from 明确冷调用。源码可证明没有在这两个热方法中显式新容器或格式化，但目标 ArkTS 运行时的 Date 原语是否分配、静态工具对此原语的摘要以及回调别名/分支可达性均**未证**。原 `TxWorkTrace.clock()` 默认也调用 Date.now，故旧图已有同类未知时钟问题；不能只给新两处白名单，也不能用运行自测通过代替静态证明。`static-adversarial.json` 初版 17 PASS/16 FAIL 包含基线 ZERO 断言失败，不能改断言凑通过。

## 分流与收口义务

这是**既有静态证明模型缺口，加上 T311 新热时钟原语的证明义务**；目前没有足够证据证明新代码发生真实分配，也没有证据放行零分配门。交 tester/静态工具维护者用同一前后源码、同根 manifest 和目标 ArkTS/HAP 版本完成定向证据：

1. 对 `Date.now`、既有 `TxWorkTrace.clock`、`P011HotWorkReceipt.begin/end` 给目标运行时原语的可核摘要或最小反例；若可能分配，明确调用路径与热态触发条件，回 architect 先修本工程合同再派 developer 移至非热计量或换有证明的原语。不得臆测 Date.now 永远无分配。
2. 对新增 18 净记录逐条提供根→调用链、冷热 guard、上下文预算触发点及可复核解析；对旧 39 显式位点/2 内建效应保留各自证伪或可达结论。工具精度不足标 `NOT_IMPLEMENTED/PROOF_GAP`，不设无据白名单、不放宽零分配断言。若发现真实热分配，由 architect 修订具体代码接口/生命周期后转 developer；若静态门要求超出其原约定能力，交 manager 裁用例口径，仍不记产品 PASS。
3. 比较诊断开关前后同输入 commit/事件/粒子结果与错误码，计时失败只影响诊断；禁止靠取消守恒、事务检查或缩短 dt 来消除报告成本。T311 逐行审计仍有 `Ω(R(N+C))` 读取，Date.now 的粗粒度/测量开销不能掩盖或否定剩余复杂度。T312 真机若仍 watchdog，凭阶段收据先回 architect 归因，不能将本静态 FAIL 自动解释成设备故障。

此轮无可归设计冲突的证据，不先派 designer。T311 开发交付及 T312 设备任务保持独立；零分配形式门保持 FAIL，具体收口需上述定向模型证据或热分配反例。GPU 主线不受 CPU V2 静态门阻断。
