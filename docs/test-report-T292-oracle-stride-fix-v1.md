# T292 M1 独立 oracle 步长纠正 · 报告 v1

- 角色：tester（离线，不使用真机）· 日期：2026-10-30 · 状态：**结束待验收（步长纠正后 B/C/D/E/F/A/G 全部独立 PASS；T287 报告勘误附后）**
- 被测：T279 候选核 `d7821a3`（同 T287，二进制未变）；独立 oracle 归档 `test/reports/t292/t292_oracle.py`
- 依据：T288 裁决 `6a686d6` + `architecture-T288-m1-event-parser-attribution-v1.md`

## T287 勘误（本卡核心）

T287 报告 NOT_INDEPENDENT 三项（B/C/F）与"events 段 48-word 交错"观察**均为 tester 自身解析错误，产品无任何布局问题**：

1. **步长单位错误**：`struct.unpack_from` 偏移单位是**字节**，T287 oracle 逐行读取写成 `off+j*64`（把 64 word 当 64 字节），导致行窗口互相重叠。正确为 `off+j*256`（T238 manifest `event64BytesPerRow=256`）。"magic 位于 [0,112,160,208]" 是重叠窗口的解析产物，非产品布局；实际 magic 在 word `[0,64,128,192]`，kind `[SAND_MOVE,CONTACT_WAIT,ARRIVE,ARRIVE]`（case33）。
2. **D 常量错误**：T287 oracle 用 `D=2*Q`，正确为 **`D=120*T`**（check 源码 line 4）——B 项 T16 速度推导因此全部失配。
3. **E"唯一行"错误**：case21 water-spread 实际有 **3 个 WATER_EDGE 行**（64→63、63→62、64→65），T287 只看到重叠解析的首行。

## 步长纠正后独立复核结果（全部 PASS）

前置断言全过：每 case `info[2]%64==0`、`status[1]==info[2]/64`、`off==len(raw)`；mask 从 T238 manifest `eventKinds[].usedMaskHex` 独立生成（不调开发 checker、不 magic 扫描）。

| 项 | 分母 | 结果 |
|---|---|---|
| **F** event64 used-mask/保留 0 | **70 行**（全 38 case 事件段逐行逐 word） | **0 违规 PASS** |
| **B** T16 RNE/τ/medium（AIR 整数 oracle：bank 位移+速度四元组） | **8 例**（air-0×4 + air-1×4，含 drag 变体） | **8/8 PASS** |
| **C** ARRIVE 确定序 (column,y,id) | case33 两 ARRIVE | **ids=[2,1] PASS** |
| **E** 水边写集全流（独立 head2：同色快照→确定性边序→prefix apply，三行） | case21 全部 WATER_EDGE | **3 行逐 word 匹配 PASS** |
| **D** 预算全流（CROSS 按 ns 分/sand 扣 total 一次两端各一次/LAND local 一端/BED 三对 delta） | **24 行**（全部非 fatal case 的 kind5/8/9） | **0 失败 PASS** |
| **A** 失败全 bank 回滚（窄证保留） | 3 例 | **PASS**（bank 与输入逐 word 相等） |
| **G** reason 码（窄证保留） | 3 例 | **PASS**（12/1/15） |

## T287 报告处置

- 原 `docs/test-report-T287-m1-candidate-indep-v1.md` **保留不改**，本报告为其勘误与补全（分母如上表）。
- T287 中"产品 events 段存在未文档化 48-word 交错布局"的判断**撤回**——该观察是 tester 脚本字节/word 单位错误的产物；T238 规格（64 word/行）自始充分。
- "NOT_INDEPENDENT 状态"解除：B/C/F 现为独立 PASS。

## 对 T280 的边界

本卡全部为 K192/Kw64 离线二进制抽核；未触及 T280 accepted bank/发布/真机。B 的 T16 速度律参数（g512 等）仍为 T237 待定标候选——本卡核的是"实现与整数公式一致"，非速度律物理正确性。

## 复跑

`python test/reports/t292/t292_oracle.py`（无依赖开发 checker；输出 JSON 含 F/B/C/D/E/A/G 各项与分母）。
