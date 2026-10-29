# T277 V2 同 run 落床锚点续证 · 报告 v1

- 角色：tester（真机 VYG-AL00 62T0225B10010020）· 日期：2026-10-29 · 状态：**BLOCKED（设备电量 3% 低电保护冻结，单一 run 无法跨过首生→8s 窗口；工具链与早期封存已验证，交 T274 architect 并待充电后重跑）**
- 被测包：**T276-v5-signed.hap**（SHA `e9c0b37c66b6013849f12292d2bc3b2b128fe98e4f522d79447e9d6361885380`，源 `5a06b06`）
- 页面：`pages/P051DerivedV2Demo`（`--ps p051v2 1 --ps t266heavy true`）
- 归档：`test/reports/t277/`（cold_raw{,2}.txt 分片原文、reassembled{,2}/ 含 index.json 与 COMPLETE 档 6 份）

## T267 缺陷 A 修复确认（静态核对 + 首轮运行）

T276 的 `T266Clock.ets`：`get secondsSinceFirstBirth():number {return this.firstBirthCommit<0?NaN:this.acceptedPhysicsSeconds-this.birthClock;}` ——**差值法实现**，即 T267 报告建议的修法之一；`birthClock` 在首生时锁定 `acceptedPhysicsSeconds` 快照。首轮 run（pid 16474）中 `T276_COLD_V1` 大对象正常分片导出（11968 行），clock 字段随 run 绑定/重置语义正确（`bind()` 清零 birthClock）。**缺陷 A 修复成立**（本日真机未见 since 冻结现象复现机会，以实现核对+早期帧运行佐证）。

## T276 分片导出/重组工具链 —— **PASS（设备端真实验证）**

- 设备端真机产生 `T276_COLD_V1` UTF16 分片（每片 ≤256 单元、JSON ≤1800 字符、FNV-1a32 摘要），hilog 抓取 12017/17098 行。
- `reassemble.cjs` 正确执行：`invalidPhysicalLines=0`；5/9 个 exportId 重组，**4 个 COMPLETE（含 SHA256）**、2 个 INCOMPLETE（MISSING_PART，缺片如实标注，无假成功路径）——**缺片/截断显式 INCOMPLETE 合同真机生效**。
- COMPLETE 档内容核对：`T248_LAND`（commit28/106，events 页）、`T248_STEP`（substeps=2/substepDt=1/30/stepMs 实测）、`PACK|frame13` 与 `PACK|frame35`（2048 float 全量：**列区 0–511 全 0 = 床四量 0**；512 起粒子条目 mass=0.5×160 条在场下落）、`T266_FRAME|frame36/37`（完整主档含 runClock/publication/boundedTrace/landWriter/pixel 五链字段）。

## 锚点执行状态

| 锚点 | 状态 | 说明 |
|---|---|---|
| 单一新 run 首生基线 | ⚠️ 部分 | 完整主档帧 36/37 在**无源期**（firstBirthCommit=null、sourceRequestedSeconds=0）——cold 导出触发早于注入；带源帧的 T266_FRAME 主档均 MISSING_PART（进程在导出中途被冻结杀死） |
| 首生后成功物理秒 ≥8 | ❌ BLOCKED | 见下 |
| LAND writer→床四量 | ⚠️ 工具就绪未触发 | writer 结构验证（fields/rows/dropped 框架完整）；bedDelta 全 0（床未触底，T249/T267 同线） |
| 床量→publication→packed→同 token 三 CURRENT/像素 | ⚠️ 链路字段齐备 | 主档五字段（publication/packed/pixel/landWriter/boundedTrace）同 token 联结结构完整；床 0 无像素可采 |

## 阻断根因（进程级/电量级证据）

- 设备电量 **3%**（`hidumper BatteryService: batteryLevel 3, pluggedType 2, chargeType 1`），USB 慢充 10 分钟无增长。
- 低电量 Doze 对前台应用周期性 cgroup 冻结（T273 已证 `ps` CPU TIME 冻结不动）；本轮三台"新 pid"（17681/11436/24189）均在冷启后数十秒内死亡或冻结，**无一个 run 能连续运行跨过首生→8s 物理秒窗口**。
- 已尝试：power-shell timeout 900000、wakeup 循环、设备本地保活脚本、aa start 前台拉起、缩短注入窗口抢跑——均被低电冻结覆盖。

## 继续条件与交接

1. **设备充电至 ≥20%**（或换稳定供电）后重跑本卡同一流程即可——包/工具链/流程全部就绪，预期一次会话可收齐首生基线、since≥8s 锚点、LAND writer 与床/packed/像素链。
2. cold 导出触发时机（heavy 臂在 early 帧自动导出 T266_FRAME）导致带源主档被冻结截断——建议 architect 裁决是否将自动 cold 导出延后到锚点帧（或加 cwdiagFrame 式手动选帧），避免低存活窗口浪费。
3. T267 缺陷 A 修复（差值法）建议 architect 确认关闭。

## 边界（如实）

- 不以多 run 拼接冒充同 run 首生后秒；未达锚点如实 BLOCKED。
- 不把 COMPLETE 档（无源期）冒充带源锚点证据；PACK 床 0 如实记录。
- T272（GPU 线）与 T276（V2 线）设备阻塞同源（低电 Doze），两卡合并待设备恢复后各自重跑。
