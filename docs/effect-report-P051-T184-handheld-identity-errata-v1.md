# P051 T184 手持采集身份勘误回执 v1

日期：2026-10-27。责任：effector。任务卡：`6ae0b709e4b0ab6770126264`。消费：T181 `67ce141`/`architecture-T181-handheld-run-identity-routing-v1.md`、T177 `267f519` 报告与四 dump、run-console.log、T166 build-manifest。  
状态：**EXECUTOR_REVISED / EXISTING_SEARCH_DONE / FIRST_ERROR_NOT_CAPTURED / OFFLINE_NO_DEVICE**。  
交付三件：采集器 v2（T181 §4.1 合规）、既存检索 JSON（首错 NOT_CAPTURED 定论）、T177 报告版本化勘误（E1–E3）。

## 1. 采集器 v2（`tools/t177v2_capture.ps1`，按 T181 §4.1 全项落实）

| T181 要求 | v2 实现 |
|---|---|
| 冷启前冻结旧 PID/run 并归档旧尾 | step1：`pidof` 读旧 PID → **force-stop 前**全量 dump `pre-freeze-old-tail-full.txt` → 提取旧 runId 入 state |
| 记录命令 stdout/stderr/退出码 | `Run-Cmd` 包装全部 hdc 调用 → `cmd-audit.log`（DESC/EXIT/CMD/OUT） |
| 启动后立即读新 PID、run/config READY | step3：冷启 → `pidof` 新 PID → `post-start-identity-full.txt` → 提取 newRunId |
| 新 PID+run 过滤窗；hilog reset 不是归属证据 | step6 终态校验 `endPid==newPid && endRun==newRun`；分析期只计双重匹配事件 |
| REUSED_RUN 显式 | newRun==oldRun 时立即中止姿态窗口并落 `capture-state.json`（runStatus=REUSED_RUN） |
| PREEXISTING_FAILED 显式 | step4 冷启静置 8s 后按新 PID 过滤查 `page-physics-failed`，命中则跳过触摸窗口（runStatus=PREEXISTING_FAILED） |
| 同进程 newrun 按 run 分界 | state 记录 oldRun/newRun/endRun 三段身份；保留两段 commit/diagnostic sequence 供分界 |
| 不反复运行无效触摸宽矩阵 | PREEXISTING_FAILED/REUSED_RUN 分支直接退出，不跑姿态 |

（设备窗口尚未执行——v2 属 T187 窄补证的执行器前置，按 T181 §4.3 由该卡到件后使用。）

## 2. 既存检索（T181 §4.3"先请原执行者提供已存在的旧 run 首错日志/原码/manifest"）

检索结果落盘 `test/artifacts/t177-real-handheld-20261027/t184-existing-evidence-search.json`：

| 源 | 结果 |
|---|---|
| 本地四 dump（A/B/B2/B3，共 4.25MB） | **首错锚点全 NONE**（`first_code5_trace`/`getLastError`/`FIELD_COLUMN`/`firstFailure` 均 0 命中——与 T181 §2 一致） |
| 设备 faultlogger T177 时段（19:2x） | **无文件**（无 appfreeze/crash） |
| 设备 /data/local/tmp | 仅截图，无 hilog 持久文件 |
| source manifest | **到件**：`test/reports/t166/build-manifest.json`（T166_BUILD_V1，baseCommit `e2163d4`，Hot/EvidenceV2 等 5 文件 SHA256）——T177 时段设备运行包身份经 s150 `f28ec28e…` 溯源到该 manifest |

**结论：旧 run（PID 21223 / `p011-run-1793100180197-3`）首错原码 NOT_CAPTURED**——既存材料不可恢复，不追溯构造原 bits；首错补证走 T185 V2 诊断 → T186 独立测试 → T187 窄补证（锁 PID 锁 run 一次姿态触摸窗口）。

## 3. T177 报告版本化勘误（E1–E3，已写入原报告顶部，原观察保留）

- **E1**：B3"新冷启 318ms 内 12 次失败后自行恢复"不成立——12 条属旧进程（PID 21223）force-stop 前的 stop 通知重入缓冲；新 run（PID 24096）有效段 page-physics-failed=0。"同 failed 对象自行恢复"无证据；旧失败是另一条真实物理失败（不归 T146/不猜 shake 因果）。
- **E2**：installTime=1792130688871 ≠ "2026-10-27 16:31"（Unix ms 换算为 10-16 14:04）；字段语义未定，保留原值；包身份改由 s150 SHA256 + T166 manifest 链承担。
- **E3**：传感器左右标注与 adapter 符号规则（`atan2(-filteredX, filteredY)`）不一致；effectiveTilt 跟随结论不变，不得按截图文字重标传感器轴。

其余结论（真实手持执行、5 START ACCEPTED、15 准入 11.21、目视未确认、SCENE_MATERIAL_DATA 阻碍）经 T181 复核保留。

## 4. 执行器工程问题清单（T181 §4.1 已确认项，v2 已全部内建）

1. 0 位移 swipe 不派发事件（T95/B1 两次重犯）→ v2 触摸全部微位移（700→715）。
2. 非冷启 `aa start` 复用 failed run（B2 无效）→ v2 强制 force-stop 冷启 + REUSED_RUN 守卫。
3. hilog -r 清缓冲后 force-stop 重写旧日志 → v2 归属不依赖 hilog reset，改 PID+run 双过滤。
4. Check-Alive 全局 grep 未过滤 PID（T94 R3 误判根源）→ v2 Check 按新 PID 过滤并留 raw 文件。

## 5. 边界与移交

- 离线交付，未占设备；v2 采集器待 T187 窄补证使用（其前置 T185/T186 未到件）。
- 旧 run 首错 NOT_CAPTURED 维持 T181"旧真实失败仍未定位"结论；不改"仅执行器错误已解决"。
- T177 派生报告勘误已按版本化处理，原始观察/产物零删除。
- 滴答 T184 卡评论已发；清单行状态留 manager。
