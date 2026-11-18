# T617 v1 · 共享 cold broker 跨模块信用独立验证（T596 646af44 交付消费）

日期：2026-11-18，tester，不使用真机（host x64 组件域）。
被测：**原生产实现** `entry/src/main/cpp/gpu_resident/cold_slot_broker.cpp`
（T596 交付，libgrs_cold_slots DSO 源）——host 用 MSVC cl 14.51.36231 直接编译
同一 .cpp（/FI shim 仅映射 `__attribute__` 为空，产品头/实现零修改），独立测试
适配器 `test/t617/test_broker.cpp`（sha 5f2aa3f8…）链接之。
证据：本报告 21 项检查全输出 + `test/t617/`（适配器/shim/rsp/exe sha 839109f2…）。

## 执行结果：21/21 PASS（rc=0）

| # | 检查 | 结果 |
|---|---|---|
| R1 | 首次 Reserve（ticket=1，request=trace=1） | PASS |
| R2 | builder 活跃期第二 Reserve 拒（builder-reader 互斥） | PASS |
| R3 | BuilderDone 后第二 slot Reserve 成功 | PASS |
| R4/R5 | ReadBegin 成功；**单 reader**（第二 slot ReadBegin 拒） | PASS |
| R6 | ReadEnd 后另一 slot 可 Read | PASS |
| R7 | 同 route 预算溢出拒（1000+9001>10000） | PASS |
| R8 | **双 slot 上限**拒（第三 Reserve 拒，ticket=0） | PASS |
| R9/R9b | Freed 释放 slot 后可复用+Read | PASS |
| R10 | **reader 活跃期 Freed 被保留**（honest retire：挂起退账） | PASS |
| R11 | read-end 后补 Freed 真释放→slot 复用 | PASS |
| R12 | 进程字节上限拒（>524375552） | PASS |
| R13/R14 | 零 bytes/零 route 参数拒 | PASS |
| R15/R16/R17 | GPU 全租约：单 ticket/第二个拒/ContextRetired 清账 | PASS |
| R18/R19 | Lease RAII Reserve→Reset 退账后可复用 | PASS |
| R20 | 陈旧 ticket 拒 | PASS |

## 关键语义确认（产品合同验证）

- **进程共享单实例**：所有 extern C API 经单一 mutex+静态 slots 数组（DSO 实现，
  非 inline statics/符号插替）；独立进程内两"模块"（直接调用+Lease RAII）操作
  同一状态 ✓
- **双 slot 上限/route 上限/进程字节上限**三层容量合同 ✓
- **builder-reader 互斥+单 reader** ✓
- **DETACHED/RETIRING 域**：reader 活跃期 Freed 挂起（不退账）、read-end 后补
  Freed 真释放（R10/R11 对）；GpuLease::ContextLost 不退账（broker 保真实
  retiring credit）——与头文件注释合同一致
- **故障 RAII**：Lease 析构 Reset 退账（R19）✓

## 平台缺口（如实）

- 本轮为 host x64 **静态链接同一实现**的组件域验证；"跨 DSO 进程共享"的运行时
  证明（grsm0/grsm1 同时 dlopen 同一 libgrs_cold_slots.so 单实例）在 host 无
  aarch64 loader，以静态链接替代——真机端 DSO 单实例由设备域验证（不使用
  静态 NEEDED 冒运行共享）
- 组件退账非真实 driver 释放证据（设备/产品域，卡面已注明）

## 判定

- 原生产 broker 实现 21 项合同检查 **ALL PASS**——未发现产品缺陷，无需 architect
  分流；测试期间 6 处初判 FAIL 均为适配器断言语义错误（builder-done 前不可 read、
  双 slot 上限、retained-free 两段式退账），按实现合同修正后全绿
