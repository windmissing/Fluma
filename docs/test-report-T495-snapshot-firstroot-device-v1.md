# T495 · GPU 快照首 root 与显式源链真机补证（v1：M1 首 root 期限独立量化 250.37ms 超期确认；显式两次新空场 reset 链与 source 动作链真机在位；上屏黑屏同族确认，物理推进证据链完整）

2026-11-13，tester，真机 62T0225B10010020（本时段设备独占）。被测=T492 双签包（commit `269ea0d`）：M1 `a87579df0500e12a3b902a54d6e8b30d3f47d2b432bc397109e52ebd337818ef`、M2 `de742561b49ea83c85c6a4f19b1bb7ae12b3dee239da3e8965a9ee9d327490b8`（与 `test/reports/t257/T492-*-handoff.json` 逐位一致，verifyApp=PASS）。证据：`test/reports/t491/t495/`（17 截图 + 8 份启动诊断 JSON）。

## 一、来源与 stage/snapshot 身份核（M1/M2 双包）

- 安装成功、冷启 `--ps grsm1 true --ps gpusha <包SHA>`；t456-boot 诊断 scope 带 commit `269ea0d` 域（M1 诊断 boot-cold.json / boot-reset1/2.json、M2 boot-m2-*.json 全部落盘可溯）。
- T493 已独立核每 manifest `source=dcf105d…+T492-overlay-v1`；本卡设备侧诊断与同源一致（boot JSON bytes/sha256 入 hilog WS_EVENT 收据）。

## 二、真实 pointerDown 源动作链（显式）

- M1 冷启 run 0:1 后两次 (147,263) pointerDown 投沙 → 页面 `最近源收据#0.0000 · 物理 0:1`（SOURCE 被接受计数=1，第一手沙落）；状态推进至 GPU_PROGRESS_TIMEOUT。物理=0:1 为**首次 root 接受后的 physics commit**（此前全程 0:0）——显式源动作→Offer→accepted SOURCE→Q 链真机走通。
- M2 冷启/暖启各执行同源点击序列；暖启物理 747→1173→1892 持续推进。

## 三、首 root 时域（ns→ms 明确，M1 独立量化）

boot-reset1.json attempt3 firstFailure 原始字段（独立计算）：

| 字段 | ns | 值 |
|---|---|---|
| a（intent） | 21358571055156 | — |
| b（deadline） | 21358821055156 | **b−a = 250.0 ms 整**（期限域确认） |
| c（detected） | 21358821445468 | **c−b = 0.390 ms**（超期检出延迟） |
| ROOT_INTENT | 21358571071093 | 与 a 差 0.016 ms |
| **首错 V2_GPU_PROGRESS_TIMEOUT** | 21358821445937 | **ROOT_INTENT→首错 250.37 ms** |

与 T490 裁定单位（245.52ms 族）一致：真机首 root GPU 队列耗时结构性超 250ms 期限，非单位换算错误。超期后页面保持 STALE/GPU_PROGRESS_TIMEOUT，物理不再推进（0:1 冻结）——**晚 pulse 不改首因**（页面 sample 12186→21328 期间 reason 恒定）。

## 四、显式两次新空场（reset 链）

- M1：GAP/超期后两次 (731,263) 新空场 → 两份独立诊断（boot-reset1/2.json，bytes 均为 53876、hilog SHA `0e4f7552…` 两份同 hash 亦各自独立落盘）；每次 attempt3 重走 SURFACE_CREATED→RESET_REQUIRED→session 1。
- M2：冷启+暖启各两次新空场（boot-m2-reset1/2/rebind.json 共四份）；GAP 后 rebind 也重走完整三 attempt——**旧 frame 不能发、新 frame 重绑**在真机成立（与 T492 controls case18 一致）。

## 五、上屏（如实：黑屏，物理推进证据链完整）

| 路线 | 观测 |
|---|---|
| M1 暖启（boot-warm.json 由 T491 报告覆盖）+本卡 M1 冷启 | READY 后/冷启后均物理 0:1 后即 V2_GPU_PROGRESS_TIMEOUT（首 root 超期同族），黑屏 |
| M2 暖启（pid 22680） | **物理 747→1173→1892 持续推进约 4 分钟**（DRAW_PENDING→GPU_WORK_PENDING），15 个 ROOT_INTENT/1670ms（root 提交活跃）；最终 V2_PULSE_CLOCK_GAP（root 2156，resourceGAP 撤权），黑屏 |
| M2 rebind | GAP 后新空场重走三 attempt，仍 FAULTED_REVOKED |

**M2 暖启物理长链推进（747→2155，约 4 分钟）是迄今最长的真机 compute 持续运行证据**：缓存命中下 compute/物理链健康、root 提交活跃；黑屏根因在 present/Swap 域（T448/T490 归因域），且 GAP 终态由真实 pulse 域触发（非本卡注入）。按卡面规则不以中位数排除 GAP、不冒上屏。

## 六、NOT_MEASURED（不冒通过）

- 系统 PNG 与层画面比对、30fps：无有效帧（黑屏）未达。
- video/actionSerial 录制链：本轮以诊断 JSON+截图替代录屏（录制时序与 GAP 触发窗口冲突），actionSerial 语义由 T492 controls case18 离线已证；如 architect 要求真机录像补证请回派。
- snapshot 跨 88/120 边界活跃期导出：T493 离线已证（page-0..124 跨边界）；设备端 snapshot 导出依赖页面菜单，GAP 态下导出按钮仍在（已点击验证 t456-boot 导出成功），快照窗口导出未在活跃期达成（同黑屏域）。
- 缓存/源/完整性/30fps 分项：缓存项 T491 已证；完整性 T493 已证；30fps NOT_MEASURED。
