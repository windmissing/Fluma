# T160 支持纯核实施独立验证（tester 报告 v1）

日期：2026-10-27。角色：tester。不使用真机。目标：T135 6ee7e19（`P011WriterKernels.ets` V2 + `P011WriterRNGraph.ets`）。
执行器：tester 自有 `test/p160_kernels_independent.cjs`（独立 BigInt 有理数 oracle 与自有夹具，不镜像开发 14 例）。结果 `test/reports/t160/p160_results.json`。

## 结论

**15/15 PASS。T135 支持纯核 V2 实施独立验证成立：逐 node 包络（ideal/error/η）被独立精确有理表达式包含、settle A≠1/MIX b≠0/CONTACT_WAIT partial-full LAND/INWARD 两次预览/原子影子事务（before/token 失效/一次消费/篡改免疫/失败不写）全部实测通过；未发现缺陷。** 开发 14/14 复跑仅作交叉参照；T134"模块不存在"RED 为历史基线自然退出（模块现已存在，数值/原子判据全保留并独立执行）。

## 独立验证覆盖

| 用例 | 实测 | 结论 |
|---|---|---|
| W1 支持域 | settle/top/land CONTACT_WAIT=SUPPORTED；PERCOLATE=STRICT_WRITER_UNSUPPORTED；AIRBORNE=CONTACT_HOLD_UNSUPPORTED | PASS |
| W2 settle RN 相 | RAW、phaseSum=0.175（独立位型比对 0.05+0.125→0.175）≠qGranted 0.15；**A 区间 [1.1667,1.1667] 排除 1**（T134 夹具） | PASS |
| W3 settle 提交 | mh/ml=phase、suspH/suspL=1−phase（守恒）、snapshotSerial 恰进一次 | PASS |
| W4 top MIX | 2 列 patch、receiver 恰收记录 phase 值 | PASS |
| W5 MIX b | 二项 T134 语义：stored M=0.30000000000000004 ≠ 精确和；b 区间非退化且覆盖残差量级 | PASS |
| W6 后两 top 分支 | MIX_LIGHT/MIX_LIGHT_HEAVY 均达 RAW、分支名正确（>20 node） | PASS |
| W7 partial 2⁻⁵⁴ | RN(m−q)=m 禁止 → ZERO、poolMass 不动、无事件、ticket 拒绝态 | PASS |
| W8 partial 2⁻⁵³ | nextDown 允许 → poolMass 恰减、ID 保留 | PASS |
| W9 full retire | 槽 state→0、ID 保留、liveCount→0、退役槽恰释放一次 | PASS |
| W10 INWARD | 2 次预览（RAW→INWARD）、qControl∈(0,hardMax)；提交保留 ID/source/motion 全 provenance、退款=debit−qControl 恰一次、event+lineage 各 1、waterTop=cache+surface | PASS |
| W11 token 单消费 | 二次同 token → STALE_TOKEN、全场零变化（JSON 快照比对） | PASS |
| W12 参与方核对 | 6 类篡改（列/source/pool/ticketDebit/identity/freeSlots）全部 IDENTITY_MISMATCH 且零写入；**篡改公共 afterBits/qControl 无法改密封写入**（密封值胜出） | PASS |
| W13 旧 token/±0 | 新 preview 后旧 token → STALE_TOKEN；ZERO 保留 ±0 位型、无 pool/event 写、ticket 拒绝态 | PASS |
| W14 settle 包络 | 54 node：每个 ideal 区间含独立精确表达式、error 区间夹 actual−ideal | PASS |
| W15 top+LAND 证书 | 全 node 重建精确；top 的 CanonicalS δ0 区间显式非零 | PASS |

## 期间测试侧修正（非判据变更）

首版执行器 4 处断言错误（W2 手工 BigInt 式 `175n/1000n` 截断为 0，改用 dy 位型比较；W5 夹具误用四项和使 RN 恰吞偏置 b=0，改回 T134 二项语义；W9/W12 夹具混用 INWARD/RAW 路径致 retire/mh 期望错位，按开发合同路径重设），修正后 15/15。oracle 修正均记录于执行器注释。

## 边界与声明

- materialOnly 域：不冒预算/pool 全事务证明；生产 strict 路由未接，**不签设备 code5 修复**。
- AIRBORNE/CONTACT_HOLD、未登记 writer 仍 UNSUPPORTED（W1 实测）。
- 开发 14/14 复跑 PASS 仅为交叉参照；本卡全部判据来自独立 oracle。
- 无产品/阈值/判据变更；T148 除法纠正（负号正确）未被触碰。

交 manager 验收。
