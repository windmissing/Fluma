# T190 GPU 旧证据归档勘误报告

日期：2026-10-28。任务：6ae1451be4b0c3103c03cdc2（tester，离线，不使用真机）。消费：T183 f445881（`architecture-T183-gpu-cost-next-v1.md` §7A/§2/§5 + `analysis-T183-existing-evidence-v1.json`）。所有权：T173 正式报告/归档补件，不改实现，不复跑设备。

## ① 五份 fault 原文正式归档：PASS

五份 fault JSONL 从 `.codex/local-test/` 原字节复制入 `test/reports/t190/`，SHA256 与 analysis JSON 逐文件匹配（OWNER 3c16685c…/QUARANTINE aecc0979…/SHADER eb53ebec…/STALE_CONSUME 60606126…/STALE_REQUEST 5b96ba1e 前缀），**原 bytes/原 run/原 PID 未手改**。原 `.codex/local-test/` 文件保留原位。

## ② 十文件 manifest：PASS

`test/reports/t190/ten-file-manifest.json` 从实际十文件（5 正式+5 补归档）生成 SHA/bytes，全部与 analysis 一致。

## ③ source 身份纠正：PASS

- **纠正**：source manifest SHA 实际为 **fac20656756675b6…**（从实际 manifest 重算一致）——T173 报告所写 ed26af35… 是 V2 生成器 source 身份，非 V3 运行身份。
- **obj/包内 SO 分列**：构建 obj SO（1593216B，0860560f…）与签包包内 SO（360208B，4a13fd52…，stripped）为两个不同物，均已单独归档，不再混写。

## ④ §2 正确口径统计（旧值标历史错误）

`test/reports/t190/correct-stats.json`：warmup 9 + 稳态 90（每 fixture 每路 30 稳态），p50=偶数取中间两数均值，p95=nearest-rank ceil(.95·n)；GPU 逐 block 与 T123/LEGACY 同 repeatId 差全量保留。稳态口径：

| fixture | 路由 | 稳态 p50 | p95 |
|---|---|---|---|
| 192_64 | GPU | 1750.5 | 1792.0 |
| 192_64 | T123 | 1727.5 | 1773.0 |
| 192_64 | LEGACY_SCAN | 1428.5 | 1456.0 |
| 512_128 | GPU | 4852.5 | 4888.0 |
| 512_128 | T123 | 5918.5 | 5952.0 |
| 512_128 | LEGACY_SCAN | 3959.5 | 3984.0 |

**勘误**：T173 报告的 p50/p95（1747/1785 等）按 n=33 含 warmup 且非 nearest-rank——**标记为历史错误，原值在原报告保留不改写**；新值以本 JSON 为准。结论方向不变（LEGACY 两档最快、GPU 无收益显现）。

## ⑤ 失败事务成功前缀核（§5，非 rollback 计数代替）

`test/reports/t190/fault-full-state-audit.json` 逐 fault 审计：

- **QUARANTINE_GL**：FAULT_OBSERVATION 保留完整成功前缀计时（requestCpu/prepare/fence/upload/dispatch/map 均非 null）+ firstFailureStage=TEST_ILLEGAL_UPLOAD_TARGET；physicsBefore/After 在行内。
- **STALE_CONSUME_HI**：计时保留（真实执行完成），firstFailureStage=OWNER_CONSUME_STALE。
- **STALE_REQUEST_HI**：identity 校验在计时区间前拒绝——prepare/fence 语义上不存在，requestCpu=null 为真值非缺测。
- **SHADER / OWNER**：configure 阶段失败/无请求，无 ATTEMPT 计时为真值。

### 不可复原字段逐项清单（`unrecoverable-fields.json`）

| 字段 | 为何不可复原 | 补救 |
|---|---|---|
| FAULT 行 headerWords/resultWords/identityBytes | reply() 失败分支不 put fullDump 字段；GL 缓冲已随 reset 释放 | T183 §7D 受控单故障取证 + 失败分支导出 |
| STALE_REQUEST_HI native 计时 | 拒绝分支语义上无该区间 | 无需 |
| SHADER run ATTEMPT 计时 | configure 失败无请求 | 无需 |
| OWNER 恢复新 PID 段 | 设备 run 未含恢复进程取证 | §7D 真机新进程取证 |
| UI 接收时延 | JSONL 标 NOT_YET_MEASURED；hilog 仅有 UI 收到时刻 | PARTIAL 可由 hilog 关联 |

**owner 新 PID 恢复：NOT_RUN**（设备 run 未含真实退出/新 PID 取证）。**UI 接收：未测完整链**。**日志未执行分支（拒编/link 失败日志）**：本轮 shader 一次编译成功，无该分支样本，如实声明。

## 结论

T190 归档补件完成：五 fault 正式存档（hash 匹配）、十文件 manifest、source fac20656 纠正、SO 双身份分列、§2 正确口径统计（T173 旧值标历史错误）、§5 成功前缀核 + 不可复原清单五项。**完成回 T183 续判缺证**；设备未复跑，无产品结论变更。
