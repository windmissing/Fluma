# T78 双场采集修复后复测执行案

日期：2026-10-26。责任：effector。任务卡：6adeedcee4b0127832ab780e。  
基线：worktree @ `114a7ee`（同 T82，含 T66 身份收尾 545f45e + T56 scene），树指纹 `403b08514017b326`，HAP s144 `af049753…`（已装机）。  
状态：**待执行**。

## 0. 目标与边界

1. **验证 T66 采集收尾**在设备闭环：完整新管线**可见沙**（高度场沉积沙 + 真实粒子）+ **双流 CLOSED 证据**（ledger/admissions 两文件完整收尾）。
2. T44 遗留三项（无可见沙、缺 CLOSED、重复 sequence）中本卡覆盖前两项；重复 sequence 已被 T65/T66 排除（完整键唯一、原序逐位一致）。
3. **finish 入口**（T66 交付）：前台"结束采集"按钮（position 20,112）或 onNewWant 三字段 `--ps p011finish <req> --ps p011finishrun <runId> --ps p011finishgeneration <gen>`；混用业务参数拒绝。CLOSED 须与两流 written/sent 严格相等。
4. **manifest 工具**：`tools/p011_capture_manifest.py ledger admissions fence.json output.json` —— 只有严格 CLOSED、计数/末条身份一致才 COMPLETE。
5. code5 记 invalid 交 T90（T82 已给设备复现样本；本卡若再现同样记录）；不可改产品代码。
6. 设备串行：T82 已释放，本卡独占；完成后 T57。

## 1. 场景矩阵

| 臂 | 目的 | 时序 |
|---|---|---|
| R0 | 冒烟：rendermode=4 双场页可达、scene present 状态、旧臂不受 finish 影响 | 冷启 `rendermode 4 testmode true p011particle 1 p011capture 1 wssnapshot false shake 0` → 12s → dump |
| R1 | 注入+摇晃可见沙：gesture 注沙 + shake 让粒子扬起落床 | 冷启 shake=1 → 3 次全宽 swipe 注沙 → 25s 静置（≥T82 建议的 20s 落床窗）→ 截图×3 → **前台"结束采集"按钮 click** → 等 CLOSED → 拉 ledger/admissions → manifest |
| R2 | Want finish 路径 | 冷启 shake=0.6 → swipe 注沙 → 15s → `aa start --ps p011finish T78R2 --ps p011finishrun <runId> --ps p011finishgeneration <gen>` → CLOSED → manifest |
| R3 | 落床时间序列（T82 缺口补证） | 冷启 shake=0 → swipe 注沙 2 次 → 连拍 0s/5s/10s/20s/25s + 每 5s 抓 WS_METRIC 段 → finish → manifest；判据：depositedMass 时间序列是否收敛（粒子是否真的落床） |

每臂记录：`P011_OBS_STORE`（filePath/admissionFilePath/endpointClockId）、`P011_OBS_STORE_CLOSED`（records/admissions/status=COMPLETE）、`particle_route`、`WS_METRIC`（metricCommitKey/particleMass/depositedMass/sourceEpoch）、`P011_RASTER_LAYOUT`（盒区）、截图。

## 2. 判据

| # | 判据 | 通过条件 |
|---|---|---|
| A1 | 双场页渲染 | present code=0 无 NEWPIPE_FAULT/STALE 常态；状态文本非故障 |
| A2 | 可见沙 | **截图像素**：床层（沉积沙）+ 粒子（亮色）均可见；对比 R0 空场基线 |
| B1 | 双流完整 | CLOSED status=COMPLETE 且 records/admissions == 本地两文件行数 |
| B2 | manifest COMPLETE | `p011_capture_manifest.py` 输出 COMPLETE（bytes/SHA256/行数/末条身份一致） |
| B3 | 身份 | ledger runId == store 事件 runId == finish runId；无重复 sequence（spot check） |
| C1 | 落床序列 | depositedMass 随时间非降且趋稳（收敛即落床成立；持续 0 → 粒子不落床缺陷交 T59/T90 线） |
| D1 | code5 | 出现则该 run 相关项 invalid，记录交 T90 |

## 3. 工具与交付

- `tools/t78_dualfield_retest.ps1`（三臂+finish 两路+manifest）+ `tools/analyze_t78.py`（CLOSED/身份/落床序列/像素审计）。
- 产物 `test/artifacts/t78-dualfield-retest-20261026/`；回执 `docs/effect-report-P011-T78-dualfield-retest-v1.md`。
- 完成：评论"已完成"+入库；清单行状态留 manager。
