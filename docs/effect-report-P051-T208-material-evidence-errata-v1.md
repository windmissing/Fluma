# P051 T208 材质现场证据提取勘误回执 v1

日期：2026-10-28。责任：effector。任务卡：`6ae15c23e4b0f11bdf252e9d`。消费：T202 `b9f15aa`（`architecture-T202-demo-material-arrival-v1.md`）、T180/T187 `0a6fe16` 报告与四臂 raw。  
状态：**ERRATA_APPLIED / INDEPENDENT_REASSEMBLY_VERIFIED / OFFLINE_NO_DEVICE**。  
按 T202 §1/§4 完成四项版本化勘误 + 24 片**独立**重组核验（SHA 与 T202 一致）；T180 报告三处结论修正以本回执为准，原报告/raw 零改动。

## 1. 独立重组与核验（不依赖 T202 重组结果）

从 `C3-full.txt` 独立解析：**24 个 `WS_EVENT scene_material_rejection` 分片**（PID 40176），token `p011-run-1793151013110-3:2:826:409`，part 1..24 无缺号，complete=true，payload 49,063 字符，**独立重组 SHA256=`0f3cedbf65e2c69697fe7de43b8d670531a4bb629b9836916737d257fdd3e39f`（与 T202 §1 一致）**。产物：`test/artifacts/t180-t187-joint-20261028/t208-reassembled-material-independent.json`。

## 2. 首错谓词与 col57 原值独立核验（对 T202 §2 表逐项一致）

`point={reason:MATERIAL_DATA_PROFILE, stage:SOURCE_VALIDATION, column:57, row:0, predicateMask:128, exceptionMessage:MATERIAL_DATA_PROFILE}`；`firstFailure.frameToken` 与 `p013_scene`/native 回执同 token。col57 原值（rawHex 独立解码 + 数组 SHA256 重算全匹配）：

| 字段 | col57 值 | raw |
|---|---:|---|
| waterTop | **−1.6653345369377348e−16** | `000000000000a8bc` |
| sandTop/bh/bl/mh/ml/suspH/L/hs/hl | +0 | `0000…0000` |
| colParticleMassH | **−1.6653345369377348e−16** | `000000000000a8bc` |
| colParticleMassL | +0 | `0000…0000` |

mask 128=top>water：零床下 waterTop(−ε) > sandTop(+0) → 拒绝成立，与 T202 "物理允许的微负派生残差进入严格渲染域"结论一致。

## 3. T180 报告四项版本化勘误（原报告零改动，以本回执为准）

| # | T180 原表述 | 勘误后（T202 依据） |
|---|---|---|
| E1 | "scene_material_rejection 分片导出 0 行" | **实际 24 片完整导出**（C3-full 原行 198674–198697，complete=true）——检索缺陷（当时只匹配了 PID 过滤的 WaterSand tag 行，rejection 行为 JSAPP tag）非导出缺失 |
| E2 | "T178 未接 DEMO 页/触发条件缺口" | **撤回**：源码已有接线且设备本次确实执行了导出；不派"补接线/放宽触发"开发返修 |
| E3 | "C2 真实注入后持续运行（双场页干净可作为对照）" | C2（PID 39412）在首 pan 前 **11.3 秒已 CLOSED**（266/0）、之后 488 条 pan 均在收尾后——**不能当"材质已修"的控制臂**；C1 同理（CLOSED 262/0 早于窗口尾） |
| E4 | "四臂 PID 链归属全程 OK" | console 实际 C0/C1 为 **DRIFT**（脚本 `$c0pid` 收到数组导致字符串比较污染）；start/end 行的标量 PID 仍分别相同，可人工按 raw 锁定四臂——归属结论不变但"自动断言通过"不成立 |

## 4. 保持成立的结论（T202 复核确认）

真实手持（T177）、C3 五次 START ACCEPTED、6 笔 dry.receipt admitted 4.451、commit 2→1368 增长、无 physics-failed/V2 导出、C0 健康基线（present 0×520）——均保留。SCENE_MATERIAL_DATA 的真实拒绝原因（零床微负 waterTop 残差 vs renderer 严格域）已闭合至 `P013FrameAdapter.pack` SOURCE 验证；数值修复走 T202 §5B（designer）→C（developer）路由，**禁止 renderer 加 EPS/钳零等伪修复**。

## 5. 采集纪律累积修正（并入 v2/v3 采集器）

1. 被动诊断解析一律按 `.rows[].event`（console.info 的 tag 是 JSAPP，不是 WaterSand）——本卡 E1 根因。
2. `Select-String -First 1` 类全历史检索必须先过滤 PID/run/window（T204 E2 同源教训第三次出现）。
3. PowerShell helper 返回值可能是数组（多行输出），须取标量（T202 §4 console DRIFT 根因）。
4. 控制臂有效性必须断言"CLOSED 未发生于注入前"，否则只是"已采到静置活动窗"。

## 6. 边界声明

- 离线交付，原 raw/报告零改动；独立重组 JSON 与本回执入库。
- poolReplayComplete=false、captureTimeMs 未测等边界沿 T202 保留；完整 writer 因果归 T202 §5C 最小观测 hunk。
- 滴答 T208 卡评论已发；清单行状态留 manager。
