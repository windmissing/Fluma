# T627 v2 · 跨 DSO broker 票据/信用独立 harness（T636 合同落实，真机执行）

日期：2026-11-19，tester，真机（011124264R000337）。
设施：`test/t636/`（adapter_a.cpp / adapter_b.cpp / driver.cpp——tester 所有权
按 architecture-T636-v1.md）；两个独立 aarch64 DSO（各自 DT_NEEDED 同一
**生产 broker 原字节**）+ driver，OHOS clang 交叉编译，hdc 推
/data/local/tmp/t636x 直接执行（非产品路径，无 GL）。
broker 身份：设备 md5 dec87ca5…=**T608 final release（sha256 715f314d…，8 符号
含 GPU 票据域）** 逐字节一致 ✓。

## 矩阵结果（17 步，16 符合期望）

| 矩阵 | 实测 | 判定 |
|---|---|---|
| M1 单 builder 门 | A.Reserve ✓(ticket=1)；B.Reserve 同 route/异 route 均拒；**B.ReadBegin(A.ticket)=1** | PASS（跨 DSO 票据语义成立） |
| M2 跨 slot 并存 | A reader 持有时 B.Reserve(route2) ✓；B 自 builder 票据 ReadBegin 拒；B BuilderDone 后仍拒 | PASS（单 reader 门隔离） |
| M3 Freed/reader 时序 | B.Freed(A.ticket) 时 A reader 持有→A.Reserve 拒；A.ReadEnd+再 ReadBegin ✓（ReadEnd 不自动 Freed）；全释放后新代 Reserve ✓ | PASS |
| M5 GPU 票据域 | A.GpuReserve(ctxA) ✓；B 同 ctx 拒；错 ticket Freed/错 ctx Retired 均不放行；B.Freed(A ticket) 后 B ✓ | PASS（3/4） |
| **M5 ContextRetired** | B.ContextRetired(ctxA) 后 A.GpuReserve(ctxA)=**0（期望 1）** | **语义发现**：ContextRetired 不释放已 Reserve 的 GPU 票据（T617 报告"ContextRetired clearing"措辞收窄——Retired 只标记 context 亡，需显式 Freed） |

按 T636 合同"不以单个 false 倒推拒因"：M5 成对证据完整（B.Freed 后 B 成功/
ContextRetired 后 A 仍拒），ContextRetired≠Freed 为实测语义而非推断。

## 结论

- **跨 DSO 票据/信用闭环成立**：单实例 broker、跨模块票据操作（B wrapper 操作
  A ticket 及反向）、单 builder/单 reader 门、route/进程信用、retained-free
  两相退休、GPU 全租约单票——全部实测验证（相对 T627 v1 的"仅加载映射"窄结论
  大幅推进）
- 组件域=BROKER_COMPONENT_CREDIT（无 GL/driver/生产 route 集成主张）；driver
  物理 free 另域不误派
- 日志原件：test/reports/t636/driver-log.ndjson（21 唯一行，≤30s 运行）
