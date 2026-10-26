# T94 双场卡死修复后复测执行案

日期：2026-10-26。责任：effector。任务卡：6adf1e5ee4b06ee357420099。  
基线：worktree @ `52a0700`（T97 独立 7/7 PASS 的 T96 池优化版）；树指纹 `926b87fce4e7842b` 与 c0c58c3 相同（T97 仅加 test/，entry/src 零差异）；HAP s146 `6350fc8b…`，modules.abc sha256 `165f5868…` 与 s145 逐字节一致（代码等价已验）。  
状态：**待执行**。

## 0. 目标与边界

1. **主目标**：T78 卡死操作序列在 T96 优化版上**真机不冻结**（三臂稳定性）+ **真实注入准入**（gesture_sand 收据非零，对照 T78 的 0 收据）+ **双流 CLOSED**（T66 收尾两路）。
2. **卡死判据**：`THREAD_BLOCK`/`APP_INPUT_BLOCK`/`appfreeze` 关键词零命中；帧循环持续（WS_METRIC 间隔正常）；进程存活（截图非桌面）。
3. **code5 披露**：若出现按 T103 机制记 invalid 交 T109（COLLECT99 归因线），不阻塞稳定性判定主目标（但如实记录）。
4. 不可改产品代码；同机串行（T103 已释放设备）。

## 1. 场景矩阵（复现 T78 卡死序列 + 稳定性加时）

| 臂 | 操作 | 时长 | 判定 |
|---|---|---|---|
| R0 | 冒烟：双场页可达、present、池诊断按钮开 | 15s | 页面活 |
| R1 | shake=1 + 3×swipe（T78 原卡死序列）+ 25s 静置 | ~50s | 不冻结 + 准入收据 |
| R2 | shake=0.6 + 1×swipe + 15s | ~35s | 不冻结 |
| R3 | shake=0 + 2×swipe + 25s（T78 R3 序列+落床加时） | ~55s | 不冻结 + 落床序列 |
| R4 | 前台"结束采集"按钮 finish → CLOSED → 拉双流 → manifest | 收尾 | CLOSED COMPLETE |

每臂开始 `hilog -r`；R1–R3 全程每 5s 连拍（卡死即桌面可见）；臂末 dump 全 hilog（freeze 关键词全文搜索，不只 WaterSand tag）。

## 2. 判据

| # | 判据 | 通过 |
|---|---|---|
| A1 | 稳定性 | 三臂零 THREAD_BLOCK/appfreeze；进程存活到收尾 |
| A2 | 真实准入 | R1/R3 `gesture_sand` 收据 admitted>0、sourceEpoch 增长（对照 T78 的 0） |
| A3 | 可见沙 | 截图像素（床层+粒子） |
| B1 | 双流 CLOSED | 前台按钮 finish → P011_OBS_STORE_CLOSED status=COMPLETE，计数=文件行数 |
| B2 | manifest | p011_capture_manifest.py 输出 COMPLETE |
| C1 | 落床 | depositedMass 时间序列收敛（≥20s 窗） |
| D1 | code5 | 出现记 invalid 交 T109，不中断采集 |
| E1 | 帧健康 | WS_FRAME 无 >1s 间隙 |

## 3. 工具与交付

`tools/t94_dualfield_stability.ps1` + `tools/analyze_t94.py`；产物 `test/artifacts/t94-dualfield-stability-20261026/`；回执 `docs/effect-report-P011-T94-dualfield-stability-v1.md`（对 T78 对比）；卡评论直发滴答。
