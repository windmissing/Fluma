# T172 GPU 隔离 V3 独立验证报告

日期：2026-10-27。任务：6ae06c77e4b0127832f14a0d（tester，离线，不使用真机）。被测 commit：**961d837**（T171 开发交付）。当前树 HEAD：c797ea9。规格：`docs/architecture-T169-gpu-load-fault-contract-v1.md` §2–5 + `docs/developer-T171-gpu-isolated-v3-v1.md`。

## 执行器与独立点

`test/p172_v3_independent.cjs`（13 用例）。独立性设计：

1. **独立参考选对**：本执行器重实现 LEGACY_SCAN 语义（a=未合并最小 id、b=满足谓词的**最小 id**、全槽位扫描），GPU 臂 stand-in 调用该参考而非 `w.mergeIndex.find`/T159Selector 代码路径——独立预期不与被测实现共享排序核。修正过程如实记录：首版参考"首个匹配即返回"与原"最小 id"tie-break 不同导致 GPU poolSha 分叉（0ee7cf0b vs 44012266），修正语义后三路一致——该分叉本身验证了参考的独立性（非镜像实现）。
2. **拒绝矩阵独立推导**：21 个拒绝边界（含开发自测未列的 steps=-1/2.5、repeatId=-1、timingMode 错值、blockId 65 字符/非法字符、queuedUptime 负数、output 空、revision V2、route 未知、非 FAULT 带故障等）全部 V3_ 前缀拒绝，合法对照接受。
3. **物理一致性断言独立构造**：每 PAIR 行核 idA<idB（selector 语义）、selectorCalls=pair+noPair+error 计数恒等式、13 组 poolBytes hex 拼接 SHA（编码口径显式：hex 解码拼接，回应 T169 §1 对 T164 报告编码含混的修正）。
4. **T171 开发自测独立复跑**：10/10 + 2/2 先行确认基线绿。

## 结果：13/13 PASS（原始 JSON：test/reports/t172/independent.json）

### A. 三路完整事务一致性（4 fixture × 3 路全部一致）

| fixture | selectorCalls | mergeCommitted | poolBytes SHA(hex 拼接) | pairChecked |
|---|---|---|---|---|
| WET_192_64 | 64 | 64 | 0ee7cf0ba566… | 64 |
| WET_192_128 | 128 | 128 | 05e898dfda7e… | 128 |
| WET_512_64 | 64 | 64 | 098d89de6672… | 64 |
| WET_512_128 | 128 | 128 | 13f4cf38b469… | 128 |

每 fixture 三路（LEGACY_SCAN/T123/GPU 臂）在 normalized after 状态、pairRows（剔除时钟/身份槽 4/5/12/30/31）、event/refund/lineage 原 F64 hex、预算/审计/回滚/λ 计数上 deepEqual；merge 实际提交 64/128 与 T169 §2 预测口径一致；GPU 臂为标注的 CPU interface stand-in，**不冒设备 GL 执行**。

### B. 协议拒绝边界（21 项独立边界 + 合法对照）

steps 0/33/-1/2.5、fixtureSha 全零、oracleMode BOTH/PERFORMANCE+ON/CORRECTNESS+OFF、假故障名、FAULT 用 CPU 路由、FAULT+NONE、非 FAULT 带故障、route 未知、repeatId=-1、timingMode 错值、blockId 65 字符/含空格非法字符、queuedUptimeMs 负、output 空、revision V2——全部以 V3_ 前缀错误拒绝。

### C. 性能 schedule 独立复算

99 = 9 warmup（3/route）+ 90 稳态（30 block × 三路各一）；六种 route 排列各恰 5 次；seed13 LCG 确定性复现（两次生成 deepEqual）；FAULT schedule：QUARANTINE/STALE 类故障+恢复两块、OWNER_EXHAUSTION 仅一块（同进程无恢复尝试，与"新 OS 进程恢复"合同一致）。

### D. 容量溢出诚实语义（独立构造）

公开 `selected()` 接口驱动 16390 次调用超过 16384 定容：overflow=true 置位、selectorCount 仍如实计 16390（数据不静默截断冒完整）、reset 可清。物理臂 overflow=false 且超出即 V3_EVIDENCE_OVERFLOW_INVALID_MEASUREMENT 中断（Worker 源码核）。

### E. 计时身份与分母

nativeTimings 32 槽位与合同逐一对应（0..14 计时、15 bridge、16..29 身份 lo/hi、30/31 role/ordinal）；未完成阶段 -1 → NaN → JSON null（非 0 冒测得）；双时钟域分列（ETS OHOS_ACTIVE uptime vs native steady_clock，标注 SEPARATE_DOMAIN 不互减）；gpuTimer 常驻 'NOT_MEASURED'（无 timer 通道，fence 只称 CPU 等待）；嵌套区间不累加（prepare 分项与 prepare 总并存、validation/consume/returnEncode 独立槽位）。

### F. manifest/schema/日志/注错语义

- 四 fixture rawfile 字节 SHA 与内嵌声明逐一重验；fixture JSON 内容（nmax/packages/massPackage/column/sourceId/synthetic）与声明一致；configId 绑定 fixtureSha256+parameters+steps+oracle+fault（任一变化即不同 id）。
- compile/link 日志：成功也查 INFO_LOG（captureLogV3 两处调用点核）、状态三分离（CAPTURE_FAILED/CAPTURED/EMPTY）、SHA 早退为 NOT_RUN、4096 预算截断带原长标志、NUL 按返回字节保留、采集错误与 firstFailure 分离（源码注释级合同核）。
- 注错计划：单次武装/一次消耗（request==0+NONE 才可 ARM）、kind 白名单、OWNER_EXHAUSTION 仅冷态、run 绑定拒绝、release 预处理产物无 armTestInjectionV3 符号、QUARANTINE 观察判定要求真实 rawGlError+TEST_ILLEGAL_UPLOAD_TARGET（禁合成错误码）、OWNER_EXHAUSTION 恢复标 NEW_OS_PROCESS_REQUIRED_NOT_ENV_RESET。
- V3 overlay 逆 diff：每笔编辑 round-trip SHA 往返独立重验（current→revert→originalSha256），9 项白名单全过；生产 merge 算术切片（pMass[a] 至 mergeOldestHot）与主工程逐字节一致。

### G. 基线独立复跑

T171 开发自测 10/10（gpu_v3_units）+ 2/2（worker_pipeline，PERFORMANCE 99 attempt/MICRO 162 请求在 SDK+NAPI CPU 桩）全过，作为被测基线确认。

## 边界与未覆盖（如实）

- GPU 臂是 CPU interface stand-in：本报告不含设备 GL 执行、真实故障注入、收益或 UI 竞争结论——归 T173 真机。
- HOST doubles（t171_binding_test/shader_source_test）经 native-units.txt 归档确认，但其 NAPI/EXECUTOR 为测试替身，不代表设备 GL 原码/生命周期。
- 生产迁移未授权；本报告不提供任何设备或产品 PASS。

## 结论

T171 V3 在独立预期（非镜像选对核）下：三路物理/事件/refund/rollback/原 F64 完整一致（4 fixture 全过）、协议拒绝边界完备（21 项）、schedule 确定性复现、容量溢出诚实、计时身份/缺测/时钟域符合合同、manifest/日志/注错单次语义全核。**T172 无阻断，T173 真机可执行**。失败上报：无。
