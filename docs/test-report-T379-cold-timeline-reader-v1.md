# T379 GPU 冷时间线独立判读工具 — 完成，oracle 独立、反例三连全过、T337/T376 双档互证

- 卡面：用 T337 真机原档和 T375 host 原档建立独立判读工具，核 stage42 与 READY/首 anchor 顺序、同 run/TID 原始 ns、首末批及 GAP 归属、accepted 到同 token 证据，缺字段明确不可判；截断/跨 run/错单位反例验证；供 T376 及后续报告消费。
- 资产：`test/t379/cold_timeline_reader.cjs`（可重跑 CLI）；报告 `test/reports/t379/{t337b_report.json, t376_cross_report.json, t375_host_report.json}`。

## 工具（oracle 独立性）

零 import 生产/开发断言代码；只消费原始行数组（T337b `rows_runN.json` 133 行/run）与 ndjson 页（T376 `tl-partN`）；BigInt 保原始 ns 精度；全部判定从行字段自推。检查项：①serial 连续+全局 ns 单调（原始单位）②同 (epoch,run,surface,TID) 分组 ns 单调 ③stage42 与 Ready/AnchorSet 顺序+carry0/1 对契约（carry0 delta=0、carry1 delta>0、root=0）④首/末批 wall 与尾 gap（BATCH_BEGIN..END 边界、OP_END→BATCH_END 尾）⑤AnchorSet→PauseBegin GAP 归属 ⑥RootSubmit→RootPoll 同 root 对（accepted/token 字段缺失时**显式 NOT_JUDGABLE** 指明需外部 publication 收据）。

## 反例自验证（三连全过）

| 反例 | 结果 |
|---|---|
| 截断（删中间行） | **抓到**（SERIAL gap 计数>0） |
| 跨 run 混排（epoch+100 副本混入） | **隔离正确**（分组键隔离，单调性不误报） |
| 错单位（某行 ns→ms 量级） | **抓到**（NS_BACKWARD 触发） |

## T337b 真机原档判读（三 run，各 133 行）

- serial 连续/ns 单调：**PASS**（0 gap，8 组）。
- stage42：**档内不存在**（T337 为预热前版本，42 系 T375 新增）——`no-stage42-in-window` 如实，非工具缺陷。
- **batch1 wall = 35.244ms**——与 T337 报告『SubmitEnter→Exit 35.244ms』**逐位一致**（独立复算互证）；batch2 空（预算耗尽未进批2，与 T337「没有任何 run 到达末批 fence」一致）。
- accepted→同 token：NOT_JUDGABLE（行无该字段，符合卡面「缺字段明确不可判」）。

## T376 时间线交叉判读（独立工具 vs T376 手工分析）

**逐位一致**：42 对（first42=49<Ready=51<Anchor=55、pairOk、warmupNs=23378125）、batch1 0.433/0.373/0.346ms、batch2 46.788/41.816/48.218ms、尾 gap 46.685/41.721/48.127ms、accepted→token NOT_JUDGABLE——双 oracle 交叉无矛盾。

## T375 host 原档

`partial-host.txt` 无逐行时间线（结构化 CASE 行）：CASE_W 行在位（"real pass 0, scratch only, fresh-root bit parity, raw wall and failed-Create retry"）——host 档仅支持结构性存在核验，逐行判读以真机档为准（如实）。

## 供消费

- T376 报告的 W1/W3 数字已被独立工具复算证实（双 oracle）。
- 后续 GPU 线报告可直接 `node test/t379/cold_timeline_reader.cjs ndjson <partN.ndjson>` 消费新冷档。
- 局限：GAP 归属（AnchorSet→Pause）在本两档中无样本行（T337b/T376 均无 PauseBegin 相邻 AnchorSet），该检查在位等待未来档；accepted→token 需 publication 收据侧输入（判别入口已注明）。
