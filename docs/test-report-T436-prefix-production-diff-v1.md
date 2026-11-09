# T436 · GPU 前缀复制生产实现独立差分报告

2026-11-09，tester，P5，不使用真机。前置：T434 实现（commit b54dc48，签包 SHA `c209967e...62626d`）、T435 oracle（6176b51）、T444 勘误合同（architecture-T444-gpu-prefix-copy-oracle-erratum-v1.md）。

## 交付

- `test/reports/t436/t436_independent_diff.json` — 独立复跑差分判定（四档 × full/prefix 两路）
- 独立复跑日志与 dump：`.codex/local-test/t434-host/` 执行（test/t434/full_host.cpp 真实 GL host，ANGLE SwiftShader GLES 3.1）

## 差分对象（T434 实际实现，非独立模型）

实际生产 `FullM1Transport::StageLedger` + `QuantumCandidate::Enqueue` 真实 GL 路径：
- 资格由同 GL owner 接受后的 native Header 构造（magic/版本/六身份/三 hash/接受 bank/n≤4096）
- 合格前缀 `ledgerCopyBytes=64*n`，n=0 跳过 copy 保原 barrier；资格不明 route=2 回退 262144B 整账
- `eventPrefixCopyBytes=256*e`（e 与 n 独立）不变；毒值后缀 `[16n,16×4096)` 合格时保持

## 独立复跑结果（本卡执行，非引用开发归档）

| 档 | prefix | full | raw accepted 二进制 | 与开发归档 SHA |
|---|---|---|---|---|
| 192/64 | 465/0 | 465/0 | **逐位一致**（b193357a...） | 逐位一致 |
| 192/128 | 437/0 | 437/0 | 逐位一致（3236d096...） | 逐位一致 |
| 512/64 | 437/0 | 437/0 | 逐位一致（d8c53af2...） | 逐位一致 |
| 512/128 | 437/0 | 437/0 | 逐位一致 | 逐位一致 |

`allRawAcceptedIdentical=true`：完整/前缀两路接受的 state+event64+completion256 字节完全相同——前缀复制不改变任何可见行为。

## 计量口径（按 T444 勘误合同，剔除 T435 旧数值断言）

T444 已裁定 T435 的 `256*n` 计量无效（来源账每条 16 U32=64B，256B 是 event64 边）。本卡以 T444 表为准对拍：

| n | 合格前缀 B | 比 262144B 少 |
|---|---|---|
| 0 | 0（跳过 copy 保 barrier） | 262144 |
| 1 | 64 | 262080 |
| 2 | 128 | 262016 |
| 4095 | 262080 | 64 |
| 4096 | 262144 | 0 |

T434 host `StagingBoundary` 内嵌断言 `ledgerCopyBytes==(n≤4096?n:4096)*64` 与 T444 表逐行一致；n=4097 与身份错代（`identity.run.lo^=1`）回退 route=2/262144B 已在执行中核过。**T435 旧 oracle 的 256*n 系列断言（S02–S04/S17/boundaryTable）按 T444 §逐项影响全部标注无效**，本卡不以错误 oracle 裁产品。

## T433 A-T 语义覆盖（T444 修正后口径）

- 毒值后缀：0xa5a5c3c3 预写 WORK `[n,4096)` 区，n<4096 断言未被覆盖（含 n=0 全后缀保持）；n=4096 满长后缀为空
- 首错/原切点：14 类故障注入（192/64 档）全部原 GPU 切点拒绝，header/bank/count 不变
- 同 n 不同 e / e>0 同对象跳过：eventPrefixCopyBytes 路径独立于 ledger 复制，条件复制语义未改
- 换代/回退：错身份回退整账；Stop 在途尾拒 header/count 不变；lease/重放未改
- n=4096 无收益、诊断 ON/OFF 等价（trace 不改复制量）、host map/upload 不增长（CopyAccounting completionMappedBytes=1024 固定）

## 边界与声明

- 大 n（4095/4096/4097）是**仅搬运 fixture**，不冒充 4096 次真实 SOURCE admission；真实满账/非法 origin 首错由 host 14 类故障覆盖
- 与 T437 独立：本卡不采设备收益，不卡设备窗口或开发关闭门
- T435 产物勘误归 tester 后续卡（T444 §接手），本报告已按 T444 口径剔除旧数值断言
