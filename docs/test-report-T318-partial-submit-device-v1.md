# T318 GPU 有界首 root 真机闭环（41 阶段 Submit 分账 + 双源同 token）（真机，结束待验收）

2026-10-31，tester 真机（VYG-AL00 62T0225B10010020）。

## 0. 实际包身份（卡面验证目标 1）

- T317 交付的 `T317-final-signed.hap`（SHA `39075978…ed39`）实测为 **T317 首次准备包**：`libgrsm1.so` 内含 `T317_STAGE39_V2`、不含 `T324_STAGE41_V1`（byte 级检索），先安装跑出一轮 39 字典导出后确认不含分片实现（developer 交付说明亦明确"SignHap 00303116 已知失败、未手签不冒签包"——分片实现从未签包）。
- tester 按 T151 隔离签包先例自行补签：从 HEAD `84bf66e`（工作区 cpp/ets 零未提交改动）hvigor 重构（BuildNativeWithNinja 3.8s 实跑 + CompileArkTS 全绿；SignHap 00303116 已知失败如实保留）→ Python 重打包（原 zip central directory 触发 sign-tool 11017001，重打包后通过）→ hap-sign-tool localSign + verify-app **PASS**。
- **T318-signed.hap SHA256 `3b428b0a3b223836c785d92e80910c54fa648869bf6f5bc23b769a506ead965e`**，设备安装成功，页面 TextInput 外部输入该 SHA（非运行时自验），hilog `GRS_TIMELINE_COLD` 回读 packageSha256 一致。**注意**：本包为 tester 手签，包 SHA 与 developer 交付无对应签包的事实已如实登记，交 manager/architect 核对交付口径。

## 1. 41 阶段字典真机落地（卡面验证目标 2 前提）

- manifest `stageRevision=T324_STAGE41_V1`（42 项，idx40=SubmitRootBegin / idx41=SubmitCancel），COMPLETE / STABLE_PREFIX_FULL_TRACE，404 行 dropped=0；首次导出 GREW_DURING_EXPORT→INCOMPLETE 如实（fail-closed 生效），静置后重导 COMPLETE。manifest/part 文件 SHA256 与 hilog cold 声明逐位一致。
- 单 run 分账数据：`test/reports/t318/t318_submit_accounting.json` + `rows41.json`。

## 2. Submit 逐批分账（原 ns，TID 4161 单 worker，不钳值）

两个独立冷启 run（epoch3/run2 与 epoch4/run3），每 run 结构相同：**SubmitRootBegin 恰 1 次**（stage40，root=1）、5 对 SubmitEnter/Exit（1 首批 + 4 continuation）、50 个 DispatchEnter/Exit、**SubmitCancel 恰 1 次**（stage41 code=1=BATCH_ACTIVE 路径）、RootSubmit 恰 1 次、RootPoll 4 次、FenceEnter/Exit **0 次**。

### run1（epoch3/run2）

| 批 | serial | wall ms | exit code | dispatch 数 | ≤8ms |
|---|---|---|---|---|---|
| 0（首） | 92→114 | **1.712** | UINT32_MAX† | 10 | ✓ |
| 1 | 120→141 | 0.145 | 16 (OK_SUBMITTING) | 10 | ✓ |
| 2 | 147→168 | 0.098 | 16 | 10 | ✓ |
| 3 | 174→195 | 0.105 | 16 | 10 | ✓ |
| 4 | 201→222 | 0.212 | 16 | 10 | ✓ |

批内 wall 合计 **2.272ms**；批间隙 0.170/8.099/7.080/8.116ms；SubmitRootBegin→末批 Exit **24.80ms**；→SubmitCancel **32.97ms**；lastAnchor→PauseBegin **33.887ms**（PauseEnd code=6, flags=3, delta 原值）。

### run2（epoch4/run3）

| 批 | serial | wall ms | exit code | dispatch 数 | ≤8ms |
|---|---|---|---|---|---|
| 0（首） | 267→289 | **0.554** | UINT32_MAX† | 10 | ✓ |
| 1 | 295→316 | 0.228 | 16 | 10 | ✓ |
| 2 | 322→343 | 0.220 | 16 | 10 | ✓ |
| 3 | 349→370 | 0.220 | 16 | 10 | ✓ |
| 4 | 376→397 | 0.118 | 16 | 10 | ✓ |

批内 wall 合计 **1.340ms**；批间隙 9.767/9.692/9.858/1.075ms；Begin→末批 Exit **31.37ms**；→Cancel **41.08ms**；lastAnchor→PauseBegin **41.402ms**（delta 41407657 原值）。

† 首批 exit code=UINT32_MAX：`SubmitQuantum`（full_transport.cpp:57-58）return result 时未走 `observed.End()`，由 SubmitScope 析构器补 Emit(exit,root,UINT32_MAX)——代码路径事实如实登记（wall 差值不受影响），交 developer 确认是否补显式 End(16)。

## 3. 判定（逐项 PASS/FAIL/NOT_RUN）

1. **实际包身份：PASS（带手签登记）**——隔离签包全流程留证（sign/verify 输出在 `.codex/local-test/t318-sign/`），SHA 外部通道核对一致；手签与 developer 交付无签包的差异如实交 manager。
2. **逐批 ≤8ms：PASS**——10/10 批批内 wall 最大 1.712ms（首批含 staging+PinCandidate+BeginQuantum），全部远低于 kBatchBudgetNs=8,000,000；**单 dispatch 最大 691µs**（serial94 carry0，words 大 dispatch）→ 合同 §5"单 Pass 超窗"在真机**不存在**（无需 designer shader 分区回流）。
3. **SubmitRootBegin 每 root 恰一次：PASS**——两 run 各 1 次，nonce 一次消耗（无重复 Begin；submitting 期间无第二次 Submit）。
4. **批次总数与 pass 展开数一致：部分符合**——每批恰 10 op=10 dispatch（kBatchMaxOps=10 生效）；carry 序列 0..49 单调无跳（INV-3 不重排不跳过）；但 **130 项 pass 表（K=192/Kw=128 档）5 批仅发出 50 项**（见 §4 结构性发现）。
5. **总墙钟披露：PASS（字段通道）**——timeline 无新 status 字段（T324 §4 的 submitTotalWallNs 等 getter 未接 bridge status JSON），但 Begin→Cancel 总墙钟由 41 阶段原 ns 差完整可算（32.97/41.08ms 原值），分账不掩盖。
6. **完整链（双源同 token 上屏）：FAIL（结构性，非缺陷回归）**——两 run 的 root 均**未封尾**：5 批后（Begin 起 24.8/31.4ms）桥 CLOCK_GAP（33ms 窗，anchor→pause 33.89/41.40ms 原值）→ Pause→CancelSubmit（BATCH_ACTIVE 路径，root 永不 inflight/accepted）。130 op 表 ÷ 10 op/批 = 13 轮 Pump，每轮受 8ms tick 节奏间隔 ~8-9.9ms → 全表需 ~104-127ms ≫ 33ms GAP 窗——**有界提交的批进度速率与现行 33ms GAP 窗在真机结构性不配比，空场首 root 无法封尾，accepted=0，双源/同 token/上屏链 NOT_RUN（如实，不冒 PASS）**。
7. **提交期行为（CancelSubmit 两路径）：部分验证**——BATCH_ACTIVE 取消两 run 实测（code=1，PauseEnd code=6 flags=3，无 quarantine 崩溃，新 run 正常 epoch3→4 重建）；FENCE_PENDING 取消与 submitting 期间二次 Submit 拒 BUSY 未在真机复现（封尾从未到达，无 fence 可等）——非机证据已覆盖（T317 partial_host 19/19），真机此两项 NOT_RUN 如实登记。

## 4. 核心发现（交 architect/designer 分诊）

**真机 41 阶段数据首次定量显示**：单批 ≤8ms 达成（分片目标本身成立），但 **批预算 8ms×10op 与每轮 Pump 一批的 8ms tick 节奏，使 130 op 的 K192/W128 root 总提交墙钟 ~104-127ms，结构性超出 33ms CLOCK_GAP 窗 3-4 倍**——分片把"单调用卡死主循环"（T317 前的问题，现已消除：每轮 Pump 最多 1.7ms）转化为"长 root 永远被 GAP 取消"。修复方向属合同层配比问题（如：每轮 Pump 连续发多批直至预算耗尽而非单批、或 GAP 窗内提交豁免/宽限），**本卡不钳值不预派**，交 manager/architect。

## 5. 结论

- 分片状态机真机行为与 T324 合同逐项吻合（nonce 消耗、逐批 8ms、单调 pass 序、BATCH_ACTIVE 取消、无崩溃）。
- **根因结论**：有界提交在真机 GAP 窗下无法封尾 root（配比问题，非实现缺陷）；完整双源链 NOT_RUN。
- **结束待验收**：分账证据完整（原 ns 逐批），结构性发现与手签包差异移交。

证据：`test/reports/t318/`（manifest×3 + part0-2 + rows41.json + t318_submit_accounting.json + layout0-9.json）；签包产物 `.codex/local-test/t318-sign/`（T318-signed.hap SHA 3b428b0a…、verify-app PASS）。
