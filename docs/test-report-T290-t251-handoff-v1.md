# T290 T251 执行报告交接 · 报告 v1

- 角色：tester（离线整理，不使用真机、不重跑）· 日期：2026-10-30 · 状态：**结束待验收（交接完成；核心结论：T251 设备面从未执行，"已有真机原档"不存在，正式报告以离线原档+缺测登记交付）**
- 背景：T251 被宣布完成/关闭，但本地未发现其正式测试报告。本卡按原档整理、给出可追溯索引与缺测登记；不造数据、不把任务关闭冒产品 PASS。

## 一、原档盘点（全部现存，可追溯）

### 1. T250 BEFORE（20 cut × 两 freeze 臂）

| 项 | 值 | 位置 |
|---|---|---|
| 执行结果 | **300/0**（40 基本 + 2 A/B 额外 + 6 同输入撤销控制，Node transpile 离线） | `test/reports/t250/before-v30/summary.json`（revision `T250_ORIGINAL_BEFORE_V10`） |
| 故障 case 归档 | **49 个 gzip**（20×2 臂 + cut5-ord2×2 + 控制 6 + 溢出），各带 SHA256 与 rawSha | `test/reports/t250/before-v30/before-*-*.json.gz`，索引 `summary.archives[]` |
| 正式字典 | 20 cut 逐项原函数/谓词/源 SHA/行号/firstCode 域/root/ordinal/rollback 边界 | `test/reports/t250/before-v30/cut-inventory.json`（`T250_BEFORE_CUTS_V10`） |
| 签包 | SHA `3b87eee5e12c…`，签名/verify PASS | `.codex/local-test/t257-sign/T250-before-v30/T250-before-v30-signed.hap`，交接 `before-v30/handoff.json` |
| 自证声明 | `"device": "NOT_RUN"`、`"sameBinaryDevicePid": "NOT_RUN"` 写在 summary 内 | 同上 |

### 2. T278 AFTER（NEXT/OBS/NA）

| 项 | 值 | 位置 |
|---|---|---|
| 执行结果 | **99/0**（Node 离线） | `test/reports/t278/summary.json`（`T278_AFTER_EXECUTION_V2`） |
| 归档 | next-N×两臂 gzip + obs×两臂 + scope-v9 执行 | `test/reports/t278/`（40 文件） |
| 自证声明 | `"device": "NOT_RUN"`（全 14 处 device 字段均 NOT_RUN） | 同目录 |

### 3. 非机差分与工程身份

- 原三 backend 两 freeze、N192/512 正常差分 12/0（`before-v30-normal.json`）
- hvigor PackageHap BUILD SUCCESSFUL（`before-v30-package.log`）；sourceManifest SHA `249d5089…`、buildInputManifest SHA `0e61dc94…`（handoff.json）

## 二、结论（不冒产品 PASS）

1. **T251 的设备面（同 binary/PID/fixture 两 freeze 臂、synthetic 满池、offered/accepted 真实负载、每 root B/M/A/λ、20 cut 真机注入、同负载 p50/p95 与五段服务成本）从未执行**——全部现存原档自证 `device=NOT_RUN`、`sameBinaryDevicePid=NOT_RUN`。**不存在"已有真机原档"**。
2. 已达成的是**离线 Node 侧 399/0**（BEFORE 300 + AFTER 99）+ 12/0 差分 + 签包/身份链完整——这些是 T250/T278 交付的一部分，不构成 T251 的设备验收。
3. T251 执行卡的**BLOCKED 根因（tester 2026-10-30 复核）**：T250-before-v30 包内 `runTxGpuFixture` 只被 Node transpile 执行器消费，**App 内无任何页面/Ability 入口**——设备 runner 接点在 T250/T278 交付中均未实现。缺什么、为何不可继续：缺"设备端 runner 接点"（含 want 触发、PID 上报、成本计量导出），需 developer 交付后方可执行设备面。

## 三、缺测登记（T251 原卡范围 vs 现状）

| T251 原卡要求 | 状态 |
|---|---|
| 同 binary/PID/fixture 两 freeze 臂（真机） | NOT_RUN（无 runner 接点） |
| synthetic 满池、offered/accepted 真实负载 | NOT_RUN |
| 每 root B/M/A/λ 逐笔 | NOT_RUN |
| 20 cut 设备原 firstCode/receipt/events/lineage/rollback 退休 | NOT_RUN（离线 300/0 已覆盖同语义，但非设备证据） |
| 同负载重复完整服务 p50/p95 | NOT_MEASURED |
| prepare/worker/fence/map/archive/UI present 五段成本 | NOT_MEASURED |
| 不重跑 64root 终态矩阵 | 遵守（未重跑） |

## 四、交接索引

- T250 交接：`docs/developer-T250-before-closure-v10.md`、`test/reports/t250/before-v30/{summary,cut-inventory,handoff}.json`
- T278 交接：`test/reports/t278/{summary,handoff,manifest}.json`
- T251 卡评论（本 tester 2026-10-30）：BLOCKED 根因复核（缺设备 runner）
- 后续路径：developer 交付设备 runner 接点 → T251 设备面可执行（设备电量已恢复 78%，无其它环境阻碍）

**本交接不作为 T251 关闭新门**；报告如 FAIL/BLOCKED 归属 architect 首次归因（本卡结论：BLOCKED 根因=缺接点，非 tester/环境）。
