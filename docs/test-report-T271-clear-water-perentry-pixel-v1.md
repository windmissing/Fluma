# T271 明水逐笔与像素真机补证 · 报告 v1

- 角色：tester（真机 VYG-AL00 62T0225B10010020）· 日期：2026-10-29 · 状态：**PASS（逐笔 ACK 与 W→层链达成；水层可见性=参数量化域，证据回 T268 architect 分流）**
- 被测包：**T270-v4-signed.hap**（SHA `599d03e6fc98278a14a1c006187791d8ebd116070f0c134117c114d6b14d793f`，诊断 `cwdiag=true`、`cwdiagSha`=包完整 SHA）
- 页面：`pages/ClearWaterDemo`（`--ps cwdemo 1`）；run `cw-run-1793273652501-1-1`（pid 49023 会话）
- 归档：`test/reports/t271/`（t271a/b.json 布局、t271c/d/e.jpeg、t271_diag{,2,3}.json 三轮导出、raw 分段原文）

## 执行序列（按 T268 §4）

冷启诊断臂 → 注沙 2 粒 → 切水（`CURRENT·当前源：水`）→ 画布新 DOWN + 两有效 tick（uinput -T -d/-m/-u）→ 点"导出诊断"→ 分段拼装。

**导出完整性**：157/158/162 分段 + END，逐段拼装后字符数与 END 声明**逐字节一致**（233761/234223/241561），`dropped`/`incomplete` 字段显式（第三轮 dropped=393）。无 chunk 丢失。

## ① 水正量 ACK 逐笔 —— **PASS**

四笔水请求全部到 TERMINAL 且逐笔可读（`t271_diag3.json` rows 行 153/158/209/214）：

| requestSeq | disposition | admitted | rejected | transactionId | kindSuccessSeq | sourceCommit | physicsCommit | W before→after | actualWaterDelta | I_water |
|---|---|---|---|---|---|---|---|---|---|---|
| 1 | ACCEPTED | 0.011116574 | 0 | 1 | 1 | 1 | 94 | 0→0.011116574 | 0.011116574 | 0.011116574 |
| 2 | ACCEPTED | 0.022234212 | 0 | 2 | 2 | 2 | 96 | 0.011116574→0.033350786 | 0.022234212 | 0.033350786 |
| 3 | ACCEPTED | 0.011132754 | 0 | 3 | 3 | 3 | 113 | →0.04448354 | 0.011132754 | 0.04448354 |
| 4 | ACCEPTED | 0.022265193 | 0 | 4 | 4 | 4 | 115 | →0.066748733 | 0.022265193 | 0.066748733 |

`admitted>0`、`transactionId`/`kindSuccessSeq` 连续、`sourceCommit` 单调、W 全局与列增量一致、I_water 与 W 同步累计——**逐笔正量 ACK 链完整**。DOWN/SELECT/CLOCK_WINDOW/ENQUEUE/BATCH 行均在环内（DOWN 2、SELECT 1、ENQUEUE 4、TERMINAL_QUERY 4）。

## ② W→publication→slot/native→同 token —— **PASS（数值链）**

帧 200–202（三帧 × 三层）native ROI 封存：`nativeWater = ledgerWater = 0.066748733`（与 rows TERMINAL 累计值**逐位一致**），`sourceCommit=133/135`、`physicsCommit=4` 同 token（token 内 frame 字段=200/201/202、configDigest 完整 inline）。slot verify 无差异、rawGL=0、readback 1.1–6.2ms/帧三层。**W 数值从物理 ledger 到 native 同 token 无损**。

## ③ 旧代终态 —— **N/A（语义内）**

`OLD_KEY_QUERY` 0 条：本轮 select(1) 时 `seq=0`（沙源无未决请求），controller `queryOld` 仅在有旧请求时触发（源码 `if(this.diagnostic!==null&&this.seq>0)`）——与 T268 §4"只在诊断开启且有旧请求时 query 旧 key"一致。沙→水→沙→水的完整往返场景留待后续卡（本轮未注沙请求后切水）。

## ④ 水层/水面像素 —— **公式正确、当前水量量化后不可见（参数域，非根因）**

- ROI（64,200,4×4）三层 rgbaHex **逐字节相同**（`12161fff` = 背景 sRGB(18,22,31)+alpha 255），层3（水体后）/层4（水面后）在 ROI 处无可见水色。
- 量化核算：`alpha=1-exp(-sigma*path)`，sigma=0.025；W=0.0667 对应列水高 path≈0.033（模型 256 域）→ alpha≈0.00082 → **8bit 量化 = 0**。公式本身随 path 单调（path=8 时 alpha=0.18→46/255 可见）——**低 alpha 公式正确但两 tick 正量（0.067）远低于可见阈值**。
- 截屏 `t271d/e.jpeg` 画布纯背景（同 T262 观察）。
- 按任务书：不自行调透明度/水量；此证据交 T268 architect→designer/effector 决定水量/积分窗口或可见性判据口径。

## 边界（如实）

- ROI 位置固定 (64,200)：第一轮 arm 帧 3–5 早于水 DOWN（nativeWater=0），第二轮起改 cwdiagFrame=200/260 命中；三轮导出均为完整 END。
- 首轮/次轮 rows 环 256 行被每帧 TICK+STOP 淹没（dropped 1424/4632）为设计内定容行为；第三轮时序压缩后水操作行全部在环内。
- 屏幕级像素（snapshot）与 native ROI 同为背景——水不可见在两个通道一致，非 ROI 采样伪影。
- UI"当前源：水"与 CURRENT 回执存在，但不作为水 ACK/水层可见证据（按任务书仅作背景）。

## 结论回 T268

**正量水 ACK 逐笔 TERMINAL/同 token W→native 数值链 PASS；旧代终态本轮语义 N/A；水层可见性未达成且归因于水量×sigma 的量化阈值（公式域），参数调整需求交 architect/designer，tester 未调任何光学参数。**

## 证据

- `t271_diag3.json`（主证据，241561 字节 END 完整）：rows/ledgers/optics/nativeSamples/profiles/tokens 全量。
- `t271_diag.json`/`t271_diag2.json`：第一、二轮（arm 帧时序对照）。
- `t271_diag_raw*.txt`：分段导出原文（可复核 END 完整性）。
- `t271c/d/e.jpeg`：水注入前后屏幕。
- `t271a/b.json`：按钮坐标（沙[150,456] 水[346,456] 导出[794,456]）。
