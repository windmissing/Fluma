# T460 · GPU M2 V3 独立归档格式反例基线

2026-11-10，tester，P5。不使用真机、不改产品。依据 `docs/analysis-T445-gpu-M2-wire-v3.json`（T445 机器 manifest，唯一合同来源）独立合成冷档 fixture 与 schema 判读预期，覆盖卡面全部边界，只针对冷档运输与版本边界，复用 T446 语义结论，不做 T453 生产差分。独立文件归 `test/t460/`。

## 交付

| 文件 | 内容 |
|---|---|
| `test/t460/archive_format_baseline.py` | 独立判读器 `indep_read`（从 wire JSON 重新推导全部判据，不 import 生产 `cold_reader.py`）+ 20 用例 fixture 合成与判定 |
| `test/reports/t460/t460-baseline.json` | 逐用例 expect/got/agree + wire SHA256 绑定 + NOT_IMPLEMENTED 登记 |
| `.codex/local-test/t460/pos-source*`、`neg-*` | 独立合成 fixture（正例取自 T452 真实 GPU 冷档 raw-1/raw-N-export） |

## 结果

- **verdict PASS，20/20 用例独立预期一致**（正例对照 2 + 负例 17 + 文件哈希层 1）。
- wire 合同绑定：`wireSha256` 落盘（SHA256 of analysis-T445-gpu-M2-wire-v3.json），改动合同即失效重跑。
- 复跑：`python -X utf8 test/t460/archive_format_baseline.py`（退出码 0=全过）。

## 覆盖矩阵（卡面逐条）

| 卡面要求 | 用例 | 预期拒绝码 |
|---|---|---|
| SOURCE128 错 stride | neg-source-stride | TRACE_STRIDE |
| QUANTUM64 错 stride | neg-quantum-stride | TRACE_STRIDE |
| rows×stride×4 不一致 | neg-source-eventsize / neg-source-rows-inflate | TRACE_EVENT_SIZE |
| 逐文件哈希/字节 | neg-file-hash（字节级篡改后 bytes/sha 收据不符） | 拒（文件层） |
| V2 reader 拒未知 | neg-revision-v2 / neg-revision-unknown | TRACE_REVISION |
| scope 错配 | neg-scope-epoch（manifest 头 epoch 漂移、page 头未随动） | TRACE_SCOPE |
| arena generation 错配 | neg-arena-oob / neg-generation-mismatch | TRACE_ARENA / TRACE_GENERATION |
| rootKind 错配 | neg-rootkind-flip | TRACE_STRIDE（stride 随 kind 派生） |
| pub/scene token 不可互替 | neg-roottoken（rootToken 篡改）/ neg-token-scene-shape（scene token 形态塞入） | TRACE_ROOT_TOKEN |
| SOURCE 行 seal/保留位/END 尾 | neg-source-rowseal / neg-source-reserved / neg-source-end-early | TRACE_SOURCE_ROW_SEAL / TRACE_SOURCE_RESERVED / TRACE_SOURCE_END_TAIL |
| QUANTUM kind 限 3–14 | neg-quantum-kind（SOURCE_BIRTH kind=1 禁入）/ neg-quantum-reserved | TRACE_QUANTUM_KIND / TRACE_QUANTUM_RESERVED |

## NOT_IMPLEMENTED（如实登记，3 项）

1. 真机冷档（GPU 页写出 bytes/SHA 收据链路）由 T461 真机档覆盖。
2. T453 生产差分中的 reader 与产品 GL 链路一致性。
3. pub/scene token 真实互替攻击（需产品 scene token 通道，当前离线无该通道）——本卡只做 token 形态位替换负例。

## 边界

- 判读器为独立实现，与生产 `cold_reader.py` 同合同不同代码；历史 V2 原档继续按原 reader（本卡不覆盖）。
- 正例基线来自 T452 真实离线 GPU 冷档（raw-1-export 等），fixture 只做最小单字段扰动，不构造复合畸形。
- 设备 NOT_RUN；未冒任何真机判读。
