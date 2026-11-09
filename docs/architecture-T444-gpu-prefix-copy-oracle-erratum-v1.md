# T444 · GPU 来源账前缀字节勘误

2026-11-09，architect，P5。不使用真机。T435 把 sourceEvent16 复制量和 event64 前缀复制量接到了同一个 n。按 T433 §7 和 T434 实际 GL 提交，分流为 tester oracle 勘误，T436 语义差分可以继续。

| 边 | 条数、单条和条件 | 字节读法 |
|---|---|---|
| 来源账 | n 来自已接受 header word34，sourceEvent16 每条16 U32 | 前缀资格成立时 `ledgerCopyBytes=64*n`，0跳过 copy，保留 barrier。完整路及资格不明回退都是262144B |
| 同帧事件 | e 来自已保留的 SOURCE 或同帧 QUANTUM event64 前缀，每条64 U32 | `eventPrefixCopyBytes=256*e` 当 e>0 且 prefixBuffer≠目标，否则0。e 与 n 独立 |
| 状态/尾包 | `stateCopyBytes` 只统计显式 GL_COPY_BUFFER，shader 写入另账。完整 root 的真实成功 map 是1024B | 冷导出和计量数据独立，不能把 workspace 容量当传输字节或用字节推耗时 |

| 来源条数 n | 合格前缀 B | 比262144B少 B |
|---:|---:|---:|
| 0 | 0 | 262144 |
| 1 | 64 | 262080 |
| 2 | 128 | 262016 |
| 4095 | 262080 | 64 |
| 4096 | 262144 | 0 |

4097或错身份不能截到4096当合格样本。对 T434 的搬运探针应得到完整复制和回退原因，真实业务错数据仍在原 GPU 切点失败。毒值写入 WORK 的 `[base+16*n,base+16*4096)` word 范围，仅合格前缀断言该后缀不变。零长度仍核同一 barrier，满长后缀为空。

## 逐项影响

- `test/t435/prefix_oracle.cjs` 的 expectedPrefixCopyBytes、E2、boundaryTable、E5 全17个 expectPrefixCopyBytes 和固定 expectLedgerCopyBytes 的解释失效。S02 原256 改64，S03 代码字面1047040与报告1048320都改262080，S04改262144，S17原512改128。E3的262144仅作为完整基线保留。E4的 workspaceBytes 只说明容量，E7 的 FAULT 行数仅作历史执行器覆盖线索。E6 的两项 EXPECT_RED 是实现前历史标记，后续结果由 T434 实际差分决定。
- S01 的跳过条件用来源账 n=0。S05 的 DIRECT_W 成功增加一个来源条目且不占水槽。S06 部分成功仍只增一个成功来源条目，拒量写终态。S07/S11/S12 的全拒、去重或失败保原 n，不能默认先前来源账为空。S08 连续 QUANTUM 的 n 固定，event64 的 e 可独立变化。S10 LAND 是 event64 事件，不增加来源账。S15 保证回退和原首错，S16/S17 对实际合格前缀重测。
- S09 分开停止 source 后已接受 J 的后续到达，以及显式根撤权时的尾拒绝和已接受前缀保留。普通 UP 单独不充当 QUANTUM 撤权。S13 区分证书不合格走完整搬运和真正旧代 root 的拒绝。S14 用原审计阶段、原失败码与原始 accepted bank 对拍。
- 原报告、`t435_oracle.json`、原 fixtures 与 T280 465/0 保留为历史工件。T435 模型数学 PASS 不用于裁定 T434。T434 开发报告的实际 ANGLE 差分和计量保留其自身范围。

## 接手

tester 不使用真机，给 T435 产物显式勘误或新 revision，将 ledger n 与 event e 设为独立输入。核表内五个边界、同 n 不同 e、同 e 不同 n、e>0同对象跳过、满账/错代回退及后缀毒值。更新报告、数字断言、fixture 及派生 JSON。已有可复跑的语义矩阵保留，逐案标明哪些是模型、旧真实 GL 或新生产路径差分。T436 可以先按 T433 §7 与本稿对拍语义和正确计量，报告明确剔除旧数值断言。T437 后续采同包实际提交账和设备结果。

执行者完成当前范围及角色检查，局部受阻先做其他项，失败交原错和证据。除本人状态外不允许修改后续工作清单，也不修改滴答同名清单。沿 XComponent+shader，失败 FAULT 或明确 STALE 的完整旧帧。
