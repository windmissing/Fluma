# T134：严格纯核测试先行（settle、top 分支与 CONTACT_WAIT LAND）v1

日期：2026-10-26。角色：tester。任务卡 `6adf48d1e4b017ab34058d27`。合同：T127 + T110 §2–§9（支持域 SETTLE/TOP_SPLIT(MIX)/LAND_RN_GRAPH_V1）。执行器：`test/p134_kernel_oracle.cjs`（独立精确有理数影子 oracle，逐 node 可溯；**12/12 PASS**，RED-01 为预期 NOT_IMPLEMENTED）。raw：`test/reports/t134/p134_results.json` + `p134_kernel_fixtures.json`（供 T135）。不使用真机；无生产 route 启用；不改阈值。

## 1. RED 基线（供 T135 阶段2）

`P011WriterEnvelope.ets` / `P011WriterKernels.ets` / `P011WriterRegistry.ets` **全部不存在**（fs 分类）→ **NOT_IMPLEMENTED**（预测试 RED，非业务缺陷）。

## 2. 逐核 oracle（T110 反例全部独立复算）

| case | 内容 | 结果 |
|---|---|---|
| K1 settle A≠1 | T110 恒等式独立复算：R=0.3、dH=0.1、dL=0.25 → **A=(dH+dL)/R=12610078956637389/10808639105689190 ≠ 1**；h+l=0.175 ≠ qControl=0.15；名义 k=1 guard 遗漏 (A−1)·q 项——必须用 A 包络 | PASS |
| K2 top MIX b≠0 | M=RN(.1+.2)=0.30000000000000004（hex `3fd3333333333334`）≠ 两存储数精确和；**b=MIX_LIGHT 偏置=精确和−M ≠ 0**；分支边界 q=M/nextUp(M) 必须以 RN 比较判定（不能用 rounded 和冒同一条件） | PASS |
| K3 LAND partial ULP 边界 | m=1：q=2⁻⁵⁴ → RN(m−q)=m（pool 无变化）→ partial **禁止**；q=2⁻⁵³ → RN(m−q)=nextDown(1)<m → 允许。边界逐 ULP 精确（T110 landSyntheticSubULPMass 复现） | PASS |
| K4 LAND full | q=m 仅 RAW 认证（不作零宽 INWARD 区间）；retire 同 slot、freeSlot 恰推一次、nextId 不推进；partial 保持原 ID/phase/source | PASS（合同夹具） |
| K5 ZERO 无写 | 全位型保存；disposition-6 包装禁止；CONTACT_HOLD_MOTION 独立（首次接触严格路径本版 UNSUPPORTED） | PASS（合同夹具） |
| K6 有界选择 | RAW/INWARD/ZERO 共用同一冻结 r0/B_e（失败不换基准）；≤2 非零影子图+1 无写 ZERO；INWARD_NONE 不做无界邻点搜索（对应 T110 boundedSelection nonzero=2/zero=1/无 ULP 循环） | PASS |
| K7 δ0 | C0=RN 链 ≠ 精确四库存和 → δ0≠0 必须进包络（C_actual−C0=A·q+B+E−**δ0**），不可吸收 | PASS |
| K8 外邻边 | 中央边安全而外邻边恰在 H：+1ULP 材料 → FINAL 在边 3 STRICT_EDGE 失败——**三边（中央+两外邻）认证强制**，LOCAL 域须含两外邻 | PASS |
| K9 registry 覆盖 | percolate/bottom/deep creep/lift-seed/merge/advection/invert/外源水——可达非零路径无 registry 命中即 STRICT_WRITER_UNSUPPORTED；仅 grant 打印 0 不证无副作用 | PASS（合同夹具） |
| K10 settle 逐 node 影子 | depth→aH/aL→dH/dL→scale→h/l 全节点精确有理值可溯；stock 约束 h≤suspH ∧ l≤suspL 成立；A 包络携带 | PASS |

## 8 处 T110 §8 反例覆盖状态

| T110 要求 | 本卡 |
|---|---|
| S0=[24,24+H存储,24] 超H 4ULP，q=0 不合法 | T125-V2-05 已证（复用不重做） |
| settle A≠1 | K1 ✓ |
| top M 舍入 b≠0 | K2 ✓ |
| q=M / RN(q−M)=BL 相邻数不同域证据 | K2 ✓（nextUp(M) 侧） |
| 外邻边在 H、三边认证 | K8 ✓ |
| LAND full/partial/q=m/RN(m−q)=m/反相 ±0 | K3/K4 ✓（反相通道保持原库存记 K4 合同） |
| q=0 仍改 CONTACT_WAIT 位置 | K5 ✓（CONTACT_HOLD 分离） |
| ID/free-list/provenance | K4 合同夹具（T135 逐字段核） |

## 3. node 域证明与数值差分分列

- 逐 node 包络证明：K10 给出 settle 全链精确有理值；T135 实现的每个 RN 节点须与本 oracle 位型一致。
- 随机差分仅辅助，不替代分支域/包络证明（K2/K8 已给构造性证据）。
- 预期 RED 供 T135；无真实生产 route 启用；CONTACT_HOLD 完整运动准入仍缺（T110 §5），不阻塞已支持纯核。

## 4. 供 T135 的夹具（p134_kernel_fixtures.json）

settle A 恒等式与 phaseSum≠q；top M 位型与 b 表达式；LAND ULP 边界（2⁻⁵⁴/2⁻⁵³）；有界选择三数；外邻边场景（H 位型）；δ0 提示。
