# T454 · GPU 来源前缀 oracle 版本勘误报告

2026-11-10，tester，P5，不使用真机。执行 T444 勘误合同（architecture-T444-gpu-prefix-copy-oracle-erratum-v1.md §接手）：更正 T435 产物的计量断言与派生 JSON。历史原件（`test/t435/prefix_oracle.cjs`、`test/reports/t435/t435_oracle.json`）保留为历史工件不动。

## 交付

- `test/t454/prefix_oracle_v2.cjs` — 勘误版 oracle（revision `T454_PREFIX_ORACLE_ERRATUM_V2`，verdict **PASS**，23 entries，supersedes 旧 T435 数字断言）
- `test/reports/t454/t454_oracle_v2.json` — 勘误判定原件

## 勘误核心（T444 表逐行落地）

**分账**：ledger n（sourceEvent16，每条 16 U32）与 event e（event64，每条 64 U32）设为独立输入：

| 边 | 计量 | 条件 |
|---|---|---|
| 来源账 | `ledgerCopyBytes = 64*n` | 合格前缀；n=0 跳过 copy 保原 barrier |
| 来源账回退 | `262144B`（route=2） | 资格不明/错代/4097（不能截到 4096 当合格） |
| 同帧事件 | `eventPrefixCopyBytes = 256*e` | e>0 且 prefixBuffer≠目标；e 与 n 独立 |

**作废断言清单**（T444 §逐项影响，对照原 t435_oracle.json）：`expectedPrefixCopyBytes`（256*n）、E2（256*1=perRow）、boundaryTable、E5 全 17 个 expectPrefixCopyBytes、S02（256→64）、S03（1047040/1048320→262080）、S04（→262144）、S17（512→128）；E3 的 262144 仅作完整基线保留；E4 workspaceBytes 降为容量说明；E7 FAULT 行数降为历史线索；E6 两项 EXPECT_RED 由 T436 实际差分关闭。

## 场景矩阵 v2（15 场景，n/e 独立，三态标注）

| state | 含义 | 场景 |
|---|---|---|
| MODEL | 语义矩阵（本 oracle 独立推导） | S01–S04 五边界（0/64/262080/262144B）、S06 同 n 不同 e（128+768）、S07 同 e 不同 n（320+512）、S08 n=0 跳 ledger 但 event 照常、S09 e>0 同对象跳过、S14 正常 UP 不充当撤权 |
| NEW_PROD | T436 新生产路径差分已证（引用不重跑） | S05 n=4097 回退 route=2、S10 满账/资格不明完整复制、S11 错代不截 4096+原首错、S12 毒值 word 界 [16n,16×4096) 保持（0xa5a5c3c3）、S13 满长后缀为空 |
| OLD_GL | 旧真实 GL 历史覆盖 | S15 显式根撤权尾拒绝+前缀保留（T280 stop 在途 header/count 不变）、E1 基线 465/0、E9 FAULT 线索 |

**正常 UP 与撤权区分**（T444 §S09 修正）：S14 普通 UP 单独不充当 QUANTUM 撤权，停止 source 后已接受 J 的到达照常；S15 显式根撤权才有尾拒绝+已接受前缀保留——两场景分开冻结。

## E8 · EXPECT_RED 关闭

T434（commit b54dc48）已实现、T436（dfb98e3）四档实际 GL 差分证明：毒值后缀保持（S16 语义）与前缀真减量（S17 语义）均已在生产路径成立。原两项 EXPECT_RED 关闭，后续结果由 T434 实际差分决定（T444 原文）。

## 边界

- 原 `t435_oracle.json`、fixtures 与 T280 465/0 保留为历史工件；T435 模型数学 PASS 不用于裁定 T434
- 本卡只更正 oracle 版本，不改生产、不重复跑 T436/T437 整卡；T437 设备收益另卡采同包实际提交账
- 逐案标注：MODEL=模型、OLD_GL=旧真实 GL、NEW_PROD=新生产路径差分（T444 要求）
