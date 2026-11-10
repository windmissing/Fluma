# T471 · GPU 对称采集脚本与 lease 补证（v1 阶段报告：D/E 判 NOT_MEASURED，驱动器设计缺陷如实移交）

2026-11-10，tester。不使用真机、不改产品。按 T465 §3 D/E 执行；结论先置：**两包均未能以可复现原件完成，判 NOT_MEASURED**，驱动器线程化设计缺陷与 SwiftShader 设施限制如实移交，不冒 PASS、不把未固化差异标环境噪声。

## 交付

| 文件 | 内容 |
|---|---|
| `test/t471/symmetric_diff.cpp` + `build.cmd` | 对称脚本差分器（预生成 40 拍调度表 / 共享安全点 reserve / 排空终止 / 240s lease 守护） |
| `test/reports/t471/t471-run-symmetric-pairs.txt` | 最近 run 原始判定记录（含真实分歧） |
| `test/reports/t471/pulse-log.ndjson` | 最后一次 run 的逐 pulse 快照（off 臂 19 拍） |

## 执行过程与发现

1. **第一版（修 T459 两缺陷后首次运行）**：预生成调度表 + 两臂等 `captureCommittedEpoch` 共同前置 + 固定脚本耗尽终止。该版完整跑完两臂，判得 **全账 8 项 PAIR_OK + completion 空-空等价 + NO_OBSERVATION_DISTURBANCE**，且 on 臂 `RESERVED` 在共享安全点 pulse=10 成功。但 summary/ndjson 未落盘（当时无先落盘逻辑），且随后迭代覆盖了该 run 的 log——**可复现原件未固化，该结果不作为证据引用**。
2. **try_lock 版（修 on 臂 19 拍停滞）**：`try_to_lock` 跳拍反而引入新输入不对称（off=17 / on=34 pulse 数不同），判定 `acceptedCount/generated/overload/clearedCarry` 真实 DIVERGE——符合 T465 "真实同输入下出现分歧回 architect" 的前提，但**该分歧源于测试驱动器跳拍（两臂喂入 pulse 数不同），非产品差异**，不能作为产品判定。
3. **E 包 lease 段**：同真实 transport 的 Acquire→release fence→未 signal 重 Acquire→signal/Retire→STALE→新代合法再 Acquire 用例已实现，但 SwiftShader transport Create 长阻塞（T459 同域）触发 240s 守护；守护超时后未再归因 Create（T465 要求），E 判 **NOT_MEASURED（设施限制）**。

## 移交 architect（T471 续办前置）

- **D 包驱动器重设计**：pulse 调度应改为 worker 循环内消费固定队列（单线程顺序消费，两臂只切 "是否执行 reserve/END 副作用"），避免独立 pulses 线程与 worker 的锁竞争/跳拍。当前双线程+try_lock 方案结构性无法保证两臂同喂入。
- **E 包设施**：SwiftShader `Create` 长阻塞需先解决（T462 真机诊断/离线 JIT 缓存预热），或提供 lease 专用轻量 transport 入口（只 Create 一次复用多次）。
- 已证可保留（不作为 PASS）：on 臂 RESERVE 在共享安全点成功（T417 回执原件在 run log）；T459 的两缺陷（动态时钟/提前停）已在本版修正——修正方案本身可复用。

## 未完成（如实）

- D：可复现对称原件未固化 → 全账对拍/首错矩阵/逐 pulse 两域序列 NOT_MEASURED。
- E：lease 全链 NOT_MEASURED（240s 守护，未归因 Create）。
- 结论：T459 的 "无观测扰动成立" 维持其原卡范围（同输入脚本时钟单臂对照），**T465-D 要求的更强对称差分未完成**。
