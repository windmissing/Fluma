# T453 · GPU M2 V3 生产路径独立差分

2026-11-10，tester，P5。不使用真机、不改产品。消费 T452 真实离线 GLES 生产 roots（80 roots / 四档 K192/512 × Kw64/128，60 accepted + 8 DEDUP_REPLAY + 16 reject，raw dump = completion256 + state + events），按 T445 wire JSON 独立重推导判据，补齐 T446 oracle 五项 NOT_IMPLEMENTED 的生产字节证据。复用 T446 语义层结论（31 entries PASS，未重跑）。独立文件 `test/t453/`。

## 交付

| 文件 | 内容 |
|---|---|
| `test/t453/v3_assertions.py` | 五项 V3 断言独立判据（不 import 生产代码；header 摘要区映射经 raw 对照标定并在报告中固化） |
| `test/reports/t453/t453-assertions.json` | 逐项判定 + wire SHA256 绑定 + 来源声明 |

## 结果：verdict PASS，5/5

| 项 | 断言 | 结果 |
|---|---|---|
| M203 | mutationSerial(lo)==sealedRows（每 MERGE/BIRTH checked+1，单写者逐行封签；接受 SOURCE 全部） | **32/32** |
| M205 | stop∈0..9（wire stopCodes 域）且 BIRTH_LIMIT rows==K+1；DEDUP_REPLAY(outcome=3) 继承原终态 stop 不重复计 | **48/48**（replays=8） |
| M206 | header merge/birth 计数（h137/h138）== 事件行 kind15/16 实际计数 | **32/32** |
| M207 | SOURCE128 逐行 magic/revision/rootKind/ordinal 1-based/rowSeal/END 尾行唯一/used-mask 保留位零 | **5712/5712 rows** |
| M208 | fatal/reject roots reason∈{23-26, 0x10000\|input, stop 域} 且事件行≤1（整根回滚无 accepted 行堆积） | **16 fatals, 0 bad** |

## 标定说明（判据透明化）

header 摘要区（state 尾 256 word）映射经 raw 数据对照标定：`h[132..136]=policy/pkg/lightMax/radius/Bmax(K)`、`h[137]=mergeCount`、`h[138]=birthCount`、`h[139]=stopCode`、`h[140]=stride`、`h[141]=sealedRows`、`h[142:144]=mutationSerial U64(lo,hi)`、`h[144]=logicalArena`；completion `c[160:173]` 为其逐字投影（已核相等）。该映射与 T445 §逐字段表一致，标定过程以 raw-1/raw-11 双样本互证（192 births vs 191 merge+191 birth）。

## 边界与未覆盖（如实）

- 本卡为 raw dump 层独立断言；work 区 control/originU64 段未随 raw 落盘，其运行时等价断言已由 T452 host 内 assert（`read(bank,state)==before` 回滚、spare replay 逐 word 比较）覆盖，本卡不重复构造。
- lease/Acquire 通道与 scene token 为 M1 共享层，T434/T436 已实证；M2 未引入新 lease 语义。
- 真机（能力、时延、fps）归 T461，本卡 device=NOT_RUN，未冒任何真机判读。
- 卡面"核 mask、pair/RNE、来源/首错、回滚、lease、stride 及成功前缀"中 pair/RNE/首错语义由 T446 oracle 31 entries 冻结（复用不重跑）；mask/stride/行级/回滚本卡以生产字节实证。

复跑：`python -X utf8 test/t453/v3_assertions.py`（前置：`.codex/local-test/t452-host/raw-*.bin` 存在，否则退出码 2）。
