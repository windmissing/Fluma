# T410 · GPU V2 同窗双源与负控真机补证（v4 阶段）

- 日期：2026-11-11
- 角色：tester（真机 62T0225B10010020）
- 结论：**通道复现确认 + 阻断根因闭环**。T467-t410 包（新代生成）复现 T450 冷启 SHA 通道；实际 V2 采集 arm 被生产 fail-closed 正确拒绝；根因链由 T477/T472 闭合（coherent shader）。
- 证据：`test/reports/t410-v4/`（截图×4+布局）
- 前序阶段：v3（e51ed33）已证 T450 冷启 SHA 通道端到端 + V2 open 无自愈发现。

## 本轮新增

1. **T450 通道在新代包复现**：T467-t410-final 包 Want `grsm1/gpusha` 冷启 → 诊断页 `SHA_DECLARED 63b0fd25…fa706` 与卡面一致；下轮进度时钟默认 V2。
2. **实际 V2 run 建立**：新空场 → run 0:2 / epoch 4 / session 2（与 T473 恢复事务同代际，探针已证该代 READY 曾达成 epoch4==ready）。
3. **arm fail-closed 复现**：`开始2秒同窗采集` → `STALE_OR_UNREADY_EPOCH epoch` 正确拒绝（GAP 撤权后 V2 无自愈，页面旧 session 引用 stale——T410 v3 发现持续成立）。

## 阻断根因闭环（新增）

设备 READY 窗口不可用的根因链已在 T477/T472 闭合：**Maleoon 驱动 shader 编译拒绝（GLSL 1138 行 coherent 限定符）→ Create SHADER_FAILED/体验链 GPU 提交卡顿 → GAP 无法恢复**。coherent 修复（developer）是 T410 双源/六负控/30fps 全部 NOT_MEASURED 项的唯一前置。

## 维持 NOT_MEASURED（不冒通过）

- 实际 V2 采集 ARMED→FROZEN 全链、双源（同窗+跨窗）、六负控、30fps：依赖稳定 READY 窗口。
- T437 前缀/完整同包配对：同域阻断（通道证据已在 4d476b0 交付，本卡不重装重复）。
