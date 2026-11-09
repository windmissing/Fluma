# T412 · GPU V2 整数配额独立序列 oracle

2026-11-09，tester。不使用真机。依据 T401 §3 冻结合同与 developer-interface-T409-evidence-capture-v1.md（T412 消费条款）。

## 1. 交付物

| 文件 | 用途 |
|---|---|
| `test/t412/quota_oracle.cjs` | BigInt 独立序列 oracle；零依赖 node；不调用产品断言、不重复 T411 归档职责 |
| `test/reports/t412-baselines/` | 7 组基线资产（trace 输入+重放终态+violation 记录），`baselines-manifest.json` 索引 |

**命令**：`--selftest`（内置合成基线全绿自证）/ `--run <trace.json>`（核任意事件序列）/ `--make-baselines <dir>` / `--consume-t405`。

## 2. oracle 引擎（独立重放）

实现 T401 §3 全部冻结整数账规则，每步操作后强制断言：

- **Q1 carry 递推**：`u = carryBefore + 120×deltaNs`，`newQuanta = u div 1e9`，`carryAfter = u mod 1e9`；carry 域 `[0,1e9)` 越界即 `CARRY_DOMAIN` violation
- **Q2 pending≤4 丢最新**：`keep = min(newQuanta, 4-pending)`，余下记 overload；不删旧已计划/在途
- **Q3 守恒恒等式**：`generated = accepted + activePlanRemaining + pending + overloadDropped + revokedDropped`（任意时刻锁内快照），破坏即 `INVARIANT_BROKEN@<位置>`
- **Q4 GAP 撤权**：delta>33,333,333ns → pending+活动未接受全部转 revoked、fractional carry 清零并登记（`revokedCarryLog`）
- **Q5 reset**：先冻结旧账终态（`runs[]` 含完整 snapshot）再建新账归零
- **Q6 SOURCE 不占账**：source-only 操作不改变任何量子字段；模拟秒 = accepted/120 整数 tick
- **Q7 重复 CURRENT 不增新帧**：重复呈现同 commit 即 `DUPLICATE_CURRENT_FRAME` violation

## 3. 基线资产（7 组，覆盖卡面全部边界）

| 基线 | 合同条款 | 覆盖 |
|---|---|---|
| B1_fast_pulse_overload | Q1+Q2 | 11 次满窗 33.33ms 脉冲→pending 触顶 4、generated=43/overload=39、gen−overload==pending |
| B2_normal_consume | Q1+Q3 | 生成→领取→接受 3 量子，accepted 精确 |
| B3_gap_revoke | Q4 | 2 脉冲建账→take→40ms GAP→revoked=4、carry 清零、第 3 脉冲新账恢复 |
| B4_reset_freezes_old | Q5 | 消费后 reset→旧账终态冻结（generated/accepted 保留在 runs[]）、新账归零 |
| B5_carry_boundary | Q1 | 30 脉冲 carry 累计近 1e9 不越域 |
| B6_source_no_ledger | Q6 | sourceOnly 全程账逐字段不变 |
| B7_duplicate_current | Q7 | 重复 CURRENT 必产生 violation |

## 4. 自测结果

**15 ok / 0 bad**——7 基线全程 invariant 无破坏+逐基线精确数值断言全过；附加负控：手工篡改 `generated` 后 invariant 函数必检出的对拍验证通过。

数值与手工 BigInt 逐轮推算交叉验证一致（如 B1：每脉冲 q=3/4 交替、carry 稳定 999,999,560–960ns）。

## 5. 消费 T405 已交证据

`--consume-t405` 输出（基于 tl2-manifest statusBefore，实际 revision=GRS_M1_PROGRESS_CLOCK_V1）：

- 配额五字段（generated/accepted/pending/overload/revoked）**全零** → 无非零 V2 配额数值账可核
- oracle 判定：**NOT_EXERCISED**（非 FAIL——合同要求"有账时账必须对"，T405 归档根本无账；与 T407 §2 "pageSession4/quotaBalanced=true 是 V1 也追加的字段且账全零，不能作 V2 非零配额证明"裁决一致）

## 6. T409 对接与预期 RED 如实记录

- T409 接口文档（developer-interface-T409-evidence-capture-v1.md）已读并对接：T409 真机归档的 `initialLedger/terminalLedger`（八字段十进制字符串）与逐事件 ledger 原子快照到达后，oracle 可直接逐行核对；**直方图不是可重建每笔 pulse 序列，不从聚合反推未归档精确序列**（接口文档明示，oracle 不会做此违规重建）
- T409 交付物现状：docs 有接口文档、`T409-handoff-signed.hap`（sha256 `09085481…63007`）已到，但 **无真机归档证据**（无 GRS_EVIDENCE_CAPTURE 导出回执/manifest）→ 真实账核对项**预期 RED**（NOT_EXERCISED），待 T409 真机归档到件后转可执行

## 7. 边界声明

- 未改账规则换 PASS：全部预期值由独立重放引擎按 T401 冻结公式计算，无任何容差或事后调整
- 不重复 T411 职责：本 oracle 只核配额序列语义，不核文件/SHA/分片
- 无阻塞问题；本卡可完成收口
