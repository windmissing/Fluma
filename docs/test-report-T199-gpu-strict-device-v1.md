# T199 GPU 严格优化设备对照接续报告

日期：2026-10-28。任务：6ae152f0e4b0f11bdf2418e1（tester，真机 VYG-AL00 62T0225B10010020，队首串行）。承接 T193（用户关闭无设备回执）完整 prompt、T183 §7D 与 T190 缺项。前置：T191 049300e + T192 732b48d 6/6 无阻断。

## 签包与身份绑定

| variant | unsigned | **signed（本卡）** | buildManifest 参数 | source |
|---|---|---|---|---|
| STRICT_O0 | df833640… | **f352958d58f74daa4d62f6e75c5b8be0fdef65706bc273267fe7ffadc59c5a29** | df833640… | 6ed2d9ea… |
| STRICT_O2 | 7a574b52… | **407d7e3950a661514b8c72967aad33314623d019849d0d5a75cefd2803901dfa** | 7a574b52… | e48340d3… |

每次运行独立安装对应 signed 包；PID 全程记录（O0: 49720/51897，O2: 50889/54020/57332）。测试后已恢复主工程 s139h 包，无权限/SELinux 修改。

## ① 正确性全 dump（CORRECTNESS WET_192_64）：O0/O2 字节等价 PASS

三路（GPU/T123/LEGACY）全部 TRANSACTION_RETURNED，selectorCalls=mergeCommitted=64：
- poolBytes SHA：O0 与 O2 **全部 0ee7cf0ba5664c26…**（与 T172/T173/T192 已知基线一致——设备上严格 FP 双优化档字节级等价）。
- after JSON 差异仅 2 个字段：`admissions/lastRecord/runId` 与 `source/lastRecord/runId`（每次 run 独立生成的身份，预期内）。

## ② O0/O2 同仪表交错三路收益（PERFORMANCE，稳态 30/路，全 TRANSACTION_RETURNED）

| 路由 | O0 p50 | O0 p95 | O2 p50 | O2 p95 | O2−O0 (p50) |
|---|---|---|---|---|---|
| GPU | 1586.0 | 1661.0 | 1598.5 | 1646.0 | +12.5ms |
| T123 | 1742.0 | 1771.0 | 1763.0 | 1809.0 | +21.0ms |
| LEGACY_SCAN | 1442.0 | 1475.0 | 1458.0 | 1486.0 | +16.0ms |

逐 block 差（同 repeatId，GPU−CPU）：
- O0：GPU−T123 mean=−147.3ms（GPU 快于 T123）；GPU−LEGACY mean=+135.8ms（慢于 LEGACY）。
- O2：GPU−T123 mean=−164.2ms；GPU−LEGACY mean=+146.2ms。

**收益结论（如实）**：STRICT_O2 相对 STRICT_O0 在本负载（物理 attempt 为主体）下**无可测收益**（各路由 p50 差 +12~+21ms，量级与块间波动同阶，GPU 物理核在 ArkTS/TS 解释执行路径上而非 SO 热路径内）——O2 只优化 native SO 常数项，与 T183 §6 预期一致，不宣产品收益。三路相对格局与 T173 一致（GPU 快于 T123、慢于 LEGACY，LEGACY 最快）。gpuTimer 仍 NOT_MEASURED；UI 竞争 NOT_RUN。

## ③ owner 旧 PID 耗尽 → 真实退出 → 新 OS 进程恢复：PASS（T183 §7D 缺项补齐）

完整恢复链首次真机取证：
1. **耗尽终态**（PID 57332）：DEDICATED_EXHAUSTION_INSTANCE 冷置 UINT64_MAX → 倒数代成功 → reset 后 configure 真实 `OWNER_GENERATION_EXHAUSTED`，记录 testProcessId=57332/testInstanceLo=1，标注 `NEW_OS_PROCESS_REQUIRED_NOT_ENV_RESET`。
2. **真实退出**：`aa force-stop` 后 `pidof` 确认空（进程真实消亡，非同 PID 新 env）。
3. **新 OS 进程**（PID 58333）：新 run t199ownerRecover 冷启动，GPU/T123/LEGACY 三路全部 TRANSACTION_RETURNED（sel=64）——新 owner configure/请求/成功全链走通。

## ④ T190 缺项精确补

- headerWords/resultWords/identityBytes 失败分支导出：本轮 FAULT 运行不含 SHADER_SHA_MISMATCH（§7D 范围内的 owner 恢复优先），字段仍属失败分支不 put 范畴；如需该字段需专项受控取证卡（维持 T190 清单登记，未回填旧计时）。
- 其余 T190 缺项（不可重建历史 raw）按登记保持 NOT_CAPTURED，未伪造。

## 结论

双 strict variant 设备对照完成：**字节等价在真机成立（O0/O2 poolSha 全等）、O2 无可测 attempt 收益（与设计预期一致，仅 SO 常数项）**、三路相对格局稳定；**owner 新 OS 进程恢复链首次真机完整取证 PASS**（T183 §7D 缺项闭环）。生产迁移未授权；O2 对照不替代整体复杂度方案（T197 线）。失败上报：无。
