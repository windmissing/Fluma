# T142 GPU 原型真机差分与收益（tester 报告 v1）

日期：2026-10-27。角色：tester。真机 VYG-AL00（62T0225B10010020，Mali B318，GLES 3.2 B318.bd51c15），同机串行，未打断在途 run。
被测：T141 cd7f517（isolated GPU pair prototype）。执行器：tester 自有真机诊断 `test/p013_compute/t142_device_diag.cpp`（复用 T141 executor/prepare/owner 代码原样，仅加 dump 与采样；原 T141 源文件零改动）。

## 结论总览

**真机实跑完成定点正确性验证与同步成本分项计时；发现两项 T141 交付缺陷（F-T142-1 wire echo 错位、F-T142-2 GLSL 保留字），完整事务三路收益主判定 NOT_MEASURED，须先回 architect 分流修复。** 算法核心（build/query/reduce 三层 + pair 选择）在真机上与独立 oracle 完全一致；失败集中在结果信封 echo 区与 shader 词法，均为 T141 离线阶段无法暴露的真实设备差异。

## 一、真机定点正确性（实测）

| 验证项 | 实测 | 结论 |
|---|---|---|
| 三 shader 真机编译 | build/query 首跑拒编（F-T142-2），tester 侧改名副本后编译通过；reduce 未改动 | F-T142-2 定性后可编译 |
| dispatch 数 | 192→11（h8+1+reduce2）、512→12（h9+1+reduce2） | 与合同一致 PASS |
| 128B 结果读回 | map/unmap 成功、waitReturn=CONDITION_SATISFIED、waitCalls=1 | PASS |
| T140-192 fixture（设计无匹配） | GPU status=2 (NO_PAIR)，w[4..9] 全 0 | 与冻结期望一致 PASS |
| T140-512 fixture（冻结 pair） | GPU `idA=16,idB=342,slotA=15,slotB=341,aRank=154,bRank=155` | 与 legacy 原扫描 (15,341) **逐字段一致** PASS |
| 合成池（diag 默认池，legacy found (4,132)） | GPU `idA=4,slotA=3,idB=132,slotB=131` | 与 legacy 一致 PASS |
| CPU 精确 range oracle | 两池 rangeOracle=true | PASS（GPU 外 CPU 侧） |
| slot/epoch/identity 协议 | executeIntegerPair 身份检查、slot 状态机往返正常 | PASS |

**注意**：T141 offline.json 声称 192 synthetic 池 `oldPairFound=false`，tester diag 同参数下 legacy found=true——T141 offline 的 192 行实际跑的是 T140 fixture 192（无匹配），其内置池与 diag 内置池结论并不矛盾，但 T141 报告未区分池来源，登记为报告清晰度问题（非缺陷）。

## 二、两项交付缺陷（交 architect 分流）

**F-T142-1（wire echo 布局错位，阻塞性）**：`pair_reduce_v1.comp` final 分支 echo `R[3..7]=H.h[7],H.h[3],H.h[4],H.h[13],H.h[14]`（uvec4 索引），而 `pair_owner_v1.cpp` 逐 word 检查 `w[16..17]==h[14..15]、w[18..19]==h[16..17]、w[20..21]==h[18..19]`（**跨 uvec4 边界的 header words 14..21**）与 `w[22..29]==h[48..55]`（identity SHA）。任何 GPU 成功结果都会被 owner 拒为 `PROTOCOL_ERROR` → `ORACLE_MISMATCH`/AGREE 判定**永远不可达**。pair 本体 w[4..11] 不受影响（与 oracle 一致）。修复需 architect 裁定以哪侧为准（T137 合同 result 布局），回 developer 同步改 shader 或 owner。

**F-T142-2（GLSL ES 保留字，部署阻塞）**：`pair_build_v1.comp` 用 `half`（GLSL ES 保留关键字）、`pair_query_v1.comp` 用 `length`/`stop`——Mali B318 驱动拒编（7 errors：`Expected token 'identifier', found 'half'` 等）。T141 离线 host（ANGLE GLES3.0，无 ES3.1）不编译 shader，故未暴露。tester 已在设备部署副本上做纯改名（half→halfM、length→segLen、stop→stopAt、start→segStart，算法逐字不变）绕过，改名副本 SHA 与原版 SHA 均已归档 `t142_source_sha.json`。正式修复归 developer。

## 三、同步成本分项计时（真机 5 轮/池，T140 fixture 输入）

| 分项 (ms) | 192 p50 | 192 p95 | 512 p50 | 512 p95 |
|---|---|---|---|---|
| uploadCpuMs | 0.007 | 0.058 | 0.007 | 0.059 |
| dispatchCpuMs（11/12 次 dispatch+barrier 提交） | 0.041 | 1.240 | 0.047 | 0.999 |
| fenceWaitCpuMs（GPU 实际执行等待） | **0.723** | 1.390 | **0.762** | 3.000 |
| mapCpuMs | 0.012 | 0.051 | 0.002 | 0.013 |
| requestCpuMs（单请求总墙钟，不含 prepare） | 0.861 | 2.830 | 0.871 | 3.097 |

观察：fence 等待 0.7ms 级且与 N 弱相关——**单次同步 pair 查询总墙钟 ≈0.87ms**，是 CPU 权威扫描/索引单次开销的量级参照；但 GPU timer NOT_MEASURED（无 timer query 通道），CPU 等待与 GPU 执行重叠部分未拆分。冷首轮 dispatchCpuMs p95≈1.2ms（首次派发驱动路径），与 T131 微基准结论一致。

## 四、NOT_MEASURED / NOT_RUN（如实登记）

- **完整原 Hot 事务三路收益（旧扫描 vs T123 CPU 索引 vs GPU，逐 pair+最终 pool 字节+预算/ID/事件）**：NOT_MEASURED。F-T142-1 使 GPU 结果无法通过 owner 消费进入 `mergePairHot`，完整 attempt 链不成立。不以 CPU selector 替代 GPU 冒成功。
- **p50/p95 最坏/错误率跨冷暖多批次**：仅 5 轮/池的同步成本分项（上表），attempt 级分位不成立。
- **UI/输入延迟与渲染竞争**：NOT_RUN（无渲染集成）。
- **资源/context 恢复（epoch 2 采样）**：T141 executor 有 quarantine/epoch 机制但本卡未注入失败场景重测（F-T142-1 修复后随三路收益一并验证）。
- **GPU timer**：NOT_MEASURED（T141 未实现 timer query）。
- **variant 包（.codex/local-test/t141-variant）真机 HAP**：NOT_RUN。variant 仅含 ets 树，缺 module.json5/cpp/CMake 打包骨架与 `armT141GpuPair` 接线调用点（EntryAbility/DeviceRunAssembly 均未调），按 T138 §7"若当前测试包无法承载原事务闭包，先报明确工程阻塞"登记——**不构成 CPU selector 替代**，测试全程用真机 native driver 直跑隔离链。

## 五、真实包源 SHA（归档 t142_source_sha.json）

driver 03e9723e…、diag 911dbaf8…、T141 so 86871325…（=T141 manifest 记录一致）、原版/改名 shader 各三份 SHA、T140 fixtures 两份 SHA 全部归档。

## 六、判定

- 硬门槛（正确性）：定点 pair/NO_PAIR 与独立 oracle 一致 = PASS（受 F-T142-1 限制，消费链未通）。
- 收益主判定：**NOT_MEASURED**，采用建议不成立（也不触发"不采用"终局——缺陷修复后重测）。
- 下一前置：architect 对 F-T142-1 布局裁决 → developer 修 shader/owner → tester 重跑本报告全部验点 + 完整 attempt 三路。

证据：`test/reports/t142/`（t142_device_timing.json、t142_device_fixture_diag.json、t142_device_driver_first.json、改名 shader 副本、t142_source_sha.json）。
